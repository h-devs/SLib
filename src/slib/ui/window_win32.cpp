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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_WIN32)

#include "window.h"

#include "view_win32.h"

#include "slib/dl/win32/dwmapi.h"
#include "slib/dl/win32/user32.h"

namespace slib
{

	namespace priv
	{
		void RunUiLoop(HWND hWndModalDialog);
		void PostCustomEvent(sl_uint32 type, HWND window);
		sl_bool IsAnyViewPainting();
	}

	using namespace priv;

	namespace
	{
		static void MakeWindowStyle(Window* window, DWORD& style, DWORD& styleEx, HMENU& hMenu)
		{
			Ref<Menu> menu = window->getMenu();
			hMenu = UIPlatform::getMenuHandle(menu.get());

			style = WS_CLIPCHILDREN;
			styleEx = WS_EX_CONTROLPARENT | WS_EX_NOPARENTNOTIFY;
			if (window->isBorderless() || window->isFullScreen() || window->isLayered()) {
				style |= WS_POPUP;
			} else {
				if (window->isTitleBarVisible()) {
					if (window->isDialog()) {
						style |= (WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_BORDER);
						styleEx |= WS_EX_DLGMODALFRAME;
					} else {
						style |= (WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION);
					}
				} else {
					style |= (WS_POPUP | WS_BORDER);
				}
				if (window->isMinimizeButtonEnabled()) {
					style |= WS_MINIMIZEBOX;
				}
				if (window->isMaximizeButtonEnabled()) {
					style |= WS_MAXIMIZEBOX;
				}
				if (window->isResizable()) {
					style |= WS_THICKFRAME;
				}
			}
			if (window->isLayered() || UIPlatform::getWindowAlpha(window->getAlpha()) != 255 || window->getColorKey().isNotZero()) {
				styleEx |= WS_EX_LAYERED;
			}
			if (window->isTransparent()) {
				styleEx |= WS_EX_TRANSPARENT;
			}
			if (!(window->isVisibleInTaskbar())) {
				styleEx |= WS_EX_TOOLWINDOW;
			}
			if (window->isAlwaysOnTop()) {
				styleEx |= WS_EX_TOPMOST;
			}
		}

		class Win32_WindowInstance : public WindowInstance
		{
		public:
			HWND m_handle;
			Ref<Menu> m_menu;

			sl_bool m_flagBorderless;
			sl_bool m_flagFullscreen;
			sl_bool m_flagModal;
			HWND m_hWndDisabledParent;

			sl_bool m_flagTitleBar;
			sl_bool m_flagResizable;

			sl_bool m_flagMinimized;
			sl_bool m_flagMaximized;

			AtomicRef<PlatformViewInstance> m_viewContent;
			sl_bool m_flagDestroyOnRelease;

			Color m_backgroundColor;

		public:
			Win32_WindowInstance()
			{
				m_handle = sl_null;

				m_flagBorderless = sl_false;
				m_flagFullscreen = sl_false;
				m_flagModal = sl_false;
				m_hWndDisabledParent = NULL;

				m_flagTitleBar = sl_false;
				m_flagResizable = sl_false;

				m_flagMinimized = sl_false;
				m_flagMaximized = sl_false;

				m_backgroundColor = Color::zero();
			}

			~Win32_WindowInstance()
			{
				close();
			}

		public:
			static Ref<Win32_WindowInstance> create(Window* window, HWND hWnd, sl_bool flagDestroyOnRelease)
			{
				if (hWnd) {
					Ref<Win32_WindowInstance> ret = new Win32_WindowInstance();
					if (ret.isNotNull()) {
						ret->initialize(window, hWnd, flagDestroyOnRelease);
						return ret;
					}
					if (flagDestroyOnRelease) {
						_destroy(hWnd);
					}
				}
				return sl_null;
			}

