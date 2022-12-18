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

#ifndef CHECKHEADER_SLIB_IO_IO
#define CHECKHEADER_SLIB_IO_IO

#include "definition.h"

#include "../core/wrapper.h"

#include "priv/def.h"

#define PRIV_SLIB_DEFINE_IO_DEFAULT_MEMBERS(CLASS) \
	public: \
		SLIB_DEFINE_WRAPPER_DEFAULT_MEMBERS_INLINE(CLASS, T, base) \
		CLASS() {} \
		SLIB_CONSTEXPR sl_bool isOpened() { return base ? sl_true : sl_false; }

namespace slib
{

	class SLIB_EXPORT IReader
	{
	public:
		virtual sl_reg read(void* buf, sl_size size);

		virtual sl_int32 read32(void* buf, sl_uint32 size);

		virtual sl_bool waitRead(sl_int32 timeout = -1);

	public:
		SLIB_DECLARE_IREADER_MEMBERS()

	};

	class SLIB_EXPORT IWriter
	{
	public:
		virtual sl_reg write(const void* buf, sl_size size);

		virtual sl_int32 write32(const void* buf, sl_uint32 size);

		virtual sl_bool waitWrite(sl_int32 timeout = -1);

	public:
		SLIB_DECLARE_IWRITER_MEMBERS()

	};

	class SLIB_EXPORT IBlockReader
	{
	public:
		virtual sl_reg readAt(sl_uint64 offset, void* buf, sl_size size);

		virtual sl_int32 readAt32(sl_uint64 offset, void* buf, sl_uint32 size);

		virtual sl_reg readFullyAt(sl_uint64 offset, void* buf, sl_size size);

		virtual sl_bool waitRead(sl_int32 timeout = -1);

	};

	class SLIB_EXPORT IBlockWriter
	{
	public:
		virtual sl_reg writeAt(sl_uint64 offset, const void* buf, sl_size size);

		virtual sl_int32 writeAt32(sl_uint64 offset, const void* buf, sl_uint32 size);

		virtual sl_reg writeFullyAt(sl_uint64 offset, const void* buf, sl_size size);

		virtual sl_bool waitWrite(sl_int32 timeout = -1);

	};


	class SLIB_EXPORT ISizeProvider
	{
	public:
		virtual sl_bool getSize(sl_uint64& outSize) = 0;

	public:
		SLIB_DECLARE_ISIZE_MEMBERS()

	};

	class SLIB_EXPORT IResizable
	{
	public:
		virtual sl_bool setSize(sl_uint64 size) = 0;
	};

	class SLIB_EXPORT ISeekable : public ISizeProvider
	{
	public:
		virtual sl_bool getPosition(sl_uint64& outPos) = 0;

		virtual sl_bool seek(sl_int64 offset, SeekPosition pos) = 0;

		virtual sl_bool isEnd(sl_bool& outFlag);

	public:
		SLIB_DECLARE_ISEEKABLE_MEMBERS()

	};

	class SLIB_EXPORT IClosable
	{
	public:
		virtual void close() = 0;
	};

	class SLIB_EXPORT IStream : public IReader, public IWriter, public IClosable
	{
	};


	template <class T>
	class SLIB_EXPORT Reader : public IReader, public IClosable
	{
		PRIV_SLIB_DEFINE_IO_DEFAULT_MEMBERS(Reader)

	public:
		sl_reg read(void* buf, sl_size size) override
		{
			return (*base).read(buf, size);
		}

		sl_bool waitRead(sl_int32 timeout = -1) override
		{
			return (*base).waitRead(timeout);
		}

		void close() override
		{
			return (*base).close();
		}

	};

	template <class T>
	class SLIB_EXPORT Writer : public IWriter, public IClosable
	{
		PRIV_SLIB_DEFINE_IO_DEFAULT_MEMBERS(Writer)

	public:
		sl_reg write(const void* buf, sl_size size) override
		{
			return (*base).write(buf, size);
		}

		sl_bool waitWrite(sl_int32 timeout = -1) override
		{
			return (*base).waitWrite(timeout);
		}

		void close() override
		{
			return (*base).close();
		}

	};

	template <class T>
	class SLIB_EXPORT BlockReader : public IBlockReader, public IClosable
	{
		PRIV_SLIB_DEFINE_IO_DEFAULT_MEMBERS(BlockReader)

	public:
		sl_reg readAt(sl_uint64 offset, void* buf, sl_size size) override
		{
			return (*base).readAt(offset, buf, size);
		}

