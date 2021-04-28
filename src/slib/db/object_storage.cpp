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

			class StorageObjectImpl;

			class ObjectStorageManagerImpl : public ObjectStorageManager
			{
			public:
				Ref<KeyValueStore> m_store;

			public:
				static Ref<ObjectStorageManagerImpl> open(const ObjectStorageParam& param)
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
					Ref<ObjectStorageManagerImpl> ret = new ObjectStorageManagerImpl;
					if (ret.isNotNull()) {
						ret->m_store = Move(store);
						return ret;
					}
					return sl_null;
				}

			public:
				Ref<KeyValueStore> getStore() override
				{
					return m_store;
				}

				Ref<StorageObject> createRootObject() override;
				
			public:
				Ref<StorageObject> createObject(sl_uint64 prefix, const StringParam& key);

				ObjectStorage getItem(sl_uint64 prefix, const StringParam& key);

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

				Iterator<String, ObjectStorage> getItemIterator(sl_uint64 prefix);

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

			class StorageObjectImpl : public StorageObject
			{
			public:
				Ref<ObjectStorageManagerImpl> m_manager;
				sl_uint64 m_prefix;

			public:
				StorageObjectImpl(ObjectStorageManagerImpl* manager, sl_uint64 prefix): m_manager(manager), m_prefix(prefix) {}

			public:
				Ref<ObjectStorageManager> getManager() override
				{
					return m_manager;
				}

				Ref<StorageObject> createObject(const StringParam& key) override
				{
					return m_manager->createObject(m_prefix, key);
				}

				ObjectStorage getItem(const StringParam& key) override
				{
					return m_manager->getItem(m_prefix, key);
				}

				sl_bool putItem(const StringParam& key, const Variant& value) override
				{
					return m_manager->putItem(m_prefix, key, value);
				}

				sl_bool removeItem(const StringParam& key) override
				{
					return m_manager->removeItem(m_prefix, key);
				}

				Iterator<String, ObjectStorage> getItemIterator() override
				{
					return m_manager->getItemIterator(m_prefix);
				}

			};

			Ref<StorageObject> ObjectStorageManagerImpl::createRootObject()
			{
				return new StorageObjectImpl(this, 0);
			}

			Ref<StorageObject> ObjectStorageManagerImpl::createObject(sl_uint64 prefixParent, const StringParam& _key)
			{
				sl_uint8 key[KEY_BUFFER_SIZE];
				sl_uint32 nKey = PrepareKey(key, prefixParent, _key);
				if (nKey) {
					AppendObjectKeySuffix(key, nKey);
					ObjectLocker lock(this);
					sl_uint64 prefixSub = getObjectPrefix(key, nKey);
					if (prefixSub) {
						return new StorageObjectImpl(this, prefixSub);
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
									return new StorageObjectImpl(this, prefixNew);
								}
							}
						}
					}
				}
				return sl_null;
			}

			ObjectStorage ObjectStorageManagerImpl::getItem(sl_uint64 prefix, const StringParam& _key)
			{
				sl_uint8 key[KEY_BUFFER_SIZE];
				sl_uint32 nKey = PrepareKey(key, prefix, _key);
				if (!nKey) {
					return ObjectStorage();
				}
				Variant value = m_store->get(StringView((char*)key, nKey));
				if (value.isNotUndefined()) {
					return value;
				}
				AppendObjectKeySuffix(key, nKey);
				sl_uint64 prefixSub = getObjectPrefix(key, nKey);
				if (prefixSub) {
					return new StorageObjectImpl(this, prefixSub);
				}
				return ObjectStorage();
			}

			class StorageItemIterator : public CIterator<String, ObjectStorage>
			{
			public:
				Ref<ObjectStorageManagerImpl> m_manager;
				Ref<KeyValueIterator> m_kvIterator;
				
				sl_bool m_flagFirstMove;
				sl_uint8 m_prefix[KEY_BUFFER_SIZE];
				sl_uint32 m_nPrefix;
				String m_key;

			public:
				StorageItemIterator(ObjectStorageManagerImpl* manager, sl_uint64 prefix, Ref<KeyValueIterator>&& iterator): m_manager(manager), m_kvIterator(Move(iterator)), m_flagFirstMove(sl_true)
				{
					m_nPrefix = PrepareKey(m_prefix, prefix);
				}

			public:
				String getKey() override
				{
					return m_key;
				}

				ObjectStorage getValue() override
				{
					char buf[VALUE_BUFFER_SIZE];
					MemoryData mem(buf, sizeof(buf));
					if (m_kvIterator->getValue(&mem)) {
						sl_uint64 prefix = DeserializeObjectPrefix(mem.data, mem.size);
						if (prefix) {
							return new StorageObjectImpl(m_manager.get(), prefix);
						} else {
							return KeyValueReader::deserialize(Move(mem));
						}
					} else {
						return ObjectStorage();
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

			Iterator<String, ObjectStorage> ObjectStorageManagerImpl::getItemIterator(sl_uint64 prefix)
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


	SLIB_DEFINE_OBJECT(StorageObject, Object)

	StorageObject::StorageObject()
	{
	}

	StorageObject::~StorageObject()
	{
	}

	Variant StorageObject::getProperty(const StringParam& name)
	{
		return getItem(name);
	}

	sl_bool StorageObject::setProperty(const StringParam& name, const Variant& value)
	{
		return putItem(name, value);
	}

	sl_bool StorageObject::clearProperty(const StringParam& name)
	{
		return removeItem(name);
	}

	PropertyIterator StorageObject::getPropertyIterator()
	{
		return PropertyIterator::from(getItemIterator());
	}


	SLIB_DEFINE_OBJECT(ObjectStorageManager, Object)

	ObjectStorageManager::ObjectStorageManager()
	{
	}

	ObjectStorageManager::~ObjectStorageManager()
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ObjectStorageParam)

	ObjectStorageParam::ObjectStorageParam()
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ObjectStorage)

	ObjectStorage::ObjectStorage() noexcept
	{
	}

	ObjectStorage::ObjectStorage(sl_null_t) noexcept: value(sl_null)
	{
	}

	ObjectStorage::ObjectStorage(StorageObject* object) noexcept: value(Variant::fromObject(object))
	{
	}

	ObjectStorage::ObjectStorage(const Ref<StorageObject>& object) noexcept: value(Variant::fromObject(object))
	{
	}

	ObjectStorage::ObjectStorage(Ref<StorageObject>&& object) noexcept: value(Variant::fromObject(Move(object)))
	{
	}

	Variant::Variant(const ObjectStorage& t) noexcept: Variant(t.value)
	{
	}

	Variant::Variant(ObjectStorage&& t) noexcept: Variant(Move(t.value))
	{
	}

	Json::Json(const ObjectStorage& t) noexcept: Json(t.value)
	{
	}

	Json::Json(ObjectStorage&& t) noexcept: Json(Move(t.value))
	{
	}

	Ref<ObjectStorageManager> ObjectStorage::getManager() const
	{
		Ref<StorageObject> object = getStorageObject();
		if (object.isNotNull()) {
			return object->getManager();
		}
		return sl_null;
	}

	ObjectStorage ObjectStorage::createObject(const StringParam& key) const
	{
		Ref<StorageObject> object = getStorageObject();
		if (object.isNotNull()) {
			return object->createObject(key);
		}
		return ObjectStorage();
	}

	sl_bool ObjectStorage::isStorageObject() const noexcept
	{
		if (value._type == VariantType::Referable || value._type == VariantType::Object) {
			Ref<Referable>& ref = *((Ref<Referable>*)(void*)&(value._value));
			return IsInstanceOf<StorageObject>(ref);
		}
		return sl_false;
	}

	Ref<StorageObject> ObjectStorage::getStorageObject() const noexcept
	{
		if (value._type == VariantType::Referable || value._type == VariantType::Object) {
			Ref<Referable>& ref = *((Ref<Referable>*)(void*)&(value._value));
			return CastRef<StorageObject>(ref);
		}
		return sl_null;
	}

	ObjectStorage ObjectStorage::getItem(const StringParam& key) const
	{
		Ref<StorageObject> object = getStorageObject();
		if (object.isNotNull()) {
			return object->getItem(key);
		} else {
			return value.getItem(key);
		}
	}

	sl_bool ObjectStorage::putItem(const StringParam& key, const Variant& value) const
	{
		Ref<StorageObject> object = getStorageObject();
		if (object.isNotNull()) {
			return object->putItem(key, value);
		}
		return sl_false;
	}

	sl_bool ObjectStorage::removeItem(const StringParam& key) const
	{
		Ref<StorageObject> object = getStorageObject();
		if (object.isNotNull()) {
			return object->removeItem(key);
		}
		return sl_false;
	}

	Iterator<String, ObjectStorage> ObjectStorage::getItemIterator() const
	{
		Ref<StorageObject> object = getStorageObject();
		if (object.isNotNull()) {
			return object->getItemIterator();
		}
		return sl_null;
	}

	sl_bool ObjectStorage::isUndefined() const noexcept
	{
		return value.isUndefined();
	}

	sl_bool ObjectStorage::isNotUndefined() const noexcept
	{
		return value.isNotUndefined();
	}

	sl_bool ObjectStorage::isNull() const noexcept
	{
		return value.isNull();
	}

	sl_bool ObjectStorage::isNotNull() const noexcept
	{
		return value.isNotNull();
	}

	sl_bool ObjectStorage::isInt32() const noexcept
	{
		return value.isInt32();
	}

	sl_bool ObjectStorage::getInt32(sl_int32* _out) const noexcept
	{
		return value.getInt32(_out);
	}

	sl_int32 ObjectStorage::getInt32(sl_int32 def) const noexcept
	{
		return value.getInt32(def);
	}

	sl_bool ObjectStorage::isUint32() const noexcept
	{
		return value.getUint32();
	}

	sl_bool ObjectStorage::getUint32(sl_uint32* _out) const noexcept
	{
		return value.getUint32(_out);
	}

	sl_uint32 ObjectStorage::getUint32(sl_uint32 def) const noexcept
	{
		return value.getUint32(def);
	}

	sl_bool ObjectStorage::isInt64() const noexcept
	{
		return value.isInt64();
	}

	sl_bool ObjectStorage::getInt64(sl_int64* _out) const noexcept
	{
		return value.getInt64(_out);
	}

	sl_int64 ObjectStorage::getInt64(sl_int64 def) const noexcept
	{
		return value.getInt64(def);
	}

	sl_bool ObjectStorage::isUint64() const noexcept
	{
		return value.isUint64();
	}

	sl_bool ObjectStorage::getUint64(sl_uint64* _out) const noexcept
	{
		return value.getUint64(_out);
	}

	sl_uint64 ObjectStorage::getUint64(sl_uint64 def) const noexcept
	{
		return value.getUint64(def);
	}

	sl_bool ObjectStorage::isInteger() const noexcept
	{
		return value.isInteger();
	}

	sl_bool ObjectStorage::isSignedInteger() const noexcept
	{
		return value.isSignedInteger();
	}

	sl_bool ObjectStorage::isUnsignedInteger() const noexcept
	{
		return value.isUnsignedInteger();
	}

	sl_bool ObjectStorage::isFloat() const noexcept
	{
		return value.isFloat();
	}

	sl_bool ObjectStorage::getFloat(float* _out) const noexcept
	{
		return value.getFloat(_out);
	}

	float ObjectStorage::getFloat(float def) const noexcept
	{
		return value.getFloat(def);
	}

	sl_bool ObjectStorage::isDouble() const noexcept
	{
		return value.isDouble();
	}

	sl_bool ObjectStorage::getDouble(double* _out) const noexcept
	{
		return value.getDouble(_out);
	}

	double ObjectStorage::getDouble(double def) const noexcept
	{
		return value.getDouble(def);
	}

	sl_bool ObjectStorage::isNumber() const noexcept
	{
		return value.isNumber();
	}

	sl_bool ObjectStorage::isBoolean() const noexcept
	{
		return value.isBoolean();
	}

	sl_bool ObjectStorage::isTrue() const noexcept
	{
		return value.isTrue();
	}

	sl_bool ObjectStorage::isFalse() const noexcept
	{
		return value.isFalse();
	}

	sl_bool ObjectStorage::getBoolean(sl_bool def) const noexcept
	{
		return value.getBoolean(def);
	}

	sl_bool ObjectStorage::isString() const noexcept
	{
		return value.isString();
	}

	String ObjectStorage::getString(const String& def) const noexcept
	{
		return value.getString(def);
	}

	String ObjectStorage::getString() const noexcept
	{
		return value.getString();
	}

	String16 ObjectStorage::getString16(const String16& def) const noexcept
	{
		return value.getString16(def);
	}

	String16 ObjectStorage::getString16() const noexcept
	{
		return value.getString16();
	}

	sl_bool ObjectStorage::isTime() const noexcept
	{
		return value.isTime();
	}

	Time ObjectStorage::getTime(const Time& def) const noexcept
	{
		return value.getTime(def);
	}

	Time ObjectStorage::getTime() const noexcept
	{
		return value.getTime();
	}

	sl_bool ObjectStorage::isCollection() const noexcept
	{
		return value.isCollection();
	}

	Ref<Collection> ObjectStorage::getCollection() const noexcept
	{
		return value.getCollection();
	}

	sl_bool ObjectStorage::isVariantList() const noexcept
	{
		return value.isVariantList();
	}

	VariantList ObjectStorage::getVariantList() const noexcept
	{
		return value.getVariantList();
	}

	sl_bool ObjectStorage::isJsonList() const noexcept
	{
		return value.isJsonList();
	}

	JsonList ObjectStorage::getJsonList() const noexcept
	{
		return value.getJsonList();
	}

	sl_bool ObjectStorage::isObject() const noexcept
	{
		return value.isObject();
	}

	Ref<Object> ObjectStorage::getObject() const noexcept
	{
		return value.getObject();
	}

	sl_bool ObjectStorage::isVariantMap() const noexcept
	{
		return value.isVariantMap();
	}

	VariantMap ObjectStorage::getVariantMap() const noexcept
	{
		return value.getVariantMap();
	}

	sl_bool ObjectStorage::isJsonMap() const noexcept
	{
		return value.isJsonMap();
	}

	JsonMap ObjectStorage::getJsonMap() const noexcept
	{
		return value.getJsonMap();
	}

	sl_bool ObjectStorage::isMemory() const noexcept
	{
		return value.isMemory();
	}

	Memory ObjectStorage::getMemory() const noexcept
	{
		return value.getMemory();
	}

	ObjectStorage ObjectStorage::operator[](const StringParam& name) noexcept
	{
		return getItem(name);
	}

	Variant ObjectStorage::operator[](sl_size index) noexcept
	{
		return value.getElement(index);
	}

	ObjectStorage ObjectStorage::open(const ObjectStorageParam& param)
	{
		Ref<ObjectStorageManagerImpl> manager = ObjectStorageManagerImpl::open(param);
		if (manager.isNotNull()) {
			return manager->createRootObject();
		}
		return sl_null;
	}

	ObjectStorage ObjectStorage::open(const StringParam& path)
	{
		ObjectStorageParam param;
		param.path = path;
		return open(param);
	}

}
