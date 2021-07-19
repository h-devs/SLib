/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CRYPTO_CRC32C
#define CHECKHEADER_SLIB_CRYPTO_CRC32C

#include "definition.h"

namespace slib
{

	class Memory;

	class SLIB_EXPORT Crc32
	{
	public:
		static sl_uint32 extend(sl_uint32 crc, const void* data, sl_size size);

		static sl_uint32 get(const void* data, sl_size size);

		static sl_uint32 extend(sl_uint32 crc, const Memory& mem);

		static sl_uint32 get(const Memory& mem);

	};

	class SLIB_EXPORT Crc32c
	{
	public:
		static sl_uint32 extend(sl_uint32 crc, const void* data, sl_size size);

		static sl_uint32 get(const void* data, sl_size size);

		static sl_uint32 extend(sl_uint32 crc, const Memory& mem);

		static sl_uint32 get(const Memory& mem);

	};

}

#endif
