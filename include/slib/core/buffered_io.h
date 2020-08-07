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

#ifndef CHECKHEADER_SLIB_CORE_BUFFERED_IO
#define CHECKHEADER_SLIB_CORE_BUFFERED_IO

#include "definition.h"

#include "io.h"
#include "ptrx.h"

#define SLIB_BUFFERED_IO_DEFAULT_SIZE 8192

namespace slib
{
	
	class SLIB_EXPORT BufferedReader : public Object, public IReader, public IClosable
	{
		SLIB_DECLARE_OBJECT

	private:
		BufferedReader();

		~BufferedReader();

	public:
		static Ref<BufferedReader> create(const Ptrx<IReader, IClosable>& reader, sl_size bufferSize = SLIB_BUFFERED_IO_DEFAULT_SIZE);

	public:
		sl_reg read(void* buf, sl_size size) override;

		void close() override;

	private:
		void _init(const Ptrx<IReader, IClosable>& reader, const Memory& buf);

	private:
		Ref<Referable> m_ref;
		IReader* m_reader;
		IClosable* m_closable;
		sl_size m_pos;
		sl_size m_count;
		Memory m_buf;
		sl_uint8* m_bufData;
		sl_size m_bufSize;

	};

}

#endif
