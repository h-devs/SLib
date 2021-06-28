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

#include "slib/network/event.h"

#include "slib/core/thread.h"
#include "slib/core/scoped_buffer.h"

#if defined(SLIB_PLATFORM_IS_WIN32)
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include "slib/core/pipe_event.h"
#include <unistd.h>
#include <poll.h>
#endif

namespace slib
{

	namespace priv
	{
		namespace socket_event
		{

#if !defined(SLIB_PLATFORM_IS_WIN32)
			class EventImpl : public PipeEvent
			{
			public:
				sl_socket socket;
				sl_uint32 events;

			public:
				EventImpl(sl_socket _socket, sl_uint32 _events)
				{
					socket = _socket;
					events = _events;
				}

			};
#endif

			static HSocketEvent CreateEventHandle(const Socket& socket, sl_uint32 events)
			{
				if (socket.isOpened()) {
					Socket::initializeSocket();
					socket.setNonBlockingMode(sl_true);
#if defined(SLIB_PLATFORM_IS_WINDOWS)
					WSAEVENT hEvent = WSACreateEvent();
					if (hEvent) { // WSA_INVALID_EVENT = NULL
						sl_uint32 ev = 0;
						if (events & SocketEvent::Read) {
							ev = ev | FD_READ | FD_ACCEPT;
						}
						if (events & SocketEvent::Write) {
							ev = ev | FD_WRITE | FD_CONNECT;
						}
						if (events & SocketEvent::Close) {
							ev = ev | FD_CLOSE;
						}

						int ret = WSAEventSelect(socket.get(), hEvent, ev);
						if (!ret) {
							return HSocketEvent(hEvent, socket.get());
						}
						WSACloseEvent(hEvent);
					}
#else
					EventImpl* ev = new EventImpl(socket.get(), events);
					if (ev) {
						if (ev->isOpened()) {
							return ev;
						}
						delete ev;
					}
#endif
				}
				return sl_null;
			}

			static void DeleteEventHandle(const HSocketEvent& handle)
			{
#if defined(SLIB_PLATFORM_IS_WIN32)
				WSACloseEvent(handle.event);
#else
				delete (EventImpl*)handle;
#endif
			}

		}
	}

	using namespace priv::socket_event;

	SLIB_DEFINE_HANDLE_CONTAINER_MEMBERS(SocketEvent, HSocketEvent, m_handle, sl_null, DeleteEventHandle)

	SocketEvent SocketEvent::create(const Socket& socket, sl_uint32 events) noexcept
	{
		return CreateEventHandle(socket, events);
	}

	SocketEvent SocketEvent::createRead(const Socket& socket) noexcept
	{
		return CreateEventHandle(socket, SocketEvent::Read | SocketEvent::Close);
	}

	SocketEvent SocketEvent::createWrite(const Socket& socket) noexcept
	{
		return CreateEventHandle(socket, SocketEvent::Write | SocketEvent::Close);
	}

	SocketEvent SocketEvent::createReadWrite(const Socket& socket) noexcept
	{
		return CreateEventHandle(socket, SocketEvent::Read | SocketEvent::Write | SocketEvent::Close);
	}

	sl_bool SocketEvent::isOpened() const noexcept
	{
		return isNotNone();
	}

	void SocketEvent::close() noexcept
	{
		setNone();
	}

	void SocketEvent::set()
	{
		if (isNotNone()) {
#if defined(SLIB_PLATFORM_IS_WIN32)
			WSASetEvent(m_handle.event);
#else
			((EventImpl*)m_handle)->set();
#endif
		}
	}

	void SocketEvent::reset()
	{
		if (isNotNone()) {
#if defined(SLIB_PLATFORM_IS_WIN32)
			WSAResetEvent(m_handle.event);
#else
			((EventImpl*)m_handle)->reset();
#endif
		}
	}

	sl_uint32 SocketEvent::waitEvents(sl_int32 timeout) noexcept
	{
		if (isOpened()) {
			SocketEvent* ev = this;
			sl_uint32 events;
			if (waitMultipleEvents(&ev, &events, 1, timeout)) {
				return events;
			}
		}
		return 0;
	}

	sl_bool SocketEvent::doWait(sl_int32 timeout)
	{
		return waitEvents(timeout) != 0;
	}

#define MAX_WAIT_EVENTS 64
	sl_bool SocketEvent::waitMultipleEvents(SocketEvent** events, sl_uint32* status, sl_uint32 count, sl_int32 timeout) noexcept
	{
		if (!count || count > MAX_WAIT_EVENTS) { // WSA_MAXIMUM_WAIT_EVENTS
			return sl_false;
		}
		Thread* thread = Thread::getCurrent();
		if (thread) {
			if (thread->isStopping()) {
				return sl_false;
			}
			thread->setWaitingEvent(events[0]);
		}
		sl_bool ret = doWaitMultipleEvents(events, status, count, timeout);
		if (thread) {
			thread->clearWaitingEvent();
		}
		return ret;
	}

	sl_bool SocketEvent::doWaitMultipleEvents(SocketEvent** events, sl_uint32* statuses, sl_uint32 count, sl_int32 timeout) noexcept
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		DWORD t = timeout >= 0 ? timeout : WSA_INFINITE;
		SLIB_SCOPED_BUFFER(WSAEVENT, 64, hEvents, count);
		SLIB_SCOPED_BUFFER(sl_uint32, 64, indexMap, count);
		sl_uint32 cEvents = 0;
		for (sl_uint32 i = 0; i < count; i++) {
			SocketEvent* ev = events[i];
			if (ev) {
				hEvents[cEvents] = ev->m_handle.event;
				indexMap[cEvents] = i;
				cEvents++;
			}
			if (statuses) {
				statuses[i] = 0;
			}
		}
		if (cEvents == 0) {
			return sl_false;
		}

