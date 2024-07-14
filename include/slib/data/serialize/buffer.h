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

#ifndef CHECKHEADER_SLIB_IO_SERIALIZE_BUFFER
#define CHECKHEADER_SLIB_IO_SERIALIZE_BUFFER

#include "../../core/ref.h"
#include "../../core/default_members.h"

namespace slib
{

	class Memory;
	class MemoryData;
	class MemoryView;

	class SLIB_EXPORT SerializeBuffer
	{
	public:
		sl_uint8* current;
		sl_uint8* begin;
		sl_uint8* end;
		Ref<CRef> ref;

	public:
		SerializeBuffer(const void* buf, sl_size size) noexcept;

		template <class REF>
		SerializeBuffer(const void* buf, sl_size size, REF&& _ref) noexcept: ref(Forward<REF>(_ref))
		{
			current = begin = (sl_uint8*)buf;
			end = begin ? begin + size : sl_null;
		}

		SerializeBuffer(const MemoryView& mem) noexcept;

		SerializeBuffer(MemoryData&& data) noexcept;

		SerializeBuffer(const Memory& mem) noexcept;

		SerializeBuffer(Memory&& mem) noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SerializeBuffer)

	public:
		SLIB_INLINE sl_size getOffset() const noexcept
		{
			return current - begin;
		}

		sl_bool read(sl_uint8& _out) noexcept;

		sl_bool write(sl_uint8 value) noexcept;

		sl_size read(void* buf, sl_size size) noexcept;

		sl_size write(const void* buf, sl_size size) noexcept;

		sl_size write(const MemoryView& mem) noexcept;

		sl_bool readUint8(sl_uint8& _out) noexcept;

		sl_bool writeUint8(sl_uint8 value) noexcept;

		sl_bool readInt8(sl_int8& _out) noexcept;

		sl_bool writeInt8(sl_int8 value) noexcept;

		sl_bool readUint16BE(sl_uint16& _out) noexcept;

		sl_bool writeUint16BE(sl_uint16 value) noexcept;

		sl_bool readUint16LE(sl_uint16& _out) noexcept;

		sl_bool writeUint16LE(sl_uint16 value) noexcept;

		sl_bool readInt16BE(sl_int16& _out) noexcept;

		sl_bool writeInt16BE(sl_int16 value) noexcept;

		sl_bool readInt16LE(sl_int16& _out) noexcept;

		sl_bool writeInt16LE(sl_int16 value) noexcept;

		sl_bool readUint32BE(sl_uint32& _out) noexcept;

		sl_bool writeUint32BE(sl_uint32 value) noexcept;

		sl_bool readUint32LE(sl_uint32& _out) noexcept;

		sl_bool writeUint32LE(sl_uint32 value) noexcept;

		sl_bool readInt32BE(sl_int32& _out) noexcept;

		sl_bool writeInt32BE(sl_int32 value) noexcept;

		sl_bool readInt32LE(sl_int32& _out) noexcept;

		sl_bool writeInt32LE(sl_int32 value) noexcept;

		sl_bool readUint64BE(sl_uint64& _out) noexcept;

		sl_bool writeUint64BE(sl_uint64 value) noexcept;

		sl_bool readUint64LE(sl_uint64& _out) noexcept;

		sl_bool writeUint64LE(sl_uint64 value) noexcept;

		sl_bool readInt64BE(sl_int64& _out) noexcept;

		sl_bool writeInt64BE(sl_int64 value) noexcept;

		sl_bool readInt64LE(sl_int64& _out) noexcept;

		sl_bool writeInt64LE(sl_int64 value) noexcept;

		sl_bool readSection(void* buf, sl_size size) noexcept;

		sl_bool skip(sl_size size) noexcept;

	};

}

#endif
