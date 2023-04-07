#include "slib/uix/edit_view.h"

#include "slib/ui/cursor.h"
#include "slib/graphics/font.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(XEditView, XControl)

	XEditView::XEditView()
	{
	}

	XEditView::~XEditView()
	{
	}

	void XEditView::init()
	{
		XControl::init();

		setCursor(Cursor::getIBeam());
		setPadding(5, 7, 5, 3, UIUpdateMode::Init);
		setBackgroundColor(Color::White, UIUpdateMode::Init);

		m_edit = new EditView;
		if (m_edit.isNotNull()) {

			m_edit->setBackgroundColor(Color::White, UIUpdateMode::Init);
			m_edit->setWidthFilling(1.0f, UIUpdateMode::Init);
			m_edit->setHeightFilling(1.0f, UIUpdateMode::Init);
			m_edit->setAlignParentLeft(UIUpdateMode::Init);
			m_edit->setAlignParentTop(UIUpdateMode::Init);
			m_edit->setBorder(sl_false, UIUpdateMode::Init);

			m_edit->setOnChanging([this](EditView*, String& value, UIEvent* ev) {
				invokeChanging(value, ev);
			});
			m_edit->setOnChange([this](EditView*, const String& value, UIEvent* ev) {
				invokeChange(value, ev);
			});
			m_edit->setOnPostChange([this](EditView*) {
				invokePostChange();
			});
			m_edit->setOnReturnKey([this](EditView*) {
				invokeReturnKey();
			});

			addChild(m_edit, UIUpdateMode::Init);
		}
	}

	String XEditView::getText()
	{
		if (m_edit.isNotNull()) {
			return m_edit->getText();
		}
		return sl_null;
	}

	void XEditView::setText(const String& text, UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			m_edit->setText(text, mode);
		}
	}

	void XEditView::appendText(const StringParam& text, UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			m_edit->appendText(text, mode);
		}
	}

	sl_bool XEditView::isChangeEventEnabled()
	{
		if (m_edit.isNotNull()) {
			return m_edit->isChangeEventEnabled();
		}
		return sl_false;
	}

	void XEditView::setChangeEventEnabled(sl_bool flag)
	{
		if (m_edit.isNotNull()) {
			m_edit->setChangeEventEnabled(flag);
		}
	}

	Alignment XEditView::getGravity()
	{
		if (m_edit.isNotNull()) {
			return m_edit->getGravity();
		}
		return Alignment::Default;
	}

	void XEditView::setGravity(const Alignment& gravity, UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			m_edit->setGravity(gravity, mode);
		}
	}

	Color XEditView::getTextColor()
	{
		if (m_edit.isNotNull()) {
			return m_edit->getTextColor();
		}
		return Color::Black;
	}

	void XEditView::setTextColor(const Color& color, UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			m_edit->setTextColor(color, mode);
		}
	}

	String XEditView::getHintText()
	{
		if (m_edit.isNotNull()) {
			return m_edit->getHintText();
		}
		return sl_null;
	}

	void XEditView::setHintText(const String& str, UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			m_edit->setHintText(str, mode);
		}
	}

	Alignment XEditView::getHintGravity()
	{
		if (m_edit.isNotNull()) {
			return m_edit->getHintGravity();
		}
		return Alignment::Default;
	}

	void XEditView::setHintGravity(const Alignment& gravity, UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			m_edit->setHintGravity(gravity, mode);
		}
	}

	Color XEditView::getHintTextColor()
	{
		if (m_edit.isNotNull()) {
			return m_edit->getHintTextColor();
		}
		return Color::Black;
	}

	void XEditView::setHintTextColor(const Color& color, UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			m_edit->setHintTextColor(color, mode);
		}
	}

	Ref<Font> XEditView::getHintFont()
	{
		if (m_edit.isNotNull()) {
			return m_edit->getHintFont();
		}
		return getFont();
	}

	void XEditView::setHintFont(const Ref<Font>& font, UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			m_edit->setHintFont(font, mode);
		}
	}

	sl_bool XEditView::isReadOnly()
	{
		if (m_edit.isNotNull()) {
			return m_edit->isReadOnly();
		}
		return sl_false;
	}

	void XEditView::setReadOnly(sl_bool flag, UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			m_edit->setReadOnly(flag, mode);
		}
	}

	sl_bool XEditView::isPassword()
	{
		if (m_edit.isNotNull()) {
			return m_edit->isPassword();
		}
		return sl_false;
	}

	void XEditView::setPassword(sl_bool flag, UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			m_edit->setPassword(flag, mode);
		}
	}

	sl_bool XEditView::isNumber()
	{
		if (m_edit.isNotNull()) {
			return m_edit->isNumber();
		}
		return sl_false;
	}

	void XEditView::setNumber(sl_bool flag, UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			m_edit->setNumber(flag, mode);
		}
	}

	sl_bool XEditView::isLowercase()
	{
		if (m_edit.isNotNull()) {
			return m_edit->isLowercase();
		}
		return sl_false;
	}

	void XEditView::setLowercase(sl_bool flag, UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			m_edit->setLowercase(flag, mode);
		}
	}

	sl_bool XEditView::isUppercase()
	{
		if (m_edit.isNotNull()) {
			return m_edit->isUppercase();
		}
		return sl_false;
	}

	void XEditView::setUppercase(sl_bool flag, UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			m_edit->setUppercase(flag, mode);
		}
	}

	MultiLineMode XEditView::getMultiLine()
	{
		if (m_edit.isNotNull()) {
			return m_edit->getMultiLine();
		}
		return MultiLineMode::Single;
	}

	void XEditView::setMultiLine(MultiLineMode multiLineMode, UIUpdateMode updateMode)
	{
		if (m_edit.isNotNull()) {
			m_edit->setMultiLine(multiLineMode, updateMode);
		}
	}

	UIReturnKeyType XEditView::getReturnKeyType()
	{
		if (m_edit.isNotNull()) {
			return m_edit->getReturnKeyType();
		}
		return UIReturnKeyType::Default;
	}

	void XEditView::setReturnKeyType(UIReturnKeyType type)
	{
		if (m_edit.isNotNull()) {
			m_edit->setReturnKeyType(type);
		}
	}

	UIKeyboardType XEditView::getKeyboardType()
	{
		if (m_edit.isNotNull()) {
			return m_edit->getKeyboardType();
		}
		return UIKeyboardType::Default;
	}

	void XEditView::setKeyboardType(UIKeyboardType type)
	{
		if (m_edit.isNotNull()) {
			m_edit->setKeyboardType(type);
		}
	}

	UIAutoCapitalizationType XEditView::getAutoCaptializationType()
	{
		if (m_edit.isNotNull()) {
			return m_edit->getAutoCaptializationType();
		}
		return UIAutoCapitalizationType::None;
	}

	void XEditView::setAutoCapitalizationType(UIAutoCapitalizationType type)
	{
		if (m_edit.isNotNull()) {
			m_edit->setAutoCapitalizationType(type);
		}
	}

	sl_bool XEditView::isAutoDismissKeyboard()
	{
		if (m_edit.isNotNull()) {
			return m_edit->isAutoDismissKeyboard();
		}
		return sl_false;
	}

	void XEditView::setAutoDismissKeyboard(sl_bool flag)
	{
		if (m_edit.isNotNull()) {
			m_edit->setAutoDismissKeyboard(flag);
		}
	}

	void XEditView::setFocusNextOnReturnKey()
	{
		if (m_edit.isNotNull()) {
			m_edit->setFocusNextOnReturnKey();
		}
	}

	void XEditView::setSelection(sl_reg start, sl_reg end)
	{
		if (m_edit.isNotNull()) {
			m_edit->setSelection(start, end);
		}
	}

	void XEditView::selectAll()
	{
		if (m_edit.isNotNull()) {
			m_edit->selectAll();
		}
	}

	void XEditView::selectNone()
	{
		if (m_edit.isNotNull()) {
			m_edit->selectNone();
		}
	}

	sl_bool XEditView::isAutoHorizontalScrolling()
	{
		if (m_edit.isNotNull()) {
			return m_edit->isAutoHorizontalScrolling();
		}
		return sl_false;
	}

	void XEditView::setAutoHorizontalScrolling(sl_bool flag)
	{
		if (m_edit.isNotNull()) {
			m_edit->setAutoHorizontalScrolling(flag);
		}
	}

	sl_bool XEditView::isAutoVerticalScrolling()
	{
		if (m_edit.isNotNull()) {
			return m_edit->isAutoVerticalScrolling();
		}
		return sl_false;
	}

	void XEditView::setAutoVerticalScrolling(sl_bool flag)
	{
		if (m_edit.isNotNull()) {
			m_edit->setAutoVerticalScrolling(flag);
		}
	}

	void XEditView::setFocus(sl_bool flagFocused, UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			m_edit->setFocus(flagFocused, mode);
		}
	}


	SLIB_DEFINE_EVENT_HANDLER(XEditView, Changing, (String& value, UIEvent* ev /* nullable */), value, ev)

	SLIB_DEFINE_EVENT_HANDLER(XEditView, Change, (const String& value, UIEvent* ev /* nullable */), value, ev)

	SLIB_DEFINE_EVENT_HANDLER(XEditView, PostChange, ())

	SLIB_DEFINE_EVENT_HANDLER(XEditView, ReturnKey, ())

	void XEditView::onChangeSizeMode(UIUpdateMode mode)
	{
		if (m_edit.isNotNull()) {
			mode = SLIB_UI_UPDATE_MODE_IS_INIT(mode) ? UIUpdateMode::Init : UIUpdateMode::None;
			if (isWidthWrapping()) {
				m_edit->setWidthWrapping(mode);
			} else if (isWidthFilling()) {
				m_edit->setWidthFilling(1.0f, mode);
			}
			if (isHeightWrapping()) {
				m_edit->setHeightWrapping(mode);
			} else if (isHeightFilling()) {
				m_edit->setHeightFilling(1.0f, mode);
			}
		}
	}

	void XEditView::onClickEvent(UIEvent* ev)
	{
		if (m_edit.isNotNull()) {
			m_edit->setFocus();
		}
		XControl::onClickEvent(ev);
	}


	XPasswordView::XPasswordView()
	{
	}

	XPasswordView::~XPasswordView()
	{
	}

	void XPasswordView::init()
	{
		XEditView::init();
		if (m_edit.isNotNull()) {
			m_edit->setPassword(sl_true, UIUpdateMode::Init);
		}
	}

}
