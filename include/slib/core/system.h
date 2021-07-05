/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_SYSTEM
#define CHECKHEADER_SLIB_CORE_SYSTEM

#include "platform_type.h"

namespace slib
{

	class String;
	class StringParam;

	typedef void (*SIGNAL_HANDLER)(int signal);
	typedef sl_bool(*DEBUG_ALLOC_HOOK)(void* ptr, sl_size size, sl_uint32 requestNumber);

#ifdef assert
#undef assert
#endif

	class SLIB_EXPORT System
	{
	public:
		static String getApplicationPath();

		static String getApplicationDirectory();

		static String getHomeDirectory();

		static String getLocalAppDataDirectory();

		static String getCachesDirectory();

		// per-user temp directory
		static String getTempDirectory();

		static String getCurrentDirectory();

		static sl_bool setCurrentDirectory(const StringParam& dir);
		
#ifdef SLIB_PLATFORM_IS_APPLE
		static String getMainBundlePath();
#endif
		
#ifdef SLIB_PLATFORM_IS_WIN32
		static String getWindowsDirectory();

		static String getSystemDirectory();

		static String getSystemWow64Directory();

		static String getProgramsDirectory();
#endif


		static sl_bool is64BitSystem();

		static String getName();

		static String getVersion();

#if defined(SLIB_PLATFORM_IS_WIN32) || defined(SLIB_PLATFORM_IS_APPLE)
		static sl_uint32 getMajorVersion();

		static sl_uint32 getMinorVersion();
#endif

		static String getMachineName();

		static String getComputerName();

		static String getUserId();
		
		static String getUserName();
		
		static String getFullUserName();


		static sl_uint32 getTickCount();
		
		static sl_uint64 getTickCount64();
	
		static sl_uint64 getHighResolutionTickCount();
	
		static void sleep(sl_uint32 millis);

		static void yield();

		static void yield(sl_uint32 elapsed);

		
		static sl_int32 execute(const StringParam& command);

		static void assert(const StringParam& msg, const StringParam& file, sl_uint32 line);

		static void setCrashHandler(SIGNAL_HANDLER handler);
		
		static void setDebugFlags();

		static void setDebugAllocHook(DEBUG_ALLOC_HOOK hook);

		
		static sl_uint32 getLastError();

		static void setLastError(sl_uint32 errorCode);

		static sl_uint32 mapError(sl_uint32 errorCode, PlatformType to, PlatformType from = PlatformType::Current);

		static String getLastErrorMessage();

		static String formatErrorCode(sl_uint32 errorCode);
		
	};
	
}

#endif

