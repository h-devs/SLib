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

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include <winsock2.h>
#include <mswsock.h>
#pragma comment(lib, "mswsock.lib")

#include "slib/core/thread.h"
#include "slib/core/log.h"

#include "network_async.h"

namespace slib
{

	namespace priv
	{
		namespace network_async
		{

			class TcpInstance : public AsyncTcpSocketInstance
			{
			public:
				sl_bool m_flagIPv6;

				WSAOVERLAPPED m_overlappedRead;
				WSABUF m_bufRead;
				DWORD m_flagsRead;
				AtomicRef<AsyncStreamRequest> m_requestReading;

				WSAOVERLAPPED m_overlappedWrite;
				WSABUF m_bufWrite;
				AtomicRef<AsyncStreamRequest> m_requestWriting;

				WSAOVERLAPPED m_overlappedConnect;
				LPFN_CONNECTEX m_funcConnectEx;

			public:
				TcpInstance()
				{
				}

				~TcpInstance()
				{
					close();
				}

			public:
				static Ref<TcpInstance> create(Socket&& socket, sl_bool flagIPv6)
				{
					if (socket.isOpened()) {
						sl_async_handle handle = (sl_async_handle)(socket.get());
						if (handle != SLIB_ASYNC_INVALID_HANDLE) {
							Ref<TcpInstance> ret = new TcpInstance();
							if (ret.isNotNull()) {
								ret->m_socket = Move(socket);
								ret->m_flagIPv6 = flagIPv6;
								ret->setHandle(handle);
								ret->initializeConnectEx();
								return ret;
							}
						}
					}
					return sl_null;
				}

				void initializeConnectEx()
				{
					sl_async_handle handle = getHandle();
					m_funcConnectEx = sl_null;
					// ConnectEx
					{
						GUID Guid = WSAID_CONNECTEX;
						DWORD dwBytes = 0;
						int ret = WSAIoctl(
							(SOCKET)(handle), SIO_GET_EXTENSION_FUNCTION_POINTER
							, &Guid, sizeof(Guid)
							, &m_funcConnectEx, sizeof(m_funcConnectEx)
							, &dwBytes, NULL, NULL);
						if (ret == SOCKET_ERROR) {
							m_funcConnectEx = sl_null;
						}
					}
					m_flagSupportingConnect = m_funcConnectEx != sl_null;
				}

				void close() override
				{
					setHandle(SLIB_ASYNC_INVALID_HANDLE);
					m_socket.close();
				}

				void onOrder() override
				{
					Socket& socket = m_socket;
					if (socket.isNone()) {
						return;
					}
					sl_async_handle handle = getHandle();
					if (handle == SLIB_ASYNC_INVALID_HANDLE) {
						return;
					}
					if (m_requestReading.isNull()) {
						Ref<AsyncStreamRequest> req;
						if (popReadRequest(req)) {
							if (req.isNotNull()) {
								if (req->data && req->size) {
									Base::zeroMemory(&m_overlappedRead, sizeof(m_overlappedRead));
									m_bufRead.buf = (CHAR*)(req->data);
									if (req->size > 0x40000000) {
										m_bufRead.len = 0x40000000;
									} else {
										m_bufRead.len = (ULONG)(req->size);
									}
									m_flagsRead = 0;
									DWORD dwRead = 0;
									int ret = WSARecv((SOCKET)handle, &m_bufRead, 1, &dwRead, &m_flagsRead, &m_overlappedRead, NULL);
									if (ret) {
										// SOCKET_ERROR
										DWORD dwErr = WSAGetLastError();
										if (dwErr == WSA_IO_PENDING) {
											m_requestReading = req;
										} else {
											_onReceive(req.get(), 0, sl_true);
										}
									} else {
										// No Error
										m_requestReading = req;
										EventDesc desc;
										desc.pOverlapped = &m_overlappedRead;
										onEvent(&desc);
									}
								} else {
									_onReceive(req.get(), 0, sl_false);
								}
							}
						}
					}
					if (m_requestWriting.isNull()) {
						Ref<AsyncStreamRequest> req;
						if (popWriteRequest(req)) {
							if (req.isNotNull()) {
								if (req->data && req->size) {
									Base::zeroMemory(&m_overlappedWrite, sizeof(m_overlappedWrite));
									m_bufWrite.buf = (CHAR*)(req->data);
									if (req->size > 0x40000000) {
										m_bufWrite.len = 0x40000000;
									} else {
										m_bufWrite.len = (ULONG)(req->size);
									}
									DWORD dwWrite = 0;
									int ret = WSASend((SOCKET)handle, &m_bufWrite, 1, &dwWrite, 0, &m_overlappedWrite, NULL);
									if (ret == 0) {
										m_requestWriting = req;
										EventDesc desc;
										desc.pOverlapped = &m_overlappedWrite;
										onEvent(&desc);
									} else {
										int dwErr = WSAGetLastError();
										if (dwErr == WSA_IO_PENDING) {
											m_requestWriting = req;
										} else {
											_onSend(req.get(), 0, sl_true);
										}
									}
								} else {
									_onSend(req.get(), 0, sl_false);
								}
							}
						}
					}
					if (m_flagRequestConnect) {
						m_flagRequestConnect = sl_false;
						if (m_funcConnectEx) {
							sockaddr_storage addr;
							sl_uint32 lenAddr = m_addressRequestConnect.getSystemSocketAddress(&addr);
							if (lenAddr) {
								Base::zeroMemory(&m_overlappedConnect, sizeof(m_overlappedConnect));
								BOOL ret = m_funcConnectEx((SOCKET)handle, (sockaddr*)&addr, lenAddr, NULL, 0, NULL, &m_overlappedConnect);
								if (ret) {
									_onConnect(sl_true);
								} else {
									int err = WSAGetLastError();
									if (err == WSAEINVAL) {
										// ConnectEx requires the socket to be 'initially bound'
										sockaddr_storage saBind;
										SocketAddress aBind;
										aBind.port = 0;
										if (m_flagIPv6) {
											aBind.ip = IPv6Address::zero();
										} else {
											aBind.ip = IPv4Address::zero();
										}
										sl_uint32 nSaBind = aBind.getSystemSocketAddress(&saBind);
										bind((SOCKET)handle, (SOCKADDR*)&saBind, nSaBind);
										BOOL ret = m_funcConnectEx((SOCKET)handle, (sockaddr*)&addr, lenAddr, NULL, 0, NULL, &m_overlappedConnect);
										if (ret) {
											_onConnect(sl_true);
										} else {
											int err = WSAGetLastError();
											if (err != ERROR_IO_PENDING) {
												_onConnect(sl_true);
											}
										}
									} else if (err != ERROR_IO_PENDING) {
										_onConnect(sl_true);
									}
								}
							}
						}
					}
				}

