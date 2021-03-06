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

#include "slib/dev/module.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "slib/core/base.h"

#include "slib/core/win32/windows.h"

#define PSAPI_VERSION 1
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

namespace slib
{
	
	const void* Module::getBaseAddress()
	{
		return getBaseAddress(sl_null);
	}

	const void* Module::getBaseAddress(const char* moduleName)
	{
		MODULEINFO mi;
		Base::zeroMemory(&mi, sizeof(mi));
		if (GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(moduleName), &mi, sizeof(mi))) {
			return mi.lpBaseOfDll;
		}
		return sl_null;
	}

}

#endif
