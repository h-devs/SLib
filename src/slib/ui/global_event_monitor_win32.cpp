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

#include "slib/core/definition.h"

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/ui/global_event_monitor.h"

#include "slib/platform/win32/message_loop.h"

#define USAGE_MOUSE 2
#define USAGE_KEYBOARD 6

namespace slib
{

	namespace
	{
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

		class RawInputMonitor : public GlobalEventMonitor
		{
		public:
			GlobalEventMask m_mask;
			sl_bool m_flagKeyboard = sl_false;
			sl_bool m_flagMouse = sl_false;

			Ref<win32::MessageLoop> m_loop;

		public:
			RawInputMonitor()
			{
			}

			~RawInputMonitor()
			{
				release();
			}

		public:
			static Ref<RawInputMonitor> create(const GlobalEventMonitorParam& param)
			{
				sl_bool flagKeyboard = param.flagKeyDown || param.flagKeyUp;
				sl_bool flagMouse = param.flagLeftButtonDown || param.flagLeftButtonUp || param.flagRightButtonDown || param.flagRightButtonUp || param.flagMiddleButtonDown || param.flagMiddleButtonUp || param.flagMouseMove || param.flagMouseWheel;
				if (!flagKeyboard && !flagMouse) {
					return sl_null;
				}
				Ref<RawInputMonitor> ret = new RawInputMonitor;
				if (ret.isNull()) {
					return sl_null;
				}
				win32::MessageLoopParam param;
				param.name = SLIB_UNICODE("GlobalEventMonitor");
				param.onMessage = SLIB_FUNCTION_WEAKREF(ret, processMessage);
				param.flagAutoStart = sl_false;
				loop = win32::MessageLoop::create(param);
				if (loop.isNull()) {
					return sl_null;
				}
				HWND hWnd = loop->getWindowHandle();
				if (!hWnd) {
					return sl_null;
				}
				if (flagKeyboard) {
					if (!(RegisterDevice(hWnd, USAGE_KEYBOARD))) {
						return sl_null;
					}
					ret->flagKeyboard = sl_true;
				}
				if (flagMouse) {
					if (!(RegisterDevice(hWnd, USAGE_MOUSE))) {
						return sl_null;
					}
					ret->flagMouse = sl_true;
				}
				ret->m_mask = param;
				ret->_initialize(param);
				loop->start();
				ret->m_loop = Move(loop);
				return ret;
			}

		public:
			void release() override
			{
				ObjectLocker lock(this);
				Ref<win32::MessageLoop> loop = Move(m_loop);
				sl_bool flagKeyboard = m_flagKeyboard;
				m_flagKeyboard = sl_false;
				sl_bool flagMouse = m_flagMouse;
				m_flagMouse = sl_false;
				lock.unlock();
				if (flagKeyboard) {
					UnregisterDevice(USAGE_KEYBOARD);
				}
				if (flagMouse) {
					UnregisterDevice(USAGE_MOUSE);
				}
				if (loop.isNotNull()) {
					loop->stop();
				}
			}

			void prepareEvent(UIEvent* ev, RAWINPUT& raw)
			{
				Time t;
				t.setMillisecondCount(GetMessageTime());
				UIPlatform::applyEventModifiers(ev.get());
				if (!(raw.header.hDevice)) {
					ev->addFlag(UIEventFlags::Injected);
				}
			}

			void processKeyEvent(UIAction action, RAWINPUT& raw)
			{
				sl_uint32 vkey = (sl_uint32)(raw.data.keyboard.VKey);
				if (vkey == 255) {
					return;
				}
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
				Ref<UIEvent> ev = UIEvent::createKeyEvent(action, key, vkey, Time::zero());
				if (ev.isNotNull()) {
					prepareEvent(ev.get(), raw);
					_onEvent(ev.get());
				}
			}

			void processMouseEvent(UIAction action, RAWINPUT& raw)
			{
				sl_ui_posf x;
				sl_ui_posf y;
				sl_ui_posf dx = 0.0f;
				sl_ui_posf dy = 0.0f;
				sl_bool flagDelta = sl_false;
				if (raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
					x = (sl_ui_posf)((raw.data.mouse.lLastX * GetSystemMetrics(SM_CXSCREEN)) >> 16);
					y = (sl_ui_posf)((raw.data.mouse.lLastY * GetSystemMetrics(SM_CYSCREEN)) >> 16);
				} else {
					POINT pt;
					if (GetCursorPos(&pt)) {
						x = (sl_ui_posf)(pt.x);
						y = (sl_ui_posf)(pt.y);
					} else {
						x = 0.0f;
						y = 0.0f;
					}
					dx = (sl_ui_posf)(raw.data.mouse.lLastX);
					dy = (sl_ui_posf)(raw.data.mouse.lLastY);
					flagDelta = sl_true;
				}
				Ref<UIEvent> ev;
				if (flagDelta) {
					ev = UIEvent::createMouseEvent(action, x, y, dx, dy, Time::zero());
				} else {
					ev = UIEvent::createMouseEvent(action, x, y, Time::zero());
				}
				if (ev.isNotNull()) {
					prepareEvent(ev.get(), raw);
					_onEvent(ev.get());
				}
			}

