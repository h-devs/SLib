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

#include "slib/platform/definition.h"

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/io/file.h"
#include "slib/core/variant.h"
#include "slib/core/scoped_buffer.h"
#include "slib/platform.h"
#include "slib/platform/win32/registry.h"
#include "slib/dl/win32/shlwapi.h"

#include <objbase.h>
#include <shellapi.h>
#pragma warning(disable: 4091)
#include <shlobj.h>

namespace slib
{

	String Win32::getStringFromGUID(const GUID& guid)
	{
		WCHAR sz[40] = { 0 };
		if (StringFromGUID2(guid, sz, 40) < 40) {
			return String::create((sl_char16*)sz);
		}
		return sl_null;
	}

	sl_bool Win32::getGUIDFromString(const String& _str, GUID* pguid)
	{
		StringCstr16 str(_str);
		CLSID clsid;
		HRESULT hr = CLSIDFromString((LPWSTR)(str.getData()), &clsid);
		if (hr == NOERROR) {
			if (pguid) {
				*pguid = clsid;
			}
			return sl_true;
		}
		return sl_false;
	}

	HGLOBAL Win32::createGlobalData(const void* data, sl_size size)
	{
		HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, size);
		if (handle) {
			void* dst = GlobalLock(handle);
			if (dst) {
				Base::copyMemory(dst, data, size);
				GlobalUnlock(dst);
			}
		}
		return handle;
	}

	namespace
	{
		typedef UINT32 (WINAPI* TYPE_RtlGetVersion)(PRTL_OSVERSIONINFOEXW lpVersionInformation);

		static void GetWindowsVersion(WindowsVersion& version)
		{
			HMODULE hModule = LoadLibraryW(L"ntdll.dll");
			if (hModule) {
				TYPE_RtlGetVersion func = (TYPE_RtlGetVersion)(GetProcAddress(hModule, "RtlGetVersion"));
				RTL_OSVERSIONINFOEXW vi = {0};
				vi.dwOSVersionInfoSize = sizeof(vi);
				func(&vi);
				version.majorVersion = (sl_uint32)(vi.dwMajorVersion);
				version.minorVersion = (sl_uint32)(vi.dwMinorVersion);
				version.servicePackMajorVersion = (sl_uint16)(vi.wServicePackMajor);
				version.servicePackMinorVersion = (sl_uint16)(vi.wServicePackMinor);
				version.buildNumber = (sl_uint32)(vi.dwBuildNumber);
				version.productType = (WindowsProductType)(vi.wProductType);
				FreeLibrary(hModule);
			}
		}
	}

	const WindowsVersion& Win32::getVersion()
	{
		static sl_bool flagChecked = sl_false;
		static WindowsVersion version = {0};
		if (flagChecked) {
			return version;
		}
		GetWindowsVersion(version);
		flagChecked = sl_true;
		return version;
	}

	sl_bool Win32::isWindowsServer()
	{
		return getVersion().productType != WindowsProductType::Workstation;
	}

	sl_bool Win32::isWindows7OrGreater()
	{
		const WindowsVersion& version = getVersion();
		return version.productType == WindowsProductType::Workstation && (version.majorVersion > WindowsVersion::Win7_MajorVersion || (version.majorVersion == WindowsVersion::Win7_MajorVersion && version.minorVersion >= WindowsVersion::Win7_MinorVersion));
	}

	sl_bool Win32::isWindows10OrGreater()
	{
		const WindowsVersion& version = getVersion();
		return version.productType == WindowsProductType::Workstation && version.majorVersion >= WindowsVersion::Win10_MajorVersion;
	}

	WindowsDllVersion Win32::getDllVersion(const StringParam& _pathDll)
	{
		StringCstr16 pathDll(_pathDll);
		WindowsDllVersion ret;
		ret.majorVersion = 0;
		ret.minorVersion = 0;
		ret.buildNumber = 0;
		HINSTANCE hDll = LoadLibraryW((LPCWSTR)(pathDll.getData()));
		if (hDll) {
			DLLGETVERSIONPROC proc = (DLLGETVERSIONPROC)(GetProcAddress(hDll, "DllGetVersion"));
			if (proc) {
				DLLVERSIONINFO info;
				Base::zeroMemory(&info, sizeof(info));
				info.cbSize = sizeof(info);
				HRESULT hr = proc(&info);
				if (SUCCEEDED(hr)) {
					ret.majorVersion = (sl_uint32)(info.dwMajorVersion);
					ret.minorVersion = (sl_uint32)(info.dwMinorVersion);
					ret.buildNumber = (sl_uint32)(info.dwBuildNumber);
				}
			}
			FreeLibrary(hDll);
		}
		return ret;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ShellExecuteParam)

	ShellExecuteParam::ShellExecuteParam()
	{
		runAsAdmin = sl_false;
		hWndParent = NULL;
		nShow = SW_NORMAL;
	}

	sl_bool Win32::shell(const ShellExecuteParam& param)
	{
		SHELLEXECUTEINFOW sei;
		Base::zeroMemory(&sei, sizeof(sei));
		sei.cbSize = sizeof(sei);
		StringCstr16 operation(param.operation);
		if (param.runAsAdmin) {
			sei.lpVerb = L"runas";
		} else if (param.operation.isNotEmpty()) {
			sei.lpVerb = (LPCWSTR)(operation.getData());
		}
		StringCstr16 path(param.path);
		sei.lpFile = (LPCWSTR)(path.getData());
		StringCstr16 params(param.params);
		if (param.params.isNotEmpty()) {
			sei.lpParameters = (LPCWSTR)(params.getData());
		}
		StringCstr16 currentDirectory(param.currentDirectory);
		if (param.currentDirectory.isNotEmpty()) {
			sei.lpDirectory = (LPCWSTR)(currentDirectory.getData());
		}
		sei.hwnd = param.hWndParent;
		sei.nShow = param.nShow;
		if (ShellExecuteExW(&sei)) {
			return sl_true;
		}
		return sl_false;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ShellOpenFolderAndSelectItemsParam)

	ShellOpenFolderAndSelectItemsParam::ShellOpenFolderAndSelectItemsParam()
	{
		flagEdit = sl_false;
		flagOpenDesktop = sl_false;
	}

	sl_bool Win32::shell(const ShellOpenFolderAndSelectItemsParam& param)
	{
		sl_bool bRet = sl_false;

		StringCstr16 path(param.path);

		PIDLIST_ABSOLUTE pidl = ILCreateFromPathW((LPCWSTR)(path.getData()));
		if (pidl) {

			DWORD dwFlags = 0;
			if (param.flagEdit) {
				dwFlags |= 1; // OFASI_EDIT;
			}
			if (param.flagOpenDesktop) {
				dwFlags |= 2; // OFASI_OPENDESKTOP;
			}

			ListLocker<StringParam> items(param.items);

			sl_uint32 n = (sl_uint32)(items.count);
			SLIB_SCOPED_BUFFER(LPITEMIDLIST, 256, arr, n)
			if (!arr) {
				n = 0;
			}

			sl_uint32 i;
			for (i = 0; i < n; i++) {
				StringCstr16 pathItem(items[i]);
				arr[i] = ILCreateFromPathW((LPCWSTR)(pathItem.getData()));
				if (!(arr[i])) {
					break;
				}
			}

			if (i == n) {
				HRESULT hr = SHOpenFolderAndSelectItems(pidl, n, (LPCITEMIDLIST*)arr, dwFlags);
				bRet = hr == S_OK;
			}

			n = i;
			for (i = 0; i < n; i++) {
				ILFree(arr[i]);
			}

			ILFree(pidl);
		}

		return bRet;
	}

	sl_bool Win32::createShortcut(const StringParam& _pathTarget, const StringParam& _pathLink)
	{
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		IShellLinkW* psl = NULL;
		HRESULT hr = CoCreateInstance(__uuidof(ShellLink), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
		if (SUCCEEDED(hr)) {
			StringCstr16 pathTarget(_pathTarget);
			psl->SetPath((LPCWSTR)(pathTarget.getData()));
			StringCstr16 workDir(File::getParentDirectoryPath(pathTarget));
			psl->SetWorkingDirectory((LPCWSTR)(workDir.getData()));
			IPersistFile* ppf = NULL;
			hr = psl->QueryInterface(IID_PPV_ARGS(&ppf));
			if (SUCCEEDED(hr)) {
				StringCstr16 pathLink(_pathLink);
				hr = ppf->Save((LPCWSTR)(pathLink.getData()), TRUE);
				ppf->Release();
			}
			psl->Release();
		}
		return SUCCEEDED(hr);
	}

	namespace {

		static sl_bool RegisterFileExtensionToProgId(const StringParam& ext, const StringParam& progId)
		{
			return win32::Registry::setValue(HKEY_CURRENT_USER, String16::concat(StringView16::literal(u"Software\\Classes\\."), ext), sl_null, progId);
		}

		static sl_bool RegisterProgId(const StringParam& progId, const StringParam& appPath)
		{
			if (win32::Registry::setValue(HKEY_CURRENT_USER, String16::concat(StringView16::literal(u"Software\\Classes\\"), progId, StringView16::literal(u"\\shell\\open\\command")), sl_null, String16::concat(appPath, StringView16::literal(u" \"%1\"")))) {
				SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
				return sl_true;
			}
			return sl_false;
		}

	}

	sl_bool Win32::registerFileExtension(const StringParam& ext, const StringParam& progId, const StringParam& appPath)
	{
		if (!(RegisterFileExtensionToProgId(ext, progId))) {
			return sl_false;
		}
		return RegisterProgId(progId, appPath);
	}

	sl_bool Win32::registerFileExtensions(const ListParam<StringParam>& extensions, const StringParam& progId, const StringParam& appPath)
	{
		ListElements<StringParam> list(extensions);
		if (!(list.count)) {
			return sl_false;
		}
		for (sl_size i = 0; i < list.count; i++) {
			if (!(RegisterFileExtensionToProgId(list[i], progId))) {
				return sl_false;
			}
		}
		return RegisterProgId(progId, appPath);
	}

	sl_bool Win32::getSYSTEMTIME(SYSTEMTIME& st, const Time& time, sl_bool flagUTC)
	{
		sl_int64 n = (time.toInt() + SLIB_INT64(11644473600000000)) * 10;  // Convert 1970 Based (time_t mode) to 1601 Based (FILETIME mode)
		if (flagUTC) {
			if (!(FileTimeToSystemTime((PFILETIME)&n, &st))) {
				return sl_false;
			}
		} else {
			SYSTEMTIME utc;
			if (!(FileTimeToSystemTime((PFILETIME)&n, &utc))) {
				return sl_false;
			}
			if (!(SystemTimeToTzSpecificLocalTime(NULL, &utc, &st))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool Win32::getTime(Time& _out, const SYSTEMTIME& st, sl_bool flagUTC)
	{
		sl_int64 n = 0;
		if (flagUTC) {
			if (!(SystemTimeToFileTime(&st, (PFILETIME)&n))) {
				return sl_false;
			}
		} else {
			SYSTEMTIME utc;
			Base::zeroMemory(&utc, sizeof(utc));
			if (!(TzSpecificLocalTimeToSystemTime(NULL, &st, &utc))) {
				return sl_false;
			}
			if (!(SystemTimeToFileTime(&utc, (PFILETIME)&n))) {
				return sl_false;
			}
		}
		return n / 10 - SLIB_INT64(11644473600000000);  // Convert 1601 Based (FILETIME mode) to 1970 Based (time_t mode)
	}

	HANDLE Win32::createDeviceHandle(const StringParam& _path, DWORD dwDesiredAccess, DWORD dwShareMode)
	{
		StringCstr16 path(_path);
		sl_char16 tmpDosName[] = SLIB_UNICODE("\\\\.\\A:");
		if (!(path.startsWith(SLIB_UNICODE("\\\\.\\")))) {
			sl_char16* s = path.getData();
			sl_char16 c = *s;
			if (path.getLength() >= 2 && SLIB_CHAR_IS_ALPHA(c) && s[1] == ':') {
				tmpDosName[4] = SLIB_CHAR_LOWER_TO_UPPER(c);
				path = tmpDosName;
			} else {
				if (path.startsWith(SLIB_UNICODE("\\\\?\\Volume{")) && path.endsWith('\\')) {
					// we need to remove trailing back-slash
					sl_reg index = path.indexOf('}');
					if (index >= 0) {
						path = String16(path.substring(0, index + 1));
					}
				}
			}
		}
		return CreateFileW((LPCWSTR)(path.getData()), dwDesiredAccess, dwShareMode, NULL, OPEN_EXISTING, 0, NULL);
	}

}

#endif
