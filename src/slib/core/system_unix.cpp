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

#if defined(SLIB_PLATFORM_IS_UNIX)

#include "slib/core/system.h"

#include "slib/core/process.h"
#include "slib/core/string.h"
#include "slib/core/list.h"
#include "slib/core/safe_static.h"
#include "slib/io/file.h"
#include "slib/data/ini.h"
#include "slib/dl/linux/rt.h"

#include <string.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <pwd.h>
#include <stdlib.h>

#include <sys/time.h>
#include <sys/utsname.h>

#ifdef assert
#undef assert
#endif

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
#	include "slib/dl/linux/rt.h"
#endif

#define PRIV_PATH_MAX 1024

namespace slib
{

	namespace priv
	{
		void Assert(const char* msg, const char* file, sl_uint32 line) noexcept
		{
#if defined(SLIB_DEBUG)
#if defined(SLIB_PLATFORM_IS_ANDROID)
			__assert(file, line, msg);
#else
			__assert(msg, file, line);
#endif
#endif
		}
	}

	namespace {

#if !defined(SLIB_PLATFORM_IS_ANDROID) && !defined(SLIB_PLATFORM_IS_APPLE)
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicString, g_strSystemName)
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicString, g_strSystemVersion)

		static void InitSystemNameAndVersion()
		{
			if (g_strSystemName.isNotNull()) {
				return;
			}
#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
			String strRelease = File::readAllTextUTF8("/etc/os-release");
			if (strRelease.isNotEmpty()) {
				// FIXME: parse non-quoted values such as NAME=Fedora, VERSION_ID=32
				sl_reg indexVersion = strRelease.indexOf("VERSION_ID=\"");
				sl_reg indexName = strRelease.indexOf("NAME=\"");
				if (indexName > 0 && strRelease.getAt(indexName - 1) == '_') {
					// prevent matching PRETTY_NAME, CFE_NAME, etc
					indexName = strRelease.indexOf("NAME=\"", indexName + 1);
				}
				if (indexVersion >= 0 && indexName >= 0) {
					indexVersion += 12;
					indexName += 6;
					sl_reg lastVersion = strRelease.indexOf('"', indexVersion);
					sl_reg lastName = strRelease.indexOf('"', indexName);
					if (lastVersion >= 0 && lastName >= 0) {
						g_strSystemVersion = strRelease.substring(indexVersion, lastVersion);
						g_strSystemName = String::concat(strRelease.substring(indexName, lastName), " ", g_strSystemVersion);
						return;
					}
				}
			}
#endif
			utsname systemInfo;
			uname(&systemInfo);
			g_strSystemName = String::concat(systemInfo.sysname, " ", systemInfo.release);
			g_strSystemVersion = systemInfo.release;
		}
#endif

	}

#if !defined(SLIB_PLATFORM_IS_APPLE) && !defined(SLIB_PLATFORM_IS_ANDROID)
	String System::getApplicationPath()
	{
		char path[PRIV_PATH_MAX] = {0};
		int n = readlink("/proc/self/exe", path, PRIV_PATH_MAX-1);
		/*
		-- another solution --

			char a[50];
			sprintf(a, "/proc/%d/exe", getpid());
			int n = readlink(a, path, size);
		*/
		String ret;
		if (n > 0) {
			ret = String::fromUtf8(path, n);
		}
		return ret;
	}

	String System::getHomeDirectory()
	{
		passwd* pwd = getpwuid(getuid());
		return pwd->pw_dir;
	}

	String System::getTempDirectory()
	{
		return "/tmp";
	}
#endif

	String System::getCurrentDirectory()
	{
		char path[PRIV_PATH_MAX] = {0};
		char* r = getcwd(path, PRIV_PATH_MAX-1);
		if (r) {
			return path;
		}
		return sl_null;
	}

	sl_bool System::setCurrentDirectory(const StringParam& _dir)
	{
		StringCstr dir(_dir);
		int iRet = chdir(dir.getData());
		if (iRet == 0) {
			return sl_true;
		}
		return sl_false;
	}

	String System::getEnvironmentVariable(const StringParam& _name)
	{
		if (_name.isNull()) {
			return sl_null;
		}
		StringCstr name(_name);
		return getenv(name.getData());
	}

	sl_bool System::setEnvironmentVariable(const StringParam& _name, const StringParam& _value)
	{
		if (_name.isNull()) {
			return sl_false;
		}
		StringCstr name(_name);
		StringCstr value(_value);
		if (_value.isNotNull()) {
			StringCstr value(_value);
			return !(setenv(name.getData(), value.getData(), 1));
		} else {
			return !(unsetenv(name.getData()));
		}
	}

	sl_bool System::is64BitSystem()
	{
#ifdef SLIB_PLATFORM_IS_WIN64
		return sl_true;
#else
		return sl_false;
#endif
	}

#if !defined(SLIB_PLATFORM_IS_ANDROID) && !defined(SLIB_PLATFORM_IS_APPLE)
	String System::getSystemVersion()
	{
		InitSystemNameAndVersion();
		return g_strSystemVersion;
	}

	String System::getSystemName()
	{
		InitSystemNameAndVersion();
		return g_strSystemName;
	}
#endif

#if !defined(SLIB_PLATFORM_IS_ANDROID)
	String System::getMachineName()
	{
		utsname systemInfo;
		uname(&systemInfo);
		return systemInfo.machine;
	}
#endif