		DWORD dwRet = WSAWaitForMultipleEvents(cEvents, hEvents, FALSE, t, TRUE);
		if (dwRet >= WSA_WAIT_EVENT_0 && dwRet < WSA_WAIT_EVENT_0 + cEvents) {
			sl_uint32 indexHandle = dwRet - WSA_WAIT_EVENT_0;
			sl_uint32 index = indexMap[indexHandle];
			WSANETWORKEVENTS ne;
			ZeroMemory(&ne, sizeof(ne));
			SocketEvent* ev = events[index];
			if (!(WSAEnumNetworkEvents(ev->m_handle.socket, hEvents[indexHandle], &ne))) {
				sl_uint32 st = 0;
				sl_uint32 le = ne.lNetworkEvents;
				if (le & (FD_CONNECT | FD_WRITE)) {
					st |= SocketEvent::Write;
				}
				if (le & (FD_ACCEPT | FD_READ)) {
					st |= SocketEvent::Read;
				}
				if (le & FD_CLOSE) {
					st |= SocketEvent::Close;
				}
				if (statuses) {
					statuses[index] = st;
				}
				return sl_true;
			}
		}
		return sl_false;
#else
		SLIB_SCOPED_BUFFER(pollfd, 64, fd, 2 * count);
		SLIB_SCOPED_BUFFER(sl_uint32, 64, indexMap, count);
		Base::zeroMemory(fd, sizeof(pollfd) * 2 * count);
		sl_uint32 cEvents = 0;
		for (sl_uint32 i = 0; i < count; i++) {
			SocketEvent* ev = events[i];
			if (ev) {
				EventImpl* handle = (EventImpl*)(ev->m_handle);
				fd[cEvents * 2].fd = handle->socket;
				sl_uint32 evs = 0;
				sl_uint32 sevs = handle->events;
				if (sevs & SocketEvent::Read) {
					evs = evs | POLLIN | POLLPRI;
				}
				if (sevs & SocketEvent::Write) {
					evs = evs | POLLOUT;
				}
				if (sevs & SocketEvent::Close) {
#if defined(SLIB_PLATFORM_IS_LINUX)
					evs = evs | POLLERR | POLLHUP | POLLRDHUP;
#else
					evs = evs | POLLERR | POLLHUP;
#endif
				}
				fd[cEvents * 2].events = evs;
				fd[cEvents * 2 + 1].fd = (int)(handle->getReadPipeHandle());
				fd[cEvents * 2 + 1].events = POLLIN | POLLPRI | POLLERR | POLLHUP;
				indexMap[cEvents] = i;
				cEvents++;
			}
			if (statuses) {
				statuses[i] = 0;
			}
		}
		if (!cEvents) {
			return sl_false;
		}

		int t = timeout >= 0 ? (int)timeout : -1;
		int iRet = poll(fd, 2 * (int)cEvents, t);
		if (iRet > 0) {
			for (sl_uint32 k = 0; k < cEvents; k++) {
				sl_uint32 status = 0;
				sl_uint32 revs = fd[k * 2].revents;
				if (revs & (POLLIN | POLLPRI)) {
					status |= SocketEvent::Read;
				}
				if (revs & POLLOUT) {
					status |= SocketEvent::Write;
				}
#if defined(SLIB_PLATFORM_IS_LINUX)
				if (revs & (POLLERR | POLLHUP | POLLRDHUP)) {
#else
				if (revs & (POLLERR | POLLHUP)) {
#endif
					status |= SocketEvent::Close;
				}
				if (statuses) {
					statuses[indexMap[k]] = status;
				}
				if (fd[k * 2 + 1].revents) {
					SocketEvent* ev = events[indexMap[k]];
					ev->reset();
				}
			}
			return sl_true;
		}
		return sl_false;
#endif
	}
	

	sl_bool Socket::connectAndWait(const SocketAddress& address, sl_int32 timeout) const noexcept
	{
		setNonBlockingMode(sl_true);
		if (connect(address)) {
			SocketEvent ev = SocketEvent::createWrite(*this);
			if (ev.isOpened()) {
				if (ev.waitEvents(timeout) & SocketEvent::Write) {
					if (getOption_Error() == 0) {
						return sl_true;
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool Socket::connectDomainAndWait(const StringParam& address, sl_int32 timeout) const noexcept
	{
		setNonBlockingMode(sl_true);
		if (connectDomain(address)) {
			SocketEvent ev = SocketEvent::createWrite(*this);
			if (ev.isOpened()) {
				if (ev.waitEvents(timeout) & SocketEvent::Write) {
					if (getOption_Error() == 0) {
						return sl_true;
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool Socket::connectAbstractDomainAndWait(const StringParam& address, sl_int32 timeout) const noexcept
	{
		setNonBlockingMode(sl_true);
		if (connectAbstractDomain(address)) {
			SocketEvent ev = SocketEvent::createWrite(*this);
			if (ev.isOpened()) {
				if (ev.waitEvents(timeout) & SocketEvent::Write) {
					if (getOption_Error() == 0) {
						return sl_true;
					}
				}
			}
		}
		return sl_false;
	}

}
