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

#ifndef CHECKHEADER_SLIB_DEVICE_PHYSICAL_MEMORY
#define CHECKHEADER_SLIB_DEVICE_PHYSICAL_MEMORY

#include "definition.h"

#include "../core/string.h"

namespace slib
{

	struct SLIB_EXPORT PhysicalMemoryStatus
	{
		sl_uint64 total; // in bytes
		sl_uint64 available; // in bytes
	};

	class SLIB_EXPORT PhysicalMemorySlotInfo
	{
	public:
		sl_uint64 capacity;
		sl_uint32 speed; // MHz
		String bank;
		String serialNumber;

	public:
		PhysicalMemorySlotInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PhysicalMemorySlotInfo)

	};

	class SLIB_EXPORT PhysicalMemory
	{
	public:
		static sl_bool getStatus(PhysicalMemoryStatus& _out);

		static sl_uint64 getTotalSize();

		static List<PhysicalMemorySlotInfo> getSlots();

	};

}

#endif
