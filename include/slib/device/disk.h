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

#ifndef CHECKHEADER_SLIB_DEVICE_DISK
#define CHECKHEADER_SLIB_DEVICE_DISK

#include "definition.h"

#include "../core/string.h"

namespace slib
{

	enum class DiskInterface
	{
		Unknown = 0,
		SCSI = 1,
		HDC = 2,
		IDE = 3,
		USB = 4,
		IEEE1394 = 5
	};

	String ToString(DiskInterface);

	enum class DiskType
	{
		Unknown = 0,
		Fixed = 1,
		External = 2,
		Removable = 3
	};

	String ToString(DiskType);


	class SLIB_EXPORT DiskInfo
	{
	public:
		sl_uint32 index;
		String path;
		DiskInterface interface;
		DiskType type;
		String model;
		String serialNumber;
		sl_uint64 capacity;

	public:
		DiskInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DiskInfo)

	};

	class SLIB_EXPORT Disk
	{
	public:
		static String getSerialNumber(sl_uint32 diskIndex);

		static List<DiskInfo> getDevices();

		static String normalizeSerialNumber(const StringParam& sn);

	};

}

#endif
