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

#include "slib/network/definition.h"

#if defined(SLIB_PLATFORM_IS_UNIX)

#include "network_async.h"

#include "slib/core/thread.h"
#include "slib/core/handle_ptr.h"

namespace slib
{

	namespace {
		class TcpInstance : public AsyncTcpSocketInstance
		{
		public:
			sl_bool m_flagConnecting;

		public:
			TcpInstance()
			{
				m_flagConnecting = sl_false;
			}

		public:
			static Ref<TcpInstance> create(Socket&& socket)
			{
				if (socket.isOpened()) {
					if (socket.setNonBlockingMode()) {
						sl_async_handle handle = (sl_async_handle)(socket.get());
						if (handle != SLIB_ASYNC_INVALID_HANDLE) {
							Ref<TcpInstance> ret = new TcpInstance();
							if (ret.isNotNull()) {
								ret->setHandle(handle);
								socket.release();
								return ret;
							}
						}
					}
				}
				return sl_null;
			}

			void processRead(sl_bool flagError)
			{
				HandlePtr<Socket> socket = getSocket();
				if (socket->isNone()) {
					return;
				}

				Ref<AsyncStreamRequest> request = Move(m_requestReading);
				sl_size nQueue = getReadRequestCount();

				Thread* thread = Thread::getCurrent();
				while (!thread || thread->isNotStopping()) {
					if (request.isNull()) {
						if (nQueue > 0) {
							nQueue--;
							popReadRequest(request);
							if (request.isNull()) {
								return;
							}
						} else {
							return;
						}
					}
					char* data = (char*)(request->data);
					sl_size size = request->size;
					if (data && size) {
						sl_int32 n = socket->receive(data, size);
						if (n > 0) {
							processStreamResult(request.get(), n, flagError ? AsyncStreamResultCode::Unknown : AsyncStreamResultCode::Success);
						} else {
							if (n == SLIB_IO_WOULD_BLOCK) {
								if (flagError) {
									processStreamResult(request.get(), 0, AsyncStreamResultCode::Unknown);
								} else {
									m_requestReading = Move(request);
								}
							} else if (n == SLIB_IO_ENDED) {
								processStreamResult(request.get(), 0, AsyncStreamResultCode::Ended);
							} else {
								processStreamResult(request.get(), 0, AsyncStreamResultCode::Ended);
							}
							return;
						}
					} else {
						processStreamResult(request.get(), 0, AsyncStreamResultCode::Success);
					}
					request.setNull();
				}
			}

			void processWrite(sl_bool flagError)
			{
				HandlePtr<Socket> socket = getSocket();
				if (socket->isNone()) {
					return;
				}

				Ref<AsyncStreamRequest> request = Move(m_requestWriting);
				sl_size nQueue = getWriteRequestCount();

				Thread* thread = Thread::getCurrent();
				while (!thread || thread->isNotStopping()) {
					if (request.isNull()) {
						if (nQueue > 0) {
							nQueue--;
							popWriteRequest(request);
							if (request.isNull()) {
								return;
							}
						} else {
							return;
						}
					}
					char* data = (char*)(request->data);
					sl_size size = request->size;
					if (data && size) {
						for (;;) {
							sl_size sizeWritten = request->sizeWritten;
							sl_int32 n = socket->send(data + sizeWritten, size - sizeWritten);
							if (n >= 0) {
								request->sizeWritten += n;
								if (request->sizeWritten >= size) {
									request->sizeWritten = 0;
									processStreamResult(request.get(), size, flagError ? AsyncStreamResultCode::Unknown : AsyncStreamResultCode::Success);
									break;
								}
							} else {
								if (n == SLIB_IO_WOULD_BLOCK) {
									if (flagError) {
										request->sizeWritten = 0;
										processStreamResult(request.get(), sizeWritten, AsyncStreamResultCode::Unknown);
									} else {
										m_requestWriting = Move(request);
									}
								} else {
									request->sizeWritten = 0;
									processStreamResult(request.get(), sizeWritten, AsyncStreamResultCode::Unknown);
								}
								return;
							}
						}
					} else {
						processStreamResult(request.get(), 0, AsyncStreamResultCode::Success);
					}
					request.setNull();
				}
			}

			void onOrder() override
			{
				HandlePtr<Socket> socket = getSocket();
				if (socket->isNone()) {
					return;
				}

				if (m_flagConnecting) {
					return;
				}
				if (m_flagRequestConnect) {
					m_flagRequestConnect = sl_false;
					if (socket->connect(m_addressRequestConnect)) {
						m_flagConnecting = sl_true;
					} else {
						_onConnect(sl_true);
					}
					return;
				}
				processRead(sl_false);
				processWrite(sl_false);
			}

