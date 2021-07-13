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
#include "slib/core/android/platform.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>

namespace slib
{

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
	
	// From Java code: slib.android.System.getDeviceNameOnSettings
	String System::getComputerName()
	{
		if (Android::getSdkVersion() >= AndroidSdkVersion::JELLY_BEAN_MR1) {
			jobject context = Android::getCurrentContext();
			if (context) {
				JniLocal<jobject> resolver = Jni::callObjectMethod(context, "getContentResolver", "()Landroid/content/ContentResolver;");
				if (resolver.isNotNull()) {
					jclass clsGlobal = Jni::getClass("android/provider/Settings$Global");
					if (clsGlobal) {
						SLIB_JNI_STRING(strDeviceName, "device_name")
						String name = Jni::callStaticStringMethod(clsGlobal, "getString", "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;", resolver.get(), strDeviceName.get());
						if (name.isNotEmpty()) {
							return name;
						}
					}
				}
			}
		}
		return getMachineName();
	}
	
	String System::getSystemName()
	{
		return "Android " + getSystemVersion();
	}

	String System::getSystemVersion()
	{
		jclass cls = Jni::getClass("android/os/Build$VERSION");
		if (cls) {
			return Jni::getStaticStringField(cls, "RELEASE");
		}
		return sl_null;
	}

	// From Java code: slib.android.device.Device.getDeviceName
	String System::getMachineName()
	{
		jclass cls = Jni::getClass("android/os/Build");
		if (cls) {
			String manufacturer = Jni::getStaticStringField(cls, "MANUFACTURER");
			String model = Jni::getStaticStringField(cls, "MODEL");
			if (model.startsWith(manufacturer)) {
				return model;
			} else {
				return String::join(manufacturer, " ", model);
			}
		}
		return sl_null;
	}

}

#endif
