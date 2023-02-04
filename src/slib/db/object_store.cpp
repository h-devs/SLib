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

#include "slib/db/object_store.h"

#include "slib/db/leveldb.h"
#include "slib/data/json.h"
#include "slib/data/serialize.h"

#define KEY_LENGTH_MAX 1024
#define KEY_BUFFER_SIZE (KEY_LENGTH_MAX + 16)
#define VALUE_BUFFER_SIZE 1024
#define DICTIONARY_BUFFER_SIZE 128
#define DICTIONARY_REMOVE_BATCH_SIZE 1024
#define KEY_NAME_LAST_DICTIONARY_ID "last_dictionary_id"

namespace slib
{

	namespace {

		static sl_uint32 PrepareKey(sl_uint8* _out, sl_uint64 parentId, sl_bool flagDictionary)
		{
			return CVLI::serialize(_out, (parentId << 1) | (flagDictionary ? 1 : 0));
		}

		static sl_uint32 PrepareKey(sl_uint8* _out, sl_uint64 parentId, sl_bool flagDictionary, const StringView& key)
		{
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

		class ObjectStoreManagerImpl : public ObjectStoreManager
		{
		public:
			Ref<KeyValueStore> m_store;

		public:
			static Ref<ObjectStoreManagerImpl> open(const ObjectStoreParam& param)
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
				Ref<ObjectStoreManagerImpl> ret = new ObjectStoreManagerImpl;
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

			Ref<ObjectStoreDictionary> getRootDictionary() override;

		public:
			Ref<ObjectStoreDictionary> createDictionary(sl_uint64 parentId, const StringView& key);

			Ref<ObjectStoreDictionary> getDictionary(sl_uint64 parentId, const StringView& key);

			sl_bool removeDictionary(sl_uint64 parentId, const StringView& _key)
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

			Iterator<String, ObjectStore> getDictionaryIterator(sl_uint64 parentId);

			Variant getItem(sl_uint64 parentId, const StringView& _key)
			{
				sl_uint8 key[KEY_BUFFER_SIZE];
				sl_uint32 nKey = PrepareKey(key, parentId, sl_false, _key);
				if (nKey) {
					return m_store->get(StringView((char*)key, nKey));
				} else {
					return Variant();
				}
			}

			sl_bool putItem(sl_uint64 parentId, const StringView& _key, const Variant& value)
			{
				sl_uint8 key[KEY_BUFFER_SIZE];
				sl_uint32 nKey = PrepareKey(key, parentId, sl_false, _key);
				if (nKey) {
					return m_store->put(StringView((char*)key, nKey), value);
				} else {
					return sl_false;
				}
			}

			sl_bool removeItem(sl_uint64 parentId, const StringView& _key)
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

		class DictionaryImpl : public ObjectStoreDictionary
		{
		public:
			Ref<ObjectStoreManagerImpl> m_manager;
			sl_uint64 m_id;

		public:
			DictionaryImpl(ObjectStoreManagerImpl* manager, sl_uint64 _id): m_manager(manager), m_id(_id) {}

		public:
			Ref<ObjectStoreManager> getManager() override
			{
				return m_manager;
			}

			Ref<ObjectStoreDictionary> createDictionary(const StringView& key) override
			{
				return m_manager->createDictionary(m_id, key);
			}

			Ref<ObjectStoreDictionary> getDictionary(const StringView& key) override
			{
				return m_manager->getDictionary(m_id, key);
			}

			sl_bool removeDictionary(const StringView& key) override
			{
				return m_manager->removeDictionary(m_id, key);
			}

			Iterator<String, ObjectStore> getDictionaryIterator() override
			{
				return m_manager->getDictionaryIterator(m_id);
			}

			Variant getItem(const StringView& key) override
			{
				return m_manager->getItem(m_id, key);
			}

			sl_bool putItem(const StringView& key, const Variant& value) override
			{
				return m_manager->putItem(m_id, key, value);
			}

			sl_bool removeItem(const StringView& key) override
			{
				return m_manager->removeItem(m_id, key);
			}

			PropertyIterator getItemIterator() override
			{
				return m_manager->getItemIterator(m_id);
			}

		};

		Ref<ObjectStoreDictionary> ObjectStoreManagerImpl::getRootDictionary()
		{
			return new DictionaryImpl(this, 0);
		}

		Ref<ObjectStoreDictionary> ObjectStoreManagerImpl::createDictionary(sl_uint64 parentId, const StringView& _key)
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

		Ref<ObjectStoreDictionary> ObjectStoreManagerImpl::getDictionary(sl_uint64 parentId, const StringView& _key)
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
			Ref<ObjectStoreManagerImpl> m_manager;
			Ref<KeyValueIterator> m_kvIterator;

			sl_bool m_flagFirstMove;
			sl_uint8 m_prefix[KEY_BUFFER_SIZE];
			sl_uint32 m_nPrefix;
			String m_key;

		public:
			BaseIterator(ObjectStoreManagerImpl* manager, sl_uint64 parentId, sl_bool flagDictionary, Ref<KeyValueIterator>&& iterator): m_manager(manager), m_kvIterator(Move(iterator)), m_flagFirstMove(sl_true)
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

		class DictionaryIterator : public BaseIterator<ObjectStore>
		{
		public:
			DictionaryIterator(ObjectStoreManagerImpl* manager, sl_uint64 parentId, Ref<KeyValueIterator>&& iterator): BaseIterator(manager, parentId, sl_true, Move(iterator)) {}

		public:
			ObjectStore getValue() override
			{
				char buf[VALUE_BUFFER_SIZE];
				sl_reg n = m_kvIterator->getValue(buf, sizeof(buf));
				if (n > 0) {
					sl_uint64 childId = DeserializeDictionaryId(buf, n);
					if (childId) {
						return new DictionaryImpl(m_manager.get(), childId);
					}
				}
				return ObjectStore();
			}

		};

		Iterator<String, ObjectStore> ObjectStoreManagerImpl::getDictionaryIterator(sl_uint64 parentId)
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
			ItemIterator(ObjectStoreManagerImpl* manager, sl_uint64 parentId, Ref<KeyValueIterator>&& iterator): BaseIterator(manager, parentId, sl_false, Move(iterator)) {}

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

		PropertyIterator ObjectStoreManagerImpl::getItemIterator(sl_uint64 parentId)
		{
			Ref<KeyValueIterator> iterator = m_store->getIterator();
			if (iterator.isNotNull()) {
				return new ItemIterator(this, parentId, Move(iterator));
			}
			return sl_null;
		}

	}


	SLIB_DEFINE_ROOT_OBJECT(ObjectStoreDictionary)

	ObjectStoreDictionary::ObjectStoreDictionary()
	{
	}

	ObjectStoreDictionary::~ObjectStoreDictionary()
	{
	}


	SLIB_DEFINE_OBJECT(ObjectStoreManager, Object)

	ObjectStoreManager::ObjectStoreManager()
	{
	}

	ObjectStoreManager::~ObjectStoreManager()
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ObjectStoreParam)