			void onEvent(EventDesc* pev) override
			{
				sl_bool flagProcessed = sl_false;
				if (pev->flagIn) {
					processRead(pev->flagError);
					flagProcessed = sl_true;
				}
				if (pev->flagOut) {
					if (m_flagConnecting) {
						m_flagConnecting = sl_false;
						if (pev->flagError) {
							_onConnect(sl_true);
						} else {
							_onConnect(sl_false);
						}
					} else {
						processWrite(pev->flagError);
					}
					flagProcessed = sl_true;
				}
				if (!flagProcessed) {
					if (pev->flagError) {
						if (m_flagConnecting) {
							m_flagConnecting = sl_false;
							_onConnect(sl_true);
						} else {
							processRead(sl_true);
							processWrite(sl_true);
						}
					}
				}
				requestOrder();
			}
		};
	}

	Ref<AsyncTcpSocketInstance> AsyncTcpSocket::_createInstance(Socket&& socket, sl_bool flagIPv6)
	{
		return TcpInstance::create(Move(socket));
	}

	namespace {
		class TcpServerInstance : public AsyncTcpServerInstance
		{
		public:
			sl_bool m_flagListening;

		public:
			TcpServerInstance()
			{
				m_flagListening = sl_false;
			}

		public:
			static Ref<TcpServerInstance> create(Socket&& socket)
			{
				if (socket.isOpened()) {
					if (socket.setNonBlockingMode()) {
						sl_async_handle handle = (sl_async_handle)(socket.get());
						if (handle != SLIB_ASYNC_INVALID_HANDLE) {
							Ref<TcpServerInstance> ret = new TcpServerInstance();
							if (ret.isNotNull()) {
								ret->setHandle(handle);
								socket.release();
								return ret;
							}
						}
					}
				}
				return sl_null;
			}

			void onOrder() override
			{
				HandlePtr<Socket> socket = getSocket();
				if (socket->isNone()) {
					return;
				}

				Thread* thread = Thread::getCurrent();
				while (!thread || thread->isNotStopping()) {
					SocketAddress addr;
					Socket socketAccept;
					if (socket->accept(socketAccept, addr)) {
						_onAccept(socketAccept, addr);
					} else {
						SocketError err = Socket::getLastError();
						if (err != SocketError::WouldBlock) {
							_onError();
						}
						return;
					}
				}
			}

			void onEvent(EventDesc* pev) override
			{
				if (pev->flagIn) {
					onOrder();
				}
				if (pev->flagError) {
					_onError();
				}
			}
		};
	}

	Ref<AsyncTcpServerInstance> AsyncTcpServer::_createInstance(Socket&& socket, sl_bool flagIPv6)
	{
		return TcpServerInstance::create(Move(socket));
	}

	namespace {
		class UdpInstance : public AsyncUdpSocketInstance
		{
		public:
			UdpInstance()
			{
			}

		public:
			static Ref<UdpInstance> create(Socket&& socket, const Memory& buffer)
			{
				if (socket.isOpened()) {
					if (socket.setNonBlockingMode()) {
						sl_async_handle handle = (sl_async_handle)(socket.get());
						if (handle != SLIB_ASYNC_INVALID_HANDLE) {
							Ref<UdpInstance> ret = new UdpInstance();
							if (ret.isNotNull()) {
								ret->m_buffer = buffer;
								ret->setHandle(handle);
								socket.release();
								return ret;
							}
						}
					}
				}
				return sl_null;
			}

			void onOrder() override
			{
				processReceive();
			}

			void onEvent(EventDesc* pev) override
			{
				if (pev->flagIn) {
					processReceive();
				}
			}

			void processReceive()
			{
				HandlePtr<Socket> socket = getSocket();
				if (socket->isNone()) {
					return;
				}

				void* buf = m_buffer.getData();
				sl_uint32 sizeBuf = (sl_uint32)(m_buffer.getSize());

				Thread* thread = Thread::getCurrent();
				while (!thread || thread->isNotStopping()) {
					SocketAddress addr;
					sl_int32 n = socket->receiveFrom(addr, buf, sizeBuf);
					if (n >= 0) {
						_onReceive(addr, n);
					} else {
						if (n != SLIB_IO_WOULD_BLOCK) {
							_onError();
						}
						break;
					}
				}
			}
		};
	}

	Ref<AsyncUdpSocketInstance> AsyncUdpSocket::_createInstance(Socket&& socket, sl_uint32 packetSize)
	{
		Memory buffer = Memory::create(packetSize);
		if (buffer.isNotNull()) {
			return UdpInstance::create(Move(socket), buffer);
		}
		return sl_null;
	}

}

#endif