				void onEvent(EventDesc* pev) override
				{
					sl_async_handle handle = getHandle();
					if (handle == SLIB_ASYNC_INVALID_HANDLE) {
						return;
					}
					OVERLAPPED* pOverlapped = (OVERLAPPED*)(pev->pOverlapped);
					DWORD dwSize = 0;
					DWORD dwFlags = 0;
					sl_bool flagError = sl_false;
					if (!WSAGetOverlappedResult((SOCKET)handle, pOverlapped, &dwSize, FALSE, &dwFlags)) {
						int err = WSAGetLastError();
						if (err == WSA_IO_INCOMPLETE) {
							return;
						}
						flagError = sl_true;
					}
					if (pOverlapped == &m_overlappedRead) {
						Ref<AsyncStreamRequest> req = m_requestReading;
						m_requestReading.setNull();
						if (req.isNotNull()) {
							_onReceive(req.get(), dwSize, flagError);
						}
					} else if (pOverlapped == &m_overlappedWrite) {
						if (dwSize == 0) {
							flagError = sl_true;
						}
						Ref<AsyncStreamRequest> req = m_requestWriting;
						m_requestWriting.setNull();
						if (req.isNotNull()) {
							_onSend(req.get(), dwSize, flagError);
						}
					} else if (pOverlapped == &m_overlappedConnect) {
						if (flagError) {
							_onConnect(sl_true);
						} else {
							_onConnect(sl_false);
						}
					}
					requestOrder();
				}
			};

			class TcpServerInstance : public AsyncTcpServerInstance
			{
			public:
				sl_bool m_flagIPv6;
				sl_bool m_flagAccepting;

				WSAOVERLAPPED m_overlapped;
				char m_bufferAccept[2 * (sizeof(SOCKADDR_IN) + 16)];
				Socket m_socketAccept;

				LPFN_ACCEPTEX m_funcAcceptEx;
				LPFN_GETACCEPTEXSOCKADDRS m_funcGetAcceptExSockaddrs;

			public:
				TcpServerInstance()
				{
					m_flagAccepting = sl_false;
				}

				~TcpServerInstance()
				{
					close();
				}

			public:
				static Ref<TcpServerInstance> create(Socket&& socket, sl_bool flagIPv6)
				{
					if (socket.isOpened()) {
						sl_async_handle handle = (sl_async_handle)(socket.get());
						if (handle != SLIB_ASYNC_INVALID_HANDLE) {
							Ref<TcpServerInstance> ret = new TcpServerInstance();
							if (ret.isNotNull()) {
								ret->m_socket = Move(socket);
								ret->m_flagIPv6 = flagIPv6;
								ret->setHandle(handle);
								if (ret->initialize()) {
									return ret;
								}
							}
						}
					}
					return sl_null;
				}

