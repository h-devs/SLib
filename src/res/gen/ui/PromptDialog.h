#pragma once

#include "../layouts_base.h"

namespace slib
{
	namespace ui
	{
		SLIB_DECLARE_WINDOW_LAYOUT_BEGIN(PromptDialog)
			slib::Ref<slib::LinearLayout> _linear1;
			slib::Ref<slib::LabelView> label;
			slib::Ref<slib::EditView> input;
			slib::Ref<slib::LinearLayout> _linear2;
			slib::Ref<slib::Button> ok;
			slib::Ref<slib::Button> cancel;
		SLIB_DECLARE_WINDOW_LAYOUT_END

	}
}
