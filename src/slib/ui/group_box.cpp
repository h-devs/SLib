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

	void GroupBox::onDrawBorder(Canvas* canvas)
	{
		Rectangle bounds = getBounds();
		sl_real p = (sl_real)(m_paddingBorder / 2);
		sl_real t = (sl_real)(m_paddingTop / 2);
		String label = m_label;
		sl_real widthLabel = 0;
		Ref<Font> font = getFont();
		if (font.isNotNull()) {
			widthLabel = font->measureText(label).x;
			canvas->drawText(label, Rectangle(bounds.left + p + t, bounds.top, bounds.left + p + t + widthLabel, bounds.top + t * 2), font, m_labelColor, Alignment::MiddleCenter);
		}
		Ref<Pen> pen = getBorder();
		if (pen.isNotNull()) {
			Point pts[] = {
				{ bounds.left + t - p, bounds.top + t },
				{ bounds.left + p, bounds.top + t },
				{ bounds.left + p, bounds.bottom - p },
				{ bounds.right - p, bounds.bottom - p },
				{ bounds.right - p, bounds.top + t },
				{ bounds.left + t + widthLabel + 3 * p, bounds.top + t }
			};
			canvas->drawLines(pts, (sl_uint32)(CountOfArray(pts)), pen);
		}
	}

	void GroupBox::onUpdateFont(const Ref<Font>& font)
	{
		_updatePaddings(font);
	}

	void GroupBox::_updatePaddings(const Ref<Font>& font)
	{
		sl_ui_len h = (sl_ui_len)(font->getFontHeight() * 1.2);
		m_paddingBorder = 4;
		m_paddingTop = h;
		setPadding(m_paddingBorder, m_paddingTop, m_paddingBorder, m_paddingBorder);
	}

}