	ObjectStoreParam::ObjectStoreParam()
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ObjectStore)

	ObjectStore::ObjectStore() noexcept
	{
	}

	ObjectStore::ObjectStore(sl_null_t) noexcept: value(sl_null)
	{
	}

	ObjectStore::ObjectStore(ObjectStoreDictionary* object) noexcept: value(Variant::fromObject(object))
	{
	}

	ObjectStore::ObjectStore(const Ref<ObjectStoreDictionary>& object) noexcept: value(Variant::fromObject(object))
	{
	}

	ObjectStore::ObjectStore(Ref<ObjectStoreDictionary>&& object) noexcept: value(Variant::fromObject(Move(object)))
	{
	}

	Variant::Variant(const ObjectStore& t) noexcept: Variant(t.value)
	{
	}

	Variant::Variant(ObjectStore&& t) noexcept: Variant(Move(t.value))
	{
	}

	Json::Json(const ObjectStore& t) noexcept: Json(t.value)
	{
	}

	Json::Json(ObjectStore&& t) noexcept: Json(Move(t.value))
	{
	}

	Ref<ObjectStoreManager> ObjectStore::getManager() const
	{
		Ref<ObjectStoreDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->getManager();
		}
		return sl_null;
	}

	sl_bool ObjectStore::isDictionary() const noexcept
	{
		return IsInstanceOf<ObjectStoreDictionary>(value.getRef());
	}

	Ref<ObjectStoreDictionary> ObjectStore::getDictionary() const noexcept
	{
		return CastRef<ObjectStoreDictionary>(value.getRef());
	}

	ObjectStore ObjectStore::createDictionary(const StringView& key) const
	{
		Ref<ObjectStoreDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->createDictionary(key);
		} else {
			return ObjectStore();
		}
	}

	ObjectStore ObjectStore::getDictionary(const StringView& key) const
	{
		Ref<ObjectStoreDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->getDictionary(key);
		} else {
			return ObjectStore();
		}
	}

	sl_bool ObjectStore::removeDictionary(const StringView& key) const
	{
		Ref<ObjectStoreDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->removeDictionary(key);
		}
		return sl_false;
	}

	Iterator<String, ObjectStore> ObjectStore::getDictionaryIterator() const
	{
		Ref<ObjectStoreDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->getDictionaryIterator();
		}
		return sl_null;
	}

	Variant ObjectStore::getItem(const StringView& key) const
	{
		Ref<ObjectStoreDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->getItem(key);
		} else {
			return value.getItem(key);
		}
	}

	sl_bool ObjectStore::putItem(const StringView& key, const Variant& value) const
	{
		Ref<ObjectStoreDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->putItem(key, value);
		}
		return sl_false;
	}

	sl_bool ObjectStore::removeItem(const StringView& key) const
	{
		Ref<ObjectStoreDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->removeItem(key);
		}
		return sl_false;
	}

	PropertyIterator ObjectStore::getItemIterator() const
	{
		Ref<ObjectStoreDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			return dictionary->getItemIterator();
		}
		return sl_null;
	}

	sl_bool ObjectStore::isUndefined() const noexcept
	{
		return value.isUndefined();
	}

	sl_bool ObjectStore::isNotUndefined() const noexcept
	{
		return value.isNotUndefined();
	}

	sl_bool ObjectStore::isNull() const noexcept
	{
		return value.isNull();
	}

	sl_bool ObjectStore::isNotNull() const noexcept
	{
		return value.isNotNull();
	}

	sl_bool ObjectStore::isInt32() const noexcept
	{
		return value.isInt32();
	}

	sl_bool ObjectStore::getInt32(sl_int32* _out) const noexcept
	{
		return value.getInt32(_out);
	}

	sl_int32 ObjectStore::getInt32(sl_int32 def) const noexcept
	{
		return value.getInt32(def);
	}

	sl_bool ObjectStore::isUint32() const noexcept
	{
		return value.getUint32();
	}

	sl_bool ObjectStore::getUint32(sl_uint32* _out) const noexcept
	{
		return value.getUint32(_out);
	}

	sl_uint32 ObjectStore::getUint32(sl_uint32 def) const noexcept
	{
		return value.getUint32(def);
	}

	sl_bool ObjectStore::isInt64() const noexcept
	{
		return value.isInt64();
	}

	sl_bool ObjectStore::getInt64(sl_int64* _out) const noexcept
	{
		return value.getInt64(_out);
	}

	sl_int64 ObjectStore::getInt64(sl_int64 def) const noexcept
	{
		return value.getInt64(def);
	}

	sl_bool ObjectStore::isUint64() const noexcept
	{
		return value.isUint64();
	}

	sl_bool ObjectStore::getUint64(sl_uint64* _out) const noexcept
	{
		return value.getUint64(_out);
	}

	sl_uint64 ObjectStore::getUint64(sl_uint64 def) const noexcept
	{
		return value.getUint64(def);
	}

	sl_bool ObjectStore::isIntegerType() const noexcept
	{
		return value.isIntegerType();
	}

	sl_bool ObjectStore::isSignedIntegerType() const noexcept
	{
		return value.isSignedIntegerType();
	}

	sl_bool ObjectStore::isUnsignedIntegerType() const noexcept
	{
		return value.isUnsignedIntegerType();
	}

	sl_bool ObjectStore::isFloat() const noexcept
	{
		return value.isFloat();
	}

	sl_bool ObjectStore::getFloat(float* _out) const noexcept
	{
		return value.getFloat(_out);
	}

	float ObjectStore::getFloat(float def) const noexcept
	{
		return value.getFloat(def);
	}

	sl_bool ObjectStore::isDouble() const noexcept
	{
		return value.isDouble();
	}

	sl_bool ObjectStore::getDouble(double* _out) const noexcept
	{
		return value.getDouble(_out);
	}

	double ObjectStore::getDouble(double def) const noexcept
	{
		return value.getDouble(def);
	}

	sl_bool ObjectStore::isNumberType() const noexcept
	{
		return value.isNumberType();
	}

	sl_bool ObjectStore::isBoolean() const noexcept
	{
		return value.isBoolean();
	}

	sl_bool ObjectStore::isTrue() const noexcept
	{
		return value.isTrue();
	}

	sl_bool ObjectStore::isFalse() const noexcept
	{
		return value.isFalse();
	}

	sl_bool ObjectStore::getBoolean(sl_bool def) const noexcept
	{
		return value.getBoolean(def);
	}

	sl_bool ObjectStore::isStringType() const noexcept
	{
		return value.isStringType();
	}

	String ObjectStore::getString(const String& def) const noexcept
	{
		return value.getString(def);
	}

	String ObjectStore::getString() const noexcept
	{
		return value.getString();
	}

	String16 ObjectStore::getString16(const String16& def) const noexcept
	{
		return value.getString16(def);
	}

	String16 ObjectStore::getString16() const noexcept
	{
		return value.getString16();
	}

	sl_bool ObjectStore::isTime() const noexcept
	{
		return value.isTime();
	}

	Time ObjectStore::getTime(const Time& def) const noexcept
	{
		return value.getTime(def);
	}

	Time ObjectStore::getTime() const noexcept
	{
		return value.getTime();
	}

	sl_bool ObjectStore::isCollection() const noexcept
	{
		return value.isCollection();
	}

	Ref<Collection> ObjectStore::getCollection() const noexcept
	{
		return value.getCollection();
	}

	sl_bool ObjectStore::isVariantList() const noexcept
	{
		return value.isVariantList();
	}

	VariantList ObjectStore::getVariantList() const noexcept
	{
		return value.getVariantList();
	}

	sl_bool ObjectStore::isJsonList() const noexcept
	{
		return value.isJsonList();
	}

	JsonList ObjectStore::getJsonList() const noexcept
	{
		return value.getJsonList();
	}

	sl_bool ObjectStore::isObject() const noexcept
	{
		return value.isObject();
	}

	Ref<Object> ObjectStore::getObject() const noexcept
	{
		return value.getObject();
	}

	sl_bool ObjectStore::isVariantMap() const noexcept
	{
		return value.isVariantMap();
	}

	VariantMap ObjectStore::getVariantMap() const noexcept
	{
		return value.getVariantMap();
	}

	sl_bool ObjectStore::isJsonMap() const noexcept
	{
		return value.isJsonMap();
	}

	JsonMap ObjectStore::getJsonMap() const noexcept
	{
		return value.getJsonMap();
	}

	sl_bool ObjectStore::isMemory() const noexcept
	{
		return value.isMemory();
	}

	Memory ObjectStore::getMemory() const noexcept
	{
		return value.getMemory();
	}

	ObjectStore ObjectStore::operator[](const String& name) noexcept
	{
		Ref<ObjectStoreDictionary> dictionary = getDictionary();
		if (dictionary.isNotNull()) {
			Ref<ObjectStoreDictionary> ret = dictionary->getDictionary(name);
			if (ret.isNotNull()) {
				return ret;
			}
			return dictionary->getItem(name);
		} else {
			return value.getItem(name);
		}
	}

	Variant ObjectStore::operator[](sl_size index) noexcept
	{
		return value.getElement(index);
	}

	ObjectStore ObjectStore::open(const ObjectStoreParam& param)
	{
		Ref<ObjectStoreManagerImpl> manager = ObjectStoreManagerImpl::open(param);
		if (manager.isNotNull()) {
			return manager->getRootDictionary();
		}
		return sl_null;
	}

	ObjectStore ObjectStore::open(const StringParam& path)
	{
		ObjectStoreParam param;
		param.path = path;
		return open(param);
	}

}
