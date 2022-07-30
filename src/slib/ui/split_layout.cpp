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

#include "slib/ui/split_layout.h"

#include "slib/ui/cursor.h"
#include "slib/ui/core.h"
#include "slib/graphics/canvas.h"

namespace slib
{
	SLIB_DEFINE_OBJECT(SplitLayout, ViewGroup)
	
	SplitLayout::SplitLayout()
	{
		setSavingCanvasState(sl_false);
		
		m_orientation = LayoutOrientation::Horizontal;
		
		m_dividerWidth = 1;
		m_dividerColor = Color::Gray;
		
		m_items.setCount_NoLock(2);
		m_items.getPointerAt(0)->weight = 0.5f;
		m_items.getPointerAt(1)->weight = 0.5f;
		
#if defined(SLIB_PLATFORM_IS_DESKTOP)
		m_cursorMargin = 4;
#else
		m_cursorMargin = SLIB_MIN(UI::getScreenWidth(), UI::getScreenHeight()) / 60;
#endif
		
		m_indexDividerDown = -1;
		m_posDown = 0;
		m_weightDown = 0;
		
		m_cursor = Cursor::getResizeLeftRight();
	}
	
	SplitLayout::~SplitLayout()
	{
	}

	void SplitLayout::init()
	{
		ViewGroup::init();

		setCapturingChildInstanceEvents(SLIB_FUNCTION_WEAKREF(this, _hitTestForCapturingChildInstanceEvents));
	}
	
	SplitLayout::Item::Item()
	{
		weight = 0;
		minWeight = 0;
		maxWeight = 1;
		minSize = 0;
		maxSize = 1000000;
		dividerWidth = 0;
		dividerColor = Color::zero();
		
		pos = 0;
		width = 0;
	}
	
	LayoutOrientation SplitLayout::getOrientation()
	{
		return m_orientation;
	}
	
	void SplitLayout::setOrientation(LayoutOrientation orientation, UIUpdateMode mode)
	{
		if (m_orientation == orientation) {
			return;
		}
		m_orientation = orientation;
		if (orientation == LayoutOrientation::Horizontal) {
			m_cursor = Cursor::getResizeLeftRight();
		} else {
			m_cursor = Cursor::getResizeUpDown();
		}
		if (!SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			ObjectLocker lock(this);
			_refreshItemFrames(mode);
		}
	}
	
	sl_bool SplitLayout::isHorizontal()
	{
		return m_orientation == LayoutOrientation::Horizontal;
	}
	
	void SplitLayout::setHorizontal(UIUpdateMode mode)
	{
		setOrientation(LayoutOrientation::Horizontal, mode);
	}
	
	sl_bool SplitLayout::isVertical()
	{
		return m_orientation == LayoutOrientation::Vertical;
	}
	
	void SplitLayout::setVertical(UIUpdateMode mode)
	{
		setOrientation(LayoutOrientation::Vertical, mode);
	}
	
	sl_size SplitLayout::getItemCount()
	{
		return m_items.getCount();
	}
	
	void SplitLayout::setItemCount(sl_size count, UIUpdateMode mode)
	{
		if (count < 2) {
			return;
		}
		ObjectLocker lock(this);
		m_items.setCount_NoLock(count);
		sl_size n = m_items.getCount();
		for (sl_size i = 0; i < n; i++) {
			setItemWeight(i, (sl_real)(1.0 / (double)n), UIUpdateMode::Init);
		}
		_refreshItemFrames(mode);
	}
	
