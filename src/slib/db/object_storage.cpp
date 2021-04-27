/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#include "slib/db/object_storage.h"

#include "slib/db/leveldb.h"

#include "slib/core/json.h"
#include "slib/core/serialize.h"

#define KEY_LENGTH_MAX 1024
#define KEY_BUFFER_SIZE (KEY_LENGTH_MAX + 16)
#define VALUE_BUFFER_SIZE 1024
#define OBJECT_PREFIX_BUFFER_SIZE 128
#define REMOVE_BATCH_SIZE 1024
#define KEY_NAME_STORAGE_OBJECT_LAST_PREFIX "storage_object_last_prefix"

namespace slib
{

	namespace priv
	{
		namespace object_storage
		{

			static sl_uint32 PrepareKey(sl_uint8* _out, sl_uint64 prefix)
			{
				return CVLI::serialize(_out, prefix);
			}

			static sl_uint32 PrepareKey(sl_uint8* _out, sl_uint64 prefix, const StringParam& _key)
			{
				StringData key(_key);
				sl_size len = key.getLength();
				if (!len) {
					return 0;
				}
				if (len > KEY_LENGTH_MAX) {
					return 0;
				}
				sl_uint32 nPrefix = CVLI::serialize(_out, prefix);
				Base::copyMemory(_out + nPrefix, key.getData(), len);
				return nPrefix + (sl_uint32)len;
			}

			static void AppendObjectKeySuffix(sl_uint8* _out, sl_uint32& n)
			{
				_out[n] = 0;
				_out[n + 1] = 1;
				n += 2;
			}

			static sl_bool RemoveObjectKeySuffix(const sl_uint8* _in, sl_uint32& n)
			{
				if (n >= 3) {
					if (!(_in[n - 2]) && _in[n - 1] == 1) {
						n -= 2;
						return sl_true;
					}
				}
				return sl_false;
			}

			sl_uint64 DeserializeObjectPrefix(const void* _data, sl_size size)
			{
				if (size < 3) {
					return 0;
				}
				sl_uint8* data = (sl_uint8*)_data;
				if (*data || data[1] != (sl_uint8)255) {
					return 0;
				}
				sl_uint64 prefix = 0;
				SerializeBuffer sb(data + 2, size - 2);
				if (CVLI::deserialize(&sb, prefix)) {
					return prefix;
				}
				return 0;
			}

			class ObjectStorageImpl : public ObjectStorage
			{
			public:
				static Ref<ObjectStorageImpl> open(const ObjectStorageParam& param)
				{
					Ref<KeyValueStore> store = param.store;
					if (store.isNull()) {
						LevelDB::Param kvsParam;
						kvsParam.path = param.path;
						store = LevelDB::open(kvsParam);
						if (store.isNull()) {
							return sl_null;
						}
					}
					Ref<ObjectStorageImpl> ret = new ObjectStorageImpl;
					if (ret.isNotNull()) {
						ret->m_store = Move(store);
						return ret;
					}
					return sl_null;
				}

			public:
				Variant createObject(const StringParam& key) override
				{
					return createObjectItem(0, key);
				}

				Variant createObject(const Variant& _parent, const StringParam& key) override;

				Variant getItem(const StringParam& key) override
				{
					return getItem(0, key);
				}

				sl_bool putItem(const StringParam& key, const Variant& value) override
				{
					return putItem(0, key, value);
				}

				sl_bool removeItem(const StringParam& key) override
				{
					return removeItem(0, key);
				}

				PropertyIterator getItemIterator() override
				{
					return getItemIterator(0);
				}

			public:
				Variant createObjectItem(sl_uint64 prefix, const StringParam& key);

				Variant getItem(sl_uint64 prefix, const StringParam& key);

				sl_bool putItem(sl_uint64 prefix, const StringParam& _key, const Variant& value)
				{
					sl_uint8 key[KEY_BUFFER_SIZE];
					sl_uint32 nKey = PrepareKey(key, prefix, _key);
					if (!nKey) {
						return sl_false;
					}
					return m_store->put(StringView((char*)key, nKey), value);
				}

