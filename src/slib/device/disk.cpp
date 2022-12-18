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

namespace slib
{

#if !defined(SLIB_PLATFORM_IS_WIN32)
	String Disk::getSerialNumber(sl_uint32 diskNo)
	{
		return sl_null;
	}

	sl_bool Disk::getSize(const StringParam& path, sl_uint64* pTotalSize, sl_uint64* pFreeSize)
	{
		return sl_false;
	}
#endif

	sl_uint64 Disk::getTotalSize(const StringParam& path)
	{
		sl_uint64 size;
		if (getSize(path, &size)) {
			return size;
		}
		return 0;
	}

	sl_uint64 Disk::getFreeSize(const StringParam& path)
	{
		sl_uint64 size;
		if (getSize(path, sl_null, &size)) {
			return size;
		}
		return 0;
	}

}
