/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_TABLE_VIEW
#define CHECKHEADER_SLIB_UI_TABLE_VIEW

#include "view.h"

namespace slib
{

	class SLIB_EXPORT TableView : public View
	{
		SLIB_DECLARE_OBJECT

	public:
		TableView();

		~TableView();

	public:
		sl_uint64 getRowCount();
		sl_uint64 getColumnCount();

		virtual void setRowCount(sl_int64 rowCount, UIUpdateMode mode = UIUpdateMode::Redraw);
		virtual void setColumnCount(sl_int64 colCount, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getRowHeight();
		//sl_ui_len getRowWidth(sl_int32 colCount);
		sl_ui_len getColumnWidth(sl_int64 colStart, sl_int64 colEnd);

		virtual void setRowHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isMultipleSelection();
		sl_bool isRowSelected(sl_int64 rowIndex);
		sl_bool isColumnSelected(sl_int64 colIndex);
		sl_bool isItemSelected(sl_int64 rowIndex, sl_int64 colIndex);

		void setMultipleSelection(sl_bool flag, UIUpdateMode mode = UIUpdateMode::Redraw);
		sl_int64 getSelectedRow();
		sl_int64 getSelectedColumn();
		List<sl_uint64> getSelectedRows();
		List<sl_uint64> getSelectedColumns();

		void setRowSelected(sl_int64 rowIndex, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnSelected(sl_int64 colIndex, UIUpdateMode mode = UIUpdateMode::Redraw);


		void selectItem(sl_int64 rowIndex, sl_uint64 colIndex, UIUpdateMode mode = UIUpdateMode::Redraw);

		void unselectItem(sl_int64 rowIndex, sl_uint64 colIndex, UIUpdateMode mode = UIUpdateMode::Redraw);

		/*void toggleItemSelection(sl_int64 rowIndex, sl_uint64 colIndex, UIUpdateMode mode = UIUpdateMode::Redraw);

		void selectItems(const ListParam<sl_uint64>& indices, UIUpdateMode mode = UIUpdateMode::Redraw);

		void unselectItems(const ListParam<sl_uint64>& indices, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setSelectedRange(sl_int64 from, sl_int64 to, UIUpdateMode mode = UIUpdateMode::Redraw);*/

		//void selectRange(sl_int64 from, sl_int64 to, UIUpdateMode mode = UIUpdateMode::Redraw);

		void unselectAll(UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_int64 getHoverIndex();

		sl_int64 getRowIndexAt(const UIPoint& pt);
		sl_int64 getColumnIndexAt(const UIPoint& pt);

		Ref<Drawable> getItemBackground();

		void setItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setItemBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getSelectedItemBackground();

		void setSelectedItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setSelectedItemBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getHoverItemBackground();

		void setHoverItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setHoverItemBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getFocusedItemBackground();

		void setFocusedItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setFocusedItemBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

	public:
		SLIB_DECLARE_EVENT_HANDLER(TableView, DrawItem, sl_int64 rowIndex, sl_int64 colIndex, Canvas* canvas, UIRect& rcItem)

		SLIB_DECLARE_EVENT_HANDLER(TableView, ClickItem, sl_int64 rowIndex, sl_int64 colIndex, UIPoint& pos, UIEvent* ev)

		SLIB_DECLARE_EVENT_HANDLER(TableView, RightButtonClickItem, sl_int64 rowIndex, sl_int64 colIndex, UIPoint& pos, UIEvent* ev)

		SLIB_DECLARE_EVENT_HANDLER(TableView, DoubleClickItem, sl_int64 rowIndex, sl_int64 colIndex, UIPoint& pos, UIEvent* ev)

		SLIB_DECLARE_EVENT_HANDLER(TableView, ChangedSelection, UIEvent* ev)

	protected:
		void onDraw(Canvas* canvas) override;

		void onClickEvent(UIEvent* ev) override;

		void onMouseEvent(UIEvent* ev) override;

		void onKeyEvent(UIEvent* ev) override;

	protected:
		sl_int64 m_rowCount;
		sl_int64 m_columnCount;
		sl_ui_len m_rowHeight;
		List<sl_ui_len> m_columnWidth;

		sl_ui_len m_heightTopHeader;
		sl_ui_len m_heightBottomHeader;
		sl_ui_len m_widthLeftHeader;
		sl_ui_len m_widthRightHeader;

		sl_int64 m_indexHover;

		sl_bool m_flagMultipleSelection;
		sl_int64 m_selectedRow;
		sl_int64 m_selectedColumn;
		
		//sl_int64 m_indexFocused;
		sl_int64 m_lastSelectedRow;
		sl_int64 m_lastSelectedColumn;
		CHashMap<sl_uint64, sl_bool> m_mapRowSelection;
		CHashMap<sl_uint64, sl_bool> m_mapColumnSelection;

		AtomicRef<Drawable> m_backgroundItem;
		AtomicRef<Drawable> m_backgroundSelectedItem;
		AtomicRef<Drawable> m_backgroundHoverItem;
		AtomicRef<Drawable> m_backgroundFocusedItem;

	};

}

#endif
