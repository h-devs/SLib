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

#ifndef CHECKHEADER_SLIB_NETWORK_ASYNC
#define CHECKHEADER_SLIB_NETWORK_ASYNC

#include "socket.h"

#include "../io/async_stream.h"
#include "../core/string.h"
#include "../core/pair.h"
#include "../core/default_members.h"

namespace slib
{

	class AsyncTcpSocket;
	class AsyncTcpSocketInstance;

	class SLIB_EXPORT AsyncTcpSocketParam
	{
	public:
		Socket socket; // optional
		SocketAddress bindAddress;
		sl_bool flagIPv6; // default: false
		sl_bool flagLogError; // default: true
		Ref<AsyncIoLoop> ioLoop;

	public:
		AsyncTcpSocketParam();

		SLIB_DECLARE_MOVEONLY_CLASS_DEFAULT_MEMBERS(AsyncTcpSocketParam)

	};

	class SLIB_EXPORT AsyncTcpSocket : public AsyncStreamBase
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncTcpSocket();

		~AsyncTcpSocket();

	public:
		static Ref<AsyncTcpSocket> create(AsyncTcpSocketParam& param);

		static Ref<AsyncTcpSocket> create();

		static Ref<AsyncTcpSocket> create(Socket&& socket);

	public:
		sl_socket getSocket();

		sl_bool connect(const SocketAddress& address, const Function<void(AsyncTcpSocket*, sl_bool flagError)>& callback);

		sl_bool receive(void* data, sl_size size, const Function<void(AsyncStreamResult&)>& callback, CRef* userObject = sl_null);

		sl_bool receive(const Memory& mem, const Function<void(AsyncStreamResult&)>& callback);

		sl_bool send(void* data, sl_size size, const Function<void(AsyncStreamResult&)>& callback, CRef* userObject = sl_null);

		sl_bool send(const Memory& mem, const Function<void(AsyncStreamResult&)>& callback);

	protected:
		Ref<AsyncTcpSocketInstance> _getIoInstance();

		void _onConnect(sl_bool flagError);

	private:
		static Ref<AsyncTcpSocketInstance> _createInstance(Socket&& socket, sl_bool flagIPv6);

	protected:
		AtomicFunction<void(AsyncTcpSocket*, sl_bool flagError)> m_onConnect;

		friend class AsyncTcpSocketInstance;

	};


	class AsyncTcpServer;
	class AsyncTcpServerInstance;

	class SLIB_EXPORT AsyncTcpServerParam
	{
	public:
		Socket socket; // optional
		SocketAddress bindAddress;

		sl_bool flagIPv6; // default: false
		sl_bool flagAutoStart; // default: true
		sl_bool flagLogError; // default: true
		Ref<AsyncIoLoop> ioLoop;

		Function<void(AsyncTcpServer*, Socket&, SocketAddress&)> onAccept;
		Function<void(AsyncTcpServer*)> onError;

	public:
		AsyncTcpServerParam();

		SLIB_DECLARE_MOVEONLY_CLASS_DEFAULT_MEMBERS(AsyncTcpServerParam)

	};

	class SLIB_EXPORT AsyncTcpServer : public AsyncIoObject
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncTcpServer();

		~AsyncTcpServer();

	public:
		static Ref<AsyncTcpServer> create(AsyncTcpServerParam& param);

	public:
		void close();

		sl_bool isOpened();

		void start();

		sl_bool isRunning();

		sl_socket getSocket();

	protected:
		Ref<AsyncTcpServerInstance> _getIoInstance();

		void _onAccept(Socket&, SocketAddress& address);

		void _onError();

	protected:
		static Ref<AsyncTcpServerInstance> _createInstance(Socket&&, sl_bool flagIPv6);

	protected:
		Function<void(AsyncTcpServer*, Socket&, SocketAddress&)> m_onAccept;
		Function<void(AsyncTcpServer*)> m_onError;

		friend class AsyncTcpServerInstance;

	};


	class AsyncUdpSocket;
	class AsyncUdpSocketInstance;

	class SLIB_EXPORT AsyncUdpSocketParam
	{
	public:
		Socket socket; // optional
		StringParam bindDevice; // optional
		SocketAddress bindAddress;
		
		sl_bool flagIPv6; // default: false
		sl_bool flagSendingBroadcast; // default: false
		sl_bool flagMulticastLoop; // default: false
		List< Pair<IPAddress, sl_uint32> > multicastGroups; // {multicastAddress, interfaceIndex} pairs

		sl_bool flagAutoStart; // default: true
		sl_bool flagLogError; // default: true
		sl_uint32 packetSize; // default: 65536
		Ref<AsyncIoLoop> ioLoop;

		Function<void(AsyncUdpSocket*, sl_uint32 interfaceIndex, IPAddress& dst, SocketAddress& src, void* data, sl_uint32 sizeReceived)> onReceive;
		Function<void(AsyncUdpSocket*, SocketAddress&, void* data, sl_uint32 sizeReceived)> onReceiveFrom; // Ignored when `onReceive` is set
		Function<void(AsyncUdpSocket*)> onError;

	public:
		AsyncUdpSocketParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AsyncUdpSocketParam)

	};

	class SLIB_EXPORT AsyncUdpSocket : public AsyncIoObject
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncUdpSocket();

		~AsyncUdpSocket();

	public:
		static Ref<AsyncUdpSocket> create(AsyncUdpSocketParam& param);

	public:
		void close();

		sl_bool isOpened();

		void start();

		sl_bool isRunning();

	public:
		sl_socket getSocket();

		void setSendingBroadcast(sl_bool flag = sl_true);

		void setSendBufferSize(sl_uint32 size);

		void setReceiveBufferSize(sl_uint32 size);

		sl_bool sendTo(const SocketAddress& addressTo, const void* data, sl_size size);

		sl_bool sendTo(const SocketAddress& addressTo, const MemoryView& mem);

		sl_bool sendTo(sl_uint32 interfaceIndex, const IPAddress& src, const SocketAddress& dst, const void* data, sl_size size);

		sl_bool sendTo(sl_uint32 interfaceIndex, const IPAddress& src, const SocketAddress& dst, const MemoryView& mem);

	protected:
		Ref<AsyncUdpSocketInstance> _getIoInstance();

		void _onReceive(SocketAddress& address, void* data, sl_uint32 sizeReceived);

		void _onReceive(sl_uint32 interfaceIndex, IPAddress& dst, SocketAddress& src, void* data, sl_uint32 sizeReceived);

		void _onError();

	protected:
		static Ref<AsyncUdpSocketInstance> _createInstance(Socket&& socket, sl_uint32 packetSize);

	protected:
		Function<void(AsyncUdpSocket*, sl_uint32 interfaceIndex, IPAddress& dst, SocketAddress& src, void* data, sl_uint32 sizeReceived)> m_onReceive;
		Function<void(AsyncUdpSocket*, SocketAddress&, void* data, sl_uint32 sizeReceived)> m_onReceiveFrom;
		Function<void(AsyncUdpSocket*)> m_onError;

		friend class AsyncUdpSocketInstance;

	};

}

#endif
