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

#include "slib/ui/event.h"

#include "slib/core/hash_table.h"
#include "slib/core/safe_static.h"
#include "slib/ui/core.h"
#include "slib/ui/platform.h"

namespace slib
{

	namespace priv
	{
		namespace ui_event
		{

			class KeyMapper
			{
			private:
				HashTable<sl_uint32, sl_uint32> mapKeyToVK;
				HashTable<sl_uint32, sl_uint32> mapVKToKey;

			public:
				KeyMapper()
				{
					map(Keycode::Tab, VK_TAB);
					map(Keycode::Enter, VK_RETURN);
					map(Keycode::Escape, VK_ESCAPE);

					map(Keycode::Space, VK_SPACE);
					map(Keycode::Grave, VK_OEM_3);
					map(Keycode::Equal, VK_OEM_PLUS);
					map(Keycode::Semicolon, VK_OEM_1);
					map(Keycode::Backslash, VK_OEM_5);
					map(Keycode::LeftBaracket, VK_OEM_4);
					map(Keycode::RightBaracket, VK_OEM_6);
					map(Keycode::Quote, VK_OEM_7);
					map(Keycode::Comma, VK_OEM_COMMA);
					map(Keycode::Minus, VK_OEM_MINUS);
					map(Keycode::Period, VK_OEM_PERIOD);
					map(Keycode::Divide, VK_OEM_2);

					map(Keycode::Num0, '0');
					map(Keycode::Num1, '1');
					map(Keycode::Num2, '2');
					map(Keycode::Num3, '3');
					map(Keycode::Num4, '4');
					map(Keycode::Num5, '5');
					map(Keycode::Num6, '6');
					map(Keycode::Num7, '7');
					map(Keycode::Num8, '8');
					map(Keycode::Num9, '9');

					map(Keycode::A, 'A');
					map(Keycode::B, 'B');
					map(Keycode::C, 'C');
					map(Keycode::D, 'D');
					map(Keycode::E, 'E');
					map(Keycode::F, 'F');
					map(Keycode::G, 'G');
					map(Keycode::H, 'H');
					map(Keycode::I, 'I');
					map(Keycode::J, 'J');
					map(Keycode::K, 'K');
					map(Keycode::L, 'L');
					map(Keycode::M, 'M');
					map(Keycode::N, 'N');
					map(Keycode::O, 'O');
					map(Keycode::P, 'P');
					map(Keycode::Q, 'Q');
					map(Keycode::R, 'R');
					map(Keycode::S, 'S');
					map(Keycode::T, 'T');
					map(Keycode::U, 'U');
					map(Keycode::V, 'V');
					map(Keycode::W, 'W');
					map(Keycode::X, 'X');
					map(Keycode::Y, 'Y');
					map(Keycode::Z, 'Z');

					map(Keycode::Numpad0, VK_NUMPAD0);
					map(Keycode::Numpad1, VK_NUMPAD1);
					map(Keycode::Numpad2, VK_NUMPAD2);
					map(Keycode::Numpad3, VK_NUMPAD3);
					map(Keycode::Numpad4, VK_NUMPAD4);
					map(Keycode::Numpad5, VK_NUMPAD5);
					map(Keycode::Numpad6, VK_NUMPAD6);
					map(Keycode::Numpad7, VK_NUMPAD7);
					map(Keycode::Numpad8, VK_NUMPAD8);
					map(Keycode::Numpad9, VK_NUMPAD9);

					map(Keycode::NumpadDivide, VK_DIVIDE);
					map(Keycode::NumpadMultiply, VK_MULTIPLY);
					map(Keycode::NumpadMinus, VK_SUBTRACT);
					map(Keycode::NumpadPlus, VK_ADD);
					mapKeyToVK.put((int)Keycode::NumpadEnter, VK_RETURN);
					map(Keycode::NumpadDecimal, VK_DECIMAL);

					map(Keycode::F1, VK_F1);
					map(Keycode::F2, VK_F2);
					map(Keycode::F3, VK_F3);
					map(Keycode::F4, VK_F4);
					map(Keycode::F5, VK_F5);
					map(Keycode::F6, VK_F6);
					map(Keycode::F7, VK_F7);
					map(Keycode::F8, VK_F8);
					map(Keycode::F9, VK_F9);
					map(Keycode::F10, VK_F10);
					map(Keycode::F11, VK_F11);
					map(Keycode::F12, VK_F12);

					map(Keycode::Backspace, VK_BACK);
					map(Keycode::PageUp, VK_PRIOR);
					map(Keycode::PageDown, VK_NEXT);
					map(Keycode::Home, VK_HOME);
					map(Keycode::End, VK_END);
					map(Keycode::Left, VK_LEFT);
					map(Keycode::Up, VK_UP);
					map(Keycode::Right, VK_RIGHT);
					map(Keycode::Down, VK_DOWN);
					map(Keycode::PrintScreen, VK_SNAPSHOT);
					map(Keycode::Insert, VK_INSERT);
					map(Keycode::Delete, VK_DELETE);
					map(Keycode::Sleep, VK_SLEEP);
					map(Keycode::Pause, VK_PAUSE);

					map(Keycode::GoHome, -1);
					map(Keycode::GoMenu, -1);
					map(Keycode::GoBack, -1);
					map(Keycode::Camera, -1);
					map(Keycode::VolumeMute, VK_VOLUME_MUTE);
					map(Keycode::VolumeDown, VK_VOLUME_DOWN);
					map(Keycode::VolumeUp, VK_VOLUME_UP);
					map(Keycode::MediaPrev, VK_MEDIA_PREV_TRACK);
					map(Keycode::MediaNext, VK_MEDIA_NEXT_TRACK);
					map(Keycode::MediaPause, VK_MEDIA_PLAY_PAUSE);
					map(Keycode::MediaStop, VK_MEDIA_STOP);
					map(Keycode::PhoneStar, -1);
					map(Keycode::PhonePound, -1);

					map(Keycode::LeftShift, VK_LSHIFT);
					map(Keycode::RightShift, VK_RSHIFT);
					map(Keycode::LeftControl, VK_LCONTROL);
					map(Keycode::RightControl, VK_RCONTROL);
					map(Keycode::LeftAlt, VK_LMENU);
					map(Keycode::RightAlt, VK_RMENU);
					map(Keycode::LeftWin, VK_LWIN);
					map(Keycode::RightWin, VK_RWIN);
					map(Keycode::CapsLock, VK_CAPITAL);
					map(Keycode::ScrollLock, VK_SCROLL);
					map(Keycode::NumLock, VK_NUMLOCK);
					map(Keycode::ContextMenu, VK_APPS);

					map(Keycode::Chinese, VK_HANJA);
					map(Keycode::Korean, VK_HANGUL);
				}

