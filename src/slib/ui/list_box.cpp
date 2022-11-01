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

#include "slib/graphics/canvas.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(ListBox, View)

	ListBox::ListBox()
	{
		setCanvasScrolling(sl_false);
		setVerticalScrolling(sl_true, UIUpdateMode::Init);
		setFocusable(sl_true);

		m_countItems = 0;
		m_heightItem = 100;
		m_indexHover = -1;

		m_flagMultipleSelection = sl_false;
		m_indexSelected = -1;
		m_indexFocused = -1;
		m_indexLastSelected = -1;
	}

	ListBox::~ListBox()
	{
	}

	sl_uint64 ListBox::getItemCount()
	{
		return m_countItems;
	}

	void ListBox::setItemCount(sl_uint64 _count, UIUpdateMode mode)
	{
		sl_int64 count = _count;
		if (count < 0) {
			count = 0;
		}
		if (m_countItems == count) {
			return;
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
		if (height < 1) {
			return;
		}
		if (m_heightItem == height) {
			return;
		}
		m_heightItem = height;
		setContentHeight((sl_scroll_pos)(height * m_countItems), mode);
	}

	sl_bool ListBox::isMultipleSelection()
	{
		return m_flagMultipleSelection;
	}

	void ListBox::setMultipleSelection(sl_bool flag, UIUpdateMode mode)
	{
		if (m_flagMultipleSelection != flag) {
			m_flagMultipleSelection = flag;
			if (flag) {
				m_indexSelected = -1;
			} else {
				m_mapSelection.removeAll();
			}
			invalidate(mode);
		}
	}

	sl_bool ListBox::isSelectedIndex(sl_int64 index)
	{
		if (index < 0) {
			return sl_false;
		}
		if (index >= m_countItems) {
			return sl_false;
		}
		if (m_flagMultipleSelection) {
			return m_mapSelection.find(index);
		} else {
			return m_indexSelected == index;
		}
	}

	sl_int64 ListBox::getSelectedIndex()
	{
		if (m_flagMultipleSelection) {
			ObjectLocker lock(&m_mapSelection);
			auto node = m_mapSelection.getLastNode();
			if (node) {
				return node->key;
			}
		} else {
			sl_int64 index = m_indexSelected;
			if (index >= 0) {
				if (index < m_countItems) {
					return index;
				}
			}
		}
		return -1;
	}

	void ListBox::setSelectedIndex(sl_int64 index, UIUpdateMode mode)
	{
		if (index < 0) {
			unselectAll(mode);
			return;
		}
		if (index >= m_countItems) {
			return;
		}
		if (m_flagMultipleSelection) {
			ObjectLocker lock(&m_mapSelection);
			m_mapSelection.removeAll_NoLock();
			m_mapSelection.put_NoLock(index, sl_true);
			invalidate(mode);
		} else {
			if (m_indexSelected != index) {
				m_indexSelected = index;
				invalidate(mode);
			}
		}
	}

	List<sl_uint64> ListBox::getSelectedIndices()
	{
		if (m_flagMultipleSelection) {
			return m_mapSelection.getAllKeys();
		} else {
			sl_int64 index = m_indexSelected;
			if (index >= 0) {
				if (index < m_countItems) {
					return List<sl_uint64>::createFromElement(index);
				}
			}
			return sl_null;
		}
	}

	void ListBox::setSelectedIndices(const ListParam<sl_uint64>& _indices, UIUpdateMode mode)
	{
		ListLocker<sl_uint64> indices(_indices);
		if (!(indices.count)) {
			unselectAll(mode);
			return;
		}
		sl_uint64 nTotal = m_countItems;
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
	}

	void ListBox::selectItem(sl_int64 index, UIUpdateMode mode)
	{
		if (index < 0) {
			return;
		}
		if (index >= m_countItems) {
			return;
		}
		if (m_flagMultipleSelection) {
			sl_bool flagInsert = sl_false;
			m_mapSelection.put_NoLock(index, sl_true, &flagInsert);
			if (flagInsert) {
				invalidate(mode);
			}
		} else {
			if (m_indexSelected != index) {
				m_indexSelected = index;
				invalidate(mode);
			}
		}
	}

	void ListBox::unselectItem(sl_int64 index, UIUpdateMode mode)
	{
		if (index < 0) {
			return;
		}
		if (m_flagMultipleSelection) {
			if (m_mapSelection.remove(index)) {
				invalidate(mode);
			}
		} else {
			if (m_indexSelected == index) {
				m_indexSelected = -1;
				invalidate(mode);
			}
		}
	}

	void ListBox::toggleItemSelection(sl_int64 index, UIUpdateMode mode)
	{
		if (index < 0) {
			return;
		}
		if (index >= m_countItems) {
			return;
		}
		if (m_flagMultipleSelection) {
			ObjectLocker lock(&m_mapSelection);
			auto node = m_mapSelection.find_NoLock(index);
			if (node) {
				m_mapSelection.removeAt(node);
				invalidate(mode);
			} else {
				sl_bool flagInsert = sl_false;
				m_mapSelection.put_NoLock(index, sl_true, &flagInsert);
				if (flagInsert) {
					invalidate(mode);
				}
			}
		} else {
			if (m_indexSelected != index) {
				m_indexSelected = index;
				invalidate(mode);
			} else {
				m_indexSelected = -1;
				invalidate(mode);
			}
		}
	}

	void ListBox::selectItems(const ListParam<sl_uint64>& _indices, UIUpdateMode mode)
	{
		ListLocker<sl_uint64> indices(_indices);
		if (!(indices.count)) {
			return;
		}
		sl_uint64 nTotal = m_countItems;
		if (!nTotal) {
			return;
		}
		if (m_flagMultipleSelection) {
			ObjectLocker locker(&m_mapSelection);
			sl_bool flagChanged = sl_false;
			for (sl_size i = 0; i < indices.count; i++) {
				if (indices[i] < nTotal) {
					sl_bool flagInsert = sl_false;
					m_mapSelection.put_NoLock(indices[i], sl_true, &flagInsert);
					if (flagInsert) {
						flagChanged = sl_true;
					}
				}
			}
			if (flagChanged) {
				invalidate(mode);
			}
		} else {
			sl_int64 index = indices[indices.count - 1];
			if ((sl_uint64)index < nTotal) {
				if (m_indexSelected != index) {
					m_indexSelected = index;
					invalidate(mode);
				}
			}
		}
	}

	void ListBox::unselectItems(const ListParam<sl_uint64>& _indices, UIUpdateMode mode)
	{
		ListLocker<sl_uint64> indices(_indices);
		if (!(indices.count)) {
			return;
		}
		if (m_flagMultipleSelection) {
			ObjectLocker locker(&m_mapSelection);
			sl_bool flagChanged = sl_false;
			for (sl_size i = 0; i < indices.count; i++) {
				if (m_mapSelection.remove_NoLock(indices[i])) {
					flagChanged = sl_true;
				}
			}
			if (flagChanged) {
				invalidate(mode);
			}
		} else {
			for (sl_size i = 0; i < indices.count; i++) {
				if (indices[i] == m_indexSelected) {
					m_indexSelected = -1;
					invalidate(mode);
					return;
				}
			}
		}
	}

	void ListBox::setSelectedRange(sl_int64 from, sl_int64 to, UIUpdateMode mode)
	{
		if (m_flagMultipleSelection) {
			if (from > to) {
				Swap(from, to);
			}
			if (from < 0) {
				return;
			}
			sl_int64 nTotal = m_countItems;
			if (!nTotal) {
				return;
			}
			if (to >= nTotal) {
				to = nTotal - 1;
			}
			ObjectLocker locker(&m_mapSelection);
			m_mapSelection.removeAll_NoLock();
			for (sl_int64 i = from; i <= to; i++) {
				m_mapSelection.put_NoLock(i, sl_true);
			}
			invalidate(mode);
		} else {
			setSelectedIndex(to, mode);
		}
	}

	void ListBox::selectRange(sl_int64 from, sl_int64 to, UIUpdateMode mode)
	{
		if (m_flagMultipleSelection) {
			if (from > to) {
				Swap(from, to);
			}
			if (from < 0) {
				return;
			}
			sl_int64 nTotal = m_countItems;
			if (!nTotal) {
				return;
			}
			if (to >= nTotal) {
				to = nTotal - 1;
			}
			ObjectLocker locker(&m_mapSelection);
			sl_bool flagChanged = sl_false;
			for (sl_int64 i = from; i <= to; i++) {
				sl_bool flagInsert = sl_false;
				m_mapSelection.put_NoLock(i, sl_true, &flagInsert);
				if (flagInsert) {
					flagChanged = sl_true;
				}
			}
			if (flagChanged) {
				invalidate(mode);
			}
		} else {
			setSelectedIndex(to, mode);
		}
	}

	void ListBox::unselectAll(UIUpdateMode mode)
	{
		if (m_flagMultipleSelection) {
			ObjectLocker locker(&m_mapSelection);
			if (m_mapSelection.isEmpty()) {
				return;
			}
			m_mapSelection.removeAll_NoLock();
		} else {
			if (m_indexSelected < 0) {
				return;
			}
			m_indexSelected = -1;
		}
		invalidate(mode);
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

	Ref<Drawable> ListBox::getItemBackground()
	{
		return m_backgroundItem;
	}

	void ListBox::setItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_backgroundItem = drawable;
		invalidate(mode);
	}

	void ListBox::setItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setItemBackground(Drawable::createColorDrawable(color), mode);
	}

	Ref<Drawable> ListBox::getSelectedItemBackground()
	{
		return m_backgroundSelectedItem;
	}

	void ListBox::setSelectedItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_backgroundSelectedItem = drawable;
		invalidate(mode);
	}

	void ListBox::setSelectedItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setSelectedItemBackground(Drawable::createColorDrawable(color), mode);
	}

	Ref<Drawable> ListBox::getHoverItemBackground()
	{
		return m_backgroundHoverItem;
	}

	void ListBox::setHoverItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_backgroundHoverItem = drawable;
		invalidate(mode);
	}

	void ListBox::setHoverItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setHoverItemBackground(Drawable::createColorDrawable(color), mode);
	}

	Ref<Drawable> ListBox::getFocusedItemBackground()
	{
		return m_backgroundFocusedItem;
	}

	void ListBox::setFocusedItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_backgroundFocusedItem = drawable;
		invalidate(mode);
	}

	void ListBox::setFocusedItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setFocusedItemBackground(Drawable::createColorDrawable(color), mode);
	}

	SLIB_DEFINE_EVENT_HANDLER(ListBox, DrawItem, sl_uint64 itemIndex, Canvas* canvas, UIRect& rcItem)

	void ListBox::dispatchDrawItem(sl_uint64 itemIndex, Canvas* canvas, UIRect& rcItem)
	{
		Ref<Drawable> background = m_backgroundItem;
		if (m_backgroundSelectedItem.isNotNull() && isSelectedIndex(itemIndex)) {
			background = m_backgroundSelectedItem;
		} else if (m_backgroundHoverItem.isNotNull() && itemIndex == getHoverIndex()) {
			background = m_backgroundHoverItem;
		} else if (m_backgroundFocusedItem.isNotNull() && isFocused() && itemIndex == m_indexFocused) {
			background = m_backgroundFocusedItem;
		}
		if (background.isNotNull()) {
			canvas->draw(rcItem, background);
		}
		SLIB_INVOKE_EVENT_HANDLER(DrawItem, itemIndex, canvas, rcItem)
	}

	SLIB_DEFINE_EVENT_HANDLER(ListBox, ClickItem, sl_uint64 itemIndex, UIPoint& pos, UIEvent* ev)

	void ListBox::dispatchClickItem(sl_uint64 itemIndex, UIPoint& pos, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(ClickItem, itemIndex, pos, ev)
		if (ev->isPreventedDefault()) {
			return;
		}
		m_indexFocused = itemIndex;
		if (ev->isShiftKey()) {
			if (m_indexLastSelected >= 0) {
				if (ev->isControlKey() || ev->isCommandKey()) {
					selectRange(m_indexLastSelected, itemIndex);
				} else {
					setSelectedRange(m_indexLastSelected, itemIndex);
				}
			} else {
				setSelectedIndex(itemIndex);
			}
			dispatchChangedSelection(ev);
		} else {
			if (ev->isControlKey() || ev->isCommandKey()) {
				toggleItemSelection(itemIndex);
			} else {
				setSelectedIndex(itemIndex);
			}
			dispatchChangedSelection(ev);
			m_indexLastSelected = itemIndex;
		}
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

	SLIB_DEFINE_EVENT_HANDLER(ListBox, ChangedSelection, UIEvent* ev)

	void ListBox::dispatchChangedSelection(UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(ChangedSelection, ev)
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
		if (ev->isMouseEvent()) {
			sl_int64 index = getItemIndexAt(ev->getPoint());
			if (index >= 0) {
				UIPoint pt = ev->getPoint();
				sl_int64 pos = (sl_int64)(getScrollY()) + pt.y;
				pt.y = (sl_ui_pos)(pos - index * m_heightItem);
				dispatchClickItem(index, pt, ev);
			}
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

	void ListBox::onKeyEvent(UIEvent* ev)
	{
		sl_int64 nTotal = m_countItems;
		if (nTotal <= 0) {
			return;
		}

		if (ev->getAction() == UIAction::KeyDown) {

			Keycode key = ev->getKeycode();

			if (key == Keycode::Space || key == Keycode::Enter) {

				sl_int64 index = m_indexFocused;
				if (index >= 0) {
					UIPoint pt(0, 0);
					dispatchClickItem(index, pt, ev);
				}

			} else if (key == Keycode::Up || key == Keycode::Down || key == Keycode::Home || key == Keycode::End) {

				sl_int64 index = m_indexFocused;

				if (key == Keycode::Up) {
					if (index > 0) {
						index--;
					} else if (index < 0) {
						index = nTotal - 1;
					} else {
						return;
					}
				} else if (key == Keycode::Down) {
					if (index >= 0) {
						if (index < nTotal - 1) {
							index++;
						} else {
							return;
						}
					} else {
						index = 0;
					}
				} else if (key == Keycode::Home) {
					index = 0;
				} else {
					index = nTotal - 1;
				}

				m_indexFocused = index;

				// check focused item is visible
				{
					sl_int64 sy = (sl_int64)(getScrollY());
					sl_ui_len height = m_heightItem;
					sl_int64 y1 = index * height;
					sl_int64 y2 = y1 + height;
					if (y1 < sy || y2 > sy + getHeight()) {
						if (key == Keycode::Up || key == Keycode::Home) {
							scrollTo(0, (sl_scroll_pos)y1, UIUpdateMode::None);
						} else {
							scrollTo(0, (sl_scroll_pos)(y2 - getHeight()), UIUpdateMode::None);
						}
					}
				}

				if (ev->isShiftKey()) {
					UIPoint pt(0, 0);
					dispatchClickItem(index, pt, ev);
				} else {
					if (ev->isControlKey() || ev->isCommandKey()) {
						invalidate();
					} else {
						UIPoint pt(0, 0);
						dispatchClickItem(index, pt, ev);
					}
				}

				ev->preventDefault();

			} else if (key == Keycode::Escape) {

				if (getSelectedIndex() < 0) {
					m_indexFocused = -1;
				}
				invalidate();

			}
		}
	}

}
