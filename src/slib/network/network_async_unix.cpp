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

#include "slib/network/definition.h"

#if defined(SLIB_PLATFORM_IS_UNIX)

#include "network_async.h"

#include "slib/core/thread.h"
#include "slib/core/handle_ptr.h"

namespace slib
{

	namespace
	{
		class StreamInstance : public AsyncSocketStreamInstance
		{
		public:
			sl_bool m_flagConnecting = sl_false;

		public:
			static Ref<StreamInstance> create(Socket&& socket)
			{
				if (socket.isOpened()) {
					if (socket.setNonBlockingMode()) {
						sl_async_handle handle = (sl_async_handle)(socket.get());
						if (handle != SLIB_ASYNC_INVALID_HANDLE) {
							Ref<StreamInstance> ret = new StreamInstance();
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
				Ref<AsyncStreamRequest> refRequest = Move(m_requestReading);
				if (refRequest.isNull()) {
					refRequest = getReadRequest();
				}
				CurrentThread thread;
				while (refRequest.isNotNull()) {
					AsyncStreamRequest* request = refRequest.get();
					char* data = (char*)(request->data);
					sl_size size = request->size;
					if (data && size) {
						char* current = data;
						for (;;) {
							sl_int32 n = socket->receive(current, size);
							if (n > 0) {
								current += n;
								if ((sl_size)n >= size) {
									processStreamResult(request, current - data, AsyncStreamResultCode::Success);
									break;
								}
								size -= n;
							} else {
								if (current > data) {
									processStreamResult(request, current - data, AsyncStreamResultCode::Success);
								} else if (flagError) {
									processStreamResult(request, 0, AsyncStreamResultCode::Unknown);
								} else if (n == SLIB_IO_WOULD_BLOCK) {
									m_requestReading = Move(refRequest);
									return;
								} else if (n == SLIB_IO_ENDED) {
									processStreamResult(request, 0, AsyncStreamResultCode::Ended);
								} else {
									processStreamResult(request, 0, AsyncStreamResultCode::Unknown);
								}
								break;
							}
						}
					} else {
						processStreamResult(request, 0, AsyncStreamResultCode::Success);
					}
					if (thread.isStopping()) {
						break;
					}
					refRequest = getReadRequest();
				}
			}

			void processWrite(sl_bool flagError)
			{
				HandlePtr<Socket> socket = getSocket();
				if (socket->isNone()) {
					return;
				}
				Ref<AsyncStreamRequest> refRequest = Move(m_requestWriting);
				if (refRequest.isNull()) {
					refRequest = getWriteRequest();
				}
				CurrentThread thread;
				while (refRequest.isNotNull()) {
					AsyncStreamRequest* request = refRequest.get();
					char* data = (char*)(request->data);
					sl_size size = request->size;
					if (data && size) {
						char* current = data;
						for (;;) {
							sl_int32 n = socket->send(current, size);
							if (n > 0) {
								current += n;
								if ((sl_size)n >= size) {
									processStreamResult(request, current - data, AsyncStreamResultCode::Success);
									break;
								}
								size -= n;
							} else {
								if (current > data) {
									processStreamResult(request, current - data, AsyncStreamResultCode::Success);
								} else if (flagError) {
									processStreamResult(request, 0, AsyncStreamResultCode::Unknown);
								} else if (n == SLIB_IO_WOULD_BLOCK) {
									m_requestWriting = Move(refRequest);
									return;
								} else {
									processStreamResult(request, 0, AsyncStreamResultCode::Unknown);
								}
								break;
							}
						}
					} else {
						processStreamResult(request, 0, AsyncStreamResultCode::Success);
					}
					if (thread.isStopping()) {
						break;
					}
					refRequest = getWriteRequest();
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
					sl_bool flagWouldBlock = sl_false;
					sl_bool flagConnected;
					if (m_addressRequestConnect.isValid()) {
						flagConnected = socket->connect(m_addressRequestConnect, &flagWouldBlock);
					} else {
						flagConnected = socket->connect(m_pathRequestConnect, &flagWouldBlock);
					}
					if (flagConnected) {
						_onConnect(sl_false);
					} else {
						if (flagWouldBlock) {
							m_flagConnecting = sl_true;
						} else {
							_onConnect(sl_true);
						}
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

	Ref<AsyncSocketStreamInstance> AsyncSocketStream::_createInstance(Socket&& socket, sl_bool flagIPv6)
	{
		return StreamInstance::create(Move(socket));
	}

	namespace
	{
		class ServerInstance : public AsyncSocketServerInstance
		{
		public:
			sl_bool m_flagListening = sl_false;

		public:
			static Ref<ServerInstance> create(Socket&& socket, sl_bool flagDomain)
			{
				if (socket.isOpened()) {
					if (socket.setNonBlockingMode()) {
						sl_async_handle handle = (sl_async_handle)(socket.get());
						if (handle != SLIB_ASYNC_INVALID_HANDLE) {
							Ref<ServerInstance> ret = new ServerInstance();
							if (ret.isNotNull()) {
								ret->m_flagDomainSocket = flagDomain;
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
					Socket socketAccept;
					if (m_flagDomainSocket) {
						DomainSocketPath path;
						if (socket->accept(socketAccept, path)) {
							_onAccept(socketAccept, path);
						} else {
							SocketError err = Socket::getLastError();
							if (err != SocketError::WouldBlock) {
								_onError();
							}
							return;
						}
					} else {
						SocketAddress addr;
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

	Ref<AsyncSocketServerInstance> AsyncSocketServer::_createInstance(Socket&& socket, sl_bool flagIPv6, sl_bool flagDomain)
	{
		return ServerInstance::create(Move(socket), flagDomain);
	}

	namespace
	{
		class UdpInstance : public AsyncUdpSocketInstance
		{
		public:
			sl_bool m_flagPacketInfo;

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
								ret->m_flagPacketInfo = socket.isReceivingPacketInformation() || socket.isReceivingIPv6PacketInformation();
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
					SocketAddress src;
					sl_int32 n;
					if (m_flagPacketInfo) {
						sl_uint32 interfaceIndex = 0;
						IPAddress dst;
						n = socket->receiveFrom(interfaceIndex, dst, src, buf, sizeBuf);
						if (n >= 0) {
							_onReceive(interfaceIndex, dst, src, n);
							continue;
						}
					} else {
						n = socket->receiveFrom(src, buf, sizeBuf);
						if (n >= 0) {
							_onReceive(src, n);
							continue;
						}
					}
					if (n != SLIB_IO_WOULD_BLOCK) {
						_onError();
					}
					break;
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
