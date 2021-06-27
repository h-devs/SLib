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

#include "../io.h"
#include "../ptrx.h"

namespace slib
{

	class SLIB_EXPORT IOUtil
	{
	public:
		static sl_uint64 skip(const Pointerx<IReader, ISeekable>& reader, sl_uint64 size);

		static sl_int64 find(const Pointer<IReader, ISeekable>& reader, const void* pattern, sl_size nPattern, sl_int64 startPosition = 0, sl_uint64 sizeFind = SLIB_UINT64_MAX);

		static sl_int64 findBackward(const Pointer<IReader, ISeekable>& reader, const void* pattern, sl_size nPattern, sl_int64 startPosition = -1, sl_uint64 sizeFind = SLIB_UINT64_MAX);

	};
	
}

#endif
