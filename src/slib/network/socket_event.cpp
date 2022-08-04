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

#include "slib/network/event.h"

#include "slib/core/thread.h"
#include "slib/core/scoped_buffer.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)
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

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			static sl_uint32 GetEventStatus(sl_uint32 ne)
			{
				sl_uint32 ret = 0;
				if (ne & (FD_CONNECT | FD_WRITE)) {
					ret |= SocketEvent::Write;
				}
				if (ne & (FD_ACCEPT | FD_READ)) {
					ret |= SocketEvent::Read;
				}
				if (ne & FD_CLOSE) {
					ret |= SocketEvent::Close;
				}
				return ret;
			}
#else
			static void PreparePoll(pollfd* fd, int socket, int pipe, sl_uint32 eventsReq)
			{
				fd->fd = socket;
				sl_uint32 events = 0;
				if (eventsReq & SocketEvent::Read) {
					events = events | POLLIN | POLLPRI;
				}
				if (eventsReq & SocketEvent::Write) {
					events = events | POLLOUT;
				}
				if (eventsReq & SocketEvent::Close) {
#if defined(SLIB_PLATFORM_IS_LINUX)
					events = events | POLLERR | POLLHUP | POLLRDHUP;
#else
					events = events | POLLERR | POLLHUP;
#endif
				}
				fd->events = events;
				fd++;
				fd->fd = pipe;
				fd->events = POLLIN | POLLPRI | POLLERR | POLLHUP;
			}

			static sl_uint32 GetEventStatus(sl_uint32 ne)
			{
				sl_uint32 ret = 0;
				if (ne & (POLLIN | POLLPRI)) {
					ret |= SocketEvent::Read;
				}
				if (ne & POLLOUT) {
					ret |= SocketEvent::Write;
				}
#if defined(SLIB_PLATFORM_IS_LINUX)
				if (ne & (POLLERR | POLLHUP | POLLRDHUP)) {
#else
				if (ne & (POLLERR | POLLHUP)) {
#endif
					ret |= SocketEvent::Close;
				}
				return ret;
			}
#endif

		}
	}

	using namespace priv::socket_event;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
	SocketEvent::SocketEvent(sl_socket socket, sl_uint32 events, void* handle): m_handle(handle)
#else
	SocketEvent::SocketEvent(sl_socket socket, sl_uint32 events, Pipe&& pipe): PipeEvent(Move(pipe))
#endif
	{
		m_socket = socket;
		m_events = events;
	}

	SocketEvent::~SocketEvent()
	{
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		if (m_handle) {
			CloseHandle(m_handle);
		}
#endif
	}

	Ref<SocketEvent> SocketEvent::create(const Socket& socket, sl_uint32 events) noexcept
	{
		if (socket.isOpened()) {
			Socket::initializeSocket();
			socket.setNonBlockingMode();
#if defined(SLIB_PLATFORM_IS_WINDOWS)
			HANDLE hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
			if (hEvent) {
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
				if (!(WSAEventSelect(socket.get(), hEvent, ev))) {
					return new SocketEvent(socket.get(), events, hEvent);
				}
				CloseHandle(hEvent);
			}
#else
			Pipe pipe = Pipe::create();
			if (pipe.isOpened()) {
				return new SocketEvent(socket.get(), events, Move(pipe));
			}
#endif
		}
		return sl_null;
	}

	Ref<SocketEvent> SocketEvent::createRead(const Socket& socket) noexcept
	{
		return create(socket, SocketEvent::Read | SocketEvent::Close);
	}

	Ref<SocketEvent> SocketEvent::createWrite(const Socket& socket) noexcept
	{
		return create(socket, SocketEvent::Write | SocketEvent::Close);
	}

	Ref<SocketEvent> SocketEvent::createReadWrite(const Socket& socket) noexcept
	{
		return create(socket, SocketEvent::Read | SocketEvent::Write | SocketEvent::Close);
	}

#if defined(SLIB_PLATFORM_IS_WINDOWS)
	void SocketEvent::set()
	{
		SetEvent(m_handle);
	}

	void SocketEvent::reset()
	{
		ResetEvent(m_handle);
	}
#endif

	sl_bool SocketEvent::wait(sl_int32 timeout) noexcept
	{
		return wait(sl_null, timeout);
	}

	sl_bool SocketEvent::wait(sl_uint32* pOutStatus, sl_int32 timeout) noexcept
	{
		Thread* thread = Thread::getCurrent();
		if (thread) {
			if (thread->isStopping()) {
				return sl_false;
			}
			thread->setWaitingEvent(this);
		}
		sl_bool ret = doWait(pOutStatus, timeout);
		if (thread) {
			thread->clearWaitingEvent();
		}
		return ret;
	}

	sl_uint32 SocketEvent::waitEvents(sl_int32 timeout) noexcept
	{
		sl_uint32 status;
		if (wait(&status, timeout)) {
			return status;
		}
		return 0;
	}

	sl_bool SocketEvent::doWait(sl_int32 timeout)
	{
		return doWait(sl_null, timeout);
	}

	sl_bool SocketEvent::doWait(sl_uint32* pOutStatus, sl_int32 timeout) noexcept
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		DWORD t = timeout >= 0 ? timeout : INFINITE;
		DWORD dwRet = WaitForSingleObjectEx(m_handle, t, TRUE);
		if (dwRet == WAIT_OBJECT_0) {
			WSANETWORKEVENTS ne = { 0 };
			if (!(WSAEnumNetworkEvents(m_socket, m_handle, &ne))) {
				if (pOutStatus) {
					*pOutStatus = GetEventStatus(ne.lNetworkEvents);
				}
				return ne.lNetworkEvents != 0;
			}
		}
		return sl_false;