				sl_bool removeItem(sl_uint64 prefix, const StringParam& _key)
				{
					sl_uint8 key[KEY_BUFFER_SIZE];
					sl_uint32 nKey = PrepareKey(key, prefix, _key);
					if (!nKey) {
						return sl_false;
					}
					sl_bool bRet = sl_false;
					if (m_store->remove(StringView((char*)key, nKey))) {
						bRet = sl_true;
					}
					AppendObjectKeySuffix(key, nKey);
					sl_uint64 prefixSub = getObjectPrefix(key, nKey);
					if (prefixSub) {
						if (removeObject(prefixSub)) {
							if (m_store->remove(StringView((char*)key, nKey))) {
								return sl_true;
							}
						}
					}
					return bRet;
				}

				PropertyIterator getItemIterator(sl_uint64 prefix);

				sl_uint64 getObjectPrefix(sl_uint8* key, sl_uint32 nKey)
				{
					char buf[OBJECT_PREFIX_BUFFER_SIZE];
					sl_reg n = m_store->get(key, nKey, buf, sizeof(buf));
					if (n > 0) {
						return DeserializeObjectPrefix(buf, n);
					}
					return 0;
				}

				sl_bool removeObject(sl_uint64 prefix)
				{
					sl_uint8 key[KEY_BUFFER_SIZE];
					sl_uint32 nKey = PrepareKey(key, prefix);
					sl_uint8 subKey[KEY_BUFFER_SIZE];
					for (;;) {
						Ref<KeyValueIterator> iterator = m_store->getIterator();
						if (iterator.isNull()) {
							return sl_false;
						}
						if (!(iterator->seek(key, nKey))) {
							break;
						}
						sl_size nRemoved = 0;
						List< Pair<Memory, sl_uint64> > listObject;
						{
							Ref<KeyValueWriteBatch> batch;
							do {
								sl_reg n = iterator->getKey(subKey, sizeof(subKey));
								if (n <= (sl_reg)nKey) {
									break;
								}
								if (!(Base::equalsMemory(key, subKey, nKey))) {
									break;
								}
								sl_bool flagRemoveObject = sl_false;
								sl_uint32 nSubKey = (sl_uint32)n;
								if (RemoveObjectKeySuffix(subKey, nSubKey)) {
									char value[OBJECT_PREFIX_BUFFER_SIZE];
									sl_reg nValue = iterator->getValue(value, sizeof(value));
									if (nValue > 0) {
										sl_uint64 prefix = DeserializeObjectPrefix(value, nValue);
										if (prefix) {
											Memory memKey = Memory::create(subKey, n);
											if (memKey.isNull()) {
												return sl_false;
											}
											if (listObject.add_NoLock(Pair<Memory, sl_uint64>(Move(memKey), prefix))) {
												flagRemoveObject = sl_true;
											} else {
												return sl_false;
											}
										}
									}
								}
								if (!flagRemoveObject) {
									if (batch.isNull()) {
										batch  = m_store->createWriteBatch();
										if (batch.isNull()) {
											return sl_false;
										}
									}
									if (!(batch->remove(subKey, n))) {
										return sl_false;
									}
								}
								nRemoved++;
								if (nRemoved > REMOVE_BATCH_SIZE) {
									break;
								}
							} while (iterator->moveNext());
							if (batch.isNotNull()) {
								if (!(batch->commit())) {
									return sl_false;
								}
							}
						}
						sl_size nObjects = listObject.getCount();
						Pair<Memory, sl_uint64>* objects = listObject.getData();
						{
							for (sl_size i = 0; i < nObjects; i++) {
								if (!(removeObject(objects[i].second))) {
									return sl_false;
								}
							}
						}
						if (nObjects) {
							Ref<KeyValueWriteBatch> batch = m_store->createWriteBatch();
							if (batch.isNull()) {
								return sl_false;
							}
							for (sl_size i = 0; i < nObjects; i++) {
								if (!(batch->remove(objects[i].first.getData(), objects[i].first.getSize()))) {
									return sl_false;
								}
							}
							if (!(batch->commit())) {
								return sl_false;
							}
						}
						if (!nRemoved) {
							break;
						}
					}
					return sl_true;
				}

			};

