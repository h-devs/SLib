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

#ifndef CHECKHEADER_SLIB_IO_MEMORY_OUTPUT
#define CHECKHEADER_SLIB_IO_MEMORY_OUTPUT

#include "io.h"

#include "../core/memory_buffer.h"

namespace slib
{

	// MemoryOutput is not thread-safe
	class SLIB_EXPORT MemoryOutput : public IWriter
	{
	public:
		MemoryOutput();

		~MemoryOutput();

	public:
		sl_reg write(const void* buf, sl_size size) override;

		sl_reg write(const Memory& mem);

		sl_reg write(Memory&& mem);

		sl_bool flush();

		sl_size getSize();

		Memory getData();

		void clear();

		void link(MemoryBuffer& mem);

		void link(MemoryOutput& other);

	public:
		sl_bool writeInt8(sl_int8 value);

		sl_bool writeUint8(sl_uint8 value);

		sl_bool writeInt16(sl_int16 value, EndianType endian = Endian::Little);

		sl_bool writeUint16(sl_uint16 value, EndianType endian = Endian::Little);

		sl_bool writeInt32(sl_int32 value, EndianType endian = Endian::Little);

		sl_bool writeUint32(sl_uint32 value, EndianType endian = Endian::Little);

		sl_bool writeInt64(sl_int64 value, EndianType endian = Endian::Little);

		sl_bool writeUint64(sl_uint64 value, EndianType endian = Endian::Little);

		sl_bool writeFloat(float value, EndianType endian = Endian::Little);

		sl_bool writeDouble(double value, EndianType endian = Endian::Little);

	protected:
		MemoryBuffer m_queue;
		List<sl_uint8> m_buffer;

	};

}

#endif
