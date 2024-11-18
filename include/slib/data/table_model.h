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

#ifndef CHECKHEADER_SLIB_DATA_TABLE_SOURCE
#define CHECKHEADER_SLIB_DATA_TABLE_SOURCE

#include "definition.h"

#include "../core/variant.h"

namespace slib
{

	class SLIB_EXPORT TableModel : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		TableModel();

		~TableModel();

	public:
		virtual Variant getRecord(sl_uint64 index);

		virtual VariantList getRecords(sl_uint64 index = 0, sl_size count = SLIB_SIZE_MAX);

		virtual sl_uint64 getRecordCount() = 0;

		virtual sl_bool isSortable() = 0;

		virtual void sort(const String& field, sl_bool flagAsc) = 0;

		virtual void filter(const Variant& filter) = 0;

		sl_size getCacheItemCount();

		void setCacheItemCount(sl_size count);

		void clearCache();

	private:
		sl_size m_nCache;
		sl_uint64 m_indexCacheUp;
		VariantList m_cacheUp;
		sl_uint64 m_indexCacheDown;
		VariantList m_cacheDown;

	};

}

#endif
