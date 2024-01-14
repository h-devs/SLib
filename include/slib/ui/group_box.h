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

#ifndef CHECKHEADER_SLIB_UI_GROUP_BOX
#define CHECKHEADER_SLIB_UI_GROUP_BOX

#include "view.h"

#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT GroupBox : public ViewGroup
	{
		SLIB_DECLARE_OBJECT

	public:
		GroupBox();

		~GroupBox();

	public:
		String getLabel();

		void setLabel(const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);

		Color getLabelColor();

		void setLabelColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Font> getLabelFont();

		void setLabelFont(const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setLabelFont(const FontDesc& desc, UIUpdateMode mode = UIUpdateMode::Redraw);

	protected:
		void onDrawBorder(Canvas* canvas) override;

		void onUpdateFont(const Ref<Font>& font) override;

	protected:
		void _updatePaddings(const Ref<Font>& font);

	protected:
		AtomicString m_label;
		Color m_labelColor;
		AtomicRef<Font> m_labelFont;

		sl_ui_len m_paddingBorder;
		sl_ui_len m_paddingTop;

	};

}

#endif
