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

#include "slib/ui/core.h"

#include "slib/ui/platform.h"
#include "slib/ui/window.h"
#include "slib/ui/screen.h"
#include "slib/ui/common_dialogs.h"
#include "slib/graphics/font.h"
#include "slib/network/url.h"
#include "slib/device/device.h"
#include "slib/core/file.h"
#include "slib/core/safe_static.h"

#ifndef SLIB_PLATFORM_IS_WINDOWS
#include <signal.h>
#endif

namespace slib
{
	
	namespace priv
	{
		namespace ui_core
		{
			
			class DefaultContext
			{
			public:
				sl_real fontSize;
				AtomicString fontFamily;
				
				SpinLock lockFont;
				Ref<Font> font;

				sl_ui_len scrollBarWidth;
				
			public:
				DefaultContext()
				{
#if defined(SLIB_PLATFORM_IS_DESKTOP)
					fontSize = 12;
					scrollBarWidth = 12;
#else
					fontSize = (sl_real)(SLIB_MIN(UI::getScreenWidth(), UI::getScreenHeight()) / 40);
					scrollBarWidth = SLIB_MIN(UI::getScreenWidth(), UI::getScreenHeight()) / 60;
#endif
				}
			};
			
			SLIB_SAFE_STATIC_GETTER(DefaultContext, getDefaultContext)

			class UICallback : public Callable<void()>
			{
			public:
				Function<void()> m_callback;
				
			public:
				UICallback(const Function<void()>& callback) noexcept: m_callback(callback) {}
				
			public:
				void invoke() noexcept override
				{
					if (UI::isUiThread()) {
						m_callback();
					} else {
						UI::dispatchToUiThread(m_callback);
					}
				}
				
			};
			
			class DispatcherImpl : public Dispatcher
			{
			public:
				sl_bool dispatch(const Function<void()>& callback, sl_uint64 delayMillis) override
				{
					if (delayMillis >> 31) {
						delayMillis = 0x7fffffff;
					}
					UI::dispatchToUiThread(callback, (sl_uint32)delayMillis);
					return sl_true;
				}
			};
			
			static sl_bool g_flagInitializedApp = sl_false;
			static sl_bool g_flagRunningApp = sl_false;
			static sl_int32 g_nLevelRunLoop = 0;
			static sl_bool g_flagQuitApp = sl_false;
			
			static void QuitLoop()
			{
				if (g_nLevelRunLoop) {
					UIPlatform::quitLoop();
				} else if (g_flagRunningApp) {
					UIPlatform::quitApp();
				}
			}
			
			static void QuitApp()
			{
				if (g_flagQuitApp) {
					return;
				}
				g_flagQuitApp = sl_true;
				QuitLoop();
			}
			
			static void TermHandler(int signum)
			{
				QuitApp();
			}

			SLIB_GLOBAL_ZERO_INITIALIZED(AtomicRef<View>, g_currentDraggingView)
			DragOperations g_currentDraggingOperationMask;
		
		}
	}
	
	using namespace priv::ui_core;

	SLIB_DEFINE_OBJECT(Screen, Object)

	Screen::Screen()
	{
	}

	Screen::~Screen()
	{
	}


	Ref<Font> UI::getDefaultFont()
	{
		DefaultContext* def = getDefaultContext();
		if (!def) {
			return sl_null;
		}
		if (def->font.isNotNull()) {
			SpinLocker lock(&def->lockFont);
			return def->font;
		} else {
			FontDesc desc;
			desc.familyName = def->fontFamily;
			desc.size = def->fontSize;
			Ref<Font> font = Font::create(desc);
			if (font.isNotNull()) {
				SpinLocker lock(&def->lockFont);
				def->font = font;
				return font;
			}
		}
		return sl_null;
	}

	void UI::setDefaultFont(const Ref<Font>& font)
	{
		DefaultContext* def = getDefaultContext();
		if (!def) {
			return;
		}
		if (font.isNotNull()) {
			def->fontFamily = font->getFamilyName();
			def->fontSize = font->getSize();
			SpinLocker lock(&(def->lockFont));
			def->font = font;
		} else {
			FontDesc desc;
			desc.familyName = def->fontFamily;
			desc.size = def->fontSize;
			Ref<Font> font = Font::create(desc);
			if (font.isNotNull()) {
				SpinLocker lock(&def->lockFont);
				def->font = font;
			}
		}
	}

	sl_real UI::getDefaultFontSize()
	{
		DefaultContext* def = getDefaultContext();
		if (def) {
			return def->fontSize;
		}
		return 0;
	}

