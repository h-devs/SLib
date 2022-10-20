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

#include "slib/db/lmdb.h"

#include "slib/core/file.h"
#include "slib/core/memory.h"

#include "lmdb/lmdb.h"

namespace slib
{

	namespace priv
	{
		namespace lmdb
		{

			class LMDBImpl : public LMDB
			{
			public:
				MDB_env* m_env;

			public:
				LMDBImpl()
				{
				}

				~LMDBImpl()
				{
					mdb_env_close(m_env);
				}

			public:
				static Ref<LMDBImpl> open(const LMDB_Param& param)
				{
					StringCstr path(param.path);
					if (path.isEmpty()) {
						return sl_null;
					}
					
					if (param.flagCreateIfMissing) {
						if (!(File::exists(path))) {
							if (!(File::createDirectory(path))) {
								return sl_null;
							}
						}
					}

					MDB_env* env = sl_null;
					int iResult = mdb_env_create(&env);
					mdb_env_set_mapsize(env, 10485760);
					mdb_env_set_maxdbs(env, 4);
					if (!iResult) {
						iResult = mdb_env_open(env, "./testdb", MDB_WRITEMAP&MDB_MAPASYNC&MDB_NOLOCK, (int)(param.mode));
						if (!iResult) {
							Ref<LMDBImpl> ret = new LMDBImpl;
							if (ret.isNotNull()) {
								ret->m_env = env;
								return ret;
							}
						}
						mdb_env_close(env);
					}
					return sl_null;
				}

			public:
				Ref<KeyValueWriteBatch> createWriteBatch() override;

				Ref<KeyValueIterator> getIterator() override;

				Ref<KeyValueSnapshot> getSnapshot() override;

				sl_bool get(const void* key, sl_size sizeKey, MemoryData* value) override
				{
					sl_bool bRet = sl_false;
					MDB_txn* txn = sl_null;
					int iResult = mdb_txn_begin(m_env, sl_null, MDB_RDONLY, &txn);
					if (!iResult) {
						MDB_dbi dbi = 0;
						iResult = mdb_dbi_open(txn, sl_null, 0, &dbi);
						if (!iResult) {
							MDB_val k, v;
							k.mv_data = (void*)key;
							k.mv_size = (size_t)sizeKey;
							iResult = mdb_get(txn, dbi, &k, &v);
							if (!iResult) {
								value->data = v.mv_data;
								value->size = (sl_size)(v.mv_size);
								bRet = sl_true;
							}
							mdb_dbi_close(m_env, dbi);
						}
						mdb_txn_abort(txn);
					}
					return bRet;
				}

				sl_bool put(const void* key, sl_size sizeKey, const void* value, sl_size sizeValue) override
				{
					sl_bool bRet = sl_false;
					MDB_txn* txn = sl_null;
					int iResult = mdb_txn_begin(m_env, sl_null, 0, &txn);
					if (!iResult) {
						MDB_dbi dbi = 0;
						iResult = mdb_dbi_open(txn, sl_null, 0, &dbi);
						if (!iResult) {
							MDB_val k, v;
							k.mv_data = (void*)key;
							k.mv_size = (size_t)sizeKey;
							v.mv_data = (void*)value;
							v.mv_size = (size_t)sizeValue;
							iResult = mdb_put(txn, dbi, &k, &v, 0);
							if (!iResult) {
								bRet = sl_true;
							}
							mdb_dbi_close(m_env, dbi);
						}
						mdb_txn_commit(txn);
					}
					return bRet;
				}

				sl_bool remove(const void* key, sl_size sizeKey) override
				{
					sl_bool bRet = sl_false;
					MDB_txn* txn = sl_null;
					int iResult = mdb_txn_begin(m_env, sl_null, 0, &txn);
					if (!iResult) {
						MDB_dbi dbi = 0;
						iResult = mdb_dbi_open(txn, sl_null, 0, &dbi);
						if (!iResult) {
							MDB_val k;
							k.mv_data = (void*)key;
							k.mv_size = (size_t)sizeKey;
							iResult = mdb_del(txn, dbi, &k, sl_null);
							if (!iResult) {
								bRet = sl_true;
							}
							mdb_dbi_close(m_env, dbi);
						}
						mdb_txn_commit(txn);
					}
					return bRet;
				}

			};

