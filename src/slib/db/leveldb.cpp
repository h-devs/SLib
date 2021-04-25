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

#include "slib/db/leveldb.h"

#include "slib/core/memory.h"
#include "slib/core/safe_static.h"

#include "leveldb/db.h"
#include "leveldb/write_batch.h"
#include "leveldb/env.h"

namespace slib
{

	namespace priv
	{
		namespace level_db
		{

			class StdStringContainer : public Referable
			{
			public:
				std::string string;

			public:
				StdStringContainer(std::string&& other): string(Move(other)) {}

			};

			static sl_bool DecodeValue(std::string&& str, MemoryData* value)
			{
				sl_size size = (sl_size)(str.size());
				if (!size) {
					value->size = 0;
					return sl_true;
				}
				if (size <= value->size) {
					Base::copyMemory(value->data, str.data(), size);
					value->size = size;
					return sl_true;
				} else {
					StdStringContainer* container = new StdStringContainer(Move(str));
					if (container) {
						value->data = (void*)(container->string.data());
						value->ref = container;
						value->size = size;
						return sl_true;
					}
				}
				return sl_false;
			}

			class DefaultEnvironmentManager : public Referable
			{
			public:
				DefaultEnvironmentManager()
				{
				}

				~DefaultEnvironmentManager()
				{
#ifdef SLIB_PLATFORM_IS_WIN32
					leveldb::Env* env = leveldb::Env::Default();
					if (env) {
						env->~Env();
					}
#endif
				}

			};

			SLIB_SAFE_STATIC_GETTER(Ref<DefaultEnvironmentManager>, GetDefaultEnironmentManager, new DefaultEnvironmentManager)

			class LevelDBImpl : public LevelDB
			{
			public:
				leveldb::DB* m_db;

				leveldb::ReadOptions m_optionsRead;
				leveldb::WriteOptions m_optionsWrite;

				Ref<DefaultEnvironmentManager> m_defaultEnvironemtManager;

			public:
				LevelDBImpl()
				{
				}

				~LevelDBImpl()
				{
					delete m_db;
				}

			public:
				static Ref<LevelDBImpl> open(const LevelDB_Param& param)
				{
					Ref<DefaultEnvironmentManager>* p = GetDefaultEnironmentManager();
					if (!p) {
						return sl_null;
					}

					StringCstr path(param.path);
					if (path.isEmpty()) {
						return sl_null;
					}

					leveldb::Options options;
					options.create_if_missing = (bool)(param.flagCreateIfMissing);

					leveldb::DB* db = sl_null;
					leveldb::Status status = leveldb::DB::Open(options, path.getData(), &db);
					if (db) {
						if (status.ok()) {
							Ref<LevelDBImpl> ret = new LevelDBImpl;
							if (ret.isNotNull()) {
								ret->m_db = db;
								ret->m_defaultEnvironemtManager = *p;
								return ret;
							}
						}
						delete db;
					}
					return sl_null;
				}

			public:
				Ref<KeyValueWriteBatch> createWriteBatch() override;

				Ref<KeyValueIterator> createIterator() override;

				Ref<KeyValueSnapshot> createSnapshot() override;

				sl_bool get(const void* key, sl_size sizeKey, MemoryData* value) override
				{
					std::string str;
					leveldb::Status status = m_db->Get(m_optionsRead, leveldb::Slice((char*)key, sizeKey), &str);
					if (status.ok()) {
						return DecodeValue(Move(str), value);
					}
					return sl_false;
				}

				sl_bool put(const void* key, sl_size sizeKey, const void* value, sl_size sizeValue) override
				{
					leveldb::Status status = m_db->Put(m_optionsWrite, leveldb::Slice((char*)key, sizeKey), leveldb::Slice((char*)value, sizeValue));
					return status.ok();
				}

				sl_bool remove(const void* key, sl_size sizeKey) override
				{
					leveldb::Status status = m_db->Delete(m_optionsWrite, leveldb::Slice((char*)key, sizeKey));
					return status.ok();
				}

				sl_bool compact(const void* from, sl_size sizeFrom, const void* end, sl_size sizeEnd) override
				{
					if (from) {
						if (end) {
							leveldb::Slice f((char*)from, sizeFrom);
							leveldb::Slice e((char*)end, sizeEnd);
							m_db->CompactRange(&f, &e);
						} else {
							leveldb::Slice f((char*)from, sizeFrom);
							m_db->CompactRange(&f, sl_null);
						}
					} else {
						if (end) {
							leveldb::Slice e((char*)end, sizeEnd);
							m_db->CompactRange(sl_null, &e);
						} else {
							m_db->CompactRange(sl_null, sl_null);
						}
					}
					return sl_true;
				}

			};

			class LevelDBWriteBatch : public KeyValueWriteBatch
			{
			public:
				Ref<LevelDBImpl> m_instance;
				leveldb::DB* m_db;
				leveldb::WriteBatch m_updates;

			public:
				LevelDBWriteBatch(LevelDBImpl* instance): m_instance(instance)
				{
					m_db = instance->m_db;
				}

				~LevelDBWriteBatch()
				{
					discard();
				}

			public:
				sl_bool put(const void* key, sl_size sizeKey, const void* value, sl_size sizeValue) override
				{
					m_updates.Put(leveldb::Slice((char*)key, sizeKey), leveldb::Slice((char*)value, sizeValue));
					return sl_true;
				}

				sl_bool remove(const void* key, sl_size sizeKey) override
				{
					m_updates.Delete(leveldb::Slice((char*)key, sizeKey));
					return sl_true;
				}