			public:
				void map(Keycode key, sl_uint32 vk)
				{
					if (vk == -1) {
						return;
					}
					mapKeyToVK.put((int)key, vk);
					mapVKToKey.put(vk, (int)key);
				}

				Keycode vkToKey(sl_uint32 vk)
				{
					sl_uint32 ret;
					if (mapVKToKey.get(vk, &ret)) {
						return (Keycode)ret;
					} else {
						return Keycode::Unknown;
					}
				}

				sl_uint32 keyToVk(Keycode code)
				{
					sl_uint32 ret;
					if (mapKeyToVK.get((int)code, &ret)) {
						return ret;
					} else {
						return -1;
					}
				}

			};

			SLIB_SAFE_STATIC_GETTER(KeyMapper, GetKeyMapper)

		}
	}

	using namespace priv::ui_event;

	sl_uint32 UIEvent::getSystemKeycode(Keycode key)
	{
		KeyMapper* mapper = GetKeyMapper();
		if (mapper) {
			return mapper->keyToVk(key);
		}
		return 0;
	}

	Keycode UIEvent::getKeycodeFromSystemKeycode(sl_uint32 vkey)
	{
		KeyMapper* mapper = GetKeyMapper();
		if (mapper) {
			return mapper->vkToKey(vkey);
		}
		return Keycode::Unknown;
	}