#if !defined(SLIB_PLATFORM_IS_APPLE) && !defined(SLIB_PLATFORM_IS_ANDROID)
	String System::getComputerName()
	{
		char buf[512] = {0};
		gethostname(buf, 500);
		return buf;
	}
#endif

	String System::getUserId()
	{
		return String::fromUint32((sl_uint32)(getuid()));
	}

#if !defined(SLIB_PLATFORM_IS_APPLE)
	String System::getUserName()
	{
		return getlogin();
	}

	String System::getFullUserName()
	{
		return getUserName();
	}

	String System::getActiveUserName(String* outActiveSessionName)
	{
#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
		String sessionName = File::readAllTextUTF8("/sys/class/tty/tty0/active").trim();
		if (outActiveSessionName) {
			*outActiveSessionName = sessionName;
		}
		if (sessionName.isNotEmpty()) {
			ListElements<String> sessions(Process::getOutput("loginctl", "list-sessions", "--no-legend").split('\n'));
			for (sl_size i = 0; i < sessions.count; i++) {
				String row = sessions[i].trim();
				if (row.endsWith(sessionName)) {
					String sid = row.split(' ', 1).getFirstValue_NoLock();
					Ini session;
					if (session.parseText(Process::getOutput("loginctl", "show-session", sid))) {
						if (session.getValue("Active") == "yes" && session.getValue("Remote") == "no") {
							return session.getValue("Name");
						}
					}
				}
			}
		}
#endif
		return sl_null;
	}
#endif

	sl_uint32 System::getTickCount()
	{
		return (sl_uint32)(getTickCount64());
	}

#if !defined(SLIB_PLATFORM_IS_APPLE)
	sl_uint64 System::getTickCount64()
	{
#if defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0 && defined(CLOCK_MONOTONIC)
		{
			static sl_bool flagCheck = sl_true;
			static sl_bool flagEnabled = sl_false;
			struct timespec ts;
			if (flagEnabled) {
				clock_gettime(CLOCK_MONOTONIC, &ts);
				return (sl_uint64)(ts.tv_sec) * 1000 + (sl_uint64)(ts.tv_nsec) / 1000000;
			} else {
				if (flagCheck) {
					flagCheck = sl_false;
					int iRet = clock_gettime(CLOCK_MONOTONIC, &ts);
					if (iRet >= 0) {
						flagEnabled = sl_true;
						return (sl_uint64)(ts.tv_sec) * 1000 + (sl_uint64)(ts.tv_nsec) / 1000000;
					}
				}
			}
		}
#endif
		struct timeval tv;
		if (!(gettimeofday(&tv, 0))) {
			return (sl_uint64)(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
		} else {
			return 0;
		}
	}
#endif

	sl_uint64 System::getHighResolutionTickCount()
	{
		return (sl_uint64)(getTickCount64());
	}

	float System::getUptime()
	{
		String strUptime = File::readAllTextUTF8("/proc/uptime");
		float t;
		sl_reg iRet = String::parseFloat(&t, strUptime.getData(), 0, strUptime.getLength());
		if (iRet >= 0) {
			return t;
		}
		return 0;
	}

	void System::sleep(sl_uint32 milliseconds)
	{
		struct timespec req;
		req.tv_sec = milliseconds / 1000;
		req.tv_nsec = (milliseconds % 1000) * 1000000;
		nanosleep(&req, sl_null);
	}

	void System::yield()
	{
		sched_yield();
	}

	sl_int32 System::execute(const StringParam& _command)
	{
#if defined(SLIB_PLATFORM_IS_IOS)
		return -1;
#else
		StringCstr command(_command);
		return (sl_int32)(system(command.getData()));
#endif
	}

	void System::assert(const StringParam& _msg, const StringParam& _file, sl_uint32 line)
	{
#if defined(SLIB_DEBUG)
		StringCstr msg(_msg);
		StringCstr file(_file);
#if defined(SLIB_PLATFORM_IS_ANDROID)
		__assert(file.getData(), line, msg.getData());
#else
		__assert(msg.getData(), file.getData(), line);
#endif
#endif
	}

#if !defined(SLIB_PLATFORM_IS_MOBILE)
	namespace {
		static volatile double g_signal_fpe_dummy = 0.0f;
	}

	void System::setCrashHandler(SIGNAL_HANDLER handler)
	{
		g_signal_fpe_dummy = 0.0f;
		struct sigaction sa;
		sa.sa_flags = SA_NODEFER;
		sa.sa_handler = handler;
		sigemptyset(&(sa.sa_mask));
		sigaction(SIGFPE, &sa, sl_null);
		sigaction(SIGSEGV, &sa, sl_null);
		sigaction(SIGBUS, &sa, sl_null);
		sigaction(SIGILL, &sa, sl_null);
		sigaction(SIGABRT, &sa, sl_null);
		sigaction(SIGIOT, &sa, sl_null);
#	if defined(SLIB_PLATFORM_IS_MACOS)
		sigaction(SIGEMT, &sa, sl_null);
#	endif
		sigaction(SIGSYS, &sa, sl_null);
	}
#endif

	sl_uint32 System::getLastError()
	{
		return errno;
	}

	void System::setLastError(sl_uint32 errorCode)
	{
		errno = (int)errorCode;
	}

	String System::formatErrorCode(sl_uint32 errorCode)
	{
		String ret = strerror(errorCode);
		if (ret.isEmpty()) {
			return String::concat("Unknown error: ", String::fromUint32(errorCode));
		}
		return ret;
	}

}

#endif
