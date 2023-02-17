/*
 *   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_GRID_VIEW
#define CHECKHEADER_SLIB_UI_GRID_VIEW

#include "view.h"

#include "../core/variant.h"
#include "../core/property.h"

namespace slib
{

	class SLIB_EXPORT GridView : public View
	{
		SLIB_DECLARE_OBJECT

	public:
		struct CellRegion
		{
			sl_uint64 startRow;
			sl_uint64 endRow;
			sl_uint32 startColumn;
			sl_uint32 endColumn;
		};

		class DrawCellParam
		{
		public:
			Canvas* canvas;
			UIRect region;
			sl_uint64 row;
			sl_uint32 col;
			Variant data;
			View* parent;
			sl_bool flagFixedCell;

		public:
			DrawCellParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DrawCellParam)
		};

		class CellEvent
		{
		public:
			UIEvent* event;
			UIRect region;
			Variant data;

		public:
			CellEvent();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CellEvent)
		};

		class Cell : public Referable
		{
		public:
			Cell();
			~Cell();

		public:
			virtual void onDraw(DrawCellParam& param) = 0;
			virtual void onEvent(CellEvent& ev);
		};

		class TextCell : public Cell
		{
		public:
			TextCell();
			~TextCell();

		public:
			void onDraw(DrawCellParam& param) override;
			void onEvent(CellEvent& ev) override;
		};

	public:
		GridView();

		~GridView();

	public:
		sl_uint64 getRowCount();

		void setRowCount(sl_uint64 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_uint32 getColumnCount();

		void setColumnCount(sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setFixedColumnCount(sl_uint32 leftCount, sl_uint32 rightCount, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTopHeaderText(sl_uint32 colIndex, const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setTopHeaderSize(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setBottomHeaderSize(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);
		sl_bool addHeaderColSpan(sl_uint32 colIndex, sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);
		void resetHeaderColSpan(UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getRowHeight();
		sl_ui_len getColumnWidth(sl_uint32 colStart, sl_uint32 colEnd);

		void setRowHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);
		void setColumnWidth(sl_uint32 colIndex, sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isMultipleSelection();
		sl_bool isRowSelected(sl_uint64 rowIndex);

		void setMultipleSelection(sl_bool flag, UIUpdateMode mode = UIUpdateMode::Redraw);
		sl_uint64 getSelectedRow();

		void setRowSelected(sl_uint64 rowIndex, UIUpdateMode mode = UIUpdateMode::Redraw);

		void unselectAll(UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_int64 getHoverIndex();

		sl_int64 getRowIndexAt(sl_int32 y);

		sl_int32 getColumnIndexAt(sl_int32 x);

		CellRegion getVisibleCellRegion();

		Ref<Drawable> getItemBackground();

		void setItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getFixedItemBackground(); 
		
		void setFixedItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode);

		void setItemBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setFixedItemBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

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
		SLIB_DECLARE_EVENT_HANDLER(GridView, ClickItem, sl_uint64 rowIndex, sl_uint32 colIndex, UIPoint& pos, UIEvent* ev)

		SLIB_DECLARE_EVENT_HANDLER(GridView, RightButtonClickItem, sl_uint64 rowIndex, sl_uint32 colIndex, UIPoint& pos, UIEvent* ev)

		SLIB_DECLARE_EVENT_HANDLER(GridView, DoubleClickItem, sl_uint64 rowIndex, sl_uint32 colIndex, UIPoint& pos, UIEvent* ev)

		SLIB_DECLARE_EVENT_HANDLER(GridView, ChangedSelection, UIEvent* ev)

	protected:
		void onDraw(Canvas* canvas) override;

		void onClickEvent(UIEvent* ev) override;

		void onMouseEvent(UIEvent* ev) override;

		void onKeyEvent(UIEvent* ev) override;

	protected:
		UIRect getGridCellRect(sl_uint64 rowIndex, sl_uint32 colIndex);
		UIRect getGridHeaderCellRect(sl_uint32 colIndex, sl_bool flagTop = sl_true);
		UIRect getGridFrozenCellRect(sl_uint64 rowIndex, sl_uint32 colIndex, sl_bool flagLeft = sl_true);
		UIPoint getPositionInGridCell(const UIPoint& pt, sl_uint64 rowIndex, sl_uint32 colIndex);
		void drawGridCellItem(sl_uint64 rowIndex, sl_uint32 colIndex, Canvas* canvas, UIRect& rcItem, sl_bool flagFixedCell = sl_false);
		void drawHeaderItem(sl_uint64 rowIndex, sl_uint32 colIndex, Canvas* canvas, UIRect& rcItem, sl_bool flagTop = sl_true);
		void drawGridHeader(Canvas* canvas, sl_bool	flagTop = sl_true);
		void drawGridFrozenArea(Canvas* canvas, sl_bool flagLeft = sl_true);
		virtual Ref<Cell> getCell(sl_uint64 row, sl_uint32 col);

		virtual Variant getCellData(sl_uint64 row, sl_uint32 col);

	public:
		SLIB_PROPERTY_FUNCTION(Ref<Cell>(sl_uint64, sl_uint32), CellCallback)
		SLIB_PROPERTY_FUNCTION(Variant(sl_uint64, sl_uint32), DataCallback)

	protected:
		sl_uint64 m_nRows;
		sl_uint32 m_nColumns;
		sl_uint32 m_nLeftColumns;
		sl_uint32 m_nRightColumns;

		sl_ui_len m_heightRow;
		sl_ui_len m_heightTopRow;
		sl_ui_len m_heightBottomRow;
		List<sl_ui_len> m_listColumnWidth;

		List<String> m_listHeaderText;
		Map<sl_int32, CellRegion> m_listHeaderSpan;


		sl_uint64 m_indexHover;
		sl_uint64 m_focusedRow;
		sl_uint64 m_focusedColumn;
		sl_uint64 m_selectedRow;

		AtomicRef<Drawable> m_backgroundItem;
		AtomicRef<Drawable> m_backgroundSelectedItem;
		AtomicRef<Drawable> m_backgroundHoverItem;
		AtomicRef<Drawable> m_backgroundFocusedItem;
		AtomicRef<Drawable> m_backgroundFixedItem;

		Ref<TextCell> m_textCell;

	};

}

#endif
