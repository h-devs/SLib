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

#ifndef CHECKHEADER_SLIB_DL_WIN32_KERNEL32
#define CHECKHEADER_SLIB_DL_WIN32_KERNEL32

#include "../dl.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "../../platform/win32/windows.h"

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(kernel32, "kernel32.dll")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			GetQueuedCompletionStatusEx,
			BOOL, WINAPI,
			HANDLE CompletionPort,
			LPOVERLAPPED_ENTRY lpCompletionPortEntries,
			ULONG ulCount,
			PULONG ulNumEntriesRemoved,
			DWORD dwMilliseconds,
			BOOL fAlertable
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			GetUserDefaultLocaleName,
			int, WINAPI,
			LPWSTR lpLocaleName,
			int cchLocaleName
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			GetTickCount64,
			ULONGLONG, WINAPI
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			Wow64EnableWow64FsRedirection,
			BOOLEAN, WINAPI,
			BOOLEAN Wow64FsEnableRedirection
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			Wow64DisableWow64FsRedirection,
			BOOL, WINAPI,
			PVOID *OldValue
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			Wow64RevertWow64FsRedirection,
			BOOL, WINAPI,
			PVOID OldValue
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			IsWow64Process,
			BOOL, WINAPI,
			HANDLE hProcess,
			PBOOL Wow64Process
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			InitializeSRWLock,
			void, WINAPI,
			PSRWLOCK SRWLock
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			AcquireSRWLockShared,
			void, WINAPI,
			PSRWLOCK SRWLock
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			ReleaseSRWLockShared,
			void, WINAPI,
			PSRWLOCK SRWLock
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			TryAcquireSRWLockShared,
			BOOLEAN, WINAPI,
			PSRWLOCK SRWLock
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			AcquireSRWLockExclusive,
			void, WINAPI,
			PSRWLOCK SRWLock
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			ReleaseSRWLockExclusive,
			void, WINAPI,
			PSRWLOCK SRWLock
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			TryAcquireSRWLockExclusive,
			BOOLEAN, WINAPI,
			PSRWLOCK SRWLock
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			CreateSymbolicLinkW,
			BOOLEAN, WINAPI,
			LPCWSTR lpSymlinkFileName,
			LPCWSTR lpTargetFileName,
			DWORD dwFlags
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			QueryFullProcessImageNameW,
			BOOL, WINAPI,
			HANDLE hProcess,
			DWORD dwFlags,
			LPWSTR lpExeName,
			PDWORD lpdwSize
		)

	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
