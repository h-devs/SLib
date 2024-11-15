/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/db/model.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(DatabaseModel, Object)

	DatabaseModel::DatabaseModel(const Ref<Database>& db, const Shared<Database::SelectParam>& query, const VariantList& params): m_db(db), m_query(query), m_params(params)
	{
	}

	DatabaseModel::~DatabaseModel()
	{
	}

	Ref<DatabaseModel> DatabaseModel::create(const Ref<Database>& db, const Shared<Database::SelectParam>& query)
	{
		if (db.isNotNull() && query.isNotNull()) {
			return new DatabaseModel(db, query, sl_null);
		}
		return sl_null;
	}

	Ref<DatabaseModel> DatabaseModel::create(const Ref<Database>& db, const DatabaseQuerySource& source)
	{
		if (db.isNotNull()) {
			Shared<Database::SelectParam> query = Shared<Database::SelectParam>::create();
			if (query.isNotNull()) {
				query->source = source;
				return new DatabaseModel(db, query, sl_null);
			}
		}
		return sl_null;
	}

	Ref<DatabaseModel> DatabaseModel::create(const Ref<Database>& db, const DatabaseQuerySource& source, const ListParam<DatabaseColumn>& columns)
	{
		if (db.isNotNull()) {
			Shared<Database::SelectParam> query = Shared<Database::SelectParam>::create();
			if (query.isNotNull()) {
				query->source = source;
				query->columns = columns.toList();
				return new DatabaseModel(db, query, sl_null);
			}
		}
		return sl_null;
	}

	Ref<DatabaseModel> DatabaseModel::create(const Ref<Database>& db, const DatabaseQuerySource& source, const DatabaseExpression& where)
	{
		if (db.isNotNull()) {
			Shared<Database::SelectParam> query = Shared<Database::SelectParam>::create();
			if (query.isNotNull()) {
				query->source = source;
				query->where = where;
				return new DatabaseModel(db, query, sl_null);
			}
		}
		return sl_null;
	}

	Ref<DatabaseModel> DatabaseModel::createBy(const Ref<Database>& db, const DatabaseQuerySource& source, const DatabaseExpression& where, const ListParam<Variant>& params)
	{
		if (db.isNotNull()) {
			Shared<Database::SelectParam> query = Shared<Database::SelectParam>::create();
			if (query.isNotNull()) {
				query->source = source;
				query->where = where;
				return new DatabaseModel(db, query, params.toList());
			}
		}
		return sl_null;
	}

	Ref<DatabaseModel> DatabaseModel::create(const Ref<Database>& db, const DatabaseQuerySource& source, const ListParam<DatabaseColumn>& columns, const DatabaseExpression& where)
	{
		if (db.isNotNull()) {
			Shared<Database::SelectParam> query = Shared<Database::SelectParam>::create();
			if (query.isNotNull()) {
				query->source = source;
				query->columns = columns.toList();
				query->where = where;
				return new DatabaseModel(db, query, sl_null);
			}
		}
		return sl_null;
	}

	Ref<DatabaseModel> DatabaseModel::createBy(const Ref<Database>& db, const DatabaseQuerySource& source, const ListParam<DatabaseColumn>& columns, const DatabaseExpression& where, const ListParam<Variant>& params)
	{
		if (db.isNotNull()) {
			Shared<Database::SelectParam> query = Shared<Database::SelectParam>::create();
			if (query.isNotNull()) {
				query->source = source;
				query->columns = columns.toList();
				query->where = where;
				return new DatabaseModel(db, query, params.toList());
			}
		}
		return sl_null;
	}
	
	VariantList DatabaseModel::getRecords(sl_uint64 index, sl_size count)
	{
		ObjectLocker lock(this);
		Database::SelectParam query = *m_query;
		if (query.offset || query.limit) {
			query.offset = query.offset + index;
			if (query.limit) {
				if (index >= query.limit) {
					return sl_null;
				}
				if (count > query.limit - index) {
					count = query.limit - index;
				}
			}
		} else {
			query.offset = index;
		}
		if (count != SLIB_SIZE_MAX) {
			query.limit = count;
		}
		Ref<DatabaseStatement> stmt = m_db->prepareQuery(query);
		if (stmt.isNotNull()) {
			Ref<DatabaseCursor> cursor = stmt->queryBy(m_params);
			if (cursor.isNotNull()) {
				VariantList ret;
				while (cursor->moveNext()) {
					ret.add_NoLock(cursor->getRow());
				}
				return ret;
			}
		}
		return sl_null;
	}

	sl_uint64 DatabaseModel::getRecordCount()
	{
		ObjectLocker lock(this);
		Database::SelectParam query = *m_query;
		sl_uint64 offset = query.offset;
		sl_uint64 limit = query.limit;
		query.columns = List<DatabaseColumn>::createFromElement(DatabaseExpression::count());
		query.offset = 0;
		query.limit = 0;
		sl_uint64 count = m_db->findValueBy(query, m_params).getUint64();
		if (offset || limit) {
			if (count <= offset) {
				return 0;
			}
			if (limit) {
				if (limit < count - offset) {
					return limit;
				}
			}
			return count - offset;
		} else {
			return count;
		}
	}

	sl_bool DatabaseModel::isSortable()
	{
		return sl_true;
	}

	void DatabaseModel::sort(const String& field, sl_bool flagAsc)
	{
		ObjectLocker lock(this);
		m_query->orders.setNull();
		if (field.isNotEmpty()) {
			m_query->addOrder(field, flagAsc ? DatabaseOrderType::Asc : DatabaseOrderType::Desc);
		}
		clearCache();
	}

	void DatabaseModel::filter(const Variant& filter)
	{
	}

}
