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

#ifndef CHECKHEADER_SLIB_NETWORK_EVENT
#define CHECKHEADER_SLIB_NETWORK_EVENT

#include "socket.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)
#include "../core/event.h"
#else
#include "../io/pipe_event.h"
#endif

namespace slib
{

#if defined(SLIB_PLATFORM_IS_WINDOWS)
	class SLIB_EXPORT SocketEvent : public Event
#else
	class SLIB_EXPORT SocketEvent : public PipeEvent
#endif
	{
	protected:
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		SocketEvent(sl_socket socket, sl_uint32 events, void* handle);
#else
		SocketEvent(sl_socket socket, sl_uint32 events, Pipe&& pipe);
#endif

		~SocketEvent();

	public:
		enum
		{
			Read = 1,	// receive, receiveFrom, accept
			Write = 2,	// send, sendTo, connect
			Close = 4	// close, error
		};

	public:
		static Ref<SocketEvent> create(const Socket& socket, sl_uint32 events) noexcept;

		static Ref<SocketEvent> createRead(const Socket& socket) noexcept;

		static Ref<SocketEvent> createWrite(const Socket& socket) noexcept;

		static Ref<SocketEvent> createReadWrite(const Socket& socket) noexcept;

	public:
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		void set() override;

		void reset() override;
#endif

		sl_bool wait(sl_int32 timeout = -1) noexcept;

		sl_bool wait(sl_uint32* pOutStatus, sl_int32 timeout = -1) noexcept;

		// returns event status
		sl_uint32 waitEvents(sl_int32 timeout = -1) noexcept;

		// count must be less than 64
		static sl_bool waitMultipleEvents(SocketEvent** events, sl_uint32* status, sl_uint32 count, sl_int32 timeout = -1) noexcept;

	protected:
		sl_bool doWait(sl_int32 timeout) override;

		sl_bool doWait(sl_uint32* pOutStatus, sl_int32 timeout) noexcept;

		static sl_bool doWaitMultipleEvents(SocketEvent** events, sl_uint32* status, sl_uint32 count, sl_int32 timeout) noexcept;

	protected:
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		void* m_handle;
#endif
		sl_socket m_socket;
		sl_uint32 m_events;

	};

}

#endif
