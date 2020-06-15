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

#ifndef CHECKHEADER_SLIB_UI_LABEL_VIEW_BASE_IMPL
#define CHECKHEADER_SLIB_UI_LABEL_VIEW_BASE_IMPL

namespace slib
{

	template <class VIEW_CLASS>
	LabelAppearanceViewBase<VIEW_CLASS>::LabelAppearanceViewBase()
	{
		m_textColor = Color::Black;
		m_textAlignment = Alignment::Left;
		m_ellipsizeMode = EllipsizeMode::None;
		m_flagEnabledHyperlinksInPlainText = sl_false;
		m_linkColor = Color::Zero;
	}

	template <class VIEW_CLASS>
	Color LabelAppearanceViewBase<VIEW_CLASS>::getTextColor()
	{
		return m_textColor;
	}

	template <class VIEW_CLASS>
	void LabelAppearanceViewBase<VIEW_CLASS>::setTextColor(const Color& color, UIUpdateMode updateMode)
	{
		m_textColor = color;
		((VIEW_CLASS*)this)->invalidateLabelAppearance(updateMode);
	}

	template <class VIEW_CLASS>
	Alignment LabelAppearanceViewBase<VIEW_CLASS>::getGravity()
	{
		return m_textAlignment;
	}

	template <class VIEW_CLASS>
	void LabelAppearanceViewBase<VIEW_CLASS>::setGravity(const Alignment& align, UIUpdateMode updateMode)
	{
		m_textAlignment = align;
		((VIEW_CLASS*)this)->invalidateLabelAppearance(updateMode);
	}

	template <class VIEW_CLASS>
	EllipsizeMode LabelAppearanceViewBase<VIEW_CLASS>::getEllipsize()
	{
		return m_ellipsizeMode;
	}

	template <class VIEW_CLASS>
	void LabelAppearanceViewBase<VIEW_CLASS>::setEllipsize(EllipsizeMode ellipsizeMode, UIUpdateMode updateMode)
	{
		m_ellipsizeMode = ellipsizeMode;
		((VIEW_CLASS*)this)->invalidateLabelAppearance(updateMode);
	}

	template <class VIEW_CLASS>
	sl_bool LabelAppearanceViewBase<VIEW_CLASS>::isDetectingHyperlinksInPlainText()
	{
		return m_flagEnabledHyperlinksInPlainText;
	}

	template <class VIEW_CLASS>
	void LabelAppearanceViewBase<VIEW_CLASS>::setDetectingHyperlinksInPlainText(sl_bool flag, UIUpdateMode updateMode)
	{
		m_flagEnabledHyperlinksInPlainText = flag;
		((VIEW_CLASS*)this)->invalidateLabelAppearance(updateMode);
	}

	template <class VIEW_CLASS>
	Color LabelAppearanceViewBase<VIEW_CLASS>::getLinkColor()
	{
		if (m_linkColor.isNotZero()) {
			return m_linkColor;
		}
		return TextParagraph::getDefaultLinkColor();
	}

	template <class VIEW_CLASS>
	void LabelAppearanceViewBase<VIEW_CLASS>::setLinkColor(const Color& color, UIUpdateMode updateMode)
	{
		m_linkColor = color;
		((VIEW_CLASS*)this)->invalidateLabelAppearance(updateMode);
	}

	template <class VIEW_CLASS>
	void LabelAppearanceViewBase<VIEW_CLASS>::_applyLabelAppearance(SimpleTextBoxParam& param)
	{
		param.ellipsizeMode = m_ellipsizeMode;
		param.align = m_textAlignment;
		param.flagEnabledHyperlinksInPlainText = m_flagEnabledHyperlinksInPlainText;
	}

	template <class VIEW_CLASS>
	void LabelAppearanceViewBase<VIEW_CLASS>::_applyLabelAppearance(SimpleTextBoxDrawParam& param)
	{
		param.color = m_textColor;
		param.linkColor = getLinkColor();
	}

}

#endif