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

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "slib/core/system.h"

#include "slib/core/file.h"
#include "slib/core/unique_ptr.h"
#include "slib/core/win32/platform.h"
#include "slib/core/dl/win32/kernel32.h"
#include "slib/core/dl/win32/wininet.h"
#include "slib/core/dl/win32/wtsapi32.h"

#include <assert.h>
#include <signal.h>
#include <float.h>
#include <stdlib.h>
#include <crtdbg.h>

#pragma warning(disable: 4091)
#include <shlobj.h>

#if defined(SLIB_PLATFORM_IS_UWP)
using namespace Win32::Storage;
using namespace Platform;
#endif

#pragma comment(lib, "version.lib")

#define PRIV_PATH_MAX 1024

#ifdef assert
#undef assert
#endif

namespace slib
{

	namespace priv
	{

		void Assert(const char* msg, const char* file, sl_uint32 line) noexcept
		{
#if defined(SLIB_DEBUG)
			System::assert(msg, file, line);
#endif
		}

		namespace system
		{

#ifdef SLIB_PLATFORM_IS_WIN32

			volatile double g_signal_fpe_dummy = 0.0f;
			SIGNAL_HANDLER g_handlerSignalCrash;
			DEBUG_ALLOC_HOOK g_debugAllocHook;

			static void DoHandleSignalCrash(int sig)
			{
				if (sig == SIGFPE) {
					_fpreset();
				}
				g_handlerSignalCrash(sig);
			}

			static LONG WINAPI DoHandleException(PEXCEPTION_POINTERS pExceptionPtrs)
			{
				DWORD code = pExceptionPtrs->ExceptionRecord->ExceptionCode;
				switch (code) {
				case EXCEPTION_ACCESS_VIOLATION:
				case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
				case EXCEPTION_DATATYPE_MISALIGNMENT:
				case EXCEPTION_FLT_DENORMAL_OPERAND:
				case EXCEPTION_FLT_DIVIDE_BY_ZERO:
				case EXCEPTION_FLT_INEXACT_RESULT:
				case EXCEPTION_FLT_INVALID_OPERATION:
				case EXCEPTION_FLT_OVERFLOW:
				case EXCEPTION_FLT_STACK_CHECK:
				case EXCEPTION_FLT_UNDERFLOW:
				case EXCEPTION_ILLEGAL_INSTRUCTION:
				case EXCEPTION_IN_PAGE_ERROR:
				case EXCEPTION_INT_DIVIDE_BY_ZERO:
				case EXCEPTION_INT_OVERFLOW:
				case EXCEPTION_INVALID_DISPOSITION:
				case EXCEPTION_NONCONTINUABLE_EXCEPTION:
				case EXCEPTION_PRIV_INSTRUCTION:
				case EXCEPTION_STACK_OVERFLOW:
					g_handlerSignalCrash(-1);
					break;
				}
				return EXCEPTION_EXECUTE_HANDLER;
			}

			static int DebugAllocHook(int allocType, void* userData, size_t size, int blockType, long requestNumber, const unsigned char* filename, int lineNumber)
			{
				return g_debugAllocHook(userData, (sl_size)size, (sl_uint32)requestNumber);
			}

			static BOOL GetVersionInfo(const StringParam& _filePath, sl_uint64* pFileVersion, sl_uint64* pProductVersion)
			{
				DWORD  verHandle = 0;
				UINT   size = 0;
				LPBYTE lpBuffer = NULL;
				StringCstr16 filePath(_filePath);
				DWORD  verSize = GetFileVersionInfoSizeW((LPCWSTR)(filePath.getData()), &verHandle);

				if (verSize != NULL) {
					UniquePtr<char[]> verData = new char[verSize];
					if (GetFileVersionInfoW((LPCWSTR)(filePath.getData()), verHandle, verSize, verData)) {
						if (VerQueryValueW(verData, L"\\", (LPVOID*)&lpBuffer, &size)) {
							if (size) {
								VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
								if (verInfo->dwSignature == 0xfeef04bd) {
									if (pFileVersion) {
										*pFileVersion = SLIB_MAKE_QWORD4(verInfo->dwFileVersionMS, verInfo->dwFileVersionLS);
									}
									if (pProductVersion) {
										*pProductVersion = SLIB_MAKE_QWORD4(verInfo->dwProductVersionMS, verInfo->dwProductVersionLS);
									}
									return TRUE;
								}
							}
						}
					}
				}

				return FALSE;
			}

