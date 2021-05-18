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

#include "slib/storage/definition.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "slib/storage/disk.h"

#include "slib/core/thread.h"
#include "slib/core/scoped.h"
#include "slib/core/safe_static.h"
#include "slib/core/platform_windows.h"
#include "slib/core/win32_message_loop.h"

#include <winioctl.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <dbt.h>

#pragma comment(lib, "setupapi.lib")

namespace slib
{

	namespace priv
	{
		namespace disk
		{

// #define FIX_SERIAL_NUMBER

			static String ProcessSerialNumber(char* sn, sl_size n)
			{
#ifdef FIX_SERIAL_NUMBER
				sl_size i;
				sl_bool flagHex = sl_true;
				for (i = 0; i < n; i++) {
					char c = sn[i];
					if (c) {
						if (!SLIB_CHAR_IS_HEX(c)) {
							flagHex = sl_false;
						}
					} else {
						n = i;
						break;
					}
				}
				if (n) {
					if (flagHex && n % 2 == 0) {
						n >>= 1;
						sl_size k = 0;
						for (i = 0; i < n; i++) {
							sn[i] = (char)((SLIB_CHAR_HEX_TO_INT(sn[k]) << 4) | SLIB_CHAR_HEX_TO_INT(sn[k + 1]));
							k += 2;
						}
						String ret = String::fromUtf8(sn, n).trim();
						if (ret.isNotEmpty()) {
							n = ret.getLength() >> 1;
							char* p = ret.getData();
							for (i = 0; i < n; i++) {
								Swap(p[0], p[1]);
								p += 2;
							}
						}
						return ret;
					} else {
						return String::fromUtf8(sn, n);
					}
				}
				return sl_null;
#else
				return String::fromUtf8(sn, Base::getStringLength(sn, n));
#endif
			}

			static char GetFirstDriveFromMask(ULONG mask)
			{
				char i;
				for (i = 0; i < 26; ++i) {
					if (mask & 0x1) {
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
				AtomicFunction<void(const String&)> m_callbackArrival;
				AtomicFunction<void(const String&)> m_callbackRemoval;

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
								if (vol->dbcv_flags & DBTF_MEDIA) {
									String path;
									if (vol->dbcv_unitmask) {
										char _path[] = "C:\\";
										_path[0] = GetFirstDriveFromMask(vol->dbcv_unitmask);
									}
									m_callbackArrival(path);
								}
							}
						} else if (wParam == DBT_DEVICEREMOVECOMPLETE) {
								DEV_BROADCAST_HDR* hdr = (DEV_BROADCAST_HDR*)lParam;
							if (hdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
								DEV_BROADCAST_VOLUME* vol = (DEV_BROADCAST_VOLUME*)lParam;
								if (vol->dbcv_flags & DBTF_MEDIA) {
									String path;
									if (vol->dbcv_unitmask) {
										char _path[] = "C:\\";
										_path[0] = GetFirstDriveFromMask(vol->dbcv_unitmask);
									}
									m_callbackRemoval(path);
								}
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
							m_loop = Win32MessageLoop::create("SLibDeviceChangeMonitor", SLIB_FUNCTION_MEMBER(DeviceChangeMonitor, onMessage, this));
						}
					}
				}

			};

			DeviceChangeMonitor* GetMonitor()
			{
				SLIB_SAFE_LOCAL_STATIC(DeviceChangeMonitor, ret);
				if (SLIB_SAFE_STATIC_CHECK_FREED(ret)) {
					return sl_null;
				}
				return &ret;
			}

		}
	}

	using namespace priv::disk;

	String Disk::getSerialNumber(sl_uint32 diskNo)
	{
		SLIB_STATIC_STRING16(pathTemplate, "\\\\.\\PhysicalDrive")
		String16 path = pathTemplate + diskNo;

		HANDLE hDevice = CreateFileW((LPCWSTR)(path.getData()), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (hDevice == INVALID_HANDLE_VALUE) {
			return sl_null;
		}
		
		String ret;

		STORAGE_PROPERTY_QUERY query;
		Base::zeroMemory(&query, sizeof(query));
		query.PropertyId = StorageDeviceProperty;
		query.QueryType = PropertyStandardQuery;
			
		STORAGE_DESCRIPTOR_HEADER header;
		Base::zeroMemory(&header, sizeof(header));

		DWORD dwBytes = 0;

		if (DeviceIoControl(
			hDevice,
			IOCTL_STORAGE_QUERY_PROPERTY,
			&query, sizeof(query),
			&header, sizeof(header),
			&dwBytes, NULL)
		) {
			sl_size nOutput = (sl_size)(header.Size);
			SLIB_SCOPED_BUFFER(sl_uint8, 256, output, nOutput)
				Base::zeroMemory(output, nOutput);

			if (DeviceIoControl(
				hDevice,
				IOCTL_STORAGE_QUERY_PROPERTY,
				&query, sizeof(query),
				output, (DWORD)nOutput,
				&dwBytes, NULL)
			) {
				STORAGE_DEVICE_DESCRIPTOR* descriptor = (STORAGE_DEVICE_DESCRIPTOR*)output;
				if (descriptor->SerialNumberOffset) {
					char* sn = (char*)(output + descriptor->SerialNumberOffset);
					sl_size n = nOutput - (sl_size)(descriptor->SerialNumberOffset);
					ret = ProcessSerialNumber(sn, n);
				}
			}
		}

		CloseHandle(hDevice);

		return ret;
	}

	sl_bool Disk::getSize(const StringParam& _path, sl_uint64* pTotalSize, sl_uint64* pFreeSize)
	{
		StringCstr16 path(_path);
		if (GetDiskFreeSpaceExW((LPCWSTR)(path.getData()), NULL, (ULARGE_INTEGER*)pTotalSize, (ULARGE_INTEGER*)pFreeSize)) {
			return sl_true;
		}
		return sl_false;
	}

	List<String> Disk::getVolumes()
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

	List<String> Disk::getRemovableVolumes()
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

	void Disk::addMediaArrivalListener(const Function<void(const String& path)>& callback)
	{
		DeviceChangeMonitor* monitor = GetMonitor();
		if (monitor) {
			monitor->m_callbackArrival.addIfNotExist(callback);
			monitor->updateCallback();
		}
	}

	void Disk::removeMediaArrivalListener(const Function<void(const String& path)>& callback)
	{
		DeviceChangeMonitor* monitor = GetMonitor();
		if (monitor) {
			monitor->m_callbackArrival.remove(callback);
			monitor->updateCallback();
		}
	}

	void Disk::removeAllMediaArrivalListeners()
	{
		DeviceChangeMonitor* monitor = GetMonitor();
		if (monitor) {
			monitor->m_callbackArrival.setNull();
			monitor->updateCallback();
		}
	}

	void Disk::addMediaRemovalListener(const Function<void(const String& path)>& callback)
	{
		DeviceChangeMonitor* monitor = GetMonitor();
		if (monitor) {
			monitor->m_callbackRemoval.addIfNotExist(callback);
			monitor->updateCallback();
		}
	}
	
	void Disk::removeMediaRemovalListener(const Function<void(const String& path)>& callback)
	{
		DeviceChangeMonitor* monitor = GetMonitor();
		if (monitor) {
			monitor->m_callbackRemoval.remove(callback);
			monitor->updateCallback();
		}
	}

	void Disk::removeAllMediaRemovalListeners()
	{
		DeviceChangeMonitor* monitor = GetMonitor();
		if (monitor) {
			monitor->m_callbackRemoval.setNull();
			monitor->updateCallback();
		}
	}

}

#endif