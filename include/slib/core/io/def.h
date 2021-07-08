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

#ifndef CHECKHEADER_SLIB_CORE_IO_DEF
#define CHECKHEADER_SLIB_CORE_IO_DEF

#include "../endian.h"
#include "../charset.h"

#define SLIB_IO_ENDED 0
#define SLIB_IO_EMPTY_CONTENT 0
#define SLIB_IO_ERROR (-1)
#define SLIB_IO_WOULD_BLOCK (-2)

#define SLIB_DECLARE_IREADER_MEMBERS(...) \
	sl_reg readFully(void* buf, sl_size size) __VA_ARGS__; \
	Memory readFully() __VA_ARGS__; \
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
	double readDouble(double def = 0, EndianType endian = Endian::Little) __VA_ARGS__; \
	sl_bool readCVLI32(sl_uint32* output) __VA_ARGS__; \
	sl_uint32 readCVLI32(sl_uint32 def = 0) __VA_ARGS__; \
	sl_bool readCVLI64(sl_uint64* output) __VA_ARGS__; \
	sl_uint64 readCVLI64(sl_uint64 def = 0) __VA_ARGS__; \
	sl_bool readCVLI(sl_size* output) __VA_ARGS__; \
	sl_size readCVLI(sl_size def = 0) __VA_ARGS__; \
	Memory readToMemory(sl_size size) __VA_ARGS__; \
	sl_bool readSectionData(void* data, sl_size& size) __VA_ARGS__; \
	sl_bool readSection(Memory* output, sl_size maxSize = SLIB_SIZE_MAX) __VA_ARGS__; \
	Memory readSection(const Memory& def, sl_size maxSize = SLIB_SIZE_MAX) __VA_ARGS__; \
	Memory readSection(sl_size maxLen = SLIB_SIZE_MAX) __VA_ARGS__; \
	sl_bool readStringSection(String* output, sl_size maxUtf8Len = SLIB_SIZE_MAX) __VA_ARGS__; \
	String readStringSection(const String& def, sl_size maxUtf8Len = SLIB_SIZE_MAX) __VA_ARGS__; \
	String readStringSection(sl_size maxUtf8Len = SLIB_SIZE_MAX) __VA_ARGS__; \
	sl_bool readTime(Time* output) __VA_ARGS__; \
	Time readTime() __VA_ARGS__; \
	Time readTime(const Time& def) __VA_ARGS__; \
	String readTextUTF8(sl_size size) __VA_ARGS__; \
	String16 readTextUTF16(sl_size size, EndianType endian = Endian::Little) __VA_ARGS__; \
	String readText(sl_size size, Charset* outCharset = sl_null) __VA_ARGS__; \
	String16 readText16(sl_size size, Charset* outCharset = sl_null) __VA_ARGS__;

#define SLIB_DECLARE_IWRITER_MEMBERS(...) \
	sl_reg writeFully(const void* buf, sl_size size) __VA_ARGS__; \
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
	sl_bool writeCVLI32(sl_uint32 value) __VA_ARGS__; \
	sl_bool writeCVLI64(sl_uint64 value) __VA_ARGS__; \
	sl_bool writeCVLI(sl_size value) __VA_ARGS__; \
	sl_size writeFromMemory(const Memory& mem) __VA_ARGS__; \
	sl_bool writeSection(const void* mem, sl_size size) __VA_ARGS__; \
	sl_bool writeSection(const Memory& mem) __VA_ARGS__; \
	sl_bool writeStringSection(const StringParam& str, sl_size maxUtf8Len = SLIB_SIZE_MAX) __VA_ARGS__; \
	sl_bool writeTime(const Time& t) __VA_ARGS__; \
	sl_bool writeTextUTF8(const StringParam& text, sl_bool flagWriteByteOrderMark = sl_false) __VA_ARGS__; \
	sl_bool writeTextUTF16LE(const StringParam& text, sl_bool flagWriteByteOrderMark = sl_false) __VA_ARGS__; \
	sl_bool writeTextUTF16BE(const StringParam& text, sl_bool flagWriteByteOrderMark = sl_false) __VA_ARGS__;

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
	sl_reg readAt(sl_uint64 offset, void* buf, sl_size size) ATTR OVERRIDE; \
	sl_int32 readAt32(sl_uint64 offset, void* buf, sl_uint32 size) ATTR OVERRIDE; \
	sl_reg readFullyAt(sl_uint64 offset, void* buf, sl_size size) ATTR OVERRIDE; \
	String readLine() ATTR; \
	String readStringUntilWhitespace() ATTR; \
	String readNullTerminatedString() ATTR; \
	Memory readAllBytes(sl_size maxSize = SLIB_SIZE_MAX) ATTR; \
	String readAllTextUTF8(sl_size maxSize = SLIB_SIZE_MAX) ATTR; \
	String16 readAllTextUTF16(EndianType endian = Endian::Little, sl_size maxSize = SLIB_SIZE_MAX) ATTR; \
	String readAllText(Charset* outCharset = sl_null, sl_size maxSize = SLIB_SIZE_MAX) ATTR; \
	String16 readAllText16(Charset* outCharset = sl_null, sl_size maxSize = SLIB_SIZE_MAX) ATTR; \
	sl_int64 find(const void* pattern, sl_size nPattern, sl_int64 startPosition = 0, sl_uint64 sizeFind = SLIB_UINT64_MAX) ATTR; \
	sl_int64 findBackward(const void* pattern, sl_size nPattern, sl_int64 startPosition = -1, sl_uint64 sizeFind = SLIB_UINT64_MAX) ATTR;

#define SLIB_DECLARE_SEEKABLE_WRITER_MEMBERS(ATTR, OVERRIDE) \
	sl_reg writeAt(sl_uint64 offset, const void* buf, sl_size size) ATTR OVERRIDE; \
	sl_int32 writeAt32(sl_uint64 offset, const void* buf, sl_uint32 size) ATTR OVERRIDE; \
	sl_reg writeFullyAt(sl_uint64 offset, const void* buf, sl_size size) ATTR OVERRIDE;

#define SLIB_DECLARE_IO_MEMBERS(...) \
	SLIB_DECLARE_ISTREAM_MEMBERS(__VA_ARGS__) \
	SLIB_DECLARE_ISIZE_MEMBERS(__VA_ARGS__) \
	SLIB_DECLARE_ISEEKABLE_MEMBERS(__VA_ARGS__) \
	SLIB_DECLARE_SEEKABLE_READER_MEMBERS(__VA_ARGS__,) \
	SLIB_DECLARE_SEEKABLE_WRITER_MEMBERS(__VA_ARGS__,)

namespace slib
{

	class Memory;
	class String;
	class String16;
	class StringParam;
	class Time;

	enum class SeekPosition
	{
		Current = 1,
		Begin = 2,
		End = 3
	};

}

#endif
