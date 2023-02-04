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

#include "async_config.h"

#if defined(ASYNC_USE_EPOLL)

#include "slib/io/async.h"

#include "slib/io/pipe_event.h"

#include <unistd.h>
#include <sys/epoll.h>
#include <sys/errno.h>

#if !defined(EPOLLRDHUP) || defined(SLIB_PLATFORM_IS_ANDROID)
#define EPOLL_LOW
#endif

namespace slib
{

	namespace {
		struct AsyncIoLoopHandle
		{
			int fdEpoll;
			Ref<PipeEvent> eventWake;
		};
	}

	void* AsyncIoLoop::_native_createHandle()
	{
		int fdEpoll;
#if defined(EPOLL_LOW)
		fdEpoll = epoll_create(1024);
#else
		fdEpoll = epoll_create1(0);
#endif
		if (fdEpoll >= 0) {
			AsyncIoLoopHandle* handle = new AsyncIoLoopHandle;
			if (handle) {
				handle->eventWake = PipeEvent::create();
				if (handle->eventWake.isNotNull()) {
					handle->fdEpoll = fdEpoll;
					// register wake event
					epoll_event ev;
					ev.data.ptr = sl_null;
					ev.events = EPOLLIN | EPOLLPRI | EPOLLET;
					if (!(epoll_ctl(fdEpoll, EPOLL_CTL_ADD, handle->eventWake->getReadPipeHandle(), &ev))) {
						return handle;
					}
				}
				delete handle;
			}
			::close(fdEpoll);
		}
		return 0;
	}

	void AsyncIoLoop::_native_closeHandle(void* _handle)
	{
		AsyncIoLoopHandle* handle = (AsyncIoLoopHandle*)_handle;
		::close(handle->fdEpoll);
		delete handle;
	}

	void AsyncIoLoop::_native_runLoop()
	{
		AsyncIoLoopHandle* handle = (AsyncIoLoopHandle*)m_handle;

		epoll_event waitEvents[ASYNC_MAX_WAIT_EVENT];

		while (m_flagRunning) {

			_stepBegin();

			int nEvents = epoll_wait(handle->fdEpoll, waitEvents, ASYNC_MAX_WAIT_EVENT, 5000);
			if (m_queueInstancesClosed.isNotEmpty()) {
				m_queueInstancesClosed.removeAll();
			}
			if (nEvents < 0) {
				int err = errno;
				if (err == EBADF || err == EFAULT || err == EINVAL) {
					//break;
				}
			}

			for (int i = 0; m_flagRunning && i < nEvents; i++) {
				epoll_event& ev = waitEvents[i];
				AsyncIoInstance* instance = (AsyncIoInstance*)(ev.data.ptr);
				if (instance) {
					if (!(instance->isClosing())) {
						AsyncIoInstance::EventDesc desc;
						desc.flagIn = sl_false;
						desc.flagOut = sl_false;
						desc.flagError = sl_false;
						int re = ev.events;
						if (re & (EPOLLIN | EPOLLPRI)) {
							desc.flagIn = sl_true;
						}
						if (re & (EPOLLOUT)) {
							desc.flagOut = sl_true;
						}
#if defined(EPOLL_LOW)
						if (re & (EPOLLERR | EPOLLHUP)) {
#else
						if (re & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
#endif
							desc.flagError = sl_true;
						}
						instance->onEvent(&desc);
					}
				} else {
					handle->eventWake->reset();
				}
			}

			if (m_flagRunning) {
				_stepEnd();
			}
		}

	}

	void AsyncIoLoop::_native_wake()
	{
		AsyncIoLoopHandle* handle = (AsyncIoLoopHandle*)m_handle;
		handle->eventWake->set();
	}

	sl_bool AsyncIoLoop::_native_attachInstance(AsyncIoInstance* instance, AsyncIoMode mode)
	{
		AsyncIoLoopHandle* handle = (AsyncIoLoopHandle*)m_handle;
		int hObject = (int)(instance->getHandle());
		epoll_event ev;
		ev.data.ptr = (void*)instance;

#if defined(EPOLL_LOW)
		ev.events = EPOLLET;
#else
		ev.events = EPOLLRDHUP | EPOLLET;
#endif
		switch (mode) {
			case AsyncIoMode::In:
				ev.events |= EPOLLIN | EPOLLPRI;
				break;
			case AsyncIoMode::Out:
				ev.events |= EPOLLOUT;
				break;
			case AsyncIoMode::InOut:
				ev.events |= EPOLLIN | EPOLLPRI | EPOLLOUT;
				break;
			default:
				return sl_true;
		}

		int ret = epoll_ctl(handle->fdEpoll, EPOLL_CTL_ADD, hObject, &ev);
		if (ret) {
			int err = errno;
			return sl_false;
		} else {
			return sl_true;
		}
	}

	void AsyncIoLoop::_native_detachInstance(AsyncIoInstance* instance)
	{
		AsyncIoLoopHandle* handle = (AsyncIoLoopHandle*)m_handle;
		int hObject = (int)(instance->getHandle());
		epoll_event ev;
		int ret = epoll_ctl(handle->fdEpoll, EPOLL_CTL_DEL, hObject, &ev);
		SLIB_UNUSED(ret);
	}

}

#endif
