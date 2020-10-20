/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_CORE
#define CHECKHEADER_SLIB_UI_CORE

#include "definition.h"

#include "event.h"

#include "../core/string.h"
#include "../core/thread.h"
#include "../core/function.h"
#include "../core/dispatch.h"

namespace slib
{

	class Window;
	class Screen;
	class Cursor;
	class Menu;
	class UIApp;
	class Font;
	class Locale;

	class SLIB_EXPORT UI
	{
	public:
		static Ref<Font> getDefaultFont();

		static void setDefaultFont(const Ref<Font>& font);
		
		static sl_real getDefaultFontSize();
		
		static void setDefaultFontSize(sl_real fontSize);
		
		static String getDefaultFontFamily();
		
		static void setDefaultFontFamily(const String& fontFamily);

		static void setDefaultFontFamilyForLocale(const Locale& locale);
		

		static sl_ui_len getDefaultScrollBarWidth();
		
		static void setDefaultScrollBarWidth(sl_ui_len width);
		
		// Screens
		static List< Ref<Screen> > getScreens();
		
		static Ref<Screen> getPrimaryScreen();
		
		static Ref<Screen> getFocusedScreen();
		
		static UIRect getScreenRegion();
		
		static UIRect getScreenRegion(const Ref<Screen>& screen);
		
		static UIRect getScreenBounds();
		
		static UIRect getScreenBounds(const Ref<Screen>& screen);
		
		static UISize getScreenSize();
		
		static UISize getScreenSize(const Ref<Screen>& screen);
		
		static sl_ui_len getScreenWidth();
		
		static sl_ui_len getScreenHeight();
		
		static List<ScreenOrientation> getAvailableScreenOrientations();
		
		static void setAvailableScreenOrientations(const List<ScreenOrientation>& orientations);
		
		static void setAvailableScreenOrientation(const ScreenOrientation& orientation);
		
		// ScreenOrientationPortrait
		static void setAvailableScreenOrientationsPortrait();
		
		// ScreenOrientationLandscapeRight, ScreenOrientationLandscapeLeft
		static void setAvailableScreenOrientationsLandscape();
		
		static void setAvailableScreenOrientationsAll();
		
		static ScreenOrientation getScreenOrientation();
		
		static void attemptRotateScreenOrientation();
		
		
		static double getScreenPPI();
		
		static sl_real pixelToInch(sl_real px);
		
		static sl_real inchToPixel(sl_real inch);
		
		static sl_real pixelToMeter(sl_real px);

		static sl_real meterToPixel(sl_real meters);
		
		static sl_real pixelToCentimeter(sl_real px);
		
		static sl_real centimeterToPixel(sl_real cm);
		
		static sl_real pixelToMillimeter(sl_real px);
		
		static sl_real millimeterToPixel(sl_real mm);
		
		static sl_real pixelToPoint(sl_real px);
		
		static sl_real pointToPixel(sl_real dp);
		
		static sl_real pixelToDp(sl_real px);
		
		static sl_real dpToPixel(sl_real dp);
		
		// Message Box
		static void alert(const String& text);

		static void alert(const String& caption, const String& text);
		
		static void alert(const Ref<Window>& parent, const String& text);
		
		static void alert(const Ref<Window>& parent, const String& caption, const String& text);
		
		static void showAlert(const String& text, const Function<void()>& onOk = Function<void()>::null());
		
		static void showAlert(const String& caption, const String& text, const Function<void()>& onOk = Function<void()>::null());
		
		static void showAlert(const Ref<Window>& parent, const String& text, const Function<void()>& onOk = Function<void()>::null());
		
		static void showAlert(const Ref<Window>& parent, const String& caption, const String& text, const Function<void()>& onOk = Function<void()>::null());
		
		static sl_bool confirm(const String& text);

		static sl_bool confirm(const String& caption, const String& text);

		static sl_bool confirm(const Ref<Window>& parent, const String& text);

		static sl_bool confirm(const Ref<Window>& parent, const String& caption, const String& text);
		
		static void showConfirm(const String& text, const Function<void(sl_bool)>& onResult = Function<void(sl_bool)>::null());
		
		static void showConfirm(const String& caption, const String& text, const Function<void(sl_bool)>& onResult = Function<void(sl_bool)>::null());
		
		static void showConfirm(const Ref<Window>& parent, const String& text, const Function<void(sl_bool)>& onResult = Function<void(sl_bool)>::null());
		
		static void showConfirm(const Ref<Window>& parent, const String& caption, const String& text, const Function<void(sl_bool)>& onResult = Function<void(sl_bool)>::null());
		

		// HID related functions (Platform Specific)
		static sl_bool checkKeyPressed(Keycode key);

		static sl_bool checkScrollLockOn();
		
		static sl_bool checkNumLockOn();
		
		static sl_bool checkLeftButtonPressed();
		
		static sl_bool checkRightButtonPressed();

		static sl_bool checkMiddleButtonPressed();				
		
		static sl_bool checkCapsLockOn();
		
		static UIPoint getCursorPos();

		static void sendKeyEvent(UIAction action, Keycode key);

		static void sendMouseEvent(UIAction action, sl_ui_pos x, sl_ui_pos y, sl_bool flagAbsolutePos = sl_true);

		// UI Thread
		static sl_bool isUiThread();
		
		static void dispatchToUiThread(const Function<void()>& callback, sl_uint32 delayMillis = 0);
		
		static void dispatchToUiThreadUrgently(const Function<void()>& callback, sl_uint32 delayMillis = 0);
		
		static void runOnUiThread(const Function<void()>& callback);
		
		static Function<void()> getCallbackOnUiThread(const Function<void()>& callback);

		static Ref<Dispatcher> getDispatcher();
		
		// Run Loop
		static void runLoop();
		
		static void quitLoop();
		
		static void runApp();
		
		static void quitApp();

		static sl_bool isRunningApp();
		
		
		static void openUrl(const String& url);
		
		static void dismissKeyboard();
		
		static UIKeyboardAdjustMode getKeyboardAdjustMode();
		
		static void setKeyboardAdjustMode(UIKeyboardAdjustMode mode);
		
		static UIEdgeInsets getSafeAreaInsets();
		
		static sl_ui_len getStatusBarHeight();
		
		static void setStatusBarStyle(StatusBarStyle style);
		
		static void setBadgeNumber(sl_uint32 number);

	};
	
}

#define SLIB_UI_CALLBACK(...) slib::UI::getCallbackOnUiThread(SLIB_BIND(void(), __VA_ARGS__))
#define SLIB_UI_CALLBACK_MEMBER(...) slib::UI::getCallbackOnUiThread(SLIB_BIND_MEMBER(void(), __VA_ARGS__))
#define SLIB_UI_CALLBACK_REF(...) slib::UI::getCallbackOnUiThread(SLIB_BIND_REF(void(), __VA_ARGS__))
#define SLIB_UI_CALLBACK_WEAKREF(...) slib::UI::getCallbackOnUiThread(SLIB_BIND_WEAKREF(void(), __VA_ARGS__))

#define SLIB_VIEW_RUN_ON_UI_THREAD(FUNC, ...) \
	if (!(UI::isUiThread())) { \
		UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef(this, FUNC, ##__VA_ARGS__)); \
		return; \
	}

#endif
