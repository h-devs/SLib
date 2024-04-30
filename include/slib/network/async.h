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

	class AsyncSocketStreamInstance;

	class SLIB_EXPORT AsyncSocketStream : public AsyncStreamBase
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncSocketStream();

		~AsyncSocketStream();

	public:
		static Ref<AsyncSocketStream> create(Socket&& socket, const Ref<AsyncIoLoop>& loop);

		static Ref<AsyncSocketStream> create(Socket&& socket);

	public:
		sl_socket getSocket();

	protected:
		Ref<AsyncSocketStreamInstance> _getIoInstance();

		static Ref<AsyncSocketStreamInstance> _createInstance(Socket&& socket, sl_bool flagIPv6);

		void _requestConnect(AsyncSocketStreamInstance* instance, sl_int32 timeout);

		void _onConnect(sl_bool flagError);

	protected:
		AtomicFunction<void(AsyncSocketStream*, sl_bool flagError)> m_onConnect;

		friend class AsyncSocketStreamInstance;
	};

	class AsyncSocketServerInstance;

	class SLIB_EXPORT AsyncSocketServer : public AsyncIoObject
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncSocketServer();

		~AsyncSocketServer();

	public:
		void start();

		sl_bool isRunning();

		sl_socket getSocket();

	protected:
		Ref<AsyncSocketServerInstance> _getIoInstance();

		void _onError();

	protected:
		static Ref<AsyncSocketServerInstance> _createInstance(Socket&&, sl_bool flagIPv6, sl_bool flagDomain);

	protected:
		Function<void(AsyncSocketServer*)> m_onError;

		friend class AsyncSocketServerInstance;

	};

	class AsyncTcpSocket;

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

	class SLIB_EXPORT AsyncTcpSocket : public AsyncSocketStream
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncTcpSocket();

		~AsyncTcpSocket();

	public:
		static Ref<AsyncTcpSocket> create(AsyncTcpSocketParam& param);

		static Ref<AsyncTcpSocket> create(const Ref<AsyncIoLoop>& loop);

		static Ref<AsyncTcpSocket> create(sl_bool flagIPv6 = sl_false);

	public:
		sl_bool connect(const SocketAddress& address, sl_int32 timeout = -1);

		void connect(const SocketAddress& address, const Function<void(AsyncTcpSocket*, sl_bool flagError)>& callback, sl_int32 timeout = -1);

	};

	class AsyncTcpServer;

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

	class SLIB_EXPORT AsyncTcpServer : public AsyncSocketServer
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncTcpServer();

		~AsyncTcpServer();

	public:
		static Ref<AsyncTcpServer> create(AsyncTcpServerParam& param);

	protected:
		void _onAccept(Socket&, SocketAddress&);

	protected:
		Function<void(AsyncTcpServer*, Socket&, SocketAddress&)> m_onAccept;

		friend class AsyncSocketServerInstance;

	};

	class SLIB_EXPORT AsyncDomainSocketParam
	{
	public:
		Socket socket; // optional
		DomainSocketPath bindPath;
		sl_bool flagLogError; // default: true
		Ref<AsyncIoLoop> ioLoop;

	public:
		AsyncDomainSocketParam();

		SLIB_DECLARE_MOVEONLY_CLASS_DEFAULT_MEMBERS(AsyncDomainSocketParam)

	};

	class SLIB_EXPORT AsyncDomainSocket : public AsyncSocketStream
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncDomainSocket();

		~AsyncDomainSocket();

	public:
		static Ref<AsyncDomainSocket> create(AsyncDomainSocketParam& param);

		static Ref<AsyncDomainSocket> create(const Ref<AsyncIoLoop>& loop);

		static Ref<AsyncDomainSocket> create();

	public:
		sl_bool connect(const DomainSocketPath& path, sl_int32 timeout = -1);

		void connect(const DomainSocketPath& path, const Function<void(AsyncDomainSocket*, sl_bool flagError)>& callback, sl_int32 timeout = -1);

	};

	class AsyncDomainSocketServer;

	class SLIB_EXPORT AsyncDomainSocketServerParam
	{
	public:
		Socket socket; // optional
		DomainSocketPath bindPath;

		sl_bool flagAutoStart; // default: true
		sl_bool flagLogError; // default: true
		Ref<AsyncIoLoop> ioLoop;

		Function<void(AsyncDomainSocketServer*, Socket&, DomainSocketPath&)> onAccept;
		Function<void(AsyncDomainSocketServer*)> onError;

	public:
		AsyncDomainSocketServerParam();

		SLIB_DECLARE_MOVEONLY_CLASS_DEFAULT_MEMBERS(AsyncDomainSocketServerParam)

	};

	class SLIB_EXPORT AsyncDomainSocketServer : public AsyncSocketServer
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncDomainSocketServer();

		~AsyncDomainSocketServer();

	public:
		static Ref<AsyncDomainSocketServer> create(AsyncDomainSocketServerParam& param);

	protected:
		void _onAccept(Socket&, DomainSocketPath&);

	protected:
		Function<void(AsyncDomainSocketServer*, Socket&, DomainSocketPath&)> m_onAccept;

		friend class AsyncSocketServerInstance;

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
		sl_bool flagReusingAddress; // default: false
		sl_bool flagReusingPort; // default: false
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
