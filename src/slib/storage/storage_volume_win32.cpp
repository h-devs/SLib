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

#include "slib/system/service_manager.h"
#include "slib/core/safe_static.h"
#include "slib/core/scoped_buffer.h"
#include "slib/platform.h"
#include "slib/platform/win32/message_loop.h"

#include <dbt.h>
#include <winioctl.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>

#pragma comment(lib, "setupapi.lib")

namespace slib
{

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

	String Storage::getVolumePath(const StringParam& _name)
	{
		StringCstr16 name(_name);
		WCHAR path[MAX_PATH];
		DWORD nWritten = 0;
		if (GetVolumePathNamesForVolumeNameW((LPCWSTR)(name.getData()), path, (DWORD)(CountOfArray(path)), &nWritten)) {
			return String::from(path);
		}
		return sl_null;
	}

	sl_bool Storage::getVolumeDescription(const StringParam& path, StorageVolumeDescription& _out)
	{
		HANDLE hDevice = Win32::createDeviceHandle(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE);
		if (hDevice == INVALID_HANDLE_VALUE) {
			return sl_false;
		}

		STORAGE_PROPERTY_QUERY query;
		Base::zeroMemory(&query, sizeof(query));
		query.PropertyId = StorageDeviceProperty;
		query.QueryType = PropertyStandardQuery;

		STORAGE_DEVICE_DESCRIPTOR desc;
		Base::zeroMemory(&desc, sizeof(desc));
		desc.Size = sizeof(desc);
		DWORD dwWritten = 0;
		sl_bool bRet = sl_false;
		if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &desc, sizeof(desc), &dwWritten, NULL)) {
			bRet = sl_true;
			_out.flagRemovable = desc.RemovableMedia;
			_out.busType = (StorageBusType)(desc.BusType);
		}
		CloseHandle(hDevice);
		return bRet;
	}

	sl_bool Storage::isRemovableVolume(const StringParam& path)
	{
		StorageVolumeDescription desc;
		if (getVolumeDescription(path, desc)) {
			return desc.flagRemovable;
		}
		return sl_false;
	}

	sl_bool Storage::isUsbVolume(const StringParam& path)
	{
		StorageVolumeDescription desc;
		if (getVolumeDescription(path, desc)) {
			return desc.busType == StorageBusType::Usb;
		}
		return sl_false;
	}

	sl_bool Storage::isCdromVolume(const StringParam& path)
	{
		HANDLE hDevice = Win32::createDeviceHandle(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE);
		if (hDevice == INVALID_HANDLE_VALUE) {
			return sl_false;
		}

		sl_bool bRet = sl_false;

		STORAGE_DEVICE_NUMBER sdn;
		DWORD dwWritten = 0;
		if (DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &dwWritten, NULL)) {
			bRet = sdn.DeviceType == FILE_DEVICE_CD_ROM;
		}
		CloseHandle(hDevice);

		return bRet;
	}

	sl_bool Storage::removeDevice(const StringParam& volumePath)
	{
		HANDLE hDevice = Win32::createDeviceHandle(volumePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE);
		if (hDevice == INVALID_HANDLE_VALUE) {
			return sl_false;
		}

		STORAGE_DEVICE_NUMBER sdn;
		{
			DWORD dwWritten = 0;
			if (!(DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &dwWritten, NULL))) {
				CloseHandle(hDevice);
				return sl_false;
			}
		}
		CloseHandle(hDevice);

		const GUID* guid;
		if (sdn.DeviceType == FILE_DEVICE_CD_ROM) {
			guid = &GUID_DEVINTERFACE_CDROM;
		} else {
			guid = &GUID_DEVINTERFACE_DISK;
		}

		HDEVINFO hDevInfo = SetupDiGetClassDevsW(guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (hDevInfo == INVALID_HANDLE_VALUE) {
			return sl_false;
		}

		SP_DEVICE_INTERFACE_DATA interfaceData;
		Base::zeroMemory(&interfaceData, sizeof(interfaceData));
		interfaceData.cbSize = sizeof(interfaceData);

		BOOL bRet = sl_false;
		DWORD index = 0;
		while (SetupDiEnumDeviceInterfaces(hDevInfo, NULL, guid, index, &interfaceData)) {
			DWORD dwSizeDetail = 0;
			SetupDiGetDeviceInterfaceDetailW(hDevInfo, &interfaceData, NULL, 0, &dwSizeDetail, NULL);
			if (dwSizeDetail) {
				SLIB_SCOPED_BUFFER(sl_uint8, 1024, _detail, dwSizeDetail)
				if (!_detail) {
					break;
				}
				SP_DEVICE_INTERFACE_DETAIL_DATA* detail = (SP_DEVICE_INTERFACE_DETAIL_DATA*)_detail;
				Base::zeroMemory(detail, dwSizeDetail);
				detail->cbSize = sizeof(*detail);
				SP_DEVINFO_DATA infoData;
				Base::zeroMemory(&infoData, sizeof(infoData));
				infoData.cbSize = sizeof(infoData);
				if (SetupDiGetDeviceInterfaceDetailW(hDevInfo, &interfaceData, detail, dwSizeDetail, &dwSizeDetail, &infoData)) {
					hDevice = CreateFileW(detail->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
					if (hDevice != INVALID_HANDLE_VALUE) {
						STORAGE_DEVICE_NUMBER sdnOther;
						DWORD dwBytesReturned = 0;
						DWORD dwWritten = 0;
						if (DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdnOther, sizeof(sdnOther), &dwWritten, NULL)) {
							CloseHandle(hDevice);
							if (sdn.DeviceNumber == sdnOther.DeviceNumber) {
								DEVINST hDevInstParent = 0;
								// get drives's parent, e.g. the USB bridge, the SATA port, an IDE channel with two drives!
								if (CM_Get_Parent(&hDevInstParent, infoData.DevInst, 0) == CR_SUCCESS) {
									PNP_VETO_TYPE vetoType = PNP_VetoTypeUnknown;
									WCHAR vetoName[MAX_PATH];
									vetoName[0] = 0;
									if (CM_Request_Device_EjectW(hDevInstParent, &vetoType, vetoName, MAX_PATH, 0) == CR_SUCCESS) {
										bRet = vetoType == PNP_VetoTypeUnknown;
									}
								}
								break;
							}
						} else {
							CloseHandle(hDevice);
						}
					}
				}
			}
			index++;
		}
		SetupDiDestroyDeviceInfoList(hDevInfo);

		return bRet;
	}

	sl_bool Storage::getVolumnSize(const StringParam& _path, sl_uint64* pTotalSize, sl_uint64* pFreeSize)
	{
		StringCstr16 path(_path);
		if (GetDiskFreeSpaceExW((LPCWSTR)(path.getData()), NULL, (ULARGE_INTEGER*)pTotalSize, (ULARGE_INTEGER*)pFreeSize)) {
			return sl_true;
		}
		return sl_false;
	}

	namespace
	{
		static sl_bool SetUsbMassStorageEnabled(sl_bool flag)
		{
			ServiceStartType type = flag ? ServiceStartType::Manual : ServiceStartType::Disabled;
			if (!(ServiceManager::setStartType(L"usbstor", type))) {
				return sl_false;
			}
			return ServiceManager::setStartType(L"winusb", type);
		}

		static sl_bool IsUsbMassStorageEnabled()
		{
			ServiceStartType type = ServiceManager::getStartType(L"usbstor");
			if (type == ServiceStartType::Disabled || type == ServiceStartType::Unknown) {
				return sl_false;
			}
			type = ServiceManager::getStartType(L"winusb");
			if (type == ServiceStartType::Disabled || type == ServiceStartType::Unknown) {
				return sl_false;
			}
			return sl_true;
		}
	}

	sl_bool Storage::disableUsbMassStorage()
	{
		return SetUsbMassStorageEnabled(sl_false);
	}

	sl_bool Storage::enableUsbMassStorage()
	{
		return SetUsbMassStorageEnabled(sl_true);
	}

	sl_bool Storage::isEnabledUsbMassStorage()
	{
		return IsUsbMassStorageEnabled();
	}

	namespace {

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
			Ref<win32::MessageLoop> m_loop;
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
						win32::MessageLoopParam param;
						param.name = SLIB_UNICODE("DeviceChangeMonitor");
						param.onMessage = SLIB_FUNCTION_MEMBER(this, onMessage);
						param.hWndParent = NULL;
						m_loop = win32::MessageLoop::create(param);
					}
				}
			}

		};

		SLIB_SAFE_STATIC_GETTER(DeviceChangeMonitor, GetMonitor)

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