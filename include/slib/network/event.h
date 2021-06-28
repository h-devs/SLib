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

#include "../core/event.h"
#include "../core/handle_container.h"

namespace slib
{

#if defined(SLIB_PLATFORM_IS_WINDOWS)
	class HSocketEvent
	{
	public:
		void* event;
		sl_socket socket;

	public:
		HSocketEvent() = default;

		constexpr HSocketEvent(sl_null_t): event(sl_null), socket(SLIB_SOCKET_INVALID_HANDLE) {}

		constexpr HSocketEvent(const HSocketEvent& other): event(other.event), socket(other.socket) {}

		constexpr HSocketEvent(void* _event, sl_socket _socket) : event(_event), socket(_socket) {}

		HSocketEvent& operator=(sl_null_t) noexcept
		{
			event = sl_null;
			socket = SLIB_SOCKET_INVALID_HANDLE;
			return *this;
		}

		HSocketEvent& operator=(const HSocketEvent& other) noexcept
		{
			event = other.event;
			socket = other.socket;
			return *this;
		}

		constexpr sl_bool operator==(sl_null_t) const
		{
			return !event;
		}

		constexpr sl_bool operator!=(sl_null_t) const
		{
			return event != sl_null;
		}

	};
#else
	typedef void* HSocketEvent;
#endif

	class SLIB_EXPORT SocketEvent : public IEvent
	{
		SLIB_DECLARE_HANDLE_CONTAINER_MEMBERS(SocketEvent, HSocketEvent, m_handle, sl_null)

	public:
		enum
		{
			Read = 1,	// receive, receiveFrom, accept
			Write = 2,	// send, sendTo, connect
			Close = 4	// close, error
		};
		
	public:
		static SocketEvent create(const Socket& socket, sl_uint32 events) noexcept;
		
		static SocketEvent createRead(const Socket& socket) noexcept;

		static SocketEvent createWrite(const Socket& socket) noexcept;

		static SocketEvent createReadWrite(const Socket& socket) noexcept;

	public:
		sl_bool isOpened() const noexcept;

		void close() noexcept;

		void set() override;

		void reset() override;

		sl_uint32 waitEvents(sl_int32 timeout = -1) noexcept;

		// count must be less than 64
		static sl_bool waitMultipleEvents(SocketEvent** events, sl_uint32* status, sl_uint32 count, sl_int32 timeout = -1) noexcept;

	protected:
		sl_bool doWait(sl_int32 timeout) override;

		static sl_bool doWaitMultipleEvents(SocketEvent** events, sl_uint32* status, sl_uint32 count, sl_int32 timeout) noexcept;
		
	};

}

#endif
