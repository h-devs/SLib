/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/list_control.h"

#include "slib/ui/core.h"

#if defined(SLIB_UI_IS_MACOS) || defined(SLIB_UI_IS_WIN32) || defined(SLIB_UI_IS_GTK)
#	define HAS_NATIVE_WIDGET_IMPL 1
#else
#	define HAS_NATIVE_WIDGET_IMPL 0
#endif

namespace slib
{

	SLIB_DEFINE_OBJECT(ListControl, View)

	ListControl::ListControl()
	{
		setSupportedNativeWidget(HAS_NATIVE_WIDGET_IMPL);
		setCreatingNativeWidget(HAS_NATIVE_WIDGET_IMPL);

		setUsingFont(sl_true);

		m_columns.setCount(1);
		m_selectedRow = -1;

		m_flagSortingOnClickHeader = sl_false;
		m_sortedColumn = -1;
		m_flagSortedAsc = sl_false;
	}

	ListControl::~ListControl()
	{
	}
	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(ListControl, Cell)

	ListControl::Cell::Cell()
	{
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(ListControl, Column)

	ListControl::Column::Column()
	{
		width = 40;
		align = Alignment::MiddleCenter;
		headerAlign = Alignment::MiddleCenter;
	}

	sl_uint32 ListControl::getColumnCount()
	{
		return (sl_uint32)(m_columns.getCount());
	}

	void ListControl::setColumnCount(sl_uint32 nCount, UIUpdateMode mode)
	{
		{
			ObjectLocker lock(this);
			m_columns.setCount_NoLock(nCount);
		}
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		Ptr<IListControlInstance> instance = getListControlInstance();
		if (instance.isNotNull()) {
			if (UI::isUiThread()) {
				instance->refreshColumnCount(this);
			}
		} else {
			invalidate(mode);
		}
	}
	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(ListControl, Row)

	ListControl::Row::Row()
	{
	}

	sl_uint32 ListControl::getRowCount()
	{
		return (sl_uint32)(m_rows.getCount());
	}

	void ListControl::setRowCount(sl_uint32 nCount, UIUpdateMode mode)
	{
		{
			ObjectLocker lock(this);
			m_rows.setCount_NoLock(nCount);
		}
		refreshItems(mode);
	}

	void ListControl::refreshItems(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		Ptr<IListControlInstance> instance = getListControlInstance();
		if (instance.isNotNull()) {
			if (UI::isUiThread()) {
				instance->refreshRowCount(this);
			} else {
				UI::dispatchToUiThreadUrgently(SLIB_BIND_WEAKREF(void(), this, refreshItems, mode));
			}
		} else {
			invalidate(mode);
		}
	}

	String ListControl::getItemText(sl_uint32 iRow, sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			Cell* cell = row->cells.getPointerAt(iCol);
			if (cell) {
				return cell->text;
			}
		}
		return sl_null;
	}

	void ListControl::setItemText(sl_uint32 iRow, sl_uint32 iCol, const String& text, UIUpdateMode mode)
	{
		{
			ObjectLocker lock(this);
			Cell* cell = sl_null;
			Row* row = m_rows.getPointerAt(iRow);
			if (row) {
				cell = row->cells.getPointerAt(iCol);
				if (!cell) {
					if (row->cells.setCount_NoLock(iCol + 1)) {
						cell = row->cells.getPointerAt(iCol);
					}
				}
			}
			if (cell) {
				cell->text = text;
			} else {
				return;
			}
		}
		refreshItems(mode);
	}