				sl_bool _commit() override
				{
					leveldb::Status status = m_db->Write(m_instance->m_optionsWrite, &m_updates);
					if (status.ok()) {
						m_updates.Clear();
						return sl_true;
					} else {
						return sl_false;
					}
				}

				void _discard() override
				{
					m_updates.Clear();
				}

			};

			Ref<KeyValueWriteBatch> LevelDBImpl::createWriteBatch()
			{
				return new LevelDBWriteBatch(this);
			}

			class LevelDBIterator : public KeyValueIterator
			{
			public:
				Ref<LevelDBImpl> m_instance;
				leveldb::Iterator* m_iterator;

			public:
				LevelDBIterator(LevelDBImpl* instance, leveldb::Iterator* iterator): m_instance(instance), m_iterator(iterator) {}

				~LevelDBIterator()
				{
					delete m_iterator;
				}

			public:
				sl_reg getKey(void* value, sl_size sizeValue) override
				{
					if (m_iterator->Valid()) {
						leveldb::Slice slice = m_iterator->key();
						sl_size sizeRequired = (sl_size)(slice.size());
						sl_size n = SLIB_MIN(sizeRequired, sizeValue);
						if (n) {
							Base::copyMemory(value, slice.data(), n);
						}
						return sizeRequired;
					}
					return -1;
				}

				sl_reg getValue(void* value, sl_size sizeValue) override
				{
					if (m_iterator->Valid()) {
						leveldb::Slice slice = m_iterator->value();
						sl_size sizeRequired = (sl_size)(slice.size());
						sl_size n = SLIB_MIN(sizeRequired, sizeValue);
						if (n) {
							Base::copyMemory(value, slice.data(), n);
						}
						return sizeRequired;
					}
					return -1;
				}

				sl_bool moveFirst() override
				{
					m_iterator->SeekToFirst();
					return m_iterator->Valid();
				}

				sl_bool moveLast() override
				{
					m_iterator->SeekToLast();
					return m_iterator->Valid();
				}

				sl_bool movePrevious() override
				{
					if (m_iterator->Valid()) {
						m_iterator->Prev();
					} else {
						m_iterator->SeekToLast();
					}
					return m_iterator->Valid();
				}

				sl_bool moveNext() override
				{
					if (m_iterator->Valid()) {
						m_iterator->Next();
					} else {
						m_iterator->SeekToFirst();
					}
					return m_iterator->Valid();
				}

				sl_bool seek(const void* key, sl_size sizeKey) override
				{
					m_iterator->Seek(leveldb::Slice((char*)key, sizeKey));
					return m_iterator->Valid();
				}

			};

			Ref<KeyValueIterator> LevelDBImpl::createIterator()
			{
				leveldb::Iterator* iterator = m_db->NewIterator(m_optionsRead);
				if (iterator) {
					Ref<KeyValueIterator> ret = new LevelDBIterator(this, iterator);
					if (ret.isNotNull()) {
						return ret;
					}
					delete iterator;
				}
				return sl_null;
			}

			class LevelDBSnapshot : public KeyValueSnapshot
			{
			public:
				Ref<LevelDBImpl> m_instance;
				leveldb::DB* m_db;
				const leveldb::Snapshot* m_snapshot;
				leveldb::ReadOptions m_optionsRead;

			public:
				LevelDBSnapshot(LevelDBImpl* instance, const leveldb::Snapshot* snapshot): m_instance(instance), m_snapshot(snapshot), m_optionsRead(instance->m_optionsRead)
				{
					m_db = instance->m_db;
					m_optionsRead.snapshot = snapshot;
				}

				~LevelDBSnapshot()
				{
					m_db->ReleaseSnapshot(m_snapshot);
				}

			public:
				sl_bool get(const void* key, sl_size sizeKey, MemoryData* value) override
				{
					std::string str;
					leveldb::Status status = m_db->Get(m_optionsRead, leveldb::Slice((char*)key, sizeKey), &str);
					if (status.ok()) {
						return DecodeValue(Move(str), value);
					}
					return sl_false;
				}

				Ref<KeyValueIterator> createIterator() override
				{
					leveldb::Iterator* iterator = m_db->NewIterator(m_optionsRead);
					if (iterator) {
						Ref<KeyValueIterator> ret = new LevelDBIterator(m_instance.get(), iterator);
						if (ret.isNotNull()) {
							return ret;
						}
						delete iterator;
					}
					return sl_null;
				}

			};

			Ref<KeyValueSnapshot> LevelDBImpl::createSnapshot()
			{
				const leveldb::Snapshot* snapshot = m_db->GetSnapshot();
				if (snapshot) {
					Ref<KeyValueSnapshot> ret = new LevelDBSnapshot(this, snapshot);
					if (ret.isNotNull()) {
						return ret;
					}
					m_db->ReleaseSnapshot(snapshot);
				}
				return sl_null;
			}

		}
	}

	using namespace priv::level_db;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(LevelDB_Param)

	LevelDB_Param::LevelDB_Param()
	{
		flagCreateIfMissing = sl_true;
	}


	SLIB_DEFINE_OBJECT(LevelDB, KeyValueStore)

	LevelDB::LevelDB()
	{
	}

	LevelDB::~LevelDB()
	{
	}

	Ref<LevelDB> LevelDB::open(const LevelDB_Param& param)
	{
		return Ref<LevelDB>::from(LevelDBImpl::open(param));
	}

	Ref<LevelDB> LevelDB::open(const StringParam& path)
	{
		LevelDB_Param param;
		param.path = path.toString();
		return open(param);
	}

}
