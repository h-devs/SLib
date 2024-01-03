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

//#define USE_SMART

namespace slib
{

	namespace
	{
		static String GetSerialNumberByStorageQuery(HANDLE hDevice)
		{
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
							return Disk::normalizeSerialNumber(StringView(sn, Base::getStringLength(sn, n)));
						}
					}
				}
			}
			return sl_null;
		}

#ifdef USE_SMART
		static sl_bool IsSmartSupported(HANDLE hDevice)
		{
			GETVERSIONINPARAMS params = { 0 };
			DWORD dwBytes = 0;
			if (DeviceIoControl(hDevice, SMART_GET_VERSION, NULL, 0, &params, sizeof(params), &dwBytes, NULL)) {
				return (params.fCapabilities & CAP_SMART_CMD) != 0;
			}
			return sl_false;
		}

		static String GetSerialNumberBySmart(HANDLE hDevice, sl_uint32 diskNumber)
		{
			sl_uint8 bufSci[sizeof(SENDCMDINPARAMS) + IDENTIFY_BUFFER_SIZE] = { 0 };
			SENDCMDINPARAMS& sci = *((SENDCMDINPARAMS*)bufSci);
			sl_uint8 bufSco[sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE] = { 0 };
			SENDCMDOUTPARAMS& sco = *((SENDCMDOUTPARAMS*)bufSco);
			sci.irDriveRegs.bCommandReg = ID_CMD;
			sci.bDriveNumber = (BYTE)diskNumber;
			sci.cBufferSize = IDENTIFY_BUFFER_SIZE;
			DWORD dwBytes = 0;
			if (DeviceIoControl(hDevice, SMART_RCV_DRIVE_DATA, &sci, sizeof(bufSci), &sco, sizeof(bufSco), &dwBytes, NULL)) {
				char sn[20];
				for (sl_size i = 0; i < 20; i += 2) {
					sn[i] = sco.bBuffer[i + 21];
					sn[i + 1] = sco.bBuffer[i + 20];
				}
				return Disk::normalizeSerialNumber(StringView(sn, 20));
			}
			return sl_null;
		}
#endif
	}

	String Disk::getSerialNumber(sl_uint32 diskNo)
	{
		SLIB_STATIC_STRING16(pathTemplate, "\\\\.\\PhysicalDrive")
		String16 path = pathTemplate + String16::fromUint32(diskNo);
#ifdef USE_SMART
		ScopedHandle hDevice = CreateFileW((LPCWSTR)(path.getData()), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (hDevice.isNone()) {
			hDevice = CreateFileW((LPCWSTR)(path.getData()), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
			if (hDevice.isNone()) {
				return sl_null;
			}
		}
		if (IsSmartSupported(hDevice.handle)) {
			String ret = GetSerialNumberBySmart(hDevice.handle, diskNo);
			if (ret.isNotNull()) {
				return ret;
			}
		}
#else
		ScopedHandle hDevice = CreateFileW((LPCWSTR)(path.getData()), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (hDevice.isNone()) {
			return sl_null;
		}
#endif
		return GetSerialNumberByStorageQuery(hDevice.handle);
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
			disk.serialNumber = Disk::normalizeSerialNumber(item.getValue("SerialNumber").getString());
			disk.capacity = item.getValue("Size").getUint64();
			ret.add_NoLock(Move(disk));
		}
		return ret;
	}

}

#endif