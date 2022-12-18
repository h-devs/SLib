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

#ifndef CHECKHEADER_SLIB_DATA_BROTLI
#define CHECKHEADER_SLIB_DATA_BROTLI

#include "compress.h"

#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT BrotliCompressor : public ICompressor
	{
	public:
		BrotliCompressor();

		~BrotliCompressor();

	public:
		sl_bool isStarted();

		// level: 0-11
		sl_bool start(sl_int32 level = 11, sl_bool flagText = sl_false);

		DataConvertResult pass(const void* input, sl_size sizeInputAvailable, sl_size& sizeInputPassed,
			void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed) override;

		DataConvertResult finish(void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed) override;

	protected:
		void* m_stream;
	};

	class SLIB_EXPORT BrotliDecompressor : public IDecompressor
	{
	public:
		BrotliDecompressor();

		~BrotliDecompressor();

	public:
		sl_bool isStarted();

		sl_bool start();

		DataConvertResult pass(const void* input, sl_size sizeInputAvailable, sl_size& sizeInputPassed,
			void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed) override;

		DataConvertResult finish(void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed) override;

	protected:
		void* m_stream;
	};

	class SLIB_EXPORT Brotli
	{
	public:
		// level: 0-11
		static Memory compress(const void* data, sl_size size, sl_int32 level = 11, sl_bool flagText = sl_false);

		static Memory decompress(const void* data, sl_size size);

	};

}

#endif
