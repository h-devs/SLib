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

#ifndef CHECKHEADER_SLIB_CORE_MEMORY_READER
#define CHECKHEADER_SLIB_CORE_MEMORY_READER

#include "io.h"
#include "memory.h"

namespace slib
{
	
	// MemoryReader is not thread-safe
	class SLIB_EXPORT MemoryReader : public IReader, public IBlockReader, public ISeekable
	{
	public:
		MemoryReader(const Memory& mem);
	
		MemoryReader(const void* buf, sl_size size);

		~MemoryReader();

	public:
		SLIB_DECLARE_SEEKABLE_READER_MEMBERS(override)
	
	public:
		void initialize(const Memory& mem);

		void initialize(const void* buf, sl_size size);

		sl_reg read(void* buf, sl_size size) override;

		sl_bool seek(sl_int64 offset, SeekPosition pos) override;

		sl_bool getPosition(sl_uint64& outPos) override;

		sl_bool getSize(sl_uint64& outSize) override;

		sl_size getPosition();

		sl_size getSize();

		sl_size getRemainedSize();

		sl_reg skip(sl_size size);

		sl_uint8* getBuffer();
		
		sl_int64 find(const void* pattern, sl_size nPattern, sl_int64 startPosition = 0, sl_int64 endPosition = -1);

		sl_int64 findBackward(const void* pattern, sl_size nPattern, sl_int64 startPosition = -1, sl_int64 endPosition = -1);

	public:
		sl_bool readInt8(sl_int8* output);

		sl_int8 readInt8(sl_int8 def = 0);

		sl_bool readUint8(sl_uint8* output);

		sl_uint8 readUint8(sl_uint8 def = 0);

		sl_bool readInt16(sl_int16* output, EndianType endian = Endian::Little);

		sl_int16 readInt16(sl_int16 def = 0, EndianType endian = Endian::Little);

		sl_bool readUint16(sl_uint16* output, EndianType endian = Endian::Little);

		sl_uint16 readUint16(sl_uint16 def = 0, EndianType endian = Endian::Little);

		sl_bool readInt32(sl_int32* output, EndianType endian = Endian::Little);

		sl_int32 readInt32(sl_int32 def = 0, EndianType endian = Endian::Little);

		sl_bool readUint32(sl_uint32* output, EndianType endian = Endian::Little);

		sl_uint32 readUint32(sl_uint32 def = 0, EndianType endian = Endian::Little);

		sl_bool readInt64(sl_int64* output, EndianType endian = Endian::Little);

		sl_int64 readInt64(sl_int64 def = 0, EndianType endian = Endian::Little);

		sl_bool readUint64(sl_uint64* output, EndianType endian = Endian::Little);

		sl_uint64 readUint64(sl_uint64 def = 0, EndianType endian = Endian::Little);

		sl_bool readFloat(float* output, EndianType endian = Endian::Little);

		float readFloat(float def = 0, EndianType endian = Endian::Little);

		sl_bool readDouble(double* output, EndianType endian = Endian::Little);

		double readDouble(double def = 0, EndianType endian = Endian::Little);

	protected:
		const void* m_buf;
		sl_size m_size;
		sl_size m_offset;
		Memory m_mem;
	
	};

}

#endif