		sl_bool waitRead(sl_int32 timeout = -1) override
		{
			return (*base).waitRead(timeout);
		}

		void close() override
		{
			return (*base).close();
		}

	};

	template <class T>
	class SLIB_EXPORT BlockWriter : public IBlockWriter, public IClosable
	{
		PRIV_SLIB_DEFINE_IO_DEFAULT_MEMBERS(BlockWriter)

	public:
		sl_reg writeAt(sl_uint64 offset, const void* buf, sl_size size) override
		{
			return (*base).writeAt(offset, buf, size);
		}

		sl_bool waitWrite(sl_int32 timeout = -1) override
		{
			return (*base).waitWrite(timeout);
		}

		void close() override
		{
			return (*base).close();
		}

	};

	template <class T>
	class SLIB_EXPORT Stream : public IStream
	{
		PRIV_SLIB_DEFINE_IO_DEFAULT_MEMBERS(Stream)

	public:
		sl_reg read(void* buf, sl_size size) override
		{
			return (*base).read(buf, size);
		}

		sl_bool waitRead(sl_int32 timeout = -1) override
		{
			return (*base).waitRead(timeout);
		}

		sl_reg write(const void* buf, sl_size size) override
		{
			return (*base).write(buf, size);
		}

		sl_bool waitWrite(sl_int32 timeout = -1) override
		{
			return (*base).waitWrite(timeout);
		}

		void close() override
		{
			return (*base).close();
		}

	};

	class SLIB_EXPORT SeekableReaderBase : public IReader, public IBlockReader, public ISeekable, public IClosable
	{
	public:
		SLIB_DECLARE_SEEKABLE_READER_MEMBERS(, override)

	public:
		sl_bool waitRead(sl_int32 timeout = -1) override;

	};

	template <class T>
	class SLIB_EXPORT SeekableReader : public SeekableReaderBase
	{
		PRIV_SLIB_DEFINE_IO_DEFAULT_MEMBERS(SeekableReader)

	public:
		sl_reg read(void* buf, sl_size size) override
		{
			return (*base).read(buf, size);
		}

		sl_bool waitRead(sl_int32 timeout = -1) override
		{
			return (*base).waitRead(timeout);
		}

		void close() override
		{
			return (*base).close();
		}

		using ISizeProvider::getSize;
		sl_bool getSize(sl_uint64& outSize) override
		{
			return (*base).getSize(outSize);
		}

		using ISeekable::getPosition;
		sl_bool getPosition(sl_uint64& outPos) override
		{
			return (*base).getPosition(outPos);
		}

		using ISeekable::seek;
		sl_bool seek(sl_int64 offset, SeekPosition pos) override
		{
			return (*base).seek(offset, pos);
		}

	};

	class SLIB_EXPORT IOBase : public IStream, public IBlockReader, public IBlockWriter, public ISeekable, public IResizable
	{
	public:
		SLIB_DECLARE_SEEKABLE_READER_MEMBERS(,override)
		SLIB_DECLARE_SEEKABLE_WRITER_MEMBERS(,override)

	public:
		sl_bool waitRead(sl_int32 timeout = -1) override;

		sl_bool waitWrite(sl_int32 timeout = -1) override;

	};

	template <class T>
	class SLIB_EXPORT IO : public IOBase
	{
		PRIV_SLIB_DEFINE_IO_DEFAULT_MEMBERS(IO)

	public:
		sl_reg read(void* buf, sl_size size) override
		{
			return (*base).read(buf, size);
		}

		sl_bool waitRead(sl_int32 timeout = -1) override
		{
			return (*base).waitRead(timeout);
		}

		sl_reg write(const void* buf, sl_size size) override
		{
			return (*base).write(buf, size);
		}

		sl_bool waitWrite(sl_int32 timeout = -1) override
		{
			return (*base).waitWrite(timeout);
		}

		void close() override
		{
			return (*base).close();
		}

		using ISizeProvider::getSize;
		sl_bool getSize(sl_uint64& outSize) override
		{
			return (*base).getSize(outSize);
		}

		using ISeekable::getPosition;
		sl_bool getPosition(sl_uint64& outPos) override
		{
			return (*base).getPosition(outPos);
		}

		using ISeekable::seek;
		sl_bool seek(sl_int64 offset, SeekPosition pos) override
		{
			return (*base).seek(offset, pos);
		}

		sl_bool setSize(sl_uint64 size) override
		{
			return (*base).setSize(size);
		}

	};

}

#endif