			class StorageObject : public Object
			{
			public:
				Ref<ObjectStorageImpl> m_storage;
				sl_uint64 m_prefix;

			public:
				StorageObject(ObjectStorageImpl* storage, sl_uint64 prefix): m_storage(storage), m_prefix(prefix) {}

			public:
				Variant getProperty(const StringParam& name) override
				{
					return m_storage->getItem(m_prefix, name);
				}

				sl_bool setProperty(const StringParam& name, const Variant& value) override
				{
					return m_storage->putItem(m_prefix, name, value);
				}

				sl_bool clearProperty(const StringParam& name) override
				{
					return m_storage->removeItem(m_prefix, name);
				}

				PropertyIterator getPropertyIterator() override
				{
					return m_storage->getItemIterator(m_prefix);
				}

			};

			Variant ObjectStorageImpl::createObject(const Variant& _parent, const StringParam& key)
			{
				if (_parent.isNull()) {
					return createObjectItem(0, key);
				}
				Ref<Object> refParent = _parent.getObject();
				if (refParent.isNotNull()) {
					StorageObject* parent = (StorageObject*)(refParent.get());
					return createObjectItem(parent->m_prefix, key);
				}
				return Variant();
			}

			Variant ObjectStorageImpl::createObjectItem(sl_uint64 prefixParent, const StringParam& _key)
			{
				sl_uint8 key[KEY_BUFFER_SIZE];
				sl_uint32 nKey = PrepareKey(key, prefixParent, _key);
				if (nKey) {
					AppendObjectKeySuffix(key, nKey);
					ObjectLocker lock(this);
					sl_uint64 prefixSub = getObjectPrefix(key, nKey);
					if (prefixSub) {
						return Variant::fromObject(new StorageObject(this, prefixSub));
					}
					Ref<KeyValueWriteBatch> batch = m_store->createWriteBatch();
					if (batch.isNotNull()) {
						sl_uint64 prefixNew = m_store->get(StringParam::literal(KEY_NAME_STORAGE_OBJECT_LAST_PREFIX)).getUint64();
						prefixNew++;
						if (batch->put(StringParam::literal(KEY_NAME_STORAGE_OBJECT_LAST_PREFIX), prefixNew)) {
							sl_uint8 buf[OBJECT_PREFIX_BUFFER_SIZE];
							buf[0] = 0;
							buf[1] = 255;
							sl_uint32 size = 2 + CVLI::serialize(buf + 2, prefixNew);
							if (batch->put(key, nKey, buf, size)) {
								if (batch->commit()) {
									return Variant::fromObject(new StorageObject(this, prefixNew));
								}
							}
						}
					}
				}
				return Variant();
			}

			Variant ObjectStorageImpl::getItem(sl_uint64 prefix, const StringParam& _key)
			{
				sl_uint8 key[KEY_BUFFER_SIZE];
				sl_uint32 nKey = PrepareKey(key, prefix, _key);
				if (!nKey) {
					return Variant();
				}
				Variant value = m_store->get(StringView((char*)key, nKey));
				if (value.isNotUndefined()) {
					return value;
				}
				AppendObjectKeySuffix(key, nKey);
				sl_uint64 prefixSub = getObjectPrefix(key, nKey);
				if (prefixSub) {
					return Variant::fromObject(new StorageObject(this, prefixSub));
				}
				return Variant();
			}

			class StorageItemIterator : public CPropertyIterator
			{
			public:
				Ref<ObjectStorageImpl> m_storage;
				Ref<KeyValueIterator> m_kvIterator;
				
				sl_bool m_flagFirstMove;
				sl_uint8 m_prefix[KEY_BUFFER_SIZE];
				sl_uint32 m_nPrefix;
				String m_key;

			public:
				StorageItemIterator(ObjectStorageImpl* storage, sl_uint64 prefix, Ref<KeyValueIterator>&& iterator): m_storage(storage), m_kvIterator(Move(iterator)), m_flagFirstMove(sl_true)
				{
					m_nPrefix = PrepareKey(m_prefix, prefix);
				}

