/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/platform.h"

#include "slib/io/file.h"
#include "slib/core/variant.h"
#include "slib/core/scoped_buffer.h"
#include "slib/core/mio.h"
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
		static sl_uint8 _version[sizeof(WindowsVersion)] = {0};
		WindowsVersion& version = *((WindowsVersion*)_version);
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
		flagRunAsAdmin = sl_false;
		flagWait = sl_false;
		hWndParent = NULL;
		nShow = SW_NORMAL;
	}

	sl_bool Win32::shell(const ShellExecuteParam& param)
	{
		SHELLEXECUTEINFOW sei = { 0 };
		sei.cbSize = sizeof(sei);
		StringCstr16 operation(param.operation);
		if (param.flagRunAsAdmin) {
			sei.lpVerb = L"runas";
		} else if (param.operation.isNotEmpty()) {
			sei.lpVerb = (LPCWSTR)(operation.getData());
		}
		StringCstr16 path(param.path);
		sei.lpFile = (LPCWSTR)(path.getData());
		StringCstr16 params(param.parameters);
		if (param.parameters.isNotEmpty()) {
			sei.lpParameters = (LPCWSTR)(params.getData());
		}
		StringCstr16 currentDirectory(param.currentDirectory);
		if (param.currentDirectory.isNotEmpty()) {
			sei.lpDirectory = (LPCWSTR)(currentDirectory.getData());
		}
		sei.hwnd = param.hWndParent;
		sei.nShow = param.nShow;
		if (param.flagWait) {
			sei.fMask = SEE_MASK_NOCLOSEPROCESS;
		}
		if (ShellExecuteExW(&sei)) {
			if (param.flagWait) {
				WaitForSingleObject(sei.hProcess, INFINITE);
				CloseHandle(sei.hProcess);
			}
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
			String16 path = String16::concat(StringView16::literal(u"Software\\Classes\\."), ext);
			Variant orig = win32::Registry::getValue(HKEY_CURRENT_USER, path, sl_null);
			if (orig.isStringType() && orig.getString16() == progId.toString16()) {
				return sl_true;
			}
			return win32::Registry::setValue(HKEY_CURRENT_USER, path, sl_null, progId);
		}

		static sl_bool RegisterProgId(const StringParam& progId, const StringParam& appPath)
		{
			String16 path = String16::concat(StringView16::literal(u"Software\\Classes\\"), progId, StringView16::literal(u"\\shell\\open\\command"));
			String16 value = String16::concat(appPath, StringView16::literal(u" \"%1\""));
			Variant orig = win32::Registry::getValue(HKEY_CURRENT_USER, path, sl_null);
			if (orig.isStringType() && orig.getString16() == value) {
				return sl_true;
			}
			if (win32::Registry::setValue(HKEY_CURRENT_USER, path, sl_null, value)) {
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

	Variant Win32::getVariantFromVARIANT(const void* pVariant)
	{
		VARIANT& var = *((VARIANT*)pVariant);
		switch (var.vt) {
			case VT_NULL:
				return sl_null;
			case VT_I2:
				return var.iVal;
			case VT_I4:
				return var.lVal;
			case VT_R4:
				return var.fltVal;
			case VT_R8:
				return var.dblVal;
			case VT_BSTR:
				return String16::from(var.bstrVal);
			case VT_BOOL:
				return var.boolVal != 0;
			case VT_I1:
				return var.cVal;
			case VT_UI1:
				return var.bVal;
			case VT_UI2:
				return var.uiVal;
			case VT_UI4:
				return var.ulVal;
			case VT_I8:
				return (sl_int64)(var.llVal);
			case VT_UI8:
				return (sl_uint64)(var.ullVal);
			case VT_INT:
				return var.intVal;
			case VT_UINT:
				return var.uintVal;
			case VT_DATE:
				return Time::withDaysF(var.date - 25569.0);
			case VT_CY:
				return String::concat(String::fromInt64(var.cyVal.int64 / 10000), ".", String::fromUint32(Math::abs((sl_int32)(var.cyVal.int64 % 10000))));
			case VT_DECIMAL:
				{
					BSTR s = NULL;
					HRESULT hr = VarBstrFromDec(&(var.decVal), LCID_INSTALLED, 0, &s);
					if (SUCCEEDED(hr)) {
						String16 ret = String16::from(s);
						SysFreeString(s);
						return ret;
					}
					break;
				}
			default:
				break;
		}
		return Variant();
	}

	sl_bool Win32::getSYSTEMTIME(SYSTEMTIME& st, const Time& time, sl_bool flagUTC)
	{
		sl_int64 n = time.toWindowsFileTime();
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
		_out.setWindowsFileTime(n);
		return sl_true;
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

	namespace
	{
		static HDESK GetInputDesktop()
		{
			return OpenInputDesktop(0, FALSE, DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL | DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
		}

		static String16 GetDesktopName(HDESK hDesk)
		{
			WCHAR name[256] = { 0 };
			DWORD size = 0;
			if (GetUserObjectInformationW(hDesk, UOI_NAME, name, sizeof(name) - 2, &size)) {
				return String16::from((sl_char16*)name, Base::getStringLength2((sl_char16*)name, CountOfArray(name)));
			}
			return sl_null;
		}

		static sl_bool SwitchToDesktop(HDESK hDesktop)
		{
			HDESK hDesktopOld = GetThreadDesktop(GetCurrentThreadId());
			if (SetThreadDesktop(hDesktop)) {
				if (hDesktopOld) {
					CloseDesktop(hDesktopOld);
				}
				return sl_true;
			} else {
				return sl_false;
			}
		}
	}

	String16 Win32::getCurrentDesktopName()
	{
		HDESK hCurrent = GetThreadDesktop(GetCurrentThreadId());
		if (hCurrent) {
			return GetDesktopName(hCurrent);
		}
		return sl_null;
	}

	String16 Win32::getInputDesktopName()
	{
		String16 ret;
		HDESK hInput = GetInputDesktop();
		if (hInput) {
			ret = GetDesktopName(hInput);
			CloseDesktop(hInput);
		}
		return ret;
	}

	sl_bool Win32::switchToInputDesktop()
	{
		HDESK hDesktop = GetInputDesktop();
		if (hDesktop) {
			if (SwitchToDesktop(hDesktop)) {
				return sl_true;
			}
			CloseDesktop(hDesktop);
		}
		return sl_false;
	}

}

#endif
