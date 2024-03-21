/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_IO_PRIV_IMPL
#define CHECKHEADER_SLIB_IO_PRIV_IMPL

#include "def.h"
#include "reader_helper.h"
#include "writer_helper.h"
#include "seekable_reader_helper.h"

#define SLIB_DEFINE_IREADER_MEMBERS(CLASS, ATTR) \
	sl_reg CLASS::readFully(void* buf, sl_size size, sl_int32 timeout) ATTR { return ReaderHelper::readFully(this, buf, size, timeout); } \
	sl_reg CLASS::readFully(MemoryBuffer& output, sl_size size, sl_size segmentSize, sl_int32 timeout) ATTR { return ReaderHelper::readFully(this, output, size, segmentSize, timeout); } \
	Memory CLASS::readFully(sl_size size, sl_size segmentSize, sl_int32 timeout) ATTR { return ReaderHelper::readFully(this, size, segmentSize, timeout); }
	sl_bool CLASS::readInt8(sl_int8* output) ATTR { return ReaderHelper::readInt8(this, output); } \
	sl_int8 CLASS::readInt8(sl_int8 def) ATTR { return ReaderHelper::readInt8(this, def); } \
	sl_bool CLASS::readUint8(sl_uint8* output) ATTR { return ReaderHelper::readUint8(this, output); } \
	sl_uint8 CLASS::readUint8(sl_uint8 def) ATTR { return ReaderHelper::readUint8(this, def); } \
	sl_bool CLASS::readInt16(sl_int16* output, EndianType endian) ATTR { return ReaderHelper::readInt16(this, output, endian); } \
	sl_int16 CLASS::readInt16(sl_int16 def, EndianType endian) ATTR { return ReaderHelper::readInt16(this, def, endian); } \
	sl_bool CLASS::readUint16(sl_uint16* output, EndianType endian) ATTR { return ReaderHelper::readUint16(this, output, endian); } \
	sl_uint16 CLASS::readUint16(sl_uint16 def, EndianType endian) ATTR { return ReaderHelper::readUint16(this, def, endian); } \
	sl_bool CLASS::readInt32(sl_int32* output, EndianType endian) ATTR { return ReaderHelper::readInt32(this, output, endian); } \
	sl_int32 CLASS::readInt32(sl_int32 def, EndianType endian) ATTR { return ReaderHelper::readInt32(this, def, endian); } \
	sl_bool CLASS::readUint32(sl_uint32* output, EndianType endian) ATTR { return ReaderHelper::readUint32(this, output, endian); } \
	sl_uint32 CLASS::readUint32(sl_uint32 def, EndianType endian) ATTR { return ReaderHelper::readUint32(this, def, endian); } \
	sl_bool CLASS::readInt64(sl_int64* output, EndianType endian) ATTR { return ReaderHelper::readInt64(this, output, endian); } \
	sl_int64 CLASS::readInt64(sl_int64 def, EndianType endian) ATTR { return ReaderHelper::readInt64(this, def, endian); } \
	sl_bool CLASS::readUint64(sl_uint64* output, EndianType endian) ATTR { return ReaderHelper::readUint64(this, output, endian); } \
	sl_uint64 CLASS::readUint64(sl_uint64 def, EndianType endian) ATTR { return ReaderHelper::readUint64(this, def, endian); } \
	sl_bool CLASS::readFloat(float* output, EndianType endian) ATTR { return ReaderHelper::readFloat(this, output, endian); } \
	float CLASS::readFloat(float def, EndianType endian) ATTR { return ReaderHelper::readFloat(this, def, endian); } \
	sl_bool CLASS::readDouble(double* output, EndianType endian) ATTR { return ReaderHelper::readDouble(this, output, endian); } \
	double CLASS::readDouble(double def, EndianType endian) ATTR { return ReaderHelper::readDouble(this, def, endian); } \

#define SLIB_DEFINE_IWRITER_MEMBERS(CLASS, ATTR) \
	sl_reg CLASS::writeFully(const void* buf, sl_size size, sl_int32 timeout) ATTR { return WriterHelper::writeFully(this, buf, size, timeout); } \
	sl_reg CLASS::writeFully(const MemoryView& mem, sl_int32 timeout) ATTR { return WriterHelper::writeFully(this, mem.data, mem.size, timeout); } \
	sl_reg CLASS::writeFully(const StringView& str, sl_int32 timeout) ATTR { return WriterHelper::writeFully(this, str.getData(), str.getLength(), timeout); } \
	sl_bool CLASS::writeInt8(sl_int8 value) ATTR { return WriterHelper::writeInt8(this, value); } \
	sl_bool CLASS::writeUint8(sl_uint8 value) ATTR { return WriterHelper::writeInt8(this, value); } \
	sl_bool CLASS::writeInt16(sl_int16 value, EndianType endian) ATTR { return WriterHelper::writeInt16(this, value, endian); } \
	sl_bool CLASS::writeUint16(sl_uint16 value, EndianType endian) ATTR { return WriterHelper::writeInt16(this, value, endian); } \
	sl_bool CLASS::writeInt32(sl_int32 value, EndianType endian) ATTR { return WriterHelper::writeInt32(this, value, endian); } \
	sl_bool CLASS::writeUint32(sl_uint32 value, EndianType endian) ATTR { return WriterHelper::writeInt32(this, value, endian); } \
	sl_bool CLASS::writeInt64(sl_int64 value, EndianType endian) ATTR { return WriterHelper::writeInt64(this, value, endian); } \
	sl_bool CLASS::writeUint64(sl_uint64 value, EndianType endian) ATTR { return WriterHelper::writeInt64(this, value, endian); } \
	sl_bool CLASS::writeFloat(float value, EndianType endian) ATTR { return WriterHelper::writeFloat(this, value, endian); } \
	sl_bool CLASS::writeDouble(double value, EndianType endian) ATTR { return WriterHelper::writeDouble(this, value, endian); } \
	sl_bool CLASS::writeAllBytes(const void* buf, sl_size size, sl_int32 timeout) ATTR { return WriterHelper::writeFully(this, buf, size, timeout) == (sl_reg)size; } \
	sl_bool CLASS::writeAllBytes(const MemoryView& mem, sl_int32 timeout) ATTR { return WriterHelper::writeFully(this, mem.data, mem.size, timeout) == (sl_reg)(mem.size); } \
	sl_bool CLASS::writeAllBytes(const StringView& str, sl_int32 timeout) ATTR { return writeAllBytes(str.getData(), str.getLength(), timeout); }

