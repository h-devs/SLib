#pragma once

#include "../layouts_base.h"

namespace sapp
{
	namespace ui
	{
		SLIB_DECLARE_WINDOW_LAYOUT_BEGIN(MainWindow)
			slib::Ref<slib::LinearLayout> _linear1;
			slib::Ref<slib::LabelView> _label1;
		SLIB_DECLARE_WINDOW_LAYOUT_END

	}
}
