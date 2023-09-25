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

#include "slib/network/async.h"

#include "network_async.h"
#include "slib/core/handle_ptr.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(AsyncTcpSocketInstance, AsyncStreamInstance)

	AsyncTcpSocketInstance::AsyncTcpSocketInstance()
	{
		m_flagRequestConnect = sl_false;
		m_flagSupportingConnect = sl_true;
	}

	AsyncTcpSocketInstance::~AsyncTcpSocketInstance()
	{
		_free();
	}

	sl_socket AsyncTcpSocketInstance::getSocket()
	{
		return (sl_socket)(getHandle());
	}

	sl_bool AsyncTcpSocketInstance::isSupportedConnect()
	{
		return m_flagSupportingConnect;
	}

	sl_bool AsyncTcpSocketInstance::connect(const SocketAddress& address)
	{
		m_flagRequestConnect = sl_true;
		m_addressRequestConnect = address;
		return sl_true;
	}

	void AsyncTcpSocketInstance::onClose()
	{
		_free();
		AsyncStreamInstance::onClose();
	}

	void AsyncTcpSocketInstance::_free()
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

	void AsyncTcpSocketInstance::_onConnect(sl_bool flagError)
	{
		Ref<AsyncTcpSocket> object = Ref<AsyncTcpSocket>::from(getObject());
		if (object.isNotNull()) {
			object->_onConnect(flagError);
		}
	}


	SLIB_DEFINE_MOVEONLY_CLASS_DEFAULT_MEMBERS(AsyncTcpSocketParam)

	AsyncTcpSocketParam::AsyncTcpSocketParam()
	{
		flagIPv6 = sl_false;
		flagLogError = sl_true;
	}


	SLIB_DEFINE_OBJECT(AsyncTcpSocket, AsyncStreamBase)

	AsyncTcpSocket::AsyncTcpSocket()
	{
	}

	AsyncTcpSocket::~AsyncTcpSocket()
	{
		if (m_onConnect.isNotNull()) {
			m_onConnect(sl_null, sl_false);
		}
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

		Ref<AsyncTcpSocketInstance> instance = _createInstance(Move(socket), flagIPv6);
		if (instance.isNotNull()) {
			Ref<AsyncIoLoop> loop = param.ioLoop;
			if (loop.isNull()) {
				loop = AsyncIoLoop::getDefault();
				if (loop.isNull()) {
					return sl_null;
				}
			}
			Ref<AsyncTcpSocket> ret = new AsyncTcpSocket;
			if (ret.isNotNull()) {
				if (ret->_initialize(instance.get(), AsyncIoMode::InOut, loop)) {
					return ret;
				}
			}
		}

		return sl_null;
	}

	Ref<AsyncTcpSocket> AsyncTcpSocket::create()
	{
		AsyncTcpSocketParam param;
		return create(param);
	}

	Ref<AsyncTcpSocket> AsyncTcpSocket::create(Socket&& socket)
	{
		AsyncTcpSocketParam param;
		param.socket = Move(socket);
		return create(param);
	}

	sl_socket AsyncTcpSocket::getSocket()
	{
		Ref<AsyncTcpSocketInstance> instance = _getIoInstance();
		if (instance.isNotNull()) {
			return instance->getSocket();
		}
		return SLIB_SOCKET_INVALID_HANDLE;
	}

	sl_bool AsyncTcpSocket::connect(const SocketAddress& address, const Function<void(AsyncTcpSocket*, sl_bool flagError)>& callback)
	{
		Ref<AsyncIoLoop> loop = getIoLoop();
		if (loop.isNull()) {
			return sl_false;
		}
		if (address.isInvalid()) {
			return sl_false;
		}
		Ref<AsyncTcpSocketInstance> instance = _getIoInstance();
		if (instance.isNotNull()) {
			HandlePtr<Socket> socket(instance->getSocket());
			if (socket->isOpened()) {
				if (m_onConnect.isNotNull()) {
					m_onConnect(this, sl_false);
				}
				m_onConnect = callback;
				if (instance->isSupportedConnect()) {
					if (instance->connect(address)) {
						loop->requestOrder(instance.get());
						return sl_true;
					}
				} else {
					if (socket->connectAndWait(address)) {
						_onConnect(sl_true);
						return sl_true;
					} else {
						_onConnect(sl_false);
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool AsyncTcpSocket::receive(void* data, sl_size size, const Function<void(AsyncStreamResult&)>& callback, CRef* userObject)
	{
		return AsyncStreamBase::read(data, size, callback, userObject);
	}

	sl_bool AsyncTcpSocket::receive(const Memory& mem, const Function<void(AsyncStreamResult&)>& callback)
	{
		return AsyncStreamBase::read(mem.getData(), mem.getSize(), callback, mem.ref.get());
	}

	sl_bool AsyncTcpSocket::send(void* data, sl_size size, const Function<void(AsyncStreamResult&)>& callback, CRef* userObject)
	{
		return AsyncStreamBase::write(data, size, callback, userObject);
	}

	sl_bool AsyncTcpSocket::send(const Memory& mem, const Function<void(AsyncStreamResult&)>& callback)
	{
		return AsyncStreamBase::write(mem.getData(), mem.getSize(), callback, mem.ref.get());
	}

	Ref<AsyncTcpSocketInstance> AsyncTcpSocket::_getIoInstance()
	{
		return Ref<AsyncTcpSocketInstance>::from(AsyncStreamBase::getIoInstance());
	}

	void AsyncTcpSocket::_onConnect(sl_bool flagError)
	{
		if (m_onConnect.isNotNull()) {
			m_onConnect(this, flagError);
			m_onConnect.setNull();
		}
	}


	SLIB_DEFINE_OBJECT(AsyncTcpServerInstance, AsyncIoInstance)

	AsyncTcpServerInstance::AsyncTcpServerInstance()
	{
		m_flagRunning = sl_false;
	}

	AsyncTcpServerInstance::~AsyncTcpServerInstance()
	{
		_closeHandle();
	}

	void AsyncTcpServerInstance::start()
	{
		ObjectLocker lock(this);
		if (m_flagRunning) {
			return;
		}
		m_flagRunning = sl_true;
		requestOrder();
	}

	sl_bool AsyncTcpServerInstance::isRunning()
	{
		return m_flagRunning;
	}

	sl_socket AsyncTcpServerInstance::getSocket()
	{
		return (sl_socket)(getHandle());
	}

	void AsyncTcpServerInstance::onClose()
	{
		m_flagRunning = sl_false;
		_closeHandle();
	}

	void AsyncTcpServerInstance::_closeHandle()
	{
		sl_socket socket = getSocket();
		if (socket != SLIB_SOCKET_INVALID_HANDLE) {
			Socket::close(socket);
			setHandle(SLIB_ASYNC_INVALID_HANDLE);
		}
	}

	void AsyncTcpServerInstance::_onAccept(Socket& socketAccept, SocketAddress& address)
	{
		Ref<AsyncTcpServer> server = Ref<AsyncTcpServer>::from(getObject());
		if (server.isNotNull()) {
			server->_onAccept(socketAccept, address);
		}
	}

	void AsyncTcpServerInstance::_onError()
	{
		Ref<AsyncTcpServer> server = Ref<AsyncTcpServer>::from(getObject());
		if (server.isNotNull()) {
			server->_onError();
		}
	}


	SLIB_DEFINE_MOVEONLY_CLASS_DEFAULT_MEMBERS(AsyncTcpServerParam)

	AsyncTcpServerParam::AsyncTcpServerParam()
	{
		flagIPv6 = sl_false;

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

#if defined(SLIB_PLATFORM_IS_UNIX)
			/*
			 * SO_REUSEADDR option allows the server applications to listen on the port that is still
			 * bound by some TIME_WAIT sockets.
			 *
			 * http://stackoverflow.com/questions/14388706/socket-options-so-reuseaddr-and-so-reuseport-how-do-they-differ-do-they-mean-t
			 */
			socket.setReusingAddress(sl_true);
#endif

			if (!(socket.bind(param.bindAddress))) {
				if (param.flagLogError) {
					LogError(TAG, "AsyncTcpServer bind error: %s, %s", param.bindAddress.toString(), Socket::getLastErrorMessage());
				}
				return sl_null;
			}
		}

		if (socket.listen()) {
			Ref<AsyncTcpServerInstance> instance = _createInstance(Move(socket), flagIPv6);
			if (instance.isNotNull()) {
				Ref<AsyncIoLoop> loop = param.ioLoop;
				if (loop.isNull()) {
					loop = AsyncIoLoop::getDefault();
					if (loop.isNull()) {
						return sl_null;
					}
				}
				Ref<AsyncTcpServer> ret = new AsyncTcpServer;
				if (ret.isNotNull()) {
					ret->m_onAccept = param.onAccept;
					ret->m_onError = param.onError;
					instance->setObject(ret.get());
					ret->setIoInstance(instance.get());
					ret->setIoLoop(loop);
					if (loop->attachInstance(instance.get(), AsyncIoMode::In)) {
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


	void AsyncTcpServer::close()
	{
		closeIoInstance();
	}

	sl_bool AsyncTcpServer::isOpened()
	{
		return getIoInstance().isNotNull();
	}

	void AsyncTcpServer::start()
	{
		Ref<AsyncTcpServerInstance> instance = _getIoInstance();
		if (instance.isNotNull()) {
			instance->start();
		}
	}

	sl_bool AsyncTcpServer::isRunning()
	{
		Ref<AsyncTcpServerInstance> instance = _getIoInstance();
		if (instance.isNotNull()) {
			return instance->isRunning();
		}
		return sl_false;
	}

	sl_socket AsyncTcpServer::getSocket()
	{
		Ref<AsyncTcpServerInstance> instance = _getIoInstance();
		if (instance.isNotNull()) {
			return instance->getSocket();
		}
		return SLIB_SOCKET_INVALID_HANDLE;
	}

	Ref<AsyncTcpServerInstance> AsyncTcpServer::_getIoInstance()
	{
		return Ref<AsyncTcpServerInstance>::from(AsyncIoObject::getIoInstance());
	}

	void AsyncTcpServer::_onAccept(Socket& socketAccept, SocketAddress& address)
	{
		m_onAccept(this, socketAccept, address);
	}

	void AsyncTcpServer::_onError()
	{
		m_onError(this);
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
#if defined(SLIB_PLATFORM_IS_UNIX)
			/*
			 * SO_REUSEADDR option allows the server applications to listen on the port that is still
			 * bound by some TIME_WAIT sockets.
			 *
			 * http://stackoverflow.com/questions/14388706/socket-options-so-reuseaddr-and-so-reuseport-how-do-they-differ-do-they-mean-t
			 */
			socket.setReusingAddress(sl_true);
#endif
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
			socket.setSendingBroadcast(sl_true);
		}

		Ref<AsyncUdpSocketInstance> instance = _createInstance(Move(socket), param.packetSize);
		if (instance.isNotNull()) {
			Ref<AsyncIoLoop> loop = param.ioLoop;
			if (loop.isNull()) {
				loop = AsyncIoLoop::getDefault();
				if (loop.isNull()) {
					return sl_null;
				}
			}
			Ref<AsyncUdpSocket> ret = new AsyncUdpSocket;
			if (ret.isNotNull()) {
				ret->m_onReceiveFrom = param.onReceiveFrom;
				instance->setObject(ret.get());
				ret->setIoInstance(instance.get());
				ret->setIoLoop(loop);
				if (loop->attachInstance(instance.get(), AsyncIoMode::In)) {
					if (param.flagAutoStart) {
						ret->start();
					}
					return ret;
				}
			}
		}

		return sl_null;
	}

	void AsyncUdpSocket::close()
	{
		closeIoInstance();
	}

	sl_bool AsyncUdpSocket::isOpened()
	{
		return getIoInstance().isNotNull();
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

	sl_bool AsyncUdpSocket::sendTo(const IPAddress& src, const SocketAddress& dst, const void* data, sl_size size)
	{
		return sendTo(0, src, dst, data, size);
	}

	sl_bool AsyncUdpSocket::sendTo(const IPAddress& src, const SocketAddress& dst, const MemoryView& mem)
	{
		return sendTo(0, src, dst, mem.data, (sl_uint32)(mem.size));
	}

	SocketError AsyncUdpSocket::getLastError()
	{
		HandlePtr<Socket> socket(getSocket());
		if (socket->isNotNone()) {
			return socket->getLastError();
		}
		return SocketError::Unknown;
	}

	Ref<AsyncUdpSocketInstance> AsyncUdpSocket::_getIoInstance()
	{
		return Ref<AsyncUdpSocketInstance>::from(AsyncIoObject::getIoInstance());
	}

	void AsyncUdpSocket::_onReceive(SocketAddress& address, void* data, sl_uint32 sizeReceived)
	{
		m_onReceiveFrom(this, address, data, sizeReceived);
	}

	void AsyncUdpSocket::_onError()
	{
		m_onError(this);
	}

}
