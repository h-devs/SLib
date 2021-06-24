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
#define DICTIONARY_BUFFER_SIZE 128
#define DICTIONARY_REMOVE_BATCH_SIZE 1024
#define KEY_NAME_LAST_DICTIONARY_ID "last_dictionary_id"

namespace slib
{

	namespace priv
	{
		namespace object_storage
		{

			static sl_uint32 PrepareKey(sl_uint8* _out, sl_uint64 parentId, sl_bool flagDictionary)
			{
				return CVLI::serialize(_out, (parentId << 1) | (flagDictionary ? 1 : 0));
			}

			static sl_uint32 PrepareKey(sl_uint8* _out, sl_uint64 parentId, sl_bool flagDictionary, const StringParam& _key)
			{
				StringData key(_key);
				sl_size len = key.getLength();
				if (!len) {
					return 0;
				}
				if (len > KEY_LENGTH_MAX) {
					return 0;
				}
				sl_uint32 nPrefix = PrepareKey(_out, parentId, flagDictionary);
				Base::copyMemory(_out + nPrefix, key.getData(), len);
				return nPrefix + (sl_uint32)len;
			}

			static sl_uint64 DeserializeDictionaryId(const void* data, sl_size size)
			{
				if (size) {
					sl_uint64 dictionaryId = 0;
					if (CVLI::deserialize(data, size, dictionaryId)) {
						return dictionaryId;
					}
				}
				return 0;
			}

			class DictionaryImpl;

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

				Ref<StorageDictionary> getRootDictionary() override;
				
			public:
				Ref<StorageDictionary> createDictionary(sl_uint64 parentId, const StringParam& key);

				Ref<StorageDictionary> getDictionary(sl_uint64 parentId, const StringParam& key);

				sl_bool removeDictionary(sl_uint64 parentId, const StringParam& _key)
				{
					sl_uint8 key[KEY_BUFFER_SIZE];
					sl_uint32 nKey = PrepareKey(key, parentId, sl_true, _key);
					if (!nKey) {
						return sl_false;
					}
					sl_uint64 childId = getDictionaryId(key, nKey);
					if (childId) {
						if (removeChildDictionaries(childId)) {
							if (removeChildItems(childId)) {
								if (m_store->remove(StringView((char*)key, nKey))) {
									return sl_true;
								}
							}
						}
					}
					return sl_false;
				}

				Iterator<String, ObjectStorage> getDictionaryIterator(sl_uint64 parentId);

				Variant getItem(sl_uint64 parentId, const StringParam& _key)
				{
					sl_uint8 key[KEY_BUFFER_SIZE];
					sl_uint32 nKey = PrepareKey(key, parentId, sl_false, _key);
					if (nKey) {
						return m_store->get(StringView((char*)key, nKey));
					} else {
						return Variant();
					}
				}

				sl_bool putItem(sl_uint64 parentId, const StringParam& _key, const Variant& value)
				{
					sl_uint8 key[KEY_BUFFER_SIZE];
					sl_uint32 nKey = PrepareKey(key, parentId, sl_false, _key);
					if (nKey) {
						return m_store->put(StringView((char*)key, nKey), value);
					} else {
						return sl_false;
					}
				}

				sl_bool removeItem(sl_uint64 parentId, const StringParam& _key)
				{
					sl_uint8 key[KEY_BUFFER_SIZE];
					sl_uint32 nKey = PrepareKey(key, parentId, sl_false, _key);
					if (nKey) {
						return m_store->remove(StringView((char*)key, nKey));
					} else {
						return sl_false;
					}
				}

				PropertyIterator getItemIterator(sl_uint64 parentId);

				sl_uint64 getDictionaryId(sl_uint8* key, sl_uint32 nKey)
				{
					char buf[DICTIONARY_BUFFER_SIZE];
					sl_reg n = m_store->get(key, nKey, buf, sizeof(buf));
					if (n > 0) {
						return DeserializeDictionaryId(buf, n);
					}
					return 0;
				}

