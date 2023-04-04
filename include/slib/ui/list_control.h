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

	class IListControlInstance;

	class SLIB_EXPORT ListControl : public View
	{
		SLIB_DECLARE_OBJECT

	public:
		ListControl();

		~ListControl();

	public:
		class Cell
		{
		public:
			String text;

		public:
			Cell();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Cell)
		};

		class Column
		{
		public:
			String title;
			sl_ui_len width;
			Alignment align;
			Alignment headerAlign;

		public:
			Column();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Column)
		};

		class Row
		{
		public:
			String id;
			List<Cell> cells;

		public:
			Row();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Row)
		};

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
		SLIB_DECLARE_EVENT_HANDLER(ListControl, SelectRow, sl_uint32 row, sl_uint32 former, UIEvent* ev /* nullable */)
		SLIB_DECLARE_EVENT_HANDLER(ListControl, ClickRow, sl_uint32 row, UIEvent* ev)
		SLIB_DECLARE_EVENT_HANDLER(ListControl, RightButtonClickRow, sl_uint32 row, UIEvent* ev)
		SLIB_DECLARE_EVENT_HANDLER(ListControl, DoubleClickRow, sl_uint32 row, UIEvent* ev)
		SLIB_DECLARE_EVENT_HANDLER(ListControl, ClickHeader, sl_uint32 column, UIEvent* ev)

	protected:
		Ref<ViewInstance> createNativeWidget(ViewInstance* parent) override;

		virtual Ptr<IListControlInstance> getListControlInstance();

	protected:
		void _selectRow(IListControlInstance* instance, sl_uint32 row, UIEvent* ev, UIUpdateMode mode);

		void _onSelectRow_NW(IListControlInstance* instance, sl_uint32 row);

		void _onClickRow_NW(IListControlInstance* instance, sl_uint32 row, const UIPoint& pt);

		void _onRightButtonClickRow_NW(IListControlInstance* instance, sl_uint32 row, const UIPoint& pt);

		void _onDoubleClickRow_NW(IListControlInstance* instance, sl_uint32 row, const UIPoint& pt);

		void _onClickHeader_NW(IListControlInstance* instance, sl_uint32 column, const UIPoint& pt);

	protected:
		CList<Column> m_columns;
		CList<Row> m_rows;
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
