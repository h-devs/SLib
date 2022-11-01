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

#ifndef CHECKHEADER_SLIB_CORE_JAVA_LIST
#define CHECKHEADER_SLIB_CORE_JAVA_LIST

#include "../definition.h"

#if defined(SLIB_PLATFORM_USE_JNI)

#include "../java.h"

namespace slib
{
	namespace java
	{

		class SLIB_EXPORT List
		{
		public:
			static sl_int32 size(jobject list) noexcept;

   			static sl_bool contains(jobject list, jobject element) noexcept;

			static JniLocal<jobject> iterator(jobject list) noexcept;

			static JniLocal<jobject> get(jobject list, sl_int32 index) noexcept;

			static JniLocal<jobject> set(jobject list, sl_int32 index, jobject element) noexcept;

			static sl_bool add(jobject list, jobject element) noexcept;

			static void add(jobject list, sl_int32 index, jobject element) noexcept;

			static sl_bool remove(jobject list, jobject element) noexcept;

			static JniLocal<jobject> remove(jobject list, sl_int32 index) noexcept;

			static void clear(jobject list) noexcept;

			static sl_int32 indexOf(jobject list, jobject element) noexcept;

			static sl_int32 lastIndexOf(jobject list, jobject element) noexcept;

		};

	}

}

#endif

#endif
