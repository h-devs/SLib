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

#include "slib/ui/label_list.h"

#include "label_list_base_impl.h"

namespace slib
{

	template class LabelListViewBase<LabelList, sl_int64>;

	SLIB_DEFINE_OBJECT(LabelList, ListBox)

    LabelList::LabelList()
	{
		m_flagUseFontHeight = sl_true;
		m_heightFont = 0;
		m_lineHeightWeight = 1.6f;

		m_gravity = Alignment::Left;
		m_ellipsizeMode = EllipsizeMode::None;

		m_textColor = Color::Black;
	}

	LabelList::~LabelList()
	{
	}

	void LabelList::init()
	{
		ListBox::init();
		setBackgroundColor(Color::White, UIUpdateMode::Init);
		setSelectedItemBackgroundColor(Color(35, 12, 146), UIUpdateMode::Init);
		setSelectedTextColor(Color::White, UIUpdateMode::Init);
		setHoverItemBackgroundColor(Color(102, 150, 215), UIUpdateMode::Init);
		setHoverTextColor(Color::White, UIUpdateMode::Init);
		setFocusedItemBackgroundColor(Color(193, 231, 234), UIUpdateMode::Init);
		setPadding(2, 2, 2, 2, UIUpdateMode::Init);
	}

	void LabelList::setItemHeight(sl_ui_len height, UIUpdateMode mode)
	{
		if (height > 0) {
			m_flagUseFontHeight = sl_false;
			ListBox::setItemHeight(height, mode);
		} else {
			Ref<Font> font = getFont();
			if (font.isNotNull()) {
				m_flagUseFontHeight = sl_true;
				m_heightFont = _getFontHeight();
				ListBox::setItemHeight(m_heightFont, mode);
			}
		}
	}

	float LabelList::getLineHeightWeight()
	{
		return m_lineHeightWeight;
	}

	void LabelList::setLineHeightWeight(float weight, UIUpdateMode mode)
	{
		m_flagUseFontHeight = sl_true;
		m_lineHeightWeight = weight;
		Ref<Font> font = getFont();
		if (font.isNotNull()) {
			m_heightFont = _getFontHeight();
			ListBox::setItemHeight(m_heightFont, mode);
		}
	}

	Alignment LabelList::getGravity()
	{
		return m_gravity;
	}

	void LabelList::setGravity(const Alignment& align, UIUpdateMode updateMode)
	{
		m_gravity = align;
		invalidate(updateMode);
	}

	EllipsizeMode LabelList::getEllipsize()
	{
		return m_ellipsizeMode;
	}

	void LabelList::setEllipsize(EllipsizeMode ellipsizeMode, UIUpdateMode updateMode)
	{
		m_ellipsizeMode = ellipsizeMode;
		invalidate(updateMode);
	}

	Color LabelList::getTextColor()
	{
		return m_textColor;
	}

	void LabelList::setTextColor(const Color& color, UIUpdateMode mode)
	{
		m_textColor = color;
		invalidate(mode);
	}

	Color LabelList::getSelectedTextColor()
	{
		return m_textColorSelected;
	}

	void LabelList::setSelectedTextColor(const Color& color, UIUpdateMode mode)
	{
		m_textColorSelected = color;
		invalidate(mode);
	}

	Color LabelList::getHoverTextColor()
	{
		return m_textColorHover;
	}

	void LabelList::setHoverTextColor(const Color& color, UIUpdateMode mode)
	{
		m_textColorHover = color;
		invalidate(mode);
	}

	Color LabelList::getFocusedTextColor()
	{
		return m_textColorFocused;
	}

	void LabelList::setFocusedTextColor(const Color& color, UIUpdateMode mode)
	{
		m_textColorFocused = color;
		invalidate(mode);
	}

	void LabelList::invalidateLabelAppearance(UIUpdateMode mode)
	{
		invalidate(mode);
	}

	void LabelList::notifyRefreshItems(UIUpdateMode mode)
	{
		invalidate(mode);
	}

	void LabelList::notifyInsertItem(sl_int64 index, const String& title, UIUpdateMode mode)
	{
		m_countItems++;
		invalidate(mode);
	}

	void LabelList::notifyRemoveItem(sl_int64 index, UIUpdateMode mode)
	{
		sl_uint64 n = m_countItems;
		if (n > 0) {
			n--;
			m_countItems = n;
			invalidate(mode);
		}
	}

	void LabelList::notifySetItemTitle(sl_int64 index, const String& title, UIUpdateMode mode)
	{
		invalidate(mode);
	}

	void LabelList::dispatchDrawItem(sl_uint64 itemIndex, Canvas* canvas, UIRect& rcItem)
	{
		ListBox::dispatchDrawItem(itemIndex, canvas, rcItem);
		SimpleTextBoxParam param;
		param.text = getItemTitle(itemIndex);
		if (param.text.isEmpty()) {
			return;
		}
		SimpleTextBoxDrawParam drawParam;
		drawParam.frame = rcItem;
		drawParam.frame.left += getPaddingLeft();
		drawParam.frame.right -= getPaddingRight();
		drawParam.frame.top += getPaddingTop();
		drawParam.frame.bottom -= getPaddingBottom();
		if (m_textColorSelected.isNotZero() && isSelectedIndex(itemIndex)) {
			drawParam.color = m_textColorSelected;
		} else if (m_textColorHover.isNotZero() && itemIndex == getHoverIndex()) {
			drawParam.color = m_textColorHover;
		} else if (m_textColorFocused.isNotZero() && isFocused() && itemIndex == m_indexFocused) {
			drawParam.color = m_textColorFocused;
		} else {
			drawParam.color = m_textColor;
		}
		SimpleTextBox box;
		param.font = getFont();
		param.width = drawParam.frame.getWidth();
		param.ellipsizeMode = m_ellipsizeMode;
		param.align = m_gravity;
		box.update(param);
		box.draw(canvas, drawParam);
	}

	void LabelList::onUpdateFont(const Ref<Font>& font)
	{
		if (m_flagUseFontHeight) {
			m_heightFont = _getFontHeight();
			ListBox::setItemHeight(m_heightFont);
		}
	}

	void LabelList::onDraw(Canvas* canvas)
	{
		if (m_flagUseFontHeight) {
			sl_ui_len height = _getFontHeight();
			if (height != m_heightFont) {
				m_heightFont = height;
				ListBox::setItemHeight(height);
				return;
			}
		}
		ListBox::onDraw(canvas);
	}

	sl_ui_len LabelList::_getFontHeight()
	{
		Ref<Font> font = getFont();
		if (font.isNotNull()) {
			return (sl_ui_len)(font->getFontHeight() * m_lineHeightWeight) + getPaddingTop() + getPaddingBottom();
		} else {
			return 20 + getPaddingTop() + getPaddingBottom();
		}
	}

}