			static String GetVersionInfo(const StringParam& _filePath, const StringParam& _verEntry)
			{
				DWORD  verHandle = 0;
				UINT   size = 0;
				LPBYTE lpBuffer = NULL;
				StringCstr16 filePath(_filePath);
				StringCstr16 verEntry(_verEntry);
				DWORD  verSize = GetFileVersionInfoSizeW((LPCWSTR)(filePath.getData()), &verHandle);

				struct LANGANDCODEPAGE {
					WORD wLanguage;
					WORD wCodePage;
				} *lpTranslate;

				if (verSize != NULL) {
					UniquePtr<char[]> verData = new char[verSize];
					if (GetFileVersionInfoW((LPCWSTR)(filePath.getData()), verHandle, verSize, verData)) {
						if (VerQueryValueW(verData, L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &size)) {
							if (size) {
								StringCstr16 subBlock = String16::concat(
									L"\\StringFileInfo\\",
									String16::fromUint32((*lpTranslate).wLanguage, 16, 4),
									String16::fromUint32((*lpTranslate).wCodePage, 16, 4),
									L"\\", verEntry);
								if (VerQueryValueW(verData, (LPCWSTR)(subBlock.getData()), (LPVOID*)&lpBuffer, &size)) {
									if (size) {
										return String::from((WCHAR*)(lpBuffer), Base::getStringLength2((sl_char16*)lpBuffer, size));
									}
								}
							}
						}
					}
				}

				return sl_null;
			}

#endif

		}
	}

	using namespace priv::system;

	String System::getApplicationPath()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		sl_char16 bufAppPath[PRIV_PATH_MAX] = { 0 };
		GetModuleFileNameW(GetModuleHandle(NULL), (WCHAR*)bufAppPath, PRIV_PATH_MAX - 1);
		return String::create(bufAppPath);
#endif
#if defined(SLIB_PLATFORM_IS_UWP)
		return String::fromUtf16(ApplicationData::Current->LocalFolder->Path->Data());
#endif
	}

	String System::getApplicationVersion()
	{
		return getFileVersion(System::getApplicationPath());
	}

	String System::getHomeDirectory()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		WCHAR path[MAX_PATH] = { 0 };
		if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, path))) {
			return String::create(path);
		}
#endif
		return getApplicationDirectory();
	}

	String System::getLocalAppDataDirectory()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		WCHAR path[MAX_PATH] = { 0 };
		if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
			return String::create(path);
		}
#endif
		return getApplicationDirectory();
	}

	String System::getTempDirectory()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		sl_char16 sz[PRIV_PATH_MAX] = { 0 };
		sl_int32 n = GetTempPathW(PRIV_PATH_MAX - 1, (LPWSTR)sz);
		if (sz[n - 1] == '\\') {
			n--;
		}
		return String::create(sz, n);
#else
		SLIB_STATIC_STRING(temp, "/temp");
		String dir = getApplicationDirectory() + temp;
		File::createDirectory(dir);
		return dir;
#endif
	}

	String System::getCurrentDirectory()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		WCHAR path[PRIV_PATH_MAX] = { 0 };
		GetCurrentDirectoryW(PRIV_PATH_MAX - 1, path);
		return String::create(path);
#else
		return getApplicationPath();
#endif
	}

	sl_bool System::setCurrentDirectory(const StringParam& _dir)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		StringCstr16 dir(_dir);
		if (SetCurrentDirectoryW((LPCWSTR)(dir.getData()))) {
			return sl_true;
		}
#endif
		return sl_false;
	}

#if defined(SLIB_PLATFORM_IS_WIN32)
	String System::getWindowsDirectory()
	{
		WCHAR path[MAX_PATH];
		UINT nLen = GetWindowsDirectoryW(path, MAX_PATH);
		return String::from(path, nLen);
	}

	String System::getSystemDirectory()
	{
		WCHAR path[MAX_PATH];
		UINT nLen = GetSystemDirectoryW(path, MAX_PATH);
		return String::from(path, nLen);
	}

	String System::getSystemWow64Directory()
	{
		WCHAR path[MAX_PATH];
		UINT nLen = GetSystemWow64DirectoryW(path, MAX_PATH);
		return String::from(path, nLen);
	}

	String System::getProgramsDirectory()
	{
		WCHAR path[MAX_PATH] = { 0 };
		if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, path))) {
			return String::create(path);
		}
		return sl_null;
	}
