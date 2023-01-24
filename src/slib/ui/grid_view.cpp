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

#include "slib/ui/text.h"
#include "slib/graphics/canvas.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(GridView, View)

	GridView::GridView()
	{
		setCanvasScrolling(sl_false);
		setVerticalScrolling(sl_true, UIUpdateMode::Init);
		setHorizontalScrolling(sl_true, UIUpdateMode::Init);
		setContentScrollingByMouse(sl_false);
		setFocusable(sl_true);

		m_rowCount = m_columnCount = 0;
		m_rowHeight = 100;
		m_indexHover = -1;

		m_heightBottomHeader = 0;
		m_heightTopHeader = 0;
		m_fixedLeftColumnCount = 0;
		m_fixedRightColumnCount = 0;

		m_gridViewStringCell = New<GridViewStringCell>();
		this->setCellCallback([this](sl_int64 row, sl_int32 column) {
			return m_gridViewStringCell;
		});
	}

	GridView::~GridView()
	{
	}

	sl_int64 GridView::getRowCount()
	{
		return m_rowCount;
	}

	sl_int32 GridView::getColumnCount()
	{
		return m_columnCount;
	}

	void GridView::setRowCount(sl_int64 rowCount, UIUpdateMode mode)
	{
		if (rowCount < 0) {
			rowCount = 0;
		}
		if (m_rowCount == rowCount) {
			return;
		}
		m_rowCount = rowCount;
		setContentHeight((sl_scroll_pos)(m_rowCount * m_rowHeight) + m_heightTopHeader + m_heightBottomHeader, mode);
	}

	void GridView::setFixedColumnCount(sl_int32 leftCount, sl_int32 rightCount, UIUpdateMode mode)
	{
		if (leftCount > 0 && leftCount < m_columnCount - 1) {
			m_fixedLeftColumnCount = leftCount;
			invalidate(mode);
		}
		if (rightCount > 0 && rightCount < m_columnCount - m_fixedLeftColumnCount) {
			m_fixedRightColumnCount = rightCount;
			invalidate(mode);
		}
	}

	void GridView::setColumnCount(sl_int32 colCount, UIUpdateMode mode)
	{
		if (colCount < 0) {
			colCount = 0;
		}
		if (m_columnCount == colCount) {
			return;
		}
		m_columnCount = colCount;
		m_listColumnWidth.removeAll();
		m_listHeaderText.removeAll();
		m_listHeaderSpan.removeAll();
		for (auto index = 0; index < m_columnCount; index++) {
			m_listColumnWidth.add(77);
			m_listHeaderText.add_NoLock("Header" + String::fromInt(index));
		}

		sl_ui_len width = getColumnWidth(0, colCount);
		setContentWidth(width, mode);
	}

	void GridView::setTopHeaderText(sl_int32 colIndex, const String& text, UIUpdateMode mode)
	{
		if (colIndex >= m_columnCount || colIndex < 0) {
			return;
		}
		m_listHeaderText.setAt_NoLock(colIndex, text);
		invalidate(mode);
	}
	
	sl_bool GridView::addHeaderColSpan(sl_int32 startIndex, sl_int32 count, UIUpdateMode mode)
	{
		if (startIndex < m_fixedLeftColumnCount && count > m_fixedLeftColumnCount - startIndex) {
			count = m_fixedLeftColumnCount - startIndex;
		}
		if (startIndex > m_fixedRightColumnCount && count > m_columnCount - m_fixedRightColumnCount - 1) {
			count = m_columnCount - m_fixedRightColumnCount - 1;
		}
		if (startIndex > m_fixedLeftColumnCount && startIndex < m_columnCount - m_fixedRightColumnCount) {
			if (count > m_columnCount - m_fixedRightColumnCount - startIndex) {
				count = m_columnCount - m_fixedRightColumnCount - startIndex;
			}
		}
		if (count > 1) {
			if (count > m_columnCount - startIndex) {
				count = m_columnCount - startIndex;
			}
			for (sl_int32 index = startIndex; index < startIndex + count; index++) {
				if (m_listHeaderSpan.get_NoLock(index)) {
					return sl_false;
				}
			}
			GridViewCellRegion spanRegion;
			spanRegion.columnStart = startIndex;
			spanRegion.columnEnd = startIndex + count;
			for (sl_int32 index = startIndex; index < startIndex + count; index++) {
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
		m_heightTopHeader = height;
		invalidate(mode);
	}

	void GridView::setBottomHeaderSize(sl_ui_len height, UIUpdateMode mode)
	{
		m_heightBottomHeader = height;
		invalidate(mode);
	}


	sl_ui_len GridView::getRowHeight()
	{
		return m_rowHeight;
	}
	
	sl_ui_len GridView::getColumnWidth(sl_int32 colStart, sl_int32 colEnd) {
		sl_ui_len ret = 0;
		if (colStart < 0 || colEnd < 0 || colEnd > m_columnCount) {
			return ret;
		}

		for (sl_int64 i = colStart; i < colEnd; i++) {
			ret += m_listColumnWidth.getValueAt_NoLock(i);
		}
		return ret;
	}

	void GridView::setColumnWidth(sl_int32 colIndex, sl_ui_len width, UIUpdateMode mode)
	{
		if (colIndex < 0 || colIndex >= m_columnCount) {
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
		if (m_rowHeight == height) {
			return;
		}
		m_rowHeight = height;
		setContentHeight((sl_scroll_pos)(height * m_rowHeight) + m_heightTopHeader + m_heightBottomHeader, mode);
	}

	sl_bool GridView::isRowSelected(sl_int64 rowIndex)
	{
		if (rowIndex < 0) {
			return sl_false;
		}
		if (rowIndex >= m_rowCount) {
			return sl_false;
		}
		return m_selectedRow == rowIndex;
	}

	sl_int64 GridView::getSelectedRow()
	{
		sl_int64 index = m_selectedRow;
		if (index >= 0) {
			if (index < m_rowCount) {
				return index;
			}
		}
		return -1;
	}


	void GridView::setRowSelected(sl_int64 rowIndex, UIUpdateMode mode)
	{
		if (rowIndex < 0) {
			unselectAll(mode);
			return;
		}
		if (rowIndex >= m_rowCount) {
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
		sl_int64 index = m_indexHover;
		if (index >= 0) {
			if (index < m_rowCount) {
				return index;
			}
		}
		return -1;
	}

	sl_int64 GridView::getRowIndexAt(sl_int32 y)
	{
		if (y < m_heightTopHeader) {
			return -1;
		}
		if (y > getHeight() - m_heightBottomHeader) {
			return -1;
		}
		sl_int64 pos = y + (sl_int64)(getScrollY()) - m_heightTopHeader;
		if (pos < 0) {
			return -1;
		}
		sl_int64 index = pos / m_rowHeight;
		if (index < m_rowCount) {
			return index;
		}
		return -1;
	}

	sl_int32 GridView::getColumnIndexAt(sl_int32 x)
	{
		sl_ui_len leftFrozenWidth = getColumnWidth(0, m_fixedLeftColumnCount);
		sl_ui_len rightFrozenWidth = getColumnWidth(m_columnCount - m_fixedRightColumnCount, m_columnCount);
		sl_ui_len width = getWidth();

		sl_int32 start = m_fixedLeftColumnCount;
		sl_int32 end = m_columnCount - m_fixedRightColumnCount;
		sl_int64 pos = (x + (sl_int64)(getScrollX())) - leftFrozenWidth;
		sl_ui_len startPosition = 0;
		if (x > 0 && x < leftFrozenWidth) {
			start = 0;
			end = m_fixedLeftColumnCount;
			pos = x;
		}

		if (x > width - rightFrozenWidth && x < width) {
			startPosition = width - rightFrozenWidth;
			start = m_columnCount - m_fixedRightColumnCount;
			end = m_columnCount;
			pos = x;
		}

		sl_int32 index = 0;
		for (sl_int32 i = start; i < end; i++) {
			sl_ui_len columnWidth = m_listColumnWidth.getValueAt_NoLock(i);
			if (startPosition < pos && pos <= startPosition + columnWidth) {
				index = i;
				break;
			}
			startPosition += columnWidth;
		}
		if (index < m_columnCount) {
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
	
	void GridView::drawHeaderItem(sl_int64 rowIndex, sl_int32 colIndex, Canvas* canvas, UIRect& rcItem, sl_bool flagTop)
	{
		Ref<Drawable> background = m_backgroundFixedItem;
		
		if (background.isNotNull()) {
			canvas->draw(rcItem, background);
		}
		canvas->drawRectangle(rcItem, Pen::createSolidPen(1, Color::Black));
		String str = flagTop ? m_listHeaderText.getValueAt_NoLock(colIndex) : m_listHeaderText.getValueAt_NoLock(colIndex);

		GridViewCellDrawParam param;
		param.canvas = canvas;
		param.data = str;
		param.row = rowIndex;
		param.col = colIndex;
		param.region = rcItem;
		param.parent = this;
		param.flagFixedCell = sl_true;

		Ref<GridViewCell> cell = this->getCellCallback()(rowIndex, colIndex);
		if (cell.isNotNull()) {
			cell->onDrawCell(param);
		}
	}
	void GridView::drawGridCellItem(sl_int64 rowIndex, sl_int32 colIndex, Canvas* canvas, UIRect& rcItem, sl_bool flagFixedCell)
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
		GridViewCellDrawParam param;
		param.canvas = canvas;
		param.data = str;
		param.row = rowIndex;
		param.col = colIndex;
		param.region = rcItem;
		param.parent = this;
		param.flagFixedCell = flagFixedCell;

		Ref<GridViewCell> cell = this->getCellCallback()(rowIndex, colIndex);
		if (cell.isNotNull()) {
			cell->onDrawCell(param);
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(GridView, ClickItem, sl_int64 rowIndex, sl_int32 colIndex, UIPoint& pos, UIEvent* ev)

	void GridView::dispatchClickItem(sl_int64 rowIndex, sl_int32 colIndex, UIPoint& pos, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(ClickItem, rowIndex, colIndex, pos, ev)
		if (ev->isPreventedDefault()) {
			return;
		}
		if (colIndex < m_fixedLeftColumnCount || colIndex > m_columnCount - m_fixedRightColumnCount - 1) {
			m_selectedRow = rowIndex;
			setRowSelected(rowIndex);
			dispatchChangedSelection(ev);
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(GridView, RightButtonClickItem, sl_int64 rowIndex, sl_int32 colIndex, UIPoint& pos, UIEvent* ev)

	void GridView::dispatchRightButtonClickItem(sl_int64 rowIndex, sl_int32 colIndex, UIPoint& pos, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(RightButtonClickItem, rowIndex, colIndex, pos, ev)
	}

	SLIB_DEFINE_EVENT_HANDLER(GridView, DoubleClickItem, sl_int64 rowIndex, sl_int32 colIndex, UIPoint& pos, UIEvent* ev)

	void GridView::dispatchDoubleClickItem(sl_int64 rowIndex, sl_int32 colIndex, UIPoint& pos, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(DoubleClickItem, rowIndex, colIndex, pos, ev)
	}

	SLIB_DEFINE_EVENT_HANDLER(GridView, ChangedSelection, UIEvent* ev)

	void GridView::dispatchChangedSelection(UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(ChangedSelection, ev)
	}
	
	Ref<GridViewCell> GridView::getCell(sl_int64 row, sl_uint32 col)
	{
		return sl_null;
	}

	Variant GridView::getCellData(sl_int64 row, sl_uint32 col)
	{
		return sl_null;
	}

	void GridView::drawGridHeader(Canvas* canvas, sl_bool flagTop)
	{
		sl_int32 posY = (sl_int32)(getScrollY());
		sl_int32 posX = (sl_int32)(getScrollX());

		GridViewCellRegion region = getVisibleGridViewCellRegion();
		if (m_listHeaderSpan.get_NoLock(region.columnStart)) {
			GridViewCellRegion headerSpanRegion = m_listHeaderSpan.getValue_NoLock(region.columnStart);
			region.columnStart = headerSpanRegion.columnStart;
		}

		for (sl_int32 j = region.columnStart; j <= region.columnEnd; j++) {
			GridViewCellRegion headerSpanRegion = m_listHeaderSpan.getValue_NoLock(j);
			sl_bool flagExist = m_listHeaderSpan.get_NoLock(j);
			if (!flagExist || headerSpanRegion.columnStart == j) {
				drawHeaderItem(0, j, canvas, getGridHeaderCellRect(j, flagTop), flagTop);
			}
		}

		for (sl_int32 j = 0; j < m_fixedLeftColumnCount; j++) {
			GridViewCellRegion headerSpanRegion = m_listHeaderSpan.getValue_NoLock(j);
			sl_bool flagExist = m_listHeaderSpan.get_NoLock(j);
			if (!flagExist || headerSpanRegion.columnStart == j) {
				drawHeaderItem(0, j, canvas, getGridHeaderCellRect(j, flagTop), flagTop);
			}
		}

		for (sl_int32 j = m_columnCount - m_fixedRightColumnCount; j < m_columnCount; j++) {
			GridViewCellRegion region = m_listHeaderSpan.getValue_NoLock(j);
			sl_bool flagExist = m_listHeaderSpan.get_NoLock(j);
			if (!flagExist || region.columnStart == j) {
				drawHeaderItem(0, j, canvas, getGridHeaderCellRect(j, flagTop), flagTop);
			}
		}
	}

	void GridView::drawGridFrozenArea(Canvas* canvas, sl_bool flagLeft)
	{
		sl_int32 posY = (sl_int32)(getScrollY());
		sl_int32 posX = (sl_int32)(getScrollX());

		GridViewCellRegion region = getVisibleGridViewCellRegion();
		sl_int32 colStart = 0;
		sl_int32 colEnd = 0;
		if (flagLeft && m_fixedLeftColumnCount > 0) {
			colStart = 0;
			colEnd = m_fixedLeftColumnCount;
		}
		else if (!flagLeft && m_fixedRightColumnCount > 0) {
			colStart = m_columnCount - m_fixedRightColumnCount;
			colEnd = m_columnCount;
		}
		for (sl_int64 i = region.rowStart; i <= region.rowEnd; i++) {
			for (sl_int32 j = colStart; j < colEnd; j++) {
				drawGridCellItem(i, j, canvas, getGridFrozenCellRect(i, j, flagLeft), sl_true);
			}
		}
	}

	GridViewCellRegion GridView::getVisibleGridViewCellRegion()
	{
		GridViewCellRegion ret;
		sl_int32 posY = (sl_int32)(getScrollY());
		sl_int32 posX = (sl_int32)(getScrollX());
		ret.rowStart = posY / m_rowHeight;
		ret.rowEnd = (posY + getHeight()) / m_rowHeight;
		if (ret.rowEnd >= m_rowCount) {
			ret.rowEnd = m_rowCount - 1;
		}

		sl_int64 widthLength = 0;
		ret.columnStart = m_fixedLeftColumnCount;
		ret.columnEnd = m_columnCount - m_fixedRightColumnCount;
		for (sl_int32 i = m_fixedLeftColumnCount; i < m_columnCount - m_fixedRightColumnCount; i++) {
			widthLength += m_listColumnWidth.getValueAt_NoLock(i);
			if (widthLength <= posX) {
				ret.columnStart = i;
			}
			else if (widthLength >= posX + getWidth()) {
				ret.columnEnd = i;
				break;
			}
		}
		if (ret.columnEnd >= m_columnCount - m_fixedRightColumnCount) {
			ret.columnEnd = m_columnCount - m_fixedRightColumnCount - 1;
		}
		return ret;
	}
	
	UIRect GridView::getGridCellRect(sl_int64 rowIndex, sl_int32 colIndex)
	{
		UIRect ret;
		sl_int32 posY = (sl_int32)(getScrollY());
		sl_int32 posX = (sl_int32)(getScrollX());
		ret.top = m_heightTopHeader + (sl_ui_pos)(rowIndex * m_rowHeight - posY);
		ret.bottom = ret.top + m_rowHeight;
		ret.left = getColumnWidth(0, colIndex) - posX;
		ret.right = ret.left + getColumnWidth(colIndex, colIndex + 1);

		if (rowIndex == -1) {
			if (m_listHeaderSpan.get_NoLock(colIndex)) {
				GridViewCellRegion region = m_listHeaderSpan.getValue_NoLock(colIndex);
				ret.right = ret.left + getColumnWidth(region.columnStart, region.columnEnd);
			}
		}
		return ret;
	}

	UIRect GridView::getGridHeaderCellRect(sl_int32 colIndex, sl_bool flagTop)
	{
		UIRect ret; 
		ret = getGridCellRect(-1, colIndex);
		if (colIndex < m_fixedLeftColumnCount) {
			ret.left = getColumnWidth(0, colIndex);
			ret.right = getColumnWidth(0, colIndex + 1);
			if (m_listHeaderSpan.get_NoLock(colIndex)) {
				GridViewCellRegion region = m_listHeaderSpan.getValue_NoLock(colIndex);
				ret.right = getColumnWidth(0, region.columnEnd);
			}
		}
		if (colIndex > m_columnCount - m_fixedRightColumnCount - 1) {
			ret.left = getWidth() - getColumnWidth(colIndex, m_columnCount);
			ret.right = getWidth() - getColumnWidth(colIndex + 1, m_columnCount);
			if (m_listHeaderSpan.get_NoLock(colIndex)) {
				GridViewCellRegion region = m_listHeaderSpan.getValue_NoLock(colIndex);
				ret.right = getWidth() - getColumnWidth(region.columnEnd, m_columnCount);
			}
		}
		if (flagTop && m_heightTopHeader > 0) {
			ret.top = 0;
			ret.bottom = m_heightTopHeader;
		}
		else if (!flagTop && m_heightBottomHeader > 0) {
			ret.top = getHeight() - m_heightBottomHeader ;
			ret.bottom = getHeight() - 1;
		}
		return ret;
	}
	
	UIRect GridView::getGridFrozenCellRect(sl_int64 rowIndex, sl_int32 colIndex, sl_bool flagLeft)
	{
		UIRect ret;
		ret = getGridCellRect(rowIndex, colIndex);
		if (flagLeft) {
			ret.left = getColumnWidth(0, colIndex);
			ret.right = getColumnWidth(0, colIndex + 1);
		}
		else if (!flagLeft) {
			ret.left = getWidth() - getColumnWidth(colIndex, m_columnCount);
			ret.right = getWidth() - getColumnWidth(colIndex + 1, m_columnCount);
		}
		return ret;
	}

	void GridView::onDraw(Canvas* canvas)
	{
		if (m_rowCount <= 0 || m_columnCount == 0) {
			return;
		}

		sl_int32 widthLeftHeader = getColumnWidth(0, m_fixedLeftColumnCount);
		sl_int32 widthRightHeader = getColumnWidth(m_columnCount - m_fixedRightColumnCount, m_columnCount);

		{
			CanvasStateScope scope(canvas);
			Rectanglei rt(widthLeftHeader, m_heightTopHeader, getWidth() - widthRightHeader + 1, getHeight() - m_heightBottomHeader + 1);
			canvas->clipToRectangle(rt);
			GridViewCellRegion region = getVisibleGridViewCellRegion();
			for (sl_int64 i = region.rowStart; i <= region.rowEnd; i++) {
				for (sl_int32 j = region.columnStart; j <= region.columnEnd; j++) {
					drawGridCellItem(i, j, canvas, getGridCellRect(i, j));
				}
			}
		}

		{
			CanvasStateScope scope(canvas);
			Rectanglei rt(0, m_heightTopHeader, getWidth(), getHeight() - m_heightBottomHeader + 1);
			canvas->clipToRectangle(rt);
			drawGridFrozenArea(canvas, sl_true);
			drawGridFrozenArea(canvas, sl_false);
		}
		{
			CanvasStateScope scope(canvas);
			Rectanglei rt(0, 0, getWidth(), getHeight());
			canvas->clipToRectangle(rt);
			if (m_heightTopHeader) {
				drawGridHeader(canvas, sl_true);
			}
			if (m_heightBottomHeader) {
				drawGridHeader(canvas, sl_false);
			}
		}
	}

	UIPoint GridView::getPositionInGridCell(const UIPoint& position, sl_int64 rowIndex, sl_int32 colIndex)
	{
		UIPoint pt = position;
		if (rowIndex >= 0 && colIndex >= 0) {
			sl_int64 posY = (sl_int64)(getScrollY()) + pt.y - m_heightTopHeader;
			pt.y = (sl_ui_pos)(posY - rowIndex * m_rowHeight);
			if (colIndex > m_fixedLeftColumnCount - 1 && colIndex < m_columnCount - m_fixedRightColumnCount) {
				sl_int64 posX = (sl_int64)(getScrollX()) + pt.x - getColumnWidth(0, m_fixedLeftColumnCount);
				pt.x = (sl_ui_pos)(posX - getColumnWidth(m_fixedLeftColumnCount, colIndex));
			}
			else if (colIndex < m_fixedLeftColumnCount) {
				pt.x = pt.x - getColumnWidth(0, colIndex);
			}
			else if (colIndex >= m_columnCount - m_fixedRightColumnCount) {
				sl_ui_len start = getWidth() - getColumnWidth(m_columnCount - m_fixedRightColumnCount, m_columnCount);
				pt.x = pt.x - start - getColumnWidth(m_columnCount - m_fixedRightColumnCount, colIndex);
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
			if (rowIndex >= 0) {
				if (colIndex < m_fixedLeftColumnCount || colIndex > m_columnCount - m_fixedRightColumnCount - 1) {
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
		sl_int64 nTotal = m_rowCount;
		if (nTotal <= 0) {
			return;
		}

		if (ev->getAction() == UIAction::KeyDown) {
		}
	}

	void GridViewStringCell::onDrawCell(GridViewCellDrawParam& cellParam)
	{
		SimpleTextBoxParam param;
		param.text = cellParam.data.toString();
		if (param.text.isEmpty()) {
			return;
		}
		SimpleTextBoxDrawParam drawParam;
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

	void GridViewStringCell::onMouseEvent(GridViewCellEvent& ev)
	{
		
	}
}
