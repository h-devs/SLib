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

#define SLIB_IMPLEMENT_DYNAMIC_LIBRARY

#include "slib/dl/linux/libc.h"

namespace slib
{

	namespace priv
	{
		namespace libc
		{

			int EmptyFcntl(int fd, int cmd, ...)
			{
				return -1;
			}

		}
	}

	using namespace priv::libc;

#if defined(SLIB_ARCH_IS_64BIT)
	FUNC_fcntl getApi_fcntl()
	{
		static FUNC_fcntl func = sl_null;
		static sl_bool flagInit = sl_true;
		if (flagInit) {
			func = libc::getApi_fcntl64();
			if (!func) {
				func = libc::getApi_fcntl();
				if (!func) {
					func = &EmptyFcntl;
				}
			}
			flagInit = sl_false;
		}
		return func;
	}
#endif

}