#else
		int t = timeout >= 0 ? (int)timeout : -1;
		pollfd fd[2] = { 0 };
		PreparePoll(fd, m_socket, (int)(getReadPipeHandle()), m_events);
		int iRet = poll(fd, 2, t);
		if (iRet > 0) {
			sl_uint32 revents = fd->revents;
			if (revents) {
				if (pOutStatus) {
					*pOutStatus = GetEventStatus(revents);
				}
				return sl_true;
			}
			if (fd[1].revents) {
				reset();
			}
		}
		return sl_false;
#endif
	}

	sl_bool SocketEvent::waitMultipleEvents(SocketEvent** events, sl_uint32* status, sl_uint32 count, sl_int32 timeout) noexcept
	{
		if (!count) {
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
		SLIB_SCOPED_BUFFER(HANDLE, 64, hEvents, count);
		SLIB_SCOPED_BUFFER(sl_uint32, 64, indexMap, count);
		if (!(hEvents && indexMap)) {
			return sl_false;
		}
		sl_uint32 cEvents = 0;
		for (sl_uint32 i = 0; i < count; i++) {
			SocketEvent* ev = events[i];
			if (ev) {
				hEvents[cEvents] = ev->m_handle;
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

		DWORD t = timeout >= 0 ? timeout : INFINITE;
		DWORD dwRet = WaitForMultipleObjectsEx(cEvents, hEvents, FALSE, t, TRUE);
		if (dwRet >= WAIT_OBJECT_0 && dwRet < WAIT_OBJECT_0 + cEvents) {
			sl_uint32 indexHandle = dwRet - WAIT_OBJECT_0;
			sl_uint32 index = indexMap[indexHandle];
			SocketEvent* ev = events[index];
			WSANETWORKEVENTS ne = { 0 };
			if (!(WSAEnumNetworkEvents(ev->m_socket, hEvents[indexHandle], &ne))) {
				if (statuses) {
					statuses[index] = GetEventStatus(ne.lNetworkEvents);
				}
				return ne.lNetworkEvents != 0;
			}
		}
		return sl_false;
#else
		SLIB_SCOPED_BUFFER(pollfd, 64, fd, 2 * count);
		SLIB_SCOPED_BUFFER(sl_uint32, 64, indexMap, count);
		if (!(fd && indexMap)) {
			return sl_false;
		}
		Base::zeroMemory(fd, sizeof(pollfd) * 2 * count);
		sl_uint32 cEvents = 0;
		for (sl_uint32 i = 0; i < count; i++) {
			SocketEvent* ev = events[i];
			if (ev) {
				PreparePoll(fd + cEvents * 2, ev->m_socket, (int)(ev->getReadPipeHandle()), ev->m_events);
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
			sl_bool flagSuccess = sl_false;
			for (sl_uint32 k = 0; k < cEvents; k++) {
				sl_uint32 revents = fd[k * 2].revents;
				if (revents) {
					flagSuccess = sl_true;
					if (statuses) {
						statuses[indexMap[k]] = GetEventStatus(revents);
					}
				}
				if (fd[k * 2 + 1].revents) {
					events[indexMap[k]]->reset();
				}
			}
			return flagSuccess;
		}
		return sl_false;
#endif
	}
	

	sl_bool Socket::connectAndWait(const SocketAddress& address, sl_int32 timeout) const noexcept
	{
		setNonBlockingMode();
		if (connect(address)) {
			Ref<SocketEvent> ev = SocketEvent::createWrite(*this);
			if (ev.isNotNull()) {
				if (ev->waitEvents(timeout) & SocketEvent::Write) {
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
		setNonBlockingMode();
		if (connectDomain(address)) {
			Ref<SocketEvent> ev = SocketEvent::createWrite(*this);
			if (ev.isNotNull()) {
				if (ev->waitEvents(timeout) & SocketEvent::Write) {
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
		setNonBlockingMode();
		if (connectAbstractDomain(address)) {
			Ref<SocketEvent> ev = SocketEvent::createWrite(*this);
			if (ev.isNotNull()) {
				if (ev->waitEvents(timeout) & SocketEvent::Write) {
					if (getOption_Error() == 0) {
						return sl_true;
					}
				}
			}
		}
		return sl_false;
	}

}
