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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_WIN32)

#include "ui_core_win32.h"

#include "ui_core_common.h"
#include "view_win32.h"

#include "slib/ui/screen.h"
#include "slib/ui/app.h"
#include "slib/ui/window.h"
#include "slib/io/file.h"
#include "slib/core/process.h"
#include "slib/core/queue.h"
#include "slib/core/safe_static.h"
#include "slib/core/scoped_buffer.h"

#include "slib/dl/win32/user32.h"

#include <commctrl.h>
#include <shobjidl.h>

#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#pragma comment(lib, "comctl32.lib")

namespace slib
{

	namespace {

		static sl_bool g_bFlagQuit = sl_false;

		struct CUSTOM_EVENT
		{
			sl_uint32 type;
			HWND window;
		};

		SLIB_SAFE_STATIC_GETTER(LinkedQueue<CUSTOM_EVENT>, GetCustomEventQueue)

		static sl_bool IsOwnedTo(HWND hWnd, HWND owner)
		{
			HWND hWndOwner = GetWindow(hWnd, GW_OWNER);
			while (hWndOwner) {
				if (hWndOwner == owner) {
					return sl_true;
				}
				hWndOwner = GetWindow(hWndOwner, GW_OWNER);
			}
			return sl_false;
		}

	}

	namespace priv
	{
		sl_bool CaptureChildInstanceEvents(View* view, MSG& msg);
		void ProcessMenuCommand(WPARAM wParam, LPARAM lParam);
		sl_bool ProcessMenuShortcutKey(MSG& msg);
		LRESULT CALLBACK WindowInstanceProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		WNDPROC g_wndProc_CustomMsgBox = NULL;
		WNDPROC g_wndProc_SystemTrayIcon = NULL;

		void PostCustomEvent(sl_uint32 type, HWND window)
		{
			LinkedQueue<CUSTOM_EVENT>* queue = GetCustomEventQueue();
			if (queue) {
				CUSTOM_EVENT msg;
				msg.type = type;
				msg.window = window;
				if (queue->getCount() < 65536) {
					queue->push(msg);
					PostMessageW(window, SLIB_UI_MESSAGE_CUSTOM_QUEUE, 0, 0);
				}
			}
		}

