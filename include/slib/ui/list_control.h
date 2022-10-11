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

#ifndef CHECKHEADER_SLIB_UI_LIST_CONTROL
#define CHECKHEADER_SLIB_UI_LIST_CONTROL

#include "view.h"

#include "../core/string.h"

namespace slib
{
	
	class ListControlColumn
	{
	public:
		String title;
		sl_ui_len width;
		Alignment align;
		Alignment headerAlign;
		
	public:
		ListControlColumn();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ListControlColumn)
		
	};
	
	class ListControlCell
	{
	public:
		String text;

	public:
		ListControlCell();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ListControlCell)

	};

	class ListControlRow
	{
	public:
		String id;
		List<ListControlCell> cells;

	public:
		ListControlRow();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ListControlRow)

	};

	class IListControlInstance;
	
	class SLIB_EXPORT ListControl : public View
	{
		SLIB_DECLARE_OBJECT
		
	public:
		ListControl();
		
		~ListControl();

	public:
		sl_uint32 getColumnCount();
		
		// Run on UI thread
		virtual void setColumnCount(sl_uint32 nCount, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_uint32 getRowCount();
		
		// Run on UI thread
		virtual void setRowCount(sl_uint32 nCount, UIUpdateMode mode = UIUpdateMode::Redraw);

		void refreshItems(UIUpdateMode mode = UIUpdateMode::Redraw);

		virtual String getItemText(sl_uint32 row, sl_uint32 col);
		
		// Run on UI thread
		virtual void setItemText(sl_uint32 row, sl_uint32 col, const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);

		virtual String getRowId(sl_uint32 row);

		virtual void setRowId(sl_uint32 row, const String& id);

		virtual sl_int32 findRowById(const String& id);
		
		String getHeaderText(sl_uint32 col);
		
		// Run on UI thread
		virtual void setHeaderText(sl_uint32 col, const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_ui_len getColumnWidth(sl_uint32 col);
		
		// Run on UI thread
		virtual void setColumnWidth(sl_uint32 col, sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Alignment getHeaderAlignment(sl_uint32 col);
		
		// Run on UI thread
		virtual void setHeaderAlignment(sl_uint32 col, const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Alignment getColumnAlignment(sl_uint32 col);
		
		// Run on UI thread
		virtual void setColumnAlignment(sl_uint32 col, const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_int32 getSelectedRow();
		
		// Returns added row index. Run on UI thread
		virtual sl_uint32 addRow(UIUpdateMode mode = UIUpdateMode::Redraw);
		
		// Run on UI thread
		virtual void insertRow(sl_uint32 row, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		// Run on UI thread
		virtual void removeRow(sl_uint32 row, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		// Run on UI thread
		virtual void removeAllRows(UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_bool isSortingOnClickHeader();

		void setSortingOnClickHeader(sl_bool flag = sl_true);

		// Run on UI thread
		void sort(sl_uint32 col, sl_bool flagAsc = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

	public:
		SLIB_DECLARE_EVENT_HANDLER(ListControl, SelectRow, sl_uint32 row)
		SLIB_DECLARE_EVENT_HANDLER(ListControl, ClickRow, sl_uint32 row, const UIPoint& pt)
		SLIB_DECLARE_EVENT_HANDLER(ListControl, RightButtonClickRow, sl_uint32 row, const UIPoint& pt)
		SLIB_DECLARE_EVENT_HANDLER(ListControl, DoubleClickRow, sl_uint32 row, const UIPoint& pt)
		SLIB_DECLARE_EVENT_HANDLER(ListControl, ClickHeader, sl_uint32 column)

	protected:
		Ref<ViewInstance> createNativeWidget(ViewInstance* parent) override;
		
		virtual Ptr<IListControlInstance> getListControlInstance();

	protected:
		CList<ListControlColumn> m_columns;
		CList<ListControlRow> m_rows;
		sl_int32 m_selectedRow;

		sl_bool m_flagSortingOnClickHeader;
		sl_int32 m_sortedColumn;
		sl_bool m_flagSortedAsc;
		
	};
	
	class IListControlInstance
	{
	public:
		virtual void refreshColumnCount(ListControl* view) = 0;
		
		virtual void refreshRowCount(ListControl* view) = 0;
		
		virtual void setHeaderText(ListControl* view, sl_uint32 col, const String& text) = 0;
		
		virtual void setColumnWidth(ListControl* view, sl_uint32 col, sl_ui_len width) = 0;
		
		virtual void setHeaderAlignment(ListControl* view, sl_uint32 col, const Alignment& align) = 0;
		
		virtual void setColumnAlignment(ListControl* view, sl_uint32 col, const Alignment& align) = 0;
		
		virtual sl_bool getSelectedRow(ListControl* view, sl_int32& row) = 0;

	};
	
}

#endif
