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

#include "slib/core/definition.h"

#if defined(SLIB_UI_IS_WIN32)

#include "slib/ui/global_event_monitor.h"

#include "slib/core/thread.h"
#include "slib/core/safe_static.h"
#include "slib/ui/platform.h"

namespace slib
{

	namespace priv
	{
		namespace global_event_monitor
		{

			class GlobalEventMonitorHelper : public GlobalEventMonitor
			{
			public:
				using GlobalEventMonitor::_onEvent;

				friend class Monitor;
			};

			static sl_bool RegisterDevice(HWND hWnd, USHORT usage)
			{
				RAWINPUTDEVICE hid;
				hid.usUsagePage = 1;
				hid.usUsage = usage;
				hid.dwFlags = RIDEV_INPUTSINK;
				hid.hwndTarget = hWnd;
				return RegisterRawInputDevices(&hid, 1, sizeof(RAWINPUTDEVICE)) != FALSE;
			}

			static void UnregisterDevice(USHORT usage)
			{
				RAWINPUTDEVICE hid;
				hid.usUsagePage = 1;
				hid.usUsage = usage;
				hid.dwFlags = RIDEV_REMOVE;
				hid.hwndTarget = NULL;
				RegisterRawInputDevices(&hid, 1, sizeof(RAWINPUTDEVICE));
			}

			static void ProcessRawInput(WPARAM wParam, LPARAM lParam)
			{
				HRAWINPUT hRawInput = (HRAWINPUT)lParam;
				static char bufRawInput[1024];
				RAWINPUT& raw = *((RAWINPUT*)bufRawInput);
				UINT uSize = sizeof(bufRawInput);
				UINT uRet = GetRawInputData(hRawInput, RID_INPUT, &raw, &uSize, sizeof(RAWINPUTHEADER));
				if (uRet < sizeof(RAWINPUTHEADER) || uRet > sizeof(bufRawInput)) {
					return;
				}
				if (raw.header.dwType == RIM_TYPEKEYBOARD) {
					UIAction action;
					switch (raw.data.keyboard.Message) {
					case WM_KEYDOWN:
					case WM_SYSKEYDOWN:
						action = UIAction::KeyDown;
						break;
					case WM_KEYUP:
					case WM_SYSKEYUP:
						action = UIAction::KeyUp;
						break;
					default:
						return;
					}
					sl_uint32 vkey = (sl_uint32)(raw.data.keyboard.VKey);
					sl_uint32 scanCode = (sl_uint32)(raw.data.keyboard.MakeCode);
					int extended = raw.data.keyboard.Flags & (RI_KEY_E0 | RI_KEY_E1);
					switch (vkey) {
					case VK_SHIFT:
						vkey = (unsigned char)(MapVirtualKeyW(scanCode, MAPVK_VSC_TO_VK_EX));
						break;
					case VK_CONTROL:
						vkey = extended ? VK_RCONTROL : VK_LCONTROL;
						break;
					case VK_MENU:
						vkey = extended ? VK_RMENU : VK_LMENU;
						break;
					}
					Keycode key = UIEvent::getKeycodeFromSystemKeycode(vkey);
					Time t;
					t.setMillisecondsCount(GetMessageTime());
					Ref<UIEvent> ev = UIEvent::createKeyEvent(action, key, vkey, t);
					if (ev.isNotNull()) {
						UIPlatform::applyEventModifiers(ev.get());
						GlobalEventMonitorHelper::_onEvent(ev.get());
					}
				} else if (raw.header.dwType == RIM_TYPEMOUSE) {
					sl_ui_posf x;
					sl_ui_posf y;
					if (raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
						x = (sl_ui_posf)(raw.data.mouse.lLastX);
						y = (sl_ui_posf)(raw.data.mouse.lLastY);
					} else {
						POINT pt;
						if (GetCursorPos(&pt)) {
							x = (sl_ui_posf)(pt.x);
							y = (sl_ui_posf)(pt.y);
						} else {
							x = 0;
							y = 0;
						}
					}
					Time t;
					t.setMillisecondsCount(GetMessageTime());
					sl_uint32 buttons = raw.data.mouse.usButtonFlags;
					if ((buttons & RI_MOUSE_WHEEL) || (buttons & 0x0800 /*RI_MOUSE_HWHEEL*/)) {
						sl_real dx = 0;
						sl_real dy = 0;
						if (buttons & RI_MOUSE_WHEEL) {
							dy = (sl_real)(short)(unsigned short)(raw.data.mouse.usButtonData);
						} else {
							dx = (sl_real)(short)(unsigned short)(raw.data.mouse.usButtonData);
						}
						Ref<UIEvent> ev = UIEvent::createMouseWheelEvent(x, y, dx, dy, t);
						if (ev.isNotNull()) {
							UIPlatform::applyEventModifiers(ev.get());
							GlobalEventMonitorHelper::_onEvent(ev.get());
						}
					} else {
						UIAction action;
						if (buttons & RI_MOUSE_LEFT_BUTTON_DOWN) {
							action = UIAction::LeftButtonDown;
						} else if (buttons & RI_MOUSE_LEFT_BUTTON_UP) {
							action = UIAction::LeftButtonUp;
						} else if (buttons & RI_MOUSE_RIGHT_BUTTON_DOWN) {
							action = UIAction::RightButtonDown;
						} else if (buttons & RI_MOUSE_RIGHT_BUTTON_UP) {
							action = UIAction::RightButtonUp;
						} else if (buttons & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
							action = UIAction::MiddleButtonDown;
						} else if (buttons & RI_MOUSE_MIDDLE_BUTTON_UP) {
							action = UIAction::MiddleButtonUp;
						} else {
							action = UIAction::MouseMove;
						}
						Ref<UIEvent> ev = UIEvent::createMouseEvent(action, x, y, t);
						if (ev.isNotNull()) {
							UIPlatform::applyEventModifiers(ev.get());
							GlobalEventMonitorHelper::_onEvent(ev.get());
						}
					}
				}
			}

