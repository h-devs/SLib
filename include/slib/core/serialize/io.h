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

#ifndef CHECKHEADER_SLIB_CORE_SERIALIZE_IO
#define CHECKHEADER_SLIB_CORE_SERIALIZE_IO

#include "buffer.h"

namespace slib
{

	class MemoryBuffer;
	class IReader;
	class IWriter;

	sl_bool SerializeByte(IWriter* writer, sl_uint8 value) noexcept;
	sl_bool SerializeByte(MemoryBuffer* buf, sl_uint8 value) noexcept;
	sl_bool SerializeByte(SerializeBuffer* buf, sl_uint8 value) noexcept;

	sl_bool SerializeRaw(IWriter* writer, const void* data, sl_size size) noexcept;
	sl_bool SerializeRaw(MemoryBuffer* buf, const void* data, sl_size size) noexcept;
	sl_bool SerializeRaw(SerializeBuffer* buf, const void* data, sl_size size) noexcept;
	sl_bool SerializeRaw(sl_uint8** buf, const void* data, sl_size size) noexcept;
	sl_bool SerializeRaw(sl_uint8*& buf, const void* data, sl_size size) noexcept;

	sl_bool SerializeRaw(IWriter* writer, const MemoryData& data) noexcept;
	sl_bool SerializeRaw(MemoryBuffer* buf, const MemoryData& data) noexcept;
	sl_bool SerializeRaw(MemoryBuffer* buf, MemoryData&& data) noexcept;
	sl_bool SerializeRaw(SerializeBuffer* buf, const MemoryData& data) noexcept;
	sl_bool SerializeRaw(sl_uint8** buf, const MemoryData& data) noexcept;
	sl_bool SerializeRaw(sl_uint8*& buf, const MemoryData& data) noexcept;

	sl_bool SerializeStatic(IWriter* writer, const void* data, sl_size size) noexcept;
	sl_bool SerializeStatic(MemoryBuffer* buf, const void* data, sl_size size) noexcept;
	sl_bool SerializeStatic(SerializeBuffer* buf, const void* data, sl_size size) noexcept;
	sl_bool SerializeStatic(sl_uint8** buf, const void* data, sl_size size) noexcept;
	sl_bool SerializeStatic(sl_uint8*& buf, const void* data, sl_size size) noexcept;

	sl_bool DeserializeByte(IReader* reader, sl_uint8& _out) noexcept;
	sl_bool DeserializeByte(SerializeBuffer* buf, sl_uint8& _out) noexcept;

	sl_bool DeserializeRaw(IReader* reader, void* data, sl_size size) noexcept;
	sl_bool DeserializeRaw(SerializeBuffer* buf, void* data, sl_size size) noexcept;
	sl_bool DeserializeRaw(sl_uint8** buf, void* data, sl_size size) noexcept;
	sl_bool DeserializeRaw(sl_uint8 const** buf, void* data, sl_size size) noexcept;
	sl_bool DeserializeRaw(sl_uint8*& buf, void* data, sl_size size) noexcept;
	sl_bool DeserializeRaw(sl_uint8 const*& buf, void* data, sl_size size) noexcept;

	SLIB_INLINE sl_bool SerializeByte(sl_uint8** _buf, sl_uint8 value) noexcept
	{
		sl_uint8*& buf = *_buf;
		*buf = value;
		buf++;
		return sl_true;
	}

	SLIB_INLINE sl_bool SerializeByte(sl_uint8*& buf, sl_uint8 value) noexcept
	{
		*buf = value;
		buf++;
		return sl_true;
	}

	SLIB_INLINE sl_bool DeserializeByte(sl_uint8** _buf, sl_uint8& _out) noexcept
	{
		sl_uint8*& buf = *_buf;
		_out = *buf;
		buf++;
		return sl_true;
	}

	SLIB_INLINE sl_bool DeserializeByte(sl_uint8*& buf, sl_uint8& _out) noexcept
	{
		_out = *buf;
		buf++;
		return sl_true;
	}

	SLIB_INLINE sl_bool DeserializeByte(sl_uint8 const** _buf, sl_uint8& _out) noexcept
	{
		sl_uint8 const*& buf = *_buf;
		_out = *buf;
		buf++;
		return sl_true;
	}

	SLIB_INLINE sl_bool DeserializeByte(sl_uint8 const*& buf, sl_uint8& _out) noexcept
	{
		_out = *buf;
		buf++;
		return sl_true;
	}

	template <class OUTPUT, sl_size N>
	SLIB_INLINE sl_bool SerializeStatic(OUTPUT* output, const char(&s)[N]) noexcept
	{
		return SerializeStatic(output, s, N - 1);
	}

}

#endif
