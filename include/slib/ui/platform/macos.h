/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_MACOS_PLATFORM
#define CHECKHEADER_SLIB_UI_MACOS_PLATFORM

#include "../definition.h"

#if defined(SLIB_UI_IS_MACOS)

#include "../platform_common.h"
#include "../event.h"
#include "../../core/function.h"

#include <CoreText/CoreText.h>

namespace slib
{

	class Screen;
	class Cursor;
	class Menu;
	class MenuItem;
	class UIEvent;

	class SLIB_EXPORT UIPlatform
	{
		PRIV_SLIB_DECLARE_UI_PLATFORM_COMMON_MEMBERS

	public:
#	if defined(__OBJC__)
		static Ref<ViewInstance> createViewInstance(NSView* handle);
		static void registerViewInstance(NSView* handle, ViewInstance* instance);
		static Ref<ViewInstance> getViewInstance(NSView* handle);
		static void removeViewInstance(NSView* handle);
		static NSView* getViewHandle(ViewInstance* instance);
		static NSView* getViewHandle(View* view);
		static sl_bool measureNativeWidgetFittingSize(View* view, UISize& _out);
		static sl_bool measureNativeWidgetFittingSize(ViewInstance* instance, UISize& _out);

		static Ref<WindowInstance> createWindowInstance(NSWindow* handle);
		static void registerWindowInstance(NSWindow* handle, WindowInstance* instance);
		static Ref<WindowInstance> getWindowInstance(NSWindow* handle);
		static void removeWindowInstance(NSWindow* handle);
		static NSWindow* getWindowHandle(WindowInstance* instance);
		static NSWindow* getWindowHandle(Window* window);
		static NSWindow* getMainWindow();
		static NSWindow* getKeyWindow();

		static Ref<Screen> createScreen(NSScreen* handle);
		static NSScreen* getScreenHandle(Screen* screen);
		static float getDpiForScreen(NSScreen* handle);

		static Ref<Cursor> createCursor(NSCursor* handle);
		static NSCursor* getCursorHandle(Cursor* cursor);

		static NSMenu* getMenuHandle(Menu* menu);
		static NSMenuItem* getMenuItemHandle(MenuItem* menu);

		static NSString* getKeyEquivalent(const KeycodeAndModifiers& km, NSUInteger& outMask);
		static void applyEventModifiers(UIEvent* ev, NSEvent* event);

		static void registerDidFinishLaunchingCallback(const Function<void(NSNotification*)>& callback);
#	endif

	};

}

#endif

#endif
