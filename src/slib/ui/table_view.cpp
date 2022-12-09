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
		setFocusable(sl_true);

		m_rowCount = m_columnCount = 0;
		m_rowHeight = 100;
		m_columnWidth.removeAll();
		m_indexHover = -1;

		m_heightBottomHeader = m_heightTopHeader = 50;
		m_widthLeftHeader = m_widthRightHeader = 100;

		m_flagMultipleSelection = sl_false;
		/*m_indexSelected = -1;
		m_indexFocused = -1;
		m_indexLastSelected = -1;*/
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

	void TableView::setRowCount(sl_int64 rowCount, UIUpdateMode mode)
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

	void TableView::setColumnCount(sl_int64 colCount, UIUpdateMode mode)
	{
		if (colCount < 0) {
			colCount = 0;
		}
		if (m_columnCount == colCount) {
			return;
		}
		m_columnCount = colCount;
		for (auto index = 0; index < m_columnCount; index++) {
			m_columnWidth.add(100 + index * 50);
		}

		sl_ui_len width = getColumnWidth(0, colCount) + m_widthLeftHeader + m_widthRightHeader;
		setContentWidth(width, mode);
	}
	sl_ui_len TableView::getRowHeight()
	{
		return m_rowHeight;
	}
	
	sl_ui_len TableView::getColumnWidth(sl_int64 colStart, sl_int64 colEnd) {
		sl_ui_len ret = 0;
		if (colStart < 0 || colEnd > m_columnCount) {
			return ret;
		}

		for (sl_int64 i = colStart; i < colEnd; i++) {
			ret += m_columnWidth.getValueAt_NoLock(i);
		}
		return ret;
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
				m_selectedColumn = -1;
				m_selectedRow = -1;
			} else {
				m_mapColumnSelection.removeAll();
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

	sl_bool TableView::isColumnSelected(sl_int64 colIndex)
	{
		if (colIndex < 0) {
			return sl_false;
		}
		if (colIndex >= m_columnCount) {
			return sl_false;
		}
		if (m_flagMultipleSelection) {
			return m_mapColumnSelection.find(colIndex);
		}
		else {
			return m_selectedColumn == colIndex;
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

	sl_int64 TableView::getSelectedColumn()
	{
		if (m_flagMultipleSelection) {
			ObjectLocker lock(&m_mapColumnSelection);
			auto node = m_mapColumnSelection.getLastNode();
			if (node) {
				return node->key;
			}
		}
		else {
			sl_int64 index = m_selectedColumn;
			if (index >= 0) {
				if (index < m_columnCount) {
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

	void TableView::setColumnSelected(sl_int64 colIndex, UIUpdateMode mode)
	{
		if (colIndex < 0) {
			unselectAll(mode);
			return;
		}
		if (colIndex >= m_columnCount) {
			return;
		}
		if (m_flagMultipleSelection) {
			ObjectLocker lock(&m_mapColumnSelection);
			m_mapColumnSelection.removeAll_NoLock();
			m_mapColumnSelection.put_NoLock(colIndex, sl_true);
			invalidate(mode);
		}
		else {
			if (m_selectedColumn != colIndex) {
				m_selectedColumn = colIndex;
				invalidate(mode);
			}
		}
	}

	/*List<sl_uint64> TableView::getSelectedIndices()
	{
		if (m_flagMultipleSelection) {
			return m_mapSelection.getAllKeys();
		} else {
			sl_int64 index = m_indexSelected;
			if (index >= 0) {
				if (index < m_rowCount) {
					return List<sl_uint64>::createFromElement(index);
				}
			}
			return sl_null;
		}
	}

	void TableView::setSelectedIndices(const ListParam<sl_uint64>& _indices, UIUpdateMode mode)
	{
		ListLocker<sl_uint64> indices(_indices);
		if (!(indices.count)) {
			unselectAll(mode);
			return;
		}
		sl_uint64 nTotal = m_rowCount;
		if (!nTotal) {
			return;
		}
		if (m_flagMultipleSelection) {
			ObjectLocker locker(&m_mapSelection);
			m_mapSelection.removeAll_NoLock();
			for (sl_size i = 0; i < indices.count; i++) {
				if (indices[i] < nTotal) {
					m_mapSelection.put_NoLock(indices[i], sl_true);
				}
			}
			invalidate(mode);
		} else {
			sl_int64 index = indices[indices.count - 1];
			if ((sl_uint64)index < nTotal) {
				if (m_indexSelected != index) {
					m_indexSelected = index;
					invalidate(mode);
				}
			}
		}
	}*/

	

	

	void TableView::unselectAll(UIUpdateMode mode)
	{
		if (m_flagMultipleSelection) {
			ObjectLocker locker(&m_mapColumnSelection);
			ObjectLocker locker1(&m_mapRowSelection);
			if (m_mapColumnSelection.isEmpty() && m_mapRowSelection.isEmpty()) {
				return;
			}
			m_mapColumnSelection.removeAll_NoLock();
			m_mapRowSelection.removeAll_NoLock();
			
		} else {
			if (m_selectedColumn < 0 && m_selectedRow < 0) {
				return;
			}
			m_selectedColumn = -1;
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

	sl_int64 TableView::getRowIndexAt(const UIPoint& pt)
	{
		sl_int64 pos = pt.y + (sl_int64)(getScrollY()) - m_heightTopHeader;
		if (pos < 0) {
			return -1;
		}
		sl_int64 index = pos / m_rowHeight;
		if (index < m_rowCount) {
			return index;
		}
		return -1;
	}
	sl_int64 TableView::getColumnIndexAt(const UIPoint& pt)
	{
		sl_int64 pos = (pt.x + (sl_int64)(getScrollX())) - m_widthLeftHeader;
		sl_ui_len startPosition = 0;
		sl_int32 index = -1;
		for (auto width : m_columnWidth) {
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

	void TableView::setItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setItemBackground(Drawable::createColorDrawable(color), mode);
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

	SLIB_DEFINE_EVENT_HANDLER(TableView, DrawItem, sl_int64 rowIndex, sl_int64 colIndex, Canvas* canvas, UIRect& rcItem)

	void TableView::dispatchDrawItem(sl_int64 rowIndex, sl_int64 colIndex, Canvas* canvas, UIRect& rcItem)
	{
		Ref<Drawable> background = m_backgroundItem;
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
		SLIB_INVOKE_EVENT_HANDLER(DrawItem, rowIndex, colIndex, canvas, rcItem)

		SimpleTextBoxParam param;
		param.text = "Test" + String::from(rowIndex) + "-" + String::from(colIndex);
		if (param.text.isEmpty()) {
			return;
		}
		SimpleTextBoxDrawParam drawParam;
		drawParam.frame = rcItem;
		drawParam.frame.left += getPaddingLeft();
		drawParam.frame.right -= getPaddingRight();
		drawParam.frame.top += getPaddingTop();
		drawParam.frame.bottom -= getPaddingBottom();
		drawParam.textColor = Color::Yellow;
		SimpleTextBox box;
		param.font = getFont();
		param.width = drawParam.frame.getWidth();
		param.ellipsizeMode = EllipsizeMode::None;
		param.align = Alignment::Center;
		box.update(param);
		box.draw(canvas, drawParam);
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

	void TableView::onDraw(Canvas* canvas)
	{
		if (m_rowCount <= 0) {
			return;
		}
		sl_int32 posY = (sl_int32)(getScrollY());
		sl_int32 posX = (sl_int32)(getScrollX());
		sl_int64 iRowStart = posY / m_rowHeight;
		sl_int64 iRowEnd = (posY + getHeight()) / m_rowHeight;
		if (iRowEnd >= m_rowCount) {
			iRowEnd = m_rowCount - 1;
		}
		sl_int32 rowCount = (sl_int32)(iRowEnd - iRowStart);

		sl_int64 widthLength = 0;
		sl_int64 iColumnStart = 0;
		sl_int64 iColumnEnd = m_columnCount;
		sl_int64 index = 0;
		for (auto width : m_columnWidth) {
			widthLength += width;
			if (widthLength <= posX) {
				iColumnStart = index;
			}
			else if (widthLength > posX + getWidth()) {
				iColumnEnd = index;
				break;
			}
			index++;
		}

		if (iColumnEnd >= m_columnCount) {
			iColumnEnd = m_columnCount - 1;
		}
		sl_int64 colCount = iColumnEnd - iColumnStart;

		{
			CanvasStateScope scope(canvas);
			Rectanglei rt(m_widthLeftHeader, m_heightTopHeader, getWidth() - m_widthRightHeader + 1, getHeight() - m_heightBottomHeader + 1);
			canvas->clipToRectangle(rt);
			UIRect rcItem;
			rcItem.top = m_heightTopHeader + (sl_ui_pos)(iRowStart * m_rowHeight - posY);
			rcItem.bottom = rcItem.top + (sl_ui_len)m_rowHeight;
			
			for (sl_reg i = 0; i <= rowCount; i++) {
				rcItem.left = m_widthLeftHeader + getColumnWidth(0, iColumnStart) - posX;
				for (sl_reg j = 0; j <= colCount; j++) {
					rcItem.right = rcItem.left + getColumnWidth(iColumnStart + j, iColumnStart + j + 1);
					dispatchDrawItem(iRowStart + i, iColumnStart + j, canvas, rcItem);
					rcItem.left = rcItem.right;
				}
				rcItem.top = rcItem.bottom;
				rcItem.bottom += m_rowHeight;
			}

			// draw line
			rcItem.top = m_heightTopHeader + (sl_ui_pos)(iRowStart * m_rowHeight - posY);
			rcItem.left = m_widthLeftHeader + getColumnWidth(0, iColumnStart) - posX;
			rcItem.right = rcItem.left + getColumnWidth(iColumnStart, iColumnEnd + 1);
			for (sl_reg i = 0; i <= rowCount; i++) {
				rcItem.bottom = rcItem.top + m_rowHeight;
				canvas->drawLine(Pointi(rcItem.left, rcItem.top), Pointi(rcItem.right, rcItem.top), Pen::createSolidPen(1, Color::Black));
				rcItem.top = rcItem.bottom;
			}
			canvas->drawLine(Pointi(rcItem.left, rcItem.top), Pointi(rcItem.right, rcItem.top), Pen::createSolidPen(1, Color::Black));

			rcItem.top = m_heightTopHeader + (sl_ui_pos)(iRowStart * m_rowHeight - posY);
			rcItem.bottom = rcItem.top + m_rowHeight * (rowCount + 1);
			rcItem.left = m_widthLeftHeader + getColumnWidth(0, iColumnStart) - posX;
			for (sl_reg j = 0; j <= colCount; j++) {
				rcItem.right = rcItem.left + getColumnWidth(iColumnStart + j, iColumnStart + j + 1);
				canvas->drawLine(Pointi(rcItem.left, rcItem.top), Pointi(rcItem.left, rcItem.bottom), Pen::createSolidPen(1, Color::Black));
				rcItem.left = rcItem.right;
			}
			canvas->drawLine(Pointi(rcItem.left, rcItem.top), Pointi(rcItem.left, rcItem.bottom), Pen::createSolidPen(1, Color::Black));
		}
		
		
	}

	void TableView::onClickEvent(UIEvent* ev)
	{
		if (ev->isMouseEvent()) {
			sl_int64 rowIndex = getRowIndexAt(ev->getPoint());
			sl_int64 colIndex = getColumnIndexAt(ev->getPoint());
			if (rowIndex >= 0 && colIndex >= 0) {
				UIPoint pt = ev->getPoint();
				sl_int64 posY = (sl_int64)(getScrollY()) + pt.y - m_heightTopHeader;
				pt.y = (sl_ui_pos)(posY - rowIndex * m_rowHeight);
				sl_int64 posX = (sl_int64)(getScrollX()) + pt.x - m_widthLeftHeader;
				pt.x = (sl_ui_pos)(posX - getColumnWidth(0, colIndex));
				dispatchClickItem(rowIndex, colIndex, pt, ev);
			}
		}
	}

	void TableView::onMouseEvent(UIEvent* ev)
	{
		UIAction action = ev->getAction();
		if (action == UIAction::RightButtonDown || action == UIAction::LeftButtonDoubleClick || action == UIAction::MouseMove || action == UIAction::MouseEnter) {
			sl_int64 rowIndex = getRowIndexAt(ev->getPoint());
			sl_int64 colIndex = getColumnIndexAt(ev->getPoint());
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

}
