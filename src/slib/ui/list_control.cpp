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
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ListControlColumn)

	ListControlColumn::ListControlColumn()
	{
		width = 40;
		align = Alignment::MiddleCenter;
		headerAlign = Alignment::MiddleCenter;
	}
	
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ListControlCell)
	
	ListControlCell::ListControlCell()
	{
	}
	
	
	SLIB_DEFINE_OBJECT(ListControl, View)
	
	ListControl::ListControl()
	{
		setSupportedNativeWidget(HAS_NATIVE_WIDGET_IMPL);
		setCreatingNativeWidget(HAS_NATIVE_WIDGET_IMPL);

		setUsingFont(sl_true);
		
		m_columns.setCount(1);
		m_nRows = 0;
		m_selectedRow = -1;
	}
	
	ListControl::~ListControl()
	{
	}

	sl_uint32 ListControl::getColumnCount()
	{
		return (sl_uint32)(m_columns.getCount());
	}
	
	void ListControl::setColumnCount(sl_uint32 nCount, UIUpdateMode mode)
	{
		Ptr<IListControlInstance> instance = getListControlInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setColumnCount, nCount, mode)
		}
		ObjectLocker lock(this);
		m_columns.setCount(nCount);
		if (instance.isNotNull()) {
			instance->refreshColumnCount(this);
		} else {
			invalidate(mode);
		}
	}
	
	sl_uint32 ListControl::getRowCount()
	{
		return m_nRows;
	}
	
	void ListControl::setRowCount(sl_uint32 nCount, UIUpdateMode mode)
	{
		Ptr<IListControlInstance> instance = getListControlInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setRowCount, nCount, mode)
		}
		ObjectLocker lock(this);
		if (nCount < m_cells.getCount()) {
			m_cells.setCount(nCount);
		}
		m_nRows = nCount;
		if (instance.isNotNull()) {
			instance->refreshRowCount(this);
		} else {
			invalidate(mode);
		}
	}
	
	String ListControl::getItemText(sl_uint32 iRow, sl_uint32 iCol)
	{
		List<ListControlCell> row = m_cells.getValueAt(iRow);
		if (row.isNotNull()) {
			MutexLocker lock(row.getLocker());
			if (iCol < row.getCount()) {
				ListControlCell* cell = row.getPointerAt(iCol);
				return cell->text;
			}
		}
		return sl_null;
	}
	
	void ListControl::setItemText(sl_uint32 iRow, sl_uint32 iCol, const String& text, UIUpdateMode mode)
	{
		Ptr<IListControlInstance> instance = getListControlInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setItemText, iRow, iCol, text, mode)
		}
		ObjectLocker lock(this);
		if (iRow < m_nRows) {
			if (iRow >= m_cells.getCount()) {
				if (!(m_cells.setCount(iRow + 1))) {
					return;
				}
			}
			List<ListControlCell> row = m_cells.getValueAt(iRow);
			if (row.isNull()) {
				row.setCount(iCol + 1);
				m_cells.setAt(iRow, row);
			}
			if (row.isNotNull()) {
				MutexLocker lock(row.getLocker());
				if (iCol >= row.getCount()) {
					if (!(row.setCount(iCol + 1))) {
						return;
					}
				}
				ListControlCell* cell = row.getPointerAt(iCol);
				cell->text = text;
			}
			if (SLIB_UI_UPDATE_MODE_IS_REDRAW(mode)) {
				if (instance.isNotNull()) {
					instance->refreshRowCount(this);
				} else {
					invalidate();
				}
			}
		}
	}
	
	String ListControl::getHeaderText(sl_uint32 iCol)
	{
		MutexLocker lock(m_columns.getLocker());
		if (iCol < m_columns.getCount()) {
			ListControlColumn* col = m_columns.getPointerAt(iCol);
			return col->title;
		}
		return sl_null;
	}
	
	void ListControl::setHeaderText(sl_uint32 iCol, const String& text, UIUpdateMode mode)
	{
		Ptr<IListControlInstance> instance = getListControlInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setHeaderText, iCol, text, mode)
		}
		MutexLocker lock(m_columns.getLocker());
		if (iCol < m_columns.getCount()) {
			ListControlColumn* col = m_columns.getPointerAt(iCol);
			col->title = text;
			if (instance.isNotNull()) {
				instance->setHeaderText(this, iCol, text);
			} else {
				invalidate(mode);
			}
		}
	}
	
	sl_ui_len ListControl::getColumnWidth(sl_uint32 iCol)
	{
		MutexLocker lock(m_columns.getLocker());
		if (iCol < m_columns.getCount()) {
			ListControlColumn* col = m_columns.getPointerAt(iCol);
			return col->width;
		}
		return 0;
	}
	
	void ListControl::setColumnWidth(sl_uint32 iCol, sl_ui_len width, UIUpdateMode mode)
	{
		Ptr<IListControlInstance> instance = getListControlInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setColumnWidth, iCol, width, mode)
		}
		MutexLocker lock(m_columns.getLocker());
		if (iCol < m_columns.getCount()) {
			ListControlColumn* col = m_columns.getPointerAt(iCol);
			col->width = width;
			if (instance.isNotNull()) {
				instance->setColumnWidth(this, iCol, width);
			} else {
				invalidate(mode);
			}
		}
	}
	
	Alignment ListControl::getHeaderAlignment(sl_uint32 iCol)
	{
		MutexLocker lock(m_columns.getLocker());
		if (iCol < m_columns.getCount()) {
			ListControlColumn* col = m_columns.getPointerAt(iCol);
			return col->headerAlign;
		}
		return Alignment::Center;
	}
	
	void ListControl::setHeaderAlignment(sl_uint32 iCol, const Alignment& align, UIUpdateMode mode)
	{
		Ptr<IListControlInstance> instance = getListControlInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setHeaderAlignment, iCol, align, mode)
		}
		MutexLocker lock(m_columns.getLocker());
		if (iCol < m_columns.getCount()) {
			ListControlColumn* col = m_columns.getPointerAt(iCol);
			col->headerAlign = align;
			if (instance.isNotNull()) {
				instance->setHeaderAlignment(this, iCol, align);
			} else {
				invalidate(mode);
			}
		}
	}
	
	Alignment ListControl::getColumnAlignment(sl_uint32 iCol)
	{
		MutexLocker lock(m_columns.getLocker());
		if (iCol < m_columns.getCount()) {
			ListControlColumn* col = m_columns.getPointerAt(iCol);
			return col->align;
		}
		return Alignment::Center;
	}
	
	void ListControl::setColumnAlignment(sl_uint32 iCol, const Alignment& align, UIUpdateMode mode)
	{
		Ptr<IListControlInstance> instance = getListControlInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setColumnAlignment, iCol, align, mode)
		}
		MutexLocker lock(m_columns.getLocker());
		if (iCol < m_columns.getCount()) {
			ListControlColumn* col = m_columns.getPointerAt(iCol);
			col->align = align;
			if (instance.isNotNull()) {
				instance->setColumnAlignment(this, iCol, align);
			} else {
				invalidate(mode);
			}
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
		ObjectLocker lock(this);
		sl_uint32 n = m_nRows;
		setRowCount(n + 1, mode);
		return n;
	}
	
	void ListControl::insertRow(sl_uint32 iRow, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		if (iRow < m_cells.getCount()) {
			m_cells.insert(iRow, List<ListControlCell>::null());
		}
		setRowCount(m_nRows+1, mode);
	}
	
	void ListControl::removeRow(sl_uint32 iRow, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		if (iRow < m_nRows) {
			if (iRow < m_cells.getCount()) {
				m_cells.removeAt(iRow);
			}
			setRowCount(m_nRows - 1, mode);
		}
	}
	
	void ListControl::removeAllRows(UIUpdateMode mode)
	{
		setRowCount(0, mode);
	}
	
	SLIB_DEFINE_EVENT_HANDLER(ListControl, SelectRow, sl_uint32 row)

	void ListControl::dispatchSelectRow(sl_uint32 row)
	{
		m_selectedRow = row;
		SLIB_INVOKE_EVENT_HANDLER(SelectRow, row)
	}
	
	SLIB_DEFINE_EVENT_HANDLER(ListControl, ClickRow, sl_uint32 row, const UIPoint& pt)

	void ListControl::dispatchClickRow(sl_uint32 row, const UIPoint& pt)
	{
		Ref<UIEvent> ev = UIEvent::createMouseEvent(UIAction::Unknown, (sl_ui_posf)(pt.x), (sl_ui_posf)(pt.y), Time::zero());
		if (ev.isNotNull()) {
			dispatchClickEvent(ev.get());
		}
		SLIB_INVOKE_EVENT_HANDLER(ClickRow, row, pt)
	}
	
	SLIB_DEFINE_EVENT_HANDLER(ListControl, RightButtonClickRow, sl_uint32 row, const UIPoint& pt)

	void ListControl::dispatchRightButtonClickRow(sl_uint32 row, const UIPoint& pt)
	{
		SLIB_INVOKE_EVENT_HANDLER(RightButtonClickRow, row, pt)
	}
	
	SLIB_DEFINE_EVENT_HANDLER(ListControl, DoubleClickRow, sl_uint32 row, const UIPoint& pt)

	void ListControl::dispatchDoubleClickRow(sl_uint32 row, const UIPoint& pt)
	{
		Ref<UIEvent> ev = UIEvent::createMouseEvent(UIAction::LeftButtonDoubleClick, (sl_real)(pt.x), (sl_real)(pt.y), Time::zero());
		if (ev.isNotNull()) {
			dispatchMouseEvent(ev.get());
		}
		SLIB_INVOKE_EVENT_HANDLER(DoubleClickRow, row, pt)
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
