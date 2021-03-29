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

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/core/platform_windows.h"

#include <setupapi.h>
#include <cfgmgr32.h>

#pragma comment(lib, "setupapi.lib")

namespace slib
{

	namespace priv
	{
		namespace platform_win32
		{

			typedef BOOL (WINAPI *TYPE_UpdateDriverForPlugAndPlayDevices)(
				HWND hwndParent,
				LPCWSTR hardwareId,
				LPCWSTR fullInfPath,
				DWORD installFlags,
				PBOOL bRebootRequired
				);

		}
	}

	using namespace priv::platform_win32;

	sl_bool Windows::installDriver(const StringParam& _pathToInf, const StringParam& _hardwareId, sl_bool* pOutRebootRequired)
	{
		if (pOutRebootRequired) {
			*pOutRebootRequired = sl_false;
		}

		StringCstr16 pathToInf(_pathToInf);
		StringCstr16 hardwareId(_hardwareId);
		if (hardwareId.isEmpty()) {
			return sl_false;
		}

		WCHAR path[1024] = { 0 };
		if (!(GetFullPathNameW((LPCWSTR)(pathToInf.getData()), (DWORD)(CountOfArray(path) - 1), path, NULL))) {
			return sl_false;
		}
		GUID classGuid;
		WCHAR className[256] = { 0 };
		if (!(SetupDiGetINFClassW(path, &classGuid, className, (DWORD)(CountOfArray(path) - 1), NULL))) {
			return sl_false;
		}
		BOOL flagCreated = sl_false;
		HDEVINFO hDevInfoSet = SetupDiCreateDeviceInfoList(&classGuid, NULL);
		if (hDevInfoSet != INVALID_HANDLE_VALUE) {
			SP_DEVINFO_DATA devInfoData;
			Base::zeroMemory(&devInfoData, sizeof(devInfoData));
			devInfoData.cbSize = sizeof(devInfoData);
			if (SetupDiCreateDeviceInfoW(hDevInfoSet, className, &classGuid, NULL, NULL, DICD_GENERATE_ID, &devInfoData)) {
				sl_char16 hwId[LINE_LEN + 4] = { 0 };
				Base::copyString2(hwId, hardwareId.getData(), LINE_LEN);
				if (SetupDiSetDeviceRegistryPropertyW(hDevInfoSet, &devInfoData, SPDRP_HARDWAREID, (LPBYTE)hwId, (DWORD)(hardwareId.getLength() + 2) * 2)) {
					if (SetupDiCallClassInstaller(DIF_REGISTERDEVICE, hDevInfoSet, &devInfoData)) {
						HMODULE hDll = LoadLibraryW(L"newdev.dll");
						if (hDll) {
							auto func = (TYPE_UpdateDriverForPlugAndPlayDevices)(GetProcAddress(hDll, "UpdateDriverForPlugAndPlayDevicesW"));
							if (func) {
								BOOL bRebootRequired = FALSE;
								if (func(NULL, (LPCWSTR)hwId, path, 0, &bRebootRequired)) {
									flagCreated = sl_true;
									if (bRebootRequired && pOutRebootRequired) {
										*pOutRebootRequired = sl_true;
									}
								}
							} else {
								flagCreated = sl_true;
							}
							FreeLibrary(hDll);
						} else {
							flagCreated = sl_true;
						}
					}
				}
			}
			SetupDiDestroyDeviceInfoList(hDevInfoSet);
		}
		return flagCreated;
	}

	sl_bool Windows::uninstallDriver(const StringParam& _hardwareId, sl_bool* pOutRebootRequired)
	{
		if (pOutRebootRequired) {
			*pOutRebootRequired = sl_false;
		}

		StringCstr16 hardwareId(_hardwareId);
		if (hardwareId.isEmpty()) {
			return sl_false;
		}

		sl_bool flagSuccess = sl_false;

		HDEVINFO hDevInfoSet = SetupDiGetClassDevsExW(NULL, NULL, NULL, DIGCF_ALLCLASSES, NULL, NULL, NULL);
		if (hDevInfoSet != INVALID_HANDLE_VALUE) {

			SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
			Base::zeroMemory(&devInfoListDetail, sizeof(devInfoListDetail));
			devInfoListDetail.cbSize = sizeof(devInfoListDetail);

			if (SetupDiGetDeviceInfoListDetailW(hDevInfoSet, &devInfoListDetail)) {

				flagSuccess = sl_true;

				SP_DEVINFO_DATA devInfoData;
				Base::zeroMemory(&devInfoData, sizeof(devInfoData));
				devInfoData.cbSize = sizeof(devInfoData);

				for (sl_uint32 index = 0; ; index++) {

					if (SetupDiEnumDeviceInfo(hDevInfoSet, index, &devInfoData)) {

						WCHAR devID[1024] = { 0 };
						DWORD type = 0;

						SetupDiGetDeviceRegistryPropertyW(hDevInfoSet, &devInfoData, SPDRP_HARDWAREID, &type, (BYTE*)devID, sizeof(devID) - 2, NULL);
						
						if (Base::equalsString2((sl_char16*)devID, hardwareId.getData(), MAX_DEVICE_ID_LEN)) {

							SP_REMOVEDEVICE_PARAMS removeParams;
							Base::zeroMemory(&removeParams, sizeof(removeParams));
							removeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
							removeParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
							removeParams.Scope = DI_REMOVEDEVICE_GLOBAL;
							removeParams.HwProfile = 0;
							if (SetupDiSetClassInstallParamsW(hDevInfoSet, &devInfoData, &(removeParams.ClassInstallHeader), sizeof(removeParams))) {
								if (SetupDiCallClassInstaller(DIF_REMOVE, hDevInfoSet, &devInfoData)) {
									SP_DEVINSTALL_PARAMS devParams;
									Base::zeroMemory(&devParams, sizeof(devParams));
									devParams.cbSize = sizeof(devParams);
									if (SetupDiGetDeviceInstallParamsW(hDevInfoSet, &devInfoData, &devParams)) {
										if (devParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT)) {
											if (pOutRebootRequired) {
												*pOutRebootRequired = sl_true;
											}
										}
									}
								} else {
									flagSuccess = sl_false;
								}
							} else {
								flagSuccess = sl_false;
							}

						}

					} else {
						break;
					}
				}

			}

			SetupDiDestroyDeviceInfoList(hDevInfoSet);
		}

		return flagSuccess;
	}

}

#endif
