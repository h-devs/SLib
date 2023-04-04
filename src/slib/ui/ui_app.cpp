/*
 *   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/app.h"

#include "slib/core/thread.h"
#include "slib/core/dispatch_loop.h"
#include "slib/io/async.h"
#include "slib/network/url_request.h"
#include "slib/ui/core.h"
#include "slib/ui/window.h"
#include "slib/ui/menu.h"
#include "slib/ui/platform.h"

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
#include "slib/dl/linux/x11.h"
#endif

#include "ui_core_common.h"

namespace slib
{

	using namespace priv;

	SLIB_DEFINE_OBJECT(UIApp, Application)

	UIApp::UIApp()
	{
	}

	UIApp::~UIApp()
	{
	}

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
	void UIApp::enableXlibThreadsSupport()
	{
		XInitThreads();
	}
#endif

	Ref<UIApp> UIApp::getApp()
	{
		return CastRef<UIApp>(Application::getApp());
	}

	AppType UIApp::getAppType()
	{
		return AppType::UI;
	}

	void UIApp::quit()
	{
		UI::quitApp();
	}

	Ref<Window> UIApp::getMainWindow()
	{
		return m_mainWindow;
	}

	void UIApp::setMainWindow(const Ref<Window>& window)
	{
		void* _thiz = this;
		if (_thiz) {
			m_mainWindow = window;
#ifdef SLIB_PLATFORM_IS_DESKTOP
			if (window.isNotNull()) {
				window->setQuitOnDestroy();
#ifdef SLIB_UI_IS_MACOS
				Ref<Menu> menu = window->getMenu();
				if (menu.isNotNull()) {
					setMenu(menu);
				}
#endif
			}
#endif
		}
	}

	Ref<Menu> UIApp::getMenu()
	{
		return m_mainMenu;
	}

	sl_bool UIApp::shouldOpenUntitledFile()
	{
		return sl_false;
	}

#if !defined(SLIB_UI_IS_MACOS)
	void UIApp::setMenu(const Ref<Menu>& menu)
	{
		m_mainMenu = menu;
	}

	sl_bool UIApp::isMenuBarVisible()
	{
		return sl_false;
	}

	void UIApp::setMenuBarVisible(sl_bool flagVisible)
	{
	}

	void UIApp::setVisibleOnDock(sl_bool flagVisible)
	{
	}

	void UIApp::activate(sl_bool flagIgnoreOtherApps)
	{
	}
#endif

#if !defined(SLIB_UI_IS_IOS) && !defined(SLIB_UI_IS_MACOS) && !defined(SLIB_UI_IS_ANDROID) && !defined(SLIB_UI_IS_WIN32)
	void UIApp::setBadgeNumber(sl_uint32 number)
	{
	}
#endif

	void UIApp::onInitApp()
	{
		UI::initApp();
	}

	sl_int32 UIApp::onRunApp()
	{
		UI::runApp();
		return 0;
	}

#if !defined(SLIB_UI_IS_MACOS) && !defined(SLIB_UI_IS_WIN32) && !defined(SLIB_UI_IS_GTK)
	sl_int32 UIApp::onExistingInstance()
	{
		return -1;
	}
#endif


	SLIB_DEFINE_EVENT_HANDLER(UIApp, Start, ())

	void UIApp::handleStart()
	{
		UrlRequest::setDefaultDispatcher(UI::getDispatcher());
		invokeStart();
	}

	void UIApp::Current::invokeStart()
	{
		Ref<UIApp> app = getApp();
		if (app.isNotNull()) {
			app->handleStart();
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(UIApp, Exit, ())

	void UIApp::Current::invokeExit()
	{
		Ref<UIApp> app = getApp();
		if (app.isNotNull()) {
			app->invokeExit();
		}
		UIDispatcher::removeAllCallbacks();
		DispatchLoop::releaseDefault();
		AsyncIoLoop::releaseDefault();
		Thread::finishAllThreads();
	}

	SLIB_DEFINE_EVENT_HANDLER(UIApp, OpenUrl, (const String& url, sl_bool& outFlagOpened), url, outFlagOpened)

	sl_bool UIApp::Current::invokeOpenUrl(const String& url)
	{
		sl_bool bRet = sl_false;
		Ref<UIApp> app = getApp();
		if (app.isNotNull()) {
			app->invokeOpenUrl(url, bRet);
		}
		return bRet;
	}

	SLIB_DEFINE_EVENT_HANDLER(UIApp, OpenUrls, (const List<String>& urls, sl_bool& outFlagOpened), urls, outFlagOpened)

	sl_bool UIApp::Current::invokeOpenUrls(const List<String>& urls)
	{
		sl_bool bRet = sl_false;
		Ref<UIApp> app = getApp();
		if (app.isNotNull()) {
			app->invokeOpenUrls(urls, bRet);
		}
		return bRet;
	}

	SLIB_DEFINE_EVENT_HANDLER(UIApp, OpenFile, (const String& filePath, sl_bool& outFlagOpened), filePath, outFlagOpened)

	sl_bool UIApp::Current::invokeOpenFile(const String& filePath)
	{
		sl_bool bRet = sl_false;
		Ref<UIApp> app = getApp();
		if (app.isNotNull()) {
			app->invokeOpenFile(filePath, bRet);
		}
		return bRet;
	}

	SLIB_DEFINE_EVENT_HANDLER(UIApp, OpenFiles, (const List<String>& files, sl_bool& outFlagOpened), files, outFlagOpened)

	sl_bool UIApp::Current::invokeOpenFiles(const List<String>& files)
	{
		sl_bool bRet = sl_false;
		Ref<UIApp> app = getApp();
		if (app.isNotNull()) {
			app->invokeOpenFiles(files, bRet);
		}
		return bRet;
	}

	SLIB_DEFINE_EVENT_HANDLER(UIApp, OpenTempFile, (const String& filePath, sl_bool& outFlagOpened), filePath, outFlagOpened)

	sl_bool UIApp::Current::invokeOpenTempFile(const String& filePath)
	{
		sl_bool bRet = sl_false;
		Ref<UIApp> app = getApp();
		if (app.isNotNull()) {
			app->invokeOpenTempFile(filePath, bRet);
		}
		return bRet;
	}

	SLIB_DEFINE_EVENT_HANDLER(UIApp, OpenUntitledFile, (sl_bool& outFlagOpened), outFlagOpened)

	sl_bool UIApp::Current::invokeOpenUntitledFile()
	{
		sl_bool bRet = sl_false;
		Ref<UIApp> app = getApp();
		if (app.isNotNull()) {
			app->invokeOpenUntitledFile(bRet);
		}
		return bRet;
	}

	SLIB_DEFINE_EVENT_HANDLER(UIApp, Reopen, (const String& commandLine, sl_bool flagHasVisibleWindows, sl_bool& outFlagPerformNormalTasks), commandLine, flagHasVisibleWindows, outFlagPerformNormalTasks)

	sl_bool UIApp::Current::invokeReopen(const String& commandLine, sl_bool flagHasVisibleWindows)
	{
		sl_bool bRet = sl_true;
		Ref<UIApp> app = getApp();
		if (app.isNotNull()) {
			app->invokeReopen(commandLine, flagHasVisibleWindows, bRet);
		}
		return bRet;
	}

}