#endif

	String System::getEnvironmentVariable(const StringParam& _name)
	{
		if (_name.isNull()) {
			return sl_null;
		}
		StringCstr16 name(_name);
		WCHAR buf[1024];
		DWORD dwRet = GetEnvironmentVariableW((LPCWSTR)(name.getData()), buf, (DWORD)(CountOfArray(buf)));
		if (dwRet) {
			if (dwRet <= CountOfArray(buf)) {
				return String::from(buf, (sl_reg)dwRet);
			} else {
				DWORD n = dwRet;
				for (;;) {
					WCHAR* p = new WCHAR[n];
					if (p) {
						sl_bool flagRet = sl_false;
						String ret;
						dwRet = GetEnvironmentVariableW((LPCWSTR)(name.getData()), p, n);
						if (dwRet) {
							if (dwRet <= n) {
								ret = String::from(p, (sl_reg)dwRet);
								flagRet = sl_true;
							} else {
								n = dwRet;
							}
						} else {
							flagRet = sl_true;
						}
						delete[] p;
						if (flagRet) {
							return ret;
						}
					} else {
						return sl_null;
					}
				}
			}
		}
		return sl_null;
	}

	sl_bool System::setEnvironmentVariable(const StringParam& _name, const StringParam& _value)
	{
		if (_name.isNull()) {
			return sl_false;
		}
		StringCstr16 name(_name);
		if (_value.isNotNull()) {
			StringCstr16 value(_value);
			return SetEnvironmentVariableW((LPCWSTR)(name.getData()), (LPCWSTR)(value.getData()));
		} else {
			return SetEnvironmentVariableW((LPCWSTR)(name.getData()), NULL);
		}
	}

	sl_bool System::is64BitSystem()
	{
#if defined(SLIB_PLATFORM_IS_WIN64)
		return sl_true;
#elif defined(SLIB_PLATFORM_IS_WIN32)
		static sl_bool flag64Bit = sl_false;
		static sl_bool flagInit = sl_true;
		if (flagInit) {
			auto func = kernel32::getApi_IsWow64Process();
			if (func) {
				BOOL flag = FALSE;
				func(GetCurrentProcess(), &flag);
				flag64Bit = flag;
			}
			flagInit = sl_false;
		}
		return flag64Bit;
#else
		return sl_false;
#endif
	}

	String System::getSystemVersion()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		WindowsVersion version = Win32::getVersion();
		if ((SLIB_WINDOWS_IS_SERVER(version) && version >= WindowsVersion::Server2012) ||
			(!SLIB_WINDOWS_IS_SERVER(version) && version >= WindowsVersion::Windows8)) {
			String kernelVersion = getProductVersion(System::getSystemDirectory() + "/kernel32.dll");
			if (kernelVersion.isNotEmpty()) {
				return kernelVersion;
			}
		}
		return String::concat(String::fromUint32(SLIB_WINDOWS_MAJOR_VERSION(version)), ".", String::fromUint32(SLIB_WINDOWS_MINOR_VERSION(version)));
#else
		return "UWP";
