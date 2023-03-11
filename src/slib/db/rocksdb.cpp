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

#include "slib/db/rocksdb.h"

#include "slib/core/memory.h"

#ifdef SLIB_PLATFORM_IS_WIN32
#pragma warning(disable: 4996)
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "shlwapi.lib")
#endif

#include "rocksdb/db.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/env.h"

namespace slib
{

	namespace {

		class StdStringContainer : public CRef
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

		class RocksDBImpl : public RocksDB
		{
		public:
			rocksdb::DB* m_db;

			rocksdb::ReadOptions m_optionsRead;
			rocksdb::WriteOptions m_optionsWrite;

		public:
			RocksDBImpl()
			{
			}

			~RocksDBImpl()
			{
				delete m_db;
			}

		public:
			static Ref<RocksDBImpl> open(const RocksDB_Param& param)
			{
				StringCstr path(param.path);
				if (path.isEmpty()) {
					return sl_null;
				}

				rocksdb::Options options;
				options.create_if_missing = (bool)(param.flagCreateIfMissing);
				options.keep_log_file_num = 2;

				rocksdb::DB* db = sl_null;
				rocksdb::Status status = rocksdb::DB::Open(options, path.getData(), &db);
				if (db) {
					if (status.ok()) {
						Ref<RocksDBImpl> ret = new RocksDBImpl;
						if (ret.isNotNull()) {
							ret->m_db = db;
							return ret;
						}
					}
					delete db;
				}
				return sl_null;
			}

		public:
			Ref<KeyValueWriteBatch> createWriteBatch() override;

			Ref<KeyValueIterator> getIterator() override;

			Ref<KeyValueSnapshot> getSnapshot() override;

			sl_bool get(const void* key, sl_size sizeKey, MemoryData* value) override
			{
				std::string str;
				rocksdb::Status status = m_db->Get(m_optionsRead, rocksdb::Slice((char*)key, sizeKey), &str);
				if (status.ok()) {
					return DecodeValue(Move(str), value);
				}
				return sl_false;
			}

			sl_bool put(const void* key, sl_size sizeKey, const void* value, sl_size sizeValue) override
			{
				rocksdb::Status status = m_db->Put(m_optionsWrite, rocksdb::Slice((char*)key, sizeKey), rocksdb::Slice((char*)value, sizeValue));
				return status.ok();
			}

			sl_bool remove(const void* key, sl_size sizeKey) override
			{
				rocksdb::Status status = m_db->Delete(m_optionsWrite, rocksdb::Slice((char*)key, sizeKey));
				return status.ok();
			}

			sl_bool compact(const void* from, sl_size sizeFrom, const void* end, sl_size sizeEnd) override
			{
				if (from) {
					if (end) {
						rocksdb::Slice f((char*)from, sizeFrom);
						rocksdb::Slice e((char*)end, sizeEnd);
						m_db->CompactRange(rocksdb::CompactRangeOptions(), &f, &e);
					} else {
						rocksdb::Slice f((char*)from, sizeFrom);
						m_db->CompactRange(rocksdb::CompactRangeOptions(), &f, sl_null);
					}
				} else {
					if (end) {
						rocksdb::Slice e((char*)end, sizeEnd);
						m_db->CompactRange(rocksdb::CompactRangeOptions(), sl_null, &e);
					} else {
						m_db->CompactRange(rocksdb::CompactRangeOptions(), sl_null, sl_null);
					}
				}
				return sl_true;
			}

		};

		class RocksDBWriteBatch : public KeyValueWriteBatch
		{
		public:
			Ref<RocksDBImpl> m_instance;
			rocksdb::DB* m_db;
			rocksdb::WriteBatch m_updates;

		public:
			RocksDBWriteBatch(RocksDBImpl* instance): m_instance(instance)
			{
				m_db = instance->m_db;
			}

			~RocksDBWriteBatch()
			{
				discard();
			}

		public:
			sl_bool put(const void* key, sl_size sizeKey, const void* value, sl_size sizeValue) override
			{
				m_updates.Put(rocksdb::Slice((char*)key, sizeKey), rocksdb::Slice((char*)value, sizeValue));
				return sl_true;
			}

