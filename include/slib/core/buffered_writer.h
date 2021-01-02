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

#ifndef CHECKHEADER_SLIB_CORE_BUFFERED_WRITER
#define CHECKHEADER_SLIB_CORE_BUFFERED_WRITER

#include "definition.h"

#include "io.h"
#include "ptrx.h"

#define SLIB_BUFFERED_WRITER_DEFAULT_SIZE 8192

namespace slib
{
	
	class SLIB_EXPORT BufferedWriter : public Object, public IWriter, public IClosable
	{
		SLIB_DECLARE_OBJECT

	protected:
		BufferedWriter();

		~BufferedWriter();

	public:
		static Ref<BufferedWriter> create(const Ptrx<IWriter, IClosable>& reader, sl_size bufferSize = SLIB_BUFFERED_WRITER_DEFAULT_SIZE);

	public:
		sl_reg write(const void* buf, sl_size size) override;

		void close() override;

	protected:
		void _init(const Ptrx<IWriter, IClosable>& reader, const Memory& buf);

	protected:
		Ref<Referable> m_ref;
		IWriter* m_writer;
		IClosable* m_closable;

		sl_size m_sizeWrite;

		Memory m_buf;
		sl_uint8* m_dataBuf;
		sl_size m_sizeBuf;

	};

}

#endif
