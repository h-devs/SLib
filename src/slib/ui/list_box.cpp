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

#include "slib/ui/list_box.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(ListBox, View)

	ListBox::ListBox()
	{
		m_countItems = 0;
		m_heightItem = 0;
		m_indexSelected = -1;
		m_indexHover = -1;
	}

	void ListBox::init()
	{
		View::init();
		setCanvasScrolling(sl_false);
		setVerticalScrolling(sl_true, UIUpdateMode::Init);
	}

	ListBox::~ListBox()
	{
	}

	sl_uint64 ListBox::getItemsCount()
	{
		return m_countItems;
	}

	void ListBox::setItemsCount(sl_uint64 _count, UIUpdateMode mode)
	{
		sl_int64 count = _count;
		if (count < 0) {
			count = 0;
		}
		m_countItems = count;
		setContentHeight((sl_scroll_pos)(count * m_heightItem), mode);
	}

	sl_ui_len ListBox::getItemHeight()
	{
		return m_heightItem;
	}

	void ListBox::setItemHeight(sl_ui_len height, UIUpdateMode mode)
	{
		m_heightItem = height;
		setContentHeight((sl_scroll_pos)(height * m_countItems), mode);
	}

	sl_int64 ListBox::getSelectedIndex()
	{
		sl_int64 index = m_indexSelected;
		if (index >= 0) {
			if (index < m_countItems) {
				return index;
			}
		}
		return -1;
	}

	void ListBox::selectItem(sl_int64 index, UIUpdateMode mode)
	{
		if (index < 0) {
			index = -1;
		}
		if (m_indexSelected != index) {
			m_indexSelected = index;
			invalidate(mode);
		}
	}

	void ListBox::unselectItem(UIUpdateMode mode)
	{
		selectItem(-1, mode);
	}

	sl_int64 ListBox::getHoverIndex()
	{
		sl_int64 index = m_indexHover;
		if (index >= 0) {
			if (index < m_countItems) {
				return index;
			}
		}
		return -1;
	}

	sl_int64 ListBox::getItemIndexAt(const UIPoint& pt)
	{
		sl_int64 index = (pt.y + (sl_int64)(getScrollY())) / m_heightItem;
		if (index < m_countItems) {
			return index;
		}
		return -1;
	}

	SLIB_DEFINE_EVENT_HANDLER(ListBox, DrawItem, sl_uint64 itemIndex, Canvas* canvas, UIRect& rcItem)

	void ListBox::dispatchDrawItem(sl_uint64 itemIndex, Canvas* canvas, UIRect& rcItem)
	{
		SLIB_INVOKE_EVENT_HANDLER(DrawItem, itemIndex, canvas, rcItem)			
	}

	SLIB_DEFINE_EVENT_HANDLER(ListBox, ClickItem, sl_uint64 itemIndex, UIPoint& pos, UIEvent* ev)

	void ListBox::dispatchClickItem(sl_uint64 itemIndex, UIPoint& pos, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(ClickItem, itemIndex, pos, ev)
		if (ev->isStoppedPropagation()) {
			return;
		}
		if (ev->isPreventedDefault()) {
			return;
		}
		selectItem(itemIndex);
	}

	SLIB_DEFINE_EVENT_HANDLER(ListBox, RightButtonClickItem, sl_uint64 itemIndex, UIPoint& pos, UIEvent* ev)

	void ListBox::dispatchRightButtonClickItem(sl_uint64 itemIndex, UIPoint& pos, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(RightButtonClickItem, itemIndex, pos, ev)
	}

	SLIB_DEFINE_EVENT_HANDLER(ListBox, DoubleClickItem, sl_uint64 itemIndex, UIPoint& pos, UIEvent* ev)

	void ListBox::dispatchDoubleClickItem(sl_uint64 itemIndex, UIPoint& pos, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(DoubleClickItem, itemIndex, pos, ev)
	}

	void ListBox::onDraw(Canvas* canvas)
	{
		sl_int64 countItems = m_countItems;
		if (countItems <= 0) {
			return;
		}
		sl_int64 pos = (sl_int64)(getScrollY());
		sl_int32 heightItem = m_heightItem;
		sl_int64 iStart = pos / heightItem;
		sl_int64 iEnd = (pos + getHeight()) / heightItem;
		if (iEnd >= countItems) {
			iEnd = countItems - 1;
		}
		
		sl_reg n = (sl_reg)(iEnd - iStart);
		UIRect rcItem;
		rcItem.top = (sl_ui_pos)(iStart * heightItem - pos);
		rcItem.bottom = rcItem.top + (sl_ui_len)heightItem;
		rcItem.left = 0;
		rcItem.right = getWidth();
		for (sl_reg i = 0; i <= n; i++) {
			dispatchDrawItem(iStart + i, canvas, rcItem);
			rcItem.top = rcItem.bottom;
			rcItem.bottom += heightItem;
		}
	}

	void ListBox::onClickEvent(UIEvent* ev)
	{
		sl_int64 index = getItemIndexAt(ev->getPoint());
		if (index >= 0) {
			UIPoint pt = ev->getPoint();
			sl_int64 pos = (sl_int64)(getScrollY()) + pt.y;
			pt.y = (sl_ui_pos)(pos - index * m_heightItem);
			dispatchClickItem(index, pt, ev);
		}
	}

	void ListBox::onMouseEvent(UIEvent* ev)
	{
		UIAction action = ev->getAction();
		if (action == UIAction::RightButtonDown || action == UIAction::LeftButtonDoubleClick || action == UIAction::MouseMove || action == UIAction::MouseEnter) {
			sl_int64 index = getItemIndexAt(ev->getPoint());
			if (index >= 0) {
				UIPoint pt = ev->getPoint();
				sl_int64 pos = (sl_int64)(getScrollY()) + pt.y;
				pt.y = (sl_ui_pos)(pos - index * m_heightItem);
				if (action == UIAction::RightButtonDown) {
					dispatchRightButtonClickItem(index, pt, ev);
				} else if (action == UIAction::LeftButtonDoubleClick) {
					dispatchDoubleClickItem(index, pt, ev);
				}
				if (m_indexHover != index) {
					m_indexHover = index;
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

}
