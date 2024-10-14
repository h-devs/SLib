/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/group_box.h"

#include "slib/ui/core.h"
#include "slib/graphics/canvas.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(GroupBox, ViewGroup)

	GroupBox::GroupBox()
	{
		setUsingFont();
		setBorder(Pen::createSolidPen(1, Color(150, 150, 150)), UIUpdateMode::Init);

		m_labelColor = Color::Black;
		m_iconWidth = 0;
		m_iconHeight = 0;
		m_iconMarginLeft = 0;
		m_iconMarginRight = 0;
		m_paddingBorder = 0;
		m_paddingTop = 0;
	}

	GroupBox::~GroupBox()
	{
	}

	String GroupBox::getLabel()
	{
		return m_label;
	}

	void GroupBox::setLabel(const String& text, UIUpdateMode mode)
	{
		m_label = text;
		invalidate(mode);
	}

	Color GroupBox::getLabelColor()
	{
		return m_labelColor;
	}

	void GroupBox::setLabelColor(const Color& color, UIUpdateMode mode)
	{
		m_labelColor = color;
		invalidate(mode);
	}

	Ref<Font> GroupBox::getLabelFont()
	{
		Ref<Font> font = m_labelFont;
		if (font.isNotNull()) {
			return font;
		}
		return getFont();
	}

	void GroupBox::setLabelFont(const Ref<Font>& font, UIUpdateMode mode)
	{
		m_labelFont = font;
		invalidate(mode);
	}

	void GroupBox::setLabelFont(const FontDesc& desc, UIUpdateMode mode)
	{
		setLabelFont(Font::create(desc), mode);
	}

	Ref<Drawable> GroupBox::getIcon()
	{
		return m_icon;
	}

	void GroupBox::setIcon(const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		m_icon = icon;
		invalidate(mode);
	}

	UISize GroupBox::getIconSize()
	{
		return UISize(m_iconWidth, m_iconHeight);
	}

	void GroupBox::setIconSize(const UISize& size, UIUpdateMode mode)
	{
		m_iconWidth = size.x;
		m_iconHeight = size.y;
		invalidate(mode);
	}

	void GroupBox::setIconSize(sl_ui_len size, UIUpdateMode mode)
	{
		m_iconWidth = size;
		m_iconHeight = size;
		invalidate(mode);
	}

	void GroupBox::setIconSize(sl_ui_len width, sl_ui_len height, UIUpdateMode mode)
	{
		m_iconWidth = width;
		m_iconHeight = height;
		invalidate(mode);
	}

	sl_ui_len GroupBox::getIconWidth()
	{
		return m_iconWidth;
	}

	void GroupBox::setIconWidth(sl_ui_len width, UIUpdateMode mode)
	{
		m_iconWidth = width;
	}

	sl_ui_len GroupBox::getIconHeight()
	{
		return m_iconHeight;
	}

	void GroupBox::setIconHeight(sl_ui_len height, UIUpdateMode mode)
	{
		m_iconHeight = height;
		invalidate(mode);
	}

	sl_ui_len GroupBox::getIconMarginLeft()
	{
		return m_iconMarginLeft;
	}

	void GroupBox::setIconMarginLeft(sl_ui_len margin, UIUpdateMode mode)
	{
		m_iconMarginLeft = margin;
		invalidate(mode);
	}

	sl_ui_len GroupBox::getIconMarginRight()
	{
		return m_iconMarginRight;
	}

	void GroupBox::setIconMarginRight(sl_ui_len margin, UIUpdateMode mode)
	{
		m_iconMarginRight = margin;
		invalidate(mode);
	}

	void GroupBox::setIconMargin(sl_ui_len margin, UIUpdateMode mode)
	{
		m_iconMarginLeft = margin;
		m_iconMarginRight = margin;
		invalidate(mode);
	}

	void GroupBox::onDrawBorder(Canvas* canvas)
	{
		Ref<Font> font = getLabelFont();
		if (!m_paddingBorder) {
			_updatePaddings(font);
		}
		Rectangle bounds = getBounds();
		sl_real p = (sl_real)(m_paddingBorder / 2);
		sl_real t = (sl_real)(m_paddingTop / 2);
		String label = m_label;
		sl_real widthLabel = 0;
		Ref<Drawable> icon = m_icon;
		sl_real iconWidth = (sl_real)m_iconWidth;
		sl_real iconHeight = (sl_real)m_iconHeight;
		sl_real totalIconWidth;
		if (icon.isNotNull()) {
			if (font.isNotNull()) {
				if (m_iconHeight <= 0) {
					iconHeight = font->getFontHeight();
				}
			}
			if (m_iconWidth <= 0) {
				iconWidth = iconHeight;
			}
			totalIconWidth = (sl_real)m_iconMarginLeft + iconWidth + (sl_real)m_iconMarginRight;
		} else {
			totalIconWidth = 0;
		}
		if (font.isNotNull()) {
			widthLabel = font->getTextAdvance(label).x;
			Rectangle rcText;
			rcText.left = bounds.left + p + t + totalIconWidth;
			rcText.top = bounds.top;
			rcText.right = rcText.left + widthLabel;
			rcText.bottom = rcText.top + t + t;
			CanvasAntiAliasScope scope(canvas, sl_true);
			canvas->drawText(label, rcText, font, m_labelColor, Alignment::MiddleCenter);
		}
		if (icon.isNotNull() && iconHeight > SLIB_EPSILON) {
			Rectangle rcIcon;
			rcIcon.left = bounds.left + p + t + (sl_real)m_iconMarginLeft;
			rcIcon.top = bounds.top + t - iconHeight * 0.5f;
			rcIcon.right = rcIcon.left + iconWidth;
			rcIcon.bottom = rcIcon.top + iconHeight;
			canvas->draw(rcIcon, icon);
		}
		Ref<Pen> pen = getBorder();
		if (pen.isNotNull()) {
			Point pts[] = {
				{ bounds.left + t - p, bounds.top + t },
				{ bounds.left + p, bounds.top + t },
				{ bounds.left + p, bounds.bottom - p },
				{ bounds.right - p, bounds.bottom - p },
				{ bounds.right - p, bounds.top + t },
				{ bounds.left + t + totalIconWidth + widthLabel + 3 * p, bounds.top + t }
			};
			canvas->drawLines(pts, (sl_uint32)(CountOfArray(pts)), pen);
		}
	}

	void GroupBox::onUpdateFont(const Ref<Font>& font)
	{
		if (m_labelFont.isNull()) {
			_updatePaddings(font);
		}
	}

	void GroupBox::_updatePaddings(const Ref<Font>& font)
	{
		sl_ui_len h = (sl_ui_len)(font->getFontHeight() * 1.2);
		m_paddingBorder = 4;
		m_paddingTop = h;
		setPadding(m_paddingBorder, m_paddingTop, m_paddingBorder, m_paddingBorder);
	}

}