	sl_bool UI::isKeyPressed(Keycode key)
	{
		sl_uint32 vk = UIEvent::getSystemKeycode(key);
		if (vk != -1) {
			return (GetKeyState(vk) & 0x8000) != 0;
		} else {
			return sl_false;
		}
	}

	sl_bool UI::isCapsLockOn()
	{
		return (GetKeyState(VK_CAPITAL) & 1) != 0;
	}

	sl_bool UI::isNumLockOn()
	{
		return (GetKeyState(VK_NUMLOCK) & 1) != 0;
	}

	sl_bool UI::isScrollLockOn()
	{
		return (GetKeyState(VK_SCROLL) & 1) != 0;
	}

	UIPoint UI::getCursorPos()
	{
		POINT pt;
		GetCursorPos(&pt);
		return UIPoint((sl_ui_pos)(pt.x), (sl_ui_pos)(pt.y));
	}

	sl_bool UI::isLeftButtonPressed()
	{
		return (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
	}

	sl_bool UI::isRightButtonPressed()
	{
		return (GetKeyState(VK_RBUTTON) & 0x8000) != 0;
	}

	sl_bool UI::isMiddleButtonPressed()
	{
		return (GetKeyState(VK_MBUTTON) & 0x8000) != 0;
	}

	void UI::sendKeyEvent(UIAction action, Keycode key)
	{
		INPUT input;
		Base::zeroMemory(&input, sizeof(input));
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = UIEvent::getSystemKeycode(key);
		if (!(input.ki.wVk)) {
			return;
		}
		switch (action) {
		case UIAction::KeyDown:
			break;
		case UIAction::KeyUp:
			input.ki.dwFlags = KEYEVENTF_KEYUP;
			break;
		default:
			return;
		}
		SendInput(1, &input, sizeof(input));
	}

	void UI::sendMouseEvent(UIAction action, sl_ui_pos x, sl_ui_pos y, sl_bool flagAbsolutePos)
	{
		INPUT input;
		Base::zeroMemory(&input, sizeof(input));
		input.type = INPUT_MOUSE;
		input.mi.dx = (LONG)x;
		input.mi.dy = (LONG)y;
		switch (action) {
		case UIAction::LeftButtonDown:
			input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			break;
		case UIAction::LeftButtonUp:
			input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
			break;
		case UIAction::RightButtonDown:
			input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
			break;
		case UIAction::RightButtonUp:
			input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
			break;
		case UIAction::MiddleButtonDown:
			input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
			break;
		case UIAction::MiddleButtonUp:
			input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
			break;
		case UIAction::MouseMove:
			input.mi.dwFlags = MOUSEEVENTF_MOVE;
			break;
		case UIAction::MouseWheel:
			input.mi.dx = 0;
			input.mi.dy = 0;
			flagAbsolutePos = sl_false;
			if (y) {
				input.mi.dwFlags = MOUSEEVENTF_WHEEL;
				input.mi.mouseData = y;
			} else if (x) {
				input.mi.dwFlags = 0x01000; // MOUSEEVENTF_HWHEEL;
				input.mi.mouseData = x;
			} else {
				return;
			}
			break;
		default:
			return;
		}
		if (flagAbsolutePos) {
			input.mi.dwFlags |= MOUSEEVENTF_ABSOLUTE;
		}
		SendInput(1, &input, sizeof(input));
	}

	void UIPlatform::applyEventModifiers(UIEvent* ev)
	{
		if (GetKeyState(VK_SHIFT) & 0x8000) {
			ev->setShiftKey();
		}
		if (GetKeyState(VK_MENU) & 0x8000) {
			ev->setAltKey();
		}
		if (GetKeyState(VK_CONTROL) & 0x8000) {
			ev->setControlKey();
		}
		if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) {
			ev->setWindowsKey();
		}
	}

}

#endif