				sl_bool removeChildDictionaries(sl_uint64 parentId)
				{
					sl_uint8 key[KEY_BUFFER_SIZE];
					sl_uint32 nKey = PrepareKey(key, parentId, sl_true);
					for (;;) {
						List<Memory> listKeys;
						List<sl_uint64> listIds;
						{
							Ref<KeyValueIterator> iterator = m_store->getIterator();
							if (iterator.isNull()) {
								return sl_false;
							}
							if (!(iterator->seek(key, nKey))) {
								break;
							}
							do {
								sl_uint8 subKey[KEY_BUFFER_SIZE];
								sl_reg n = iterator->getKey(subKey, sizeof(subKey));
								if (n <= (sl_reg)nKey) {
									break;
								}
								if (!(Base::equalsMemory(key, subKey, nKey))) {
									break;
								}
								Memory memKey = Memory::create(subKey, n);
								if (memKey.isNull()) {
									return sl_false;
								}
								if (!(listKeys.add_NoLock(Move(memKey)))) {
									return sl_false;
								}
								char value[DICTIONARY_BUFFER_SIZE];
								sl_reg nValue = iterator->getValue(value, sizeof(value));
								if (nValue >= 0) {
									sl_uint64 childId = DeserializeDictionaryId(value, nValue);
									if (childId) {
										if (!(listIds.add_NoLock(childId))) {
											return sl_false;
										}
									}
								}
								if (listKeys.getCount() >= DICTIONARY_REMOVE_BATCH_SIZE) {
									break;
								}
							} while (iterator->moveNext());
						}
						{
							ListElements<sl_uint64> ids(listIds);
							for (sl_size i = 0; i < ids.count; i++) {
								if (!(removeChildDictionaries(ids[i]))) {
									return sl_false;
								}
								if (!(removeChildItems(ids[i]))) {
									return sl_false;
								}
							}
						}
						{
							ListElements<Memory> keys(listKeys);
							if (!(keys.count)) {
								break;
							}
							Ref<KeyValueWriteBatch> batch = m_store->createWriteBatch();
							if (batch.isNull()) {
								return sl_false;
							}
							for (sl_size i = 0; i < keys.count; i++) {
								if (!(batch->remove(keys[i].getData(), keys[i].getSize()))) {
									return sl_false;
								}
							}
							if (!(batch->commit())) {
								return sl_false;
							}
						}
					}
					return sl_true;
				}

				sl_bool removeChildItems(sl_uint64 parentId)
				{
					sl_uint8 key[KEY_BUFFER_SIZE];
					sl_uint32 nKey = PrepareKey(key, parentId, sl_false);
					for (;;) {
						List<Memory> listItems;
						{
							Ref<KeyValueIterator> iterator = m_store->getIterator();
							if (iterator.isNull()) {
								return sl_false;
							}
							if (!(iterator->seek(key, nKey))) {
								break;
							}
							do {
								sl_uint8 subKey[KEY_BUFFER_SIZE];
								sl_reg n = iterator->getKey(subKey, sizeof(subKey));
								if (n <= (sl_reg)nKey) {
									break;
								}
								if (!(Base::equalsMemory(key, subKey, nKey))) {
									break;
								}
								Memory memKey = Memory::create(subKey, n);
								if (memKey.isNull()) {
									return sl_false;
								}
								if (!(listItems.add_NoLock(Move(memKey)))) {
									return sl_false;
								}
								if (listItems.getCount() >= DICTIONARY_REMOVE_BATCH_SIZE) {
									break;
								}
							} while (iterator->moveNext());
						}
						{
							ListElements<Memory> items(listItems);
							if (!(items.count)) {
								break;
							}
							Ref<KeyValueWriteBatch> batch = m_store->createWriteBatch();
							if (batch.isNull()) {
								return sl_false;
							}
							for (sl_size i = 0; i < items.count; i++) {
								if (!(batch->remove(items[i].getData(), items[i].getSize()))) {
									return sl_false;
								}
							}
							if (!(batch->commit())) {
								return sl_false;
							}
						}
					}
					return sl_true;
				}

			};

			class DictionaryImpl : public StorageDictionary
			{
			public:
				Ref<ObjectStorageManagerImpl> m_manager;
				sl_uint64 m_id;

			public:
				DictionaryImpl(ObjectStorageManagerImpl* manager, sl_uint64 _id): m_manager(manager), m_id(_id) {}

			public:
				Ref<ObjectStorageManager> getManager() override
				{
					return m_manager;
				}

				Ref<StorageDictionary> createDictionary(const StringParam& key) override
				{
					return m_manager->createDictionary(m_id, key);
				}

