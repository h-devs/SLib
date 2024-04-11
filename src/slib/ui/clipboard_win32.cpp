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

#include "slib/ui/clipboard.h"

#if defined(SLIB_UI_IS_WIN32)

#include "slib/core/safe_static.h"
#include "slib/platform.h"

namespace slib
{

	namespace
	{

		static LRESULT CALLBACK OwnerWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
		{
			switch (message) {
				case WM_RENDERFORMAT:
				case WM_RENDERALLFORMATS:
					break;
				case WM_DESTROY:
					return 0;
			}
			return DefWindowProcW(hwnd, message, wparam, lparam);
		}

		class Context
		{
		public:
			HWND owner;

		public:
			Context()
			{
				LPCWSTR szClassName = L"ClipboardOwner";
				WNDCLASSEXW wc;
				Base::zeroMemory(&wc, sizeof(wc));
				wc.cbSize = sizeof(wc);
				wc.lpszClassName = szClassName;
				wc.lpfnWndProc = OwnerWndProc;
				wc.hInstance = GetModuleHandleW(NULL);
				RegisterClassExW(&wc);
				owner = CreateWindowW(szClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);
			}

			~Context()
			{
				DestroyWindow(owner);
				owner = NULL;
			}

		};

		SLIB_SAFE_STATIC_GETTER(Context, GetContext)

	}

	sl_bool Clipboard::hasText()
	{
		return IsClipboardFormatAvailable(CF_UNICODETEXT) ? sl_true : sl_false;
	}

	String Clipboard::getText()
	{
		HANDLE data = GetClipboardData(CF_UNICODETEXT);
		if (data) {
			void* src = GlobalLock(data);
			if (src) {
				String ret = String::create((sl_char16*)src);
				GlobalUnlock(src);
				return ret;
			}
		}
		return sl_null;
	}

#define SET_CLIPBOARD_BEGIN \
		Context* context = GetContext(); \
		if (!context) { \
			return; \
		} \
		if (!(OpenClipboard(context->owner))) { \
			return; \
		} \
		EmptyClipboard();

#define SET_CLIPBOARD_END \
		CloseClipboard();

	void Clipboard::setText(const StringParam& _text)
	{
		SET_CLIPBOARD_BEGIN
		StringCstr16 text(_text);
		HGLOBAL handle = Win32::createGlobalData(text.getData(), (text.getLength() + 1) << 1);
		if (handle) {
			SetClipboardData(CF_UNICODETEXT, handle);
		}
		SET_CLIPBOARD_END
	}

}

#endif