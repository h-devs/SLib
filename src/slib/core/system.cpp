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

#include "slib/core/system.h"

#include "slib/core/file.h"
#include "slib/core/string.h"

#include <errno.h>

namespace slib
{

	String System::getApplicationDirectory()
	{
		String path = getApplicationPath();
#if !defined(SLIB_PLATFORM_IS_ANDROID)
		path = File::getParentDirectoryPath(path);
#endif
		return path;
	}

#if !defined(SLIB_PLATFORM_IS_WINDOWS)
	String System::getLocalAppDataDirectory()
	{
		return getHomeDirectory();
	}
#endif

#if !defined(SLIB_PLATFORM_IS_APPLE)
	String System::getCachesDirectory()
	{
		String dir = getHomeDirectory() + "/.caches";
		File::createDirectory(dir);
		return dir;
	}
#endif

	sl_uint32 System::mapError(sl_uint32 errorCode, PlatformType to, PlatformType from)
	{
		if (to == PlatformType::Unknown) {
			return errorCode;
		}
		if (from == PlatformType::Unknown) {
			from = PlatformType::Current;
		}
		if (from == to) {
			return errorCode;
		}

		if (from == PlatformType::Windows && to == PlatformType::Unix) {
			switch (errorCode)
			{
			case 1/*ERROR_INVALID_FUNCTION*/:
				return EINVAL;
			case 2/*ERROR_FILE_NOT_FOUND*/:
				return ENOENT;
			case 3/*ERROR_PATH_NOT_FOUND*/:
				return ENOENT;
			case 4/*ERROR_TOO_MANY_OPEN_FILES*/:
				return EMFILE;
			case 5/*ERROR_ACCESS_DENIED*/:
				return EACCES;
			case 6/*ERROR_INVALID_HANDLE*/:
				return EBADF;
			case 7/*ERROR_ARENA_TRASHED*/:
				return ENOMEM;
			case 8/*ERROR_NOT_ENOUGH_MEMORY*/:
				return ENOMEM;
			case 9/*ERROR_INVALID_BLOCK*/:
				return ENOMEM;
			case 10/*ERROR_BAD_ENVIRONMENT*/:
				return E2BIG;
			case 11/*ERROR_BAD_FORMAT*/:
				return ENOEXEC;
			case 12/*ERROR_INVALID_ACCESS*/:
				return EINVAL;
			case 13/*ERROR_INVALID_DATA*/:
				return EINVAL;
			case 15/*ERROR_INVALID_DRIVE*/:
				return ENOENT;
			case 16/*ERROR_CURRENT_DIRECTORY*/:
				return EACCES;
			case 17/*ERROR_NOT_SAME_DEVICE*/:
				return EXDEV;
			case 18/*ERROR_NO_MORE_FILES*/:
				return ENOENT;
			case 33/*ERROR_LOCK_VIOLATION*/:
				return EACCES;
			case 53/*ERROR_BAD_NETPATH*/:
				return ENOENT;
			case 65/*ERROR_NETWORK_ACCESS_DENIED*/:
				return EACCES;
			case 67/*ERROR_BAD_NET_NAME*/:
				return ENOENT;
			case 80/*ERROR_FILE_EXISTS*/:
				return EEXIST;
			case 82/*ERROR_CANNOT_MAKE*/:
				return EACCES;
			case 83/*ERROR_FAIL_I24*/:
				return EACCES;
			case 87/*ERROR_INVALID_PARAMETER*/:
				return EINVAL;
			case 89/*ERROR_NO_PROC_SLOTS*/:
				return EAGAIN;
			case 108/*ERROR_DRIVE_LOCKED*/:
				return EACCES;
			case 109/*ERROR_BROKEN_PIPE*/:
				return EPIPE;
			case 112/*ERROR_DISK_FULL*/:
				return ENOSPC;
			case 114/*ERROR_INVALID_TARGET_HANDLE*/:
				return EBADF;
			case 128/*ERROR_WAIT_NO_CHILDREN*/:
				return ECHILD;
			case 129/*ERROR_CHILD_NOT_COMPLETE*/:
				return ECHILD;
			case 130/*ERROR_DIRECT_ACCESS_HANDLE*/:
				return EBADF;
			case 131/*ERROR_NEGATIVE_SEEK*/:
				return EINVAL;
			case 132/*ERROR_SEEK_ON_DEVICE*/:
				return EACCES;
			case 145/*ERROR_DIR_NOT_EMPTY*/:
				return ENOTEMPTY;
			case 158/*ERROR_NOT_LOCKED*/:
				return EACCES;
			case 161/*ERROR_BAD_PATHNAME*/:
				return ENOENT;
			case 164/*ERROR_MAX_THRDS_REACHED*/:
				return EAGAIN;
			case 167/*ERROR_LOCK_FAILED*/:
				return EACCES;
			case 183/*ERROR_ALREADY_EXISTS*/:
				return EEXIST;
			case 206/*ERROR_FILENAME_EXCED_RANGE*/:
				return ENOENT;
			case 215/*ERROR_NESTING_NOT_ALLOWED*/:
				return EAGAIN;
			case 1816/*ERROR_NOT_ENOUGH_QUOTA*/:
				return ENOMEM;
			default:
				if (19/*ERROR_WRITE_PROTECT*/ <= errorCode && errorCode <= 36/*ERROR_SHARING_BUFFER_EXCEEDED*/) {
					return EACCES;
				} else if (188/*ERROR_INVALID_STARTING_CODESEG*/ <= errorCode && errorCode <= 202/*ERROR_INFLOOP_IN_RELOC_CHAIN*/) {
					return ENOEXEC;
				}
			}
		} else if (from == PlatformType::Unix && to == PlatformType::Windows) {
			switch (errorCode)
			{
			case EPERM:
			case EINVAL:
				return 1/*ERROR_INVALID_FUNCTION*/;
			case ENOENT:
				return 2/*ERROR_FILE_NOT_FOUND*/;
			case EMFILE:
				return 4/*ERROR_TOO_MANY_OPEN_FILES*/;
			case EACCES:
				return 5/*ERROR_ACCESS_DENIED*/;
			case EBADF:
				return 6/*ERROR_INVALID_HANDLE*/;
			case ENOMEM:
				return 8/*ERROR_NOT_ENOUGH_MEMORY*/;
			case EEXIST:
				return 80/*ERROR_FILE_EXISTS*/;
			case ENOTEMPTY:
				return 145/*ERROR_DIR_NOT_EMPTY*/;
			case ENOSPC:
				return 112/*ERROR_DISK_FULL*/;

			case E2BIG:
				return 10/*ERROR_BAD_ENVIRONMENT*/;
			case ENOEXEC:
				return 11/*ERROR_BAD_FORMAT*/;
			case EXDEV:
				return 17/*ERROR_NOT_SAME_DEVICE*/;
			case EAGAIN:
				return 89/*ERROR_NO_PROC_SLOTS*/;
			case EPIPE:
				return 109/*ERROR_BROKEN_PIPE*/;
			case ECHILD:
				return 128/*ERROR_WAIT_NO_CHILDREN*/;
			}
		}
		return errorCode;
	}

	String System::getLastErrorMessage()
	{
		sl_uint32 err = getLastError();
		if (err) {
			return formatErrorCode(err);
		}
		return sl_null;
	}

#if defined(SLIB_PLATFORM_IS_MOBILE)
	void System::setCrashHandler(SIGNAL_HANDLER handler)
	{
	}
#endif
	
#if !defined(SLIB_PLATFORM_IS_WIN32)
	void System::setDebugFlags()
	{
	}
#endif

	void System::yield(sl_uint32 elapsed)
	{
		if (elapsed < 16) {
			return;
		}
		if (elapsed < 32) {
			System::yield();
		} else {
			System::sleep(1);
		}
	}

#if defined(SLIB_PLATFORM_IS_MOBILE)

	void System::registerApplicationRunAtStartup(const String& path)
	{
	}
	
	void System::registerApplicationRunAtStartup()
	{
	}
	
	void System::unregisterApplicationRunAtStartup(const String& path)
	{
	}
	
	void System::unregisterApplicationRunAtStartup()
	{
	}
	
#endif
	
}
