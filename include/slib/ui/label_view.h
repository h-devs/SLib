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

#ifndef CHECKHEADER_SLIB_UI_LABEL_VIEW
#define CHECKHEADER_SLIB_UI_LABEL_VIEW

#include "definition.h"

#include "view.h"

#include "../graphics/text.h"

namespace slib
{

	class LabelViewCell;

	class SLIB_EXPORT LabelView : public View
	{
		SLIB_DECLARE_OBJECT
		
	public:
		LabelView();
		
		~LabelView();

	protected:
		void init() override;
		
	public:
		String getText();
		
		sl_bool isHyperText();
		
		void setText(const String& text, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		void setHyperText(const String& text, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		MultiLineMode getMultiLine();

		void setMultiLine(MultiLineMode multiLineMode, UIUpdateMode updateMode = UIUpdateMode::UpdateLayout);

		sl_uint32 getLinesCount();

		void setLinesCount(sl_uint32 nLines, UIUpdateMode updateMode = UIUpdateMode::UpdateLayout);

		sl_bool isMnemonic();

		// call before `setText()`
		void setMnemonic(sl_bool flag);


		Color getTextColor();

		void setTextColor(const Color& color, UIUpdateMode updateMode = UIUpdateMode::Redraw);

		Alignment getGravity();

		void setGravity(const Alignment& align, UIUpdateMode updateMode = UIUpdateMode::Redraw);

		EllipsizeMode getEllipsize();

		void setEllipsize(EllipsizeMode ellipsizeMode, UIUpdateMode updateMode = UIUpdateMode::UpdateLayout);

		sl_bool isDetectingHyperlinksInPlainText();

		void setDetectingHyperlinksInPlainText(sl_bool flag, UIUpdateMode updateMode = UIUpdateMode::Redraw);

		Color getLinkColor();

		void setLinkColor(const Color& color, UIUpdateMode updateMode = UIUpdateMode::Redraw);


		UISize measureSize();

	public:
		SLIB_DECLARE_EVENT_HANDLER(LabelView, ClickLink, const String& href, UIEvent* ev)
		
	protected:
		void onDraw(Canvas* canvas) override;
		
		void onClickEvent(UIEvent* ev) override;
		
		void onSetCursor(UIEvent* ev) override;

		void onUpdateLayout() override;
		
	protected:
		void prepareLabelViewCellLayout(LabelViewCell* cell);

	protected:
		Ref<LabelViewCell> m_cell;
		
	};

	class LabelViewCell : public ViewCell
	{
		SLIB_DECLARE_OBJECT

	public:
		AtomicString text;
		sl_bool flagHyperText;
		MultiLineMode multiLineMode;
		sl_uint32 linesCount;
		sl_bool flagMnemonic;

		Color textColor;
		Alignment gravity;
		EllipsizeMode ellipsizeMode;
		sl_bool flagEnabledHyperlinksInPlainText;
		Color linkColor;

		sl_real shadowOpacity;
		sl_real shadowRadius;
		Color shadowColor;
		Point shadowOffset;

		sl_bool flagWrapping;
		sl_ui_len maxWidth;

		Function<void(const String& href, UIEvent* ev)> onClickLink;

	public:
		LabelViewCell();

		~LabelViewCell();

	public:
		UISize measureSize();

	public:
		void onDraw(Canvas* canvas) override;

		void onClickEvent(UIEvent* ev) override;

		void onSetCursor(UIEvent* ev) override;

		void onMeasure(UISize& size, sl_bool flagHorizontalWrapping, sl_bool flagVerticalWrapping) override;

	protected:
		void _updateTextBox(sl_ui_len width);

		void _updateTextBox(sl_bool flagWrapping, sl_ui_len width, sl_ui_len padding, const Alignment& align);

	protected:
		SimpleTextBox m_textBox;

	};

}

#endif
