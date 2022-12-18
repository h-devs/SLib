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

#ifndef CHECKHEADER_SLIB_PLATFORM_ANDROID_PREFERENCE
#define CHECKHEADER_SLIB_PLATFORM_ANDROID_PREFERENCE

#include "../definition.h"

#if defined(SLIB_PLATFORM_IS_ANDROID)

#include "../java.h"

namespace slib
{
	namespace android
	{

		class SLIB_EXPORT SharedPreferences
		{
		public:
			static JniLocal<jobject> getEditor(jobject thiz) noexcept;

			static String getString(jobject thiz, const StringParam& key, const StringParam& def) noexcept;

		};

		class SLIB_EXPORT SharedPreferencesEditor
		{
		public:
			static void apply(jobject thiz) noexcept;
			static sl_bool commit(jobject thiz) noexcept;

			static void putString(jobject thiz, const StringParam& key, const StringParam& value) noexcept;

			static void remove(jobject thiz, const StringParam& key) noexcept;

		};

	}

}

#endif

#endif
