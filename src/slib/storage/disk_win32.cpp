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

#include "slib/storage/disk.h"

#include "slib/core/platform_windows.h"
#include "slib/core/scoped.h"

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

}
