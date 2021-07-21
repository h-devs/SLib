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

#ifndef CHECKHEADER_SLIB_CRYPTO_ZSTD
#define CHECKHEADER_SLIB_CRYPTO_ZSTD

#include "compress.h"

#include "../core/string.h"

/*
	Zstd supports regular compression levels from 1 up to `Zstd::getMaximumLevel()`,
	which is currently 22. Levels >= 20, labeled `--ultra`, should be used with
	caution, as they require more memory. The library also offers negative
	compression levels, which extend the range of speed vs. ratio preferences.
	The lower the level, the faster the speed (at the cost of compression).
*/

namespace slib
{
	
	class SLIB_EXPORT ZstdCompressor : public ICompressor
	{
	public:
		ZstdCompressor();

		~ZstdCompressor();
	
	public:
		sl_bool isStarted();

		sl_bool start(sl_int32 level = 3);

		DataFilterResult pass(const void* input, sl_size sizeInputAvailable, sl_size& sizeInputPassed,
			void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed) override;

		DataFilterResult finish(void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed) override;

		sl_size getRecommendedInputSize() override;

		sl_size getRecommendedOutputSize() override;

	protected:
		void* m_stream;
	};
	
	class SLIB_EXPORT ZstdDecompressor : public IDecompressor
	{
	public:
		ZstdDecompressor();

		~ZstdDecompressor();

	public:
		sl_bool isStarted();

		sl_bool start();

		DataFilterResult pass(const void* input, sl_size sizeInputAvailable, sl_size& sizeInputPassed,
			void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed) override;

		DataFilterResult finish(void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed) override;

		sl_size getRecommendedInputSize() override;

		sl_size getRecommendedOutputSize() override;

	protected:
		void* m_stream;
	};

	class SLIB_EXPORT Zstd
	{
	public:
		static sl_int32 getMaximumLevel();

		static sl_int32 getMinimumLevel();

		static Memory compress(const void* data, sl_size size, sl_int32 level = 3);

		static Memory decompress(const void* data, sl_size size);
	
	};

}

#endif