				Ref<StorageDictionary> getDictionary(const StringParam& key) override
				{
					return m_manager->getDictionary(m_id, key);
				}

				sl_bool removeDictionary(const StringParam& key) override
				{
					return m_manager->removeDictionary(m_id, key);
				}

				Iterator<String, ObjectStorage> getDictionaryIterator() override
				{
					return m_manager->getDictionaryIterator(m_id);
				}

				Variant getItem(const StringParam& key) override
				{
					return m_manager->getItem(m_id, key);
				}

				sl_bool putItem(const StringParam& key, const Variant& value) override
				{
					return m_manager->putItem(m_id, key, value);
				}

				sl_bool removeItem(const StringParam& key) override
				{
					return m_manager->removeItem(m_id, key);
				}

				PropertyIterator getItemIterator() override
				{
					return m_manager->getItemIterator(m_id);
				}

			};

			Ref<StorageDictionary> ObjectStorageManagerImpl::getRootDictionary()
			{
				return new DictionaryImpl(this, 0);
			}

			Ref<StorageDictionary> ObjectStorageManagerImpl::createDictionary(sl_uint64 parentId, const StringParam& _key)
			{
				sl_uint8 key[KEY_BUFFER_SIZE];
				sl_uint32 nKey = PrepareKey(key, parentId, sl_true, _key);
				if (nKey) {
					ObjectLocker lock(this);
					sl_uint64 childId = getDictionaryId(key, nKey);
					if (childId) {
						return new DictionaryImpl(this, childId);
					}
					Ref<KeyValueWriteBatch> batch = m_store->createWriteBatch();
					if (batch.isNotNull()) {
						sl_uint64 newId = m_store->get(StringParam::literal(KEY_NAME_LAST_DICTIONARY_ID)).getUint64();
						newId++;
						if (batch->put(StringParam::literal(KEY_NAME_LAST_DICTIONARY_ID), newId)) {
							sl_uint8 buf[DICTIONARY_BUFFER_SIZE];
							sl_uint32 size = CVLI::serialize(buf, newId);
							if (batch->put(key, nKey, buf, size)) {
								if (batch->commit()) {
									return new DictionaryImpl(this, newId);
								}
							}
						}
					}
				}
				return sl_null;
			}

			Ref<StorageDictionary> ObjectStorageManagerImpl::getDictionary(sl_uint64 parentId, const StringParam& _key)
			{
				sl_uint8 key[KEY_BUFFER_SIZE];
				sl_uint32 nKey = PrepareKey(key, parentId, sl_true, _key);
				if (nKey) {
					sl_uint64 childId = getDictionaryId(key, nKey);
					if (childId) {
						return new DictionaryImpl(this, childId);
					}
				}
				return sl_null;
			}

			template <class T>
			class BaseIterator : public CIterator<String, T>
			{
			public:
				Ref<ObjectStorageManagerImpl> m_manager;
				Ref<KeyValueIterator> m_kvIterator;

				sl_bool m_flagFirstMove;
				sl_uint8 m_prefix[KEY_BUFFER_SIZE];
				sl_uint32 m_nPrefix;
				String m_key;

			public:
				BaseIterator(ObjectStorageManagerImpl* manager, sl_uint64 parentId, sl_bool flagDictionary, Ref<KeyValueIterator>&& iterator): m_manager(manager), m_kvIterator(Move(iterator)), m_flagFirstMove(sl_true)
				{
					m_nPrefix = PrepareKey(m_prefix, parentId, flagDictionary);
				}

