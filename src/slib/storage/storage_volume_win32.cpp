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

#include "slib/device/definition.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "slib/storage/storage.h"

#include "slib/core/win32_message_loop.h"
#include "slib/core/platform_windows.h"
#include "slib/core/safe_static.h"

#include <dbt.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>

#pragma comment(lib, "setupapi.lib")


namespace slib
{

	namespace priv
	{
		namespace storage
		{

			static char GetFirstDriveFromMask(ULONG mask)
			{
				char i;
				for (i = 0; i < 26; ++i) {
					if (mask & 1) {
						break;
					}
					mask >>= 1;
				}
				return i + 'A';
			}

			class DeviceChangeMonitor
			{
			public:
				Mutex m_lock;
				Ref<Win32MessageLoop> m_loop;
				Atomic<VolumeArrivalCallback> m_callbackArrival;
				Atomic<VolumeRemovalCallback> m_callbackRemoval;

			public:
				DeviceChangeMonitor()
				{
				}

				~DeviceChangeMonitor()
				{
				}

			public:
				sl_bool onMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
				{
					if (uMsg == WM_DEVICECHANGE) {
						if (wParam == DBT_DEVICEARRIVAL) {
							DEV_BROADCAST_HDR* hdr = (DEV_BROADCAST_HDR*)lParam;
							if (hdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
								DEV_BROADCAST_VOLUME* vol = (DEV_BROADCAST_VOLUME*)lParam;
								String path;
								if (vol->dbcv_unitmask) {
									char _path[] = "C:\\";
									_path[0] = GetFirstDriveFromMask(vol->dbcv_unitmask);
									path = _path;
								}
								m_callbackArrival(path);
							}
						} else if (wParam == DBT_DEVICEREMOVECOMPLETE) {
							DEV_BROADCAST_HDR* hdr = (DEV_BROADCAST_HDR*)lParam;
							if (hdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
								DEV_BROADCAST_VOLUME* vol = (DEV_BROADCAST_VOLUME*)lParam;
								String path;
								if (vol->dbcv_unitmask) {
									char _path[] = "C:\\";
									_path[0] = GetFirstDriveFromMask(vol->dbcv_unitmask);
									path = _path;
								}
								m_callbackRemoval(path);
							}
						}
						return sl_true;
					}
					return sl_false;
				}

				void updateCallback()
				{
					MutexLocker locker(&m_lock);
					if (m_callbackArrival.isNull() && m_callbackRemoval.isNull()) {
						m_loop.setNull();
					} else {
						if (m_loop.isNull()) {
							Win32MessageLoopParam param;
							param.name = SLIB_UNICODE("SLibDeviceChangeMonitor");
							param.onMessage = SLIB_FUNCTION_MEMBER(DeviceChangeMonitor, onMessage, this);
							param.hWndParent = NULL;
							m_loop = Win32MessageLoop::create(param);
						}
					}
				}

			};

			SLIB_SAFE_STATIC_GETTER(DeviceChangeMonitor, GetMonitor)

		}
	}

	using namespace priv::storage;

	List<String> Storage::getAllVolumes()
	{
		WCHAR volumeName[MAX_PATH];
		HANDLE hFind = FindFirstVolumeW(volumeName, (DWORD)(CountOfArray(volumeName)));
		if (hFind == INVALID_HANDLE_VALUE) {
			return sl_null;
		}
		List<String> ret;
		for (;;) {
			ret.add_NoLock(String::from(volumeName));
			if (!(FindNextVolumeW(hFind, volumeName, (DWORD)(CountOfArray(volumeName))))) {
				break;
			}
		}
		FindVolumeClose(hFind);
		return ret;
	}

	List<String> Storage::getRemovableVolumes()
	{
		HDEVINFO hDevInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISKDRIVE, NULL, NULL, DIGCF_PRESENT);
		if (hDevInfo == INVALID_HANDLE_VALUE) {
			return sl_null;
		}

		SP_DEVINFO_DATA devInfo;
		Base::zeroMemory(&devInfo, sizeof(devInfo));
		devInfo.cbSize = sizeof(devInfo);

		List<String> ret;
		DWORD index = 0;

		while (SetupDiEnumDeviceInfo(hDevInfo, index, &devInfo)) {
			DWORD type, value, size;
			if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfo, SPDRP_REMOVAL_POLICY, &type, (PBYTE)&value, sizeof(value), &size)) {
				if (value == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL || value == CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL) {
					ret.add_NoLock("\\\\?\\Volume" + Windows::getStringFromGUID(devInfo.ClassGuid) + "\\");
				}
			}
			index++;
		}
		SetupDiDestroyDeviceInfoList(hDevInfo);

		return ret;

	}

	void Storage::addOnVolumeArrival(const VolumeArrivalCallback& callback)
	{
		DeviceChangeMonitor* monitor = GetMonitor();
		if (monitor) {
			monitor->m_callbackArrival.addIfNotExist(callback);
			monitor->updateCallback();
		}
	}

	void Storage::removeOnVolumeArrival(const VolumeArrivalCallback& callback)
	{
		DeviceChangeMonitor* monitor = GetMonitor();
		if (monitor) {
			monitor->m_callbackArrival.remove(callback);
			monitor->updateCallback();
		}
	}

	void Storage::setOnVolumeArrival(const VolumeArrivalCallback& callback)
	{
		DeviceChangeMonitor* monitor = GetMonitor();
		if (monitor) {
			monitor->m_callbackArrival = callback;
			monitor->updateCallback();
		}
	}

	void Storage::addOnVolumeRemoval(const VolumeRemovalCallback& callback)
	{
		DeviceChangeMonitor* monitor = GetMonitor();
		if (monitor) {
			monitor->m_callbackRemoval.addIfNotExist(callback);
			monitor->updateCallback();
		}
	}

	void Storage::removeOnVolumeRemoval(const VolumeRemovalCallback& callback)
	{
		DeviceChangeMonitor* monitor = GetMonitor();
		if (monitor) {
			monitor->m_callbackRemoval.remove(callback);
			monitor->updateCallback();
		}
	}

	void Storage::setOnVolumeRemoval(const VolumeRemovalCallback& callback)
	{
		DeviceChangeMonitor* monitor = GetMonitor();
		if (monitor) {
			monitor->m_callbackRemoval = callback;
			monitor->updateCallback();
		}
	}

}

#endif