#define SLIB_DEFINE_ISTREAM_MEMBERS(CLASS, ATTR) \
	SLIB_DEFINE_IREADER_MEMBERS(CLASS, ATTR) \
	SLIB_DEFINE_IWRITER_MEMBERS(CLASS, ATTR)

#define SLIB_DEFINE_ISIZE_MEMBERS(CLASS, ATTR) \
	sl_uint64 CLASS::getSize() ATTR \
	{ \
		sl_uint64 size; \
		if (getSize(size)) { \
			return size; \
		} \
		return 0; \
	}

#define SLIB_DEFINE_ISEEKABLE_MEMBERS(CLASS, ATTR) \
	sl_uint64 CLASS::getPosition() ATTR \
	{ \
		sl_uint64 pos; \
		if (getPosition(pos)) { \
			return pos; \
		} \
		return 0; \
	} \
	sl_bool CLASS::isEnd() ATTR \
	{ \
		sl_bool flag; \
		if (isEnd(flag)) { \
			return flag; \
		} \
		return sl_false; \
	} \
	sl_bool CLASS::seekToBegin() ATTR \
	{ \
		return seek(0, SeekPosition::Begin); \
	} \
	sl_bool CLASS::seekToEnd() ATTR \
	{ \
		return seek(0, SeekPosition::End); \
	}

#define SLIB_DEFINE_SEEKABLE_READER_MEMBERS(CLASS, ATTR) \
	sl_reg CLASS::readAt(sl_uint64 offset, void* buf, sl_size size) ATTR { return (seek(offset, SeekPosition::Begin)) ? read(buf, size) : -1; } \
	sl_int32 CLASS::readAt32(sl_uint64 offset, void* buf, sl_uint32 size) ATTR { return (seek(offset, SeekPosition::Begin)) ? read32(buf, size) : -1; } \
	sl_reg CLASS::readFullyAt(sl_uint64 offset, void* buf, sl_size size, sl_int32 timeout) ATTR { return (seek(offset, SeekPosition::Begin)) ? readFully(buf, size, timeout) : -1; } \
	String CLASS::readLine() ATTR { return SeekableReaderHelper::readLine(this, this); } \
	String CLASS::readNullTerminatedString() ATTR { return SeekableReaderHelper::readNullTerminatedString(this, this); } \
	Memory CLASS::readAllBytes(sl_size maxSize) ATTR { return SeekableReaderHelper::readAllBytes(this, this, maxSize); } \
	sl_int64 CLASS::find(const void* pattern, sl_size nPattern, sl_int64 startPosition, sl_uint64 sizeFind) ATTR { return SeekableReaderHelper::find(this, this, pattern, nPattern, startPosition, sizeFind); } \
	sl_int64 CLASS::findBackward(const void* pattern, sl_size nPattern, sl_int64 startPosition, sl_uint64 sizeFind) ATTR { return SeekableReaderHelper::findBackward(this, this, pattern, nPattern, startPosition, sizeFind); }

#define SLIB_DEFINE_SEEKABLE_WRITER_MEMBERS(CLASS, ATTR) \
	sl_reg CLASS::writeAt(sl_uint64 offset, const void* buf, sl_size size) ATTR { return (seek(offset, SeekPosition::Begin)) ? write(buf, size) : -1; } \
	sl_int32 CLASS::writeAt32(sl_uint64 offset, const void* buf, sl_uint32 size) ATTR { return (seek(offset, SeekPosition::Begin)) ? write32(buf, size) : -1; } \
	sl_reg CLASS::writeFullyAt(sl_uint64 offset, const void* buf, sl_size size, sl_int32 timeout) ATTR { return (seek(offset, SeekPosition::Begin)) ? writeFully(buf, size, timeout) : -1; }

#define SLIB_DEFINE_IO_MEMBERS(CLASS, ATTR) \
	SLIB_DEFINE_ISTREAM_MEMBERS(CLASS, ATTR) \
	SLIB_DEFINE_ISIZE_MEMBERS(CLASS, ATTR) \
	SLIB_DEFINE_ISEEKABLE_MEMBERS(CLASS, ATTR) \
	SLIB_DEFINE_SEEKABLE_READER_MEMBERS(CLASS, ATTR) \
	SLIB_DEFINE_SEEKABLE_WRITER_MEMBERS(CLASS, ATTR)

#endif
