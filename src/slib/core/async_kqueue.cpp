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

#include "async_config.h"

#if defined(ASYNC_USE_KQUEUE)

#include "slib/core/async.h"
#include "slib/core/pipe_event.h"
#include "slib/core/system.h"

#include <sys/types.h>
#include <sys/event.h>
#include <unistd.h>

namespace slib
{

	namespace priv
	{
		namespace async_kqueue
		{
			struct AsyncIoLoopHandle
			{
				int kq;
				PipeEvent eventWake;
			};
		}
	}
	
	using namespace priv::async_kqueue;
	
	void* AsyncIoLoop::_native_createHandle()
	{
		int kq;
		kq = ::kqueue();
		if (kq != -1) {
			AsyncIoLoopHandle* handle = new AsyncIoLoopHandle;
			if (handle) {
				if (handle->eventWake.isOpened()) {
					handle->kq = kq;
					// register wake event
					struct kevent ke;
					EV_SET(&ke, (int)(handle->eventWake.getReadPipeHandle()), EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, sl_null);
					if (-1 != ::kevent(kq, &ke, 1, sl_null, 0, sl_null)) {
						return handle;
					}
				}
				delete handle;
			}
			::close(kq);
		}
		return sl_null;
	}

	void AsyncIoLoop::_native_closeHandle(void* _handle)
	{
		AsyncIoLoopHandle* handle = (AsyncIoLoopHandle*)_handle;
		::close(handle->kq);
		delete handle;
	}

	void AsyncIoLoop::_native_runLoop()
	{
		AsyncIoLoopHandle* handle = (AsyncIoLoopHandle*)m_handle;

		struct kevent waitEvents[ASYNC_MAX_WAIT_EVENT];

		while (m_flagRunning) {

			_stepBegin();

			timespec timeout;
			timeout.tv_nsec = 0;
			timeout.tv_sec = 5;
			int nEvents = ::kevent(handle->kq, sl_null, 0, waitEvents, ASYNC_MAX_WAIT_EVENT, &timeout);
			if (m_queueInstancesClosed.isNotEmpty()) {
				m_queueInstancesClosed.removeAll();
			}

			for (int i = 0; m_flagRunning && i < nEvents; i++) {
				struct kevent& ev = waitEvents[i];
				AsyncIoInstance* instance = (AsyncIoInstance*)(ev.udata);
				if (instance) {
					if (!(instance->isClosing())) {
						AsyncIoInstance::EventDesc desc;
						desc.flagIn = sl_false;
						desc.flagOut = sl_false;
						desc.flagError = sl_false;
						int re = ev.filter;
						if (re == EVFILT_READ) {
							desc.flagIn = sl_true;
						}
						if (re == EVFILT_WRITE) {
							desc.flagOut = sl_true;
						}
						int flags = ev.flags;
						if (flags & (EV_EOF | EV_ERROR)) {
							desc.flagError = sl_true;
						}
						instance->onEvent(&desc);
					}
				} else {
					handle->eventWake.reset();
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
		handle->eventWake.set();
	}

	sl_bool AsyncIoLoop::_native_attachInstance(AsyncIoInstance* instance, AsyncIoMode mode)
	{
		AsyncIoLoopHandle* handle = (AsyncIoLoopHandle*)m_handle;
		int hObject = (int)(instance->getHandle());
		
		struct kevent ke[2];
		int ret = -1;

		switch (mode) {
			case AsyncIoMode::InOut:
				EV_SET(ke, hObject, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, instance);
				EV_SET(ke + 1, hObject, EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, instance);
				ret = ::kevent(handle->kq, ke, 2, sl_null, 0, sl_null);
				break;
			case AsyncIoMode::In:
				EV_SET(ke, hObject, EVFILT_READ, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, instance);
				ret = ::kevent(handle->kq, ke, 1, sl_null, 0, sl_null);
				break;
			case AsyncIoMode::Out:
				EV_SET(ke, hObject, EVFILT_WRITE, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, instance);
				ret = ::kevent(handle->kq, ke, 1, sl_null, 0, sl_null);
				break;
			default:
				break;
		}
		if (ret != -1) {
			instance->setMode(mode);
			return sl_true;
		}
		return sl_false;
	}

	void AsyncIoLoop::_native_detachInstance(AsyncIoInstance* instance)
	{
		AsyncIoLoopHandle* handle = (AsyncIoLoopHandle*)m_handle;
		int hObject = (int)(instance->getHandle());
		
		AsyncIoMode mode = instance->getMode();
		struct kevent ke[2];
		int ret = -1;
		
		switch (mode) {
			case AsyncIoMode::InOut:
				EV_SET(ke, hObject, EVFILT_READ, EV_DELETE, 0, 0, sl_null);
				EV_SET(ke + 1, hObject, EVFILT_WRITE, EV_DELETE, 0, 0, sl_null);
				ret = ::kevent(handle->kq, ke, 2, sl_null, 0, sl_null);
				break;
			case AsyncIoMode::In:
				EV_SET(ke, hObject, EVFILT_READ, EV_DELETE, 0, 0, sl_null);
				ret = ::kevent(handle->kq, ke, 1, sl_null, 0, sl_null);
				break;
			case AsyncIoMode::Out:
				EV_SET(ke, hObject, EVFILT_WRITE, EV_DELETE, 0, 0, sl_null);
				ret = ::kevent(handle->kq, ke, 1, sl_null, 0, sl_null);
				break;
			default:
				break;
		}
		
		if (ret == -1) {
			SLIB_UNUSED(ret);
		}
	}

}

#endif
