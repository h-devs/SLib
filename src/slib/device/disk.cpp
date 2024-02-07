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

#include "slib/device/disk.h"

#include "slib/core/scoped_buffer.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(DiskInfo)

	DiskInfo::DiskInfo()
	{
		index = 0;
		capacity = 0;
	}


#if !defined(SLIB_PLATFORM_IS_WIN32) && !defined(SLIB_PLATFORM_IS_LINUX) && !defined(SLIB_PLATFORM_IS_MACOS)
	String Device::getSerialNumber(sl_uint32 diskNo)
	{
		return sl_null;
	}

	List<DiskInfo> Disk::getDevices()
	{
		return sl_null;
	}
#endif

	String Disk::normalizeSerialNumber(const StringParam& _sn)
	{
		if (_sn.isNull()) {
			return sl_null;
		}
		StringData data(_sn);
		const sl_char8* sn = data.getData();
		sl_size n = data.getLength();
		if (!n) {
			return String::getEmpty();
		}
		sl_bool flagHex = sl_true;
		if (n & 1) {
			flagHex = sl_false;
		} else {
			sl_size m = n >> 1;
			for (sl_size i = 0; i < n; i += 2) {
				sl_char8 c1 = sn[i];
				sl_char8 c2 = sn[i + 1];
				if (SLIB_CHAR_IS_HEX(c1) && SLIB_CHAR_IS_HEX(c2)) {
					char c = (SLIB_CHAR_HEX_TO_INT(c1) << 4) | SLIB_CHAR_HEX_TO_INT(c2);
					if (!SLIB_CHAR_IS_PRINTABLE_ASCII(c)) {
						flagHex = sl_false;
						break;
					}
				} else {
					flagHex = sl_false;
					break;
				}
			}
		}
		if (flagHex) {
			sl_size m = n >> 1;
			SLIB_SCOPED_BUFFER(sl_char8, 1024, h, m)
			for (sl_size i = 0; i < n; i += 2) {
				sl_char8 c1 = sn[i];
				sl_char8 c2 = sn[i + 1];
				h[i >> 1] = (SLIB_CHAR_HEX_TO_INT(c1) << 4) | SLIB_CHAR_HEX_TO_INT(c2);
			}
			return StringView(h, m).trim();
		}
		sl_bool flagNoPrintable = sl_false;
		sl_size iFirst = 0, iLast = 0;
		{
			for (sl_size i = 0; i < n; i++) {
				sl_char8 c = sn[i];
				if (!SLIB_CHAR_IS_WHITE_SPACE(c)) {
					if (!iLast) {
						iFirst = i;
					}
					iLast = i + 1;
					if (!SLIB_CHAR_IS_PRINTABLE_ASCII(c)) {
						flagNoPrintable = sl_true;
					}
				}
			}
		}
		if (flagNoPrintable) {
			sn += iFirst;
			n = iLast - iFirst;
			if (!n) {
				return String::getEmpty();
			}
			SLIB_SCOPED_BUFFER(sl_char8, 1024, s, n)
			sl_size k = 0;
			for (sl_size i = 0; i < n; i++) {
				sl_char8 c = sn[i];
				if (SLIB_CHAR_IS_PRINTABLE_ASCII(c)) {
					s[k++] = c;
				}
			}
			return String(s, k);
		} else {
			if (!iFirst && iLast == n) {
				return _sn.toString();
			} else {
				return String(sn + iFirst, iLast - iFirst);
			}
		}
	}

	String ToString(DiskInterface interface)
	{
		switch (interface) {
			case DiskInterface::IDE:
				SLIB_RETURN_STRING("IDE")
			case DiskInterface::USB:
				SLIB_RETURN_STRING("USB")
			case DiskInterface::SCSI:
				SLIB_RETURN_STRING("SCSI")
			case DiskInterface::HDC:
				SLIB_RETURN_STRING("HDC")
			case DiskInterface::IEEE1394:
				SLIB_RETURN_STRING("1394")
			default:
				break;
		}
		return sl_null;
	}

	String ToString(DiskType type)
	{
		switch (type) {
			case DiskType::Fixed:
				SLIB_RETURN_STRING("Fixed")
			case DiskType::External:
				SLIB_RETURN_STRING("External")
			case DiskType::Removable:
				SLIB_RETURN_STRING("Removable")
			default:
				break;
		}
		return sl_null;
	}

}