	Ref<View> SplitLayout::getItemView(sl_size index)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			return item->view;
		}
		return sl_null;
	}
	
	void SplitLayout::setItemView(sl_size index, const Ref<View>& view, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			if (item->view.isNotNull()) {
				removeChild(item->view);
			}
			addChild(view);
			item->view = view;
			_refreshItemFrames(mode);
		}
	}
	
	sl_ui_len SplitLayout::getItemSize(sl_size index)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			return (sl_ui_len)(item->weight * _getTotalSize());
		}
		return 0;
	}
	
	void SplitLayout::setItemSize(sl_size index, sl_ui_len size, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		sl_ui_len total = _getTotalSize();
		if (total <= 0 || Math::isAlmostZero(total)) {
			return;
		}
		setItemWeight(index, (sl_real)size / (sl_real)total, mode);
	}
	
	sl_real SplitLayout::getItemWeight(sl_size index)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			return item->weight;
		}
		return 0;
	}
	
	void SplitLayout::setItemWeight(sl_size index, sl_real weight, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			sl_size nItems = m_items.getCount();
			Item* itemNext;
			if (index == nItems - 1) {
				index --;
				itemNext = item;
				item = m_items.getPointerAt(index);
				if (!item) {
					return;
				}
				weight = item->weight + itemNext->weight - weight;
			} else {
				itemNext = m_items.getPointerAt(index + 1);
				if (!itemNext) {
					return;
				}
			}
			sl_real weight1 = item->weight;
			sl_real weight2 = itemNext->weight;
			sl_real weightTotal = weight1 + weight2;
			sl_ui_len sizeTotal = _getTotalSize();
			sl_real minWeight1;
			sl_real maxWeight1;
			sl_real minWeight2;
			sl_real maxWeight2;
			if (sizeTotal <= 0 || Math::isAlmostZero(sizeTotal)) {
				minWeight1 = item->minWeight;
				maxWeight1 = item->maxWeight;
				minWeight2 = itemNext->minWeight;
				maxWeight2 = itemNext->maxWeight;
			} else {
				minWeight1 = (sl_real)(item->minSize) / (sl_real)sizeTotal;
				minWeight1 = SLIB_MAX(minWeight1, item->minWeight);
				maxWeight1 = (sl_real)(item->maxSize) / (sl_real)sizeTotal;
				maxWeight1 = SLIB_MIN(maxWeight1, item->maxWeight);
				minWeight2 = (sl_real)(itemNext->minSize) / (sl_real)sizeTotal;
				minWeight2 = SLIB_MAX(minWeight2, itemNext->minWeight);
				maxWeight2 = (sl_real)(itemNext->maxSize) / (sl_real)sizeTotal;
				maxWeight2 = SLIB_MIN(maxWeight2, itemNext->maxWeight);
			}
			if (weight < weightTotal - maxWeight2) {
				weight = weightTotal - maxWeight2;
			}
			if (weight > maxWeight1) {
				weight = maxWeight1;
			}
			if (weight > weightTotal - minWeight2) {
				weight = weightTotal - minWeight2;
			}
			if (weight < minWeight1) {
				weight = minWeight1;
			}
			if (weight < 0) {
				weight = 0;
			}
			if (weight > weightTotal) {
				weight = weightTotal;
			}
			item->weight = weight;
			itemNext->weight = weightTotal - weight;
			_refreshItemFrames(mode);
		}
	}
	
	sl_real SplitLayout::getItemMinimumWeight(sl_size index)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			return item->minWeight;
		}
		return 0;
	}
	
	void SplitLayout::setItemMinimumWeight(sl_size index, sl_real weight, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			if (weight < 0) {
				weight = 0;
			}
			if (weight > 1) {
				weight = 1;
			}
			item->minWeight = weight;
			if (index > 0) {
				setItemWeight(index - 1, getItemWeight(index - 1), UIUpdateMode::Init);
			}
			setItemWeight(index, item->weight, mode);
		}
	}
	
	sl_real SplitLayout::getItemMaximumWeight(sl_size index)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			return item->maxWeight;
		}
		return 0;
	}
	
	void SplitLayout::setItemMaximumWeight(sl_size index, sl_real weight, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			if (weight < 0) {
				weight = 0;
			}
			if (weight > 1) {
				weight = 1;
			}
			item->maxWeight = weight;
			if (index > 0) {
				setItemWeight(index - 1, getItemWeight(index - 1), UIUpdateMode::Init);
			}
			setItemWeight(index, item->weight, mode);
		}
	}
	
	sl_ui_len SplitLayout::getItemMinimumSize(sl_size index)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			return item->minSize;
		}
		return 0;
	}
	
	void SplitLayout::setItemMinimumSize(sl_size index, sl_ui_len size, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			item->minSize = size;
			if (index > 0) {
				setItemWeight(index - 1, getItemWeight(index - 1), UIUpdateMode::Init);
			}
			setItemWeight(index, item->weight, mode);
		}
	}
	
	sl_ui_len SplitLayout::getItemMaximumSize(sl_size index)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			return item->maxSize;
		}
		return 0;
	}
	
	void SplitLayout::setItemMaximumSize(sl_size index, sl_ui_len size, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			item->maxSize = size;
			if (index > 0) {
				setItemWeight(index - 1, getItemWeight(index - 1), UIUpdateMode::Init);
			}
			setItemWeight(index, item->weight, mode);
		}
	}
	
	sl_ui_len SplitLayout::getItemDividerWidth(sl_size index)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			return item->dividerWidth;
		}
		return 0;
	}
	
	void SplitLayout::setItemDividerWidth(sl_size index, sl_ui_len width, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			item->dividerWidth = width;
			_resetWeights(mode);
		}
	}
	
	Ref<Drawable> SplitLayout::getItemDividerBackground(sl_size index)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			return item->dividerBackground;
		}
		return sl_null;
	}
	
	void SplitLayout::setItemDividerBackground(sl_size index, const Ref<Drawable>& background, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			item->dividerBackground = background;
			invalidate(mode);
		}
	}
	
	Color SplitLayout::getItemDividerColor(sl_size index)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			return item->dividerColor;
		}
		return 0;
	}
	
	void SplitLayout::setItemDividerColor(sl_size index, const Color& color, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		Item* item = m_items.getPointerAt(index);
		if (item) {
			item->dividerColor = color;
			invalidate(mode);
		}
	}
	
	sl_ui_len SplitLayout::getDividerWidth()
	{
		return m_dividerWidth;
	}
	
	void SplitLayout::setDividerWidth(sl_ui_len width, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		m_dividerWidth = width;
		_resetWeights(mode);
	}
	
	Ref<Drawable> SplitLayout::getDividerBackground()
	{
		return m_dividerBackground;
	}
	
	void SplitLayout::setDividerBackground(const Ref<Drawable>& background, UIUpdateMode mode)
	{
		m_dividerBackground = background;
		invalidate(mode);
	}
	
	Color SplitLayout::getDividerColor()
	{
		return m_dividerColor;
	}
	
	void SplitLayout::setDividerColor(const Color& color, UIUpdateMode mode)
	{
		m_dividerColor = color;
		invalidate(mode);
	}
	
	sl_ui_len SplitLayout::getCursorMargin()
	{
		return m_cursorMargin;
	}
	
	void SplitLayout::setCursorMargin(sl_ui_len margin)
	{
		m_cursorMargin = margin;
	}
	
	void SplitLayout::relayout(UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		_refreshItemFrames(mode);
	}
	
	void SplitLayout::onResize(sl_ui_len width, sl_ui_len height)
	{
		ObjectLocker lock(this);
		_resetWeights(UIUpdateMode::UpdateLayout);
	}
	
	void SplitLayout::onDraw(Canvas* canvas)
	{
		ObjectLocker lock(this);
		LayoutOrientation orientation = m_orientation;
		sl_ui_pos height;
		if (orientation == LayoutOrientation::Horizontal) {
			height = getHeight();
		} else {
			height = getWidth();
		}
		ListElements<Item> items(m_items);
		for (sl_size i = 0; i + 1 < items.count; i++) {
			Item& item = items[i];
			sl_ui_pos dw = item.dividerWidth;
			if (dw == 0) {
				dw = m_dividerWidth;
			}
			if (dw > 0) {
				Color color = item.dividerColor;
				if (color.isZero()) {
					color = m_dividerColor;
				}
				Ref<Drawable> background = item.dividerBackground;
				if (background.isNull()) {
					background = m_dividerBackground;
				}
				if (color.isNotZero() || background.isNotNull()) {
					UIRect rc;
					if (orientation == LayoutOrientation::Horizontal) {
						rc.left = item.pos + item.width;
						rc.top = 0;
						rc.right = rc.left + dw;
						rc.bottom = height;
					} else {
						rc.left = 0;
						rc.top = item.pos + item.width;
						rc.right = height;
						rc.bottom = rc.top + dw;
					}
					if (color.isNotZero()) {
						canvas->fillRectangle(rc, color);
					}
					if (background.isNotNull()) {
						canvas->draw(rc, background);
					}
				}
			}
		}
	}
	
	void SplitLayout::dispatchMouseEvent(UIEvent* ev)
	{
		{
			UIAction action = ev->getAction();
			UIPoint pt = ev->getPoint();
			ObjectLocker lock(this);
			if (action == UIAction::LeftButtonDown) {
				sl_int32 index = _getDividerIndexAtPoint(pt);
				if (index >= 0) {
					m_weightDown = getItemWeight(index);
					if (m_orientation == LayoutOrientation::Horizontal) {
						m_posDown = pt.x;
					} else {
						m_posDown = pt.y;
					}
				}
				if (index >= 0) {
					m_indexDividerDown = index;
					return;
				} else {
					m_indexDividerDown = -1;
				}
			}
			if (action == UIAction::LeftButtonDrag || action == UIAction::LeftButtonUp) {
				sl_int32 index = m_indexDividerDown;
				if (index >= 0) {
					sl_real dx;
					if (m_orientation == LayoutOrientation::Horizontal) {
						dx = (sl_real)(pt.x - m_posDown) / (sl_real)(getWidth());
					} else {
						dx = (sl_real)(pt.y - m_posDown) / (sl_real)(getHeight());
					}
					setItemWeight(index, m_weightDown + dx);
					return;
				}
			}
		}
		
		ViewGroup::dispatchMouseEvent(ev);
		
	}
	
	void SplitLayout::dispatchSetCursor(UIEvent* ev)
	{
		{
			UIPoint pt = ev->getPoint();
			ObjectLocker lock(this);
			sl_int32 index = _getDividerIndexAtPoint(pt);
			if (index >= 0) {
				ev->setCursor(m_cursor);
				return;
			}
		}
		
		ViewGroup::dispatchSetCursor(ev);
		
	}
	
	sl_ui_len SplitLayout::_getTotalSize()
	{
		sl_ui_pos total;
		if (m_orientation == LayoutOrientation::Horizontal) {
			total = getWidth();
		} else {
			total = getHeight();
		}
		ListElements<Item> items(m_items);
		for (sl_size i = 0; i + 1 < items.count; i++) {
			Item& item = items[i];
			sl_ui_pos dw = item.dividerWidth;
			if (dw == 0) {
				dw = m_dividerWidth;
			}
			if (dw < 0) {
				dw = 0;
			}
			total -= dw;
		}
		if (total < 0) {
			total = 0;
		}
		return total;
	}
	
	void SplitLayout::_refreshItemFrames(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		
		LayoutOrientation orientation = m_orientation;
		sl_ui_pos total = _getTotalSize();
		sl_ui_pos pos = 0;
		sl_ui_pos height;
		if (orientation == LayoutOrientation::Horizontal) {
			height = getHeight();
		} else {
			height = getWidth();
		}
		
		ListElements<Item> items(m_items);
		for (sl_size i = 0; i < items.count; i++) {
			Item& item = items[i];
			sl_ui_pos width = (sl_ui_pos)(item.weight * total);
			if (width < 0) {
				width = 0;
			}
			if (item.view.isNotNull()) {
				if (orientation == LayoutOrientation::Horizontal) {
					item.view->setFrame(UIRect(pos, 0, pos + width, height));
				} else {
					item.view->setFrame(UIRect(0, pos, height, pos + width));
				}
			}
			
			item.pos = pos;
			item.width = width;
			
			pos += width;
			sl_ui_pos dw = item.dividerWidth;
			if (dw == 0) {
				dw = m_dividerWidth;
			}
			if (dw < 0) {
				dw = 0;
			}
			pos += dw;
		}
		
		invalidate(mode);
	}
	
	void SplitLayout::_resetWeights(UIUpdateMode mode)
	{
		ListElements<Item> items(m_items);
		for (sl_size i = 0; i + 1 < items.count; i++) {
			setItemWeight(i, items[i].weight, UIUpdateMode::Init);
		}
		_refreshItemFrames(mode);
	}
	
	sl_int32 SplitLayout::_getDividerIndexAtPoint(const UIPoint& pt)
	{
		LayoutOrientation orientation = m_orientation;
		sl_ui_pos height;
		if (orientation == LayoutOrientation::Horizontal) {
			height = getHeight();
		} else {
			height = getWidth();
		}
		sl_ui_pos margin = m_cursorMargin;
		ListElements<Item> items(m_items);
		for (sl_size i = 0; i + 1 < items.count; i++) {
			Item& item = items[i];
			sl_ui_pos dw = item.dividerWidth;
			if (dw == 0) {
				dw = m_dividerWidth;
			}
			if (dw < 0) {
				dw = 0;
			}
			UIRect rc;
			if (orientation == LayoutOrientation::Horizontal) {
				rc.left = item.pos + item.width;
				rc.top = 0;
				rc.right = rc.left + dw;
				rc.bottom = height;
				rc.left -= margin;
				rc.right += margin;
			} else {
				rc.left = 0;
				rc.top = item.pos + item.width;
				rc.right = height;
				rc.bottom = rc.top + dw;
				rc.top -= margin;
				rc.bottom += margin;
			}
			if (rc.containsPoint(pt)) {
				return (sl_int32)i;
			}
		}
		return -1;
	}
		
	sl_bool SplitLayout::_hitTestForCapturingChildInstanceEvents(const UIPoint& pt)
	{
		ObjectLocker lock(this);
		sl_int32 index = _getDividerIndexAtPoint(pt);
		return index >= 0;
	}

	VerticalSplitLayout::VerticalSplitLayout()
	{
		setOrientation(LayoutOrientation::Vertical, UIUpdateMode::Init);
	}
	
	VerticalSplitLayout::~VerticalSplitLayout()
	{
	}

	HorizontalSplitLayout::HorizontalSplitLayout()
	{
		setOrientation(LayoutOrientation::Horizontal, UIUpdateMode::Init);
	}

	HorizontalSplitLayout::~HorizontalSplitLayout()
	{
	}

}