	void UI::setDefaultFontSize(sl_real fontSize)
	{
		DefaultContext* def = getDefaultContext();
		if (!def) {
			return;
		}

		if (fontSize < 0) {
			fontSize = 0;
		}
		fontSize = SLIB_FONT_SIZE_PRECISION_APPLY(fontSize);
		if (def->fontSize == fontSize) {
			return;
		}
		def->fontSize = fontSize;

		SpinLocker lock(&(def->lockFont));
		if (def->font.isNotNull()) {
			FontDesc desc;
			def->font->getDesc(desc);
			desc.size = fontSize;
			Ref<Font> fontNew = Font::create(desc);
			if (fontNew.isNotNull()) {
				def->font = fontNew;
			}
		}
	}

	String UI::getDefaultFontFamily()
	{
		DefaultContext* def = getDefaultContext();
		if (def) {
			String name = def->fontFamily;
			if (name.isNotEmpty()) {
				return name;
			}
		}
		return Font::getDefaultFontFamily();
	}

	void UI::setDefaultFontFamily(const String& fontFamily)
	{
		DefaultContext* def = getDefaultContext();
		if (!def) {
			return;
		}

		if (String(def->fontFamily) == fontFamily) {
			return;
		}
		def->fontFamily = fontFamily;

		SpinLocker lock(&(def->lockFont));
		if (def->font.isNotNull()) {
			FontDesc desc;
			def->font->getDesc(desc);
			desc.familyName = fontFamily;
			Ref<Font> fontNew = Font::create(desc);
			if (fontNew.isNotNull()) {
				def->font = fontNew;
			}
		}
	}

	void UI::setDefaultFontFamilyForLocale(const Locale& locale)
	{
		UI::setDefaultFontFamily(Font::getDefaultFontFamilyForLocale(locale));
	}


	sl_ui_len UI::getDefaultScrollBarWidth()
	{
		DefaultContext* def = getDefaultContext();
		if (!def) {
			return 0;
		}
		return def->scrollBarWidth;
	}

	void UI::setDefaultScrollBarWidth(sl_ui_len len)
	{
		DefaultContext* def = getDefaultContext();
		if (!def) {
			return;
		}
		def->scrollBarWidth = len;
	}

	UIRect UI::getScreenRegion()
	{
		UISize size = Device::getScreenSize();
		return UIRect(0, 0, size.x, size.y);
	}

	UIRect UI::getScreenRegion(const Ref<Screen>& _screen)
	{
		UISize size = Device::getScreenSize();
		return UIRect(0, 0, size.x, size.y);
	}

	UIRect UI::getScreenBounds()
	{
		UISize size = Device::getScreenSize();
		return UIRect(0, 0, size.x, size.y);
	}

	UIRect UI::getScreenBounds(const Ref<Screen>& _screen)
	{
		UISize size = Device::getScreenSize();
		return UIRect(0, 0, size.x, size.y);
	}

	UISize UI::getScreenSize()
	{
		return Device::getScreenSize();
	}

	UISize UI::getScreenSize(const Ref<Screen>& _screen)
	{
		return Device::getScreenSize();
	}

	sl_ui_len UI::getScreenWidth()
	{
		return getScreenSize().x;
	}

	sl_ui_len UI::getScreenHeight()
	{
		return getScreenSize().y;
	}

	double UI::getScreenPPI()
	{
		double ppi = Device::getScreenPPI();
		if (ppi < 1) {
			return 1;
		} else {
			return ppi;
		}
	}

#if !defined(SLIB_PLATFORM_IS_WIN32)
	Ref<Canvas> UI::getScreenCanvas()
	{
		return sl_null;
	}
#endif
	
	sl_real UI::pixelToInch(sl_real px)
	{
		return (sl_real)(px / getScreenPPI());
	}
	
	sl_real UI::inchToPixel(sl_real inch)
	{
		return (sl_real)(inch * getScreenPPI());
	}
	
	sl_real UI::pixelToMeter(sl_real px)
	{
		return (sl_real)(px / getScreenPPI() * 0.0254);
	}
	
	sl_real UI::meterToPixel(sl_real meters)
	{
		return (sl_real)(meters * 39.3701 * getScreenPPI());
	}

	sl_real UI::pixelToCentimeter(sl_real px)
	{
		return (sl_real)(px * 2.54 / getScreenPPI());
	}
	
	sl_real UI::centimeterToPixel(sl_real cm)
	{
		return (sl_real)(cm * getScreenPPI() * 0.393701);
	}
	
	sl_real UI::pixelToMillimeter(sl_real px)
	{
		return (sl_real)(px * 25.4 / getScreenPPI());
	}
	
	sl_real UI::millimeterToPixel(sl_real mm)
	{
		return (sl_real)(mm * getScreenPPI() * 0.0393701);
	}
	
	sl_real UI::pixelToPoint(sl_real px)
	{
		return (sl_real)(px * 72 / getScreenPPI());
	}
	