			class LMDBWriteBatch : public KeyValueWriteBatch
			{
			public:
				Ref<LMDBImpl> m_instance;
				MDB_txn* m_txn;
				MDB_dbi m_dbi;

			public:
				LMDBWriteBatch(LMDBImpl* instance, MDB_txn* txn, MDB_dbi dbi): m_instance(instance), m_txn(txn), m_dbi(dbi) {}

				~LMDBWriteBatch()
				{
					discard();
				}

			public:
				sl_bool put(const void* key, sl_size sizeKey, const void* value, sl_size sizeValue) override
				{
					MDB_val k, v;
					k.mv_data = (void*)key;
					k.mv_size = (size_t)sizeKey;
					v.mv_data = (void*)value;
					v.mv_size = (size_t)sizeValue;
					return !(mdb_put(m_txn, m_dbi, &k, &v, 0));
				}

				sl_bool remove(const void* key, sl_size sizeKey) override
				{
					MDB_val k;
					k.mv_data = (void*)key;
					k.mv_size = (size_t)sizeKey;
					return !(mdb_del(m_txn, m_dbi, &k, sl_null));
				}

				sl_bool _commit() override
				{
					mdb_dbi_close(m_instance->m_env, m_dbi);
					return !(mdb_txn_commit(m_txn));
				}

				void _discard() override
				{
					mdb_dbi_close(m_instance->m_env, m_dbi);
					mdb_txn_abort(m_txn);
				}

			};

			Ref<KeyValueWriteBatch> LMDBImpl::createWriteBatch()
			{
				MDB_txn* txn = sl_null;
				int iResult = mdb_txn_begin(m_env, sl_null, 0, &txn);
				if (!iResult) {
					MDB_dbi dbi = 0;
					iResult = mdb_dbi_open(txn, sl_null, 0, &dbi);
					if (!iResult) {
						Ref<KeyValueWriteBatch> ret = new LMDBWriteBatch(this, txn, dbi);
						if (ret.isNotNull()) {
							return ret;
						}
						mdb_dbi_close(m_env, dbi);
					}
					mdb_txn_abort(txn);
				}
				return sl_null;
			}

			class LMDBIterator : public KeyValueIterator
			{
			public:
				Ref<LMDBImpl> m_instance;
				MDB_txn* m_txn;
				MDB_dbi m_dbi;
				MDB_cursor* m_cursor;

				sl_bool m_flagValidItem;
				MDB_val m_key;
				MDB_val m_value;

			public:
				LMDBIterator(LMDBImpl* instance, MDB_txn* txn, MDB_dbi dbi, MDB_cursor* cursor): m_instance(instance), m_txn(txn), m_dbi(dbi), m_cursor(cursor), m_flagValidItem(sl_false), m_key({0}), m_value({0}) {}

				~LMDBIterator()
				{
					mdb_cursor_close(m_cursor);
					if (m_txn) {
						mdb_dbi_close(m_instance->m_env, m_dbi);
						mdb_txn_abort(m_txn);
					}
				}

			public:
				sl_bool getKey(MemoryData* pOut) override
				{
					if (m_flagValidItem) {
						pOut->data = m_key.mv_data;
						pOut->size = m_key.mv_size;
						return sl_true;
					}
					return sl_false;
				}

				sl_bool getValue(MemoryData* pOut) override
				{
					if (m_flagValidItem) {
						pOut->data = m_value.mv_data;
						pOut->size = m_value.mv_size;
						return sl_true;
					}
					return sl_false;
				}

				sl_bool moveFirst() override
				{
					m_flagValidItem = !(mdb_cursor_get(m_cursor, &m_key, &m_value, MDB_FIRST));
					return m_flagValidItem;
				}

				sl_bool moveLast() override
				{
					m_flagValidItem = !(mdb_cursor_get(m_cursor, &m_key, &m_value, MDB_LAST));
					return m_flagValidItem;
				}

				sl_bool movePrevious() override
				{
					m_flagValidItem = !(mdb_cursor_get(m_cursor, &m_key, &m_value, MDB_PREV));
					return m_flagValidItem;
				}

				sl_bool moveNext() override
				{
					m_flagValidItem = !(mdb_cursor_get(m_cursor, &m_key, &m_value, MDB_NEXT));
					return m_flagValidItem;
				}

