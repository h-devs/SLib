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

#ifndef CHECKHEADER_SLIB_UI_APP
#define CHECKHEADER_SLIB_UI_APP

#include "definition.h"

#include "../core/app.h"
#include "../core/function.h"

namespace slib
{

	class Window;
	class Menu;
	
	class SLIB_EXPORT UIApp : public Application
	{
		SLIB_DECLARE_OBJECT
		
	public:
		UIApp();
		
		~UIApp();

	public:
	
#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
		static void enableXlibThreadsSupport();
#endif

		static Ref<UIApp> getApp();
		
	public:
		AppType getAppType() override;
		
		static void quit();
		
		
		Ref<Window> getMainWindow();
		
		void setMainWindow(const Ref<Window>& window);
		
		Ref<Menu> getMenu();
		
		void setMenu(const Ref<Menu>& menu);
		
		
		virtual sl_bool shouldOpenUntitledFile();
		
		
		static sl_bool isMenuBarVisible();
		
		static void setMenuBarVisible(sl_bool flagVisible);
				
		static void setVisibleOnDock(sl_bool flagVisible);
		
		static void activate(sl_bool flagIgnoreOtherApps = sl_true);
		
		static void setBadgeNumber(sl_uint32 number);

	protected:
		void onInitApp() override;

		sl_int32 onRunApp() override;
		
		sl_int32 onExistingInstance() override;
		
	public:
		SLIB_DECLARE_EVENT_HANDLER(UIApp, Start)
		SLIB_DECLARE_EVENT_HANDLER(UIApp, Exit)
		
		SLIB_DECLARE_EVENT_HANDLER(UIApp, OpenUrl, const String& url, sl_bool& outFlagOpened)
		SLIB_DECLARE_EVENT_HANDLER(UIApp, OpenUrls, const List<String>& urls, sl_bool& outFlagOpened)
		SLIB_DECLARE_EVENT_HANDLER(UIApp, OpenFile, const String& filePath, sl_bool& outFlagOpened)
		SLIB_DECLARE_EVENT_HANDLER(UIApp, OpenFiles, const List<String>& files, sl_bool& outFlagOpened)
		SLIB_DECLARE_EVENT_HANDLER(UIApp, OpenTempFile, const String& filePath, sl_bool& outFlagOpened)
		SLIB_DECLARE_EVENT_HANDLER(UIApp, OpenUntitledFile, sl_bool& outFlagOpened)

		SLIB_DECLARE_EVENT_HANDLER(UIApp, Reopen, const String& commandLine, sl_bool flagHasVisibleWindows, sl_bool& outFlagPerformNormalTasks)

	public:
		static void dispatchStartToApp();
		static void dispatchExitToApp();
		
		static sl_bool dispatchOpenUrlToApp(const String& url);
		static sl_bool dispatchOpenUrlsToApp(const List<String>& urls);
		static sl_bool dispatchOpenFileToApp(const String& filePath);
		static sl_bool dispatchOpenFilesToApp(const List<String>& files);
		static sl_bool dispatchOpenTempFileToApp(const String& filePath);
		static sl_bool dispatchOpenUntitledFileToApp();

		static sl_bool dispatchReopenToApp(const String& commandLine, sl_bool flagHasVisibleWindows);

	private:
		AtomicRef<Window> m_mainWindow;
		AtomicRef<Menu> m_mainMenu;
		
	};

}

#endif
