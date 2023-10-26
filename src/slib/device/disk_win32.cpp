/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/device/disk.h"

#include "slib/core/scoped_buffer.h"
#include "slib/platform.h"
#include "slib/platform/win32/wmi.h"

#include <winioctl.h>

namespace slib
{

	String Disk::getSerialNumber(sl_uint32 diskNo)
	{
		SLIB_STATIC_STRING16(pathTemplate, "\\\\.\\PhysicalDrive")
		String16 path = pathTemplate + String16::fromUint32(diskNo);

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
			if (output) {
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
						ret = String(sn, Base::getStringLength(sn, n)).trim();
					}
				}
			}
		}

		CloseHandle(hDevice);

		return ret;
	}

	namespace
	{
		static DiskInterface GetInterfaceType(const String& type)
		{
			if (type.equals_IgnoreCase(StringView::literal("IDE"))) {
				return DiskInterface::IDE;
			} if (type.equals_IgnoreCase(StringView::literal("USB"))) {
				return DiskInterface::USB;
			} if (type.equals_IgnoreCase(StringView::literal("SCSI"))) {
				return DiskInterface::SCSI;
			} if (type.equals_IgnoreCase(StringView::literal("HDC"))) {
				return DiskInterface::HDC;
			} if (type.equals_IgnoreCase(StringView::literal("1394"))) {
				return DiskInterface::IEEE1394;
			}
			return DiskInterface::Unknown;
		}

		static DiskType GetMediaType(const String& type)
		{
			if (type.startsWith_IgnoreCase(StringView::literal("Fixed"))) {
				return DiskType::Fixed;
			} else if (type.startsWith_IgnoreCase(StringView::literal("External"))) {
				return DiskType::External;
			} else if (type.startsWith_IgnoreCase(StringView::literal("Removable"))) {
				return DiskType::Removable;
			}
			return DiskType::Unknown;
		}
	}

	List<DiskInfo> Disk::getDevices()
	{
		List<DiskInfo> ret;
		ListElements<VariantMap> items(win32::Wmi::getQueryResponseRecords(L"SELECT * FROM Win32_DiskDrive", L"DeviceID", L"Index", L"InterfaceType", L"Size", L"Model", L"MediaType", L"SerialNumber"));
		for (sl_size i = 0; i < items.count; i++) {
			DiskInfo disk;
			VariantMap& item = items[i];
			disk.index = item.getValue("Index").getUint32();
			disk.path = item.getValue("DeviceID").getString();
			disk.interface = GetInterfaceType(item.getValue("InterfaceType").getString());
			disk.type = GetMediaType(item.getValue("MediaType").getString());
			disk.model = item.getValue("Model").getString();
			disk.serialNumber = item.getValue("SerialNumber").getString().trim();
			disk.capacity = item.getValue("Size").getUint64();
			ret.add_NoLock(Move(disk));
		}
		return ret;
	}

}

#endif