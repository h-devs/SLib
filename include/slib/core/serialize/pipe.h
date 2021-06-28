/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation pipes (the "Software"), to deal
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

#ifndef CHECKHEADER_SLIB_CORE_SERIALIZE_PIPE
#define CHECKHEADER_SLIB_CORE_SERIALIZE_PIPE

#include "../pipe.h"
#include "../memory.h"

namespace slib
{

	SLIB_INLINE static sl_bool SerializeByte(Pipe* pipe, sl_uint8 value) noexcept
	{
		return pipe->writeUint8(value);
	}

	SLIB_INLINE static sl_bool SerializeRaw(Pipe* pipe, const void* data, sl_size size) noexcept
	{
		return pipe->writeFully(data, size) == size;
	}

	SLIB_INLINE static sl_bool SerializeRaw(Pipe* pipe, const MemoryData& data) noexcept
	{
		return pipe->writeFully(data.data, data.size) == data.size;
	}

	SLIB_INLINE static sl_bool SerializeStatic(Pipe* pipe, const void* data, sl_size size) noexcept
	{
		return pipe->writeFully(data, size) == size;
	}

	SLIB_INLINE static sl_bool DeserializeByte(Pipe* pipe, sl_uint8& value) noexcept
	{
		return pipe->readUint8(&value);
	}

	SLIB_INLINE static sl_bool DeserializeRaw(Pipe* pipe, void* data, sl_size size) noexcept
	{
		return pipe->readFully(data, size) == size;
	}

}

#endif
