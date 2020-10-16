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

#ifndef CHECKHEADER_SLIB_UI_EDIT_VIEW
#define CHECKHEADER_SLIB_UI_EDIT_VIEW

#include "definition.h"

#include "view.h"

namespace slib
{
	
	class IEditViewInstance;

	class SLIB_EXPORT EditView : public View
	{
		SLIB_DECLARE_OBJECT
		
	public:
		EditView();

		~EditView();
		
	public:
		String getText();
		
		String getInstanceText();

		void setText(const String& text, UIUpdateMode mode = UIUpdateMode::UpdateLayout);
		
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
		
	public:
		SLIB_DECLARE_EVENT_HANDLER(EditView, Change, String& value)
		
		SLIB_DECLARE_EVENT_HANDLER(EditView, ReturnKey)
		
	protected:
		void onUpdateLayout() override;
		
		void onDraw(Canvas* canvas) override;

		void onClickEvent(UIEvent* ev) override;
		
	protected:
		Ref<ViewInstance> createNativeWidget(ViewInstance* parent) override;
		
		virtual Ptr<IEditViewInstance> getEditViewInstance();
		
	public:
		void dispatchKeyEvent(UIEvent* ev) override;
		
	protected:
		AtomicString m_text;
		Alignment m_gravity;
		Color m_textColor;
		AtomicString m_hintText;
		Alignment m_hintGravity;
		Color m_hintTextColor;
		AtomicRef<Font> m_hintFont;
		sl_bool m_flagReadOnly;
		sl_bool m_flagPassword;
		MultiLineMode m_multiLine;
		UIReturnKeyType m_returnKeyType;
		UIKeyboardType m_keyboardType;
		UIAutoCapitalizationType m_autoCapitalizationType;
		sl_bool m_flagAutoDismissKeyboard;
		
		Ref<Referable> m_dialog;

	};
	
	class PasswordView : public EditView
	{
	public:
		PasswordView();
		
	};
	
	class TextArea : public EditView
	{
		SLIB_DECLARE_OBJECT
		
	public:
		TextArea();
		
		~TextArea();

	protected:
		Ref<ViewInstance> createNativeWidget(ViewInstance* parent) override;
		
		Ptr<IEditViewInstance> getEditViewInstance() override;
		
	};

	class SLIB_EXPORT IEditViewInstance
	{
	public:
		virtual sl_bool getText(EditView* view, String& _out) = 0;
		
		virtual void setText(EditView* view, const String& text) = 0;
		
		virtual void setGravity(EditView* view, const Alignment& gravity) = 0;
		
		virtual void setTextColor(EditView* view, const Color& color) = 0;
		
		virtual void setHintText(EditView* view, const String& text) = 0;
		
		virtual void setHintGravity(EditView* view, const Alignment& gravity) = 0;
		
		virtual void setHintTextColor(EditView* view, const Color& color) = 0;
		
		virtual void setHintFont(EditView* view, const Ref<Font>& font) = 0;
		
		virtual void setReadOnly(EditView* view, sl_bool flag) = 0;
		
		virtual void setPassword(EditView* view, sl_bool flag) = 0;
		
		virtual void setMultiLine(EditView* view, MultiLineMode mode) = 0;
		
		virtual void setReturnKeyType(EditView* view, UIReturnKeyType type);
		
		virtual void setKeyboardType(EditView* view, UIKeyboardType type);
		
		virtual void setAutoCapitalizationType(EditView* view, UIAutoCapitalizationType type);
		
		virtual sl_ui_len measureHeight(EditView* view) = 0;
		
	};

}

#endif
