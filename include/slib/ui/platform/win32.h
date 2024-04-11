/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_WIN32_PLATFORM
#define CHECKHEADER_SLIB_UI_WIN32_PLATFORM

#include "../definition.h"

#if defined(SLIB_UI_IS_WIN32)

#include "../platform_common.h"
#include "../event.h"

namespace slib
{

	class Cursor;
	class Menu;
	class UIEvent;

	class SLIB_EXPORT UIPlatform
	{
		PRIV_SLIB_DECLARE_UI_PLATFORM_COMMON_MEMBERS

	public:
		static Ref<ViewInstance> createViewInstance(HWND hWnd, sl_bool flagDestroyOnRelease = sl_true);
		static void registerViewInstance(HWND hWnd, ViewInstance* instance);
		static Ref<ViewInstance> getViewInstance(HWND hWnd);
		static void removeViewInstance(HWND hWnd);
		static HWND getViewHandle(ViewInstance* instance);
		static HWND getViewHandle(View* view);

		static Ref<WindowInstance> createWindowInstance(HWND hWnd, sl_bool flagDestroyOnRelease = sl_true);
		static void registerWindowInstance(HWND hWnd, WindowInstance* instance);
		static Ref<WindowInstance> getWindowInstance(HWND hWnd);
		static void removeWindowInstance(HWND hWnd);
		static HWND getWindowHandle(WindowInstance* instance);
		static HWND getWindowHandle(Window* window);

		static Ref<Cursor> createCursor(HCURSOR hCursor, sl_bool flagDestroyOnRelease = sl_true);
		static HCURSOR getCursorHandle(Cursor* cursor);

		static HMENU getMenuHandle(Menu* menu);
		static Ref<Menu> getMenu(HMENU hMenu);

		static void applyEventModifiers(UIEvent* ev);

		static sl_bool isWindowVisible(HWND hWnd);
		static String getWindowText(HWND hWnd);
		static String16 getWindowText16(HWND hWnd);
		static void setWindowText(HWND hWnd, const StringParam& text);

		static void setWindowStyle(HWND hWnd, LONG flags, sl_bool flagAddOrRemove);
		static void removeAndAddWindowStyle(HWND hWnd, LONG flagsRemove, LONG flagsAdd);
		static void setWindowExStyle(HWND hWnd, LONG flags, sl_bool flagAddOrRemove);
		static void removeAndAddWindowExStyle(HWND hWnd, LONG flagsRemove, LONG flagsAdd);

		static sl_bool processWindowHorizontalScrollEvents(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, sl_uint32 nLine, sl_uint32 nWheel);
		static sl_bool processWindowVerticalScrollEvents(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, sl_uint32 nLine, sl_uint32 nWheel);
		static void setWindowHorizontalScrollParam(HWND hWnd, sl_int32 nMin, sl_int32 nMax, sl_int32 nPage);
		static void setWindowVerticalScrollParam(HWND hWnd, sl_int32 nMin, sl_int32 nMax, sl_int32 nPage);

		static sl_bool registerTouchWindow(HWND hWnd);
		static void unregisterTouchWindow(HWND hWnd);
		static sl_bool isCurrentMessageFromTouch();

		SLIB_INLINE static sl_uint8 getWindowAlpha(sl_real alpha)
		{
			return (sl_uint8)(Math::clamp0_255((sl_int32)(alpha * 256)));
		}

		static void initLayeredWindowAttributes(HWND hWnd, sl_uint8 alpha, const Color& colorKey);
		static void updateLayeredWindowAttributes(HWND hWnd, sl_uint8 alpha, const Color& colorKey);

	};

}

#define SLIB_UI_MESSAGE_BEGIN 0x7100
#define SLIB_UI_MESSAGE_CUSTOM_QUEUE (SLIB_UI_MESSAGE_BEGIN+1)
#define SLIB_UI_MESSAGE_CLOSE_VIEW (SLIB_UI_MESSAGE_BEGIN+2)
#define SLIB_UI_MESSAGE_DISPATCH (SLIB_UI_MESSAGE_BEGIN+3)
#define SLIB_UI_MESSAGE_DISPATCH_DELAYED (SLIB_UI_MESSAGE_BEGIN+4)
#define SLIB_UI_MESSAGE_CUSTOM_MSGBOX (SLIB_UI_MESSAGE_BEGIN+5)
#define SLIB_UI_MESSAGE_SYSTEM_TRAY_ICON (SLIB_UI_MESSAGE_BEGIN+6)

#define SLIB_UI_EVENT_QUIT_LOOP 1
#define SLIB_UI_EVENT_CLOSE_WINDOW 2

#endif

#endif