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

#include <dlfcn.h>
#include <stdlib.h>

namespace priv
{
	namespace slib
	{
		namespace wrapped_symbols
		{
			
			void* g_libc = sl_null;
			void* g_libm = sl_null;

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
	RET NAME(__VA_ARGS__) { \
		static FUNC_##NAME func = sl_null; \
		if (!func) { \
			LoadLibrary(); \
			func = (FUNC_##NAME)(dlsym(g_lib##LIB_SUFFIX, #NAME)); \
		}

#define CALL_ORIGINAL(...) func(__VA_ARGS__)

#define END_WRAPPER }

extern "C"
{

	BEGIN_WRAPPER(c, memcpy, void, void* dst, const void* src, size_t size)
		return CALL_ORIGINAL(dst, src, size);
	END_WRAPPER

}

#endif