			void processMouseWheelEvent(RAWINPUT& raw)
			{
				sl_ui_posf x;
				sl_ui_posf y;
				if (raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
					x = (sl_ui_posf)((raw.data.mouse.lLastX * GetSystemMetrics(SM_CXSCREEN)) >> 16);
					y = (sl_ui_posf)((raw.data.mouse.lLastY * GetSystemMetrics(SM_CYSCREEN)) >> 16);
				} else {
					POINT pt;
					if (GetCursorPos(&pt)) {
						x = (sl_ui_posf)(pt.x);
						y = (sl_ui_posf)(pt.y);
					} else {
						x = 0.0f;
						y = 0.0f;
					}
				}
				sl_real dx = 0.0f;
				sl_real dy = 0.0f;
				if (buttons & RI_MOUSE_WHEEL) {
					dy = (sl_real)((short)((unsigned short)(raw.data.mouse.usButtonData)));
				} else {
					dx = (sl_real)((short)((unsigned short)(raw.data.mouse.usButtonData)));
				}
				Ref<UIEvent> ev = UIEvent::createMouseWheelEvent(x, y, dx, dy, Time::zero());
				if (ev.isNotNull()) {
					prepareEvent(ev.get(), raw);
					_onEvent(ev.get());
				}
			}

			void processRawInput(WPARAM wParam, LPARAM lParam)
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
							if (m_mask.flagKeyDown) {
								processKeyEvent(UIAction::KeyDown, raw);
							}
							break;
						case WM_KEYUP:
						case WM_SYSKEYUP:
							if (m_mask.flagKeyUp) {
								processKeyEvent(UIAction::KeyUp, raw);
							}
							break;
						default:
							break;
					}
				} else if (raw.header.dwType == RIM_TYPEMOUSE) {
					sl_uint32 buttons = raw.data.mouse.usButtonFlags;
					if ((buttons & RI_MOUSE_WHEEL) || (buttons & 0x0800 /*RI_MOUSE_HWHEEL*/)) {
						if (m_mask.flagMouseWheel) {
							processMouseWheelEvent(raw);
						}
					} else {
						UIAction action;
						if (buttons & RI_MOUSE_LEFT_BUTTON_DOWN) {
							if (m_mask.flagLeftButtonDown) {
								processMouseEvent(UIAction::LeftButtonDown, raw);
							}
						} else if (buttons & RI_MOUSE_LEFT_BUTTON_UP) {
							if (m_mask.flagLeftButtonUp) {
								processMouseEvent(UIAction::LeftButtonUp, raw);
							}
						} else if (buttons & RI_MOUSE_RIGHT_BUTTON_DOWN) {
							if (m_mask.flagRightButtonDown) {
								processMouseEvent(UIAction::RightButtonDown, raw);
							}
						} else if (buttons & RI_MOUSE_RIGHT_BUTTON_UP) {
							if (m_mask.flagRightButtonUp) {
								processMouseEvent(UIAction::RightButtonUp, raw);
							}
						} else if (buttons & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
							if (m_mask.flagMiddleButtonDown) {
								processMouseEvent(UIAction::MiddleButtonDown, raw);
							}
						} else if (buttons & RI_MOUSE_MIDDLE_BUTTON_UP) {
							if (m_mask.flagMiddleButtonUp) {
								processMouseEvent(UIAction::MiddleButtonUp, raw);
							}
						} else {
							if (m_mask.flagMouseMove) {
								processMouseEvent(UIAction::MouseMove, raw);
							}
						}
					}
				}
			}

			sl_bool processMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
			{
				if (uMsg == WM_INPUT) {
					processRawInput(wParam, lParam);
					return sl_true;
				}
				return sl_false;
			}
		};
	}

	Ref<GlobalEventMonitor> GlobalEventMonitor::create(const GlobalEventMonitorParam& param)
	{
		return Ref<GlobalEventMonitor>::cast(RawInputMonitor::create(param));
	}

}

#endif