			static HWND createHandle(Window* window)
			{
				Win32_UI_Shared* shared = Win32_UI_Shared::get();
				if (!shared) {
					return NULL;
				}

				HINSTANCE hInst = shared->hInstance;
				ATOM atom;
				if (window->isCloseButtonEnabled()) {
					atom = shared->getWndClassForWindow();
				} else {
					atom = shared->getWndClassForWindowNoClose();
				}

				Ref<WindowInstance> parent;
				HWND hParent = (HWND)(window->getParentHandle(parent));

				HMENU hMenu;
				DWORD style;
				DWORD styleEx;
				MakeWindowStyle(window, style, styleEx, hMenu);

				UIRect frameWindow = MakeWindowFrame(window);

				StringCstr16 title = window->getTitle();

				HWND hWnd = CreateWindowExW(
					styleEx, // ex-style
					(LPCWSTR)((LONG_PTR)atom),
					(LPCWSTR)(title.getData()),
					style,
					(int)(frameWindow.left), (int)(frameWindow.top),
					(int)(frameWindow.getWidth()), (int)(frameWindow.getHeight()),
					hParent, // parent
					hMenu, // menu
					hInst,
					NULL);
				if (!hWnd) {
					return NULL;
				}

				String iconName = window->getIconResource();
				if (iconName.isNotNull()) {
					setIcon(hWnd, iconName);
				}

				if ((style & WS_THICKFRAME) && (style & WS_CAPTION) != WS_CAPTION && Win32::isWindows10OrGreater()) {
					auto api = dwmapi::getApi_DwmExtendFrameIntoClientArea();
					if (api) {
						dwmapi::MARGINS m = { -1 };
						api(hWnd, &m);
					}
				}
				if (styleEx & WS_EX_LAYERED) {
					if (!(window->isLayered())) {
						UIPlatform::initLayeredWindowAttributes(hWnd, UIPlatform::getWindowAlpha(window->getAlpha()), window->getColorKey());
					}
				}
				if (window->isExcludingFromCapture()) {
					auto api = user32::getApi_SetWindowDisplayAffinity();
					if (api) {
						api(hWnd, 0x11); // WDA_EXCLUDEFROMCAPTURE
					}
				}

				return hWnd;
			}

			void initialize(Window* window, HWND hWnd, sl_bool flagDestroyOnRelease)
			{
				m_handle = hWnd;
				m_flagDestroyOnRelease = flagDestroyOnRelease;
				if (window) {
					m_flagBorderless = window->isBorderless();
					m_flagFullscreen = window->isFullScreen();
					if (m_flagBorderless) {
						m_flagTitleBar = sl_false;
					} else {
						m_flagTitleBar = window->isTitleBarVisible();
					}
					m_flagResizable = window->isResizable();
					m_flagModal = window->isModal();
					if (window->isDefaultBackgroundColor()) {
						m_backgroundColor = window->getBackgroundColor();
					}
				}
				Ref<PlatformViewInstance> content = Ref<PlatformViewInstance>::cast(UIPlatform::createViewInstance(hWnd, sl_false));
				if (content.isNotNull()) {
					content->setWindowContent(sl_true);
					if (window->isLayered()) {
						content->initNativeLayer();
					}
					m_viewContent = content;
				}
				UIPlatform::registerWindowInstance(hWnd, this);
			}

			static void _destroy(HWND hWnd)
			{
				PostCustomEvent(SLIB_UI_EVENT_CLOSE_WINDOW, hWnd);
			}

		public:
			void* getHandle() override
			{
				return m_handle;
			}

			void close() override
			{
				ObjectLocker lock(this);
				HWND handle = m_handle;
				if (handle) {
					if (m_hWndDisabledParent) {
						EnableWindow(m_hWndDisabledParent, TRUE);
					}
					m_handle = NULL;
					UIPlatform::removeWindowInstance(handle);
					m_viewContent.setNull();
					if (m_flagDestroyOnRelease) {
						_destroy(handle);
					}
				}
			}

			sl_bool isClosed() override
			{
				return m_handle == NULL;
			}

			void setParentHandle(void* parent) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					if (parent) {
						SetWindowLongPtrW(hWnd, GWLP_HWNDPARENT, (LONG_PTR)parent);
					} else {
						SetWindowLongPtrW(hWnd, GWLP_HWNDPARENT, (LONG_PTR)NULL);
					}
				}
			}

			Ref<ViewInstance> getContentView() override
			{
				return m_viewContent;
			}

