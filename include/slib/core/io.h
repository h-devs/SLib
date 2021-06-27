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

#ifndef CHECKHEADER_SLIB_CORE_IO
#define CHECKHEADER_SLIB_CORE_IO

#include "io/def.h"

namespace slib
{
	
	class SLIB_EXPORT IReader
	{
	public:
		virtual sl_reg read(void* buf, sl_size size);

		virtual sl_int32 read32(void* buf, sl_uint32 size);
	
	public:
		SLIB_DECLARE_IREADER_MEMBERS
		
	};
	
	
	class SLIB_EXPORT IWriter
	{
	public:
		virtual sl_reg write(const void* buf, sl_size size);

		virtual sl_int32 write32(const void* buf, sl_uint32 size);
	
	public:
		SLIB_DECLARE_IWRITER_MEMBERS
		
	};

	class SLIB_EXPORT IBlockReader
	{
	public:
		virtual sl_reg readAt(sl_uint64 offset, void* buf, sl_size size);

		virtual sl_int32 readAt32(sl_uint64 offset, void* buf, sl_uint32 size);

		virtual sl_reg readFullyAt(sl_uint64 offset, void* buf, sl_size size);

	};

	class SLIB_EXPORT IBlockWriter
	{
	public:
		virtual sl_reg writeAt(sl_uint64 offset, const void* buf, sl_size size);

		virtual sl_int32 writeAt32(sl_uint64 offset, const void* buf, sl_uint32 size);

		virtual sl_reg writeFullyAt(sl_uint64 offset, const void* buf, sl_size size);

	};

	class SLIB_EXPORT ISize
	{
	public:
		virtual sl_bool getSize(sl_uint64& outSize) = 0;

	public:
		SLIB_DECLARE_ISIZE_MEMBERS

	};

	class SLIB_EXPORT IResizable
	{
	public:
		virtual sl_bool setSize(sl_uint64 size) = 0;
	};

	class SLIB_EXPORT ISeekable : public ISize
	{
	public:
		virtual sl_bool getPosition(sl_uint64& outPos) = 0;

		virtual sl_bool seek(sl_int64 offset, SeekPosition pos) = 0;

		virtual sl_bool isEnd(sl_bool& outFlag);

	public:
		SLIB_DECLARE_ISEEKABLE_MEMBERS
	
	};

	class SLIB_EXPORT IClosable
	{
	public:
		virtual void close() = 0;
	};

	class SLIB_EXPORT IStream : public IReader, public IWriter, public IClosable
	{
	};
	
	class SLIB_EXPORT IO : public IStream, public IBlockReader, public IBlockWriter, public ISeekable, public IResizable
	{
	public:
		SLIB_DECLARE_SEEKABLE_READER_MEMBERS(override)
		SLIB_DECLARE_SEEKABLE_WRITER_MEMBERS(override)
	};
	
}

#endif
