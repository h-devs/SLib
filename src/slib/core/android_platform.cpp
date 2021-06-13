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

#include "slib/core/definition.h"

#ifdef SLIB_PLATFORM_IS_ANDROID

#include "slib/core/android/platform.h"
#include "slib/core/safe_static.h"

namespace slib
{

	namespace priv
	{
		namespace android
		{

			SLIB_GLOBAL_ZERO_INITIALIZED(AtomicJniGlobal<jobject>, g_activityCurrent);
			SLIB_GLOBAL_ZERO_INITIALIZED(AtomicString, g_strSystemRelease);
			SLIB_GLOBAL_ZERO_INITIALIZED(AtomicString, g_strDeviceName);

		}
	}

	using namespace priv::android;

	void Android::initialize(JavaVM* jvm) noexcept
	{
		Jni::initialize(jvm);
	}

	AndroidSdkVersion Android::getSdkVersion() noexcept
	{
		static AndroidSdkVersion version = AndroidSdkVersion::CUR_DEVELOPMENT;
		if (version == AndroidSdkVersion::CUR_DEVELOPMENT) {
			JniClass cls = Jni::getClass("android/os/Build$VERSION");
			if (cls.isNotNull()) {
				version = (AndroidSdkVersion)(cls.getStaticIntField("SDK_INT"));
			}
		}
		return version;
	}
	
	String Android::getSystemRelease() noexcept
	{
		if (g_strSystemRelease.isNull()) {
			JniClass cls = Jni::getClass("android/os/Build$VERSION");
			if (cls.isNotNull()) {
				String release = cls.getStaticStringField("RELEASE");
				g_strSystemRelease = release;
				return release;
			}
		}
		return g_strSystemRelease;
	}

	// From Java code: slib.android.Device.getDeviceName
	String Android::getDeviceName() noexcept
	{
		if (g_strDeviceName.isNull()) {
			JniClass cls = Jni::getClass("android/os/Build");
			if (cls.isNotNull()) {
				String manufacturer = cls.getStaticStringField("MANUFACTURER");
				String model = cls.getStaticStringField("MODEL");
				if (!(model.startsWith(manufacturer))) {
					model = String::join(manufacturer, " ", model);
				}
				g_strDeviceName = model;
				return model;
			}
		}
		return g_strDeviceName;
	}

	jobject Android::getCurrentActivity() noexcept
	{
		return g_activityCurrent.get();
	}

	void Android::setCurrentActivity(jobject activity) noexcept
	{
		g_activityCurrent = activity;
	}

}

#endif
