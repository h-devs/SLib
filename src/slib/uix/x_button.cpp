#include "slib/uix/button.h"

#include "slib/ui/cursor.h"

namespace slib
{

	XButton::XButton()
	{
	}

	XButton::~XButton()
	{
	}

	void XButton::init()
	{
		Button::init();

		setCursor(Cursor::getHand());
		setAntiAlias(sl_true, UIUpdateMode::Init);
		setBoundRadius(7.0f, UIUpdateMode::Init);
		setBackgroundColor(Color(0, 128, 255), UIUpdateMode::Init);
		setUsingFocusedState();
		setBorderColor(Color(0, 128, 255), UIUpdateMode::Init);
		setBorderWidth(2, UIUpdateMode::Init);
		setPadding(6, UIUpdateMode::Init);
		setTextColor(Color::White, UIUpdateMode::Init);

	}

}
