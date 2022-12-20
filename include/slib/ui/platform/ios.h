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

#ifndef CHECKHEADER_SLIB_UI_IOS_PLATFORM
#define CHECKHEADER_SLIB_UI_IOS_PLATFORM

#include "../definition.h"

#if defined(SLIB_UI_IS_MACOS) || defined(SLIB_UI_IS_IOS)

#include "../platform_common.h"
#include "../../core/function.h"

#include <CoreText/CoreText.h>

namespace slib
{

	class Screen;
	class PushNotificationMessage;

	class SLIB_EXPORT UIPlatform
	{
		PRIV_SLIB_DECLARE_UI_PLATFORM_COMMON_MEMBERS

	public:
#	if defined(__OBJC__)
		static Ref<ViewInstance> createViewInstance(UIView* handle);
		static void registerViewInstance(UIView* handle, ViewInstance* instance);
		static Ref<ViewInstance> getViewInstance(UIView* handle);
		static void removeViewInstance(UIView* handle);
		static UIView* getViewHandle(ViewInstance* instance);
		static UIView* getViewHandle(View* view);

		static Ref<WindowInstance> createWindowInstance(UIView* handle);
		static void registerWindowInstance(UIView* handle, WindowInstance* instance);
		static Ref<WindowInstance> getWindowInstance(UIView* handle);
		static void removeWindowInstance(UIView* handle);
		static UIView* getWindowHandle(WindowInstance* instance);
		static UIWindow* getMainWindow();
		static UIWindow* getKeyWindow();

		static Ref<Screen> createScreen(UIScreen* handle);
		static UIScreen* getScreenHandle(Screen* screen);

		static UIViewController* getCurrentViewController();
		static UIViewController* getCurrentViewController(const Ref<Window>& parentWindow);
		static UIView* findFirstResponder(UIView* rootView);

		static CGFloat getGlobalScaleFactor();
		static void setGlobalScaleFactor(CGFloat factor);

		static void registerDidFinishLaunchingCallback(const Function<void(NSDictionary*)>& callback);
		static void registerDidRegisterForRemoteNotifications(const Function<void(NSData*, NSError*)>& callback);
		static void registerDidReceiveRemoteNotificationCallback(const Function<void(NSDictionary*)>& callback);
		static void registerOpenUrlCallback(const Function<BOOL(NSURL*, NSDictionary*)>& callback);

		static sl_bool parseRemoteNotificationInfo(NSDictionary* userInfo, PushNotificationMessage& _out);
#	endif
	};

}

#endif

#endif
