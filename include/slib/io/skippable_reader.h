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

#ifndef CHECKHEADER_SLIB_IO_SKIPPABLE_READER
#define CHECKHEADER_SLIB_IO_SKIPPABLE_READER

#include "io.h"

#include "../core/ptrx.h"

namespace slib
{

	class SLIB_EXPORT SkippableReader : public IReader
	{
	public:
		SkippableReader();

		SkippableReader(const Ptrx<IReader, ISeekable>& reader);

		~SkippableReader();

	public:
		sl_bool setReader(const Ptrx<IReader, ISeekable>& reader);

		IReader* getReader()
		{
			return m_reader;
		}

		ISeekable* getSeekable()
		{
			return m_seekable;
		}

	public:
		operator IReader*()
		{
			return m_reader;
		}

		operator ISeekable*()
		{
			return m_seekable;
		}

	public:
		sl_reg read(void* buf, sl_size size) override;

		sl_int32 read32(void* buf, sl_uint32 size) override;

		sl_uint64 skip(sl_uint64 size);

		sl_uint64 getPosition();

	private:
		Ref<Referable> m_ref;
		IReader* m_reader;
		ISeekable* m_seekable;

		sl_uint64 m_pos;

	};

}

#endif
