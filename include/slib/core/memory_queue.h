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

#ifndef CHECKHEADER_SLIB_CORE_MEMORY_QUEUE
#define CHECKHEADER_SLIB_CORE_MEMORY_QUEUE

#include "memory.h"
#include "queue.h"

namespace slib
{
	
	class SLIB_EXPORT MemoryQueue : public Lockable
	{
	public:
		MemoryQueue();

		~MemoryQueue();
	
	public:
		sl_size getSize() const;
		
		sl_bool add_NoLock(const MemoryData& mem);
		
		sl_bool add(const MemoryData& mem);
		
		sl_bool add_NoLock(const Memory& mem);
		
		sl_bool add(const Memory& mem);
		
		sl_bool addStatic_NoLock(const void* buf, sl_size size);

		template <sl_size N>
		sl_bool addStatic_NoLock(const char (&buf)[N])
		{
			return addStatic_NoLock(buf, N - 1);
		}

		sl_bool addStatic(const void* buf, sl_size size);
		
		template <sl_size N>
		sl_bool addStatic(const char (&buf)[N])
		{
			return addStatic(buf, N - 1);
		}

		void link_NoLock(MemoryQueue& buf);
		
		void link(MemoryQueue& buf);
		
		void clear_NoLock();
		
		void clear();

		sl_bool pop_NoLock(MemoryData& data);
		
		sl_bool pop(MemoryData& data);
		
		sl_size pop_NoLock(void* buf, sl_size size);
	
		sl_size pop(void* buf, sl_size size);
		
		Memory merge_NoLock() const;
	
		Memory merge() const;
	
	private:
		LinkedQueue<MemoryData> m_queue;
		sl_size m_size;
		MemoryData m_memCurrent;
		sl_size m_posCurrent;

	};

}

#endif
