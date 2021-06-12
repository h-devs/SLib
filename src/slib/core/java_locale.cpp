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

#include "slib/core/definition.h"

#ifdef SLIB_PLATFORM_USE_JNI

#include "slib/core/java/locale.h"

#include "slib/core/java.h"

namespace slib
{
	
	namespace priv
	{
		namespace java_locale
		{

			SLIB_JNI_BEGIN_CLASS(JLocale, "java/util/Locale")
				SLIB_JNI_STATIC_METHOD(getDefault, "getDefault", "()Ljava/util/Locale;");
				SLIB_JNI_METHOD(getLanguage, "getLanguage", "()Ljava/lang/String;");
				SLIB_JNI_METHOD(getCountry, "getCountry", "()Ljava/lang/String;");
				SLIB_JNI_METHOD_OPTIONAL(getScript, "getScript", "()Ljava/lang/String;");
			SLIB_JNI_END_CLASS

		}
	}

	using namespace priv::java_locale;

	namespace java
	{

		jobject Locale::getDefault() noexcept
		{
			return JLocale::getDefault.callObject(sl_null);
		}
	
		String Locale::getLanguage(jobject thiz) noexcept
		{
			return JLocale::getLanguage.callString(thiz);
		}

		String Locale::getCountry(jobject thiz) noexcept
		{
			return JLocale::getCountry.callString(thiz);
		}

		String Locale::getScript(jobject thiz) noexcept
		{
			if (JLocale::getScript.id) {
				return JLocale::getScript.callString(thiz);
			}
			return sl_null;
		}

	}

}

#endif
