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

#ifndef CHECKHEADER_SLIB_CORE_MEMORY_BUFFER
#define CHECKHEADER_SLIB_CORE_MEMORY_BUFFER

#include "memory.h"
#include "queue.h"

namespace slib
{

	// MemoryBuffer is not thread-safe
	class SLIB_EXPORT MemoryBuffer
	{
	public:
		MemoryBuffer();

		~MemoryBuffer();
	
	public:
		sl_size getSize() const;

		sl_bool add(const MemoryData& mem);

		sl_bool add(MemoryData&& mem);

		template <class REF>
		sl_bool add(const void* buf, sl_size size, REF&& ref)
		{
			return add(MemoryData(buf, size, Forward<REF>(ref)));
		}

		sl_bool add(const Memory& mem);

		sl_bool addStatic(const void* buf, sl_size size);
	
		template <sl_size N>
		sl_bool addStatic(const char (&buf)[N])
		{
			return addStatic(buf, N - 1);
		}

		sl_bool pop(MemoryData& data);

		sl_bool pushFront(const MemoryData& data);
	
		void link(MemoryBuffer& buf);
	
		void clear();
	
		Memory merge() const;

	protected:
		LinkedQueue<MemoryData> m_queue;
		sl_size m_size;

	};

}

#endif
