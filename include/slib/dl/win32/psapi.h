/*
 *   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_DL_WIN32_PSAPI
#define CHECKHEADER_SLIB_DL_WIN32_PSAPI

#include "../dl.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "../../platform/win32/windows.h"

#include <psapi.h>

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(psapi, "psapi.dll")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			EnumProcesses,
			BOOL, WINAPI,
			DWORD *pProcessIds,
			DWORD cb,
			DWORD *pBytesReturned
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			EnumProcessModules,
			BOOL, WINAPI,
			HANDLE hProcess,
			HMODULE *lphModule,
			DWORD cb,
			LPDWORD lpcbNeeded
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			EnumProcessModulesEx,
			BOOL, WINAPI,
			HANDLE hProcess,
			HMODULE *lphModule,
			DWORD cb,
			LPDWORD lpcbNeeded,
			DWORD dwFilterFlag
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			GetModuleFileNameExW,
			DWORD, WINAPI,
			HANDLE hProcess,
			HMODULE hModule,
			LPWSTR lpFilename,
			DWORD nSize
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			GetModuleBaseNameW,
			DWORD, WINAPI,
			HANDLE hProcess,
			HMODULE hModule,
			LPTSTR lpBaseName,
			DWORD nSize
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			GetModuleInformation,
			BOOL, WINAPI,
			HANDLE hProcess,
			HMODULE hModule,
			LPMODULEINFO lpmodinfo,
			DWORD cb
		)

	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
