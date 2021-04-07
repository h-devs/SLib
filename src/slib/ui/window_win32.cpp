/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

namespace slib
{

	namespace priv
	{

		namespace ui_core
		{
			void RunLoop(HWND hWndModalDialog);
		}

		namespace view
		{
			extern sl_bool g_flagDuringPaint;
		}

		namespace window
		{

			static sl_uint8 ToWindowAlpha(float alpha)
			{
				int a = (int)(alpha * 255);
				if (a < 0) {
					return 0;
				}
				if (a > 255) {
					return 255;
				}
				return (sl_uint8)a;
			}

			static void MakeWindowStyle(Window* window, DWORD& style, DWORD& styleEx, HMENU& hMenu)
			{
				hMenu = UIPlatform::getMenuHandle(window->getMenu());

				style = WS_CLIPCHILDREN;
				styleEx = WS_EX_CONTROLPARENT | WS_EX_NOPARENTNOTIFY;
				if (window->isBorderless() || window->isFullScreen()) {
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
					if (window->isLayered()) {
						styleEx |= WS_EX_LAYERED;
					}
				}
				sl_uint8 alpha = ToWindowAlpha(window->getAlpha());
				if (alpha < 255) {
					styleEx |= WS_EX_LAYERED;
				}
				if (window->isTransparent()) {
					styleEx |= WS_EX_TRANSPARENT;
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

				sl_bool m_flagResizable;
				sl_bool m_flagLayered;
				sl_uint8 m_alpha;

				sl_bool m_flagMinimized;
				sl_bool m_flagMaximized;

				AtomicRef<ViewInstance> m_viewContent;
				sl_bool m_flagDestroyOnRelease;

				Color m_backgroundColor;

			public:
				Win32_WindowInstance()
				{
					m_handle = sl_null;

					m_flagBorderless = sl_false;
					m_flagFullscreen = sl_false;

					m_flagResizable = sl_false;
					m_flagLayered = sl_false;
					m_alpha = 255;

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
							PostMessageW(hWnd, SLIB_UI_MESSAGE_CLOSE, 0, 0);
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

					Ref<Window> parent = window->getParent();
					HWND hParent = UIPlatform::getWindowHandle(parent.get());

					HMENU hMenu;
					DWORD style;
					DWORD styleEx;
					MakeWindowStyle(window, style, styleEx, hMenu);

					UIRect frameWindow = MakeWindowFrame(window);

					String16 title = String16::from(window->getTitle());

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

					if (hWnd) {
						sl_uint8 alpha = ToWindowAlpha(window->getAlpha());
						if (alpha < 255) {
							SetLayeredWindowAttributes(hWnd, 0, alpha, LWA_ALPHA);
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
						if (window->isDefaultBackgroundColor()) {
							m_backgroundColor = window->getBackgroundColor();
						}
						m_alpha = ToWindowAlpha(window->getAlpha());
					}
					Ref<ViewInstance> content = UIPlatform::createViewInstance(hWnd, sl_false);
					if (content.isNotNull()) {
						content->setWindowContent(sl_true);
						m_viewContent = content;
						if (window->isLayered()) {
							((Win32_ViewInstance*)(content.get()))->setLayered(sl_true);
						}
					}
					UIPlatform::registerWindowInstance(hWnd, this);
				}

				void close() override
				{
					ObjectLocker lock(this);
					HWND hWnd = m_handle;
					if (hWnd) {
						m_handle = NULL;
						UIPlatform::removeWindowInstance(hWnd);
						m_viewContent.setNull();
						if (m_flagDestroyOnRelease) {
							PostMessageW(hWnd, SLIB_UI_MESSAGE_CLOSE, 0, 0);
						}
					}
				}

				sl_bool isClosed() override
				{
					return m_handle == NULL;
				}

				void setParent(const Ref<WindowInstance>& window) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						if (window.isNotNull()) {
							Win32_WindowInstance* w = static_cast<Win32_WindowInstance*>(window.get());
							HWND hWndParent = w->m_handle;
							if (hWndParent) {
								SetWindowLongPtrW(hWnd, GWLP_HWNDPARENT, (LONG_PTR)hWndParent);
							}
						} else {
							SetWindowLongPtrW(hWnd, GWLP_HWNDPARENT, (LONG_PTR)NULL);
						}
					}
				}

				Ref<ViewInstance> getContentView() override
				{
					return m_viewContent;
				}

				UIRect getFrame() override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						RECT rect;
						GetWindowRect(hWnd, &rect);
						return UIRect((sl_ui_pos)(rect.left), (sl_ui_pos)(rect.top), (sl_ui_pos)(rect.right), (sl_ui_pos)(rect.bottom));
					} else {
						return UIRect::zero();
					}
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
						Windows::setWindowText(hWnd, title);
					}
				}

