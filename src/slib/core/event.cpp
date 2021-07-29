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

	SLIB_DEFINE_ROOT_OBJECT(Event)
	
	Event::Event()
	{
	}

	Event::~Event()
	{
	}

	Ref<Event> Event::create(sl_bool flagAutoReset)
	{
		return Ref<Event>::from(GenericEvent::create(flagAutoReset));
	}

	sl_bool Event::wait(sl_int32 timeout)
	{
		Thread* thread = Thread::getCurrent();
		if (thread) {
			if (thread->isStopping()) {
				return sl_false;
			}
			thread->setWaitingEvent(this);
		}
		sl_bool ret = doWait(timeout);
		if (thread) {
			thread->clearWaitingEvent();
		}
		return ret;
	}


	GenericEvent::GenericEvent()
	{
		m_handle = sl_null;
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		m_flagCloseOnRelease = sl_true;
#endif
	}

	GenericEvent::~GenericEvent()
	{
		if (m_handle) {
#if defined(SLIB_PLATFORM_IS_WINDOWS)
			if (!m_flagCloseOnRelease) {
				return;
			}
#endif
			closeHandle(m_handle);
		}
	}

	Ref<GenericEvent> GenericEvent::create(sl_bool flagAutoReset)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		HANDLE handle = CreateEventW(NULL, flagAutoReset ? FALSE : TRUE, FALSE, NULL);
#elif defined(SLIB_PLATFORM_IS_WINDOWS)
		HANDLE handle = CreateEventEx(NULL, NULL, flagAutoReset ? CREATE_EVENT_INITIAL_SET : CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
#else
		HEvent handle = new posix::Event(flagAutoReset);
#endif
		return create(handle);
	}

#if defined(SLIB_PLATFORM_IS_WINDOWS)
	Ref<GenericEvent> GenericEvent::create(HEvent handle, sl_bool flagCloseOnRelease)
#else
	Ref<GenericEvent> GenericEvent::create(HEvent handle)
#endif
	{
		if (handle) {
			Ref<GenericEvent> ret = new GenericEvent;
			if (ret.isNotNull()) {
				ret->m_handle = handle;
#if defined(SLIB_PLATFORM_IS_WINDOWS)
				ret->m_flagCloseOnRelease = flagCloseOnRelease;
#endif
				return ret;
			}
#if defined(SLIB_PLATFORM_IS_WINDOWS)
			if (flagCloseOnRelease) {
				closeHandle(handle);
			}
#endif
		}
		return sl_null;
	}

	void GenericEvent::closeHandle(HEvent handle)
	{
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		CloseHandle(handle);
#else
		delete handle;
#endif
	}

	void GenericEvent::set()
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

	void GenericEvent::reset()
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

	sl_bool GenericEvent::doWait(sl_int32 timeout)
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
	
	Ref<Event> Win32::createEvent(HANDLE hEvent, sl_bool flagCloseOnRelease)
	{
		return Ref<Event>::from(GenericEvent::create(hEvent, flagCloseOnRelease));
	}

	HANDLE Win32::getEventHandle(Event* ev)
	{
		if (ev) {
			return ((GenericEvent*)ev)->getHandle();
		}
		return sl_null;
	}

#else

	namespace posix
	{

		Event::Event(sl_bool _flagAutoReset) noexcept
		{
			pthread_cond_init(&cond, sl_null);
			pthread_mutex_init(&mutex, sl_null);
			flagAutoReset = _flagAutoReset;
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
