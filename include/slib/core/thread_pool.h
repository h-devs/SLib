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

#ifndef CHECKHEADER_SLIB_CORE_THREAD_POOL
#define CHECKHEADER_SLIB_CORE_THREAD_POOL

#include "thread.h"
#include "dispatch.h"
#include "queue.h"

namespace slib
{

	class SLIB_EXPORT ThreadPool : public Dispatcher
	{
		SLIB_DECLARE_OBJECT

	private:
		ThreadPool();

		~ThreadPool();

	public:
		static Ref<ThreadPool> create(sl_uint32 minThreads = 0, sl_uint32 maxThreads = 30);
	
	public:
		sl_uint32 getMinimumThreadCount();

		void setMinimumThreadCount(sl_uint32 n);

		
		sl_uint32 getMaximumThreadCount();

		void setMaximumThreadCount(sl_uint32 n);

		
		sl_uint32 getThreadStackSize();

		void setThreadStackSize(sl_uint32 n);


		void release();

		sl_bool isRunning();

		sl_uint32 getThreadCount();
	
		sl_bool addTask(const Function<void()>& task);

		sl_bool dispatch(const Function<void()>& callback, sl_uint64 delayMillis = 0) override;
	
	protected:
		void onRunWorker();
	
	protected:
		CList< Ref<Thread> > m_threadWorkers;
		LinkedQueue< Ref<Thread> > m_threadSleeping;
		LinkedQueue< Function<void()> > m_tasks;

		sl_uint32 m_minimumThreadCount;
		sl_uint32 m_maximumThreadCount;
		sl_uint32 m_threadStackSize;

		sl_bool m_flagRunning;

	};

}

#endif
