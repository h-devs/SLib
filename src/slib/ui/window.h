#ifndef CHECKHEADER_SLIB_SRC_UI_WINDOW
#define CHECKHEADER_SLIB_SRC_UI_WINDOW

#include "slib/ui/window.h"

#include "slib/ui/core.h"
#include "slib/ui/screen.h"
#include "slib/ui/platform.h"

namespace slib
{
	namespace priv
	{
		namespace window
		{

			class WindowHelper : public Window
			{
			public:
				UIRect makeFrame()
				{
					UIRect frameScreen;
					Ref<Screen> screen = m_screen;
					if (screen.isNotNull()) {
						frameScreen = screen->getRegion();
					} else {
						frameScreen = UI::getScreenRegion();
					}
					UIRect frame;
					if (m_flagFullScreen) {
						frame.setLeftTop(0, 0);
						frame.setSize(frameScreen.getSize());
					} else {
						frame = getFrame();
						if (m_flagCenterScreen) {
							frame.setLocation(frameScreen.getWidth() / 2 - frame.getWidth() / 2, frameScreen.getHeight() / 2 - frame.getHeight() / 2);
						}
					}
					frame.fixSizeError();
					return frame;
				}

			};

			SLIB_INLINE static UIRect MakeWindowFrame(Window* window)
			{
				return ((WindowHelper*)window)->makeFrame();
			}

		}
	}
}

#endif
