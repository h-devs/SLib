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

#ifndef CHECKHEADER_SLIB_IO_PRIV_DEF
#define CHECKHEADER_SLIB_IO_PRIV_DEF

#include "../../core/endian.h"
#include "../../core/charset.h"

#define SLIB_IO_ENDED 0
#define SLIB_IO_EMPTY_CONTENT 0
#define SLIB_IO_ERROR (-1)
#define SLIB_IO_WOULD_BLOCK (-2)
#define SLIB_IO_TIMEOUT (-2)

#define SLIB_DECLARE_IREADER_MEMBERS(...) \
	sl_reg readFully(void* buf, sl_size size, sl_int32 timeout = -1) __VA_ARGS__; \
	sl_reg readFully(MemoryBuffer& output, sl_size size = SLIB_SIZE_MAX, sl_size segmentSize = 0, sl_int32 timeout = -1) __VA_ARGS__; \
	Memory readFully(sl_size size = SLIB_SIZE_MAX, sl_size segmentSize = 0, sl_int32 timeout = -1) __VA_ARGS__; \
	sl_bool readInt8(sl_int8* output) __VA_ARGS__; \
	sl_int8 readInt8(sl_int8 def = 0) __VA_ARGS__; \
	sl_bool readUint8(sl_uint8* output) __VA_ARGS__; \
	sl_uint8 readUint8(sl_uint8 def = 0) __VA_ARGS__; \
	sl_bool readInt16(sl_int16* output, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_int16 readInt16(sl_int16 def = 0, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool readUint16(sl_uint16* output, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_uint16 readUint16(sl_uint16 def = 0, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool readInt32(sl_int32* output, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_int32 readInt32(sl_int32 def = 0, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool readUint32(sl_uint32* output, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_uint32 readUint32(sl_uint32 def = 0, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool readInt64(sl_int64* output, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_int64 readInt64(sl_int64 def = 0, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool readUint64(sl_uint64* output, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_uint64 readUint64(sl_uint64 def = 0, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool readFloat(float* output, EndianType endian = Endian::Little) __VA_ARGS__; \
	float readFloat(float def = 0, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool readDouble(double* output, EndianType endian = Endian::Little) __VA_ARGS__; \
	double readDouble(double def = 0, EndianType endian = Endian::Little) __VA_ARGS__;

#define SLIB_DECLARE_IWRITER_MEMBERS(...) \
	sl_reg writeFully(const void* buf, sl_size size, sl_int32 timeout = -1) __VA_ARGS__; \
	sl_reg writeFully(const MemoryView& mem, sl_int32 timeout = -1) __VA_ARGS__; \
	sl_bool writeInt8(sl_int8 value) __VA_ARGS__; \
	sl_bool writeUint8(sl_uint8 value) __VA_ARGS__; \
	sl_bool writeInt16(sl_int16 value, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool writeUint16(sl_uint16 value, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool writeInt32(sl_int32 value, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool writeUint32(sl_uint32 value, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool writeInt64(sl_int64 value, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool writeUint64(sl_uint64 value, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool writeFloat(float value, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool writeDouble(double value, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool writeAll(const void* buf, sl_size size) __VA_ARGS__; \
	sl_bool writeAll(const MemoryView& mem) __VA_ARGS__; \
	sl_bool writeAll(const StringView& str) __VA_ARGS__;

#define SLIB_DECLARE_ISTREAM_MEMBERS(...) \
	SLIB_DECLARE_IREADER_MEMBERS(__VA_ARGS__) \
	SLIB_DECLARE_IWRITER_MEMBERS(__VA_ARGS__)

#define SLIB_DECLARE_ISIZE_MEMBERS(...) \
	sl_uint64 getSize() __VA_ARGS__;

#define SLIB_DECLARE_ISEEKABLE_MEMBERS(...) \
	sl_uint64 getPosition() __VA_ARGS__; \
	sl_bool isEnd() __VA_ARGS__; \
	sl_bool seekToBegin() __VA_ARGS__; \
	sl_bool seekToEnd() __VA_ARGS__;

#define SLIB_DECLARE_SEEKABLE_READER_MEMBERS(ATTR, OVERRIDE) \
	sl_reg readAt(sl_uint64 offset, void* buf, sl_size size, sl_int32 timeout = -1) ATTR OVERRIDE; \
	sl_int32 readAt32(sl_uint64 offset, void* buf, sl_uint32 size, sl_int32 timeout = -1) ATTR OVERRIDE; \
	sl_reg readFullyAt(sl_uint64 offset, void* buf, sl_size size, sl_int32 timeout = -1) ATTR OVERRIDE; \
	String readLine() ATTR; \
	String readNullTerminatedString() ATTR; \
	Memory readAllBytes(sl_size maxSize = SLIB_SIZE_MAX) ATTR; \
	sl_int64 find(const void* pattern, sl_size nPattern, sl_int64 startPosition = 0, sl_uint64 sizeFind = SLIB_UINT64_MAX) ATTR; \
	sl_int64 findBackward(const void* pattern, sl_size nPattern, sl_int64 startPosition = -1, sl_uint64 sizeFind = SLIB_UINT64_MAX) ATTR;

#define SLIB_DECLARE_SEEKABLE_WRITER_MEMBERS(ATTR, OVERRIDE) \
	sl_reg writeAt(sl_uint64 offset, const void* buf, sl_size size, sl_int32 timeout = -1) ATTR OVERRIDE; \
	sl_int32 writeAt32(sl_uint64 offset, const void* buf, sl_uint32 size, sl_int32 timeout = -1) ATTR OVERRIDE; \
	sl_reg writeFullyAt(sl_uint64 offset, const void* buf, sl_size size, sl_int32 timeout = -1) ATTR OVERRIDE;

#define SLIB_DECLARE_IO_MEMBERS(...) \
	SLIB_DECLARE_ISTREAM_MEMBERS(__VA_ARGS__) \
	SLIB_DECLARE_ISIZE_MEMBERS(__VA_ARGS__) \
	SLIB_DECLARE_ISEEKABLE_MEMBERS(__VA_ARGS__) \
	SLIB_DECLARE_SEEKABLE_READER_MEMBERS(__VA_ARGS__,) \
	SLIB_DECLARE_SEEKABLE_WRITER_MEMBERS(__VA_ARGS__,)

namespace slib
{

	class Memory;
	class MemoryBuffer;
	class String;
	class StringView;

	enum class SeekPosition
	{
		Current = 1,
		Begin = 2,
		End = 3
	};

}

#endif
