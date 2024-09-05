/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "event.h"

#include "../core/string.h"
#include "../core/function.h"
#include "../core/dispatch.h"

namespace slib
{

	class Window;
	class Screen;
	class Canvas;
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

		static UIRect getScreenRegion();

		static UIRect getScreenRegion(const Ref<Screen>& screen);

		static UIRect getScreenWorkingRegion();

		static UIRect getScreenWorkingRegion(const Ref<Screen>& screen);

		static UIRect getScreenBounds();

		static UIRect getScreenBounds(const Ref<Screen>& screen);

		static UISize getScreenSize();

		static UISize getScreenSize(const Ref<Screen>& screen);

		static sl_ui_len getScreenWidth();

		static sl_ui_len getScreenHeight();

		static Ref<Canvas> getScreenCanvas();


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
		static void alert(const StringParam& text);

		static void alert(const StringParam& caption, const StringParam& text);

		static void alert(AlertIcon icon, const StringParam& text);

		static void alert(AlertIcon icon, const StringParam& caption, const StringParam& text);

		static void alert(const Ref<Window>& parent, const StringParam& text);

		static void alert(const Ref<Window>& parent, const StringParam& caption, const StringParam& text);

		static void alert(const Ref<Window>& parent, AlertIcon icon, const StringParam& text);

		static void alert(const Ref<Window>& parent, AlertIcon icon, const StringParam& caption, const StringParam& text);

		static void showAlert(const StringParam& text, const Function<void()>& onOK = sl_null);

		static void showAlert(const StringParam& caption, const StringParam& text, const Function<void()>& onOK = sl_null);

		static void showAlert(AlertIcon icon, const StringParam& text, const Function<void()>& onOK = sl_null);

		static void showAlert(AlertIcon icon, const StringParam& caption, const StringParam& text, const Function<void()>& onOK = sl_null);

		static void showAlert(const Ref<Window>& parent, const StringParam& text, const Function<void()>& onOK = sl_null);

		static void showAlert(const Ref<Window>& parent, const StringParam& caption, const StringParam& text, const Function<void()>& onOK = sl_null);

		static void showAlert(const Ref<Window>& parent, AlertIcon icon, const StringParam& text, const Function<void()>& onOK = sl_null);

		static void showAlert(const Ref<Window>& parent, AlertIcon icon, const StringParam& caption, const StringParam& text, const Function<void()>& onOK = sl_null);

		static sl_bool confirm(const StringParam& text);

		static sl_bool confirm(const StringParam& caption, const StringParam& text);

		static sl_bool confirm(const Ref<Window>& parent, const StringParam& text);

		static sl_bool confirm(const Ref<Window>& parent, const StringParam& caption, const StringParam& text);

		static void showConfirm(const StringParam& text, const Function<void(sl_bool)>& onResult = sl_null);

		static void showConfirm(const StringParam& caption, const StringParam& text, const Function<void(sl_bool)>& onResult = sl_null);

		static void showConfirm(const Ref<Window>& parent, const StringParam& text, const Function<void(sl_bool)>& onResult = sl_null);

		static void showConfirm(const Ref<Window>& parent, const StringParam& caption, const StringParam& text, const Function<void(sl_bool)>& onResult = sl_null);

		static String prompt(const StringParam& message);

		static String prompt(const StringParam& caption, const StringParam& message, const StringParam& defaultValue);

		static String prompt(const Ref<Window>& parent, const StringParam& message);

		static String prompt(const Ref<Window>& parent, const StringParam& caption, const StringParam& message, const StringParam& defaultValue);

		static void showPrompt(const StringParam& message, const Function<void(String&)>& onResult = sl_null);

		static void showPrompt(const StringParam& caption, const StringParam& message, const StringParam& defaultValue, const Function<void(String&)>& onResult = sl_null);

		static void showPrompt(const Ref<Window>& parent, const StringParam& message, const Function<void(String&)>& onResult = sl_null);

		static void showPrompt(const Ref<Window>& parent, const StringParam& caption, const StringParam& message, const StringParam& defaultValue, const Function<void(String&)>& onResult = sl_null);


		// HID related functions (Platform Specific)
		static sl_bool isKeyPressed(Keycode key);

		static sl_bool isScrollLockOn();

		static sl_bool isNumLockOn();

		static sl_bool isLeftButtonPressed();

		static sl_bool isRightButtonPressed();

		static sl_bool isMiddleButtonPressed();

		static sl_bool isCapsLockOn();

		static UIPoint getCursorPos();

		static void sendKeyEvent(UIAction action, Keycode key);

		static void sendMouseEvent(UIAction action, sl_ui_pos x, sl_ui_pos y, sl_bool flagAbsolutePos = sl_true);

		// UI Thread
		static sl_bool isUiThread();

		static void setUiThread();

		static void resetUiThread();

		static void dispatchToUiThread(const Function<void()>& callback, sl_uint32 delayMillis = 0);

		static void dispatchToUiThreadUrgently(const Function<void()>& callback, sl_uint32 delayMillis = 0);

		static void runOnUiThread(const Function<void()>& callback);

		static Function<void()> getCallbackOnUiThread(const Function<void()>& callback);

		static Ref<Dispatcher> getDispatcher();

		// Run Loop
		static void runLoop();

		static void quitLoop();

		static void initApp();

		static void runApp();

		static void quitApp();

		static sl_bool isRunningApp();

		static sl_bool isQuitingApp();


		static void openUrl(const StringParam& url);

		static void openFile(const StringParam& path);

		static void openDirectory(const StringParam& path);

		static void openDirectoryAndSelectFile(const StringParam& path);


		static void showKeyboard();

		static void dismissKeyboard();


		static void getActiveApplicationAndWindow(String& appName, String& windowTitle, sl_int32 timeout = -1);

		static String getActiveApplicationName();

		// [macOS] Accessibility authentication is required. See `Application::isAccessibilityEnabled()`.
		static String getActiveWindowTitle(sl_int32 timeout = -1);

	};

}

#define SLIB_UI_CALLBACK(...) slib::UI::getCallbackOnUiThread(Function<void()>::bind(__VA_ARGS__))
#define SLIB_UI_CALLBACK_MEMBER(...) slib::UI::getCallbackOnUiThread(Function<void()>::bindMember(__VA_ARGS__))
#define SLIB_UI_CALLBACK_REF(...) slib::UI::getCallbackOnUiThread(Function<void()>::bindRef(__VA_ARGS__))
#define SLIB_UI_CALLBACK_WEAKREF(...) slib::UI::getCallbackOnUiThread(Function<void()>::bindWeakRef(__VA_ARGS__))

#define SLIB_VIEW_RUN_ON_UI_THREAD(MEMBER_NAME, ...) \
	if (!(UI::isUiThread())) { \
		UI::dispatchToUiThreadUrgently(SLIB_BIND_WEAKREF(void(), this, MEMBER_NAME, ##__VA_ARGS__)); \
		return; \
	}

#define SLIB_VIEW_RUN_ON_UI_THREAD2(FUNC, ...) \
	if (!(UI::isUiThread())) { \
		UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef(this, FUNC, ##__VA_ARGS__)); \
		return; \
	}

#endif