				sl_bool initialize()
				{
					sl_async_handle handle = getHandle();
					m_funcAcceptEx = sl_null;
					m_funcGetAcceptExSockaddrs = sl_null;
					// AcceptEx
					{
						GUID Guid = WSAID_ACCEPTEX;
						DWORD dwBytes = 0;
						int ret = WSAIoctl(
							(SOCKET)(handle), SIO_GET_EXTENSION_FUNCTION_POINTER
							, &Guid, sizeof(Guid)
							, &m_funcAcceptEx, sizeof(m_funcAcceptEx)
							, &dwBytes, NULL, NULL);
						if (ret == SOCKET_ERROR) {
							m_funcAcceptEx = sl_null;
							LogError(TAG, "Get AcceptEx extension error");
						}
					}
					// GetAcceptExSockaddrs
					{
						GUID Guid = WSAID_GETACCEPTEXSOCKADDRS;
						DWORD dwBytes = 0;
						int ret = WSAIoctl(
							(SOCKET)(handle), SIO_GET_EXTENSION_FUNCTION_POINTER
							, &Guid, sizeof(Guid)
							, &m_funcGetAcceptExSockaddrs, sizeof(m_funcGetAcceptExSockaddrs)
							, &dwBytes, NULL, NULL);
						if (ret == SOCKET_ERROR) {
							m_funcGetAcceptExSockaddrs = sl_null;
							LogError(TAG, "Get GetAcceptExSockaddrs extension error");
						}
					}
					return m_funcAcceptEx != sl_null && m_funcGetAcceptExSockaddrs != sl_null;
				}

				void close() override
				{
					AsyncTcpServerInstance::close();
					setHandle(SLIB_ASYNC_INVALID_HANDLE);
					m_socket.close();
				}

				void onOrder() override
				{
					Socket& socket = m_socket;
					if (socket.isNone()) {
						return;
					}
					if (m_flagAccepting) {
						return;
					}
					sl_async_handle handle = getHandle();
					if (handle == SLIB_ASYNC_INVALID_HANDLE) {
						return;
					}
					Thread* thread = Thread::getCurrent();
					while (!thread || thread->isNotStopping()) {
						Socket socketAccept = Socket::open(m_flagIPv6 ? SocketType::StreamIPv6 : SocketType::Stream);
						if (socketAccept.isOpened()) {
							SOCKET handleAccept = socketAccept.get();
							m_socketAccept = Move(socketAccept);
							Base::zeroMemory(&m_overlapped, sizeof(WSAOVERLAPPED));
							DWORD dwSize = 0;
							BOOL ret = m_funcAcceptEx(
								(SOCKET)(handle), handleAccept,
								m_bufferAccept, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwSize,
								&m_overlapped);
							if (ret) {
								processAccept(sl_false);
							} else {
								int err = WSAGetLastError();
								if (err = ERROR_IO_PENDING) {
									m_flagAccepting = sl_true;
								} else {
									processAccept(sl_true);
									requestOrder();
								}
								break;
							}
						} else {
							LogError(TAG, "Failed to create accept socket");
							processAccept(sl_true);
							break;
						}
					}
				}

				void onEvent(EventDesc* pev) override
				{
					sl_async_handle handle = getHandle();
					if (handle == SLIB_ASYNC_INVALID_HANDLE) {
						return;
					}
					OVERLAPPED* pOverlapped = (OVERLAPPED*)(pev->pOverlapped);
					DWORD dwSize = 0;
					DWORD dwFlags = 0;
					sl_bool flagError = sl_false;
					if (WSAGetOverlappedResult((SOCKET)handle, pOverlapped, &dwSize, FALSE, &dwFlags)) {
						m_flagAccepting = sl_false;
						processAccept(sl_false);
					} else {
						int err = WSAGetLastError();
						if (err == WSA_IO_INCOMPLETE) {
							return;
						}
						m_flagAccepting = sl_false;
						processAccept(sl_true);
					}
					onOrder();
				}

				void processAccept(sl_bool flagError)
				{
					Ref<AsyncTcpServer> server = Ref<AsyncTcpServer>::from(getObject());
					if (server.isNull()) {
						return;
					}
					Socket& socketAccept = m_socketAccept;
					if (socketAccept.isNone()) {
						return;
					}
					if (flagError) {
						_onError();
					} else {
						SOCKADDR_IN *paddr_local, *paddr_remote;
						int lenaddr_local = 0;
						int lenaddr_remote = 0;
						m_funcGetAcceptExSockaddrs(m_bufferAccept, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (sockaddr**)&paddr_local, &lenaddr_local, (sockaddr**)&paddr_remote, &lenaddr_remote);
						if (paddr_remote) {
							SOCKET socketListen = (SOCKET)(getHandle());
							setsockopt(socketAccept.get(), SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&socketListen, sizeof(void*));
							SocketAddress addressRemote;
							addressRemote.setSystemSocketAddress(paddr_remote);
							SocketAddress addressLocal;
							addressLocal.setSystemSocketAddress(paddr_local);
							_onAccept(socketAccept, addressRemote);
						}
					}
				}

			};

