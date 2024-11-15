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

#include "slib/data/table_model.h"

namespace slib
{
	
	SLIB_DEFINE_OBJECT(TableModel, Object)

	TableModel::TableModel()
	{
		m_indexCacheUp = 0;
		m_indexCacheDown = 0;
		m_nCache = 50;
	}

	TableModel::~TableModel()
	{
	}

	Variant TableModel::getRecord(sl_uint64 index)
	{
		ObjectLocker lock(this);
		sl_uint64 endCacheDown = m_indexCacheDown + m_cacheDown.getCount();
		if (index >= m_indexCacheDown && index < endCacheDown) {
			return m_cacheDown.getValueAt_NoLock((sl_size)(index - m_indexCacheDown));
		}
		if (index >= m_indexCacheUp && index < m_indexCacheUp + m_cacheUp.getCount()) {
			return m_cacheUp.getValueAt_NoLock((sl_size)(index - m_indexCacheUp));
		}
		if (index == endCacheDown) {
			m_cacheUp = Move(m_cacheDown);
			m_indexCacheUp = m_indexCacheDown;
			m_indexCacheDown = endCacheDown;
			m_cacheDown = getRecords(index, m_nCache);
			return m_cacheDown.getValueAt_NoLock(0);
		}
		if (m_indexCacheUp && index + m_nCache >= m_indexCacheUp && index + m_nCache < m_indexCacheUp + m_cacheUp.getCount()) {
			m_cacheDown = Move(m_cacheUp);
			m_indexCacheDown = m_indexCacheUp;
			sl_uint64 start = m_indexCacheUp < m_nCache ? 0 : m_indexCacheUp - m_nCache;
			m_cacheUp = getRecords(start, (sl_size)(m_indexCacheUp - start));
			m_indexCacheUp = start;
			return m_cacheUp.getValueAt_NoLock((sl_size)(index - start));
		}
		m_indexCacheDown = index;
		m_cacheDown = getRecords(index, m_nCache);
		return m_cacheDown.getValueAt_NoLock(0);
	}

	VariantList TableModel::getRecords(sl_uint64 index, sl_size count)
	{
		VariantList ret;
		for (sl_uint64 i = 0; i < count; i++) {
			Variant record = getRecord(index + i);
			if (record.isUndefined()) {
				break;
			}
			ret.add_NoLock(Move(record));
		}
		return ret;
	}

	sl_size TableModel::getCacheItemCount()
	{
		return m_nCache;
	}

	void TableModel::setCacheItemCount(sl_size count)
	{
		if (count < 1) {
			count = 1;
		}
		m_nCache = count;
	}

	void TableModel::clearCache()
	{
		ObjectLocker lock(this);
		m_cacheDown.setNull();
		m_cacheUp.setNull();
		m_indexCacheDown = 0;
		m_indexCacheUp = 0;
	}

}
