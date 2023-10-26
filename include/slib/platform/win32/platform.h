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

#ifndef CHECKHEADER_SLIB_PLATFORM_WIN32_PLATFORM
#define CHECKHEADER_SLIB_PLATFORM_WIN32_PLATFORM

#include "../definition.h"

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "windows.h"

#include "../../core/string.h"
#include "../../core/list.h"
#include "../../core/time.h"
#include "../../core/event.h"

namespace slib
{

	class Variant;

	enum class WindowsProductType
	{
		Workstation = 1, // VER_NT_WORKSTATION
		DomainController = 2, // VER_NT_DOMAIN_CONTROLLER
		Server = 3 // VER_NT_SERVER
	};

	class WindowsVersion
	{
	public:
		sl_uint32 majorVersion;
		sl_uint32 minorVersion;
		sl_uint16 servicePackMajorVersion;
		sl_uint16 servicePackMinorVersion;
		sl_uint32 buildNumber;
		WindowsProductType productType;

	public:
		enum {
			Win2000_MajorVersion = 5,
			Win2000_MinorVersion = 0,
			XP_MajorVersion = 5,
			XP_MinorVersion = 1,
			XP64_MajorVersion = 5,
			XP64_MinorVersion = 2,
			Server2003_MajorVersion = 5,
			Server2003_MinorVersion = 2,
			Vista_MajorVersion = 6,
			Vista_MinorVersion = 0,
			Server2008_MajorVersion = 6,
			Server2008_MinorVersion = 0,
			Win7_MajorVersion = 6,
			Win7_MinorVersion = 1,
			Server2008R2_MajorVersion = 6,
			Server2008R2_MinorVersion = 1,
			Win8_MajorVersion = 6,
			Win8_MinorVersion = 2,
			Server2012_MajorVersion = 6,
			Server2012_MinorVersion = 2,
			Win8_1_MajorVersion = 6,
			Win8_1_MinorVersion = 3,
			Server2012R2_MajorVersion = 6,
			Server2012R2_MinorVersion = 3,
			Win10_MajorVersion = 10,
			Server2016_MajorVersion = 10,

			Win11_BuildNumber = 22000,
			Server2019_BuildNumber = 17763,
			Server2022_BuildNumber = 20348
		};
	};

	struct WindowsDllVersion
	{
		sl_uint32 majorVersion;
		sl_uint32 minorVersion;
		sl_uint32 buildNumber;
	};

	class ShellExecuteParam
	{
	public:
		StringParam operation;
		StringParam path;
		StringParam params;
		sl_bool runAsAdmin; // `shellExecute` returns sl_false if the user refused the elevation
		StringParam currentDirectory;
		HWND hWndParent;
		int nShow;

	public:
		ShellExecuteParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ShellExecuteParam)

	};

	class ShellOpenFolderAndSelectItemsParam
	{
	public:
		StringParam path;
		ListParam<StringParam> items;
		sl_bool flagEdit;
		sl_bool flagOpenDesktop;

	public:
		ShellOpenFolderAndSelectItemsParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ShellOpenFolderAndSelectItemsParam)

	};

	class SLIB_EXPORT Win32
	{
	public:
		static String getStringFromGUID(const GUID& guid);

		static sl_bool getGUIDFromString(const String& str, GUID* pguid);


		static HGLOBAL createGlobalData(const void* data, sl_size size);


		static Ref<Event> createEvent(HANDLE hEvent, sl_bool flagCloseOnRelease = sl_true);

		static HANDLE getEventHandle(Event* event);


		static void setApplicationRunAtStartup(const StringParam& name, const StringParam& path, sl_bool flagRegister);


		static const WindowsVersion& getVersion();

		static sl_bool isWindowsServer();

		static sl_bool isWindows7OrGreater();

		static sl_bool isWindows10OrGreater();

		static WindowsDllVersion getDllVersion(const StringParam& pathDll);


		static sl_bool shell(const ShellExecuteParam& param);

		static sl_bool shell(const ShellOpenFolderAndSelectItemsParam& param);

		static sl_bool createShortcut(const StringParam& pathTarget, const StringParam& pathLink);

		static sl_bool registerFileExtension(const StringParam& ext, const StringParam& progId, const StringParam& appPath);

		static sl_bool registerFileExtensions(const ListParam<StringParam>& extensions, const StringParam& progId, const StringParam& appPath);


		static Variant getVariantFromVARIANT(const void* pVariant);

		static sl_bool getSYSTEMTIME(SYSTEMTIME& _out, const Time& time, sl_bool flagUTC);

		static sl_bool getTime(Time& _out, const SYSTEMTIME& st, sl_bool flagUTC);


		static HANDLE createDeviceHandle(const StringParam& path, DWORD dwDesiredAccess, DWORD dwShareMode);

	};

}

#endif

#endif
