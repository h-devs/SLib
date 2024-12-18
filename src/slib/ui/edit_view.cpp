/*
 *   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/edit_view.h"

#include "slib/ui/text.h"
#include "slib/ui/core.h"
#include "slib/ui/mobile_app.h"
#include "slib/ui/resource.h"
#include "slib/ui/button.h"
#include "slib/ui/cursor.h"
#include "slib/core/timer.h"
#include "slib/core/stringx.h"

#if defined(SLIB_UI)
#	define HAS_NATIVE_WIDGET_IMPL 1
#else
#	define HAS_NATIVE_WIDGET_IMPL 0
#endif

namespace slib
{

	SLIB_DEFINE_OBJECT(EditView, View)

	EditView::EditView()
	{
		setSupportedNativeWidget(HAS_NATIVE_WIDGET_IMPL);
		setCreatingNativeWidget(HAS_NATIVE_WIDGET_IMPL);

		setUsingFont();
		setFocusable();
		setUsingIME();

		setBorder(sl_true, UIUpdateMode::Init);
		setBackgroundColor(Color::White, UIUpdateMode::Init);
		setPadding((sl_ui_pos)(UI::dpToPixel(2)), UIUpdateMode::Init);

		m_flagInvalidateText = sl_false;
		m_flagChangeEvent = sl_true;
		m_gravity = Alignment::Default;
		m_textColor = Color::Black;
		m_hintGravity = Alignment::Default;
		m_hintTextColor = Color(150, 150, 150);
		m_flagReadOnly = sl_false;
		m_flagPassword = sl_false;
		m_flagLowercase = sl_false;
		m_flagUppercase = sl_false;
		m_multiLine = MultiLineMode::Single;
		m_returnKeyType = UIReturnKeyType::Default;
		m_keyboardType = UIKeyboardType::Default;
		m_autoCapitalizationType = UIAutoCapitalizationType::None;
		m_flagAutoDismissKeyboard = sl_true;
		m_flagAutoHorizontalScrolling = sl_true;
		m_flagAutoVerticalScrolling = sl_true;
		m_flagUsingPopup = sl_false;

		m_indexSelectionStart = 0;
		m_indexSelectionEnd = -1;

		m_nCountDrawCaret = 0;

	}

	EditView::~EditView()
	{
	}

	String EditView::getText()
	{
		if (m_flagInvalidateText) {
			return getInstanceText();
		} else {
			return m_text;
		}
	}

	String EditView::getInstanceText()
	{
		m_flagInvalidateText = sl_false;
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			if (UI::isUiThread()) {
				String text;
				if (instance->getText(this, text)) {
					m_text = Move(text);
				}
			}
		}
		return m_text;
	}

	void EditView::setText(const String& text, UIUpdateMode mode)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setText, text, mode)
		}
		String _text = text;
		_change(instance.get(), _text, sl_null, mode);
	}

	void EditView::appendText(const StringParam& text, UIUpdateMode mode)
	{
		if (text.isEmpty()) {
			return;
		}
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(appendText, text.toString(), mode)
			if (instance->appendText(this, text)) {
				m_flagInvalidateText = sl_true;
				return;
			}
		}
		setText(String::concat(getText(), text), mode);
	}

	sl_bool EditView::isChangeEventEnabled()
	{
#ifndef SLIB_PLATFORM_IS_WIN32
		if (m_flagUppercase || m_flagLowercase) {
			return sl_true;
		}
#endif
		return m_flagChangeEvent;
	}

	void EditView::setChangeEventEnabled(sl_bool flag)
	{
		m_flagChangeEvent = flag;
	}

	void EditView::invalidateText()
	{
		m_flagInvalidateText = sl_true;
	}

	Alignment EditView::getGravity()
	{
		return m_gravity;
	}

	void EditView::setGravity(const Alignment& gravity, UIUpdateMode mode)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setGravity, gravity, mode)
			m_gravity = gravity;
			instance->setGravity(this, gravity);
		} else {
			m_gravity = gravity;
			invalidate(mode);
		}
	}

	Color EditView::getTextColor()
	{
		return m_textColor;
	}

	void EditView::setTextColor(const Color& color, UIUpdateMode mode)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setTextColor, color, mode)
			m_textColor = color;
			instance->setTextColor(this, color);
		} else {
			m_textColor = color;
			invalidate(mode);
		}
	}

	String EditView::getHintText()
	{
		return m_hintText;
	}

	void EditView::setHintText(const String& str, UIUpdateMode mode)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setHintText, str, mode)
			m_hintText = str;
			instance->setHintText(this, str);
		} else {
			m_hintText = str;
			invalidate(mode);
		}
	}

	Alignment EditView::getHintGravity()
	{
		return m_hintGravity;
	}

	void EditView::setHintGravity(const Alignment& gravity, UIUpdateMode mode)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setHintGravity, gravity, mode)
			m_hintGravity = gravity;
			instance->setHintGravity(this, gravity);
		} else {
			m_hintGravity = gravity;
			invalidate(mode);
		}
	}

	Color EditView::getHintTextColor()
	{
		return m_hintTextColor;
	}

	void EditView::setHintTextColor(const Color& color, UIUpdateMode mode)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setHintTextColor, color, mode)
			m_hintTextColor = color;
			instance->setHintTextColor(this, color);
		} else {
			m_hintTextColor = color;
			invalidate(mode);
		}
	}

	Ref<Font> EditView::getHintFont()
	{
		Ref<Font> font = m_hintFont;
		if (font.isNotNull()) {
			return font;
		}
		return getFont();
	}

	void EditView::setHintFont(const Ref<Font>& font, UIUpdateMode mode)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			void (EditView::*func)(const Ref<Font>&, UIUpdateMode) = &EditView::setHintFont;
			SLIB_VIEW_RUN_ON_UI_THREAD2(func, font, mode)
			m_hintFont = font;
			instance->setHintFont(this, getHintFont());
		} else {
			m_hintFont = font;
			invalidate(mode);
		}
	}

	void EditView::setHintFont(const FontDesc& desc, UIUpdateMode mode)
	{
		setHintFont(Font::create(desc, getHintFont()), mode);
	}

	sl_bool EditView::isReadOnly()
	{
		return m_flagReadOnly;
	}

	void EditView::setReadOnly(sl_bool flag, UIUpdateMode mode)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setReadOnly, flag, mode)
			m_flagReadOnly = flag;
			instance->setReadOnly(this, flag);
		} else {
			m_flagReadOnly = flag;
			invalidate(mode);
		}
	}

	sl_bool EditView::isPassword()
	{
		return m_flagPassword;
	}

	void EditView::setPassword(sl_bool flag, UIUpdateMode mode)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setPassword, flag, mode)
			m_flagPassword = flag;
			instance->setPassword(this, flag);
		} else {
			m_flagPassword = flag;
			invalidate(mode);
		}
		setUsingIME(!flag);
	}

	sl_bool EditView::isNumber()
	{
		return getKeyboardType() == UIKeyboardType::Numpad;
	}

	void EditView::setNumber(sl_bool flag, UIUpdateMode mode)
	{
		setKeyboardType(UIKeyboardType::Numpad);
		invalidate(mode);
	}

	sl_bool EditView::isLowercase()
	{
		return m_flagLowercase;
	}

	void EditView::setLowercase(sl_bool flag, UIUpdateMode mode)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setLowercase, flag, mode)
			m_flagLowercase = flag;
			instance->setLowercase(this, flag);
		} else {
			m_flagLowercase = flag;
			invalidate(mode);
		}
	}

	sl_bool EditView::isUppercase()
	{
		return m_flagUppercase;
	}

	void EditView::setUppercase(sl_bool flag, UIUpdateMode mode)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setUppercase, flag, mode)
			m_flagUppercase = flag;
			instance->setUppercase(this, flag);
		} else {
			m_flagUppercase = flag;
			invalidate(mode);
		}
	}

	MultiLineMode EditView::getMultiLine()
	{
		return m_multiLine;
	}

	void EditView::setMultiLine(MultiLineMode multiLineMode, UIUpdateMode updateMode)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setMultiLine, multiLineMode, updateMode)
			m_multiLine = multiLineMode;
			instance->setMultiLine(this, multiLineMode);
		} else {
			m_multiLine = multiLineMode;
			invalidate(updateMode);
		}
	}

	UIReturnKeyType EditView::getReturnKeyType()
	{
		return m_returnKeyType;
	}

	void EditView::setReturnKeyType(UIReturnKeyType type)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setReturnKeyType, type)
			m_returnKeyType = type;
			instance->setReturnKeyType(this, type);
		} else {
			m_returnKeyType = type;
		}
	}

	UIKeyboardType EditView::getKeyboardType()
	{
		return m_keyboardType;
	}

	void EditView::setKeyboardType(UIKeyboardType type)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setKeyboardType, type)
			m_keyboardType = type;
			instance->setKeyboardType(this, type);
		} else {
			m_keyboardType = type;
		}
	}

	UIAutoCapitalizationType EditView::getAutoCaptializationType()
	{
		return m_autoCapitalizationType;
	}

	void EditView::setAutoCapitalizationType(UIAutoCapitalizationType type)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setAutoCapitalizationType, type)
			m_autoCapitalizationType = type;
			instance->setAutoCapitalizationType(this, type);
		} else {
			m_autoCapitalizationType = type;
		}
	}

	sl_bool EditView::isAutoDismissKeyboard()
	{
		return m_flagAutoDismissKeyboard;
	}

	void EditView::setAutoDismissKeyboard(sl_bool flag)
	{
		m_flagAutoDismissKeyboard = flag;
	}

	void EditView::setFocusNextOnReturnKey()
	{
		setOnReturnKey([](EditView* view) {
			Ref<View> next = view->getNextTabStop();
			if (next.isNotNull()) {
				next->setFocus();
			}
		});
	}

	void EditView::setSelection(sl_reg start, sl_reg end)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setSelection, start, end)
			m_indexSelectionStart = start;
			m_indexSelectionEnd = end;
			instance->setSelection(this, start, end);
		} else {
			m_indexSelectionStart = start;
			m_indexSelectionEnd = end;
		}
	}

	void EditView::selectAll()
	{
		setSelection(0, -1);
	}

	void EditView::selectNone()
	{
		setSelection(-1, 0);
	}

	sl_reg EditView::getRawSelectionStart()
	{
		return m_indexSelectionStart;
	}

	sl_reg EditView::getRawSelectionEnd()
	{
		return m_indexSelectionEnd;
	}

	sl_bool EditView::isAutoHorizontalScrolling()
	{
		return m_flagAutoHorizontalScrolling;
	}

	void EditView::setAutoHorizontalScrolling(sl_bool flag)
	{
		m_flagAutoHorizontalScrolling = flag;
	}

	sl_bool EditView::isAutoVerticalScrolling()
	{
		return m_flagAutoVerticalScrolling;
	}

	void EditView::setAutoVerticalScrolling(sl_bool flag)
	{
		m_flagAutoVerticalScrolling = flag;
	}

	sl_bool EditView::isUsingPopup()
	{
		return m_flagUsingPopup;
	}

	void EditView::setUsingPopup(sl_bool flag)
	{
		m_flagUsingPopup = flag;
	}

	TextInput* EditView::getTextInput(Matrix3* outTransform)
	{
		if (m_textBox.isNotNull()) {
			if (outTransform) {
				*outTransform = Matrix3::identity();
			}
			return m_textBox.get();
		} else {
			return sl_null;
		}
	}

	void EditView::onUpdateLayout()
	{
		sl_bool flagHorizontalWrapping = isLastWidthWrapping();
		sl_bool flagVerticalWrapping = isLastHeightWrapping();

		if (!flagHorizontalWrapping && !flagVerticalWrapping) {
			return;
		}

		Ref<Font> font = getFont();
		if (flagHorizontalWrapping) {
			sl_ui_pos width = getPaddingLeft() + getPaddingRight();
			if (font.isNotNull()) {
				sl_ui_pos t = (sl_ui_pos)(font->getFontHeight());
				if (t > 0) {
					width += t * 4;
				}
			}
			if (width < 0) {
				width = 0;
			}
			setLayoutWidth(width);
		}
		if (flagVerticalWrapping) {
			sl_ui_pos height = 0;
			do {
				Ptr<IEditViewInstance> instance = getEditViewInstance();
				if (instance.isNotNull()) {
					height = instance->measureHeight(this);
					if (height > 0) {
						break;
					}
				}
				if (font.isNotNull()) {
					height = (sl_ui_pos)(font->getFontHeight() * 1.5f);
					if (height < 0) {
						height = 0;
					}
				}
				height += getPaddingTop() + getPaddingBottom();
				if (height < 0) {
					height = 0;
				}
			} while (0);
			setLayoutHeight(height);
		}
	}

	void EditView::onDraw(Canvas* canvas)
	{
		String text(m_text);
		if (text.isEmpty()) {
			// Draw hint text
			text = m_hintText;
			if (text.isEmpty()) {
				return;
			}
			canvas->drawText(text, getBoundsInnerPadding(), getHintFont(), m_hintTextColor, m_hintGravity);
			return;
		}

		Ref<UITextBox>& box = m_textBox;
		if (box.isNull()) {
			return;
		}
		TextBox::DrawParam param;
		param.frame = getBoundsInnerPadding();
		param.textColor = m_textColor;
		box->draw(canvas, param);
	}

	namespace
	{

		class EditViewHelper : public EditView
		{
		public:
			void closeDialog()
			{
				m_popup.setNull();
			}

		};

		class EditDialog : public CRef
		{
		private:
			WeakRef<EditViewHelper> m_view;
			Ref<Window> m_window;
			Ref<EditView> m_edit;

		public:
			static Ref<EditDialog> open(const Ref<EditView>& view)
			{
				if (view.isNotNull()) {
					Ref<EditDialog> ret = new EditDialog;
					if (ret.isNotNull()) {
						if (ret->_initialize(view)) {
							return ret;
						}
					}
				}
				return sl_null;
			}

			sl_bool _initialize(const Ref<EditView>& view)
			{
				Ref<Window> window = new Window;
				if (window.isNull()) {
					return sl_false;
				}
				window->setBackgroundColor(Color::White);
				Ref<EditView> edit;
				if (view->isPassword()) {
					edit = new PasswordView;
				} else {
#if defined(SLIB_UI_IS_IOS)
					edit = new TextArea;
#else
					edit = new EditView;
#endif
				}
				if (edit.isNull()) {
					return sl_false;
				}
				edit->setText(view->getText(), UIUpdateMode::Init);
				edit->setWidthFilling(1, UIUpdateMode::Init);
				edit->setHeightFilling(1, UIUpdateMode::Init);
#if defined(SLIB_PLATFORM_IS_DESKTOP)
				edit->setFont(view->getFont(), UIUpdateMode::Init);
#else
				edit->setMargin(UIResource::getScreenMinimum() / 20, UIUpdateMode::Init);
				edit->setFont(Font::create(view->getFontFamily(), (sl_real)(UIResource::getScreenMinimum() / 20)), UIUpdateMode::Init);
#endif
				edit->setBorder(sl_false, UIUpdateMode::Init);
				edit->setGravity(Alignment::TopLeft, UIUpdateMode::Init);
				edit->setMultiLine(view->getMultiLine(), UIUpdateMode::Init);
				edit->setOnChanging(SLIB_FUNCTION_WEAKREF(this, _onChanging));
				edit->setOnChange(SLIB_FUNCTION_WEAKREF(this, _onChange));
				edit->setOnPostChange(SLIB_FUNCTION_WEAKREF(this, _onPostChange));
				edit->setOnReturnKey(SLIB_FUNCTION_WEAKREF(this, _onReturnKey));
				UIReturnKeyType returnKeyType = view->getReturnKeyType();
				MultiLineMode multiLineMode = view->getMultiLine();
				if (returnKeyType == UIReturnKeyType::Default && multiLineMode == MultiLineMode::Single) {
					edit->setReturnKeyType(UIReturnKeyType::Done);
				} else {
					edit->setReturnKeyType(returnKeyType);
				}
				edit->setKeyboardType(view->getKeyboardType());
				edit->setAutoCapitalizationType(view->getAutoCaptializationType());
				window->addView(edit, UIUpdateMode::Init);
				window->setOnClose(SLIB_FUNCTION_WEAKREF(this, _onClose));
				edit->setFocus(sl_true, UIUpdateMode::Init);

#if defined(SLIB_UI_IS_IOS)
				sl_bool flagDoneButton = multiLineMode != MultiLineMode::Single;
#else
				sl_bool flagDoneButton = sl_true;
#endif
#if defined(SLIB_UI_IS_ANDROID)
				UI::dispatchToUiThread([] {
					UI::showKeyboard();
				}, 500);
#endif
				if (flagDoneButton) {
					Ref<Button> btnDone = new Button;
					if (btnDone.isNull()) {
						return sl_false;
					}
					btnDone->setText("Done", UIUpdateMode::Init);
					btnDone->setAlignParentRight(UIUpdateMode::Init);
					btnDone->setOnClick(SLIB_FUNCTION_WEAKREF(this, _onDone));
#if defined(SLIB_PLATFORM_IS_DESKTOP)
					edit->setLeftOf(btnDone, UIUpdateMode::Init);
					btnDone->setWidthWrapping(UIUpdateMode::Init);
					btnDone->setHeightWrapping(UIUpdateMode::Init);
					btnDone->setPaddingLeft(10, UIUpdateMode::Init);
					btnDone->setPaddingRight(10, UIUpdateMode::Init);
					btnDone->setCreatingNativeWidget(sl_true);
#else
					sl_ui_pos sw = UIResource::getScreenMinimum();
					edit->setMarginRight(sw / 5 - sw / 20, UIUpdateMode::Init);
					btnDone->setWidth(sw / 5, UIUpdateMode::Init);
					btnDone->setMargin(sw / 20, UIUpdateMode::Init);
					btnDone->setMarginRight(sw / 40, UIUpdateMode::Init);
					btnDone->setHeight(sw / 10, UIUpdateMode::Init);
					btnDone->setFont(Font::create(view->getFontFamily(), (sl_real)(sw / 20)), UIUpdateMode::Init);
#endif
					window->addView(btnDone, UIUpdateMode::Init);
				}

				m_window = window;
				m_edit = edit;
				m_view = Ref<EditViewHelper>::cast(view);

#if defined(SLIB_PLATFORM_IS_DESKTOP)
				window->setParent(view->getWindow());
				window->setCenterScreen(sl_true);
				window->setWidth(UI::getScreenWidth() / 2);
				window->setHeight(UI::getScreenHeight() / 2);
				window->showModal();
#else
				window->create();
#endif
				return sl_true;
			}

			void _onChanging(EditView*, String& text, UIEvent* ev)
			{
				Ref<EditViewHelper> view = m_view;
				if (view.isNull()) {
					return;
				}
				view->invokeChanging(text, ev);
				if (view->getMultiLine() == MultiLineMode::Single) {
					sl_reg index = Stringx::indexOfLine(text);
					if (index >= 0) {
						text = text.mid(0, index);
					}
				}
			}

			void _onChange(EditView*, const String& text, UIEvent* ev)
			{
				Ref<EditViewHelper> view = m_view;
				if (view.isNull()) {
					return;
				}
				view->invokeChange(text, ev);
			}

			void _onPostChange(EditView*)
			{
				Ref<EditViewHelper> view = m_view;
				if (view.isNull()) {
					return;
				}
				view->invokePostChange();
			}

			void _onReturnKey(EditView* ev)
			{
				Ref<EditViewHelper> view = m_view;
				if (view.isNull()) {
					return;
				}
				if (m_edit->getMultiLine() == MultiLineMode::Single) {
					_onDone(sl_null);
				}
				view->invokeReturnKey();
			}

			void _onDone(View* v)
			{
				Ref<EditViewHelper> view = m_view;
				if (view.isNull()) {
					return;
				}
				m_window->close();
				view->invalidate();
				view->closeDialog();
#if defined(SLIB_PLATFORM_IS_ANDROID)
				UI::dismissKeyboard();
#endif
			}

			void _onClose(Window* window)
			{
				window->onClose();
				Ref<EditViewHelper> view = m_view;
				if (view.isNull()) {
					return;
				}
				view->invalidate();
				_onDone(sl_null);
				view->invokeReturnKey();
			}

		};

	}

	void EditView::onClickEvent(UIEvent* ev)
	{
		View::onClickEvent(ev);

		if (m_flagReadOnly) {
			return;
		}
		if (isNativeWidget()) {
			return;
		}
		if (m_flagUsingPopup) {
			if (m_popup.isNull()) {
				m_popup = EditDialog::open(this);
			}
		} else {
			_initTextBox();
			setFocus();
		}
	}

	void EditView::onChangeFocus(sl_bool flagFocused)
	{
		if (isNativeWidget()) {
			return;
		}
		if (flagFocused) {
			if (m_timerDrawCaret.isNull()) {
				WeakRef<EditView> thiz = ToWeakRef(this);
				m_timerDrawCaret = startTimer([this, thiz](Timer*) {
					Ref<EditView> ref = thiz;
					if (ref.isNull()) {
						return;
					}
					m_nCountDrawCaret++;
					invalidate();
				}, 500);
			}
			m_nCountDrawCaret = 0;
			invalidate();
		} else {
			m_timerDrawCaret.setNull();
		}
	}

	void EditView::onSetCursor(UIEvent* ev)
	{
		if (isNativeWidget()) {
			return;
		}
		ev->setCursor(Cursor::getIBeam());
		ev->accept();
	}

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(EditView, Changing, (String& value, UIEvent* ev /* nullable */), value, ev)

	void EditView::onChanging(String& value, UIEvent* ev)
	{
#ifndef SLIB_PLATFORM_IS_WIN32
		if (m_flagUppercase) {
			value = value.toUpper();
		} else if (m_flagLowercase) {
			value = value.toLower();
		}
#endif
	}

	SLIB_DEFINE_EVENT_HANDLER(EditView, Change, (const String& value, UIEvent* ev /* nullable */), value, ev)

	void EditView::_initTextBox()
	{
		ObjectLocker lock(this);
		if (m_textBox.isNull()) {
			if (getFont().isNull()) {
				return;
			}
			Ref<UITextBox> box = new UITextBox;
			if (box.isNull()) {
				return;
			}
			m_textBox = Move(box);
			_updateTextBox();
		}
	}

	void EditView::_updateTextBox()
	{
		ObjectLocker lock(this);
		Ref<UITextBox>& box = m_textBox;
		if (box.isNull()) {
			return;
		}
		TextBoxParam param;
		param.font = getFont();
		if (param.font.isNull()) {
			return;
		}
		param.text = m_text;
		param.align = m_gravity;
		param.multiLineMode = m_multiLine;
		param.width = (sl_real)(getBoundsInnerPadding().getWidth());
		box->update(param);
	}

	void EditView::_change(IEditViewInstance* instance, String& text, UIEvent* ev, UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		if (m_flagInvalidateText) {
			m_flagInvalidateText = sl_false;
			invokeChanging(text, ev);
		} else {
			if (text == m_text) {
				return;
			}
			invokeChanging(text, ev);
			if (text == m_text) {
				return;
			}
		}
		m_text = text;
		if (instance) {
			if (!ev) {
				instance->setText(this, text);
			}
			if (isHeightWrapping()) {
				invalidateLayoutOfWrappingControl(mode);
			}
		} else {
			if (isHeightWrapping()) {
				invalidateLayoutOfWrappingControl(mode);
			} else {
				invalidate(mode);
			}
		}
		lock.unlock();
		invokeChange(text, ev);
		if (ev && !instance) {
			invokePostChange();
		}
	}

	void EditView::_onChange_NW(IEditViewInstance* instance, String& text)
	{
		Ref<UIEvent> ev = UIEvent::createUnknown(Time::now());
		if (ev.isNotNull()) {
			_change(instance, text, ev.get());
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(EditView, PostChange, ())

	void EditView::_onPostChange_NW()
	{
		invokePostChange();
		invalidateLayoutOfWrappingControl();
	}

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(EditView, ReturnKey, ())

	void EditView::onReturnKey()
	{
		if (m_multiLine == MultiLineMode::Single && m_flagAutoDismissKeyboard) {
			UI::dismissKeyboard();
		}
	}

	void EditView::onKeyEvent(UIEvent* ev)
	{
		UIAction action = ev->getAction();
		Keycode keycode = ev->getKeycode();
		if (m_multiLine == MultiLineMode::Single || keycode == Keycode::Escape || m_flagReadOnly) {
			if (action == UIAction::KeyDown) {
				if (keycode == Keycode::Enter || keycode == Keycode::NumpadEnter) {
					invokeReturnKey();
					View::onKeyEvent(ev);
					return;
				}
			}
			View::onKeyEvent(ev);
		}
		if (ev->isAccepted()) {
			return;
		}
		if (isNativeWidget()) {
			if (action == UIAction::KeyDown) {
#ifdef SLIB_PLATFORM_IS_WIN32
				// On Win32, Readonly Edit Control does not support Ctrl+A
				if (m_flagReadOnly) {
					if (keycode == Keycode::A && ev->isControlKey()) {
						selectAll();
					}
				}
#endif
				if (!(ev->isAltKey())) {
					ev->acceptByNative();
				}
			} else {
				ev->acceptByNative();
			}
			return;
		}
		if (m_flagReadOnly) {
			return;
		}
		_initTextBox();
		Ref<UITextBox>& box = m_textBox;
		if (box.isNull()) {
			return;
		}
		if (action == UIAction::KeyDown) {
			Keycode key = ev->getKeycode();
			switch (key) {
				case Keycode::Backspace:
					if (!(ev->isControlKey()) && !(ev->isWindowsKey())) {
					}
					break;
				default:
					break;
			}
		}
	}


	PasswordView::PasswordView()
	{
		m_flagPassword = sl_true;
		setUsingIME(sl_false);
	}

	PasswordView::~PasswordView()
	{
	}


	SLIB_DEFINE_OBJECT(TextArea, EditView)

	TextArea::TextArea()
	{
		setChangeEventEnabled(sl_false);
		m_multiLine = MultiLineMode::Multiple;
		m_flagAutoDismissKeyboard = sl_false;
		m_gravity = Alignment::TopLeft;
		setReturnKeyType(UIReturnKeyType::Return);
		setScrolling(sl_true, sl_true, UIUpdateMode::Init);
	}

	TextArea::~TextArea()
	{
	}

#if !HAS_NATIVE_WIDGET_IMPL
	Ref<ViewInstance> EditView::createNativeWidget(ViewInstance* parent)
	{
		return sl_null;
	}

	Ptr<IEditViewInstance> EditView::getEditViewInstance()
	{
		return sl_null;
	}


	Ref<ViewInstance> TextArea::createNativeWidget(ViewInstance* parent)
	{
		return sl_null;
	}

	Ptr<IEditViewInstance> TextArea::getEditViewInstance()
	{
		return sl_null;
	}
#endif

	sl_bool IEditViewInstance::appendText(EditView* view, const StringParam& text)
	{
		return sl_false;
	}

	void IEditViewInstance::setLowercase(EditView* view, sl_bool flag)
	{
	}

	void IEditViewInstance::setUppercase(EditView* view, sl_bool flag)
	{
	}

	void IEditViewInstance::setReturnKeyType(EditView* view, UIReturnKeyType type)
	{
	}

	void IEditViewInstance::setKeyboardType(EditView* view, UIKeyboardType type)
	{
	}

	void IEditViewInstance::setAutoCapitalizationType(EditView* view, UIAutoCapitalizationType type)
	{
	}

	void IEditViewInstance::setSelection(EditView* view, sl_reg start, sl_reg end)
	{
	}

}
