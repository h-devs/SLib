/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_THREAD_SERVICE
#define CHECKHEADER_SLIB_CORE_THREAD_SERVICE

#include "thread.h"

namespace slib
{

	class SLIB_EXPORT ThreadService
	{
	public:
		ThreadService();

		~ThreadService();

	public:
		void release();

		sl_bool start();

		void stop();

		sl_bool isReleased();

		sl_bool isRunning();

		sl_bool isNotRunning();

	protected:
		Ref<Thread> m_serviceThread;
		sl_bool m_flagReleasedService;

		Mutex* m_serviceLock;
		Function<void()> m_onRunService;
		Function<Function<void()>()> m_onReleaseService;
	};

}

#endif
