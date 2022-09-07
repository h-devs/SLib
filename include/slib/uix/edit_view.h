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

#ifndef CHECKHEADER_SLIB_UIX_EDIT_VIEW
#define CHECKHEADER_SLIB_UIX_EDIT_VIEW

#include "control.h"

#include "../ui/edit_view.h"

namespace slib
{
	
	class SLIB_EXPORT XEditView : public XControl
	{
		SLIB_DECLARE_OBJECT
		
	public:
		XEditView();

		~XEditView();

	protected:
		void init() override;
		
	public:
		String getText();
		
		void setText(const String& text, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void appendText(const StringParam& text, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isChangeEventEnabled();

		void setChangeEventEnabled(sl_bool flag = sl_true);

		Alignment getGravity();
		
		void setGravity(const Alignment& gravity, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Color getTextColor();
		
		void setTextColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		String getHintText();
		
		void setHintText(const String& str, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Alignment getHintGravity();
		
		void setHintGravity(const Alignment& gravity, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Color getHintTextColor();
		
		void setHintTextColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Ref<Font> getHintFont();
		
		void setHintFont(const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_bool isReadOnly();
		
		void setReadOnly(sl_bool flag, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		sl_bool isPassword();
		
		void setPassword(sl_bool flag, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isNumber();

		void setNumber(sl_bool flag, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isLowercase();

		void setLowercase(sl_bool flag, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isUppercase();

		void setUppercase(sl_bool flag, UIUpdateMode mode = UIUpdateMode::Redraw);

		MultiLineMode getMultiLine();
		
		void setMultiLine(MultiLineMode multiLineMode, UIUpdateMode updateMode = UIUpdateMode::UpdateLayout);

		UIReturnKeyType getReturnKeyType();
		
		void setReturnKeyType(UIReturnKeyType type);
		
		UIKeyboardType getKeyboardType();
		
		void setKeyboardType(UIKeyboardType type);

		UIAutoCapitalizationType getAutoCaptializationType();

		void setAutoCapitalizationType(UIAutoCapitalizationType type);
		
		sl_bool isAutoDismissKeyboard();
		
		void setAutoDismissKeyboard(sl_bool flag);
		
		void setFocusNextOnReturnKey();

		// `start`: negative means non-selection, `end`: negative means `end of text`, In character unit
		void setSelection(sl_reg start, sl_reg end);

		void selectAll();

		void selectNone();

		sl_bool isAutoHorizontalScrolling();

		void setAutoHorizontalScrolling(sl_bool flag = sl_true);

		sl_bool isAutoVerticalScrolling();

		void setAutoVerticalScrolling(sl_bool flag = sl_true);

		void setFocus(sl_bool flagFocused = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw) override;

	public:
		SLIB_DECLARE_EVENT_HANDLER(XEditView, Change, String& value)
		
		SLIB_DECLARE_EVENT_HANDLER(XEditView, PostChange)
		
		SLIB_DECLARE_EVENT_HANDLER(XEditView, ReturnKey)

	protected:
		void onChangeSizeMode(UIUpdateMode mode) override;

	public:
		void dispatchClickEvent(UIEvent* ev) override;
		
	protected:
		Ref<EditView> m_edit;

	};
	
	class XPasswordView : public XEditView
	{
	public:
		XPasswordView();

		~XPasswordView();

	protected:
		void init() override;
		
	};
	
}

#endif