	sl_real UI::pointToPixel(sl_real pt)
	{
		return (sl_real)(pt * getScreenPPI() / 72);
	}
	
	sl_real UI::pixelToDp(sl_real px)
	{
		return (sl_real)(px * 160 / getScreenPPI());
	}
	
	sl_real UI::dpToPixel(sl_real dp)
	{
		return (sl_real)(dp * getScreenPPI() / 160);
	}
	
	void UI::alert(const StringParam& text)
	{
		AlertDialog alert;
		alert.text = text.toString();
		alert.run();
	}

	void UI::alert(const StringParam& caption, const StringParam& text)
	{
		AlertDialog alert;
		alert.caption = caption.toString();
		alert.text = text.toString();
		alert.run();
	}

	void UI::alert(const Ref<Window>& parent, const StringParam& text)
	{
		AlertDialog alert;
		alert.parent = parent;
		alert.text = text.toString();
		alert.run();
	}

	void UI::alert(const Ref<Window>& parent, const StringParam& caption, const StringParam& text)
	{
		AlertDialog alert;
		alert.parent = parent;
		alert.caption = caption.toString();
		alert.text = text.toString();
		alert.run();
	}

	void UI::showAlert(const StringParam& text, const Function<void()>& onOk)
	{
		AlertDialog alert;
		alert.text = text.toString();
		alert.onOk = onOk;
		alert.show();
	}

	void UI::showAlert(const StringParam& caption, const StringParam& text, const Function<void()>& onOk)
	{
		AlertDialog alert;
		alert.caption = caption.toString();
		alert.text = text.toString();
		alert.onOk = onOk;
		alert.show();
	}

	void UI::showAlert(const Ref<Window>& parent, const StringParam& text, const Function<void()>& onOk)
	{
		AlertDialog alert;
		alert.parent = parent;
		alert.text = text.toString();
		alert.onOk = onOk;
		alert.show();
	}
	
	void UI::showAlert(const Ref<Window>& parent, const StringParam& caption, const StringParam& text, const Function<void()>& onOk)
	{
		AlertDialog alert;
		alert.parent = parent;
		alert.caption = caption.toString();
		alert.text = text.toString();
		alert.onOk = onOk;
		alert.show();
	}

	sl_bool UI::confirm(const StringParam& text)
	{
		AlertDialog alert;
		alert.buttons = AlertDialogButtons::OkCancel;
		alert.text = text.toString();
		return alert.run() == DialogResult::Ok;
	}
	
	sl_bool UI::confirm(const StringParam& caption, const StringParam& text)
	{
		AlertDialog alert;
		alert.buttons = AlertDialogButtons::OkCancel;
		alert.caption = caption.toString();
		alert.text = text.toString();
		return alert.run() == DialogResult::Ok;
	}
	
	sl_bool UI::confirm(const Ref<Window>& parent, const StringParam& text)
	{
		AlertDialog alert;
		alert.buttons = AlertDialogButtons::OkCancel;
		alert.parent = parent;
		alert.text = text.toString();
		return alert.run() == DialogResult::Ok;
	}
	
	sl_bool UI::confirm(const Ref<Window>& parent, const StringParam& caption, const StringParam& text)
	{
		AlertDialog alert;
		alert.buttons = AlertDialogButtons::OkCancel;
		alert.parent = parent;
		alert.caption = caption.toString();
		alert.text = text.toString();
		return alert.run() == DialogResult::Ok;
	}
	
	void UI::showConfirm(const StringParam& text, const Function<void(sl_bool)>& onResult)
	{
		AlertDialog alert;
		alert.buttons = AlertDialogButtons::OkCancel;
		alert.text = text.toString();
		alert.onComplete = [onResult](DialogResult result) {
			if (result == DialogResult::Ok) {
				onResult(sl_true);
			} else {
				onResult(sl_false);
			}
		};
		alert.show();
	}
	
	void UI::showConfirm(const StringParam& caption, const StringParam& text, const Function<void(sl_bool)>& onResult)
	{
		AlertDialog alert;
		alert.buttons = AlertDialogButtons::OkCancel;
		alert.caption = caption.toString();
		alert.text = text.toString();
		alert.onComplete = [onResult](DialogResult result) {
			if (result == DialogResult::Ok) {
				onResult(sl_true);
			} else {
				onResult(sl_false);
			}
		};
		alert.show();
	}
	
	void UI::showConfirm(const Ref<Window>& parent, const StringParam& text, const Function<void(sl_bool)>& onResult)
	{
		AlertDialog alert;
		alert.buttons = AlertDialogButtons::OkCancel;
		alert.parent = parent;
		alert.text = text.toString();
		alert.onComplete = [onResult](DialogResult result) {
			if (result == DialogResult::Ok) {
				onResult(sl_true);
			} else {
				onResult(sl_false);
			}
		};
		alert.show();
	}
	
