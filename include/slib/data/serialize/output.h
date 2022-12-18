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

#ifndef CHECKHEADER_SLIB_DATA_SERIALIZE_OUTPUT
#define CHECKHEADER_SLIB_DATA_SERIALIZE_OUTPUT

#include "../definition.h"

namespace slib
{

	class Memory;

	class SLIB_EXPORT SerializeOutput
	{
	public:
		sl_uint8* begin;
		sl_size size;
		sl_size offset;

	public:
		SerializeOutput() noexcept: begin(sl_null), size(0), offset(0) {}

		SerializeOutput(const SerializeOutput&) = delete;

		SerializeOutput(SerializeOutput&& other) noexcept
		{
			begin = other.begin;
			size = other.size;
			offset = other.offset;
			other.begin = sl_null;
			other.size = 0;
			other.offset = 0;
		}

		~SerializeOutput();

	public:
		SerializeOutput& operator=(const SerializeOutput&) = delete;

		SerializeOutput& operator=(SerializeOutput&& other) noexcept
		{
			begin = other.begin;
			size = other.size;
			offset = other.offset;
			other.begin = sl_null;
			other.size = 0;
			other.offset = 0;
			return *this;
		}

	public:
		sl_size getWrittenSize() noexcept
		{
			return offset;
		}

		sl_bool write(sl_uint8 value) noexcept;

		sl_size write(const void* buf, sl_size size) noexcept;

		sl_size write(const MemoryView& mem) noexcept;

		void* allocate(sl_size size) noexcept;

		sl_bool writeUint8(sl_uint8 value) noexcept;

		sl_bool writeInt8(sl_int8 value) noexcept;

		sl_bool writeUint16BE(sl_uint16 value) noexcept;

		sl_bool writeUint16LE(sl_uint16 value) noexcept;

		sl_bool writeInt16BE(sl_int16 value) noexcept;

		sl_bool writeInt16LE(sl_int16 value) noexcept;

		sl_bool writeUint32BE(sl_uint32 value) noexcept;

		sl_bool writeUint32LE(sl_uint32 value) noexcept;

		sl_bool writeInt32BE(sl_int32 value) noexcept;

		sl_bool writeInt32LE(sl_int32 value) noexcept;

		sl_bool writeUint64BE(sl_uint64 value) noexcept;

		sl_bool writeUint64LE(sl_uint64 value) noexcept;

		sl_bool writeInt64BE(sl_int64 value) noexcept;

		sl_bool writeInt64LE(sl_int64 value) noexcept;

		Memory releaseToMemory() noexcept;

	protected:
		sl_bool _growSmallSize(sl_size addSize) noexcept;

		sl_bool _growSize(sl_size newSize) noexcept;

	};

}

#endif
