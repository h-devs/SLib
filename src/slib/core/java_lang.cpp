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

#include "slib/core/java/object.h"
#include "slib/core/java/string.h"

namespace slib
{

	namespace priv
	{
		namespace java_lang
		{

			SLIB_JNI_BEGIN_CLASS(JObject, "java/lang/Object")
				SLIB_JNI_METHOD(toString, "toString", "()Ljava/lang/String;")
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JString, "java/lang/String")
			SLIB_JNI_END_CLASS

		}
	}

	using namespace priv::java_lang;

	namespace java
	{

		slib::String Object::toString(jobject thiz) noexcept
		{
			return JObject::toString.callString(thiz);
		}

		jclass String::getClass() noexcept
		{
			return JString::get();
		}

	}

}

#endif