			sl_bool getFrame(UIRect& _out) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					RECT rect = { 0 };
					GetWindowRect(hWnd, &rect);
					_out.left = (sl_ui_pos)(rect.left);
					_out.top = (sl_ui_pos)(rect.top);
					_out.right = (sl_ui_pos)(rect.right);
					_out.bottom = (sl_ui_pos)(rect.bottom);
					return sl_true;
				}
				return sl_false;
			}

			void setFrame(const UIRect& frame) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					SetWindowPos(hWnd, NULL,
						(int)(frame.left), (int)(frame.top),
						(int)(frame.getWidth()), (int)(frame.getHeight()),
						SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
					applyRegion(hWnd, frame.getWidth(), frame.getHeight());
				}
			}

			void setTitle(const String& title) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					UIPlatform::setWindowText(hWnd, title);
				}
			}

			static void setIcon(HWND hWnd, const String& name)
			{
				Win32_UI_Shared* shared = Win32_UI_Shared::get();
				if (!shared) {
					return;
				}
				HICON hIcon;
				sl_uint32 resID;
				if (name.parseUint32(&resID)) {
					hIcon = LoadIconA(shared->hInstance, MAKEINTRESOURCEA(resID));
					if (!hIcon) {
						hIcon = LoadIconA(shared->hInstance, name.getData());
						if (!hIcon) {
							return;
						}
					}
				} else {
					hIcon = LoadIconA(shared->hInstance, name.getData());
					if (!hIcon) {
						return;
					}
				}
				HICON hIconOld = (HICON)(SendMessageW(hWnd, WM_SETICON, 0, (LPARAM)hIcon));
				if (hIconOld) {
					DeleteObject(hIconOld);
				}
			}

			void setIcon(const String& name) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					setIcon(hWnd, name);
				}
			}

			void setMenu(const Ref<Menu>& menu) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					_setMenu(hWnd, UIPlatform::getMenuHandle(menu.get()));
				}
			}

			void _setMenu(HWND hWnd, HMENU hMenu)
			{
				HMENU hMenuOld = GetMenu(hWnd);
				Ref<Menu> menuOld = UIPlatform::getMenu(hMenuOld);
				if (menuOld.isNull()) {
					DestroyMenu(hMenuOld);
				}
				SetMenu(hWnd, hMenu);
			}

			sl_bool isActive() override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					return hWnd == GetForegroundWindow();
				}
				return sl_false;
			}

			void activate() override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					SetForegroundWindow(hWnd);
				}
			}

			void setBackgroundColor(const Color& color) override
			{
				if (!(UI::isUiThread()) || IsAnyViewPainting()) {
					UI::dispatchToUiThreadUrgently(SLIB_BIND_WEAKREF(void(), this, setBackgroundColor, color));
					return;
				}
				m_backgroundColor = color;
				Ref<PlatformViewInstance> content = m_viewContent;
				if (content.isNotNull()) {
					Ref<View> view = content->getView();
					if (view.isNotNull()) {
						content->invalidate(view.get());
					}
				}
			}

			void isMinimized(sl_bool& _out) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					_out = IsIconic(hWnd) ? sl_true : sl_false;
				}
			}

			void setMinimized(sl_bool flag) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					if (flag) {
						ShowWindowAsync(hWnd, SW_MINIMIZE);
					} else {
						ShowWindowAsync(hWnd, SW_RESTORE);
					}
				}
			}

			void isMaximized(sl_bool& _out) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					_out = IsZoomed(hWnd) ? sl_true : sl_false;
				}
			}

			void setMaximized(sl_bool flag) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					if (flag) {
						ShowWindowAsync(hWnd, SW_MAXIMIZE);
					} else {
						ShowWindowAsync(hWnd, SW_RESTORE);
					}
				}
			}

			void setVisible(sl_bool flag) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					if (flag) {
						ShowWindowAsync(hWnd, SW_SHOW);
					} else {
						ShowWindowAsync(hWnd, SW_HIDE);
					}
				}
			}

			void setAlwaysOnTop(sl_bool flag) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					if (flag) {
						SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
							SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
					} else {
						SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
							SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
					}
				}
			}

			void setMinimizeButtonEnabled(sl_bool flag) override
			{
				if (m_flagFullscreen) {
					return;
				}
				if (m_flagTitleBar) {
					UIPlatform::setWindowStyle(m_handle, WS_MINIMIZEBOX, flag);
				}
			}

			void setMaximizeButtonEnabled(sl_bool flag) override
			{
				if (m_flagFullscreen) {
					return;
				}
				if (m_flagTitleBar) {
					UIPlatform::setWindowStyle(m_handle, WS_MAXIMIZEBOX, flag);
				}
			}

			void setResizable(sl_bool flag) override
			{
				m_flagResizable = flag;
				if (m_flagFullscreen) {
					return;
				}
				if (m_flagTitleBar) {
					UIPlatform::setWindowStyle(m_handle, WS_THICKFRAME, flag);
				}
			}

			void setAlpha(sl_real alpha) override
			{
				Ref<PlatformViewInstance> content = m_viewContent;
				if (content.isNotNull()) {
					Ref<View> view = content->getView();
					if (view.isNotNull()) {
						content->setAlpha(view.get(), alpha);
					}
				}
			}

			void setColorKey(const Color& color) override
			{
				Ref<PlatformViewInstance> content = m_viewContent;
				if (content.isNotNull()) {
					Ref<View> view = content->getView();
					if (view.isNotNull()) {
						content->setColorKey(view.get(), color);
					}
				}
			}

			void setTransparent(sl_bool flag) override
			{
				UIPlatform::setWindowExStyle(m_handle, WS_EX_TRANSPARENT, flag);
			}

			void setVisibleInTaskbar(sl_bool flag) override
			{
				UIPlatform::setWindowExStyle(m_handle, WS_EX_TOOLWINDOW, !flag);
			}

			void setExcludingFromCapture(sl_bool flag) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					auto api = user32::getApi_SetWindowDisplayAffinity();
					if (api) {
						api(hWnd, flag ?
							0x11 // WDA_EXCLUDEFROMCAPTURE
							: 0
						);
					}
				}
			}

			sl_bool getClientInsets(UIEdgeInsets& _out) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					POINT pt = { 0, 0 };
					if (ClientToScreen(hWnd, &pt)) {
						RECT rcWindow;
						if (GetWindowRect(hWnd, &rcWindow)) {
							RECT rcClient;
							if (GetClientRect(hWnd, &rcClient)) {
								_out.left = (sl_ui_len)(pt.x - rcWindow.left);
								_out.top = (sl_ui_len)(pt.y - rcWindow.top);
								_out.right = (sl_ui_len)(rcWindow.right - (pt.x + rcClient.right));
								_out.bottom = (sl_ui_len)(rcWindow.bottom - (pt.y + rcClient.bottom));
								return sl_true;
							}
						}
					}
				}
				return sl_false;
			}

			sl_bool doModal() override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					RunUiLoop(hWnd);
				}
				return sl_true;
			}

			void doPostCreate() override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					if (m_flagModal) {
						HWND hWndParent = GetWindow(hWnd, GW_OWNER);
						if (hWndParent) {
							if (hWndParent != GetDesktopWindow() && IsWindowEnabled(hWndParent)) {
								EnableWindow(hWndParent, FALSE);
								m_hWndDisabledParent = hWndParent;
							}
						}
					}
					if (GetWindowLongW(hWnd, GWL_STYLE) & WS_POPUP) {
						RECT rc = { 0 };
						GetClientRect(hWnd, &rc);
						WindowInstance::onResize((sl_ui_len)(rc.right), (sl_ui_len)(rc.bottom));
					}
				}
			}

			void onResize(HWND hWnd, sl_ui_pos width, sl_ui_pos height)
			{
				applyRegion(hWnd);
				WindowInstance::onResize(width, height);
			}

			void onAttachedContentView(View* content) override
			{
				HWND hWnd = m_handle;
				if (hWnd) {
					applyRegion(hWnd);
				}
			}

			void applyRegion(HWND hWnd)
			{
				RECT rc = { 0 };
				GetWindowRect(hWnd, &rc);
				applyRegion(hWnd, (sl_ui_pos)(rc.right - rc.left), (sl_ui_pos)(rc.bottom - rc.top));
			}

			void applyRegion(HWND hWnd, sl_ui_pos width, sl_ui_pos height)
			{
				Ref<ViewInstance> vi = getContentView();
				if (vi.isNotNull()) {
					Ref<View> view = vi->getView();
					if (view.isNotNull()) {
						BoundShape shape = view->getBoundShape();
						if (shape == BoundShape::Ellipse) {
							HRGN hRgn = CreateEllipticRgn(0, 0, (int)width, (int)height);
							if (hRgn) {
								SetWindowRgn(hWnd, hRgn, TRUE);
								DeleteObject(hRgn);
							}
						} else if (shape == BoundShape::RoundRect) {
							Size radius = view->getBoundRadius();
							HRGN hRgn = CreateRoundRectRgn(0, 0, (int)width, (int)height, (int)(radius.x), (int)(radius.y));
							if (hRgn) {
								SetWindowRgn(hWnd, hRgn, TRUE);
								DeleteObject(hRgn);
							}
						}
					}
				}
			}

			void _onClose()
			{
				HWND handle = m_handle;
				if (!handle) {
					return;
				}
				if (onClose()) {
					close();
				}
			}

			void _onResize(WPARAM wParam, LPARAM lParam)
			{
				HWND handle = m_handle;
				if (!handle) {
					return;
				}
				int width = LOWORD(lParam);
				int height = HIWORD(lParam);
				if (width < 0) {
					width = 0;
				}
				if (height < 0) {
					height = 0;
				}
				if (width > 60000) {
					width = 60000;
				}
				if (height > 60000) {
					height = 60000;
				}
				if (wParam == SIZE_MINIMIZED) {
					m_flagMinimized = sl_true;
					onMinimize();
				} else if (wParam == SIZE_MAXIMIZED) {
					if (m_flagMinimized) {
						m_flagMinimized = sl_false;
						onDeminimize();
					} else {
						m_flagMaximized = sl_true;
						onMaximize();
					}
					onResize(handle, (sl_ui_pos)width, (sl_ui_pos)height);
				} else if (wParam == SIZE_RESTORED) {
					if (m_flagMinimized) {
						m_flagMinimized = sl_false;
						onDeminimize();
					} else if (m_flagMaximized) {
						m_flagMaximized = sl_false;
						onDemaximize();
						onResize(handle, (sl_ui_pos)width, (sl_ui_pos)height);
					} else {
						onResize(handle, (sl_ui_pos)width, (sl_ui_pos)height);
					}
				}
			}

			void _onResizing(WPARAM wParam, LPARAM lParam)
			{
				HWND handle = m_handle;
				if (!handle) {
					return;
				}
				RECT rcClient = { 0 };
				RECT rcWindow = { 0 };
				GetClientRect(handle, &rcClient);
				GetWindowRect(handle, &rcWindow);
				sl_ui_len dw = (sl_ui_len)(rcWindow.right - rcWindow.left - rcClient.right);
				sl_ui_len dh = (sl_ui_len)(rcWindow.bottom - rcWindow.top - rcClient.bottom);

				RECT& rect = *(RECT*)(lParam);
				UISize size((sl_ui_pos)(rect.right - rect.left), (sl_ui_pos)(rect.bottom - rect.top));
				size.x -= dw;
				size.y -= dh;
				onResizing(size, wParam != WMSZ_TOP && wParam != WMSZ_BOTTOM);
				if (size.x < 0) {
					size.x = 0;
				}
				if (size.y < 0) {
					size.y = 0;
				}
				if (size.x > 60000) {
					size.x = 60000;
				}
				if (size.y > 60000) {
					size.y = 60000;
				}
				size.x += dw;
				size.y += dh;
				switch (wParam) {
					case WMSZ_TOPLEFT:
						rect.left = (int)(rect.right - size.x);
						rect.top = (int)(rect.bottom - size.y);
						break;
					case WMSZ_TOP:
						rect.right = (int)(rect.left + size.x);
						rect.top = (int)(rect.bottom - size.y);
						break;
					case WMSZ_TOPRIGHT:
						rect.right = (int)(rect.left + size.x);
						rect.top = (int)(rect.bottom - size.y);
						break;
					case WMSZ_LEFT:
						rect.left = (int)(rect.right - size.x);
						rect.bottom = (int)(rect.top + size.y);
						break;
					case WMSZ_RIGHT:
						rect.right = (int)(rect.left + size.x);
						rect.bottom = (int)(rect.top + size.y);
						break;
					case WMSZ_BOTTOMLEFT:
						rect.left = (int)(rect.right - size.x);
						rect.bottom = (int)(rect.top + size.y);
						break;
					case WMSZ_BOTTOM:
						rect.right = (int)(rect.left + size.x);
						rect.bottom = (int)(rect.top + size.y);
						break;
					case WMSZ_BOTTOMRIGHT:
						rect.right = (int)(rect.left + size.x);
						rect.bottom = (int)(rect.top + size.y);
						break;
				}
			}

			sl_bool _onNcHitTest(WPARAM wParam, LPARAM lParam, LRESULT& result)
			{
				HWND handle = m_handle;
				if (!handle) {
					return sl_false;
				}
				short x = (short)(lParam & 0xFFFF);
				short y = (short)((lParam >> 16) & 0xFFFF);
				Ref<Window> window = getWindow();
				if (window.isNotNull()) {
					Function<WindowPart(sl_ui_pos x, sl_ui_pos y)> tester = window->getHitTester();
					if (tester.isNotNull()) {
						WindowPart part = tester(x, y);
						if (part != WindowPart::Nowhere) {
							result = (LRESULT)part;
							return sl_true;
						}
					}
				}
				if (m_flagTitleBar) {
					return sl_false;
				}
				if (m_flagResizable && !(IsZoomed(handle))) {
					RECT rc = { 0 };
					GetWindowRect(handle, &rc);
#define BORDER_SIZE 4
					rc.left -= BORDER_SIZE;
					rc.top -= BORDER_SIZE;
					rc.right += BORDER_SIZE;
					rc.bottom += BORDER_SIZE;
					if (!(PtInRect(&rc, POINT{ x, y }))) {
						return sl_false;
					}
					rc.left += BORDER_SIZE * 2;
					rc.top += BORDER_SIZE * 2;
					rc.right -= BORDER_SIZE * 2;
					rc.bottom -= BORDER_SIZE * 2;
					if (x >= rc.right) {
						if (y >= rc.bottom) {
							result = HTBOTTOMRIGHT;
						} else if (y <= rc.top) {
							result = HTTOPRIGHT;
						} else {
							result = HTRIGHT;
						}
					} else if (x <= rc.left) {
						if (y >= rc.bottom) {
							result = HTBOTTOMLEFT;
						} else if (y <= rc.top) {
							result = HTTOPLEFT;
						} else {
							result = HTLEFT;
						}
					} else if (y >= rc.bottom) {
						result = HTBOTTOM;
					} else if (y <= rc.top) {
						result = HTTOP;
					} else {
						return sl_false;
					}
					return sl_true;
				}
				return sl_false;
			}

			sl_bool _onNcCalcSize(WPARAM wParam, LPARAM lParam, LRESULT& result)
			{
				HWND handle = m_handle;
				if (!handle) {
					return sl_false;
				}
				if (m_flagTitleBar || m_flagBorderless) {
					return sl_false;
				}
				if (m_flagResizable && Win32::isWindows10OrGreater()) {
					if (wParam) {
						result = 0;
						return sl_true;
					}
				}
				return sl_false;
			}

			sl_bool _onNcActivate(WPARAM wParam, LPARAM lParam, LRESULT& result)
			{
				HWND handle = m_handle;
				if (!handle) {
					return sl_false;
				}
				if (m_flagTitleBar || m_flagBorderless) {
					return sl_false;
				}
				if (m_flagResizable && Win32::isWindows10OrGreater()) {
					if (IsIconic(handle)) {
						return sl_false;
					}
					result = 1;
					return sl_true;
				}
				return sl_false;
			}

			sl_bool _onGetMinMaxInfo(WPARAM wParam, LPARAM lParam, LRESULT& result)
			{
				HWND handle = m_handle;
				if (!handle) {
					return sl_false;
				}
				if (m_flagTitleBar) {
					return sl_false;
				}
				MINMAXINFO* mmi = (MINMAXINFO*)lParam;
				HMONITOR hMonitor = MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST);
				if (hMonitor) {
					MONITORINFO mi;
					Base::zeroMemory(&mi, sizeof(mi));
					mi.cbSize = sizeof(mi);
					GetMonitorInfoW(hMonitor, &mi);
					mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
					result = 0;
					return sl_true;
				}
				return sl_false;
			}
		};
	}

	Ref<WindowInstance> Window::createWindowInstance()
	{
		HWND hWnd = Win32_WindowInstance::createHandle(this);
		if (hWnd) {
			return Win32_WindowInstance::create(this, hWnd, sl_true);
		}
		return sl_null;
	}

	Ref<Window> Window::getActiveWindow()
	{
		HWND hWnd = GetActiveWindow();
		if (hWnd) {
			Ref<WindowInstance> instance = UIPlatform::getWindowInstance(hWnd);
			if (instance.isNotNull()) {
				return instance->getWindow();
			}
		}
		return sl_null;
	}

	sl_bool Window::_getClientInsets(UIEdgeInsets& _out)
	{
		HMENU hMenu;
		DWORD style;
		DWORD styleEx;
		MakeWindowStyle(this, style, styleEx, hMenu);
		RECT rc = { 100, 100, 200, 200 };
		if (AdjustWindowRectEx(&rc, style, hMenu != sl_null, styleEx)) {
			_out.left = (sl_ui_len)(100 - rc.left);
			_out.top = (sl_ui_len)(100 - rc.top);
			_out.right = (sl_ui_len)(rc.right - 200);
			_out.bottom = (sl_ui_len)(rc.bottom - 200);
			return sl_true;
		}
		return sl_false;
	}


	Ref<WindowInstance> UIPlatform::createWindowInstance(HWND hWnd, sl_bool flagDestroyOnRelease)
	{
		Ref<WindowInstance> ret = UIPlatform::_getWindowInstance((void*)hWnd);
		if (ret.isNotNull()) {
			return ret;
		}
		return Win32_WindowInstance::create(sl_null, hWnd, flagDestroyOnRelease);
	}

	void UIPlatform::registerWindowInstance(HWND hWnd, WindowInstance* instance)
	{
		UIPlatform::_registerWindowInstance((void*)hWnd, instance);
	}

	Ref<WindowInstance> UIPlatform::getWindowInstance(HWND hWnd)
	{
		return UIPlatform::_getWindowInstance((void*)hWnd);
	}

	void UIPlatform::removeWindowInstance(HWND hWnd)
	{
		UIPlatform::_removeWindowInstance((void*)hWnd);
	}

	HWND UIPlatform::getWindowHandle(WindowInstance* _instance)
	{
		Win32_WindowInstance* instance = (Win32_WindowInstance*)_instance;
		if (instance) {
			return instance->m_handle;
		} else {
			return NULL;
		}
	}

	HWND UIPlatform::getWindowHandle(Window* window)
	{
		if (window) {
			Ref<WindowInstance> _instance = window->getWindowInstance();
			Win32_WindowInstance* instance = (Win32_WindowInstance*)(_instance.get());
			if (instance) {
				return instance->m_handle;
			}
		}
		return NULL;
	}

	namespace priv
	{
		LRESULT CALLBACK WindowInstanceProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			Ref<WindowInstance> _window = UIPlatform::getWindowInstance(hWnd);
			Win32_WindowInstance* window = (Win32_WindowInstance*)(_window.get());
			if (window && window->m_handle) {
				switch (uMsg) {
					case WM_CLOSE:
						window->_onClose();
						return 1;
					case WM_ACTIVATE:
						if (wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE) {
							window->onActivate();
						} else {
							window->onDeactivate();
						}
						break;
					case WM_SIZE:
						window->_onResize(wParam, lParam);
						break;
					case WM_SIZING:
						window->_onResizing(wParam, lParam);
						break;
					case WM_MOVE:
						window->onMove((sl_ui_pos)((short)(lParam & 0xFFFF)), (sl_ui_pos)((short)((lParam >> 16) & 0xFFFF)));
						break;
					case WM_NCHITTEST:
						{
							LRESULT result;
							if (window->_onNcHitTest(wParam, lParam, result)) {
								return result;
							}
							break;
						}
					case WM_NCCALCSIZE:
						{
							LRESULT result;
							if (window->_onNcCalcSize(wParam, lParam, result)) {
								return result;
							}
							break;
						}
					case WM_NCACTIVATE:
						{
							LRESULT result;
							if (window->_onNcActivate(wParam, lParam, result)) {
								return result;
							}
							break;
						}
					case WM_GETMINMAXINFO:
						{
							LRESULT result;
							if (window->_onGetMinMaxInfo(wParam, lParam, result)) {
								return result;
							}
							break;
						}
					case WM_KILLFOCUS:
						return 0;
				}
			}
			return ViewInstanceProc(hWnd, uMsg, wParam, lParam);
		}
	}

}

#endif
