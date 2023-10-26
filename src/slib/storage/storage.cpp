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

#include "slib/storage/storage.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(StorageVolumeDescription)

	StorageVolumeDescription::StorageVolumeDescription()
	{
	}


#if !defined(SLIB_PLATFORM_IS_WIN32)
	sl_bool Storage::getVolumnSize(const StringParam& path, sl_uint64* pTotalSize, sl_uint64* pFreeSize)
	{
		return sl_false;
	}
#endif

	sl_uint64 Storage::getVolumnTotalSize(const StringParam& path)
	{
		sl_uint64 size;
		if (getVolumnSize(path, &size)) {
			return size;
		}
		return 0;
	}

	sl_uint64 Storage::getVolumnFreeSize(const StringParam& path)
	{
		sl_uint64 size;
		if (getVolumnSize(path, sl_null, &size)) {
			return size;
		}
		return 0;
	}

}
