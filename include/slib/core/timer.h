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

#ifndef CHECKHEADER_SLIB_CORE_TIMER
#define CHECKHEADER_SLIB_CORE_TIMER

#include "object.h"
#include "function.h"

namespace slib
{

	class DispatchLoop;
	class Dispatcher;

	class SLIB_EXPORT Timer : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		Timer();

		~Timer();

	public:
		static Ref<Timer> create(const Function<void(Timer*)>& task, sl_uint64 interval_ms);

		static Ref<Timer> start(const Function<void(Timer*)>& task, sl_uint64 interval_ms);

		static Ref<Timer> createWithLoop(const Ref<DispatchLoop>& loop, const Function<void(Timer*)>& task, sl_uint64 interval_ms);

		static Ref<Timer> startWithLoop(const Ref<DispatchLoop>& loop, const Function<void(Timer*)>& task, sl_uint64 interval_ms);

		static Ref<Timer> createWithDispatcher(const Ref<Dispatcher>& dispatcher, const Function<void(Timer*)>& task, sl_uint64 interval_ms);

		static Ref<Timer> startWithDispatcher(const Ref<Dispatcher>& dispatcher, const Function<void(Timer*)>& task, sl_uint64 interval_ms);

	public:
		sl_uint64 getLastRunTime();

		void setLastRunTime(sl_uint64 time);


		sl_uint32 getMaxConcurrentThread();

		void setMaxConcurrentThread(sl_uint32 n);


		void start();

		void stop();

		sl_bool isStarted();

		Function<void(Timer*)> getTask();

		sl_uint64 getInterval();

		void run();

		void stopAndWait();

	protected:
		void _runFromDispatcher();

		static Ref<Timer> _createWithLoop(const Ref<DispatchLoop>& loop, const Function<void(Timer*)>& task, sl_uint64 interval_ms);

	protected:
		sl_bool m_flagStarted;
		Function<void(Timer*)> m_task;
		sl_uint64 m_interval;
		volatile sl_int32 m_nCountRun;

		sl_uint64 m_lastRunTime;
		sl_uint32 m_maxConcurrentThread;

		Ref<Dispatcher> m_dispatcher;
		WeakRef<DispatchLoop> m_loop;

		sl_bool m_flagDispatched;

	};

}

#endif
