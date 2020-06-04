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

#ifndef CHECKHEADER_SLIB_UI_LABEL_VIEW
#define CHECKHEADER_SLIB_UI_LABEL_VIEW

#include "definition.h"

#include "view.h"

#include "../graphics/text.h"

namespace slib
{

	template <class VIEW_CLASS>
	class LabelAppearanceViewBase
	{
	public:
		LabelAppearanceViewBase()
		{
			m_textColor = Color::Black;
			m_textAlignment = Alignment::Left;
			m_ellipsizeMode = EllipsizeMode::None;
			m_flagEnabledHyperlinksInPlainText = sl_false;
			m_linkColor = Color::Zero;
		}

	public:
		Color getTextColor()
		{
			return m_textColor;
		}

		void setTextColor(const Color& color, UIUpdateMode updateMode = UIUpdateMode::Redraw)
		{
			m_textColor = color;
			((VIEW_CLASS*)this)->invalidateLabelAppearance(updateMode);
		}

		Alignment getGravity()
		{
			return m_textAlignment;
		}

		void setGravity(const Alignment& align, UIUpdateMode updateMode = UIUpdateMode::Redraw)
		{
			m_textAlignment = align;
			((VIEW_CLASS*)this)->invalidateLabelAppearance(updateMode);
		}

		EllipsizeMode getEllipsize()
		{
			return m_ellipsizeMode;
		}

		void setEllipsize(EllipsizeMode ellipsizeMode, UIUpdateMode updateMode = UIUpdateMode::UpdateLayout)
		{
			m_ellipsizeMode = ellipsizeMode;
			((VIEW_CLASS*)this)->invalidateLabelAppearance(updateMode);
		}

		sl_bool isDetectingHyperlinksInPlainText()
		{
			return m_flagEnabledHyperlinksInPlainText;
		}

		void setDetectingHyperlinksInPlainText(sl_bool flag, UIUpdateMode updateMode = UIUpdateMode::Redraw)
		{
			m_flagEnabledHyperlinksInPlainText = flag;
			((VIEW_CLASS*)this)->invalidateLabelAppearance(updateMode);
		}

		Color getLinkColor()
		{
			if (m_linkColor.isNotZero()) {
				return m_linkColor;
			}
			return TextParagraph::getDefaultLinkColor();
		}

		void setLinkColor(const Color& color, UIUpdateMode updateMode = UIUpdateMode::Redraw)
		{
			m_linkColor = color;
			((VIEW_CLASS*)this)->invalidateLabelAppearance(updateMode);
		}

	protected:
		void _applyLabelAppearance(SimpleTextBoxParam& param)
		{
			param.ellipsizeMode = m_ellipsizeMode;
			param.align = m_textAlignment;
			param.flagEnabledHyperlinksInPlainText = m_flagEnabledHyperlinksInPlainText;
		}

		void _applyLabelAppearance(SimpleTextBoxDrawParam& param)
		{
			param.color = m_textColor;
			param.linkColor = getLinkColor();
		}

	protected:
		Color m_textColor;
		Alignment m_textAlignment;
		EllipsizeMode m_ellipsizeMode;
		sl_bool m_flagEnabledHyperlinksInPlainText;
		Color m_linkColor;

	};

	class SLIB_EXPORT LabelView : public View, public LabelAppearanceViewBase<LabelView>
	{
		SLIB_DECLARE_OBJECT
		
	public:
		LabelView();
		
		~LabelView();
		
	public:
		String getText();
		
		sl_bool isHyperText();
		
		virtual void setText(const String& text, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
		virtual void setHyperText(const String& text, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		MultiLineMode getMultiLine();

		virtual void setMultiLine(MultiLineMode multiLineMode, UIUpdateMode updateMode = UIUpdateMode::UpdateLayout);

		sl_uint32 getLinesCount();

		virtual void setLinesCount(sl_uint32 nLines, UIUpdateMode updateMode = UIUpdateMode::UpdateLayout);

		virtual void invalidateLabelAppearance(UIUpdateMode mode);

		UISize measureSize();

	public:
		SLIB_DECLARE_EVENT_HANDLER(LabelView, ClickLink, const String& href, UIEvent* ev)
		
	protected:
		void _updateTextBox(sl_ui_len width);
		
	protected:
		void onDraw(Canvas* canvas) override;
		
		void onClickEvent(UIEvent* ev) override;
		
		void onSetCursor(UIEvent* ev) override;

		void onUpdateLayout() override;
		
	protected:
		AtomicString m_text;
		sl_bool m_flagHyperText;
		MultiLineMode m_multiLineMode;
		sl_uint32 m_linesCount;

		SimpleTextBox m_textBox;
		
	};

}

#endif