				sl_bool seek(const void* key, sl_size sizeKey) override
				{
					MDB_val k;
					k.mv_data = (void*)key;
					k.mv_size = (size_t)sizeKey;
					if (!(mdb_cursor_get(m_cursor, &k, &m_value, MDB_SET_RANGE))) {
						m_flagValidItem = !(mdb_cursor_get(m_cursor, &m_key, &m_value, MDB_GET_CURRENT));
					} else {
						m_flagValidItem = sl_false;
					}
					return m_flagValidItem;
				}

			};

			Ref<KeyValueIterator> LMDBImpl::getIterator()
			{
				MDB_txn* txn = sl_null;
				int iResult = mdb_txn_begin(m_env, sl_null, MDB_RDONLY, &txn);
				if (!iResult) {
					MDB_dbi dbi = 0;
					iResult = mdb_dbi_open(txn, sl_null, 0, &dbi);
					if (!iResult) {
						MDB_cursor* cursor = sl_null;
						iResult = mdb_cursor_open(txn, dbi, &cursor);
						if (!iResult) {
							Ref<KeyValueIterator> ret = new LMDBIterator(this, txn, dbi, cursor);
							if (ret.isNotNull()) {
								return ret;
							}
							mdb_cursor_close(cursor);
						}
						mdb_dbi_close(m_env, dbi);
					}
					mdb_txn_abort(txn);
				}
				return sl_null;
			}

			class LMDBSnapshot : public KeyValueSnapshot
			{
			public:
				Ref<LMDBImpl> m_instance;
				MDB_txn* m_txn;
				MDB_dbi m_dbi;

			public:
				LMDBSnapshot(LMDBImpl* instance, MDB_txn* txn, MDB_dbi dbi): m_instance(instance), m_txn(txn), m_dbi(dbi) {}

				~LMDBSnapshot()
				{
					mdb_dbi_close(m_instance->m_env, m_dbi);
					mdb_txn_abort(m_txn);
				}

			public:
				sl_bool get(const void* key, sl_size sizeKey, MemoryData* value) override
				{
					MDB_val k, v;
					k.mv_data = (void*)key;
					k.mv_size = (size_t)sizeKey;
					int iResult = mdb_get(m_txn, m_dbi, &k, &v);
					if (!iResult) {
						value->data = v.mv_data;
						value->size = (sl_size)(v.mv_size);
						return sl_true;
					}
					return sl_false;
				}

				Ref<KeyValueIterator> getIterator() override
				{
					MDB_cursor* cursor = sl_null;
					int iResult = mdb_cursor_open(m_txn, m_dbi, &cursor);
					if (!iResult) {
						Ref<KeyValueIterator> ret = new LMDBIterator(m_instance.get(), sl_null, 0, cursor);
						if (ret.isNotNull()) {
							return ret;
						}
						mdb_cursor_close(cursor);
					}
					return sl_null;
				}

			};

			Ref<KeyValueSnapshot> LMDBImpl::getSnapshot()
			{
				MDB_txn* txn = sl_null;
				int iResult = mdb_txn_begin(m_env, sl_null, 0, &txn);
				if (!iResult) {
					MDB_dbi dbi = 0;
					iResult = mdb_dbi_open(txn, sl_null, 0, &dbi);
					if (!iResult) {
						Ref<KeyValueSnapshot> ret = new LMDBSnapshot(this, txn, dbi);
						if (ret.isNotNull()) {
							return ret;
						}
						mdb_dbi_close(m_env, dbi);
					}
					mdb_txn_abort(txn);
				}
				return sl_null;
			}

		}
	}

	using namespace priv::lmdb;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(LMDB_Param)

	LMDB_Param::LMDB_Param()
	{
		flagCreateIfMissing = sl_true;
		mode = 0664;
	}


	SLIB_DEFINE_OBJECT(LMDB, KeyValueStore)

	LMDB::LMDB()
	{
	}

	LMDB::~LMDB()
	{
	}

	Ref<LMDB> LMDB::open(const LMDB_Param& param)
	{
		return Ref<LMDB>::from(LMDBImpl::open(param));
	}

	Ref<LMDB> LMDB::open(const StringParam& path)
	{
		LMDB_Param param;
		param.path = path.toString();
		return open(param);
	}

}