#endif
	}
	
	String System::getSystemName()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		WindowsVersion version = Win32::getVersion();
		if ((SLIB_WINDOWS_IS_SERVER(version) && version >= WindowsVersion::Server2012) ||
			(!SLIB_WINDOWS_IS_SERVER(version) && version >= WindowsVersion::Windows8)) {
			/*
				Applications not manifested for Windows 8.1 or Windows 10 will return the Windows 8 OS version value (6.2).
				For more information, visit at https://docs.microsoft.com/en-us/windows/desktop/SysInfo/targeting-your-application-at-windows-8-1
			*/
			sl_uint64 productVersion = 0;
			if (getFileVersionInfo(System::getSystemDirectory() + "/kernel32.dll", sl_null, &productVersion)) {
				if (SLIB_WINDOWS_IS_SERVER(version)) {
					version = (WindowsVersion)(PRIV_SLIB_SERVER_VERSION_CODE(SLIB_GET_WORD3(productVersion), SLIB_GET_WORD2(productVersion), 0));
				} else {
					sl_uint16 major = SLIB_GET_WORD3(productVersion);
					sl_uint16 minor = SLIB_GET_WORD2(productVersion);
					sl_uint16 buildNumber = SLIB_GET_WORD1(productVersion);
					if (major >= 10 && buildNumber >= 22000) {
						// Windows 11
						version = (WindowsVersion)(PRIV_SLIB_WORKSTATION_VERSION_CODE(major, minor, 1));
					} else {
						version = (WindowsVersion)(PRIV_SLIB_WORKSTATION_VERSION_CODE(major, minor, 0));
					}
				}
			}
		}
		switch (version) {
		case WindowsVersion::Server2016:
			return "Windows Server 2016";
		case WindowsVersion::Server2012_R2:
			return "Windows Server 2012 R2";
		case WindowsVersion::Server2012:
			return "Windows Server 2012";
		case WindowsVersion::Server2008_R2:
			return "Windows Server 2008 R2";
		case WindowsVersion::Server2008:
			return "Windows Server 2008";
		case WindowsVersion::Server2003:
			return "Windows Server 2003";
		case WindowsVersion::Windows11:
			return "Windows 11";
		case WindowsVersion::Windows10:
			return "Windows 10";
		case WindowsVersion::Windows8_1:
			return "Windows 8.1";
		case WindowsVersion::Windows8:
			return "Windows 8";
		case WindowsVersion::Windows7_SP1:
			return "Windows 7 SP1";
		case WindowsVersion::Windows7:
			return "Windows 7";
		case WindowsVersion::Vista_SP2:
			return "Windows Vista SP2";
		case WindowsVersion::Vista_SP1:
			return "Windows Vista SP1";
		case WindowsVersion::Vista:
			return "Windows Vista";
		case WindowsVersion::XP_64:
			return "Windows XP 64bit";
		case WindowsVersion::XP_SP3:
			return "Windows XP SP3";
		case WindowsVersion::XP_SP2:
			return "Windows XP SP2";
		case WindowsVersion::XP_SP1:
			return "Windows XP SP1";
		default:
			return "Windows XP";
		}
#else
		return "UWP";
#endif
	}

#if defined(SLIB_PLATFORM_IS_WIN32)
	sl_uint32 System::getMajorVersion()
	{
		WindowsVersion version = Win32::getVersion();
		if ((SLIB_WINDOWS_IS_SERVER(version) && version >= WindowsVersion::Server2012) ||
			(!SLIB_WINDOWS_IS_SERVER(version) && version >= WindowsVersion::Windows8)) {
			sl_uint64 productVersion = 0;
			if (getFileVersionInfo(System::getSystemDirectory() + "/kernel32.dll", sl_null, &productVersion)) {
				return SLIB_GET_WORD3(productVersion);
			}
		}
		return SLIB_WINDOWS_MAJOR_VERSION(version);
	}

	sl_uint32 System::getMinorVersion()
	{
		WindowsVersion version = Win32::getVersion();
		if ((SLIB_WINDOWS_IS_SERVER(version) && version >= WindowsVersion::Server2012) ||
			(!SLIB_WINDOWS_IS_SERVER(version) && version >= WindowsVersion::Windows8)) {
			sl_uint64 productVersion = 0;
			if (getFileVersionInfo(System::getSystemDirectory() + "/kernel32.dll", sl_null, &productVersion)) {
				return SLIB_GET_WORD2(productVersion);
			}
		}
		return SLIB_WINDOWS_MINOR_VERSION(version);
	}

	sl_uint32 System::getPatchVersion()
	{
		WindowsVersion version = Win32::getVersion();
		return SLIB_WINDOWS_SERVICE_PACK(version);
	}

	String System::getBuildVersion()
	{
		sl_uint64 productVersion = 0;
		if (getFileVersionInfo(System::getSystemDirectory() + "/kernel32.dll", sl_null, &productVersion)) {
			return String::concat(String::fromUint32(SLIB_GET_WORD1(productVersion)), ".", String::fromUint32(SLIB_GET_WORD0(productVersion)));
		}
		return sl_null;
	}

	sl_bool System::getFileVersionInfo(const StringParam& filePath, sl_uint64* pFileVersion, sl_uint64* pProductVersion)
	{
		return GetVersionInfo(filePath, pFileVersion, pProductVersion);
	}

	String System::getFileVersion(const StringParam& filePath)
	{
		String version = GetVersionInfo(filePath, "FileVersion");
		if (version.isEmpty()) {
			sl_uint64 fileVersion = 0;
			if (GetVersionInfo(filePath, &fileVersion, sl_null)) {
				version = String::concat(
					String::fromUint32(SLIB_GET_WORD3(fileVersion)), ".",
					String::fromUint32(SLIB_GET_WORD2(fileVersion)), ".",
					String::fromUint32(SLIB_GET_WORD1(fileVersion)), ".",
					String::fromUint32(SLIB_GET_WORD0(fileVersion)));
			}
		}
		return version;
	}

	String System::getProductVersion(const StringParam& filePath)
	{
		String version = GetVersionInfo(filePath, "ProductVersion");
		if (version.isEmpty()) {
			sl_uint64 productVersion = 0;
			if (GetVersionInfo(filePath, sl_null, &productVersion)) {
				version = String::concat(
					String::fromUint32(SLIB_GET_WORD3(productVersion)), ".",
					String::fromUint32(SLIB_GET_WORD2(productVersion)), ".",
					String::fromUint32(SLIB_GET_WORD1(productVersion)), ".",
					String::fromUint32(SLIB_GET_WORD0(productVersion)));
			}
		}
		return version;
	}
