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
#include "slib/platform/win32/scoped_handle.h"

#include <winioctl.h>

namespace slib
{

	namespace
	{

		enum STORAGE_PROTOCOL_TYPE {
			ProtocolTypeUnknown = 0,
			ProtocolTypeScsi,
			ProtocolTypeAta,
			ProtocolTypeNvme,
			ProtocolTypeSd
		};

		struct STORAGE_PROTOCOL_SPECIFIC_DATA
		{
			STORAGE_PROTOCOL_TYPE ProtocolType;
			ULONG DataType;
			ULONG ProtocolDataRequestValue;
			ULONG ProtocolDataRequestSubValue;
			ULONG ProtocolDataOffset;
			ULONG ProtocolDataLength;
			ULONG FixedProtocolReturnData;
			ULONG Reserved[3];
		};

		struct STORAGE_PROTOCOL_SPECIFIC_QUERY_WITH_BUFFER
		{
			struct { // STORAGE_PROPERTY_QUERY without AdditionalsParameters[1]
				STORAGE_PROPERTY_ID PropertyId;
				STORAGE_QUERY_TYPE QueryType;
			} PropertyQuery;
			STORAGE_PROTOCOL_SPECIFIC_DATA ProtocolSpecific;
			BYTE DataBuffer[4096];
		};

		static String GetSerialNumberFromNVMe(HANDLE hDevice)
		{
			STORAGE_PROTOCOL_SPECIFIC_QUERY_WITH_BUFFER query;
			Base::zeroMemory(&query, sizeof(query));
			query.PropertyQuery.PropertyId = (STORAGE_PROPERTY_ID)50; // StorageDeviceProtocolSpecificProperty
			query.PropertyQuery.QueryType = PropertyStandardQuery;
			query.ProtocolSpecific.ProtocolType = ProtocolTypeNvme;
			query.ProtocolSpecific.DataType = 1; // NVMeDataTypeIdentify
			query.ProtocolSpecific.ProtocolDataRequestValue = 1; // cdw10
			query.ProtocolSpecific.ProtocolDataRequestSubValue = 0; // nsid
			query.ProtocolSpecific.ProtocolDataOffset = sizeof(query.ProtocolSpecific);
			query.ProtocolSpecific.ProtocolDataLength = sizeof(query.DataBuffer);

			DWORD dwBytes = 0;
			if (DeviceIoControl(
				hDevice,
				IOCTL_STORAGE_QUERY_PROPERTY,
				&query, sizeof(query),
				&query, sizeof(query),
				&dwBytes, NULL)
				) {
				char* sn = (char*)(query.DataBuffer) + 4;
				return Disk::normalizeSerialNumber(StringView(sn, Base::getStringLength(sn, 20)));
			}
			return sl_null;
		}

		static String GetSerialNumberByStorageQuery(HANDLE hDevice)
		{
			STORAGE_PROPERTY_QUERY query;
			Base::zeroMemory(&query, sizeof(query));
			query.PropertyId = StorageDeviceProperty;
			query.QueryType = PropertyStandardQuery;

			STORAGE_DESCRIPTOR_HEADER header = { 0 };

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
						if (descriptor->BusType == 0x11) {
							// NVMe
							String ret = GetSerialNumberFromNVMe(hDevice);
							if (ret.isNotNull()) {
								return ret;
							}
						}
						if (descriptor->SerialNumberOffset) {
							char* sn = (char*)(output + descriptor->SerialNumberOffset);
							sl_size n = nOutput - (sl_size)(descriptor->SerialNumberOffset);
							return Disk::normalizeSerialNumber(StringView(sn, Base::getStringLength(sn, n)));
						}
					}
				}
			}
			return sl_null;
		}

		static DiskInterface GetInterfaceType(const String& type)
		{
			if (type.equals_IgnoreCase(StringView::literal("IDE"))) {
				return DiskInterface::IDE;
			} else if (type.equals_IgnoreCase(StringView::literal("USB"))) {
				return DiskInterface::USB;
			} else if (type.equals_IgnoreCase(StringView::literal("SCSI"))) {
				return DiskInterface::SCSI;
			} else if (type.equals_IgnoreCase(StringView::literal("HDC"))) {
				return DiskInterface::HDC;
			} else if (type.equals_IgnoreCase(StringView::literal("1394"))) {
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

	String Disk::getSerialNumber(sl_uint32 diskNo)
	{
		SLIB_STATIC_STRING16(pathTemplate, "\\\\.\\PhysicalDrive")
		String16 path = pathTemplate + String16::fromUint32(diskNo);
		ScopedHandle hDevice = CreateFileW((LPCWSTR)(path.getData()), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (hDevice.isNone()) {
			return sl_null;
		}
		return GetSerialNumberByStorageQuery(hDevice.handle);
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
			disk.serialNumber = Disk::getSerialNumber(disk.index);
			if (disk.serialNumber.isNull()) {
				disk.serialNumber = Disk::normalizeSerialNumber(item.getValue("SerialNumber").getString());
			}
			disk.capacity = item.getValue("Size").getUint64();
			ret.add_NoLock(Move(disk));
		}
		return ret;
	}

}

#endif