	String ListControl::getRowId(sl_uint32 iRow)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			return row->id;
		}
		return sl_null;
	}

	void ListControl::setRowId(sl_uint32 iRow, const String& id)
	{
		ObjectLocker lock(this);
		Row* row = m_rows.getPointerAt(iRow);
		if (row) {
			row->id = id;
		}
	}

	sl_int32 ListControl::findRowById(const String& id)
	{
		ObjectLocker lock(this);
		ListElements<Row> rows(m_rows);
		for (sl_size i = 0; i < rows.count; i++) {
			if (rows[i].id == id) {
				return (sl_int32)i;
			}
		}
		return -1;
	}

	String ListControl::getHeaderText(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->title;
		}
		return sl_null;
	}

	void ListControl::setHeaderText(sl_uint32 iCol, const String& text, UIUpdateMode mode)
	{
		{
			ObjectLocker lock(this);
			Column* col = m_columns.getPointerAt(iCol);
			if (col) {
				col->title = text;
			} else {
				return;
			}
		}
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		Ptr<IListControlInstance> instance = getListControlInstance();
		if (instance.isNotNull()) {
			if (UI::isUiThread()) {
				instance->setHeaderText(this, iCol, text);
			}
		} else {
			invalidate(mode);
		}
	}

	sl_ui_len ListControl::getColumnWidth(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->width;
		}
		return 0;
	}

	void ListControl::setColumnWidth(sl_uint32 iCol, sl_ui_len width, UIUpdateMode mode)
	{
		{
			ObjectLocker lock(this);
			Column* col = m_columns.getPointerAt(iCol);
			if (col) {
				col->width = width;
			} else {
				return;
			}
		}
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		Ptr<IListControlInstance> instance = getListControlInstance();
		if (instance.isNotNull()) {
			if (UI::isUiThread()) {
				instance->setColumnWidth(this, iCol, width);
			}
		} else {
			invalidate(mode);
		}
	}

	Alignment ListControl::getHeaderAlignment(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->headerAlign;
		}
		return Alignment::Center;
	}

	void ListControl::setHeaderAlignment(sl_uint32 iCol, const Alignment& align, UIUpdateMode mode)
	{
		{
			ObjectLocker lock(this);
			Column* col = m_columns.getPointerAt(iCol);
			if (col) {
				col->headerAlign = align;
			} else {
				return;
			}
		}
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		Ptr<IListControlInstance> instance = getListControlInstance();
		if (instance.isNotNull()) {
			if (UI::isUiThread()) {
				instance->setHeaderAlignment(this, iCol, align);
			}
		} else {
			invalidate(mode);
		}
	}

	Alignment ListControl::getColumnAlignment(sl_uint32 iCol)
	{
		ObjectLocker lock(this);
		Column* col = m_columns.getPointerAt(iCol);
		if (col) {
			return col->align;
		}
		return Alignment::Center;
	}

	void ListControl::setColumnAlignment(sl_uint32 iCol, const Alignment& align, UIUpdateMode mode)
	{
		{
			ObjectLocker lock(this);
			Column* col = m_columns.getPointerAt(iCol);
			if (col) {
				col->align = align;
			} else {
				return;
			}
		}
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		Ptr<IListControlInstance> instance = getListControlInstance();
		if (instance.isNotNull()) {
			if (UI::isUiThread()) {
				instance->setColumnAlignment(this, iCol, align);
			}
		} else {
			invalidate(mode);
		}
	}

	sl_int32 ListControl::getSelectedRow()
	{
		Ptr<IListControlInstance> instance = getListControlInstance();
		if (instance.isNotNull()) {
			instance->getSelectedRow(this, m_selectedRow);
		}
		return m_selectedRow;
	}

	sl_uint32 ListControl::addRow(UIUpdateMode mode)
	{
		sl_uint32 n = (sl_uint32)(m_rows.getCount());
		setRowCount(n + 1, mode);
		return n;
	}

	void ListControl::insertRow(sl_uint32 iRow, UIUpdateMode mode)
	{
		{
			ObjectLocker lock(this);
			if (!(m_rows.insert_NoLock(iRow))) {
				return;
			}
		}
		refreshItems(mode);
	}

	void ListControl::removeRow(sl_uint32 iRow, UIUpdateMode mode)
	{
		{
			ObjectLocker lock(this);
			if (!(m_rows.removeAt_NoLock(iRow))) {
				return;
			}
		}
		refreshItems(mode);
	}

	void ListControl::removeAllRows(UIUpdateMode mode)
	{
		setRowCount(0, mode);
	}

	sl_bool ListControl::isSortingOnClickHeader()
	{
		return m_flagSortingOnClickHeader;
	}

	void ListControl::setSortingOnClickHeader(sl_bool flag)
	{
		m_flagSortingOnClickHeader = flag;
	}

	void ListControl::sort(sl_uint32 col, sl_bool flagAsc, UIUpdateMode mode)
	{
		struct Compare
		{
			sl_uint32 m_col;

			Compare(sl_uint32 col): m_col(col) {}

			sl_compare_result operator()(const Row& row1, const Row& row2) const noexcept
			{
				String value1;
				Cell* cell1 = row1.cells.getPointerAt(m_col);
				if (cell1) {
					value1 = cell1->text;
				}
				String value2;
				Cell* cell2 = row2.cells.getPointerAt(m_col);
				if (cell2) {
					value2 = cell2->text;
				}
				return value1.compare(value2);
			}
		};
		{
			ObjectLocker lock(this);
			if (flagAsc) {
				m_rows.sort_NoLock(Compare(col));
			} else {
				m_rows.sortDesc_NoLock(Compare(col));
			}
		}
		refreshItems(mode);
	}

	SLIB_DEFINE_EVENT_HANDLER(ListControl, SelectRow, (sl_uint32 row, sl_int32 former, UIEvent* ev /* nullable */), row, former, ev)

	void ListControl::_selectRow(IListControlInstance* instance, sl_uint32 row, UIEvent* ev, UIUpdateMode mode)
	{
		ObjectLocker locker(this);
		sl_int32 oldRow = m_selectedRow;
		if (oldRow == row) {
			return;
		}
		oldRow = row;
		locker.unlock();
		invokeSelectRow(row, oldRow, ev);
	}

	void ListControl::_onSelectRow_NW(IListControlInstance* instance, sl_uint32 row)
	{
		Ref<UIEvent> ev = UIEvent::createUnknown(Time::now());
		if (ev.isNotNull()) {
			_selectRow(instance, row, ev.get(), UIUpdateMode::None);
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(ListControl, ClickRow, (sl_uint32 row, UIEvent* ev), row, ev)

	void ListControl::_onClickRow_NW(sl_uint32 row, const UIPoint& pt)
	{
		Ref<UIEvent> ev = UIEvent::createMouseEvent(UIAction::LeftButtonDown, pt.x, pt.y, Time::now());
		if (ev.isNotNull()) {
			invokeClickRow(row, ev.get());
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(ListControl, RightButtonClickRow, (sl_uint32 row, UIEvent* ev), row, ev)

	void ListControl::_onRightButtonClickRow_NW(sl_uint32 row, const UIPoint& pt)
	{
		Ref<UIEvent> ev = UIEvent::createMouseEvent(UIAction::RightButtonDown, pt.x, pt.y, Time::now());
		if (ev.isNotNull()) {
			invokeRightButtonClickRow(row, ev.get());
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(ListControl, DoubleClickRow, (sl_uint32 row, UIEvent* ev), row, ev)

	void ListControl::_onDoubleClickRow_NW(sl_uint32 row, const UIPoint& pt)
	{
		Ref<UIEvent> ev = UIEvent::createMouseEvent(UIAction::LeftButtonDoubleClick, pt.x, pt.y, Time::now());
		if (ev.isNotNull()) {
			invokeDoubleClickRow(row, ev.get());
		}
	}

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(ListControl, ClickHeader, (sl_uint32 col, UIEvent* ev), col, ev)

	void ListControl::onClickHeader(sl_uint32 column, UIEvent* ev)
	{
		if (m_flagSortingOnClickHeader) {
			if (m_sortedColumn == column) {
				m_flagSortedAsc = !m_flagSortedAsc;
			} else {
				m_sortedColumn = column;
				m_flagSortedAsc = sl_true;
			}
			sort(column, m_flagSortedAsc);
		}
	}

	void ListControl::_onClickHeader_NW(sl_uint32 column, const UIPoint& pt)
	{
		Ref<UIEvent> ev = UIEvent::createMouseEvent(UIAction::LeftButtonDown, pt.x, pt.y, Time::now());
		if (ev.isNotNull()) {
			invokeClickHeader(column, ev.get());
		}
	}

#if !HAS_NATIVE_WIDGET_IMPL
	Ref<ViewInstance> ListControl::createNativeWidget(ViewInstance* parent)
	{
		return sl_null;
	}

	Ptr<IListControlInstance> ListControl::getListControlInstance()
	{
		return sl_null;
	}
#endif

}
