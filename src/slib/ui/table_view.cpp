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

#include "slib/ui/text.h"
#include "slib/ui/table_view.h"

#include "slib/graphics/canvas.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(TableView, View)

	TableView::TableView()
	{
		setCanvasScrolling(sl_false);
		setVerticalScrolling(sl_true, UIUpdateMode::Init);
		setHorizontalScrolling(sl_true, UIUpdateMode::Init);
		setContentScrollingByMouse(sl_false);
		setFocusable(sl_true);

		m_rowCount = m_columnCount = 0;
		m_rowHeight = 100;
		m_indexHover = -1;

		m_heightBottomHeader = m_heightTopHeader = 50;
		m_fixedLeftColumnCount = 2;
		m_fixedRightColumnCount = 1;

		m_flagMultipleSelection = sl_false;
		m_gridViewStringCell = New<GridViewStringCell>();
		this->setCellCallback([this](sl_int64 row, sl_int32 column) {
			return m_gridViewStringCell;
		});
	}

	TableView::~TableView()
	{
	}

	sl_uint64 TableView::getRowCount()
	{
		return m_rowCount;
	}

	sl_uint64 TableView::getColumnCount()
	{
		return m_columnCount;
	}

	void TableView::setRowCount(sl_int32 rowCount, UIUpdateMode mode)
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

	void TableView::setColumnCount(sl_int32 colCount, UIUpdateMode mode)
	{
		if (colCount < 0) {
			colCount = 0;
		}
		if (m_columnCount == colCount) {
			return;
		}
		m_columnCount = colCount;
		m_listColumnWidth.removeAll();
		m_listTopHeaderText.removeAll();
		m_listBottomHeaderText.removeAll();
		for (auto index = 0; index < m_columnCount; index++) {
			m_listColumnWidth.add(100 + 10 * index);
			m_listTopHeaderText.add_NoLock("");
			m_listBottomHeaderText.add_NoLock("");
		}

		sl_ui_len width = getColumnWidth(0, colCount);
		setContentWidth(width, mode);
	}

	void TableView::setTopHeaderText(sl_int32 colIndex, const String& text, UIUpdateMode mode)
	{
		if (colIndex >= m_columnCount || colIndex < 0) {
			return;
		}
		m_listTopHeaderText.setAt_NoLock(colIndex, text);
		invalidate(mode);
	}

	void TableView::setBottomHeaderText(sl_int32 colIndex, const String& text, UIUpdateMode mode)
	{
		if (colIndex >= m_columnCount || colIndex < 0) {
			return;
		}
		m_listBottomHeaderText.setAt_NoLock(colIndex, text);
		invalidate(mode);
	}

	void TableView::setTopHeaderSize(sl_ui_len height, UIUpdateMode mode)
	{
		m_heightTopHeader = height;
		invalidate(mode);
	}

	void TableView::setBottomHeaderSize(sl_ui_len height, UIUpdateMode mode)
	{
		m_heightBottomHeader = height;
		invalidate(mode);
	}


	sl_ui_len TableView::getRowHeight()
	{
		return m_rowHeight;
	}
	
	sl_ui_len TableView::getColumnWidth(sl_int64 colStart, sl_int64 colEnd) {
		sl_ui_len ret = 0;
		if (colStart < 0 || colEnd < 0 || colEnd > m_columnCount) {
			return ret;
		}

		for (sl_int64 i = colStart; i < colEnd; i++) {
			ret += m_listColumnWidth.getValueAt_NoLock(i);
		}
		return ret;
	}

	void TableView::setColumnWidth(sl_int32 colIndex, sl_ui_len width, UIUpdateMode mode)
	{
		if (colIndex < 0 || colIndex >= m_columnCount) {
			return;
		}
		m_listColumnWidth.setAt_NoLock(colIndex, width);
		invalidate(mode);
	}

	void TableView::setRowHeight(sl_ui_len height, UIUpdateMode mode)
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

	sl_bool TableView::isMultipleSelection()
	{
		return m_flagMultipleSelection;
	}

	void TableView::setMultipleSelection(sl_bool flag, UIUpdateMode mode)
	{
		if (m_flagMultipleSelection != flag) {
			m_flagMultipleSelection = flag;
			if (flag) {
				m_selectedRow = -1;
			} else {
				m_mapRowSelection.removeAll();
			}
			invalidate(mode);
		}
	}

	sl_bool TableView::isRowSelected(sl_int64 rowIndex)
	{
		if (rowIndex < 0) {
			return sl_false;
		}
		if (rowIndex >= m_rowCount) {
			return sl_false;
		}
		if (m_flagMultipleSelection) {
			return m_mapRowSelection.find(rowIndex);
		} else {
			return m_selectedRow == rowIndex;
		}
	}

	sl_int64 TableView::getSelectedRow()
	{
		if (m_flagMultipleSelection) {
			ObjectLocker lock(&m_mapRowSelection);
			auto node = m_mapRowSelection.getLastNode();
			if (node) {
				return node->key;
			}
		} else {
			sl_int64 index = m_selectedRow;
			if (index >= 0) {
				if (index < m_rowCount) {
					return index;
				}
			}
		}
		return -1;
	}


	void TableView::setRowSelected(sl_int64 rowIndex, UIUpdateMode mode)
	{
		if (rowIndex < 0) {
			unselectAll(mode);
			return;
		}
		if (rowIndex >= m_rowCount) {
			return;
		}
		if (m_flagMultipleSelection) {
			ObjectLocker lock(&m_mapRowSelection);
			m_mapRowSelection.removeAll_NoLock();
			m_mapRowSelection.put_NoLock(rowIndex, sl_true);
			invalidate(mode);
		} else {
			if (m_selectedRow != rowIndex) {
				m_selectedRow = rowIndex;
				invalidate(mode);
			}
		}
	}

	void TableView::unselectAll(UIUpdateMode mode)
	{
		if (m_flagMultipleSelection) {
			ObjectLocker locker1(&m_mapRowSelection);
			if (m_mapRowSelection.isEmpty()) {
				return;
			}
			m_mapRowSelection.removeAll_NoLock();
			
		} else {
			if (m_selectedRow < 0) {
				return;
			}
			m_selectedRow = -1;
		}
		invalidate(mode);
	}

	sl_int64 TableView::getHoverIndex()
	{
		sl_int64 index = m_indexHover;
		if (index >= 0) {
			if (index < m_rowCount) {
				return index;
			}
		}
		return -1;
	}

	sl_int64 TableView::getRowIndexAt(sl_int32 y)
	{
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

	sl_int64 TableView::getColumnIndexAt(sl_int32 x)
	{
		sl_int64 pos = (x + (sl_int64)(getScrollX())) - getColumnWidth(0, m_fixedLeftColumnCount);
		sl_ui_len startPosition = 0;
		sl_int32 index = -1;
		for (sl_int32 i = m_fixedLeftColumnCount; i < m_columnCount - m_fixedRightColumnCount; i++) {
			sl_ui_len width = m_listColumnWidth.getValueAt_NoLock(i);
			if (startPosition >= pos && pos <= startPosition + width) {
				break;
			}
			startPosition += width;
			index++;
		}
		if (index < m_columnCount) {
			return index;
		}
		return -1;
	}


	Ref<Drawable> TableView::getItemBackground()
	{
		return m_backgroundItem;
	}

	void TableView::setItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_backgroundItem = drawable;
		invalidate(mode);
	}

	void TableView::setFixedItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_backgroundFixedItem = drawable;
		invalidate(mode);
	}


	void TableView::setItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setItemBackground(Drawable::createColorDrawable(color), mode);
	}

	void TableView::setFixedItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setFixedItemBackground(Drawable::createColorDrawable(color), mode);
	}

	Ref<Drawable> TableView::getSelectedItemBackground()
	{
		return m_backgroundSelectedItem;
	}

	void TableView::setSelectedItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_backgroundSelectedItem = drawable;
		invalidate(mode);
	}

	void TableView::setSelectedItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setSelectedItemBackground(Drawable::createColorDrawable(color), mode);
	}

	Ref<Drawable> TableView::getHoverItemBackground()
	{
		return m_backgroundHoverItem;
	}

	void TableView::setHoverItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_backgroundHoverItem = drawable;
		invalidate(mode);
	}

	void TableView::setHoverItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setHoverItemBackground(Drawable::createColorDrawable(color), mode);
	}

	Ref<Drawable> TableView::getFocusedItemBackground()
	{
		return m_backgroundFocusedItem;
	}

	void TableView::setFocusedItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_backgroundFocusedItem = drawable;
		invalidate(mode);
	}

	void TableView::setFocusedItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setFocusedItemBackground(Drawable::createColorDrawable(color), mode);
	}

	SLIB_DEFINE_EVENT_HANDLER(TableView, DrawItem, sl_int64 rowIndex, sl_int64 colIndex, Canvas* canvas, UIRect& rcItem, sl_bool flagFixedCell)

	void TableView::dispatchDrawItem(sl_int64 rowIndex, sl_int64 colIndex, Canvas* canvas, UIRect& rcItem, sl_bool flagFixedCell)
	{
		Ref<Drawable> background = m_backgroundItem;
		if (m_backgroundSelectedItem.isNotNull() && isRowSelected(rowIndex)) {
			background = m_backgroundSelectedItem;
		} else if (m_backgroundHoverItem.isNotNull() && rowIndex == getHoverIndex()) {
			background = m_backgroundHoverItem;
		} else if (m_backgroundFocusedItem.isNotNull() && isFocused() && rowIndex == m_selectedRow) {
			background = m_backgroundFocusedItem;
		}
		if (flagFixedCell) {
			background = m_backgroundFixedItem;
		}
		if (background.isNotNull()) {
			canvas->draw(rcItem, background);
		}
		canvas->drawRectangle(rcItem, Pen::createSolidPen(1, Color::Black));
		SLIB_INVOKE_EVENT_HANDLER(DrawItem, rowIndex, colIndex, canvas, rcItem, flagFixedCell)

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

	SLIB_DEFINE_EVENT_HANDLER(TableView, ClickItem, sl_int64 rowIndex, sl_int64 colIndex, UIPoint& pos, UIEvent* ev)

	void TableView::dispatchClickItem(sl_int64 rowIndex, sl_int64 colIndex, UIPoint& pos, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(ClickItem, rowIndex, colIndex, pos, ev)
		if (ev->isPreventedDefault()) {
			return;
		}
		m_selectedRow = rowIndex;
		if (ev->isShiftKey()) {
			if (m_lastSelectedRow >= 0) {
				if (ev->isControlKey() || ev->isCommandKey()) {
					//selectRange(m_lastSelectedRow, rowIndex);
				} else {
					//setSelectedRange(m_lastSelectedRow, rowIndex);
				}
			} else {
				setRowSelected(rowIndex);
			}
			dispatchChangedSelection(ev);
		} else {
			if (ev->isControlKey() || ev->isCommandKey()) {
				//toggleItemSelection(rowIndex);
			} else {
				setRowSelected(rowIndex);
			}
			dispatchChangedSelection(ev);
			m_lastSelectedRow = rowIndex;
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(TableView, RightButtonClickItem, sl_int64 rowIndex, sl_int64 colIndex, UIPoint& pos, UIEvent* ev)

	void TableView::dispatchRightButtonClickItem(sl_int64 rowIndex, sl_int64 colIndex, UIPoint& pos, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(RightButtonClickItem, rowIndex, colIndex, pos, ev)
	}

	SLIB_DEFINE_EVENT_HANDLER(TableView, DoubleClickItem, sl_int64 rowIndex, sl_int64 colIndex, UIPoint& pos, UIEvent* ev)

	void TableView::dispatchDoubleClickItem(sl_int64 rowIndex, sl_int64 colIndex, UIPoint& pos, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(DoubleClickItem, rowIndex, colIndex, pos, ev)
	}

	SLIB_DEFINE_EVENT_HANDLER(TableView, ChangedSelection, UIEvent* ev)

	void TableView::dispatchChangedSelection(UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(ChangedSelection, ev)
	}
	
	Ref<GridViewCell> TableView::getCell(sl_uint64 row, sl_uint32 col)
	{
		return sl_null;
	}

	Variant TableView::getCellData(sl_uint64 row, sl_uint32 col)
	{
		return sl_null;
	}

	void TableView::drawGridVerticalHeader(Canvas* canvas, sl_bool flagTop)
	{
		sl_int32 posY = (sl_int32)(getScrollY());
		sl_int32 posX = (sl_int32)(getScrollX());

		sl_int32 rowStart = 0, rowEnd = 0, colStart = 0, colEnd = 0;
		getViewportGridIndex(rowStart, rowEnd, colStart, colEnd);

		for (sl_reg j = colStart; j <= colEnd; j++) {
			dispatchDrawItem(0, j, canvas, getGridHeaderCellPosition(j, flagTop), sl_true);
		}
	}

	void TableView::drawGridHorizontalHeader(Canvas* canvas, sl_bool flagLeft)
	{
		sl_int32 posY = (sl_int32)(getScrollY());
		sl_int32 posX = (sl_int32)(getScrollX());

		sl_int32 rowStart = 0, rowEnd = 0, colStart = 0, colEnd = 0;
		getViewportGridIndex(rowStart, rowEnd, colStart, colEnd);

		UIRect rcItem;
		if (flagLeft && m_fixedLeftColumnCount > 0) {
			colStart = 0;
			colEnd = m_fixedLeftColumnCount;
		}
		else if (!flagLeft && m_fixedRightColumnCount > 0) {
			colStart = m_columnCount - m_fixedRightColumnCount;
			colEnd = m_columnCount;
		}
		for (sl_reg j = colStart; j < colEnd; j++) {
			for (sl_reg i = rowStart; i <= rowEnd; i++) {
				rcItem.bottom = rcItem.top + m_rowHeight;
				canvas->draw(rcItem, Drawable::createColorDrawable(Color::Gray));
				canvas->drawLine(Pointi(rcItem.left, rcItem.top), Pointi(rcItem.right, rcItem.top), Pen::createSolidPen(1, Color::Black));
				drawGridItemText(canvas, "Header" + String::fromInt32(i), rcItem);
				rcItem.top = rcItem.bottom;
			}
		}
	}

	void TableView::getViewportGridIndex(sl_int32& rowStart, sl_int32& rowEnd, sl_int32& columnStart, sl_int32& columnEnd)
	{
		sl_int32 posY = (sl_int32)(getScrollY());
		sl_int32 posX = (sl_int32)(getScrollX());
		rowStart = posY / m_rowHeight;
		rowEnd = (posY + getHeight()) / m_rowHeight;
		if (rowEnd >= m_rowCount) {
			rowEnd = m_rowCount - 1;
		}

		sl_int64 widthLength = 0;
		columnStart = m_fixedLeftColumnCount;
		columnEnd = m_columnCount - m_fixedRightColumnCount;
		for (sl_int32 i = m_fixedLeftColumnCount; i < m_columnCount - m_fixedRightColumnCount; i++) {
			widthLength += m_listColumnWidth.getValueAt_NoLock(i);
			if (widthLength <= posX) {
				columnStart = i;
			}
			else if (widthLength >= posX + getWidth()) {
				columnEnd = i;
				break;
			}
		}
		if (columnEnd >= m_columnCount - m_fixedRightColumnCount) {
			columnEnd = m_columnCount - m_fixedRightColumnCount - 1;
		}
	}
	
	UIRect TableView::getGridCellPosition(sl_int64 rowIndex, sl_int32 colIndex)
	{
		UIRect ret;
		sl_int32 posY = (sl_int32)(getScrollY());
		sl_int32 posX = (sl_int32)(getScrollX());
		ret.top = m_heightTopHeader + (sl_ui_pos)(rowIndex * m_rowHeight - posY);
		ret.bottom = ret.top + m_rowHeight;
		ret.left = getColumnWidth(0, colIndex) - posX;
		ret.right = ret.left + getColumnWidth(colIndex, colIndex + 1);
		return ret;
	}

	UIRect TableView::getGridHeaderCellPosition(sl_int32 colIndex, sl_bool flagTop)
	{
		UIRect ret; 
		ret = getGridCellPosition(0, colIndex);
		if (flagTop && m_heightTopHeader > 0) {
			ret.top = 0;
			ret.bottom = m_heightTopHeader;
		}
		else if (!flagTop && m_heightBottomHeader > 0) {
			ret.top = getHeight() - m_heightBottomHeader + 1;
			ret.bottom = getHeight() - 1;
		}
		return ret;
	}
	
	void TableView::onDraw(Canvas* canvas)
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
			sl_int32 rowStart = 0, rowEnd = 0, colStart = 0, colEnd = 0;
			getViewportGridIndex(rowStart, rowEnd, colStart, colEnd);
			for (sl_reg i = rowStart; i <= rowEnd; i++) {
				for (sl_reg j = colStart; j <= colEnd; j++) {
					dispatchDrawItem(rowStart + i, j, canvas, getGridCellPosition(i, j));
				}
			}
		}
		{
			CanvasStateScope scope(canvas);
			Rectanglei rt(widthLeftHeader, 0, getWidth() - widthRightHeader + 1, getHeight());
			canvas->clipToRectangle(rt);
			if (m_heightTopHeader) {
				drawGridVerticalHeader(canvas, sl_true);
			}
			if (m_heightBottomHeader) {
				drawGridVerticalHeader(canvas, sl_false);
			}
		}

		{
			CanvasStateScope scope(canvas);
			Rectanglei rt(0, m_heightTopHeader, getWidth(), getHeight() - m_heightBottomHeader + 1);
			canvas->clipToRectangle(rt);
			drawGridHorizontalHeader(canvas, sl_true);
			drawGridHorizontalHeader(canvas, sl_false);
		}
	}

	void TableView::onClickEvent(UIEvent* ev)
	{
		/*if (ev->isMouseEvent()) {
			sl_int64 rowIndex = getRowIndexAt((sl_int32)ev->getPoint().y);
			sl_int64 colIndex = getColumnIndexAt((sl_int32)ev->getPoint().x);
			if (rowIndex >= 0 && colIndex >= 0) {
				UIPoint pt = ev->getPoint();
				sl_int64 posY = (sl_int64)(getScrollY()) + pt.y - m_heightTopHeader;
				pt.y = (sl_ui_pos)(posY - rowIndex * m_rowHeight);
				sl_int64 posX = (sl_int64)(getScrollX()) + pt.x - m_widthLeftHeader;
				pt.x = (sl_ui_pos)(posX - getColumnWidth(0, colIndex));
				dispatchClickItem(rowIndex, colIndex, pt, ev);
			}
		}*/
	}

	void TableView::onMouseEvent(UIEvent* ev)
	{
		UIAction action = ev->getAction();
		if (action == UIAction::RightButtonDown || action == UIAction::LeftButtonDoubleClick || action == UIAction::MouseMove || action == UIAction::MouseEnter) {
			sl_int64 rowIndex = getRowIndexAt((sl_int32)ev->getPoint().y);
			sl_int64 colIndex = getColumnIndexAt((sl_int32)ev->getPoint().x);
			if (rowIndex >= 0) {
				UIPoint pt = ev->getPoint();
				sl_int64 pos = (sl_int64)(getScrollY()) + pt.y;
				pt.y = (sl_ui_pos)(pos - rowIndex * m_rowHeight);
				if (action == UIAction::RightButtonDown) {
					dispatchRightButtonClickItem(rowIndex, colIndex, pt, ev);
				} else if (action == UIAction::LeftButtonDoubleClick) {
					dispatchDoubleClickItem(rowIndex,colIndex, pt, ev);
				}
				if (m_indexHover != rowIndex) {
					m_indexHover = rowIndex;
					invalidate();
				}
			} else {
				if (m_indexHover != -1) {
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

	void TableView::onKeyEvent(UIEvent* ev)
	{
		sl_int64 nTotal = m_rowCount;
		if (nTotal <= 0) {
			return;
		}

		if (ev->getAction() == UIAction::KeyDown) {

			Keycode key = ev->getKeycode();

			if (key == Keycode::Space || key == Keycode::Enter) {

				/*sl_int64 rowIndex = m_indexFocused;
				if (rowIndex >= 0) {
					UIPoint pt(0, 0);
					dispatchClickItem(rowIndex, pt, ev);
				}*/

			} else if (key == Keycode::Up || key == Keycode::Down || key == Keycode::Home || key == Keycode::End) {

				//sl_int64 index = m_indexFocused;

				//if (key == Keycode::Up) {
				//	if (index > 0) {
				//		index--;
				//	} else if (index < 0) {
				//		index = nTotal - 1;
				//	} else {
				//		return;
				//	}
				//} else if (key == Keycode::Down) {
				//	if (index >= 0) {
				//		if (index < nTotal - 1) {
				//			index++;
				//		} else {
				//			return;
				//		}
				//	} else {
				//		index = 0;
				//	}
				//} else if (key == Keycode::Home) {
				//	index = 0;
				//} else {
				//	index = nTotal - 1;
				//}

				//m_indexFocused = index;

				//// check focused item is visible
				//{
				//	sl_int64 sy = (sl_int64)(getScrollY());
				//	sl_ui_len height = m_heightItem;
				//	sl_int64 y1 = index * height;
				//	sl_int64 y2 = y1 + height;
				//	if (y1 < sy || y2 > sy + getHeight()) {
				//		if (key == Keycode::Up || key == Keycode::Home) {
				//			scrollTo(0, (sl_scroll_pos)y1, UIUpdateMode::None);
				//		} else {
				//			scrollTo(0, (sl_scroll_pos)(y2 - getHeight()), UIUpdateMode::None);
				//		}
				//	}
				//}

				//if (ev->isShiftKey()) {
				//	UIPoint pt(0, 0);
				//	dispatchClickItem(index, pt, ev);
				//} else {
				//	if (ev->isControlKey() || ev->isCommandKey()) {
				//		invalidate();
				//	} else {
				//		UIPoint pt(0, 0);
				//		dispatchClickItem(index, pt, ev);
				//	}
				//}

				ev->preventDefault();

			} else if (key == Keycode::Escape) {

				/*if (getSelectedIndex() < 0) {
					m_indexFocused = -1;
				}*/
				invalidate();

			}
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
