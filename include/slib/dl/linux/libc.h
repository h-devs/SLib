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

#ifndef CHECKHEADER_SLIB_DL_LINUX_LIBC
#define CHECKHEADER_SLIB_DL_LINUX_LIBC

#include "../dl.h"

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(libc, "libc.so.6")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			fcntl,
			int, ,
			int fd, int cmd, ...
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			fcntl64,
			int, ,
			int fd, int cmd, ...
		)

	SLIB_IMPORT_LIBRARY_END

#if defined(SLIB_ARCH_IS_64BIT)
	typedef int (*FUNC_fcntl)(int fd, int cmd, ...);
	FUNC_fcntl GetApi_fcntl();
#	ifdef fcntl
#		undef fcntl
#	endif
#	define fcntl GetApi_fcntl()
#endif

}

#endif

#endif