			class UdpInstance : public AsyncUdpSocketInstance
			{
			public:
				sl_bool m_flagReceiving;

				WSAOVERLAPPED m_overlappedReceive;
				WSABUF m_bufReceive;
				DWORD m_flagsReceive;
				sockaddr_storage m_addrReceive;
				int m_lenAddrReceive;

			public:
				UdpInstance()
				{
					m_flagReceiving = sl_false;
				}

				~UdpInstance()
				{
					close();
				}

			public:
				static Ref<UdpInstance> create(Socket&& socket, const Memory& buffer)
				{
					if (socket.isOpened()) {
						if (socket.setNonBlockingMode(sl_true)) {
							sl_async_handle handle = (sl_async_handle)(socket.get());
							if (handle != SLIB_ASYNC_INVALID_HANDLE) {
								Ref<UdpInstance> ret = new UdpInstance();
								if (ret.isNotNull()) {
									ret->m_socket = Move(socket);
									ret->setHandle(handle);
									ret->m_buffer = buffer;
									return ret;
								}
							}
						}
					}
					return sl_null;
				}

				void close() override
				{
					AsyncUdpSocketInstance::close();
					setHandle(SLIB_ASYNC_INVALID_HANDLE);
					m_socket.close();
				}

				void onOrder() override
				{
					processReceive();
				}

				void onEvent(EventDesc* pev) override
				{
					sl_async_handle handle = getHandle();
					if (handle == SLIB_ASYNC_INVALID_HANDLE) {
						return;
					}
					OVERLAPPED* pOverlapped = (OVERLAPPED*)(pev->pOverlapped);
					if (pOverlapped == &m_overlappedReceive) {
						DWORD dwSize = 0;
						DWORD dwFlags = 0;
						if (WSAGetOverlappedResult((SOCKET)handle, pOverlapped, &dwSize, FALSE, &dwFlags)) {
							m_flagReceiving = sl_false;
							SocketAddress addr;
							if (m_lenAddrReceive > 0) {
								if (addr.setSystemSocketAddress(&m_addrReceive, m_lenAddrReceive)) {
									_onReceive(addr, dwSize);
								}
							}
						} else {
							int err = WSAGetLastError();
							if (err == WSA_IO_INCOMPLETE) {
								return;
							}
							m_flagReceiving = sl_false;
							_onError();
						}
						processReceive();
					}

				}

				void processReceive()
				{
					if (m_flagReceiving) {
						return;
					}
					sl_async_handle handle = getHandle();
					if (handle == SLIB_ASYNC_INVALID_HANDLE) {
						return;
					}

					void* buf = m_buffer.getData();
					sl_uint32 sizeBuf = (sl_uint32)(m_buffer.getSize());

					Base::zeroMemory(&m_overlappedReceive, sizeof(m_overlappedReceive));
					m_bufReceive.buf = (CHAR*)(buf);
					m_bufReceive.len = sizeBuf;
					m_flagsReceive = 0;
					DWORD dwRead = 0;
					m_lenAddrReceive = sizeof(sockaddr_storage);
					int ret = WSARecvFrom((SOCKET)handle, &m_bufReceive, 1, &dwRead, &m_flagsReceive
						, (sockaddr*)&m_addrReceive, &m_lenAddrReceive, &m_overlappedReceive, NULL);
					if (ret) {
						// SOCKET_ERROR
						DWORD dwErr = WSAGetLastError();
						if (dwErr == WSA_IO_PENDING) {
							m_flagReceiving = sl_true;
						} else {
							requestOrder();
						}
					} else {
						// Success
						m_flagReceiving = sl_true;						
					}
				}

			};

		}
	}

	Ref<AsyncTcpSocketInstance> AsyncTcpSocket::_createInstance(Socket&& socket, sl_bool flagIPv6)
	{
		return priv::network_async::TcpInstance::create(Move(socket), flagIPv6);
	}


	Ref<AsyncTcpServerInstance> AsyncTcpServer::_createInstance(Socket&& socket, sl_bool flagIPv6)
	{
		return priv::network_async::TcpServerInstance::create(Move(socket), flagIPv6);
	}

	Ref<AsyncUdpSocketInstance> AsyncUdpSocket::_createInstance(Socket&& socket, sl_uint32 packetSize)
	{
		Memory buffer = Memory::create(packetSize);
		if (buffer.isNotNull()) {
			return priv::network_async::UdpInstance::create(Move(socket), buffer);
		}
		return sl_null;
	}

}

#endif