		void RunUiLoop(HWND hWndModalDialog)
		{
			if (g_bFlagQuit) {
				return;
			}
			LinkedQueue<CUSTOM_EVENT>* customQueue = GetCustomEventQueue();
			if (!customQueue) {
				return;
			}
			ReleaseCapture();
			MSG msg;
			while (GetMessageW(&msg, NULL, 0, 0)) {
				if (msg.message == WM_QUIT) {
					PostQuitMessage((int)(msg.wParam));
					return;
				} else if (msg.message == WM_MENUCOMMAND) {
					ProcessMenuCommand(msg.wParam, msg.lParam);
				} else {
					do {
						if (hWndModalDialog) {
							if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN || msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP) {
								if (hWndModalDialog != msg.hwnd && hWndModalDialog != GetAncestor(msg.hwnd, GA_ROOT)) {
									msg.hwnd = hWndModalDialog;
									SetFocus(hWndModalDialog);
								}
							}
						}
						if (ProcessMenuShortcutKey(msg)) {
							break;
						}
						Ref<Win32_ViewInstance> instance = Ref<Win32_ViewInstance>::from(UIPlatform::getViewInstance(msg.hwnd));
						if (instance.isNotNull()) {
							Ref<View> view = instance->getView();
							if (view.isNotNull()) {
								if (CaptureChildInstanceEvents(view.get(), msg)) {
									break;
								}
							}
						}
						TranslateMessage(&msg);
						DispatchMessageW(&msg);
					} while (0);
				}
				if (customQueue->isNotEmpty()) {
					CUSTOM_EVENT ev;
					while (customQueue->pop(&ev)) {
						if (ev.type == SLIB_UI_EVENT_QUIT_LOOP) {
							return;
						} else if (ev.type == SLIB_UI_EVENT_CLOSE_WINDOW) {
							if (hWndModalDialog) {
								if (ev.window == hWndModalDialog) {
									DestroyWindow(ev.window);
									return;
								}
								if (IsOwnedTo(hWndModalDialog, ev.window)) {
									PostCustomEvent(SLIB_UI_EVENT_CLOSE_WINDOW, ev.window);
									return;
								}
							}
							DestroyWindow(ev.window);
						}
					}
				}
				if (g_bFlagQuit) {
					break;
				}
			}
		}

	}

	using namespace priv;

	namespace {
		class MonitorScreen : public Screen
		{
		public:
			HMONITOR m_hMonitor;

		public:
			MonitorScreen(HMONITOR handle): m_hMonitor(handle)
			{
			}

		public:
			UIRect getRegion() override
			{
				MONITORINFOEXW info;
				Base::zeroMemory(&info, sizeof(info));
				info.cbSize = sizeof(info);
				if (GetMonitorInfoW(m_hMonitor, &info)) {
					RECT& rc = info.rcMonitor;
					return UIRect(
						(sl_ui_pos)(rc.left),
						(sl_ui_pos)(rc.top),
						(sl_ui_pos)(rc.right),
						(sl_ui_pos)(rc.bottom)
					);
				}
				return UIRect::zero();
			}

			UIRect getWorkingRegion() override
			{
				MONITORINFOEXW info;
				Base::zeroMemory(&info, sizeof(info));
				info.cbSize = sizeof(info);
				if (GetMonitorInfoW(m_hMonitor, &info)) {
					RECT& rc = info.rcWork;
					return UIRect(
						(sl_ui_pos)(rc.left),
						(sl_ui_pos)(rc.top),
						(sl_ui_pos)(rc.right),
						(sl_ui_pos)(rc.bottom)
					);
				}
				return UIRect::zero();
			}

		};

		static BOOL CALLBACK EnumAllDisplayMonitorsCallback(HMONITOR hMonitor, HDC hDC, LPRECT pClip, LPARAM lParam)
		{
			List< Ref<Screen> >& list = *((List< Ref<Screen> >*)lParam);
			Ref<Screen> screen = new MonitorScreen(hMonitor);
			if (screen.isNotNull()) {
				list.add_NoLock(Move(screen));
			}
			return TRUE;
		}

	}

	List< Ref<Screen> > UI::getScreens()
	{
		List< Ref<Screen> > ret;
		EnumDisplayMonitors(NULL, NULL, EnumAllDisplayMonitorsCallback, (LPARAM)&ret);
		return ret;
	}

	namespace {
		class PrimaryScreen : public Screen
		{
		public:
			UIRect getRegion() override
			{
				return UI::getScreenRegion();
			}

			UIRect getWorkingRegion() override
			{
				return UI::getScreenWorkingRegion();
			}
		};

		SLIB_SAFE_STATIC_GETTER(Ref<Screen>, GetPrimaryScreen, new PrimaryScreen())
	}

	Ref<Screen> UI::getPrimaryScreen()
	{
		Ref<Screen>* pScreen = GetPrimaryScreen();
		if (!pScreen) {
			return sl_null;
		}
		return *pScreen;
	}

	UIRect UI::getScreenRegion()
	{
		sl_ui_pos width = (sl_ui_pos)(GetSystemMetrics(SM_CXSCREEN));
		sl_ui_pos height = (sl_ui_pos)(GetSystemMetrics(SM_CYSCREEN));
		return UIRect(0, 0, width, height);
	}

	UIRect UI::getScreenWorkingRegion()
	{
		RECT rc;
		if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &rc, 0)) {
			return UIRect(
				(sl_ui_pos)(rc.left),
				(sl_ui_pos)(rc.top),
				(sl_ui_pos)(rc.right),
				(sl_ui_pos)(rc.bottom)
			);
		} else {
			return getScreenRegion();
		}
	}

	UISize UI::getScreenSize()
	{
		sl_ui_pos width = (sl_ui_pos)(GetSystemMetrics(SM_CXSCREEN));
		sl_ui_pos height = (sl_ui_pos)(GetSystemMetrics(SM_CYSCREEN));
		return UISize(width, height);
	}

	Ref<Canvas> UI::getScreenCanvas()
	{
		HDC hDC = GetDC(NULL);
		if (hDC) {
			Gdiplus::Graphics* graphics = new Gdiplus::Graphics(hDC);
			if (graphics) {
				UISize size = getScreenSize();
				return GraphicsPlatform::createCanvas(CanvasType::View, graphics, size.x, size.y, [hDC, graphics]() {
					delete graphics;
					ReleaseDC(NULL, hDC);
				});
			}
			ReleaseDC(NULL, hDC);
		}
		return sl_null;
	}

	namespace {

		static sl_bool g_bSetThreadMain = sl_false;
		static DWORD g_threadMain = 0;

		class MainThreadSetter
		{
		public:
			MainThreadSetter()
			{
				g_threadMain = GetCurrentThreadId();
				g_bSetThreadMain = sl_true;
			}
		};
		static MainThreadSetter g_seterMainThread;

		static void PostGlobalMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			Win32_UI_Shared* shared = Win32_UI_Shared::get();
			if (!shared) {
				return;
			}
			PostMessageW(shared->hWndMessage, uMsg, wParam, lParam);
		}

	}

	sl_bool UI::isUiThread()
	{
		if (g_bSetThreadMain) {
			return g_threadMain == GetCurrentThreadId();
		} else {
			return sl_true;
		}
	}

	void UI::dispatchToUiThread(const Function<void()>& callback, sl_uint32 delayMillis)
	{
		if (callback.isNull()) {
			return;
		}
		if (delayMillis) {
			Dispatch::setTimeout([callback]() {
				Win32_UI_Shared* shared = Win32_UI_Shared::get();
				if (shared) {
					sl_reg callbackId;
					if (UIDispatcher::addDelayedCallback(callback, callbackId)) {
						PostGlobalMessage(SLIB_UI_MESSAGE_DISPATCH_DELAYED, 0, (LPARAM)callbackId);
					}
				} else {
					UIDispatcher::addCallback(callback);
				}
			}, delayMillis);
		} else {
			if (UIDispatcher::addCallback(callback)) {
				PostGlobalMessage(SLIB_UI_MESSAGE_DISPATCH, 0, 0);
			}
		}
	}

	void UI::openUrl(const StringParam& url)
	{
		ShellExecuteParam param;
		param.operation = "open";
		param.path = url;
		Win32::shell(param);
	}

	void UI::openDirectoryAndSelectFile(const StringParam& _path)
	{
		String path = _path.toString().replaceAll('/', '\\');
		String dir = File::getParentDirectoryPath(path);
		ShellOpenFolderAndSelectItemsParam param;
		param.path = dir;
		param.items.add(path);
		Win32::shell(param);
	}

	String UI::getActiveApplicationName()
	{
		String ret;
		HWND hWnd = GetForegroundWindow();
		if (hWnd) {
			DWORD dwProcessId = 0;
			GetWindowThreadProcessId(hWnd, &dwProcessId);
			if (dwProcessId) {
				ret = File::getFileName(Process::getImagePath(dwProcessId));
			}
		}
		return ret;
	}

	String UI::getActiveWindowTitle()
	{
		HWND hWnd = GetForegroundWindow();
		if (hWnd) {
			return UIPlatform::getWindowText(hWnd);
		}
		return sl_null;
	}

	void UIPlatform::runLoop(sl_uint32 level)
	{
		RunUiLoop(NULL);
	}

	void UIPlatform::quitLoop()
	{
		PostCustomEvent(SLIB_UI_EVENT_QUIT_LOOP, NULL);
	}

	void UIPlatform::initApp()
	{
		GraphicsPlatform::startGdiplus();

		OleInitialize(NULL);

		INITCOMMONCONTROLSEX icex;
		icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_DATE_CLASSES;
		InitCommonControlsEx(&icex);

		Win32_UI_Shared::initialize();
	}

	void UIPlatform::runApp()
	{
		Win32_UI_Shared* shared = Win32_UI_Shared::get();
		if (!shared) {
			return;
		}

		Ref<UIApp> app = UIApp::getApp();
		if (app.isNotNull()) {
			String appId = app->getApplicationId();
			if (appId.isNotEmpty()) {
				UIPlatform::setWindowText(shared->hWndMessage, appId);
			}
		}

		UIDispatcher::processCallbacks();
		UIApp::Current::invokeStart();
		RunUiLoop(NULL);
		UIApp::Current::invokeExit();
	}

	void UIPlatform::quitApp()
	{
		g_bFlagQuit = sl_true;
		PostQuitMessage(0);
	}

	sl_bool UIPlatform::isWindowVisible(HWND hWnd)
	{
		if (!(IsWindow(hWnd))) {
			return sl_false;
		}
		if (!(IsWindowVisible(hWnd))) {
			return sl_false;
		}
		if (IsIconic(hWnd)) {
			return sl_false;
		}
		hWnd = GetAncestor(hWnd, GA_PARENT);
		if (hWnd) {
			return isWindowVisible(hWnd);
		}
		return sl_true;
	}

	String UIPlatform::getWindowText(HWND hWnd)
	{
		sl_int32 len = GetWindowTextLengthW(hWnd);
		if (len > 0) {
			SLIB_SCOPED_BUFFER(WCHAR, 1024, buf, len + 2);
			if (buf) {
				len = GetWindowTextW(hWnd, buf, len + 1);
				return String::create(buf, len);
			}
		}
		return sl_null;
	}

	String16 UIPlatform::getWindowText16(HWND hWnd)
	{
		int len = GetWindowTextLengthW(hWnd);
		if (len > 0) {
			String16 ret = String16::allocate(len);
			if (ret.isNotNull()) {
				int n = GetWindowTextW(hWnd, (LPWSTR)(ret.getData()), len + 1);
				if (n < len) {
					return ret.substring(0, n);
				} else {
					return ret;
				}
			}
		}
		return sl_null;
	}

	void UIPlatform::setWindowText(HWND hWnd, const StringParam& _str)
	{
		if (hWnd) {
			StringCstr16 str(_str);
			SetWindowTextW(hWnd, (LPCWSTR)(str.getData()));
		}
	}

	void UIPlatform::setWindowStyle(HWND hWnd, LONG flags, sl_bool flagAdd)
	{
		if (!hWnd) {
			return;
		}
		LONG old = GetWindowLongW(hWnd, GWL_STYLE);
		if (flagAdd) {
			SetWindowLongW(hWnd, GWL_STYLE, old | flags);
		} else {
			SetWindowLongW(hWnd, GWL_STYLE, old & (~flags));
		}
		SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
			SWP_FRAMECHANGED | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
	}

	void UIPlatform::removeAndAddWindowStyle(HWND hWnd, LONG flagsRemove, LONG flagsAdd)
	{
		if (!hWnd) {
			return;
		}
		LONG old = GetWindowLongW(hWnd, GWL_STYLE);
		SetWindowLongW(hWnd, GWL_STYLE, (old & (~flagsRemove)) | flagsAdd);
		SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
			SWP_FRAMECHANGED | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
	}

	void UIPlatform::setWindowExStyle(HWND hWnd, LONG flags, sl_bool flagAdd)
	{
		if (!hWnd) {
			return;
		}
		LONG old = GetWindowLongW(hWnd, GWL_EXSTYLE);
		if (flagAdd) {
			SetWindowLongW(hWnd, GWL_EXSTYLE, old | flags);
		} else {
			SetWindowLongW(hWnd, GWL_EXSTYLE, old & (~flags));
		}
		SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
			SWP_FRAMECHANGED | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
	}

	void UIPlatform::removeAndAddWindowExStyle(HWND hWnd, LONG flagsRemove, LONG flagsAdd)
	{
		if (!hWnd) {
			return;
		}
		LONG old = GetWindowLongW(hWnd, GWL_EXSTYLE);
		SetWindowLongW(hWnd, GWL_EXSTYLE, (old & (~flagsRemove)) | flagsAdd);
		SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
			SWP_FRAMECHANGED | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
	}

	sl_bool UIPlatform::processWindowHorizontalScrollEvents(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, sl_uint32 nLine, sl_uint32 nWheel)
	{
		int nSBCode = LOWORD(wParam);

		if (uMsg == WM_HSCROLL) {

			SCROLLINFO si;
			Base::zeroMemory(&si, sizeof(si));
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE | SIF_TRACKPOS;
			GetScrollInfo(hWnd, SB_HORZ, &si);
			switch (nSBCode) {
			case SB_TOP:
			case SB_LINEUP:
				si.nPos -= (int)nLine;
				break;
			case SB_BOTTOM:
			case SB_LINEDOWN:
				si.nPos += (int)nLine;
				break;
			case SB_PAGEUP:
				si.nPos -= si.nPage;
				break;
			case SB_PAGEDOWN:
				si.nPos += si.nPage;
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				si.nPos = si.nTrackPos;
				break;
			}

			if (si.nPos < si.nMin) {
				si.nPos = si.nMin;
			}
			if (si.nPos >= si.nMax) {
				si.nPos = si.nMax - 1;
			}

			si.fMask = SIF_POS;
			SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

			return sl_true;

		} else if (uMsg == 0x020E) {
			// WM_MOUSEHWHEEL
			int delta = (short)((wParam >> 16) & 0xffff);

			if (delta != 0) {

				SCROLLINFO si;
				Base::zeroMemory(&si, sizeof(si));
				si.cbSize = sizeof(si);
				si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
				GetScrollInfo(hWnd, SB_HORZ, &si);

				si.nPos += delta * (int)nWheel / WHEEL_DELTA;
				if (si.nPos < si.nMin) {
					si.nPos = si.nMin;
				}
				if (si.nPos >= si.nMax) {
					si.nPos = si.nMax - 1;
				}

				si.fMask = SIF_POS;
				SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool UIPlatform::processWindowVerticalScrollEvents(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, sl_uint32 nLine, sl_uint32 nWheel)
	{
		int nSBCode = LOWORD(wParam);

		if (uMsg == WM_VSCROLL) {

			SCROLLINFO si;
			Base::zeroMemory(&si, sizeof(si));
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE | SIF_TRACKPOS;
			GetScrollInfo(hWnd, SB_VERT, &si);

			switch (nSBCode) {
			case SB_TOP:
			case SB_LINEUP:
				si.nPos -= (int)nLine;
				break;
			case SB_BOTTOM:
			case SB_LINEDOWN:
				si.nPos += (int)nLine;
				break;
			case SB_PAGEUP:
				si.nPos -= si.nPage;
				break;
			case SB_PAGEDOWN:
				si.nPos += si.nPage;
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				si.nPos = si.nTrackPos;
				break;
			}

			if (si.nPos < si.nMin) {
				si.nPos = si.nMin;
			}
			if (si.nPos >= si.nMax) {
				si.nPos = si.nMax - 1;
			}
			si.fMask = SIF_POS;
			SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

			return sl_true;

		} else if (uMsg == WM_MOUSEWHEEL) {

			int delta = (short)((wParam >> 16) & 0xffff);

			if (delta != 0) {

				SCROLLINFO si;
				Base::zeroMemory(&si, sizeof(si));
				si.cbSize = sizeof(si);
				si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
				GetScrollInfo(hWnd, SB_VERT, &si);

				si.nPos -= delta * (int)nWheel / WHEEL_DELTA;
				if (si.nPos < si.nMin) {
					si.nPos = si.nMin;
				}
				if (si.nPos >= si.nMax) {
					si.nPos = si.nMax - 1;
				}

				si.fMask = SIF_POS;
				SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
			}
			return sl_true;

		}
		return sl_false;
	}

	void UIPlatform::setWindowHorizontalScrollParam(HWND hWnd, sl_int32 nMin, sl_int32 nMax, sl_int32 nPage)
	{
		if (nMax < nMin) {
			nMax = nMin;
		}
		SCROLLINFO si;
		Base::zeroMemory(&si, sizeof(si));
		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE | SIF_RANGE | SIF_DISABLENOSCROLL;
		si.nMin = nMin;
		si.nMax = nMax;
		si.nPage = nPage;
		SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);
	}

	void UIPlatform::setWindowVerticalScrollParam(HWND hWnd, sl_int32 nMin, sl_int32 nMax, sl_int32 nPage)
	{
		if (nMax < nMin) {
			nMax = nMin;
		}
		SCROLLINFO si;
		Base::zeroMemory(&si, sizeof(si));
		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE | SIF_RANGE | SIF_DISABLENOSCROLL;
		si.nMin = nMin;
		si.nMax = nMax;
		si.nPage = nPage;
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
	}

	sl_bool UIPlatform::registerTouchWindow(HWND hWnd)
	{
		auto func = user32::getApi_RegisterTouchWindow();
		if (func) {
			return func(hWnd, 0) != 0;
		}
		return sl_false;
	}

	void UIPlatform::unregisterTouchWindow(HWND hWnd)
	{
		auto func = user32::getApi_UnregisterTouchWindow();
		if (func) {
			func(hWnd);
		}
	}

#define MOUSEEVENTF_FROMTOUCH 0xFF515700

	sl_bool UIPlatform::isCurrentMessageFromTouch()
	{
		return (GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) == MOUSEEVENTF_FROMTOUCH;
	}

	void UIPlatform::initLayeredWindowAttributes(HWND hWnd, sl_uint8 alpha, const Color& colorKey)
	{
		DWORD flags = alpha == 255 ? 0 : LWA_ALPHA;
		COLORREF ck = 0;
		if (colorKey.isNotZero()) {
			ck = GraphicsPlatform::getColorRef(colorKey);
			flags |= LWA_COLORKEY;
		}
		SetLayeredWindowAttributes(hWnd, ck, alpha, flags);
	}

	void UIPlatform::updateLayeredWindowAttributes(HWND hWnd, sl_uint8 alpha, const Color& colorKey)
	{
		DWORD flags = LWA_ALPHA;
		COLORREF ck = 0;
		if (colorKey.isNotZero()) {
			ck = GraphicsPlatform::getColorRef(colorKey);
			flags |= LWA_COLORKEY;
		}
		SetLayeredWindowAttributes(hWnd, ck, alpha, flags);
	}

	sl_int32 UIApp::onExistingInstance()
	{
		StringCstr16 appId = getApplicationId();
		if (appId.isEmpty()) {
			return -1;
		}
		HWND hWnd = FindWindowW(PRIV_SLIB_UI_MESSAGE_WINDOW_CLASS_NAME, (LPCWSTR)(appId.getData()));
		if (hWnd) {
			COPYDATASTRUCT data;
			Base::zeroMemory(&data, sizeof(data));
			data.cbData = sizeof(data);
			LPWSTR sz = GetCommandLineW();
			data.lpData = sz;
			data.cbData = (DWORD)(Base::getStringLength2((sl_char16*)sz) * 2);
			SendMessageW(hWnd, WM_COPYDATA, 0, (LPARAM)&data);
			return 0;
		} else {
			return -1;
		}
	}

	namespace {

		static sl_uint32 g_nBadgeNumber = 0;

		static void ApplyBadgeNumber()
		{
			SLIB_SAFE_LOCAL_STATIC(Ref<Font>, font1, Font::create("Courier", 24, sl_true));
			if (SLIB_SAFE_STATIC_CHECK_FREED(font1)) {
				return;
			}
			SLIB_SAFE_LOCAL_STATIC(Ref<Font>, font2, Font::create("Courier", 20, sl_true));
			if (SLIB_SAFE_STATIC_CHECK_FREED(font2)) {
				return;
			}
			Ref<UIApp> app = UIApp::getApp();
			if (app.isNull()) {
				return;
			}
			Ref<Window> window = app->getMainWindow();
			HWND hWnd = UIPlatform::getWindowHandle(window.get());
			if (!hWnd) {
				return;
			}
			ITaskbarList3* pList = NULL;
			CoCreateInstance(__uuidof(TaskbarList), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pList));
			if (pList) {
				HRESULT hr = pList->HrInit();
				if (SUCCEEDED(hr)) {
					HICON hIcon = NULL;
					sl_uint32 n = g_nBadgeNumber;
					if (n > 0) {
						if (n >= 100) {
							n = 99;
						}
						Ref<Bitmap> bitmap = Bitmap::create(32, 32);
						if (bitmap.isNotNull()) {
							bitmap->resetPixels(Color::zero());
							Ref<Canvas> canvas = bitmap->getCanvas();
							if (canvas.isNotNull()) {
								canvas->setAntiAlias(sl_true);
								canvas->fillEllipse(0, 0, 32, 32, Color::Red);
								canvas->setAntiAlias(sl_false);
								canvas->drawText(String::fromUint32(n), Rectangle(0, 0, 32, 30), n < 10 ? font1 : font2, Color::White, Alignment::MiddleCenter);
								canvas.setNull();
								hIcon = GraphicsPlatform::createHICON(bitmap);
							}
						}
					}
					pList->SetOverlayIcon(hWnd, hIcon, L"Status");
					if (hIcon) {
						DestroyIcon(hIcon);
					}
				}
				pList->Release();
			}
		}

	}

	void UIApp::setBadgeNumber(sl_uint32 num)
	{
		g_nBadgeNumber = num;
		ApplyBadgeNumber();
	}


	namespace {

		static sl_bool g_flagInitializedSharedUIContext = sl_false;

		static Win32_UI_Shared* GetSharedUIContextInternal()
		{
			SLIB_SAFE_LOCAL_STATIC(Win32_UI_Shared, ret);
			if (SLIB_SAFE_STATIC_CHECK_FREED(ret)) {
				return sl_null;
			}
			return &ret;
		}

		static LRESULT CALLBACK MessageWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			static UINT uMsgTaskbarCreated = 0;
			static UINT uMsgTaskbarButtonCreated = 0;
			switch (uMsg) {
				case WM_CREATE:
					uMsgTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
					uMsgTaskbarButtonCreated = RegisterWindowMessageW(L"TaskbarButtonCreated");
					break;
				case SLIB_UI_MESSAGE_DISPATCH:
					UIDispatcher::processCallbacks();
					return 0;
				case SLIB_UI_MESSAGE_DISPATCH_DELAYED:
					UIDispatcher::processDelayedCallback((sl_reg)lParam);
					return 0;
				case SLIB_UI_MESSAGE_CLOSE_VIEW:
					DestroyWindow((HWND)lParam);
					break;
				case SLIB_UI_MESSAGE_CUSTOM_MSGBOX:
					if (g_wndProc_CustomMsgBox) {
						return g_wndProc_CustomMsgBox(hWnd, uMsg, wParam, lParam);
					}
					return 0;
				case SLIB_UI_MESSAGE_SYSTEM_TRAY_ICON:
					if (g_wndProc_SystemTrayIcon) {
						return g_wndProc_SystemTrayIcon(hWnd, uMsg, wParam, lParam);
					}
					return 0;
				case WM_MENUCOMMAND:
					ProcessMenuCommand(wParam, lParam);
					return 0;
				case WM_COPYDATA:
					{
						COPYDATASTRUCT* data = (COPYDATASTRUCT*)lParam;
						UIApp::Current::invokeReopen(String::fromUtf16((sl_char16*)(data->lpData), data->cbData / 2), sl_true);
					}
					return 0;
			}
			if (uMsgTaskbarCreated) {
				if (uMsg == uMsgTaskbarCreated) {
					if (g_wndProc_SystemTrayIcon) {
						g_wndProc_SystemTrayIcon(NULL, WM_CREATE, 0, 0);
					}
				}
			}
			if (uMsgTaskbarButtonCreated) {
				if (uMsg == uMsgTaskbarButtonCreated) {
					ApplyBadgeNumber();
				}
			}
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
		}

	}

	Win32_UI_Shared::Win32_UI_Shared()
	{

		hInstance = GetModuleHandleW(NULL);

		// register view class
		{
			WNDCLASSEXW wc;
			prepareClassForView(wc);
			wndClassForView = RegisterClassExW(&wc);
		}

		m_wndClassForWindow = 0;
		m_wndClassForWindowNoClose = 0;

		// Mesage Window
		{
			WNDCLASSW wc;
			Base::zeroMemory(&wc, sizeof(wc));
			wc.hInstance = hInstance;
			wc.lpfnWndProc = MessageWindowProc;
			wc.lpszClassName = PRIV_SLIB_UI_MESSAGE_WINDOW_CLASS_NAME;
			m_wndClassForMessage = RegisterClassW(&wc);
			hWndMessage = CreateWindowExW(0, (LPCWSTR)((LONG_PTR)m_wndClassForMessage), L"", 0, 0, 0, 0, 0, NULL, 0, hInstance, 0);
		}

	}

	Win32_UI_Shared::~Win32_UI_Shared()
	{
		if (hWndMessage) {
			DestroyWindow(hWndMessage);
		}
	}

	Win32_UI_Shared* Win32_UI_Shared::get()
	{
		if (g_flagInitializedSharedUIContext) {
			return GetSharedUIContextInternal();
		} else {
			return sl_null;
		}
	}

	void Win32_UI_Shared::initialize()
	{
		GetSharedUIContextInternal();
		g_flagInitializedSharedUIContext = sl_true;
	}

	ATOM Win32_UI_Shared::getWndClassForWindow()
	{
		if (m_wndClassForWindow) {
			return m_wndClassForWindow;
		}
		MutexLocker lock(&m_lock);
		if (m_wndClassForWindow) {
			return m_wndClassForWindow;
		}
		WNDCLASSEXW wc;
		prepareClassForWindow(wc);
		ATOM atom = RegisterClassExW(&wc);
		if (atom) {
			m_wndClassForWindow = atom;
		}
		return atom;
	}

	ATOM Win32_UI_Shared::getWndClassForWindowNoClose()
	{
		if (m_wndClassForWindowNoClose) {
			return m_wndClassForWindowNoClose;
		}
		MutexLocker lock(&m_lock);
		if (m_wndClassForWindowNoClose) {
			return m_wndClassForWindowNoClose;
		}
		WNDCLASSEXW wc;
		prepareClassForWindow(wc);
		wc.style |= CS_NOCLOSE;
		wc.lpszClassName = PRIV_SLIB_UI_NOCLOSE_WINDOW_CLASS_NAME;
		ATOM atom = RegisterClassExW(&wc);
		if (atom) {
			m_wndClassForWindowNoClose = atom;
		}
		return atom;
	}

	void Win32_UI_Shared::prepareClassForView(WNDCLASSEXW& wc)
	{
		Base::zeroMemory(&wc, sizeof(wc));
		wc.cbSize = sizeof(wc);
		wc.style = CS_DBLCLKS | CS_PARENTDC;
		wc.lpfnWndProc = ViewInstanceProc;
		wc.hInstance = hInstance;
		wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszClassName = PRIV_SLIB_UI_VIEW_WINDOW_CLASS_NAME;
	}

	namespace
	{
		static BOOL CALLBACK EnumApplicationIcon(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam
		)
		{
			WNDCLASSEXW& wc = *((WNDCLASSEXW*)lParam);
			wc.hIcon = LoadIconW(hModule, lpName);
			return FALSE;
		}
	}

	void Win32_UI_Shared::prepareClassForWindow(WNDCLASSEXW& wc)
	{
		Base::zeroMemory(&wc, sizeof(wc));
		wc.cbSize = sizeof(wc);
		wc.style = CS_DBLCLKS;
		wc.lpfnWndProc = WindowInstanceProc;
		wc.hInstance = hInstance;
		EnumResourceNamesW(hInstance, RT_GROUP_ICON, &EnumApplicationIcon, (LONG_PTR)&wc);
		wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszClassName = PRIV_SLIB_UI_GENERIC_WINDOW_CLASS_NAME;
	}

}

#endif
