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

#include "slib/ui/edit_view.h"

#include "slib/ui/mobile_app.h"
#include "slib/ui/resource.h"
#include "slib/ui/core.h"
#include "slib/ui/button.h"

#include "slib/core/platform_android.h"

#if defined(SLIB_UI)
#	define HAS_NATIVE_WIDGET_IMPL 1
#else
#	define HAS_NATIVE_WIDGET_IMPL 0
#endif

namespace slib
{

	namespace priv
	{
		namespace edit_view
		{

			class EditViewHelper : public EditView
			{
			public:
				void closeDialog()
				{
					m_dialog.setNull();
				}

			};

			class EditDialog : public Referable
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
					if (IsInstanceOf<PasswordView>(view)) {
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
					edit->setOnChange(SLIB_FUNCTION_WEAKREF(EditDialog, _onChange, this));
					edit->setOnReturnKey(SLIB_FUNCTION_WEAKREF(EditDialog, _onReturnKey, this));
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
					window->setOnClose(SLIB_FUNCTION_WEAKREF(EditDialog, _onClose, this));
					edit->setFocus(sl_true, UIUpdateMode::Init);

#if defined(SLIB_UI_IS_IOS)
					sl_bool flagDoneButton = multiLineMode != MultiLineMode::Single;
#else
					sl_bool flagDoneButton = sl_true;
#endif
#if defined(SLIB_UI_IS_ANDROID)
					UI::dispatchToUiThread([] {
						Android::showKeyboard();
					}, 500);
#endif
					if (flagDoneButton) {
						Ref<Button> btnDone = new Button;
						if (btnDone.isNull()) {
							return sl_false;
						}
						btnDone->setText("Done", UIUpdateMode::Init);
						btnDone->setAlignParentRight(UIUpdateMode::Init);
						btnDone->setOnClick(SLIB_FUNCTION_WEAKREF(EditDialog, _onDone, this));
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
					m_view = Ref<EditViewHelper>::from(view);

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

				void _onChange(EditView* ev, String* text)
				{
					Ref<EditViewHelper> view = m_view;
					if (view.isNull()) {
						return;
					}
					view->dispatchChange(text);
					if (m_edit->getMultiLine() == MultiLineMode::Single) {
						sl_reg index = ParseUtil::indexOfLine(*text);
						if (index >= 0) {
							*text = text->mid(0, index);
						}
					}
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
					view->dispatchReturnKey();
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
					Android::dismissKeyboard();
#endif
				}

				void _onClose(Window* window, UIEvent* ev)
				{
					Ref<EditViewHelper> view = m_view;
					if (view.isNull()) {
						return;
					}
					view->invalidate();
					_onDone(sl_null);
					view->dispatchReturnKey();
				}

			};

		}
	}

	using namespace priv::edit_view;

	SLIB_DEFINE_OBJECT(EditView, View)

	EditView::EditView()
	{		
	}

	EditView::~EditView()
	{
	}

	void EditView::init()
	{
		View::init();

		setSupportedNativeWidget(HAS_NATIVE_WIDGET_IMPL);
		setCreatingNativeWidget(HAS_NATIVE_WIDGET_IMPL);

		setUsingFont(sl_true);
		setFocusable(sl_true);

		m_gravity = Alignment::MiddleCenter;
		m_textColor = Color::Black;
		m_hintGravity = Alignment::MiddleCenter;
		m_hintTextColor = Color(150, 150, 150);
		m_flagReadOnly = sl_false;
		m_flagPassword = sl_false;
		m_multiLine = MultiLineMode::Single;
		m_returnKeyType = UIReturnKeyType::Default;
		m_keyboardType = UIKeyboardType::Default;
		m_autoCapitalizationType = UIAutoCapitalizationType::None;
		m_flagAutoDismissKeyboard = sl_true;

		setBorder(sl_true, UIUpdateMode::Init);
		setPadding((sl_ui_pos)(UI::dpToPixel(2)), UIUpdateMode::Init);
	}

	String EditView::getText()
	{
		return m_text;
	}

	String EditView::getInstanceText()
	{
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
			SLIB_VIEW_RUN_ON_UI_THREAD(&EditView::setText, text, mode)
			m_text = text;
			instance->setText(this, text);
			if (isHeightWrapping()) {
				invalidateLayoutOfWrappingControl(mode);
			}
		} else {
			m_text = text;
			if (isHeightWrapping()) {
				invalidateLayoutOfWrappingControl(mode);
			} else {
				invalidate(mode);
			}
		}
	}

	Alignment EditView::getGravity()
	{
		return m_gravity;
	}

