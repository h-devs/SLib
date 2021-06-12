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

#if defined(SLIB_PLATFORM_IS_ANDROID)

#include "slib/core/system.h"

#include "slib/core/file.h"
#include "slib/core/platform.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

namespace slib
{

	namespace priv
	{
		namespace system
		{

			SLIB_JNI_BEGIN_CLASS(JBuild, "android/os/Build")
			SLIB_JNI_END_CLASS

		}
	}

	using namespace priv::system;

#define PRIV_PATH_MAX 1024

	String System::getApplicationPath()
	{
		char path[PRIV_PATH_MAX] = {0};
		char a[50];

		sprintf(a, "/proc/%d/cmdline", getpid());
		FILE* fp = fopen(a, "rb");
		int n = fread(path, 1, PRIV_PATH_MAX - 1, fp);
		fclose(fp);

		String ret;
		if (n > 0) {
			ret = String::fromUtf8(path);
		}
		return "/data/data/" + ret;
	}

	String System::getHomeDirectory()
	{
		return getApplicationDirectory();
	}

	String System::getTempDirectory()
	{
		String dir = System::getApplicationDirectory() + "/temp";
		File::createDirectory(dir);
		return dir;
	}
	
	String System::getComputerName()
	{
		if (Android::getSdkVersion() >= AndroidSdkVersion::JELLY_BEAN_MR1) {
			jobject jactivity = Android::getCurrentActivity();
			if (jactivity) {
				JniClass clsActivity = Jni::getClass("android/app/Activity");
				if (clsActivity.isNotNull()) {
					JniLocal<jobject> resolver = clsActivity.callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;", jactivity);
					if (resolver.isNotNull()) {
						JniClass clsGlobal = Jni::getClass("android/provider/Settings/Global");
						if (clsGlobal.isNotNull()) {
							JniLocal<jstring> strDeviceName = Jni::getJniString("device_name");
							String name = clsGlobal.callStaticStringMethod("getString", "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;", resolver.value, strDeviceName.value);
							if (name.isNotEmpty()) {
								return name;
							}
						}
					}
				}
			}
		}
		return Android::getDeviceName();
	}
	
	String System::getUserName()
	{
		return "mobile";
	}
	
	String System::getFullUserName()
	{
		return "Mobile User";
	}

	void System::abort(const StringParam& _msg, const StringParam& _file, sl_uint32 line)
	{
#if defined(SLIB_DEBUG)
		StringCstr msg(_msg);
		StringCstr file(_file);
		__assert(file.getData(), line, msg.getData());
#endif
	}

	namespace priv
	{
		void Abort(const char* msg, const char* file, sl_uint32 line) noexcept
		{
#if defined(SLIB_DEBUG)
			__assert(file, line, msg);
#endif
		}
	}

}

#endif
