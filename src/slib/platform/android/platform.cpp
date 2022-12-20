/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/platform/definition.h"

#ifdef SLIB_PLATFORM_IS_ANDROID

#include "slib/core/safe_static.h"
#include "slib/platform.h"

namespace slib
{

	namespace priv
	{
		namespace platform
		{

			SLIB_GLOBAL_ZERO_INITIALIZED(JniGlobal<jobject>, g_contextCurrent);

		}
	}

	using namespace priv::platform;

	void Android::initialize(JavaVM* jvm) noexcept
	{
		Jni::initialize(jvm);
	}

	AndroidSdkVersion Android::getSdkVersion() noexcept
	{
		static AndroidSdkVersion version = AndroidSdkVersion::CUR_DEVELOPMENT;
		if (version == AndroidSdkVersion::CUR_DEVELOPMENT) {
			jclass cls = Jni::getClass("android/os/Build$VERSION");
			if (cls) {
				version = (AndroidSdkVersion)(Jni::getStaticIntField(cls, "SDK_INT"));
			}
		}
		return version;
	}

	jobject Android::getCurrentContext() noexcept
	{
		return g_contextCurrent.get();
	}

	void Android::setCurrentContext(jobject context) noexcept
	{
		SLIB_STATIC_SPINLOCKER(lock)
		g_contextCurrent = JniGlobal<jobject>::create(context);
	}

	void Android::initializeContext(jobject context) noexcept
	{
		SLIB_STATIC_SPINLOCKER(lock)
		if (g_contextCurrent.isNull()) {
			g_contextCurrent = JniGlobal<jobject>::create(context);
		}
	}

}

#endif
