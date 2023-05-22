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

#include "slib/ui/text.h"

namespace slib
{

	TextInput::TextInput()
	{
		m_selectedRange.location = 0;
	}

	TextInput::~TextInput()
	{
	}

	TextRange TextInput::getSelectedRange()
	{
		return m_selectedRange;
	}

	void TextInput::setSelectedRange(const TextRange& range)
	{
		if (m_selectedRange != range) {
			m_selectedRange = range;
			onChangeSelectedRange();
		}
	}

	TextRange TextInput::getMarkedRange()
	{
		return m_markedRange;
	}

	void TextInput::setMarkedRange(const TextRange& range)
	{
		if (m_markedRange != range) {
			m_markedRange = range;
			onChangeMarkedRange();
		}
	}

	void TextInput::replaceText(const TextRange& range, const StringView32& text)
	{
	}

	void TextInput::onChangeMarkedRange()
	{
	}

	void TextInput::onChangeSelectedRange()
	{
	}


	SLIB_DEFINE_OBJECT(UITextBox, TextBox)

	UITextBox::UITextBox()
	{
	}

	UITextBox::~UITextBox()
	{
	}

	String32 UITextBox::getTextInRange(const TextRange& range)
	{
		return sl_null;
	}

	sl_text_pos UITextBox::getPositionAtPoint(const Point& pt)
	{
		return 0;
	}

	UIRect UITextBox::getFirstRectangleForRange(const TextRange& range)
	{
		return UIRect::zero();
	}

	List<UIRect> UITextBox::getRectanglesForRange(const TextRange& range)
	{
		return sl_null;
	}

	UIRect UITextBox::getCaretRectangleForPosition(sl_text_pos pos)
	{
		return UIRect::zero();
	}

	sl_text_pos UITextBox::getClosestPositionToPoint(const Point& pt, const TextRange& range)
	{
		return 0;
	}

	void UITextBox::onReplaceText(const TextRange& range, const StringView32& text)
	{
	}

	void UITextBox::onChangeMarkedRange()
	{
	}

	void UITextBox::onChangeSelectedRange()
	{
	}

}
