#ifndef CHECKHEADER_SLIB_SRC_UI_WINDOW
#define CHECKHEADER_SLIB_SRC_UI_WINDOW

#include "slib/ui/window.h"

#include "slib/ui/core.h"
#include "slib/ui/screen.h"
#include "slib/ui/platform.h"

namespace slib
{
	namespace {

		class WindowHelper : public Window
		{
		public:
			using Window::_makeFrame;
		};

		SLIB_INLINE static UIRect MakeWindowFrame(Window* window)
		{
			return ((WindowHelper*)window)->_makeFrame();
		}

	}
}

#endif