#endif

	String System::getMachineName()
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		switch (si.wProcessorArchitecture) {
		case PROCESSOR_ARCHITECTURE_INTEL:
			return "x86";
		case PROCESSOR_ARCHITECTURE_AMD64:
			return "x64";
		case PROCESSOR_ARCHITECTURE_IA64:
			return "ia64";
		case PROCESSOR_ARCHITECTURE_ARM:
			return "arm";
		case 12: // PROCESSOR_ARCHITECTURE_ARM64
			return "arm64";
		}
		return "unknown";
	}

	String System::getComputerName()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		WCHAR buf[512] = { 0 };
		DWORD nBuf = 500;
		GetComputerNameW(buf, &nBuf);
		return String::fromUtf16((sl_char16*)buf, (sl_reg)nBuf);
#else
		return sl_null;
#endif
	}

	String System::getUserId()
	{
		return getUserName();
	}

	String System::getUserName()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		WCHAR buf[512] = { 0 };
		DWORD nBuf = 500;
		if (GetUserNameW(buf, &nBuf)) {
			if (nBuf) {
				return String::fromUtf16((sl_char16*)buf, (sl_reg)(nBuf - 1));
			}
		}
		return sl_null;
#else
		return "uwp";
#endif
	}
	
	String System::getFullUserName()
	{
		return getUserName();
	}

	String System::getActiveUserName(String* outActiveSessionName)
	{
		auto apiEnumSessions = wtsapi32::getApi_WTSEnumerateSessionsW();
		if (!apiEnumSessions) {
			return sl_null;
		}
		auto apiQuerySessionInfo = wtsapi32::getApi_WTSQuerySessionInformationW();
		if (!apiQuerySessionInfo) {
			return sl_null;
		}
		auto apiFreeMemory = wtsapi32::getApi_WTSFreeMemory();
		if (!apiFreeMemory) {
			return sl_null;
		}
		String ret;
		WTS_SESSION_INFOW* pSessionInfos = sl_null;
		DWORD nSessions = 0;
		if (apiEnumSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfos, &nSessions)) {
			for (DWORD iSession = 0; iSession < nSessions; iSession++) {
				WTS_SESSION_INFOW& si = pSessionInfos[iSession];
				if (si.State == WTSActive && ret.isNull()) {
					if (outActiveSessionName) {
						*outActiveSessionName = String::create(si.pWinStationName);
					}
					WCHAR* bufUserName = sl_null;
					DWORD sizeUserName = 0;
					if (apiQuerySessionInfo(WTS_CURRENT_SERVER_HANDLE, si.SessionId, WTSUserName, &bufUserName, &sizeUserName)) {
						if (bufUserName) {
							sl_size lenUserName = (sl_size)(sizeUserName >> 1);
							if (lenUserName) {
								ret = String::create(bufUserName, lenUserName - 1);
							}
							apiFreeMemory(bufUserName);
						}
					}
				}
			}
			apiFreeMemory(pSessionInfos);
		}
		return ret;
	}

	sl_uint32 System::getTickCount()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		return GetTickCount();
