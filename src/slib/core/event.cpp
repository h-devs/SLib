/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/core/event.h"

#include "slib/core/thread.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)
#include "slib/core/win32/platform.h"
#else
#include "slib/core/posix/event.h"
#include <time.h>
#include <sys/time.h>
#endif

namespace slib
{

	namespace priv
	{
		namespace ev
		{

			static HEvent CreateEventHandle(sl_bool flagAutoReset) noexcept
			{
#if defined(SLIB_PLATFORM_IS_WIN32)
				return CreateEventW(NULL, flagAutoReset ? FALSE : TRUE, FALSE, NULL);
#elif defined(SLIB_PLATFORM_IS_WINDOWS)
				return CreateEventEx(NULL, NULL, flagAutoReset ? CREATE_EVENT_INITIAL_SET : CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
#else
				return new posix::Event(flagAutoReset);
#endif
			}

			static void CloseEventHandle(HEvent handle) noexcept
			{
#if defined(SLIB_PLATFORM_IS_WINDOWS)
				CloseHandle((HANDLE)handle);
#else
				delete handle;
#endif
			}

		}
	}

	using namespace priv::ev;

	sl_bool IEvent::wait(sl_int32 timeout)
	{
		Ref<Thread> thread = Thread::getCurrent();
		if (thread.isNotNull()) {
			if (thread->isStopping()) {
				return sl_false;
			}
			thread->setWaitingEvent(this);
		}
		sl_bool ret = doWait(timeout);
		if (thread.isNotNull()) {
			thread->clearWaitingEvent();
		}
		return ret;
	}

	SLIB_DEFINE_NULLABLE_HANDLE_CONTAINER_MEMBERS(Event, HEvent, m_handle, CloseEventHandle)
	SLIB_DEFINE_ATOMIC_NULLABLE_HANDLE_CONTAINER_MEMBERS(Event, HEvent, m_handle, CloseEventHandle)

	Event::Event(sl_bool flagAutoReset) noexcept: m_handle(CreateEventHandle(flagAutoReset))
	{
	}

	Event Event::create(sl_bool flagAutoReset) noexcept
	{
		return CreateEventHandle(flagAutoReset);
	}

	void Event::set()
	{
		if (!m_handle) {
			return;
		}
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		SetEvent((HANDLE)m_handle);
#else
		m_handle->set();
#endif
	}

	void Event::reset()
	{
		if (!m_handle) {
			return;
		}
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		ResetEvent((HANDLE)m_handle);
#else
		m_handle->reset();
#endif
	}

	sl_bool Event::doWait(sl_int32 timeout)
	{
		if (!m_handle) {
			return sl_false;
		}
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		DWORD dwRet = WaitForSingleObjectEx(m_handle, timeout >= 0 ? timeout : INFINITE, TRUE);
		return dwRet != WAIT_TIMEOUT;
#else
		return m_handle->wait(timeout);
#endif
	}

#if defined(SLIB_PLATFORM_IS_WINDOWS)
	
	Event Win32::createEvent(HANDLE hEvent)
	{
		return hEvent;
	}

	HANDLE Win32::getEventHandle(const Event& ev)
	{
		return ev.get();
	}

#else

	namespace posix
	{

		Event::Event(sl_bool flagAutoReset) noexcept
		{
			pthread_cond_init(&cond, sl_null);
			pthread_mutex_init(&mutex, sl_null);
			flagAutoReset = flagAutoReset;
			signal = sl_false;
		}

		Event::~Event() noexcept
		{
			pthread_cond_destroy(&cond);
			pthread_mutex_destroy(&mutex);
		}

		void Event::set() noexcept
		{
			pthread_mutex_lock(&mutex);
			signal = sl_true;
			if (flagAutoReset) {
				pthread_cond_signal(&cond);
			} else {
				pthread_cond_broadcast(&cond);
			}
			pthread_mutex_unlock(&mutex);
		}

		void Event::reset() noexcept
		{
			pthread_mutex_lock(&mutex);
			signal = sl_false;
			pthread_mutex_unlock(&mutex);
		}

		sl_bool Event::wait(sl_int32 timeout) noexcept
		{
			pthread_mutex_lock(&mutex);

			struct timeval now;
			struct timespec to;
			sl_int32 _t = timeout;
			if (timeout < 0) {
				_t = 0;
			}
			{
				gettimeofday(&now, NULL);
				long ms = (now.tv_usec / 1000) + _t;
				long s = now.tv_sec + (ms / 1000);
				ms = ms % 1000;
				to.tv_sec = s;
				to.tv_nsec = ms * 1000000;
			}

			sl_bool ret = sl_true;
			while (!signal) {
				if (Thread::isStoppingCurrent()) {
					break;
				}
				if (timeout >= 0) {
					if (pthread_cond_timedwait(&cond, &mutex, &to)) {
						ret = sl_false;
						break;
					}
				} else {
#ifdef SLIB_PLATFORM_IS_BLACKBERRY
					_t = 10000;
					{
						gettimeofday(&now, NULL);
						long ms = (now.tv_usec / 1000) + _t;
						long s = now.tv_sec + (ms / 1000);
						ms = ms % 1000;
						to.tv_sec = s;
						to.tv_nsec = ms * 1000000;
					}
					pthread_cond_timedwait(&cond, &mutex, &to);
#else
					pthread_cond_wait(&cond, &mutex);
#endif
				}
			}
			if (flagAutoReset) {
				signal = sl_false;
			}

			pthread_mutex_unlock(&mutex);
			return ret;
		}

	}

#endif

}