			static LRESULT CALLBACK RawInputWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
			{
				switch (uMsg) {
				case WM_INPUT:
					ProcessRawInput(wParam, lParam);
					return 0;
				case WM_USER + 1:
					PostQuitMessage(0);
					break;
				}
				return DefWindowProcW(hWnd, uMsg, wParam, lParam);
			}

			static HWND g_hWnd = NULL;

			static void RunMonitor()
			{
				HINSTANCE hInstance = GetModuleHandleW(NULL);
				LPCWSTR szClassName = L"SLibGlobalEventMonitor";

				WNDCLASSEXW wc;
				Base::zeroMemory(&wc, sizeof(wc));
				wc.cbSize = sizeof(wc);
				wc.lpszClassName = szClassName;
				wc.lpfnWndProc = RawInputWindowProc;
				wc.hInstance = hInstance;
				RegisterClassExW(&wc);
				g_hWnd = CreateWindowW(szClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);
				if (!g_hWnd) {
					return;
				}

				MSG msg;
				while (GetMessageW(&msg, NULL, 0, 0)) {
					DispatchMessageW(&msg);
				}

				CloseWindow(g_hWnd);
				UnregisterClassW(szClassName, hInstance);

				g_hWnd = NULL;
			}

#define USAGE_MOUSE 2
#define USAGE_KEYBOARD 6

			class Monitor
			{
			public:
				sl_bool flagInstalledKeyboard;
				sl_bool flagInstalledMouse;
				Ref<Thread> thread;

			public:
				Monitor()
				{
					flagInstalledKeyboard = sl_false;
					flagInstalledMouse = sl_false;
				}

				~Monitor()
				{
					quitThread();
				}

			public:
				void quitThread()
				{
					if (thread.isNotNull()) {
						HWND hWnd = g_hWnd;
						if (hWnd) {
							PostMessageW(hWnd, WM_USER + 1, 0, 0);
						}
						thread->finishAndWait();
						thread.setNull();
					}
				}

				sl_bool update(sl_uint32 mask)
				{
					if (!mask) {
						if (flagInstalledKeyboard) {
							UnregisterDevice(USAGE_KEYBOARD);
							flagInstalledKeyboard = sl_false;
						}
						if (flagInstalledMouse) {
							UnregisterDevice(USAGE_MOUSE);
							flagInstalledMouse = sl_false;
						}
						quitThread();
						return sl_true;
					}
					HWND hWnd;
					for (;;) {
						hWnd = g_hWnd;
						if (hWnd) {
							break;
						}
						if (thread.isNull()) {
							thread = Thread::start(&RunMonitor);
							if (thread.isNull()) {
								flagInstalledKeyboard = sl_false;
								flagInstalledMouse = sl_false;
								return sl_false;
							}
						} else {
							if (thread->isRunning()) {
								Sleep(10);
							} else {
								flagInstalledKeyboard = sl_false;
								flagInstalledMouse = sl_false;
								thread.setNull();
								return sl_false;
							}
						}
					}
					if (mask & GlobalEventMonitorHelper::MASK_KEYBOARD) {
						if (!flagInstalledKeyboard) {
							flagInstalledKeyboard = RegisterDevice(hWnd, USAGE_KEYBOARD);
							if (!flagInstalledKeyboard) {
								return sl_false;
							}
						}
					} else {
						if (flagInstalledKeyboard) {
							UnregisterDevice(USAGE_KEYBOARD);
							flagInstalledKeyboard = sl_false;
						}
					}
					if (mask & GlobalEventMonitorHelper::MASK_MOUSE) {
						if (!flagInstalledMouse) {
							flagInstalledMouse = RegisterDevice(hWnd, USAGE_MOUSE);
							if (!flagInstalledMouse) {
								return sl_false;
							}
						}
					} else {
						if (flagInstalledMouse) {
							UnregisterDevice(USAGE_MOUSE);
							flagInstalledMouse = sl_false;
						}
					}
					return sl_true;
				}

			};

			Monitor* GetMonitor()
			{
				SLIB_SAFE_LOCAL_STATIC(Monitor, ret);
				if (SLIB_SAFE_STATIC_CHECK_FREED(ret)) {
					return sl_null;
				}
				return &ret;
			}

		}
	}

	using namespace priv::global_event_monitor;

	sl_bool GlobalEventMonitor::_updateMonitor(sl_uint32 mask)
	{
		Monitor* context = GetMonitor();
		if (!context) {
			return sl_false;
		}
		return context->update(mask);		
	}

}

#endif
