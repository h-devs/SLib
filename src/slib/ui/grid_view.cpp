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

#include "slib/ui/grid_view.h"

#include "slib/graphics/canvas.h"
#include "slib/graphics/text.h"

namespace slib
{

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, DrawCellParam)

	GridView::DrawCellParam::DrawCellParam()
	{
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(GridView, CellEvent)

	GridView::CellEvent::CellEvent()
	{
	}


	GridView::Cell::Cell()
	{
	}

	GridView::Cell::~Cell()
	{
	}

	void GridView::Cell::onEvent(CellEvent& ev)
	{
	}


	SLIB_DEFINE_OBJECT(GridView, View)

	GridView::GridView()
	{
		setCanvasScrolling(sl_false);
		setVerticalScrolling(sl_true, UIUpdateMode::Init);
		setHorizontalScrolling(sl_true, UIUpdateMode::Init);
		setContentScrollingByMouse(sl_false);
		setFocusable();
		setUsingFont();

		m_nRows = 0;
		m_nColumns = 0;
		m_heightRow = 100;
		m_indexHover = -1;

		m_heightBottomRow = 0;
		m_heightTopRow = 0;
		m_nLeftColumns = 0;
		m_nRightColumns = 0;

		m_textCell = New<TextCell>();
		setCellCallback([this](sl_int64 row, sl_int32 column) {
			return m_textCell;
		});
	}

	GridView::~GridView()
	{
	}

	sl_uint64 GridView::getRowCount()
	{
		return m_nRows;
	}

	void GridView::setRowCount(sl_uint64 nRows, UIUpdateMode mode)
	{
		if (m_nRows == nRows) {
			return;
		}
		m_nRows = nRows;
		setContentHeight((sl_scroll_pos)(m_nRows * m_heightRow + m_heightTopRow + m_heightBottomRow), mode);
	}

	sl_uint32 GridView::getColumnCount()
	{
		return m_nColumns;
	}

	void GridView::setColumnCount(sl_uint32 nColumns, UIUpdateMode mode)
	{
		if (m_nColumns == nColumns) {
			return;
		}
		m_nColumns = nColumns;
		m_listColumnWidth.removeAll();
		m_listHeaderText.removeAll();
		m_listHeaderSpan.removeAll();
		for (sl_uint32 index = 0; index < nColumns; index++) {
			m_listColumnWidth.add(77);
			m_listHeaderText.add_NoLock("Header" + String::fromInt(index));
		}

		sl_ui_len width = getColumnWidth(0, nColumns);
		setContentWidth(width, mode);
	}

	void GridView::setFixedColumnCount(sl_uint32 leftCount, sl_uint32 rightCount, UIUpdateMode mode)
	{
		if (leftCount && leftCount < m_nColumns - 1) {
			m_nLeftColumns = leftCount;
			invalidate(mode);
		}
		if (rightCount && rightCount < m_nColumns - m_nLeftColumns) {
			m_nRightColumns = rightCount;
			invalidate(mode);
		}
	}

	void GridView::setTopHeaderText(sl_uint32 colIndex, const String& text, UIUpdateMode mode)
	{
		if (colIndex >= m_nColumns) {
			return;
		}
		m_listHeaderText.setAt_NoLock(colIndex, text);
		invalidate(mode);
	}
	
	sl_bool GridView::addHeaderColSpan(sl_uint32 startIndex, sl_uint32 count, UIUpdateMode mode)
	{
		if (startIndex < m_nLeftColumns && count > m_nLeftColumns - startIndex) {
			count = m_nLeftColumns - startIndex;
		}
		if (startIndex > m_nRightColumns && count > m_nColumns - m_nRightColumns - 1) {
			count = m_nColumns - m_nRightColumns - 1;
		}
		if (startIndex > m_nLeftColumns && startIndex < m_nColumns - m_nRightColumns) {
			if (count > m_nColumns - m_nRightColumns - startIndex) {
				count = m_nColumns - m_nRightColumns - startIndex;
			}
		}
		if (count > 1) {
			if (count > m_nColumns - startIndex) {
				count = m_nColumns - startIndex;
			}
			for (sl_uint32 index = startIndex; index < startIndex + count; index++) {
				if (m_listHeaderSpan.get_NoLock(index)) {
					return sl_false;
				}
			}
			CellRegion spanRegion;
			spanRegion.startColumn = startIndex;
			spanRegion.endColumn = startIndex + count;
			for (sl_uint32 index = startIndex; index < startIndex + count; index++) {
				m_listHeaderSpan.add_NoLock(index, spanRegion);
			}
		}
		invalidate(mode);
		return sl_true;
	}

	void GridView::resetHeaderColSpan(UIUpdateMode mode)
	{
		m_listHeaderSpan.removeAll();
		invalidate(mode);
	}

	void GridView::setTopHeaderSize(sl_ui_len height, UIUpdateMode mode)
	{
		m_heightTopRow = height;
		invalidate(mode);
	}

	void GridView::setBottomHeaderSize(sl_ui_len height, UIUpdateMode mode)
	{
		m_heightBottomRow = height;
		invalidate(mode);
	}


	sl_ui_len GridView::getRowHeight()
	{
		return m_heightRow;
	}
	
	sl_ui_len GridView::getColumnWidth(sl_uint32 colStart, sl_uint32 colEnd) {
		sl_ui_len ret = 0;
		if (colEnd > m_nColumns) {
			return ret;
		}

		for (sl_uint32 i = colStart; i < colEnd; i++) {
			ret += m_listColumnWidth.getValueAt_NoLock(i);
		}
		return ret;
	}

	void GridView::setColumnWidth(sl_uint32 colIndex, sl_ui_len width, UIUpdateMode mode)
	{
		if (colIndex >= m_nColumns) {
			return;
		}
		m_listColumnWidth.setAt_NoLock(colIndex, width);
		invalidate(mode);
	}

	void GridView::setRowHeight(sl_ui_len height, UIUpdateMode mode)
	{
		if (height < 1) {
			return;
		}
		if (m_heightRow == height) {
			return;
		}
		m_heightRow = height;
		setContentHeight((sl_scroll_pos)(height * m_heightRow) + m_heightTopRow + m_heightBottomRow, mode);
	}

	sl_bool GridView::isRowSelected(sl_uint64 rowIndex)
	{
		if (rowIndex >= m_nRows) {
			return sl_false;
		}
		return m_selectedRow == rowIndex;
	}

	sl_uint64 GridView::getSelectedRow()
	{
		sl_uint64 index = m_selectedRow;
		if (index < m_nRows) {
			return index;
		}
		return -1;
	}


	void GridView::setRowSelected(sl_uint64 rowIndex, UIUpdateMode mode)
	{
		if (rowIndex < 0) {
			unselectAll(mode);
			return;
		}
		if (rowIndex >= m_nRows) {
			return;
		}
		
		if (m_selectedRow != rowIndex) {
			m_selectedRow = rowIndex;
			invalidate(mode);
		}
	}

	void GridView::unselectAll(UIUpdateMode mode)
	{
		if (m_selectedRow < 0) {
			return;
		}
		m_selectedRow = -1;
		invalidate(mode);
	}

	sl_int64 GridView::getHoverIndex()
	{
		sl_uint64 index = m_indexHover;
		if (index < m_nRows) {
			return index;
		}
		return -1;
	}

	sl_int64 GridView::getRowIndexAt(sl_int32 y)
	{
		if (y < m_heightTopRow) {
			return -1;
		}
		if (y > getHeight() - m_heightBottomRow) {
			return -1;
		}
		sl_int64 pos = y + (sl_int64)(getScrollY()) - m_heightTopRow;
		if (pos < 0) {
			return -1;
		}
		sl_int64 index = pos / m_heightRow;
		if (index < (sl_int64)m_nRows) {
			return index;
		}
		return -1;
	}

	sl_int32 GridView::getColumnIndexAt(sl_int32 x)
	{
		sl_ui_len leftFrozenWidth = getColumnWidth(0, m_nLeftColumns);
		sl_ui_len rightFrozenWidth = getColumnWidth(m_nColumns - m_nRightColumns, m_nColumns);
		sl_ui_len width = getWidth();

		sl_uint32 start = m_nLeftColumns;
		sl_uint32 end = m_nColumns - m_nRightColumns;
		sl_int64 pos = (x + (sl_int64)(getScrollX())) - leftFrozenWidth;
		sl_ui_len startPosition = 0;
		if (x > 0 && x < leftFrozenWidth) {
			start = 0;
			end = m_nLeftColumns;
			pos = x;
		}

		if (x > width - rightFrozenWidth && x < width) {
			startPosition = width - rightFrozenWidth;
			start = m_nColumns - m_nRightColumns;
			end = m_nColumns;
			pos = x;
		}

		sl_uint32 index = 0;
		for (sl_uint32 i = start; i < end; i++) {
			sl_ui_len columnWidth = m_listColumnWidth.getValueAt_NoLock(i);
			if (startPosition < pos && pos <= startPosition + columnWidth) {
				index = i;
				break;
			}
			startPosition += columnWidth;
		}
		if (index < m_nColumns) {
			return index;
		}
		return -1;
	}


	Ref<Drawable> GridView::getItemBackground()
	{
		return m_backgroundItem;
	}

	void GridView::setItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_backgroundItem = drawable;
		invalidate(mode);
	}

	void GridView::setFixedItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_backgroundFixedItem = drawable;
		invalidate(mode);
	}


	void GridView::setItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setItemBackground(Drawable::createColorDrawable(color), mode);
	}

	Ref<Drawable> GridView::getFixedItemBackground()
	{
		return m_backgroundFixedItem;
	}

	void GridView::setFixedItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setFixedItemBackground(Drawable::createColorDrawable(color), mode);
	}

	Ref<Drawable> GridView::getSelectedItemBackground()
	{
		return m_backgroundSelectedItem;
	}

	void GridView::setSelectedItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_backgroundSelectedItem = drawable;
		invalidate(mode);
	}

	void GridView::setSelectedItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setSelectedItemBackground(Drawable::createColorDrawable(color), mode);
	}

	Ref<Drawable> GridView::getHoverItemBackground()
	{
		return m_backgroundHoverItem;
	}

	void GridView::setHoverItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_backgroundHoverItem = drawable;
		invalidate(mode);
	}

	void GridView::setHoverItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setHoverItemBackground(Drawable::createColorDrawable(color), mode);
	}

	Ref<Drawable> GridView::getFocusedItemBackground()
	{
		return m_backgroundFocusedItem;
	}

	void GridView::setFocusedItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_backgroundFocusedItem = drawable;
		invalidate(mode);
	}

	void GridView::setFocusedItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setFocusedItemBackground(Drawable::createColorDrawable(color), mode);
	}
	
	void GridView::drawHeaderItem(sl_uint64 rowIndex, sl_uint32 colIndex, Canvas* canvas, UIRect& rcItem, sl_bool flagTop)
	{
		Ref<Drawable> background = m_backgroundFixedItem;
		
		if (background.isNotNull()) {
			canvas->draw(rcItem, background);
		}
		canvas->drawRectangle(rcItem, Pen::createSolidPen(1, Color::Black));
		String str = flagTop ? m_listHeaderText.getValueAt_NoLock(colIndex) : m_listHeaderText.getValueAt_NoLock(colIndex);

		DrawCellParam param;
		param.canvas = canvas;
		param.data = str;
		param.row = rowIndex;
		param.col = colIndex;
		param.region = rcItem;
		param.parent = this;
		param.flagFixedCell = sl_true;

		Ref<Cell> cell = this->getCellCallback()(rowIndex, colIndex);
		if (cell.isNotNull()) {
			cell->onDraw(param);
		}
	}
	void GridView::drawGridCellItem(sl_uint64 rowIndex, sl_uint32 colIndex, Canvas* canvas, UIRect& rcItem, sl_bool flagFixedCell)
	{
		Ref<Drawable> background = m_backgroundItem;
		if (flagFixedCell) {
			background = m_backgroundFixedItem;
		}
		if (m_backgroundFocusedItem.isNotNull() && m_focusedColumn == colIndex && m_focusedRow == rowIndex) {
			background = m_backgroundFocusedItem;
		}
		if (m_backgroundSelectedItem.isNotNull() && isRowSelected(rowIndex)) {
			background = m_backgroundSelectedItem;
		} else if (m_backgroundHoverItem.isNotNull() && rowIndex == getHoverIndex()) {
			background = m_backgroundHoverItem;
		} else if (m_backgroundFocusedItem.isNotNull() && isFocused() && rowIndex == m_selectedRow) {
			background = m_backgroundFocusedItem;
		}
		if (background.isNotNull()) {
			canvas->draw(rcItem, background);
		}
		canvas->drawRectangle(rcItem, Pen::createSolidPen(1, Color::Black));

		Variant str = this->getDataCallback()(rowIndex, colIndex);
		DrawCellParam param;
		param.canvas = canvas;
		param.data = str;
		param.row = rowIndex;
		param.col = colIndex;
		param.region = rcItem;
		param.parent = this;
		param.flagFixedCell = flagFixedCell;

		Ref<Cell> cell = this->getCellCallback()(rowIndex, colIndex);
		if (cell.isNotNull()) {
			cell->onDraw(param);
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(GridView, ClickItem, sl_uint64 rowIndex, sl_uint32 colIndex, UIPoint& pos, UIEvent* ev)

	void GridView::dispatchClickItem(sl_uint64 rowIndex, sl_uint32 colIndex, UIPoint& pos, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(ClickItem, rowIndex, colIndex, pos, ev)
		if (ev->isPreventedDefault()) {
			return;
		}
		if (colIndex < m_nLeftColumns || colIndex > m_nColumns - m_nRightColumns - 1) {
			m_selectedRow = rowIndex;
			setRowSelected(rowIndex);
			dispatchChangedSelection(ev);
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(GridView, RightButtonClickItem, sl_uint64 rowIndex, sl_uint32 colIndex, UIPoint& pos, UIEvent* ev)

	void GridView::dispatchRightButtonClickItem(sl_uint64 rowIndex, sl_uint32 colIndex, UIPoint& pos, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(RightButtonClickItem, rowIndex, colIndex, pos, ev)
	}

	SLIB_DEFINE_EVENT_HANDLER(GridView, DoubleClickItem, sl_uint64 rowIndex, sl_uint32 colIndex, UIPoint& pos, UIEvent* ev)

	void GridView::dispatchDoubleClickItem(sl_uint64 rowIndex, sl_uint32 colIndex, UIPoint& pos, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(DoubleClickItem, rowIndex, colIndex, pos, ev)
	}

	SLIB_DEFINE_EVENT_HANDLER(GridView, ChangedSelection, UIEvent* ev)

	void GridView::dispatchChangedSelection(UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(ChangedSelection, ev)
	}
	
	Ref<GridView::Cell> GridView::getCell(sl_uint64 row, sl_uint32 col)
	{
		return sl_null;
	}

	Variant GridView::getCellData(sl_uint64 row, sl_uint32 col)
	{
		return sl_null;
	}

	void GridView::drawGridHeader(Canvas* canvas, sl_bool flagTop)
	{
		sl_int32 posY = (sl_int32)(getScrollY());
		sl_int32 posX = (sl_int32)(getScrollX());

		CellRegion region = getVisibleCellRegion();
		if (m_listHeaderSpan.get_NoLock(region.startColumn)) {
			CellRegion headerSpanRegion = m_listHeaderSpan.getValue_NoLock(region.startColumn);
			region.startColumn = headerSpanRegion.startColumn;
		}

		for (sl_uint32 j = region.startColumn; j <= region.endColumn; j++) {
			CellRegion headerSpanRegion = m_listHeaderSpan.getValue_NoLock(j);
			sl_bool flagExist = m_listHeaderSpan.get_NoLock(j);
			if (!flagExist || headerSpanRegion.startColumn == j) {
				drawHeaderItem(0, j, canvas, getGridHeaderCellRect(j, flagTop), flagTop);
			}
		}

		for (sl_uint32 j = 0; j < m_nLeftColumns; j++) {
			CellRegion headerSpanRegion = m_listHeaderSpan.getValue_NoLock(j);
			sl_bool flagExist = m_listHeaderSpan.get_NoLock(j);
			if (!flagExist || headerSpanRegion.startColumn == j) {
				drawHeaderItem(0, j, canvas, getGridHeaderCellRect(j, flagTop), flagTop);
			}
		}

		for (sl_uint32 j = m_nColumns - m_nRightColumns; j < m_nColumns; j++) {
			CellRegion region = m_listHeaderSpan.getValue_NoLock(j);
			sl_bool flagExist = m_listHeaderSpan.get_NoLock(j);
			if (!flagExist || region.startColumn == j) {
				drawHeaderItem(0, j, canvas, getGridHeaderCellRect(j, flagTop), flagTop);
			}
		}
	}

	void GridView::drawGridFrozenArea(Canvas* canvas, sl_bool flagLeft)
	{
		sl_int32 posY = (sl_int32)(getScrollY());
		sl_int32 posX = (sl_int32)(getScrollX());

		CellRegion region = getVisibleCellRegion();
		sl_uint32 colStart = 0;
		sl_uint32 colEnd = 0;
		if (flagLeft && m_nLeftColumns > 0) {
			colStart = 0;
			colEnd = m_nLeftColumns;
		}
		else if (!flagLeft && m_nRightColumns > 0) {
			colStart = m_nColumns - m_nRightColumns;
			colEnd = m_nColumns;
		}
		for (sl_uint64 i = region.startRow; i <= region.endRow; i++) {
			for (sl_uint32 j = colStart; j < colEnd; j++) {
				drawGridCellItem(i, j, canvas, getGridFrozenCellRect(i, j, flagLeft), sl_true);
			}
		}
	}

	GridView::CellRegion GridView::getVisibleCellRegion()
	{
		CellRegion ret;
		sl_int32 posY = (sl_int32)(getScrollY());
		sl_int32 posX = (sl_int32)(getScrollX());
		ret.startRow = posY / m_heightRow;
		ret.endRow = (posY + getHeight()) / m_heightRow;
		if (ret.endRow >= m_nRows) {
			ret.endRow = m_nRows - 1;
		}

		sl_int64 widthLength = 0;
		ret.startColumn = m_nLeftColumns;
		ret.endColumn = m_nColumns - m_nRightColumns;
		for (sl_uint32 i = m_nLeftColumns; i < m_nColumns - m_nRightColumns; i++) {
			widthLength += m_listColumnWidth.getValueAt_NoLock(i);
			if (widthLength <= posX) {
				ret.startColumn = i;
			}
			else if (widthLength >= posX + getWidth()) {
				ret.endColumn = i;
				break;
			}
		}
		if (ret.endColumn >= m_nColumns - m_nRightColumns) {
			ret.endColumn = m_nColumns - m_nRightColumns - 1;
		}
		return ret;
	}
	
	UIRect GridView::getGridCellRect(sl_uint64 rowIndex, sl_uint32 colIndex)
	{
		UIRect ret;
		sl_int32 posY = (sl_int32)(getScrollY());
		sl_int32 posX = (sl_int32)(getScrollX());
		ret.top = m_heightTopRow + (sl_ui_pos)(rowIndex * m_heightRow - posY);
		ret.bottom = ret.top + m_heightRow;
		ret.left = getColumnWidth(0, colIndex) - posX;
		ret.right = ret.left + getColumnWidth(colIndex, colIndex + 1);

		if (rowIndex == -1) {
			if (m_listHeaderSpan.get_NoLock(colIndex)) {
				CellRegion region = m_listHeaderSpan.getValue_NoLock(colIndex);
				ret.right = ret.left + getColumnWidth(region.startColumn, region.endColumn);
			}
		}
		return ret;
	}

	UIRect GridView::getGridHeaderCellRect(sl_uint32 colIndex, sl_bool flagTop)
	{
		UIRect ret; 
		ret = getGridCellRect(-1, colIndex);
		if (colIndex < m_nLeftColumns) {
			ret.left = getColumnWidth(0, colIndex);
			ret.right = getColumnWidth(0, colIndex + 1);
			if (m_listHeaderSpan.get_NoLock(colIndex)) {
				CellRegion region = m_listHeaderSpan.getValue_NoLock(colIndex);
				ret.right = getColumnWidth(0, region.endColumn);
			}
		}
		if (colIndex > m_nColumns - m_nRightColumns - 1) {
			ret.left = getWidth() - getColumnWidth(colIndex, m_nColumns);
			ret.right = getWidth() - getColumnWidth(colIndex + 1, m_nColumns);
			if (m_listHeaderSpan.get_NoLock(colIndex)) {
				CellRegion region = m_listHeaderSpan.getValue_NoLock(colIndex);
				ret.right = getWidth() - getColumnWidth(region.endColumn, m_nColumns);
			}
		}
		if (flagTop && m_heightTopRow > 0) {
			ret.top = 0;
			ret.bottom = m_heightTopRow;
		}
		else if (!flagTop && m_heightBottomRow > 0) {
			ret.top = getHeight() - m_heightBottomRow ;
			ret.bottom = getHeight() - 1;
		}
		return ret;
	}
	
	UIRect GridView::getGridFrozenCellRect(sl_uint64 rowIndex, sl_uint32 colIndex, sl_bool flagLeft)
	{
		UIRect ret;
		ret = getGridCellRect(rowIndex, colIndex);
		if (flagLeft) {
			ret.left = getColumnWidth(0, colIndex);
			ret.right = getColumnWidth(0, colIndex + 1);
		}
		else if (!flagLeft) {
			ret.left = getWidth() - getColumnWidth(colIndex, m_nColumns);
			ret.right = getWidth() - getColumnWidth(colIndex + 1, m_nColumns);
		}
		return ret;
	}

	void GridView::onDraw(Canvas* canvas)
	{
		if (m_nRows <= 0 || m_nColumns == 0) {
			return;
		}

		sl_int32 widthLeftHeader = getColumnWidth(0, m_nLeftColumns);
		sl_int32 widthRightHeader = getColumnWidth(m_nColumns - m_nRightColumns, m_nColumns);

		{
			CanvasStateScope scope(canvas);
			Rectanglei rt(widthLeftHeader, m_heightTopRow, getWidth() - widthRightHeader + 1, getHeight() - m_heightBottomRow + 1);
			canvas->clipToRectangle(rt);
			CellRegion region = getVisibleCellRegion();
			for (sl_uint64 i = region.startRow; i <= region.endRow; i++) {
				for (sl_uint32 j = region.startColumn; j <= region.endColumn; j++) {
					drawGridCellItem(i, j, canvas, getGridCellRect(i, j));
				}
			}
		}

		{
			CanvasStateScope scope(canvas);
			Rectanglei rt(0, m_heightTopRow, getWidth(), getHeight() - m_heightBottomRow + 1);
			canvas->clipToRectangle(rt);
			drawGridFrozenArea(canvas, sl_true);
			drawGridFrozenArea(canvas, sl_false);
		}
		{
			CanvasStateScope scope(canvas);
			Rectanglei rt(0, 0, getWidth(), getHeight());
			canvas->clipToRectangle(rt);
			if (m_heightTopRow) {
				drawGridHeader(canvas, sl_true);
			}
			if (m_heightBottomRow) {
				drawGridHeader(canvas, sl_false);
			}
		}
	}

	UIPoint GridView::getPositionInGridCell(const UIPoint& position, sl_uint64 rowIndex, sl_uint32 colIndex)
	{
		UIPoint pt = position;
		if (rowIndex >= 0 && colIndex >= 0) {
			sl_int64 posY = (sl_int64)(getScrollY()) + pt.y - m_heightTopRow;
			pt.y = (sl_ui_pos)(posY - rowIndex * m_heightRow);
			if (colIndex > m_nLeftColumns - 1 && colIndex < m_nColumns - m_nRightColumns) {
				sl_int64 posX = (sl_int64)(getScrollX()) + pt.x - getColumnWidth(0, m_nLeftColumns);
				pt.x = (sl_ui_pos)(posX - getColumnWidth(m_nLeftColumns, colIndex));
			}
			else if (colIndex < m_nLeftColumns) {
				pt.x = pt.x - getColumnWidth(0, colIndex);
			}
			else if (colIndex >= m_nColumns - m_nRightColumns) {
				sl_ui_len start = getWidth() - getColumnWidth(m_nColumns - m_nRightColumns, m_nColumns);
				pt.x = pt.x - start - getColumnWidth(m_nColumns - m_nRightColumns, colIndex);
			}
		}
		return pt;
	}

	void GridView::onClickEvent(UIEvent* ev)
	{
		if (ev->isMouseEvent()) {
			sl_int64 rowIndex = getRowIndexAt((sl_int32)ev->getPoint().y);
			sl_int32 colIndex = getColumnIndexAt((sl_int32)ev->getPoint().x);
			if (rowIndex >= 0 && colIndex >= 0) {
				UIPoint pt = getPositionInGridCell(ev->getPoint(), rowIndex, colIndex);
				dispatchClickItem(rowIndex, colIndex, pt, ev);
			}
		}
	}

	void GridView::onMouseEvent(UIEvent* ev)
	{
		UIAction action = ev->getAction();
		if (action == UIAction::RightButtonDown || action == UIAction::LeftButtonDoubleClick || action == UIAction::MouseMove || action == UIAction::MouseEnter) {
			sl_int64 rowIndex = getRowIndexAt((sl_int32)ev->getPoint().y);
			sl_int32 colIndex = getColumnIndexAt((sl_int32)ev->getPoint().x);
			if (rowIndex >= 0 && colIndex >= 0) {
				if ((sl_uint32)colIndex < m_nLeftColumns || (sl_uint32)colIndex > m_nColumns - m_nRightColumns - 1) {
					UIPoint pt = getPositionInGridCell(ev->getPoint(), rowIndex, colIndex);
					if (action == UIAction::RightButtonDown) {
						dispatchRightButtonClickItem(rowIndex, colIndex, pt, ev);
					}
					else if (action == UIAction::LeftButtonDoubleClick) {
						dispatchDoubleClickItem(rowIndex, colIndex, pt, ev);
					}
					if (m_indexHover != rowIndex) {
						m_indexHover = rowIndex;
						invalidate();
					}
				}
				else {
					m_focusedRow = rowIndex;
					m_focusedColumn = colIndex;
					if (m_indexHover != -1) {
						m_indexHover = -1;
					}
					invalidate();
				}
				
			} else {
				if (m_indexHover != -1 || m_focusedRow != -1 || m_focusedColumn != -1) {
					m_focusedRow = -1;
					m_focusedColumn = -1;
					m_indexHover = -1;
					invalidate();
				}
			}
		} else if (action == UIAction::MouseLeave) {
			if (m_indexHover != -1) {
				m_indexHover = -1;
				invalidate();
			}
		}
	}

	void GridView::onKeyEvent(UIEvent* ev)
	{
		sl_int64 nTotal = m_nRows;
		if (nTotal <= 0) {
			return;
		}

		if (ev->getAction() == UIAction::KeyDown) {
		}
	}


	GridView::TextCell::TextCell()
	{
	}

	GridView::TextCell::~TextCell()
	{
	}

	void GridView::TextCell::onDraw(DrawCellParam& cellParam)
	{
		SimpleTextBoxParam param;
		param.text = cellParam.data.toString();
		if (param.text.isEmpty()) {
			return;
		}
		SimpleTextBox::DrawParam drawParam;
		drawParam.frame = cellParam.region;
		drawParam.textColor = Color::Black;
		SimpleTextBox box;
		param.font = cellParam.parent->getFont();
		param.width = drawParam.frame.getWidth();
		param.ellipsizeMode = EllipsizeMode::None;
		param.align = Alignment::Center;
		box.update(param);
		box.draw(cellParam.canvas, drawParam);
	}

	void GridView::TextCell::onEvent(CellEvent& ev)
	{
	}

}
