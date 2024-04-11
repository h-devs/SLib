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

#ifndef CHECKHEADER_SLIB_CORE_TIMEOUT
#define CHECKHEADER_SLIB_CORE_TIMEOUT

#include "definition.h"

#include "../system/system.h"

namespace slib
{

	SLIB_INLINE static sl_int64 GetTickFromTimeout(sl_int32 timeout)
	{
		if (timeout >= 0) {
			return System::getTickCount64() + timeout;
		} else {
			return -1;
		}
	}

	SLIB_INLINE static sl_int32 GetTimeoutFromTick(sl_int64 tick)
	{
		if (tick >= 0) {
			sl_uint64 current = System::getTickCount64();
			if ((sl_uint64)tick > current) {
				return (sl_int32)(tick - current);
			} else {
				return 0;
			}
		} else {
			return -1;
		}
	}

}

#endif