#else
		return (sl_uint32)(GetTickCount64());
#endif
	}

	sl_uint64 System::getTickCount64()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		auto func = kernel32::getApi_GetTickCount64();
		if (func) {
			return (sl_uint64)(func());
		}
		static sl_uint32 tickOld = 0;
		static sl_uint32 tickHigh = 0;
		SLIB_STATIC_SPINLOCKER(lock)
		sl_uint32 tick = GetTickCount();
		if (tick < tickOld) {
			tickHigh++;
		}
		tickOld = tick;
		return SLIB_MAKE_QWORD4(tickHigh, tick);
#else
		return (sl_uint64)(GetTickCount64());
#endif
	}

	sl_uint64 System::getHighResolutionTickCount()
	{
		static LARGE_INTEGER freq;
		static sl_bool flagInitQPC = sl_true;
		static sl_bool flagEnabledQPC = sl_false;
		if (flagInitQPC) {
			flagEnabledQPC = (sl_bool)(QueryPerformanceFrequency(&freq));
			flagInitQPC = sl_false;
		}
		if (flagEnabledQPC) {
			LARGE_INTEGER ticks;
			if (QueryPerformanceCounter(&ticks)) {
				return (sl_uint64)(ticks.QuadPart) * SLIB_UINT64(1000) / (sl_uint64)(freq.QuadPart);
			}
		}
		return getTickCount64();
	}

	void System::sleep(sl_uint32 milliseconds)
	{
		Sleep(milliseconds);
	}

	void System::yield()
	{
		if (!(SwitchToThread())) {
			Sleep(0);
		}
	}

	sl_int32 System::execute(const StringParam& _command)
	{
		StringCstr16 command(_command);
		return (sl_int32)(_wsystem((WCHAR*)(command.getData())));
	}

	void System::assert(const StringParam& _msg, const StringParam& _file, sl_uint32 line)
	{
#if defined(SLIB_DEBUG)
		StringCstr16 msg(_msg);
		StringCstr16 file(_file);
		_wassert((wchar_t*)(msg.getData()), (wchar_t*)(file.getData()), line);
#endif
	}

#if defined(SLIB_PLATFORM_IS_WIN32)
	void System::setCrashHandler(SIGNAL_HANDLER handler)
	{
		priv::system::g_handlerSignalCrash = handler;
		SetUnhandledExceptionFilter(DoHandleException);
		handler = DoHandleSignalCrash;
		signal(SIGFPE, handler);
		signal(SIGSEGV, handler);
		signal(SIGILL, handler);
		signal(SIGABRT, handler);
		signal(SIGABRT_COMPAT, handler);
	}

	void System::setDebugFlags()
	{
#ifdef SLIB_DEBUG
		int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
		// logically OR leak check bit
		flag |= _CRTDBG_LEAK_CHECK_DF;
		// set the flags again
		_CrtSetDbgFlag(flag);
#endif
	}

	void System::setDebugAllocHook(DEBUG_ALLOC_HOOK hook)
	{
#ifdef SLIB_DEBUG
		g_debugAllocHook = hook;
		_CrtSetAllocHook(DebugAllocHook);
#endif
	}
#endif

	sl_uint32 System::getLastError()
	{
		return (sl_uint32)(GetLastError());
	}

	void System::setLastError(sl_uint32 errorCode)
	{
		SetLastError((DWORD)errorCode);
	}

	String System::formatErrorCode(sl_uint32 errorCode)
	{
		String ret;
		if (errorCode > 0) {
			LPWSTR buf = sl_null;
			DWORD size = FormatMessageW(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
				(HMODULE)(wininet::getLibrary()),
				errorCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPWSTR)&buf, 0,
				NULL);
			if (buf) {
				ret = String::create(buf, size);
				LocalFree(buf);
			}
		}
		if (ret.isEmpty()) {
			return String::concat("Unknown error: ", String::fromUint32(errorCode));
		}
		return ret;
	}

}

#endif