			public:
				String getKey() override
				{
					return m_key;
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

			class DictionaryIterator : public BaseIterator<ObjectStorage>
			{
			public:
				DictionaryIterator(ObjectStorageManagerImpl* manager, sl_uint64 parentId, Ref<KeyValueIterator>&& iterator): BaseIterator(manager, parentId, sl_true, Move(iterator)) {}

			public:
				ObjectStorage getValue() override
				{
					char buf[VALUE_BUFFER_SIZE];
					sl_reg n = m_kvIterator->getValue(buf, sizeof(buf));
					if (n > 0) {
						sl_uint64 childId = DeserializeDictionaryId(buf, n);
						if (childId) {
							return new DictionaryImpl(m_manager.get(), childId);
						}
					}
					return ObjectStorage();
				}

			};

			Iterator<String, ObjectStorage> ObjectStorageManagerImpl::getDictionaryIterator(sl_uint64 parentId)
			{
				Ref<KeyValueIterator> iterator = m_store->getIterator();
				if (iterator.isNotNull()) {
					return new DictionaryIterator(this, parentId, Move(iterator));
				}
				return sl_null;
			}

			class ItemIterator : public BaseIterator<Variant>
			{
			public:
				ItemIterator(ObjectStorageManagerImpl* manager, sl_uint64 parentId, Ref<KeyValueIterator>&& iterator): BaseIterator(manager, parentId, sl_false, Move(iterator)) {}

			public:
				Variant getValue() override
				{
					char buf[VALUE_BUFFER_SIZE];
					MemoryData mem(buf, sizeof(buf));
					if (m_kvIterator->getValue(&mem)) {
						return KeyValueReader::deserialize(Move(mem));
					} else {
						return Variant();
					}
				}

			};

			PropertyIterator ObjectStorageManagerImpl::getItemIterator(sl_uint64 parentId)
			{
				Ref<KeyValueIterator> iterator = m_store->getIterator();
				if (iterator.isNotNull()) {
					return new ItemIterator(this, parentId, Move(iterator));
				}
				return sl_null;
			}

		}
	}

	using namespace priv::object_storage;


	SLIB_DEFINE_ROOT_OBJECT(StorageDictionary)

	StorageDictionary::StorageDictionary()
	{
	}

	StorageDictionary::~StorageDictionary()
	{
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

	ObjectStorage::ObjectStorage(StorageDictionary* object) noexcept: value(Variant::fromObject(object))
	{
	}

	ObjectStorage::ObjectStorage(const Ref<StorageDictionary>& object) noexcept: value(Variant::fromObject(object))
	{
	}

	ObjectStorage::ObjectStorage(Ref<StorageDictionary>&& object) noexcept: value(Variant::fromObject(Move(object)))
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
		Ref<StorageDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->getManager();
		}
		return sl_null;
	}

	sl_bool ObjectStorage::isDictionary() const noexcept
	{
		return IsInstanceOf<StorageDictionary>(value.getRef());
	}

	Ref<StorageDictionary> ObjectStorage::getDictionary() const noexcept
	{
		return CastRef<StorageDictionary>(value.getRef());
	}

	ObjectStorage ObjectStorage::createDictionary(const StringParam& key) const
	{
		Ref<StorageDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->createDictionary(key);
		} else {
			return ObjectStorage();
		}
	}

	ObjectStorage ObjectStorage::getDictionary(const StringParam& key) const
	{
		Ref<StorageDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->getDictionary(key);
		} else {
			return ObjectStorage();
		}
	}

	sl_bool ObjectStorage::removeDictionary(const StringParam& key) const
	{
		Ref<StorageDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->removeDictionary(key);
		}
		return sl_false;
	}

	Iterator<String, ObjectStorage> ObjectStorage::getDictionaryIterator() const
	{
		Ref<StorageDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->getDictionaryIterator();
		}
		return sl_null;
	}

	Variant ObjectStorage::getItem(const StringParam& key) const
	{
		Ref<StorageDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->getItem(key);
		} else {
			return value.getItem(key);
		}
	}

	sl_bool ObjectStorage::putItem(const StringParam& key, const Variant& value) const
	{
		Ref<StorageDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->putItem(key, value);
		}
		return sl_false;
	}

	sl_bool ObjectStorage::removeItem(const StringParam& key) const
	{
		Ref<StorageDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->removeItem(key);
		}
		return sl_false;
	}

	PropertyIterator ObjectStorage::getItemIterator() const
	{
		Ref<StorageDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->getItemIterator();
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
		Ref<StorageDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			Ref<StorageDictionary> ret = dictionary->getDictionary(name);
			if (ret.isNotNull()) {
				return ret;
			}
			return dictionary->getItem(name);
		} else {
			return value.getItem(name);
		}
	}

	Variant ObjectStorage::operator[](sl_size index) noexcept
	{
		return value.getElement(index);
	}

	ObjectStorage ObjectStorage::open(const ObjectStorageParam& param)
	{
		Ref<ObjectStorageManagerImpl> manager = ObjectStorageManagerImpl::open(param);
		if (manager.isNotNull()) {
			return manager->getRootDictionary();
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
