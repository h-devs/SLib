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

#ifndef CHECKHEADER_SLIB_CRYPTO_FILTER_BUFFER_IO
#define CHECKHEADER_SLIB_CRYPTO_FILTER_BUFFER_IO

#include "data_filter.h"

#include "../core/memory.h"
#include "../core/io.h"

namespace slib
{

	// Not thread-safe
	class SLIB_EXPORT FilterBufferIO
	{
	public:
		FilterBufferIO();

		~FilterBufferIO();
		
	public:
		DataFilterResult pass(IDataFilter* filter, const void* input, sl_size size, sl_size& sizeInputPassed, IWriter* writer);

		DataFilterResult pass(IDataFilter* filter, IReader* reader, void* output, sl_size size, sl_size& sizeOutputUsed);

		DataFilterResult pass(IDataFilter* filter, IReader* reader, IWriter* writer);

		sl_bool isFinishing();

		void setFinishing();

	protected:
		DataFilterResult _processWriteResult(sl_reg nWrite);

		sl_bool _resetInputBuffer(IDataFilter* filter);

		sl_bool _resetOutputBuffer(IDataFilter* filter);

	protected:
		Memory m_bufInput;
		void* m_dataInput;
		sl_size m_sizeInput;
		Memory m_bufOutput;
		void* m_dataOutput;
		sl_size m_sizeOutput;

		sl_bool m_flagFinishing;
		sl_bool m_flagFinished;

	};

}

#endif
