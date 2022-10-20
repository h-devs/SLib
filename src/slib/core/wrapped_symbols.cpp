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

#if defined(__linux__) && !defined(__ANDROID__)

#include "slib/core/definition.h"

#include <dlfcn.h>
#include <stdlib.h>

namespace priv
{
	namespace slib
	{
		namespace wrapped_symbols
		{
			
			void* g_libc = 0;
			void* g_libm = 0;

			static void LoadLibrary()
			{
				if (!g_libc) {
					g_libc = (void*)(dlopen("libc.so.6", RTLD_LAZY));
					g_libm = (void*)(dlopen("libm.so.6", RTLD_LAZY));
				}
			}

		}
	}
}

using namespace priv::slib::wrapped_symbols;

#define BEGIN_WRAPPER(LIB_SUFFIX, NAME, RET, ...) \
	typedef RET(*FUNC_##NAME)(__VA_ARGS__); \
	RET __wrap_##NAME(__VA_ARGS__) { \
		static FUNC_##NAME func = 0; \
		if (!func) { \
			LoadLibrary(); \
			func = (FUNC_##NAME)(dlsym(g_lib##LIB_SUFFIX, #NAME)); \
		}

#define CALL_ORIGINAL(...) func(__VA_ARGS__)

#define END_WRAPPER }

extern "C"
{

	BEGIN_WRAPPER(c, memcpy, void, void* dst, const void* src, size_t size)
		CALL_ORIGINAL(dst, src, size);
	END_WRAPPER

#if defined(SLIB_ARCH_IS_64BIT)
	BEGIN_WRAPPER(c, fcntl64, int, int fd, int cmd, size_t arg)
		if (func) {
			return CALL_ORIGINAL(fd, cmd, arg);
		} else {
			func = (FUNC_fcntl64)(dlsym(g_libc, "fcntl"));
			return CALL_ORIGINAL(fd, cmd, arg);
		}
	END_WRAPPER
#endif

	BEGIN_WRAPPER(m, pow, float, float x, float y)
		return CALL_ORIGINAL(x, y);
	END_WRAPPER

	BEGIN_WRAPPER(m, powf, float, float x, float y)
		return CALL_ORIGINAL(x, y);
	END_WRAPPER

	BEGIN_WRAPPER(m, log, float, float f)
		return CALL_ORIGINAL(f);
	END_WRAPPER

	BEGIN_WRAPPER(m, logf, float, float f)
		return CALL_ORIGINAL(f);
	END_WRAPPER

	BEGIN_WRAPPER(m, exp, float, float f)
		return CALL_ORIGINAL(f);
	END_WRAPPER

	BEGIN_WRAPPER(m, expf, float, float f)
		return CALL_ORIGINAL(f);
	END_WRAPPER

}

#endif