			sl_bool remove(const void* key, sl_size sizeKey) override
			{
				m_updates.Delete(rocksdb::Slice((char*)key, sizeKey));
				return sl_true;
			}

			sl_bool _commit() override
			{
				rocksdb::Status status = m_db->Write(m_instance->m_optionsWrite, &m_updates);
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

		Ref<KeyValueWriteBatch> RocksDBImpl::createWriteBatch()
		{
			return new RocksDBWriteBatch(this);
		}

		class RocksDBIterator : public KeyValueIterator
		{
		public:
			Ref<RocksDBImpl> m_instance;
			rocksdb::Iterator* m_iterator;

		public:
			RocksDBIterator(RocksDBImpl* instance, rocksdb::Iterator* iterator): m_instance(instance), m_iterator(iterator) {}

			~RocksDBIterator()
			{
				delete m_iterator;
			}

		public:
			sl_reg getKey(void* value, sl_size sizeValue) override
			{
				if (m_iterator->Valid()) {
					rocksdb::Slice slice = m_iterator->key();
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
					rocksdb::Slice slice = m_iterator->value();
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
				m_iterator->Seek(rocksdb::Slice((char*)key, sizeKey));
				return m_iterator->Valid();
			}

		};

		Ref<KeyValueIterator> RocksDBImpl::getIterator()
		{
			rocksdb::Iterator* iterator = m_db->NewIterator(m_optionsRead);
			if (iterator) {
				Ref<KeyValueIterator> ret = new RocksDBIterator(this, iterator);
				if (ret.isNotNull()) {
					return ret;
				}
				delete iterator;
			}
			return sl_null;
		}

		class RocksDBSnapshot : public KeyValueSnapshot
		{
		public:
			Ref<RocksDBImpl> m_instance;
			rocksdb::DB* m_db;
			const rocksdb::Snapshot* m_snapshot;
			rocksdb::ReadOptions m_optionsRead;

		public:
			RocksDBSnapshot(RocksDBImpl* instance, const rocksdb::Snapshot* snapshot): m_instance(instance), m_snapshot(snapshot), m_optionsRead(instance->m_optionsRead)
			{
				m_db = instance->m_db;
				m_optionsRead.snapshot = snapshot;
			}

			~RocksDBSnapshot()
			{
				m_db->ReleaseSnapshot(m_snapshot);
			}

		public:
			sl_bool get(const void* key, sl_size sizeKey, MemoryData* value) override
			{
				std::string str;
				rocksdb::Status status = m_db->Get(m_optionsRead, rocksdb::Slice((char*)key, sizeKey), &str);
				if (status.ok()) {
					return DecodeValue(Move(str), value);
				}
				return sl_false;
			}

			Ref<KeyValueIterator> getIterator() override
			{
				rocksdb::Iterator* iterator = m_db->NewIterator(m_optionsRead);
				if (iterator) {
					Ref<KeyValueIterator> ret = new RocksDBIterator(m_instance.get(), iterator);
					if (ret.isNotNull()) {
						return ret;
					}
					delete iterator;
				}
				return sl_null;
			}

		};

		Ref<KeyValueSnapshot> RocksDBImpl::getSnapshot()
		{
			const rocksdb::Snapshot* snapshot = m_db->GetSnapshot();
			if (snapshot) {
				Ref<KeyValueSnapshot> ret = new RocksDBSnapshot(this, snapshot);
				if (ret.isNotNull()) {
					return ret;
				}
				m_db->ReleaseSnapshot(snapshot);
			}
			return sl_null;
		}

	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(RocksDB_Param)

	RocksDB_Param::RocksDB_Param()
	{
		flagCreateIfMissing = sl_true;
	}


	SLIB_DEFINE_OBJECT(RocksDB, KeyValueStore)

	RocksDB::RocksDB()
	{
	}

	RocksDB::~RocksDB()
	{
	}

	Ref<RocksDB> RocksDB::open(const RocksDB_Param& param)
	{
		return Ref<RocksDB>::from(RocksDBImpl::open(param));
	}

	Ref<RocksDB> RocksDB::open(const StringParam& path)
	{
		RocksDB_Param param;
		param.path = path.toString();
		return open(param);
	}

}
