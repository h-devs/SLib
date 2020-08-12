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

#ifndef CHECKHEADER_SLIB_CORE_IO_UTIL
#define CHECKHEADER_SLIB_CORE_IO_UTIL

#include "definition.h"

#include "io.h"
#include "ptrx.h"

namespace slib
{

	class SLIB_EXPORT IOUtil
	{
	public:
		static sl_uint64 skip(const Pointerx<IReader, ISeekable>& reader, sl_uint64 size);

		static sl_int64 find(const Pointer<IReader, ISeekable>& reader, const void* pattern, sl_size nPattern, sl_int64 startPosition = 0, sl_int64 endPosition = -1);

		static sl_int64 findBackward(const Pointer<IReader, ISeekable>& reader, const void* pattern, sl_size nPattern, sl_int64 startPosition = -1, sl_int64 endPosition = -1);

	};
	
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

	private:
		Ref<Referable> m_ref;
		IReader* m_reader;
		ISeekable* m_seekable;

	};

}

#endif