			public:
				String getKey() override
				{
					return m_key;
				}

				Variant getValue() override
				{
					char buf[VALUE_BUFFER_SIZE];
					MemoryData mem(buf, sizeof(buf));
					if (m_kvIterator->getValue(&mem)) {
						sl_uint64 prefix = DeserializeObjectPrefix(mem.data, mem.size);
						if (prefix) {
							return Variant::fromObject(new StorageObject(m_storage.get(), prefix));
						} else {
							return KeyValueReader::deserialize(Move(mem));
						}
					} else {
						return Variant();
					}
				}

				sl_bool moveNext() override
				{
					if (m_flagFirstMove) {
						m_flagFirstMove = sl_false;
						if (!(m_kvIterator->seek(m_prefix, m_nPrefix))) {
							return sl_false;
						}
					} else {
						if (!(m_kvIterator->moveNext())) {
							m_key.setNull();
							return sl_false;
						}
					}
					return validateKey();
				}

				sl_bool seek(const String& key) override
				{
					sl_size len = key.getLength();
					if (!len) {
						return sl_false;
					}
					if (len > KEY_LENGTH_MAX) {
						return sl_false;
					}
					sl_uint8 k[KEY_BUFFER_SIZE];
					Base::copyMemory(k, m_prefix, m_nPrefix);
					Base::copyMemory(k + m_nPrefix, key.getData(), len);
					if (m_kvIterator->seek(k, m_nPrefix + len)) {
						m_flagFirstMove = sl_false;
						return validateKey();
					} else {
						m_key.setNull();
						return sl_false;
					}
				}

				sl_bool validateKey()
				{
					sl_uint8 k[KEY_BUFFER_SIZE];
					sl_reg n = m_kvIterator->getKey(k, sizeof(k));
					if (n > 0) {
						sl_uint32 nKey = (sl_uint32)n;
						RemoveObjectKeySuffix(k, nKey);
						if (nKey > m_nPrefix) {
							if (Base::equalsMemory(k, m_prefix, m_nPrefix)) {
								m_key = String((sl_char8*)k + m_nPrefix, nKey - m_nPrefix);
								return m_key.isNotNull();
							}
						}
					}
					m_key.setNull();
					return sl_false;
				}

			};

			PropertyIterator ObjectStorageImpl::getItemIterator(sl_uint64 prefix)
			{
				Ref<KeyValueIterator> iterator = m_store->getIterator();
				if (iterator.isNotNull()) {
					return new StorageItemIterator(this, prefix, Move(iterator));
				}
				return sl_null;
			}

		}
	}

	using namespace priv::object_storage;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ObjectStorageParam)

	ObjectStorageParam::ObjectStorageParam()
	{
	}


	SLIB_DEFINE_OBJECT(ObjectStorage, Object)

	ObjectStorage::ObjectStorage()
	{
	}

	ObjectStorage::~ObjectStorage()
	{
	}

	KeyValueStore* ObjectStorage::getStore() noexcept
	{
		return m_store.get();
	}

	Json ObjectStorage::toJson()
	{
		Json ret;
		ret.setObject(this);
		return ret;
	}

	Variant ObjectStorage::getProperty(const StringParam& name)
	{
		return getItem(name);
	}

	sl_bool ObjectStorage::setProperty(const StringParam& name, const Variant& value)
	{
		return putItem(name, value);
	}

	sl_bool ObjectStorage::clearProperty(const StringParam& name)
	{
		return removeItem(name);
	}

	PropertyIterator ObjectStorage::getPropertyIterator()
	{
		return getItemIterator();
	}

	Ref<ObjectStorage> ObjectStorage::open(const ObjectStorageParam& param)
	{
		return Ref<ObjectStorage>::from(ObjectStorageImpl::open(param));
	}

	Ref<ObjectStorage> ObjectStorage::open(const StringParam& path)
	{
		ObjectStorageParam param;
		param.path = path;
		return open(param);
	}

}
