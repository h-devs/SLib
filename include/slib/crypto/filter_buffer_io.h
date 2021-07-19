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

#include "../core/memory_buffer.h"
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
		/*
			returns
				>0: Size of written output
				SLIB_IO_ENDED, SLIB_IO_ERROR, SLIB_IO_WOULD_BLOCK

			returns SLIB_IO_ERROR when output size is zero
		*/
		DataFilterResult pass(IDataFilter* filter, const void* input, sl_size sizeInput, void* output, sl_size sizeOutput, sl_size& sizeOutputUsed);
		
		DataFilterResult pass(IDataFilter* filter, const void* input, sl_size size, IWriter* writer);

		DataFilterResult pass(IDataFilter* filter, IReader* reader, void* output, sl_size size, sl_size& sizeOutputUsed);

		DataFilterResult pass(IDataFilter* filter, IReader* reader, IWriter* writer);

		sl_size getInputBufferSize();

		sl_bool isFinishing();

		void setFinishing();

	protected:
		DataFilterResult _addInput(const void* data, sl_size size, DataFilterResult result);

		DataFilterResult _doPass(IDataFilter* filter, MemoryData& input, void*& output, sl_size& sizeOutput, sl_size& sizeOutputUsed);

		DataFilterResult _processWriteResult(MemoryData& slice, sl_reg nWrite);

		DataFilterResult _doPass(IDataFilter* filter, MemoryData& input, IWriter* writer);

		DataFilterResult _step(IDataFilter* filter, IReader* reader, void* output, sl_size size, sl_size& sizeUsed);

		DataFilterResult _step(IDataFilter* filter, IReader* reader, IWriter* writer);

	protected:
		MemoryBuffer m_bufInput;
		MemoryBuffer m_bufOutput;

		Memory m_chunk;
		char* m_chunkData;
		
		sl_bool m_flagFinishing;
		sl_bool m_flagFinished;

	};

}

#endif
