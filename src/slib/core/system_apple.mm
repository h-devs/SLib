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
#include "slib/core/list.h"
#include "slib/core/safe_static.h"

#include "slib/core/apple/platform.h"

#include <mach-o/dyld.h>
#include <mach/mach_time.h>

#define PRIV_PATH_MAX 1024

namespace slib
{

	namespace priv
	{
		namespace system
		{

			SLIB_GLOBAL_ZERO_INITIALIZED(AtomicString, g_systemVersion);
			sl_uint32 g_systemVersionMajor = 0;
			sl_uint32 g_systemVersionMinor = 0;
			sl_uint32 g_systemVersionPatch = 0;
			sl_bool g_flagInitSystemVersion = sl_true;
			
			void InitSystemVersion()
			{
				if (g_flagInitSystemVersion) {
#if defined(SLIB_PLATFORM_IS_MACOS)
					double v = NSAppKitVersionNumber;
					if (v >= NSAppKitVersionNumber10_10) {
						NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
						g_systemVersionMajor = (sl_uint32)(version.majorVersion);
						g_systemVersionMinor = (sl_uint32)(version.minorVersion);
						g_systemVersionPatch = (sl_uint32)(version.patchVersion);
						g_systemVersion = String::concat(String::fromUint32(g_systemVersionMajor), ".", String::fromUint32(g_systemVersionMinor));
						if (g_systemVersionPatch > 0) {
							g_systemVersion = String::join(g_systemVersion, ".", String::fromUint32(g_systemVersionPatch));
						}
					} else if (v >= NSAppKitVersionNumber10_9) {
						g_systemVersion = "10.9";
						g_systemVersionMajor = 10;
						g_systemVersionMinor = 9;
					} else if (v >= NSAppKitVersionNumber10_8) {
						g_systemVersion = "10.8";
						g_systemVersionMajor = 10;
						g_systemVersionMinor = 8;
					} else if (v >= NSAppKitVersionNumber10_7) {
						g_systemVersion = "10.7";
						g_systemVersionMajor = 10;
						g_systemVersionMinor = 7;
					} else if (v >= NSAppKitVersionNumber10_6) {
						g_systemVersion = "10.6";
						g_systemVersionMajor = 10;
						g_systemVersionMinor = 6;
					}
#elif defined(SLIB_PLATFORM_IS_IOS)
					NSString* _version = [[UIDevice currentDevice] systemVersion];
					String version = Apple::getStringFromNSString(_version);
					if (version.isNotEmpty()) {
						ListElements<String> list(version.split("."));
						if (list.count > 0) {
							g_systemVersionMajor = list[0].parseUint32();
							if (list.count > 1) {
								g_systemVersionMinor = list[1].parseUint32();
							}
						}
					}
					g_systemVersion = version;
#endif
					g_flagInitSystemVersion = sl_false;
				}
			}

		}
	}

	using namespace priv::system;

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

	String System::getLocalAppDataDirectory()
	{
		String dir = getHomeDirectory() + "/Library/Application Support";
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
	
	String System::getMainBundlePath()
	{
		NSString* path = [[NSBundle mainBundle] bundlePath];
		return Apple::getStringFromNSString(path);
	}

	String System::getMainBundleInfo(const StringParam& key)
	{
		NSDictionary* dict = [[NSBundle mainBundle] infoDictionary];
		NSString* info = [dict objectForKey:Apple::getNSStringFromString(key)];
		return Apple::getStringFromNSString(info);
	}

	String System::getSystemName()
	{
		InitSystemVersion();
#if defined(SLIB_PLATFORM_IS_MACOS)
		return "macOS " + g_systemVersion;
#elif defined(SLIB_PLATFORM_IS_IOS)
		return "iOS " + g_systemVersion;
#else
		return sl_null;
#endif
	}
	
	String System::getSystemVersion()
	{
		String plistFileName = "/System/Library/CoreServices/SystemVersion.plist";
		if (File::isFile(plistFileName)) {
			NSString* path = Apple::getNSStringFromString(plistFileName);
			NSMutableDictionary *dict = [[NSMutableDictionary alloc] initWithContentsOfFile:path];
			NSString* info = [dict objectForKey:@"ProductVersion"];
			String version = Apple::getStringFromNSString(info);
			if (version.isNotEmpty()) {
				return version;
			}
		}
		InitSystemVersion();
		return g_systemVersion;
	}

	sl_uint32 System::getMajorVersion()
	{
		InitSystemVersion();
		return g_systemVersionMajor;
	}

	sl_uint32 System::getMinorVersion()
	{
		InitSystemVersion();
		return g_systemVersionMinor;
	}

	sl_uint32 System::getPatchVersion()
	{
		InitSystemVersion();
		return g_systemVersionPatch;
	}

	String System::getBuildVersion()
	{
		String plistFileName = "/System/Library/CoreServices/SystemVersion.plist";
		if (File::isFile(plistFileName)) {
			NSString* path = Apple::getNSStringFromString(plistFileName);
			NSMutableDictionary *dict = [[NSMutableDictionary alloc] initWithContentsOfFile:path];
			NSString* info = [dict objectForKey:@"ProductBuildVersion"];
			String version = Apple::getStringFromNSString(info);
			if (version.isNotEmpty()) {
				return version;
			}
		}
		return sl_null;
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

}

#endif
