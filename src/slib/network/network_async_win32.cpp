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

#define _WIN32_WINNT 0x0600

#include "slib/network/definition.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#pragma comment(lib, "mswsock.lib")

#include "slib/core/thread.h"
#include "slib/core/log.h"

#include "network_async.h"

namespace slib
{

	namespace {
		class StreamInstance : public AsyncSocketStreamInstance
		{
		public:
			sl_bool m_flagIPv6;

			WSAOVERLAPPED m_overlappedRead;
			WSABUF m_bufRead;
			DWORD m_flagsRead;

			WSAOVERLAPPED m_overlappedWrite;
			WSABUF m_bufWrite;

			WSAOVERLAPPED m_overlappedConnect;
			LPFN_CONNECTEX m_funcConnectEx;

		public:
			static Ref<StreamInstance> create(Socket&& socket, sl_bool flagIPv6)
			{
				if (socket.isOpened()) {
					sl_async_handle handle = (sl_async_handle)(socket.get());
					if (handle != SLIB_ASYNC_INVALID_HANDLE) {
						Ref<StreamInstance> ret = new StreamInstance();
						if (ret.isNotNull()) {
							ret->m_flagIPv6 = flagIPv6;
							ret->setHandle(handle);
							ret->initializeConnectEx();
							socket.release();
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
						(SOCKET)(handle),
						SIO_GET_EXTENSION_FUNCTION_POINTER,
						&Guid, sizeof(Guid),
						&m_funcConnectEx, sizeof(m_funcConnectEx),
						&dwBytes,
						NULL, NULL);
					if (ret == SOCKET_ERROR) {
						m_funcConnectEx = sl_null;
					}
				}
				m_flagSupportingConnect = m_funcConnectEx != sl_null;
			}

			void onOrder() override
			{
				sl_async_handle handle = getHandle();
				if (handle == SLIB_ASYNC_INVALID_HANDLE) {
					return;
				}
				if (m_requestReading.isNull()) {
					Ref<AsyncStreamRequest> req;
					if (popReadRequest(req)) {
						if (req.isNotNull()) {
							char* data = (char*)(req->data);
							sl_size size = req->size;
							if (data && size) {
								Base::zeroMemory(&m_overlappedRead, sizeof(m_overlappedRead));
								m_bufRead.buf = data;
								if (size > 0x40000000) {
									m_bufRead.len = 0x40000000;
								} else {
									m_bufRead.len = (ULONG)size;
								}
								m_flagsRead = 0;
								DWORD dwRead = 0;
								int ret = WSARecv((SOCKET)handle, &m_bufRead, 1, &dwRead, &m_flagsRead, &m_overlappedRead, NULL);
								if (ret) {
									// SOCKET_ERROR
									DWORD dwErr = WSAGetLastError();
									if (dwErr == WSA_IO_PENDING) {
										m_requestReading = Move(req);
									} else {
										processStreamResult(req.get(), 0, AsyncStreamResultCode::Unknown);
									}
								} else {
									// No Error
									m_requestReading = Move(req);
									EventDesc desc;
									desc.pOverlapped = &m_overlappedRead;
									onEvent(&desc);
								}
							} else {
								processStreamResult(req.get(), 0, AsyncStreamResultCode::Success);
							}
						}
					}
				}
				if (m_requestWriting.isNull()) {
					Ref<AsyncStreamRequest> req;
					if (popWriteRequest(req)) {
						if (req.isNotNull()) {
							char* data = (char*)(req->data);
							sl_size size = req->size;
							if (data && size) {
								Base::zeroMemory(&m_overlappedWrite, sizeof(m_overlappedWrite));
								m_bufWrite.buf = data;
								if (size > 0x40000000) {
									m_bufWrite.len = 0x40000000;
								} else {
									m_bufWrite.len = (ULONG)size;
								}
								DWORD dwWrite = 0;
								int ret = WSASend((SOCKET)handle, &m_bufWrite, 1, &dwWrite, 0, &m_overlappedWrite, NULL);
								if (ret) {
									// SOCKET_ERROR
									int dwErr = WSAGetLastError();
									if (dwErr == WSA_IO_PENDING) {
										m_requestWriting = Move(req);
									} else {
										processStreamResult(req.get(), 0, AsyncStreamResultCode::Unknown);
									}
								} else {
									// No Error
									m_requestWriting = Move(req);
									EventDesc desc;
									desc.pOverlapped = &m_overlappedWrite;
									onEvent(&desc);
								}
							} else {
								processStreamResult(req.get(), 0, AsyncStreamResultCode::Success);
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
				if (!(WSAGetOverlappedResult((SOCKET)handle, pOverlapped, &dwSize, FALSE, &dwFlags))) {
					int err = WSAGetLastError();
					if (err == WSA_IO_INCOMPLETE) {
						return;
					}
					flagError = sl_true;
				}
				if (pOverlapped == &m_overlappedRead) {
					Ref<AsyncStreamRequest> req = Move(m_requestReading);
					if (req.isNotNull()) {
						if (flagError) {
							processStreamResult(req.get(), 0, AsyncStreamResultCode::Unknown);
						} else if (dwSize) {
							processStreamResult(req.get(), dwSize, AsyncStreamResultCode::Success);
						} else {
							processStreamResult(req.get(), 0, AsyncStreamResultCode::Ended);
						}
					}
				} else if (pOverlapped == &m_overlappedWrite) {
					Ref<AsyncStreamRequest> req = Move(m_requestWriting);
					if (req.isNotNull()) {
						if (flagError) {
							processStreamResult(req.get(), 0, AsyncStreamResultCode::Unknown);
						} else {
							processStreamResult(req.get(), dwSize, AsyncStreamResultCode::Success);
						}
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
	}

	Ref<AsyncSocketStreamInstance> AsyncSocketStream::_createInstance(Socket&& socket, sl_bool flagIPv6)
	{
		return StreamInstance::create(Move(socket), flagIPv6);
	}

	namespace {

		struct SOCKET_ADDRESS
		{
			ADDRESS_FAMILY family;
			char data[256];
		};

		class ServerInstance : public AsyncSocketServerInstance
		{
		public:
			sl_bool m_flagAccepting = sl_false;
			sl_bool m_flagIPv6;
			sl_bool m_flagDomain;

			WSAOVERLAPPED m_overlapped;
			char m_bufferAccept[sizeof(SOCKET_ADDRESS) << 1];
			Socket m_socketAccept;

			LPFN_ACCEPTEX m_funcAcceptEx;
			LPFN_GETACCEPTEXSOCKADDRS m_funcGetAcceptExSockaddrs;

		public:
			static Ref<ServerInstance> create(Socket&& socket, sl_bool flagIPv6, sl_bool flagDomain)
			{
				if (socket.isOpened()) {
					sl_async_handle handle = (sl_async_handle)(socket.get());
					if (handle != SLIB_ASYNC_INVALID_HANDLE) {
						Ref<ServerInstance> ret = new ServerInstance();
						if (ret.isNotNull()) {
							ret->m_flagIPv6 = flagIPv6;
							ret->m_flagDomain = flagDomain;
							ret->setHandle(handle);
							if (ret->initialize()) {
								socket.release();
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
						(SOCKET)(handle),
						SIO_GET_EXTENSION_FUNCTION_POINTER,
						&Guid, sizeof(Guid),
						&m_funcAcceptEx, sizeof(m_funcAcceptEx),
						&dwBytes,
						NULL, NULL);
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
						(SOCKET)(handle),
						SIO_GET_EXTENSION_FUNCTION_POINTER,
						&Guid, sizeof(Guid),
						&m_funcGetAcceptExSockaddrs, sizeof(m_funcGetAcceptExSockaddrs),
						&dwBytes,
						NULL, NULL);
					if (ret == SOCKET_ERROR) {
						m_funcGetAcceptExSockaddrs = sl_null;
						LogError(TAG, "Get GetAcceptExSockaddrs extension error");
					}
				}
				return m_funcAcceptEx != sl_null && m_funcGetAcceptExSockaddrs != sl_null;
			}

			void onOrder() override
			{
				if (m_flagAccepting) {
					return;
				}
				sl_async_handle handle = getHandle();
				if (handle == SLIB_ASYNC_INVALID_HANDLE) {
					return;
				}
				Thread* thread = Thread::getCurrent();
				while (!thread || thread->isNotStopping()) {
					Socket socketAccept = Socket::open(m_flagDomain ? SocketType::DomainStream : (m_flagIPv6 ? SocketType::StreamIPv6 : SocketType::Stream));
					if (socketAccept.isOpened()) {
						SOCKET handleAccept = socketAccept.get();
						m_socketAccept = Move(socketAccept);
						Base::zeroMemory(&m_overlapped, sizeof(WSAOVERLAPPED));
						DWORD dwSize = 0;
						BOOL ret = m_funcAcceptEx(
							(SOCKET)(handle), handleAccept,
							m_bufferAccept, 0, sizeof(SOCKET_ADDRESS), sizeof(SOCKET_ADDRESS), &dwSize,
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
					SOCKET_ADDRESS* pLocalAddress;
					SOCKET_ADDRESS* pRemoteAddress;
					int nLocalAddress = 0;
					int nRemoteAddress = 0;
					m_funcGetAcceptExSockaddrs(m_bufferAccept, 0, sizeof(SOCKET_ADDRESS), sizeof(SOCKET_ADDRESS), (sockaddr**)&pLocalAddress, &nLocalAddress, (sockaddr**)&pRemoteAddress, &nRemoteAddress);
					if (pRemoteAddress) {
						SOCKET socketListen = (SOCKET)(getHandle());
						setsockopt(socketAccept.get(), SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&socketListen, sizeof(void*));
						if (m_flagDomain) {
							sl_bool flagAbstract = sl_false;
							String path = SocketAddress::getDomainPathFromSystemSocketAddress(pRemoteAddress, nRemoteAddress, &flagAbstract);
							if (path.isNotNull()) {
								_onAccept(socketAccept, path, flagAbstract);
							}
						} else {
							SocketAddress addressRemote;
							if (addressRemote.setSystemSocketAddress(pRemoteAddress, nRemoteAddress)) {
								_onAccept(socketAccept, addressRemote);
							}
						}
					}
				}
			}
		};
	}

	Ref<AsyncSocketServerInstance> AsyncSocketServer::_createInstance(Socket&& socket, sl_bool flagIPv6, sl_bool flagDomain)
	{
		return ServerInstance::create(Move(socket), flagIPv6, flagDomain);
	}

	namespace winsock
	{
		LPFN_WSARECVMSG GetWSARecvMsg();
	}

	namespace {
		class UdpInstance : public AsyncUdpSocketInstance
		{
		public:
			sl_bool m_flagReceiving = sl_false;

			WSAOVERLAPPED m_overlappedReceive;
			WSABUF m_bufReceive;
			DWORD m_flagsReceive;
			sockaddr_storage m_addrReceive;
			sl_uint8 m_bufControl[1024];
			WSAMSG m_msgReceive;
			LPFN_WSARECVMSG m_fnRecvMsg;

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
								if (socket.isReceivingPacketInformation() || socket.isReceivingIPv6PacketInformation()) {
									ret->m_fnRecvMsg = winsock::GetWSARecvMsg();
								}
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
						SocketAddress src;
						if (m_msgReceive.namelen > 0) {
							if (src.setSystemSocketAddress(&m_addrReceive, m_msgReceive.namelen)) {
								if (m_fnRecvMsg) {
									sl_uint32 interfaceIndex = 0;
									IPAddress dst;
									cmsghdr* cmsg = CMSG_FIRSTHDR(&m_msgReceive);
									while (cmsg) {
										if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO) {
											in_pktinfo info;
											Base::copyMemory(&info, WSA_CMSG_DATA(cmsg), sizeof(info));
											interfaceIndex = (sl_uint32)(info.ipi_ifindex);
											dst = IPv4Address((sl_uint8*)(&(info.ipi_addr)));
											break;
										} else if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO) {
											in6_pktinfo info;
											Base::copyMemory(&info, WSA_CMSG_DATA(cmsg), sizeof(info));
											interfaceIndex = (sl_uint32)(info.ipi6_ifindex);
											dst = IPv6Address((sl_uint8*)(&(info.ipi6_addr)));
											break;
										}
										cmsg = CMSG_NXTHDR(&m_msgReceive, cmsg);
									}
									_onReceive(interfaceIndex, dst, src, dwSize);
								} else {
									_onReceive(src, dwSize);
								}
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
				m_msgReceive.namelen = sizeof(sockaddr_storage);
				int ret;
				if (m_fnRecvMsg) {
					m_msgReceive.name = (sockaddr*)&m_addrReceive;
					m_msgReceive.lpBuffers = &m_bufReceive;
					m_msgReceive.dwBufferCount = 1;
					m_msgReceive.Control.buf = (CHAR*)m_bufControl;
					m_msgReceive.Control.len = sizeof(m_bufControl);
					m_msgReceive.dwFlags = 0;
					ret = m_fnRecvMsg((SOCKET)handle, &m_msgReceive, &dwRead, &m_overlappedReceive, NULL);
				} else {
					ret = WSARecvFrom((SOCKET)handle, &m_bufReceive, 1, &dwRead, &m_flagsReceive, (sockaddr*)&m_addrReceive, &(m_msgReceive.namelen), &m_overlappedReceive, NULL);
				}
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
