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

#ifndef CHECKHEADER_SLIB_CORE_MEMORY_IO
#define CHECKHEADER_SLIB_CORE_MEMORY_IO

#include "io.h"
#include "memory.h"

namespace slib
{
	
	// MemoryIO is not thread-safe
	class SLIB_EXPORT MemoryIO : public IOBase
	{
	public:
		MemoryIO();
		
		MemoryIO(sl_size size);

		MemoryIO(void* data, sl_size size);

		MemoryIO(const Memory& mem);
	
		~MemoryIO();
	
	public:
		void initialize();

		void initialize(sl_size size);

		void initialize(void* data, sl_size size);

		void initialize(const Memory& mem);

		void close() override;
	
		sl_reg read(void* buf, sl_size size) override;
	
		sl_reg write(const void* buf, sl_size size) override;

		sl_bool getPosition(sl_uint64& outPos) override;

		sl_bool getSize(sl_uint64& outSize) override;

		sl_bool seek(sl_int64 offset, SeekPosition pos = SeekPosition::Current) override;
	
		sl_bool setSize(sl_uint64 size) override;

		sl_size getPosition();

		sl_size getSize();

		sl_uint8* getBuffer();

		sl_bool isResizable();

		sl_bool setResizable(sl_bool flag);

		Memory getData();

		sl_int64 find(const void* pattern, sl_size nPattern, sl_int64 startPosition = 0, sl_int64 endPosition = -1);

		sl_int64 findBackward(const void* pattern, sl_size nPattern, sl_int64 startPosition = -1, sl_int64 endPosition = -1);

	protected:
		void _initialize();

		void _initialize(sl_size size);

		void _initialize(void* data, sl_size size);

		void _initialize(const Memory& mem);

		sl_bool _growCapacity(sl_size size);

	protected:
		void* m_buf;
		sl_size m_size;
		sl_size m_offset;
		sl_bool m_flagResizable;
		Memory m_data;
	
	};

}

#endif
