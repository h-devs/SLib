#include "menus.h"

#include "strings.h"
#include "drawables.h"

namespace slib
{
	namespace menu
	{

		SLIB_DEFINE_MENU_BEGIN(label_view_context, sl_true)
			SLIB_DEFINE_MENU_ITEM(root, copy, string::copy::get(), slib::Keycode::Unknown)
		SLIB_DEFINE_MENU_END

	}
}
