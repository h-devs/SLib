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

#ifndef CHECKHEADER_SLIB_DB_TABLE_SOURCE
#define CHECKHEADER_SLIB_DB_TABLE_SOURCE

#include "database.h"

#include "../data/table_model.h"

namespace slib
{

	class Database;

	class SLIB_EXPORT DatabaseModel : public TableModel
	{
		SLIB_DECLARE_OBJECT

	protected:
		DatabaseModel(const Ref<Database>& db, const Shared<Database::SelectParam>& query, const VariantList& params);

		~DatabaseModel();

	public:
		static Ref<DatabaseModel> create(const Ref<Database>& db, const Shared<Database::SelectParam>& query);

		static Ref<DatabaseModel> create(const Ref<Database>& db, const DatabaseQuerySource& source);

		static Ref<DatabaseModel> create(const Ref<Database>& db, const DatabaseQuerySource& source, const ListParam<DatabaseColumn>& columns);

		static Ref<DatabaseModel> create(const Ref<Database>& db, const DatabaseQuerySource& source, const DatabaseExpression& where, const ListParam<Variant>& params);

		static Ref<DatabaseModel> create(const Ref<Database>& db, const DatabaseQuerySource& source, const ListParam<DatabaseColumn>& columns, const DatabaseExpression& where, const ListParam<Variant>& params);

	public:
		VariantList getRecords(sl_uint64 index, sl_size count) override;

		sl_uint64 getRecordCount() override;

		sl_bool isSortable() override;

		void sort(const String& field, sl_bool flagAsc) override;

	protected:
		Ref<Database> m_db;
		Shared<Database::SelectParam> m_query;
		VariantList m_params;

	};

}

#endif