	void UI::showConfirm(const Ref<Window>& parent, const StringParam& caption, const StringParam& text, const Function<void(sl_bool)>& onResult)
	{
		AlertDialog alert;
		alert.buttons = AlertDialogButtons::OkCancel;
		alert.parent = parent;
		alert.caption = caption.toString();
		alert.text = text.toString();
		alert.onComplete = [onResult](DialogResult result) {
			if (result == DialogResult::Ok) {
				onResult(sl_true);
			} else {
				onResult(sl_false);
			}
		};
		alert.show();
	}
	
#if !defined(SLIB_UI_IS_MACOS) && !defined(SLIB_UI_IS_GTK)
	void UI::dispatchToUiThreadUrgently(const Function<void()>& callback, sl_uint32 delayMillis)
	{
		dispatchToUiThread(callback, delayMillis);
	}
#endif
	
	void UI::runOnUiThread(const Function<void()>& callback)
	{
		if (callback.isNotNull()) {
			if (isUiThread()) {
				callback();
			} else {
				dispatchToUiThread(callback);
			}
		}
	}

	Function<void()> UI::getCallbackOnUiThread(const Function<void()>& callback)
	{
		if (callback.isNotNull()) {
			return static_cast<Callable<void()>*>(new UICallback(callback));
		}
		return sl_null;
	}

	Ref<Dispatcher> UI::getDispatcher()
	{
		return new DispatcherImpl();
	}

	void UI::runLoop()
	{
		if (g_flagQuitApp) {
			return;
		}
		if (!(UI::isUiThread())) {
			return;
		}
		g_nLevelRunLoop++;
		UIPlatform::runLoop(g_nLevelRunLoop);
		g_nLevelRunLoop--;
		if (g_flagQuitApp) {
			QuitLoop();
		}
	}

	void UI::quitLoop()
	{
		if (UI::isUiThread()) {
			QuitLoop();
		} else {
			UI::dispatchToUiThread(&QuitLoop);
		}
	}

	void UI::initApp()
	{
		if (g_flagInitializedApp) {
			return;
		}
		UIPlatform::initApp();
		g_flagInitializedApp = sl_true;
	}

	void UI::runApp()
	{
		initApp();
		if (g_flagQuitApp) {
			return;
		}
#ifndef SLIB_PLATFORM_IS_WINDOWS
		struct sigaction sa;
		Base::zeroMemory(&sa, sizeof(sa));
		sa.sa_handler = &TermHandler;
		sigaction(SIGTERM, &sa, sl_null);
#endif
		g_flagRunningApp = sl_true;
		UIPlatform::runApp();
	#if !defined(SLIB_PLATFORM_IS_ANDROID)
		g_flagRunningApp = sl_false;
	#endif
	}

	void UI::quitApp()
	{
		if (UI::isUiThread()) {
			QuitApp();
		} else {
			UI::dispatchToUiThread(&QuitApp);
		}
	}
	
	sl_bool UI::isRunningApp()
	{
		return g_flagRunningApp;
	}

	sl_bool UI::isQuitingApp()
	{
		return g_flagQuitApp;
	}

	void UI::openFile(const StringParam& path)
	{
		UI::openUrl(Url::toFileUri(path));
	}

	void UI::openDirectory(const StringParam& path)
	{
		String uri = Url::toFileUri(path);
		if (!(uri.endsWith('/'))) {
			uri += "/";
		}
		UI::openUrl(uri);
	}

#if !defined(SLIB_UI_IS_WIN32)
	void UI::openDirectoryAndSelectFile(const StringParam& path)
	{
		openDirectory(File::getParentDirectoryPath(path));
	}
#endif

#if !defined(SLIB_UI_IS_ANDROID)
	void UI::showKeyboard()
	{
	}
#endif
	
#if !defined(SLIB_UI_IS_IOS) && !defined(SLIB_UI_IS_ANDROID)
	void UI::dismissKeyboard()
	{
	}
#endif

#if !(defined(SLIB_UI_IS_GTK))
	void UI::getActiveApplicationAndWindow(String& appName, String& windowTitle)
	{
		appName = getActiveApplicationName();
		windowTitle = getActiveWindowTitle();
	}
#endif

#if !defined(SLIB_UI_IS_MACOS) && !defined(SLIB_UI_IS_WIN32) && !(defined(SLIB_UI_IS_GTK))
	String UI::getActiveApplicationName()
	{
		return sl_null;
	}

	String UI::getActiveWindowTitle()
	{
		return sl_null;
	}
#endif

}
