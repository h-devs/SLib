/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/network/async.h"

#include "network_async.h"

#include "slib/core/handle_ptr.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(AsyncSocketStreamInstance, AsyncStreamInstance)

	AsyncSocketStreamInstance::AsyncSocketStreamInstance()
	{
		m_flagRequestConnect = sl_false;
		m_flagSupportingConnect = sl_true;
	}

	AsyncSocketStreamInstance::~AsyncSocketStreamInstance()
	{
		_free();
	}

	sl_socket AsyncSocketStreamInstance::getSocket()
	{
		return (sl_socket)(getHandle());
	}

	sl_bool AsyncSocketStreamInstance::isSupportedConnect()
	{
		return m_flagSupportingConnect;
	}

	sl_bool AsyncSocketStreamInstance::connect(const SocketAddress& address)
	{
		if (address.isInvalid()) {
			return sl_false;
		}
		m_flagRequestConnect = sl_true;
		m_addressRequestConnect = address;
		m_pathRequestConnect.length = 0;
		return sl_true;
	}

	sl_bool AsyncSocketStreamInstance::connect(const DomainSocketPath& path)
	{
		if (!(path.length)) {
			return sl_false;
		}
		m_flagRequestConnect = sl_true;
		m_pathRequestConnect = path;
		m_addressRequestConnect.setNone();
		return sl_true;
	}

	void AsyncSocketStreamInstance::onClose()
	{
		_free();
		AsyncStreamInstance::onClose();
	}

	void AsyncSocketStreamInstance::_free()
	{
		if (m_requestReading.isNotNull()) {
			processStreamResult(m_requestReading.get(), 0, AsyncStreamResultCode::Closed);
			m_requestReading.setNull();
		}
		if (m_requestWriting.isNotNull()) {
			processStreamResult(m_requestWriting.get(), 0, AsyncStreamResultCode::Closed);
			m_requestWriting.setNull();
		}
		sl_socket socket = getSocket();
		if (socket != SLIB_SOCKET_INVALID_HANDLE) {
			Socket::close(socket);
			setHandle(SLIB_ASYNC_INVALID_HANDLE);
		}
	}

	void AsyncSocketStreamInstance::_onConnect(sl_bool flagError)
	{
		Ref<AsyncSocketStream> object = Ref<AsyncSocketStream>::from(getObject());
		if (object.isNotNull()) {
			object->_onConnect(flagError);
		}
	}


	SLIB_DEFINE_OBJECT(AsyncSocketStream, AsyncStreamBase)

	AsyncSocketStream::AsyncSocketStream()
	{
	}

	AsyncSocketStream::~AsyncSocketStream()
	{
		if (m_onConnect.isNotNull()) {
			m_onConnect(sl_null, sl_false);
		}
	}

	Ref<AsyncSocketStream> AsyncSocketStream::create(Socket&& socket, const Ref<AsyncIoLoop>& loop)
	{
		Ref<AsyncSocketStreamInstance> instance = _createInstance(Move(socket), sl_false);
		if (instance.isNotNull()) {
			Ref<AsyncSocketStream> ret = new AsyncSocketStream;
			if (ret.isNotNull()) {
				if (ret->initialize(loop, instance.get(), AsyncIoMode::InOut)) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	Ref<AsyncSocketStream> AsyncSocketStream::create(Socket&& socket)
	{
		return create(Move(socket), Ref<AsyncIoLoop>::null());
	}

	sl_socket AsyncSocketStream::getSocket()
	{
		Ref<AsyncSocketStreamInstance> instance = _getIoInstance();
		if (instance.isNotNull()) {
			return instance->getSocket();
		}
		return SLIB_SOCKET_INVALID_HANDLE;
	}
	
	Ref<AsyncSocketStreamInstance> AsyncSocketStream::_getIoInstance()
	{
		return Ref<AsyncSocketStreamInstance>::from(AsyncStreamBase::getIoInstance());
	}

	void AsyncSocketStream::_requestConnect(AsyncSocketStreamInstance* instance, sl_int32 timeout)
	{
		Ref<AsyncIoLoop> loop = getIoLoop();
		if (loop.isNotNull()) {
			if (timeout >= 0) {
				WeakRef<AsyncSocketStream> thiz = this;
				if (loop->dispatch([thiz, this]() {
					Ref<AsyncSocketStream> ref = thiz;
					if (ref.isNull()) {
						return;
					}
					_onConnect(sl_true);
				}, timeout)) {
					loop->requestOrder(instance);
					return;
				}
			} else {
				loop->requestOrder(instance);
				return;
			}
		}
		_onConnect(sl_true);
	}

	void AsyncSocketStream::_onConnect(sl_bool flagError)
	{
		Function<void(AsyncSocketStream*, sl_bool flagError)> onConnect = m_onConnect.release();
		if (onConnect.isNotNull()) {
			onConnect(this, flagError);
			onConnect.setNull();
		}
	}


	SLIB_DEFINE_OBJECT(AsyncSocketServerInstance, AsyncIoInstance)

	AsyncSocketServerInstance::AsyncSocketServerInstance()
	{
		m_flagDomainSocket = sl_false;
		m_flagRunning = sl_false;
	}

	AsyncSocketServerInstance::~AsyncSocketServerInstance()
	{
		_closeHandle();
	}

	void AsyncSocketServerInstance::start()
	{
		ObjectLocker lock(this);
		if (m_flagRunning) {
			return;
		}
		m_flagRunning = sl_true;
		requestOrder();
	}

	sl_bool AsyncSocketServerInstance::isRunning()
	{
		return m_flagRunning;
	}

	sl_socket AsyncSocketServerInstance::getSocket()
	{
		return (sl_socket)(getHandle());
	}

	void AsyncSocketServerInstance::onClose()
	{
		m_flagRunning = sl_false;
		_closeHandle();
	}

	void AsyncSocketServerInstance::_closeHandle()
	{
		sl_socket socket = getSocket();
		if (socket != SLIB_SOCKET_INVALID_HANDLE) {
			Socket::close(socket);
			setHandle(SLIB_ASYNC_INVALID_HANDLE);
		}
	}

	void AsyncSocketServerInstance::_onAccept(Socket& client, SocketAddress& address)
	{
		Ref<AsyncTcpServer> server = Ref<AsyncTcpServer>::from(getObject());
		if (server.isNotNull()) {
			server->_onAccept(client, address);
		}
	}

	void AsyncSocketServerInstance::_onAccept(Socket& client, DomainSocketPath& path)
	{
		Ref<AsyncDomainSocketServer> server = Ref<AsyncDomainSocketServer>::from(getObject());
		if (server.isNotNull()) {
			server->_onAccept(client, path);
		}
	}

	void AsyncSocketServerInstance::_onError()
	{
		Ref<AsyncSocketServer> server = Ref<AsyncSocketServer>::from(getObject());
		if (server.isNotNull()) {
			server->_onError();
		}
	}


	SLIB_DEFINE_OBJECT(AsyncSocketServer, AsyncIoObject)

	AsyncSocketServer::AsyncSocketServer()
	{
	}

	AsyncSocketServer::~AsyncSocketServer()
	{
	}

	void AsyncSocketServer::start()
	{
		Ref<AsyncSocketServerInstance> instance = _getIoInstance();
		if (instance.isNotNull()) {
			instance->start();
		}
	}

	sl_bool AsyncSocketServer::isRunning()
	{
		Ref<AsyncSocketServerInstance> instance = _getIoInstance();
		if (instance.isNotNull()) {
			return instance->isRunning();
		}
		return sl_false;
	}

	sl_socket AsyncSocketServer::getSocket()
	{
		Ref<AsyncSocketServerInstance> instance = _getIoInstance();
		if (instance.isNotNull()) {
			return instance->getSocket();
		}
		return SLIB_SOCKET_INVALID_HANDLE;
	}

	Ref<AsyncSocketServerInstance> AsyncSocketServer::_getIoInstance()
	{
		return Ref<AsyncSocketServerInstance>::from(AsyncIoObject::getIoInstance());
	}

	void AsyncSocketServer::_onError()
	{
		m_onError(this);
	}


	SLIB_DEFINE_MOVEONLY_CLASS_DEFAULT_MEMBERS(AsyncTcpSocketParam)

	AsyncTcpSocketParam::AsyncTcpSocketParam()
	{
		flagIPv6 = sl_false;
		flagLogError = sl_true;
	}


	SLIB_DEFINE_OBJECT(AsyncTcpSocket, AsyncSocketStream)

	AsyncTcpSocket::AsyncTcpSocket()
	{
	}

	AsyncTcpSocket::~AsyncTcpSocket()
	{
	}

	Ref<AsyncTcpSocket> AsyncTcpSocket::create(AsyncTcpSocketParam& param)
	{
		sl_bool flagIPv6 = param.flagIPv6;
		Socket& socket = param.socket;
		if (socket.isNone()) {
			if (param.bindAddress.ip.isIPv6()) {
				flagIPv6 = sl_true;
			}
			if (flagIPv6) {
				socket = Socket::openTcp_IPv6();
			} else {
				socket = Socket::openTcp();
			}
			if (socket.isNone()) {
				return sl_null;
			}
			if (param.bindAddress.ip.isNotNone() || param.bindAddress.port != 0) {
				if (!(socket.bind(param.bindAddress))) {
					if (param.flagLogError) {
						LogError(TAG, "AsyncTcpSocket bind error: %s, %s", param.bindAddress.toString(), Socket::getLastErrorMessage());
					}
					return sl_null;
				}
			}
		}
		Ref<AsyncSocketStreamInstance> instance = _createInstance(Move(socket), flagIPv6);
		if (instance.isNotNull()) {
			Ref<AsyncTcpSocket> ret = new AsyncTcpSocket;
			if (ret.isNotNull()) {
				if (ret->initialize(param.ioLoop, instance.get(), AsyncIoMode::InOut)) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	Ref<AsyncTcpSocket> AsyncTcpSocket::create(const Ref<AsyncIoLoop>& loop)
	{
		AsyncTcpSocketParam param;
		param.ioLoop = loop;
		return create(param);
	}

	Ref<AsyncTcpSocket> AsyncTcpSocket::create(sl_bool flagIPv6)
	{
		AsyncTcpSocketParam param;
		param.flagIPv6 = flagIPv6;
		return create(param);
	}

	sl_bool AsyncTcpSocket::connect(const SocketAddress& address, sl_int32 timeout)
	{
		if (address.isInvalid()) {
			return sl_false;
		}
		Ref<AsyncSocketStreamInstance> instance = _getIoInstance();
		if (instance.isNotNull()) {
			HandlePtr<Socket> socket(instance->getSocket());
			if (socket->isOpened()) {
				return socket->connectAndWait(address, timeout);
			}
		}
		return sl_false;
	}

	void AsyncTcpSocket::connect(const SocketAddress& address, const Function<void(AsyncTcpSocket*, sl_bool flagError)>& callback, sl_int32 timeout)
	{
		_onConnect(sl_true);
		if (address.isValid()) {
			Ref<AsyncSocketStreamInstance> instance = _getIoInstance();
			if (instance.isNotNull()) {
				HandlePtr<Socket> socket(instance->getSocket());
				if (socket->isOpened()) {
					if (instance->isSupportedConnect()) {
						m_onConnect = Function<void(AsyncSocketStream*, sl_bool)>::from(callback);
						instance->connect(address);
						_requestConnect(instance.get(), timeout);
						return;
					} else {
						if (socket->connectAndWait(address, timeout)) {
							callback(this, sl_false);
							return;
						}
					}
				}
			}
		}
		callback(this, sl_true);
	}


	SLIB_DEFINE_MOVEONLY_CLASS_DEFAULT_MEMBERS(AsyncTcpServerParam)

	AsyncTcpServerParam::AsyncTcpServerParam()
	{
		flagIPv6 = sl_false;
#if defined(SLIB_PLATFORM_IS_UNIX)
		/*
		* SO_REUSEADDR option allows the server applications to listen on the port that is still
		* bound by some TIME_WAIT sockets.
		*
		* http://stackoverflow.com/questions/14388706/socket-options-so-reuseaddr-and-so-reuseport-how-do-they-differ-do-they-mean-t
		*/
		socket.setReusingAddress(sl_true);
		flagReusingAddress = sl_true;
#else
		flagReusingAddress = sl_false;
#endif
		flagReusingPort = sl_false;
		flagAutoStart = sl_true;
		flagLogError = sl_true;
	}


	SLIB_DEFINE_OBJECT(AsyncTcpServer, AsyncIoObject)

	AsyncTcpServer::AsyncTcpServer()
	{
	}

	AsyncTcpServer::~AsyncTcpServer()
	{
	}

	Ref<AsyncTcpServer> AsyncTcpServer::create(AsyncTcpServerParam& param)
	{
		sl_bool flagIPv6 = param.flagIPv6;
		Socket& socket = param.socket;
		if (socket.isNone()) {
			if (!(param.bindAddress.port)) {
				return sl_null;
			}
			if (param.bindAddress.ip.isIPv6()) {
				flagIPv6 = sl_true;
			}
			if (flagIPv6) {
				socket = Socket::openTcp_IPv6();
			} else {
				socket = Socket::openTcp();
			}
			if (socket.isNone()) {
				return sl_null;
			}
			if (param.flagReusingAddress) {
				socket.setReusingAddress();
			}
			if (param.flagReusingPort) {
				socket.setReusingPort();
			}
			if (!(socket.bind(param.bindAddress))) {
				if (param.flagLogError) {
					LogError(TAG, "AsyncTcpServer bind error: %s, %s", param.bindAddress.toString(), Socket::getLastErrorMessage());
				}
				return sl_null;
			}
		}
		if (socket.listen()) {
			Ref<AsyncSocketServerInstance> instance = _createInstance(Move(socket), flagIPv6, sl_false);
			if (instance.isNotNull()) {
				Ref<AsyncTcpServer> ret = new AsyncTcpServer;
				if (ret.isNotNull()) {
					ret->m_onAccept = param.onAccept;
					ret->m_onError = Function<void(AsyncSocketServer*)>::from(param.onError);
					if (ret->initialize(param.ioLoop, instance.get(), AsyncIoMode::In)) {
						if (param.flagAutoStart) {
							instance->start();
						}
						return ret;
					}
				}
			}
		} else {
			if (param.flagLogError) {
				LogError(TAG, "AsyncTcpServer listen error: %s, %s", param.bindAddress.toString(), Socket::getLastErrorMessage());
			}
		}
		return sl_null;
	}

	void AsyncTcpServer::_onAccept(Socket& client, SocketAddress& address)
	{
		m_onAccept(this, client, address);
	}


	SLIB_DEFINE_MOVEONLY_CLASS_DEFAULT_MEMBERS(AsyncDomainSocketParam)

	AsyncDomainSocketParam::AsyncDomainSocketParam()
	{
		flagLogError = sl_true;
	}


	SLIB_DEFINE_OBJECT(AsyncDomainSocket, AsyncSocketStream)

	AsyncDomainSocket::AsyncDomainSocket()
	{
	}

	AsyncDomainSocket::~AsyncDomainSocket()
	{
	}

	Ref<AsyncDomainSocket> AsyncDomainSocket::create(AsyncDomainSocketParam& param)
	{
		Socket& socket = param.socket;
		if (socket.isNone()) {
			socket = Socket::openDomainStream();
			if (socket.isNone()) {
				return sl_null;
			}
			if (param.bindPath.length) {
				if (!(socket.bind(param.bindPath))) {
					if (param.flagLogError) {
						LogError(TAG, "AsyncDomainSocket bind error: %s, %s", param.bindPath.get(), Socket::getLastErrorMessage());
					}
					return sl_null;
				}
			}
		}
		Ref<AsyncSocketStreamInstance> instance = _createInstance(Move(socket), sl_false);
		if (instance.isNotNull()) {
			Ref<AsyncDomainSocket> ret = new AsyncDomainSocket;
			if (ret.isNotNull()) {
				if (ret->initialize(param.ioLoop, instance.get(), AsyncIoMode::InOut)) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	Ref<AsyncDomainSocket> AsyncDomainSocket::create(const Ref<AsyncIoLoop>& loop)
	{
		AsyncDomainSocketParam param;
		param.ioLoop = loop;
		return create(param);
	}

	Ref<AsyncDomainSocket> AsyncDomainSocket::create()
	{
		AsyncDomainSocketParam param;
		return create(param);
	}

	sl_bool AsyncDomainSocket::connect(const DomainSocketPath& path, sl_int32 timeout)
	{
		Ref<AsyncSocketStreamInstance> instance = _getIoInstance();
		if (instance.isNotNull()) {
			HandlePtr<Socket> socket(instance->getSocket());
			if (socket->isOpened()) {
				return socket->connectAndWait(path, timeout);
			}
		}
		return sl_false;
	}

	void AsyncDomainSocket::connect(const DomainSocketPath& path, const Function<void(AsyncDomainSocket*, sl_bool flagError)>& callback, sl_int32 timeout)
	{
		_onConnect(sl_true);
		if (path.length) {
			Ref<AsyncSocketStreamInstance> instance = _getIoInstance();
			if (instance.isNotNull()) {
				HandlePtr<Socket> socket(instance->getSocket());
				if (socket->isOpened()) {
					if (instance->isSupportedConnect()) {
						m_onConnect = Function<void(AsyncSocketStream*, sl_bool)>::from(callback);
						instance->connect(path);
						_requestConnect(instance.get(), timeout);
						return;
					} else {
						if (socket->connectAndWait(path, timeout)) {
							callback(this, sl_false);
							return;
						}
					}
				}
			}
		}
		callback(this, sl_true);
	}


	SLIB_DEFINE_MOVEONLY_CLASS_DEFAULT_MEMBERS(AsyncDomainSocketServerParam)

	AsyncDomainSocketServerParam::AsyncDomainSocketServerParam()
	{
		flagAutoStart = sl_true;
		flagLogError = sl_true;
	}


	SLIB_DEFINE_OBJECT(AsyncDomainSocketServer, AsyncIoObject)

	AsyncDomainSocketServer::AsyncDomainSocketServer()
	{
	}

	AsyncDomainSocketServer::~AsyncDomainSocketServer()
	{
	}

	Ref<AsyncDomainSocketServer> AsyncDomainSocketServer::create(AsyncDomainSocketServerParam& param)
	{
		Socket& socket = param.socket;
		if (socket.isNone()) {
			if (!(param.bindPath.length)) {
				return sl_null;
			}
			socket = Socket::openDomainStream();
			if (socket.isNone()) {
				return sl_null;
			}
			if (!(socket.bind(param.bindPath))) {
				if (param.flagLogError) {
					LogError(TAG, "AsyncDomainSocketServer bind error: %s, %s", param.bindPath.get(), Socket::getLastErrorMessage());
				}
				return sl_null;
			}
		}
		if (socket.listen()) {
			Ref<AsyncSocketServerInstance> instance = _createInstance(Move(socket), sl_false, sl_true);
			if (instance.isNotNull()) {
				Ref<AsyncDomainSocketServer> ret = new AsyncDomainSocketServer;
				if (ret.isNotNull()) {
					ret->m_onAccept = param.onAccept;
					ret->m_onError = Function<void(AsyncSocketServer*)>::from(param.onError);
					if (ret->initialize(param.ioLoop, instance.get(), AsyncIoMode::In)) {
						if (param.flagAutoStart) {
							instance->start();
						}
						return ret;
					}
				}
			}
		} else {
			if (param.flagLogError) {
				LogError(TAG, "AsyncDomainSocketServer listen error: %s, %s", param.bindPath.get(), Socket::getLastErrorMessage());
			}
		}
		return sl_null;
	}

	void AsyncDomainSocketServer::_onAccept(Socket& client, DomainSocketPath& path)
	{
		m_onAccept(this, client, path);
	}


	SLIB_DEFINE_OBJECT(AsyncUdpSocketInstance, AsyncIoInstance)

	AsyncUdpSocketInstance::AsyncUdpSocketInstance()
	{
		m_flagRunning = sl_false;
	}

	AsyncUdpSocketInstance::~AsyncUdpSocketInstance()
	{
		_closeHandle();
	}

	void AsyncUdpSocketInstance::start()
	{
		ObjectLocker lock(this);
		if (m_flagRunning) {
			return;
		}
		m_flagRunning = sl_true;
		requestOrder();
	}

	sl_bool AsyncUdpSocketInstance::isRunning()
	{
		return m_flagRunning;
	}

	sl_socket AsyncUdpSocketInstance::getSocket()
	{
		return (sl_socket)(getHandle());
	}

	void AsyncUdpSocketInstance::onClose()
	{
		m_flagRunning = sl_false;
		_closeHandle();
	}

	void AsyncUdpSocketInstance::_closeHandle()
	{
		sl_socket socket = getSocket();
		if (socket != SLIB_SOCKET_INVALID_HANDLE) {
			Socket::close(socket);
			setHandle(SLIB_ASYNC_INVALID_HANDLE);
		}
	}

	void AsyncUdpSocketInstance::_onReceive(SocketAddress& address, sl_uint32 size)
	{
		Ref<AsyncUdpSocket> object = Ref<AsyncUdpSocket>::from(getObject());
		if (object.isNotNull()) {
			object->_onReceive(address, m_buffer.getData(), size);
		}
	}

	void AsyncUdpSocketInstance::_onReceive(sl_uint32 interfaceIndex, IPAddress& dst, SocketAddress& src, sl_uint32 size)
	{
		Ref<AsyncUdpSocket> object = Ref<AsyncUdpSocket>::from(getObject());
		if (object.isNotNull()) {
			object->_onReceive(interfaceIndex, dst, src, m_buffer.getData(), size);
		}
	}

	void AsyncUdpSocketInstance::_onError()
	{
		Ref<AsyncUdpSocket> object = Ref<AsyncUdpSocket>::from(getObject());
		if (object.isNotNull()) {
			object->_onError();
		}
	}

	SLIB_DEFINE_MOVEONLY_CLASS_DEFAULT_MEMBERS(AsyncUdpSocketParam)

	AsyncUdpSocketParam::AsyncUdpSocketParam()
	{
		flagIPv6 = sl_false;
		flagSendingBroadcast = sl_false;
		flagMulticastLoop = sl_false;
#if defined(SLIB_PLATFORM_IS_UNIX)
		/*
		 * SO_REUSEADDR option allows the server applications to listen on the port that is still
		 * bound by some TIME_WAIT sockets.
		 *
		 * http://stackoverflow.com/questions/14388706/socket-options-so-reuseaddr-and-so-reuseport-how-do-they-differ-do-they-mean-t
		 */
		flagReusingAddress = sl_true;
#else
		flagReusingAddress = sl_false;
#endif
		flagReusingPort = sl_false;
		flagAutoStart = sl_true;
		flagLogError = sl_false;
		packetSize = 65536;
	}


	SLIB_DEFINE_OBJECT(AsyncUdpSocket, AsyncIoObject)

	AsyncUdpSocket::AsyncUdpSocket()
	{
	}

	AsyncUdpSocket::~AsyncUdpSocket()
	{
	}

	Ref<AsyncUdpSocket> AsyncUdpSocket::create(AsyncUdpSocketParam& param)
	{
		if (param.packetSize < 1) {
			return sl_null;
		}

		Socket& socket = param.socket;
		if (socket.isNone()) {
			sl_bool flagIPv6 = param.flagIPv6;
			if (param.bindAddress.ip.isIPv6()) {
				flagIPv6 = sl_true;
			}
			if (flagIPv6) {
				socket = Socket::openUdp_IPv6();
			} else {
				socket = Socket::openUdp();
			}
			if (socket.isNone()) {
				return sl_null;
			}
			if (param.flagReusingAddress) {
				socket.setReusingAddress();
			}
			if (param.flagReusingPort) {
				socket.setReusingPort();
			}
			if (param.bindAddress.ip.isNotNone() || param.bindAddress.port != 0) {
				if (!(socket.bind(param.bindAddress))) {
					if (param.flagLogError) {
						LogError(TAG, "AsyncTcpSocket bind error: %s, %s", param.bindAddress.toString(), Socket::getLastErrorMessage());
					}
					return sl_null;
				}
			}
			if (param.bindDevice.isNotNull()) {
				if (!(socket.bindToDevice(param.bindDevice))) {
					if (param.flagLogError) {
						LogError(TAG, "AsyncTcpSocket bind device error: %s, %s", param.bindDevice.toString(), Socket::getLastErrorMessage());
					}
					return sl_null;
				}
			}
		}
		if (param.flagSendingBroadcast) {
			socket.setSendingBroadcast();
		}
		if (param.flagMulticastLoop) {
			if (param.flagIPv6) {
				socket.setIPv6MulticastLoop();
			} else {
				socket.setMulticastLoop();
			}
		}
		if (param.multicastGroups.isNotNull()) {
			ListElements< Pair<IPAddress, sl_uint32> > items(param.multicastGroups);
			for (sl_size i = 0; i < items.count; i++) {
				Pair<IPAddress, sl_uint32>& item = items[i];
				sl_bool flagError = sl_false;
				if (item.first.isIPv4()) {
					flagError = !(socket.joinMulticast(item.first.getIPv4(), item.second));
				} else if (item.first.isIPv6()) {
					flagError = !(socket.joinMulticast(item.first.getIPv6(), item.second));
				}
				if (flagError && param.flagLogError) {
					LogError(TAG, "AsyncTcpSocket join multicast error: {%s, %s}, %s", item.first.toString(), item.second, Socket::getLastErrorMessage());
				}
			}
		}
		if (param.onReceive.isNotNull()) {
			if (param.flagIPv6) {
				socket.setReceivingIPv6PacketInformation();
			} else {
				socket.setReceivingPacketInformation();
			}
		}

		Ref<AsyncUdpSocketInstance> instance = _createInstance(Move(socket), param.packetSize);
		if (instance.isNotNull()) {
			Ref<AsyncUdpSocket> ret = new AsyncUdpSocket;
			if (ret.isNotNull()) {
				ret->m_onReceive = param.onReceive;
				if (param.onReceive.isNull()) {
					ret->m_onReceiveFrom = param.onReceiveFrom;
				}
				if (ret->initialize(param.ioLoop, instance.get(), AsyncIoMode::In)) {
					if (param.flagAutoStart) {
						ret->start();
					}
					return ret;
				}
			}
		}

		return sl_null;
	}

	void AsyncUdpSocket::start()
	{
		Ref<AsyncUdpSocketInstance> instance = _getIoInstance();
		if (instance.isNotNull()) {
			instance->start();
		}
	}

	sl_bool AsyncUdpSocket::isRunning()
	{
		Ref<AsyncUdpSocketInstance> instance = _getIoInstance();
		if (instance.isNotNull()) {
			return instance->isRunning();
		}
		return sl_false;
	}

	sl_socket AsyncUdpSocket::getSocket()
	{
		Ref<AsyncUdpSocketInstance> instance = _getIoInstance();
		if (instance.isNotNull()) {
			return instance->getSocket();
		}
		return SLIB_SOCKET_INVALID_HANDLE;
	}

	void AsyncUdpSocket::setSendingBroadcast(sl_bool flag)
	{
		HandlePtr<Socket> socket(getSocket());
		if (socket->isNotNone()) {
			socket->setSendingBroadcast(flag);
		}
	}

	void AsyncUdpSocket::setSendBufferSize(sl_uint32 size)
	{
		HandlePtr<Socket> socket(getSocket());
		if (socket->isNotNone()) {
			socket->setSendBufferSize(size);
		}
	}

	void AsyncUdpSocket::setReceiveBufferSize(sl_uint32 size)
	{
		HandlePtr<Socket> socket(getSocket());
		if (socket->isNotNone()) {
			socket->setReceiveBufferSize(size);
		}
	}

	sl_bool AsyncUdpSocket::sendTo(const SocketAddress& addressTo, const void* data, sl_size size)
	{
		HandlePtr<Socket> socket(getSocket());
		if (socket->isNotNone()) {
			return socket->sendTo(addressTo, data, size) == size;
		}
		return sl_false;
	}

	sl_bool AsyncUdpSocket::sendTo(const SocketAddress& addressTo, const MemoryView& mem)
	{
		return sendTo(addressTo, mem.data, (sl_uint32)(mem.size));
	}

	sl_bool AsyncUdpSocket::sendTo(sl_uint32 interfaceIndex, const IPAddress& src, const SocketAddress& dst, const void* data, sl_size size)
	{
		HandlePtr<Socket> socket(getSocket());
		if (socket->isNotNone()) {
			return socket->sendTo(interfaceIndex, src, dst, data, size) == size;
		}
		return sl_false;
	}

	sl_bool AsyncUdpSocket::sendTo(sl_uint32 interfaceIndex, const IPAddress& src, const SocketAddress& dst, const MemoryView& mem)
	{
		return sendTo(interfaceIndex, src, dst, mem.data, (sl_uint32)(mem.size));
	}

	Ref<AsyncUdpSocketInstance> AsyncUdpSocket::_getIoInstance()
	{
		return Ref<AsyncUdpSocketInstance>::from(AsyncIoObject::getIoInstance());
	}

	void AsyncUdpSocket::_onReceive(SocketAddress& address, void* data, sl_uint32 sizeReceived)
	{
		if (m_onReceive.isNotNull()) {
			IPAddress ip;
			m_onReceive(this, 0, ip, address, data, sizeReceived);
		} else {
			m_onReceiveFrom(this, address, data, sizeReceived);
		}
	}

	void AsyncUdpSocket::_onReceive(sl_uint32 interfaceIndex, IPAddress& dst, SocketAddress& src, void* data, sl_uint32 sizeReceived)
	{
		if (m_onReceive.isNotNull()) {
			m_onReceive(this, interfaceIndex, dst, src, data, sizeReceived);
		} else {
			m_onReceiveFrom(this, src, data, sizeReceived);
		}
	}

	void AsyncUdpSocket::_onError()
	{
		m_onError(this);
	}

}
