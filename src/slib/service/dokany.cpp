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

#include "slib/service/dokany.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "dokany/dokan.h"
#ifdef DOKANAPI
#undef DOKANAPI
#endif
#define DOKANAPI __stdcall

#include "slib/core/service_manager.h"
#include "slib/core/dynamic_library.h"
#include "slib/core/platform_windows.h"

#define DOKAN_DRIVER_SERVICE L"Dokan1"

namespace slib
{

	namespace priv
	{
		namespace dokany
		{

			void* g_libDll;

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanMain, int, DOKANAPI,
				PDOKAN_OPTIONS DokanOptions,
				PDOKAN_OPERATIONS DokanOperations)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanUnmount, BOOL, DOKANAPI,
				WCHAR DriveLetter)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanServiceInstall, BOOL, DOKANAPI,
				LPCWSTR ServiceName,
				DWORD ServiceType,
				LPCWSTR ServiceFullPath)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanServiceDelete, BOOL, DOKANAPI,
				LPCWSTR ServiceName)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanVersion, ULONG, DOKANAPI)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanDriverVersion, ULONG, DOKANAPI)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanResetTimeout, BOOL, DOKANAPI,
				ULONG Timeout,
				PDOKAN_FILE_INFO FileInfo)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanOpenRequestorToken, HANDLE, DOKANAPI,
				PDOKAN_FILE_INFO DokanFileInfo)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanRemoveMountPoint, BOOL, DOKANAPI,
				LPCWSTR MountPoint)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanMapKernelToUserCreateFileFlags, void, DOKANAPI,
				ACCESS_MASK DesiredAccess,
				ULONG FileAttributes,
				ULONG CreateOptions,
				ULONG CreateDisposition,
				ACCESS_MASK* outDesiredAccess,
				DWORD *outFileAttributesAndFlags,
				DWORD *outCreationDisposition)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanGetMountPointList, BOOL, DOKANAPI,
				PDOKAN_CONTROL list,
				ULONG length,
				BOOL uncOnly,
				PULONG nbRead)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanNtStatusFromWin32, NTSTATUS, DOKANAPI,
				DWORD Error)

		}
	}

	using namespace priv::dokany;

	sl_bool Dokany::initialize(const StringParam& pathDll)
	{
		if (g_libDll) {
			return sl_true;
		}
		g_libDll = DynamicLibrary::loadLibrary(pathDll);
		return g_libDll != sl_null;
	}

	sl_bool Dokany::initialize()
	{
		return initialize("dokan1.dll");
	}

	ServiceState Dokany::getDriverState()
	{
		return ServiceManager::getState(DOKAN_DRIVER_SERVICE);
	}

	sl_bool Dokany::startDriver()
	{
		ServiceState state = getDriverState();
		if (state == ServiceState::None) {
			return sl_false;
		}
		if (state == ServiceState::Running) {
			return sl_true;
		}
		return ServiceManager::start(DOKAN_DRIVER_SERVICE);
	}

	sl_bool Dokany::stopDriver()
	{
		ServiceState state = getDriverState();
		if (state == ServiceState::None) {
			return sl_false;
		}
		if (state == ServiceState::Stopped) {
			return sl_true;
		}
		return ServiceManager::stop(DOKAN_DRIVER_SERVICE);
	}

	sl_bool Dokany::registerDriver(const StringParam& pathSys)
	{
		ServiceCreateParam param;
		param.type = ServiceType::FileSystem;
		param.name = DOKAN_DRIVER_SERVICE;
		if (pathSys.isNotEmpty()) {
			param.path = pathSys;
		} else {
			param.path = Windows::getSystemDirectory() + "\\drivers\\dokan1.sys";
		}
		return ServiceManager::create(param);
	}

	sl_bool Dokany::registerDriver()
	{
		return registerDriver(sl_null);
	}

	sl_bool Dokany::registerAndStartDriver(const StringParam& pathSys)
	{
		ServiceState state = getDriverState();
		if (state == ServiceState::Running) {
			return sl_true;
		}
		if (state != ServiceState::None) {
			return startDriver();
		}
		if (registerDriver(pathSys)) {
			return startDriver();
		}
		return sl_false;
	}

	sl_bool Dokany::registerAndStartDriver()
	{
		return registerAndStartDriver(sl_null);
	}

	sl_bool Dokany::unregisterDriver()
	{
		ServiceState state = getDriverState();
		if (state == ServiceState::None) {
			return sl_true;
		}
		if (state != ServiceState::Stopped) {
			if (!(stopDriver())) {
				return sl_false;
			}
		}
		return ServiceManager::remove(DOKAN_DRIVER_SERVICE);
	}

	sl_bool Dokany::unmount(const StringParam& _mountPoint)
	{
		auto func = getApi_DokanRemoveMountPoint();
		if (func) {
			StringCstr16 mountPoint(_mountPoint);
			return func((LPCWSTR)(mountPoint.getData()));
		}
		return sl_false;
	}

}

#endif
