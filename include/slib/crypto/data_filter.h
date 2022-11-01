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

#ifndef CHECKHEADER_SLIB_CRYPTO_DATA_FILTER
#define CHECKHEADER_SLIB_CRYPTO_DATA_FILTER

#include "definition.h"

namespace slib
{

	class Memory;
	class MemoryBuffer;

	enum class DataFilterResult
	{
		Finished = 0,
		Continue = 1,
		Error = 2,
		WouldBlock = 3
	};

	// always fill the output sizes
	class SLIB_EXPORT IDataFilter
	{
	public:
		// returns true on success, false on error
		virtual DataFilterResult pass(const void* input, sl_size sizeInputAvailable, sl_size& sizeInputPassed,
			void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed);

		virtual DataFilterResult finish(void* output, sl_size sizeOutputAvailable, sl_size& sizeOutputUsed);

		virtual DataFilterResult pass32(const void* input, sl_uint32 sizeInputAvailable, sl_uint32& sizeInputPassed,
			void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed);

		virtual DataFilterResult finish32(void* output, sl_uint32 sizeOutputAvailable, sl_uint32& sizeOutputUsed);

		virtual sl_size getRecommendedInputSize();

		virtual sl_size getRecommendedOutputSize();

		DataFilterResult pass(const void* input, sl_size size, MemoryBuffer& output);

		Memory pass(const void* input, sl_size size);

		DataFilterResult passAndFinish(const void* input, sl_size size, MemoryBuffer& output);

		Memory passAndFinish(const void* input, sl_size size);

	};

}

#endif
