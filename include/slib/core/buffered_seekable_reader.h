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

#ifndef CHECKHEADER_SLIB_CORE_BUFFERED_SEEKABLE_READER
#define CHECKHEADER_SLIB_CORE_BUFFERED_SEEKABLE_READER

#include "io.h"
#include "buffered_reader.h"

namespace slib
{
	
	class SLIB_EXPORT BufferedSeekableReader : public IReader, public IBlockReader, public ISeekable, public IClosable
	{
	public:
		BufferedSeekableReader();

		~BufferedSeekableReader();

	public:
		sl_bool open(const Ptrx<IReader, ISeekable, IClosable>& reader, sl_size bufferSize = SLIB_BUFFERED_READER_DEFAULT_SIZE);

		sl_bool isOpened();

		sl_reg read(void*& buf);

		sl_reg read(void* buf, sl_size size) override;

		sl_bool waitRead(sl_int32 timeout = -1) override;

		using ISeekable::getPosition;
		sl_bool getPosition(sl_uint64& outPos) override;

		using ISeekable::getSize;
		sl_bool getSize(sl_uint64& outSize) override;

		sl_bool seek(sl_int64 offset, SeekPosition pos) override;

		void close() override;

	public:
		SLIB_DECLARE_SEEKABLE_READER_MEMBERS(,override)

	private:
		void _init(const Ptrx<IReader, ISeekable, IClosable>& reader, sl_uint64 size, const Memory& buf);

		sl_reg _readInBuf(void* buf, sl_size size);

		sl_bool _seekInternal(sl_uint64 pos);

		sl_reg _readInternal(sl_uint64 pos, void* buf, sl_size size);

		sl_reg _fillBuf(sl_uint64 pos, sl_size size);

		sl_reg _fillBuf(sl_uint64 pos);

		sl_reg _readFillingBuf(sl_uint64 pos, void* buf, sl_size size);

	private:
		Ref<Referable> m_ref;
		IReader* m_reader;
		ISeekable* m_seekable;
		IClosable* m_closable;

		sl_uint64 m_posInternal;
		sl_uint64 m_posCurrent;
		sl_uint64 m_sizeTotal;

		Memory m_buf;
		sl_uint8* m_dataBuf;
		sl_size m_sizeBuf;
		sl_size m_sizeRead;
		sl_uint64 m_posBuf;

	};

}

#endif