	void EditView::setGravity(const Alignment& gravity, UIUpdateMode mode)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(&EditView::setGravity, gravity, mode)
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
			SLIB_VIEW_RUN_ON_UI_THREAD(&EditView::setTextColor, color, mode)
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
			SLIB_VIEW_RUN_ON_UI_THREAD(&EditView::setHintText, str, mode)
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
			SLIB_VIEW_RUN_ON_UI_THREAD(&EditView::setHintGravity, gravity, mode)
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
			SLIB_VIEW_RUN_ON_UI_THREAD(&EditView::setHintTextColor, color, mode)
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
			SLIB_VIEW_RUN_ON_UI_THREAD(&EditView::setHintFont, font, mode)
			m_hintFont = font;
			instance->setHintFont(this, getHintFont());
		} else {
			m_hintFont = font;
			invalidate(mode);
		}
	}

	sl_bool EditView::isReadOnly()
	{
		return m_flagReadOnly;
	}

	void EditView::setReadOnly(sl_bool flag, UIUpdateMode mode)
	{
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(&EditView::setReadOnly, flag, mode)
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
			SLIB_VIEW_RUN_ON_UI_THREAD(&EditView::setPassword, flag, mode)
			m_flagPassword = flag;
			instance->setPassword(this, flag);
		} else {
			m_flagPassword = flag;
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
			SLIB_VIEW_RUN_ON_UI_THREAD(&EditView::setMultiLine, multiLineMode, updateMode)
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
			SLIB_VIEW_RUN_ON_UI_THREAD(&EditView::setReturnKeyType, type)
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
			SLIB_VIEW_RUN_ON_UI_THREAD(&EditView::setKeyboardType, type)
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
			SLIB_VIEW_RUN_ON_UI_THREAD(&EditView::setAutoCapitalizationType, type)
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

	void EditView::onUpdateLayout()
	{
		sl_bool flagHorizontalWrapping = isWidthWrapping();
		sl_bool flagVerticalWrapping = isHeightWrapping();
		
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
		if (m_text.isEmpty()) {
			canvas->drawText(m_hintText, getBoundsInnerPadding(), getHintFont(), m_hintTextColor, m_hintGravity);
		} else {
			if (m_flagPassword) {
				canvas->drawText(String('*', m_text.getLength()), getBoundsInnerPadding(), getFont(), m_textColor, m_gravity);
			} else {
				canvas->drawText(m_text, getBoundsInnerPadding(), getFont(), m_textColor, m_gravity);
			}
		}
	}
	
	void EditView::onClickEvent(UIEvent* ev)
	{
		if (m_flagReadOnly) {
			return;
		}
		Ptr<IEditViewInstance> instance = getEditViewInstance();
		if (instance.isNotNull()) {
			return;
		}
		if (m_dialog.isNull()) {
			m_dialog = EditDialog::open(this);
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(EditView, Change, String* value)

	void EditView::dispatchChange(String* value)
	{
		if (*value == m_text) {
			return;
		}
		SLIB_INVOKE_EVENT_HANDLER(Change, value)
		if (*value == m_text) {
			return;
		}
		m_text = *value;
		if (isNativeWidget()) {
			invalidateLayoutOfWrappingControl();
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(EditView, ReturnKey)

	void EditView::dispatchReturnKey()
	{
		SLIB_INVOKE_EVENT_HANDLER(ReturnKey)
		if (m_multiLine == MultiLineMode::Single && m_flagAutoDismissKeyboard) {
			UI::dismissKeyboard();
		}
	}
	
	void EditView::dispatchKeyEvent(UIEvent* ev)
	{
		if (m_multiLine == MultiLineMode::Single || ev->getKeycode() == Keycode::Escape) {
			if (ev->getAction() == UIAction::KeyDown) {
				if (ev->getKeycode() == Keycode::Enter) {
					dispatchReturnKey();
				}
			}
			View::dispatchKeyEvent(ev);
		} else {
			SLIB_INVOKE_EVENT_HANDLER(KeyEvent, ev)
			ev->stopPropagation();
		}
	}

	
	PasswordView::PasswordView()
	{
		m_flagPassword = sl_true;
	}

	
	SLIB_DEFINE_OBJECT(TextArea, EditView)

	TextArea::TextArea()
	{
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
	
	void IEditViewInstance::setReturnKeyType(EditView* view, UIReturnKeyType type)
	{
	}
	
	void IEditViewInstance::setKeyboardType(EditView* view, UIKeyboardType type)
	{
	}
	
	void IEditViewInstance::setAutoCapitalizationType(EditView* view, UIAutoCapitalizationType type)
	{
	}
	
}