				void setMenu(const Ref<Menu>& menu) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						_setMenu(hWnd, UIPlatform::getMenuHandle(menu));
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
						return hWnd == GetActiveWindow();
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
					if (!(UI::isUiThread()) || priv::view::g_flagDuringPaint) {
						UI::dispatchToUiThreadUrgently(SLIB_BIND_WEAKREF(void(), WindowInstance, setBackgroundColor, this, color));
						return;
					}
					if (m_flagLayered) {
						redrawLayered();
					} else {
						HWND hWnd = m_handle;
						if (hWnd) {
							m_backgroundColor = color;
							InvalidateRect(hWnd, NULL, TRUE);
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
						sl_bool f1 = IsIconic(hWnd) ? sl_true : sl_false;
						sl_bool f2 = flag ? sl_true : sl_false;
						if (f1 != f2) {
							if (f2) {
								ShowWindowAsync(hWnd, SW_FORCEMINIMIZE);
							} else {
								ShowWindowAsync(hWnd, SW_RESTORE);
							}
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
						sl_bool f1 = IsZoomed(hWnd) ? sl_true : sl_false;
						sl_bool f2 = flag ? sl_true : sl_false;
						if (f1 != f2) {
							if (f2) {
								ShowWindowAsync(hWnd, SW_MAXIMIZE);
							} else {
								ShowWindowAsync(hWnd, SW_RESTORE);
							}
						}
					}
				}

				void setVisible(sl_bool flag) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						sl_bool f1 = IsWindowVisible(hWnd) ? sl_true : sl_false;
						sl_bool f2 = flag ? sl_true : sl_false;
						if (f1 != f2) {
							if (f2) {
								ShowWindowAsync(hWnd, SW_SHOW);
							} else {
								ShowWindowAsync(hWnd, SW_HIDE);
							}
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
					if (m_flagBorderless || m_flagFullscreen) {
						return;
					}
					Windows::setWindowStyle(m_handle, WS_MINIMIZEBOX, flag);
				}

				void setMaximizeButtonEnabled(sl_bool flag) override
				{
					if (m_flagBorderless || m_flagFullscreen) {
						return;
					}
					Windows::setWindowStyle(m_handle, WS_MAXIMIZEBOX, flag);
				}

				void setResizable(sl_bool flag) override
				{
					m_flagResizable = flag;
					if (m_flagBorderless || m_flagFullscreen) {
						return;
					}
					Windows::setWindowStyle(m_handle, WS_THICKFRAME, flag);
				}

				void setLayered(sl_bool flag) override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						if (m_flagLayered == flag) {
							return;
						}
						m_flagLayered = flag;
						Windows::setWindowExStyle(hWnd, WS_EX_LAYERED, flag);
						Ref<ViewInstance> instance = m_viewContent;
						if (instance.isNotNull()) {
							((Win32_ViewInstance*)(instance.get()))->setLayered(flag);
						}
					}
				}

				void setAlpha(sl_real alpha) override
				{
					if (m_flagLayered) {
						return;
					}
					HWND hWnd = m_handle;
					if (hWnd) {
						sl_uint8 a = ToWindowAlpha(alpha);
						if (m_alpha == a) {
							return;
						}
						m_alpha = a;
						Windows::setWindowExStyle(hWnd, WS_EX_LAYERED, m_alpha < 255);
						SetLayeredWindowAttributes(hWnd, 0, m_alpha, LWA_ALPHA);
						RedrawWindow(hWnd,
							NULL,
							NULL,
							RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
					}
				}

				void setTransparent(sl_bool flag) override
				{
					Windows::setWindowExStyle(m_handle, WS_EX_TRANSPARENT, flag);
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
									_out.right = (sl_ui_len)(rcWindow.right - (pt.x + rcClient.right - rcClient.left));
									_out.bottom = (sl_ui_len)(rcWindow.bottom - (pt.y + rcClient.bottom - rcClient.top));
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
						HWND hWndParent = Windows::getOwnerWindow(hWnd);
						sl_bool flagEnableParent = sl_false;
						if (hWndParent) {
							if (hWndParent != GetDesktopWindow() && IsWindowEnabled(hWndParent)) {
								EnableWindow(hWndParent, FALSE);
								flagEnableParent = sl_true;
							}
						}
						ui_core::RunLoop(hWnd);
						if (flagEnableParent) {
							EnableWindow(hWndParent, TRUE);
							SetForegroundWindow(hWndParent);
						}
					}
					return sl_true;
				}

				void doPostCreate()
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						if (GetWindowLongW(hWnd, GWL_STYLE) & WS_POPUP) {
							WindowInstance::onResize();
						}
					}
				}

				void redrawLayered()
				{
					Ref<ViewInstance> instance = m_viewContent;
					if (instance.isNotNull()) {
						((Win32_ViewInstance*)(instance.get()))->onDrawLayered();
					}
				}

				void onResize(HWND hWnd, sl_ui_pos width, sl_ui_pos height)
				{
					applyRegion(hWnd);
					WindowInstance::onResize(width, height);
				}

				void onAttachedContentView() override
				{
					HWND hWnd = m_handle;
					if (hWnd) {
						applyRegion(hWnd);
					}
				}

				void applyRegion(HWND hWnd)
				{
					RECT rc;
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

			};

			LRESULT CALLBACK WindowInstanceProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
			{
				Ref<WindowInstance> _window = UIPlatform::getWindowInstance(hWnd);
				Win32_WindowInstance* window = (Win32_WindowInstance*)(_window.get());
				if (window && window->m_handle) {
					switch (uMsg) {
					case WM_CLOSE:
						{
							if (window->onClose()) {
								window->close();
							}
							return 1;
						}
					case WM_ACTIVATE:
						{
							if (wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE) {
								window->onActivate();
							} else {
								window->onDeactivate();
							}
							break;
						}
					case WM_SIZE:
						{
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
							if (wParam == SIZE_MAXIMIZED) {
								window->m_flagMaximized = sl_true;
								window->onMaximize();
								window->onResize(hWnd, (sl_ui_pos)width, (sl_ui_pos)height);
							} else if (wParam == SIZE_MINIMIZED) {
								window->m_flagMinimized = sl_true;
								window->onMinimize();
							} else if (wParam == SIZE_RESTORED) {
								if (window->m_flagMinimized) {
									window->m_flagMinimized = sl_false;
									window->onDeminimize();
								} else if (window->m_flagMaximized) {
									window->m_flagMaximized = sl_false;
									window->onDemaximize();
									window->onResize(hWnd, (sl_ui_pos)width, (sl_ui_pos)height);
								} else {
									window->onResize(hWnd, (sl_ui_pos)width, (sl_ui_pos)height);
								}
							}
							break;
						}
					case WM_SIZING:
						{
							RECT rcClient = { 0 };
							RECT rcWindow = { 0 };
							GetClientRect(hWnd, &rcClient);
							GetWindowRect(hWnd, &rcWindow);
							sl_ui_len dw = (sl_ui_len)(rcWindow.right - rcWindow.left - (rcClient.right - rcClient.left));
							sl_ui_len dh = (sl_ui_len)(rcWindow.bottom - rcWindow.top - (rcClient.bottom - rcClient.top));

							RECT& rect = *(RECT*)(lParam);
							UISize size((sl_ui_pos)(rect.right - rect.left), (sl_ui_pos)(rect.bottom - rect.top));
							size.x -= dw;
							size.y -= dh;
							window->onResizing(size, wParam != WMSZ_TOP && wParam != WMSZ_BOTTOM);
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
							break;
						}
					case WM_MOVE:
						{
							window->onMove();
							break;
						}
					case WM_NCHITTEST:
						{
							if (window->m_flagBorderless) {
								if (window->m_flagResizable) {
									short x = (short)(lParam & 0xFFFF);
									short y = (short)((lParam >> 16) & 0xFFFF);
									RECT rc;
									GetWindowRect(hWnd, &rc);
#define BORDER_SIZE 4
									rc.left += BORDER_SIZE;
									rc.top += BORDER_SIZE;
									rc.right -= BORDER_SIZE;
									rc.bottom -= BORDER_SIZE;
									if (x >= rc.right) {
										if (y >= rc.bottom) {
											return HTBOTTOMRIGHT;
										}
										if (y <= rc.top) {
											return HTTOPRIGHT;
										}
										return HTRIGHT;
									}
									if (x <= rc.left) {
										if (y >= rc.bottom) {
											return HTBOTTOMLEFT;
										}
										if (y <= rc.top) {
											return HTTOPLEFT;
										}
										return HTLEFT;
									}
									if (y >= rc.bottom) {
										return HTBOTTOM;
									}
									if (y <= rc.top) {
										return HTTOP;
									}
								}
							}
							break;
						}
					case WM_GETMINMAXINFO:
						{
							if (window->m_flagBorderless) {
								MINMAXINFO* mmi = (MINMAXINFO*)lParam;
								HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
								if (hMonitor) {
									MONITORINFO mi;
									Base::zeroMemory(&mi, sizeof(mi));
									mi.cbSize = sizeof(mi);
									GetMonitorInfoW(hMonitor, &mi);
									mmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
									return 0;
								}
							}
							break;
						}
					}
				}
				return priv::view::ViewInstanceProc(hWnd, uMsg, wParam, lParam);
			}

		}
	}

	using namespace priv::window;

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

	HWND UIPlatform::getWindowHandle(WindowInstance* instance)
	{
		Win32_WindowInstance* window = (Win32_WindowInstance*)instance;
		if (window) {
			return window->m_handle;
		} else {
			return 0;
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
		return 0;
	}

}

#endif
