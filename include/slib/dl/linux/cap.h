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

#ifndef CHECKHEADER_SLIB_DL_LINUX_CAP
#define CHECKHEADER_SLIB_DL_LINUX_CAP

#include "../dl.h"

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)

#include "sys/capability.h"

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(cap, "libcap.so.2")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			cap_from_text,
			cap_t, ,
			const char *
		)
		#define cap_from_text slib::cap::getApi_cap_from_text()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			cap_to_text,
			char *, ,
			cap_t, ssize_t *
		)
		#define cap_to_text slib::cap::getApi_cap_to_text()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			cap_get_file,
			cap_t, ,
			const char *
		)
		#define cap_get_file slib::cap::getApi_cap_get_file()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			cap_set_file,
			int, ,
			const char *, cap_t
		)
		#define cap_set_file slib::cap::getApi_cap_set_file()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			cap_get_flag,
			int, ,
			cap_t, cap_value_t, cap_flag_t, cap_flag_value_t *
		)
		#define cap_get_flag slib::cap::getApi_cap_get_flag()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			cap_set_flag,
			int, ,
			cap_t, cap_flag_t, int, const cap_value_t *, cap_flag_value_t
		)
		#define cap_set_flag slib::cap::getApi_cap_set_flag()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			cap_free,
			int, ,
			void *
		)
		#define cap_free slib::cap::getApi_cap_free()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			cap_compare,
			int, ,
			cap_t, cap_t
		)
		#define cap_compare slib::cap::getApi_cap_compare()

	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
