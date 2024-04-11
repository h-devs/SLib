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

#ifndef CHECKHEADER_SLIB_DL_WIN32_USER32
#define CHECKHEADER_SLIB_DL_WIN32_USER32

#include "../dl.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "../../platform/win32/windows.h"

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(user32, "user32.dll")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			ShowScrollBar,
			BOOL, WINAPI,
			HWND hWnd,
			int  wBar,
			BOOL bShow
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			RegisterTouchWindow,
			BOOL, WINAPI,
			HWND hWnd,
			ULONG ulFlags
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			UnregisterTouchWindow,
			BOOL, WINAPI,
			HWND hWnd
		)

		struct TOUCHINPUT {
			LONG x;
			LONG y;
			HANDLE hSource;
			DWORD dwID;
			DWORD dwFlags;
			DWORD dwMask;
			DWORD dwTime;
			ULONG_PTR dwExtraInfo;
			DWORD cxContact;
			DWORD cyContact;
		};

		SLIB_IMPORT_LIBRARY_FUNCTION(
			GetTouchInputInfo,
			BOOL, WINAPI,
			void* hTouchInput, // HTOUCHINPUT
			UINT cInputs,
			TOUCHINPUT* pInputs,
			int cbSize
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			CloseTouchInputHandle,
			BOOL, WINAPI,
			void* hTouchInput // HTOUCHINPUT
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			SetWindowDisplayAffinity,
			BOOL, WINAPI,
			HWND hWnd, DWORD dwAffinity
		)

	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
