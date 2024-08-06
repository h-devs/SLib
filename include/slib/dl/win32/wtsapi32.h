/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_DL_WIN32_WTSAPI32
#define CHECKHEADER_SLIB_DL_WIN32_WTSAPI32

#include "../dl.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "../../platform/win32/windows.h"

#include <wtsapi32.h>

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(wtsapi32, "wtsapi32.dll")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			WTSEnumerateSessionsW,
			BOOL, WINAPI,
			HANDLE hServer,
			DWORD Reserved,
			DWORD Version,
			PWTS_SESSION_INFOW* ppSessionInfo,
			DWORD* pCount
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			WTSQuerySessionInformationW,
			BOOL, WINAPI,
			HANDLE hServer,
			DWORD SessionId,
			WTS_INFO_CLASS WTSInfoClass,
			LPWSTR* ppBuffer,
			DWORD* pBytesReturned
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			WTSFreeMemory,
			VOID, WINAPI,
			PVOID pMemory
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			WTSQueryUserToken,
			BOOL, WINAPI,
			ULONG SessionId,
			PHANDLE phToken
		)
	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
