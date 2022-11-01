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

#include "slib/dev/protect.h"

#include "slib/core/win32/windows.h"

namespace slib
{

	sl_bool MemoryProtection::setProtection(const void* address, sl_size size, sl_bool flagExecute, sl_bool flagWrite, sl_bool flagRead, sl_bool flagWriteCopy)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		DWORD dwProtect;
		if (flagExecute) {
			if (flagWriteCopy) {
				dwProtect = PAGE_EXECUTE_WRITECOPY;
			} else if (flagWrite) {
				dwProtect = PAGE_EXECUTE_READWRITE;
			} else if (flagRead) {
				dwProtect = PAGE_EXECUTE_READ;
			} else {
				dwProtect = PAGE_EXECUTE;
			}
		} else {
			if (flagWriteCopy) {
				dwProtect = PAGE_WRITECOPY;
			} else if (flagWrite) {
				dwProtect = PAGE_READWRITE;
			} else if (flagRead) {
				dwProtect = PAGE_READONLY;
			} else {
				dwProtect = PAGE_NOACCESS;
			}
		}
		DWORD dwOldProtect;
		if (VirtualProtectEx(GetCurrentProcess(), (LPVOID)address, size, dwProtect, &dwOldProtect)) {
			return sl_true;
		}
#endif
		return sl_false;
	}


	sl_bool MemoryProtection::setNoAccess(const void* address, sl_size size)
	{
		return setProtection(address, size, sl_false, sl_false, sl_false);
	}

	sl_bool MemoryProtection::setReadOnly(const void* address, sl_size size)
	{
		return setProtection(address, size, sl_false, sl_false, sl_true);
	}

	sl_bool MemoryProtection::setReadWrite(const void* address, sl_size size)
	{
		return setProtection(address, size, sl_false, sl_true, sl_true);
	}

	sl_bool MemoryProtection::setWriteCopy(const void* address, sl_size size)
	{
		return setProtection(address, size, sl_false, sl_true, sl_true, sl_true);
	}

	sl_bool MemoryProtection::setExecute(const void* address, sl_size size)
	{
		return setProtection(address, size, sl_true, sl_false, sl_false);
	}

	sl_bool MemoryProtection::setExecuteRead(const void* address, sl_size size)
	{
		return setProtection(address, size, sl_true, sl_false, sl_true);
	}

	sl_bool MemoryProtection::setExecuteReadWrite(const void* address, sl_size size)
	{
		return setProtection(address, size, sl_true, sl_true, sl_true);
	}

	sl_bool MemoryProtection::setExecuteWriteCopy(const void* address, sl_size size)
	{
		return setProtection(address, size, sl_true, sl_true, sl_true, sl_true);
	}

}
