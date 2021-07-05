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

#ifndef CHECKHEADER_SLIB_CORE_WIN32_PLATFORM
#define CHECKHEADER_SLIB_CORE_WIN32_PLATFORM

#include "../definition.h"

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "windows.h"

#include "../string.h"
#include "../list.h"
#include "../time.h"
#include "../event.h"

namespace slib
{

#define PRIV_SLIB_WORKSTATION_VERSION_CODE(MAJOR, MINOR, SP) ((MAJOR << 16) | (MINOR << 8) | SP)
#define PRIV_SLIB_SERVER_VERSION_CODE(MAJOR, MINOR, SP) (0x01000000 | (MAJOR << 16) | (MINOR << 8) | SP)

#define SLIB_WINDOWS_MAJOR_VERSION(v) ((((int)v) >> 16) & 255)
#define SLIB_WINDOWS_MINOR_VERSION(v) ((((int)v) >> 8) & 255)
#define SLIB_WINDOWS_SERVICE_PACK(v) (((int)v) & 255)


	enum class WindowsVersion
	{
		XP = PRIV_SLIB_WORKSTATION_VERSION_CODE(5, 1, 0),
		XP_SP1 = PRIV_SLIB_WORKSTATION_VERSION_CODE(5, 1, 1),
		XP_SP2 = PRIV_SLIB_WORKSTATION_VERSION_CODE(5, 1, 2),
		XP_SP3 = PRIV_SLIB_WORKSTATION_VERSION_CODE(5, 1, 3),
		XP_64 = PRIV_SLIB_WORKSTATION_VERSION_CODE(5, 2, 0),
		Vista = PRIV_SLIB_WORKSTATION_VERSION_CODE(6, 0, 0),
		Vista_SP1 = PRIV_SLIB_WORKSTATION_VERSION_CODE(6, 0, 1),
		Vista_SP2 = PRIV_SLIB_WORKSTATION_VERSION_CODE(6, 0, 2),
		Windows7 = PRIV_SLIB_WORKSTATION_VERSION_CODE(6, 1, 0),
		Windows7_SP1 = PRIV_SLIB_WORKSTATION_VERSION_CODE(6, 1, 1),
		Windows8 = PRIV_SLIB_WORKSTATION_VERSION_CODE(6, 2, 0),
		Windows8_1 = PRIV_SLIB_WORKSTATION_VERSION_CODE(6, 3, 0),
		Windows10 = PRIV_SLIB_WORKSTATION_VERSION_CODE(10, 0, 0),
		Server2003 = PRIV_SLIB_SERVER_VERSION_CODE(5, 2, 0),
		Server2008 = PRIV_SLIB_SERVER_VERSION_CODE(6, 0, 0),
		Server2008_R2 = PRIV_SLIB_SERVER_VERSION_CODE(6, 1, 0),
		Server2012 = PRIV_SLIB_SERVER_VERSION_CODE(6, 2, 0),
		Server2012_R2 = PRIV_SLIB_SERVER_VERSION_CODE(6, 3, 0),
		Server2016 = PRIV_SLIB_SERVER_VERSION_CODE(10, 0, 0)
	};

	struct WindowsDllVersion
	{
		sl_uint32 major;
		sl_uint32 minor;
		sl_uint32 build;
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

	
		static Ref<Event> createEvent(HANDLE hEvent);

		static HANDLE getEventHandle(Event* event);
	
	
		static void setApplicationRunAtStartup(const StringParam& name, const StringParam& path, sl_bool flagRegister);


		static WindowsVersion getVersion();

		static WindowsDllVersion getDllVersion(const StringParam& pathDll);


		static sl_bool shell(const ShellExecuteParam& param);

		static sl_bool shell(const ShellOpenFolderAndSelectItemsParam& param);


		static sl_bool getSYSTEMTIME(const Time& time, sl_bool flagUTC, SYSTEMTIME* _out);

		static Time getTime(const SYSTEMTIME* st, sl_bool flagUTC);


		static HANDLE createDeviceHandle(const StringParam& path, DWORD dwDesiredAccess, DWORD dwShareMode);


		static sl_bool isWindowVisible(HWND hWnd);

	};
	
}

#endif

#endif
