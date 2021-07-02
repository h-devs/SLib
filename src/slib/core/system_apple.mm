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

#if defined(SLIB_PLATFORM_IS_APPLE)

#include "slib/core/system.h"

#include "slib/core/file.h"
#include "slib/core/platform.h"

#include <mach-o/dyld.h>
#include <mach/mach_time.h>

#define PRIV_PATH_MAX 1024

namespace slib
{

	String System::getApplicationPath()
	{
		char path[PRIV_PATH_MAX] = {0};
		char pathResolved[PRIV_PATH_MAX] = {0};
		uint32_t size = PRIV_PATH_MAX - 1;
		if (_NSGetExecutablePath(path, &size) == 0) {
			realpath(path, pathResolved);
		}
		return pathResolved;
	}

	String System::getHomeDirectory()
	{
		NSString* path = NSHomeDirectory();
		return Apple::getStringFromNSString(path);
	}

	String System::getCachesDirectory()
	{
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
		if ([paths count] > 0) {
			NSString* path = [paths objectAtIndex:0];
			return Apple::getStringFromNSString(path);
		}
		String dir = getHomeDirectory() + "/.caches";
		File::createDirectory(dir);
		return dir;
	}

	String System::getTempDirectory()
	{
		NSString* path = NSTemporaryDirectory();
		String ret = Apple::getStringFromNSString(path);
		if (ret.endsWith('/')) {
			return ret.substring(0, ret.getLength() - 1);
		} else {
			return ret;
		}
	}
	
	String System::getComputerName()
	{
#if defined(SLIB_PLATFORM_IS_MACOS)
		return Apple::getStringFromNSString([[NSHost currentHost] localizedName]);
#else
		return Apple::getStringFromNSString([[UIDevice currentDevice] name]);
#endif
	}

	String System::getUserName()
	{
		return Apple::getStringFromNSString(NSUserName());
	}
	
	String System::getFullUserName()
	{
		return Apple::getStringFromNSString(NSFullUserName());
	}

	sl_uint64 System::getTickCount64()
	{
		static sl_bool flagInit = sl_true;
		static mach_timebase_info_data_t base;
		static sl_uint64 start = 0;
		
		if (flagInit) {
			mach_timebase_info(&base);
			start = mach_absolute_time();
			flagInit = sl_false;
			return 0;
		}
		sl_uint64 t = (sl_uint64)(mach_absolute_time() - start);
		return t * base.numer / base.denom / 1000000;
	}
	
#if defined(SLIB_PLATFORM_IS_MACOS)
	void System::registerApplicationRunAtStartup(const String& path)
	{
		Apple::setBundleLoginItemEnabled(path, sl_true);
	}

	void System::registerApplicationRunAtStartup()
	{
		Apple::setBundleLoginItemEnabled(Apple::getMainBundlePath(), sl_true);
	}

	void System::unregisterApplicationRunAtStartup(const String& path)
	{
		Apple::setBundleLoginItemEnabled(path, sl_false);
	}

	void System::unregisterApplicationRunAtStartup()
	{
		Apple::setBundleLoginItemEnabled(Apple::getMainBundlePath(), sl_false);
	}
#endif

}

#endif
