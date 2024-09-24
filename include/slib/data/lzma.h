/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_DATA_LZMA
#define CHECKHEADER_SLIB_DATA_LZMA

#include "compress.h"

#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT LzmaParam
	{
	public:
		// Input
		sl_uint32 level; // Compression level: 0 <= level <= 9, Default: 5
		sl_uint32 dictSize; // Dictionary size: (1 << N) or (3 << N), 4 KB < dictSize <= 128MB/1GB (32/64Bit), Default: 1<<24
		sl_uint32 lc; // Number of literal context bits: 0 <= lc <= 8, Default: 3
		sl_uint32 lp; // Number of literal pos bits: 0 <= lp <= 4, Default: 0
		sl_uint32 pb; // Number of pos bits: 0 <= pb <= 4, Default: 2
		sl_uint32 fb; // Word size: 5 <= fb <= 273, Default: 32
		sl_bool flagWriteEndMark;
		sl_uint32 numThreads; // Number of thereads: 1 or 2, Default: 1

		// Output
		sl_uint8 props[5]; // Written properties

	public:
		LzmaParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(LzmaParam)

	};

	class SLIB_EXPORT LzmaDecompressor : public IDecompressor
	{
	public:
		LzmaDecompressor();

		~LzmaDecompressor();

	public:
		sl_bool isStarted();

		sl_bool start(const sl_uint8 props[5]);

		DataConvertResult pass32(const void* input, sl_uint32 sizeInputAvailable, sl_uint32& sizeInputPassed,
			void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed) override;

		DataConvertResult finish32(void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed) override;

	protected:
		sl_uint8 m_decoder[256]; // bigger than sizeof(CLzmaDec)
		sl_bool m_flagStarted;
	};

	class SLIB_EXPORT Lzma
	{
	public:
		static Memory compress(LzmaParam& param, const void* data, sl_size size);

		static Memory decompress(const sl_uint8 props[5], const void* data, sl_size size);

	};

}

#endif
