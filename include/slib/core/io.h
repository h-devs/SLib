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

#ifndef CHECKHEADER_SLIB_CORE_IO
#define CHECKHEADER_SLIB_CORE_IO

#include "io_base.h"
#include "memory.h"
#include "string.h"
#include "time.h"

namespace slib
{
	
	class SeekableReaderHelper
	{
	public:
		static String readLine(IReader* reader, ISeekable* seekable);

		static String readStringUntilWhitespace(IReader* reader, ISeekable* seekable);

		static String readNullTerminatedString(IReader* reader, ISeekable* seekable);

		static Memory readAllBytes(IReader* reader, ISeekable* seekable, sl_size maxSize);

		static String readAllTextUTF8(IReader* reader, ISeekable* seekable, sl_size maxSize);

		static String16 readAllTextUTF16(IReader* reader, ISeekable* seekable, EndianType endian, sl_size maxSize);

		static String readAllText(IReader* reader, ISeekable* seekable, Charset* outCharset, sl_size maxSize);

		static String16 readAllText16(IReader* reader, ISeekable* seekable, Charset* outCharset, sl_size maxSize);

		static sl_int64 find(IReader* reader, ISeekable* seekable, const void* pattern, sl_size nPattern, sl_int64 startPosition, sl_uint64 sizeFind);

		static sl_int64 findBackward(IReader* reader, ISeekable* seekable, const void* pattern, sl_size nPattern, sl_int64 startPosition, sl_uint64 sizeFind);

	};

	template <class READER>
	class SLIB_EXPORT SeekableReaderBase : public IBlockReader
	{
	public:
		sl_reg readAt(sl_uint64 offset, void* buf, sl_size size) override
		{
			READER* reader = (READER*)this;
			if (reader->seek(offset, SeekPosition::Begin)) {
				return reader->read(buf, size);
			}
			return -1;
		}

		sl_int32 readAt32(sl_uint64 offset, void* buf, sl_uint32 size) override
		{
			READER* reader = (READER*)this;
			if (reader->seek(offset, SeekPosition::Begin)) {
				return reader->read32(buf, size);
			}
			return -1;
		}

		sl_reg readFullyAt(sl_uint64 offset, void* buf, sl_size size) override
		{
			READER* reader = (READER*)this;
			if (reader->seek(offset, SeekPosition::Begin)) {
				return reader->readFully(buf, size);
			}
			return -1;
		}

		String readLine()
		{
			return SeekableReaderHelper::readLine((READER*)this, (READER*)this);
		}

		String readStringUntilWhitespace()
		{
			return SeekableReaderHelper::readStringUntilWhitespace((READER*)this, (READER*)this);
		}

		String readNullTerminatedString()
		{
			return SeekableReaderHelper::readNullTerminatedString((READER*)this, (READER*)this);
		}

		Memory readAllBytes(sl_size maxSize = SLIB_SIZE_MAX)
		{
			return SeekableReaderHelper::readAllBytes((READER*)this, (READER*)this, maxSize);
		}

		String readAllTextUTF8(sl_size maxSize = SLIB_SIZE_MAX)
		{
			return SeekableReaderHelper::readAllTextUTF8((READER*)this, (READER*)this, maxSize);
		}

		String16 readAllTextUTF16(EndianType endian = Endian::Little, sl_size maxSize = SLIB_SIZE_MAX)
		{
			return SeekableReaderHelper::readAllTextUTF16((READER*)this, (READER*)this, endian, maxSize);
		}

		String readAllText(Charset* outCharset = sl_null, sl_size maxSize = SLIB_SIZE_MAX)
		{
			return SeekableReaderHelper::readAllText((READER*)this, (READER*)this, outCharset, maxSize);
		}

		String16 readAllText16(Charset* outCharset = sl_null, sl_size maxSize = SLIB_SIZE_MAX)
		{
			return SeekableReaderHelper::readAllText16((READER*)this, (READER*)this, outCharset, maxSize);
		}

		sl_int64 find(const void* pattern, sl_size nPattern, sl_int64 startPosition = 0, sl_uint64 sizeFind = SLIB_UINT64_MAX)
		{
			return SeekableReaderHelper::find((READER*)this, (READER*)this, pattern, nPattern, startPosition, sizeFind);
		}

		sl_int64 findBackward(const void* pattern, sl_size nPattern, sl_int64 startPosition = -1, sl_uint64 sizeFind = SLIB_UINT64_MAX)
		{
			return SeekableReaderHelper::findBackward((READER*)this, (READER*)this, pattern, nPattern, startPosition, sizeFind);
		}

	};

	template <class WRITER>
	class SLIB_EXPORT SeekableWriterBase : public IBlockWriter
	{
	public:
		sl_reg writeAt(sl_uint64 offset, const void* buf, sl_size size) override
		{
			WRITER* writer = (WRITER*)this;
			if (writer->seek(offset, SeekPosition::Begin)) {
				return writer->write(buf, size);
			}
			return -1;
		}

		sl_int32 writeAt32(sl_uint64 offset, const void* buf, sl_uint32 size) override
		{
			WRITER* writer = (WRITER*)this;
			if (writer->seek(offset, SeekPosition::Begin)) {
				return writer->write32(buf, size);
			}
			return -1;
		}

		sl_reg writeFullyAt(sl_uint64 offset, const void* buf, sl_size size) override
		{
			WRITER* writer = (WRITER*)this;
			if (writer->seek(offset, SeekPosition::Begin)) {
				return writer->writeFully(buf, size);
			}
			return -1;
		}

	};

	class SLIB_EXPORT IO : public IStream, public ISeekable, public IResizable, public SeekableReaderBase<IO>, public SeekableWriterBase<IO>
	{
	};
	
}

#endif
