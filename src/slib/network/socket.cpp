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

#if defined(_WIN32)
#	define _WIN32_WINNT 0x0600
#endif

#include "slib/network/socket.h"

#include "slib/network/event.h"
#include "slib/core/log.h"
#include "slib/core/event.h"
#include "slib/core/handle_ptr.h"
#include "slib/system/system.h"
#include "slib/io/file.h"
#include "slib/io/priv/impl.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)
#	include "slib/platform/win32/socket.h"
#else
#	include <unistd.h>
#	include <sys/socket.h>
#	include <sys/un.h>
#	if defined(SLIB_PLATFORM_IS_LINUX)
#		include <linux/tcp.h>
#		include <linux/if.h>
#		include <linux/if_packet.h>
#		include <sys/ioctl.h>
#		ifndef SO_REUSEPORT
#			define SO_REUSEPORT 15
#		endif
#	else
#		include <netinet/tcp.h>
#	endif
#	include <netinet/in.h>
#	include <signal.h>
#	include <stddef.h>
#	include <errno.h>
#	define SOCKET_ERROR -1
#endif

#ifndef IPV6_RECVPKTINFO
#define IPV6_RECVPKTINFO IPV6_PKTINFO
#endif

namespace slib
{

	void L2PacketInfo::setMacAddress(const MacAddress& address) noexcept
	{
		lenHardwareAddress = 6;
		Base::copyMemory(hardwareAddress, address.m, 6);
		hardwareAddress[6] = 0;
		hardwareAddress[7] = 0;
	}

	sl_bool L2PacketInfo::getMacAddress(MacAddress* address) const noexcept
	{
		if (lenHardwareAddress == 6) {
			if (address) {
				address->setBytes(hardwareAddress);
			}
			return sl_true;
		} else {
			return sl_false;
		}
	}

	void L2PacketInfo::clearAddress() noexcept
	{
		lenHardwareAddress = 0;
		Base::zeroMemory(hardwareAddress, 8);
	}


	namespace {
		SLIB_INLINE static void CloseSocket(sl_socket socket) noexcept
		{
#if defined(SLIB_PLATFORM_IS_WINDOWS)
			closesocket(socket);
#else
			close(socket);
#endif
		}
	}

	SLIB_DEFINE_HANDLE_CONTAINER_MEMBERS(Socket, sl_socket, m_socket, SLIB_SOCKET_INVALID_HANDLE, CloseSocket)
	SLIB_DEFINE_ISTREAM_MEMBERS(Socket, const noexcept)

	Socket Socket::open(SocketType type, sl_uint32 _protocol) noexcept
	{
		initializeSocket();

		int af = 0;
		int st = 0;
		int protocol = _protocol;
		switch (type) {
			case SocketType::Stream:
				af = AF_INET;
				st = SOCK_STREAM;
				break;
			case SocketType::Datagram:
				af = AF_INET;
				st = SOCK_DGRAM;
				break;
			case SocketType::Raw:
				af = AF_INET;
				st = SOCK_RAW;
				break;
			case SocketType::StreamIPv6:
				af = AF_INET6;
				st = SOCK_STREAM;
				break;
			case SocketType::DatagramIPv6:
				af = AF_INET6;
				st = SOCK_DGRAM;
				break;
			case SocketType::RawIPv6:
				af = AF_INET6;
				st = SOCK_RAW;
				break;
			case SocketType::DomainStream:
				af = AF_UNIX;
				st = SOCK_STREAM;
				break;
			case SocketType::DomainDatagram:
				af = AF_UNIX;
				st = SOCK_DGRAM;
				break;
#if defined(SLIB_PLATFORM_IS_LINUX)
			case SocketType::PacketRaw:
				af = AF_PACKET;
				st = SOCK_RAW;
				protocol = htons(_protocol);
				break;
			case SocketType::PacketDatagram:
				af = AF_PACKET;
				st = SOCK_DGRAM;
				protocol = htons(_protocol);
				break;
#endif
			default:
				return SLIB_SOCKET_INVALID_HANDLE;
		}

#if defined(SLIB_PLATFORM_IS_WINDOWS)
		sl_socket handle = WSASocketW(af, st, protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
		sl_socket handle = ::socket(af, st, protocol);
#endif
		if (handle != SLIB_SOCKET_INVALID_HANDLE) {
			Socket ret = handle;
			if (isIPv6Type(type)) {
				ret.setIPv6Only(sl_false);
			}
#if defined(SLIB_PLATFORM_IS_APPLE)
			ret.setOption(SOL_SOCKET, SO_NOSIGPIPE, 1);
#endif
			return ret;
		}
		return SLIB_SOCKET_INVALID_HANDLE;
	}

	Socket Socket::openStream(InternetProtocol internetProtocol) noexcept
	{
		return open(SocketType::Stream, (sl_uint32)internetProtocol);
	}

	Socket Socket::openTcp() noexcept
	{
		return open(SocketType::Stream);
	}

	Socket Socket::openDatagram(InternetProtocol internetProtocol) noexcept
	{
		return open(SocketType::Datagram, (sl_uint32)internetProtocol);
	}

	Socket Socket::openUdp() noexcept
	{
		return open(SocketType::Datagram);
	}

	Socket Socket::openRaw(InternetProtocol internetProtocol) noexcept
	{
		return open(SocketType::Raw, (sl_uint32)internetProtocol);
	}

	Socket Socket::openStream_IPv6(InternetProtocol internetProtocol) noexcept
	{
		return open(SocketType::StreamIPv6, (sl_uint32)internetProtocol);
	}

	Socket Socket::openTcp_IPv6() noexcept
	{
		return open(SocketType::StreamIPv6);
	}

	Socket Socket::openDatagram_IPv6(InternetProtocol internetProtocol) noexcept
	{
		return open(SocketType::DatagramIPv6, (sl_uint32)internetProtocol);
	}

	Socket Socket::openUdp_IPv6() noexcept
	{
		return open(SocketType::DatagramIPv6);
	}

	Socket Socket::openRaw_IPv6(InternetProtocol internetProtocol) noexcept
	{
		return open(SocketType::RawIPv6, (sl_uint32)internetProtocol);
	}

	Socket Socket::openDomainStream() noexcept
	{
		return open(SocketType::DomainStream);
	}

	Socket Socket::openDomainDatagram() noexcept
	{
		return open(SocketType::DomainDatagram);
	}

	Socket Socket::openPacketRaw(EtherType type) noexcept
	{
		return open(SocketType::PacketRaw, (sl_uint32)type);
	}

	Socket Socket::openPacketDatagram(EtherType type) noexcept
	{
		return open(SocketType::PacketDatagram, (sl_uint32)type);
	}

	Socket Socket::openTcp(const SocketAddress& bindAddress) noexcept
	{
		if (bindAddress.port) {
			Socket socket;
			if (bindAddress.ip.isIPv6()) {
				socket = openTcp_IPv6();
			} else {
				socket = openTcp();
			}
			if (socket.isOpened()) {
				if (socket.bind(bindAddress)) {
					return socket;
				}
			}
		}
		return SLIB_SOCKET_INVALID_HANDLE;
	}

	Socket Socket::openTcp_IPv6(const SocketAddress& bindAddress) noexcept
	{
		if (bindAddress.port) {
			Socket socket = openTcp_IPv6();
			if (socket.isOpened()) {
				if (socket.bind(bindAddress)) {
					return socket;
				}
			}
		}
		return SLIB_SOCKET_INVALID_HANDLE;
	}

	Socket Socket::openTcp_Listen(const SocketAddress& address) noexcept
	{
		Socket socket = openTcp(address);
		if (socket.isOpened()) {
			if (socket.listen()) {
				return socket;
			}
		}
		return SLIB_SOCKET_INVALID_HANDLE;
	}

	Socket Socket::openTcp_IPv6_Listen(const SocketAddress& address) noexcept
	{
		Socket socket = openTcp_IPv6(address);
		if (socket.isOpened()) {
			if (socket.listen()) {
				return socket;
			}
		}
		return SLIB_SOCKET_INVALID_HANDLE;
	}

	Socket Socket::openTcp_Connect(const SocketAddress& address) noexcept
	{
		if (address.isValid()) {
			Socket socket;
			if (address.ip.isIPv6()) {
				socket = Socket::openTcp_IPv6();
			} else {
				socket = Socket::openTcp();
			}
			if (socket.isOpened()) {
				if (socket.connect(address)) {
					return socket;
				}
			}
		}
		return SLIB_SOCKET_INVALID_HANDLE;
	}

	Socket Socket::openTcp_ConnectAndWait(const SocketAddress& address, sl_int32 timeout) noexcept
	{
		if (address.isValid()) {
			Socket socket;
			if (address.ip.isIPv6()) {
				socket = Socket::openTcp_IPv6();
			} else {
				socket = Socket::openTcp();
			}
			if (socket.isOpened()) {
				if (socket.connectAndWait(address, timeout)) {
					return socket;
				}
			}
		}
		return SLIB_SOCKET_INVALID_HANDLE;
	}

	Socket Socket::openUdp(const SocketAddress& bindAddress) noexcept
	{
		if (bindAddress.port) {
			Socket socket;
			if (bindAddress.ip.isIPv6()) {
				socket = openUdp_IPv6();
			} else {
				socket = openUdp();
			}
			if (socket.isOpened()) {
				if (socket.bind(bindAddress)) {
					return socket;
				}
			}
		}
		return SLIB_SOCKET_INVALID_HANDLE;
	}

	Socket Socket::openUdp_IPv6(const SocketAddress& bindAddress) noexcept
	{
		if (bindAddress.port) {
			Socket socket = openUdp_IPv6();
			if (socket.isOpened()) {
				if (socket.bind(bindAddress)) {
					return socket;
				}
			}
		}
		return SLIB_SOCKET_INVALID_HANDLE;
	}

	String Socket::getTypeText(SocketType type) noexcept
	{
		switch (type) {
			case SocketType::Stream:			SLIB_RETURN_STRING("Stream/IPv4")
			case SocketType::Datagram:			SLIB_RETURN_STRING("Datagram/IPv4")
			case SocketType::Raw:				SLIB_RETURN_STRING("Raw/IPv4")
			case SocketType::StreamIPv6:		SLIB_RETURN_STRING("Stream/IPv6")
			case SocketType::DatagramIPv6:		SLIB_RETURN_STRING("Datagram/IPv6")
			case SocketType::RawIPv6:			SLIB_RETURN_STRING("Raw/IPv6")
			case SocketType::DomainStream:		SLIB_RETURN_STRING("Stream/Domain")
			case SocketType::DomainDatagram:	SLIB_RETURN_STRING("Datagram/Domain")
			case SocketType::PacketRaw:			SLIB_RETURN_STRING("Raw/Packet")
			case SocketType::PacketDatagram:	SLIB_RETURN_STRING("Datagram/Packet")
			default:							SLIB_RETURN_STRING("None")
		}
	}

	sl_bool Socket::isStreamType(SocketType type) noexcept
	{
		return (SocketType)((int)type & (int)(SocketType::MASK_ADDRESS_TYPE)) == SocketType::Stream;
	}

	sl_bool Socket::isDatagramType(SocketType type) noexcept
	{
		return (SocketType)((int)type & (int)(SocketType::MASK_ADDRESS_TYPE)) == SocketType::Datagram;
	}

	sl_bool Socket::isRawType(SocketType type) noexcept
	{
		return (SocketType)((int)type & (int)(SocketType::MASK_ADDRESS_TYPE)) == SocketType::Raw;
	}

	sl_bool Socket::isIPv4Type(SocketType type) noexcept
	{
		return (SocketType)((int)type & (int)(SocketType::MASK_ADDRESS_FAMILY)) == SocketType::ADDRESS_FAMILY_IPv4;
	}

	sl_bool Socket::isIPv6Type(SocketType type) noexcept
	{
		return (SocketType)((int)type & (int)(SocketType::MASK_ADDRESS_FAMILY)) == SocketType::ADDRESS_FAMILY_IPv6;
	}

	sl_bool Socket::isDomainType(SocketType type) noexcept
	{
		return (SocketType)((int)type & (int)(SocketType::MASK_ADDRESS_FAMILY)) == SocketType::ADDRESS_FAMILY_DOMAIN;
	}

	sl_bool Socket::isPacketType(SocketType type) noexcept
	{
		return (SocketType)((int)type & (int)(SocketType::MASK_ADDRESS_FAMILY)) == SocketType::ADDRESS_FAMILY_PACKET;
	}

	void Socket::close() noexcept
	{
		setNone();
	}

	void Socket::close(sl_socket socket) noexcept
	{
		CloseSocket(socket);
	}

	sl_bool Socket::isOpened() const noexcept
	{
		return m_socket != SLIB_SOCKET_INVALID_HANDLE;
	}

	sl_bool Socket::shutdown(SocketShutdownMode mode) const noexcept
	{
		if (isOpened()) {
#if defined(SLIB_PLATFORM_IS_WINDOWS)
			int ret = ::shutdown(m_socket, mode == SocketShutdownMode::Receive ? SD_RECEIVE : mode == SocketShutdownMode::Send ? SD_SEND : SD_BOTH);
#else
			int ret = ::shutdown(m_socket, mode == SocketShutdownMode::Receive ? SHUT_RD : mode == SocketShutdownMode::Send ? SHUT_WR : SHUT_RDWR);
#endif
			if (!ret) {
				return sl_true;
			} else {
				_checkError();
			}
		} else {
			_setError(SocketError::Closed);
		}
		return sl_false;
	}

	sl_bool Socket::bind(const SocketAddress& address) const noexcept
	{
		if (isOpened()) {
			sockaddr_storage addr;
			if (address.ip.isNotNone()) {
				int sizeAddr = (int)(address.getSystemSocketAddress(&addr));
				if (sizeAddr) {
					int ret = ::bind(m_socket, (sockaddr*)&addr, sizeAddr);
					if (ret != SOCKET_ERROR) {
						return sl_true;
					} else {
						_checkError();
					}
				} else {
					_setError(SocketError::Invalid);
				}
			} else {
				SocketAddress addrAny;
				addrAny.ip = IPv4Address::Any;
				addrAny.port = address.port;
				int sizeAddr = (int)(addrAny.getSystemSocketAddress(&addr));
				int ret = ::bind(m_socket, (sockaddr*)&addr, sizeAddr);
				if (ret != SOCKET_ERROR) {
					return sl_true;
				} else {
					addrAny.ip = IPv6Address::zero();
					int sizeAddr = (int)(addrAny.getSystemSocketAddress(&addr));
					int ret = ::bind(m_socket, (sockaddr*)&addr, sizeAddr);
					if (ret != SOCKET_ERROR) {
						return sl_true;
					} else {
						_checkError();
					}
				}
			}
		} else {
			_setError(SocketError::Closed);
		}
		return sl_false;
	}

	sl_bool Socket::bind(const DomainSocketPath& path) const noexcept
	{
		if (isOpened()) {
			sockaddr_un addr;
			sl_uint32 len = path.getSystemSocketAddress(&addr);
			if (len) {
				int ret = ::bind(m_socket, (sockaddr*)&addr, (int)len);
				if (ret != SOCKET_ERROR) {
					return sl_true;
				} else {
					_checkError();
				}
			} else {
				_setError(SocketError::Invalid);
			}
		} else {
			_setError(SocketError::Closed);
		}
		return sl_false;
	}

	sl_bool Socket::bindToDevice(const StringParam& _ifname) const noexcept
	{
#if defined(SLIB_PLATFORM_IS_LINUX)
		StringCstr ifname(_ifname);
		return setOption(SOL_SOCKET, SO_BINDTODEVICE, ifname.getData(), ifname.getLength());
#else
		_setError(SocketError::NotSupported);
		return sl_false;
#endif
	}

	sl_bool Socket::listen() const noexcept
	{
		if (isOpened()) {
			int ret = ::listen(m_socket, SOMAXCONN);
			if (ret != SOCKET_ERROR) {
				return sl_true;
			} else {
				_checkError();
			}
		} else {
			_setError(SocketError::Closed);
		}
		return sl_false;
	}

	sl_bool Socket::isListening() const noexcept
	{
		return getOption(SOL_SOCKET, SO_ACCEPTCONN) != 0;
	}

	sl_bool Socket::accept(Socket& socket, SocketAddress& address) const noexcept
	{
		if (isOpened()) {
			sockaddr_storage addr = { 0 };
			socklen_t len = sizeof(addr);
			Socket client = ::accept(m_socket, (sockaddr*)&addr, &len);
			if (client != SLIB_SOCKET_INVALID_HANDLE) {
				if (address.setSystemSocketAddress(&addr)) {
					socket = Move(client);
					return sl_true;
				} else {
					_setError(SocketError::Invalid);
				}
			} else {
				_checkError();
			}
		} else {
			_setError(SocketError::Closed);
		}
		return sl_false;
	}

	Socket Socket::accept(SocketAddress& address) const noexcept
	{
		Socket ret;
		accept(ret, address);
		return ret;
	}

	sl_bool Socket::accept(Socket& socket, DomainSocketPath& path) const noexcept
	{
		if (isOpened()) {
			sockaddr_un addr = { 0 };
			socklen_t len = sizeof(addr);
			Socket client = ::accept(m_socket, (sockaddr*)&addr, &len);
			if (client != SLIB_SOCKET_INVALID_HANDLE) {
				if (path.setSystemSocketAddress(&addr, (sl_uint32)len)) {
					socket = Move(client);
				} else {
					_setError(SocketError::Invalid);
				}
			} else {
				_checkError();
			}
		} else {
			_setError(SocketError::Closed);
		}
		return sl_false;
	}

	Socket Socket::accept(DomainSocketPath& path) const noexcept
	{
		Socket ret;
		accept(ret, path);
		return ret;
	}

	sl_bool Socket::connect(const SocketAddress& address) const noexcept
	{
		if (isOpened()) {
			sockaddr_storage addr;
			int sizeAddr = address.getSystemSocketAddress(&addr);
			if (sizeAddr) {
				int ret = ::connect(m_socket, (sockaddr*)&addr, sizeAddr);
				if (ret != SOCKET_ERROR) {
					return sl_true;
				} else {
					SocketError e = _checkError();
#if defined(SLIB_PLATFORM_IS_WINDOWS)
					return (e == SocketError::WouldBlock);
#else
					return (e == SocketError::InProgress);
#endif
				}
			} else {
				_setError(SocketError::Invalid);
			}
		} else {
			_setError(SocketError::Closed);
		}
		return sl_false;
	}

	sl_bool Socket::connect(const DomainSocketPath& path) const noexcept
	{
		if (isOpened()) {
			sockaddr_un addr;
			sl_uint32 sizeAddr = path.getSystemSocketAddress(&addr);
			if (sizeAddr) {
				int ret = ::connect(m_socket, (sockaddr*)&addr, (int)sizeAddr);
				if (ret != SOCKET_ERROR) {
					return sl_true;
				} else {
					SocketError e = _checkError();
#if defined(SLIB_PLATFORM_IS_WINDOWS)
					return (e == SocketError::WouldBlock);
#else
					return (e == SocketError::InProgress);
#endif
				}
			} else {
				_setError(SocketError::Invalid);
			}
		} else {
			_setError(SocketError::Closed);
		}
		return sl_false;
	}

	sl_int32 Socket::send(const void* buf, sl_size size) const noexcept
	{
		if (isOpened()) {
			if (size > 0x40000000) {
				size = 0x40000000;
			}
#if	defined(SLIB_PLATFORM_IS_LINUX)
			sl_int32 ret = (sl_int32)(::send(m_socket, (const char*)buf, (int)size, MSG_NOSIGNAL));
#else
			sl_int32 ret = (sl_int32)(::send(m_socket, (const char*)buf, (int)size, 0));
#endif
			return _processResult(ret);
		} else {
			_setError(SocketError::Closed);
		}
		return SLIB_IO_ERROR;
	}

	sl_reg Socket::sendFully(const void* _buf, sl_size size, SocketEvent* ev, sl_int32 timeout) const noexcept
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		sl_int64 tickEnd = GetTickFromTimeout(timeout);
		sl_size nSent = 0;
		CurrentThread thread;
		do {
			sl_int32 m = send(buf, size);
			if (m > 0) {
				nSent += m;
				if (size <= (sl_uint32)m) {
					return nSent;
				}
				buf += m;
				size -= m;
			} else if (m == SLIB_IO_WOULD_BLOCK) {
				sl_bool flagSignal = sl_false;
				if (timeout) {
					if (ev) {
						flagSignal = ev->wait(timeout);
					} else {
						Ref<SocketEvent> sev = SocketEvent::createWrite(*this);
						if (sev.isNull()) {
							return SLIB_IO_ERROR;
						}
						flagSignal = sev->wait(timeout);
					}
				}
				if (!flagSignal) {
					if (nSent) {
						return nSent;
					} else {
						return SLIB_IO_TIMEOUT;
					}
				}
				if (!size) {
					return SLIB_IO_EMPTY_CONTENT;
				}
				timeout = GetTimeoutFromTick(tickEnd);
			} else if (m == SLIB_IO_ENDED) {
				return nSent;
			} else {
				return m;
			}
		} while (thread.isNotStopping());
		return SLIB_IO_ERROR;
	}

	sl_int32 Socket::write32(const void* _buf, sl_uint32 size, sl_int32 timeout) const noexcept
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		for (;;) {
			sl_int32 m = send(buf, size);
			if (m > 0) {
				return m;
			}
			if (m != SLIB_IO_WOULD_BLOCK) {
				return m;
			}
			if (!timeout) {
				return SLIB_IO_WOULD_BLOCK;
			}
			Ref<SocketEvent> ev = SocketEvent::createWrite(*this);
			if (ev.isNull()) {
				return SLIB_IO_ERROR;
			}
			if (ev->wait(timeout)) {
				timeout = 0;
			} else {
				return SLIB_IO_TIMEOUT;
			}
		}
		return SLIB_IO_ERROR;
	}

	sl_reg Socket::write(const void* buf, sl_size size, sl_int32 timeout) const noexcept
	{
		return WriterHelper::writeWithWrite32(this, buf, size, timeout);
	}

	sl_int32 Socket::receive(void* buf, sl_size size) const noexcept
	{
		if (isOpened()) {
			if (size > 0x40000000) {
				size = 0x40000000;
			}
			if (!size) {
				return SLIB_IO_EMPTY_CONTENT;
			}
			sl_int32 ret = (sl_int32)(::recv(m_socket, (char*)buf, (int)size, 0));
			return _processResult(ret);
		} else {
			_setError(SocketError::Closed);
		}
		return SLIB_IO_ERROR;
	}

	sl_reg Socket::receiveFully(void* _buf, sl_size size, SocketEvent* ev, sl_int32 timeout) const noexcept
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		sl_int64 tickEnd = GetTickFromTimeout(timeout);
		sl_size nReceived = 0;
		CurrentThread thread;
		do {
			sl_int32 m = receive(buf, size);
			if (m > 0) {
				nReceived += m;
				if (size <= (sl_uint32)m) {
					return nReceived;
				}
				buf += m;
				size -= m;
			} else if (m == SLIB_IO_WOULD_BLOCK) {
				sl_bool flagSignal = sl_false;
				if (timeout) {
					if (ev) {
						flagSignal = ev->wait(timeout);
					} else {
						Ref<SocketEvent> sev = SocketEvent::createRead(*this);
						if (sev.isNull()) {
							return SLIB_IO_ERROR;
						}
						flagSignal = sev->wait(timeout);
					}
				}
				if (!flagSignal) {
					if (nReceived) {
						return nReceived;
					} else {
						return SLIB_IO_TIMEOUT;
					}
				}
				if (!size) {
					return SLIB_IO_EMPTY_CONTENT;
				}
				timeout = GetTimeoutFromTick(tickEnd);
			} else if (m == SLIB_IO_ENDED) {
				return nReceived;
			} else {
				return m;
			}
		} while (thread.isNotStopping());
		return SLIB_IO_ERROR;
	}

	sl_int32 Socket::read32(void* _buf, sl_uint32 size, sl_int32 timeout) const noexcept
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		for (;;) {
			sl_int32 m = receive(buf, size);
			if (m > 0) {
				return m;
			}
			if (m != SLIB_IO_WOULD_BLOCK) {
				return m;
			}
			if (!timeout) {
				return SLIB_IO_WOULD_BLOCK;
			}
			Ref<SocketEvent> ev = SocketEvent::createRead(*this);
			if (ev.isNull()) {
				return SLIB_IO_ERROR;
			}
			if (ev->wait(timeout)) {
				timeout = 0;
			} else {
				return SLIB_IO_TIMEOUT;
			}
		}
		return SLIB_IO_ERROR;
	}

	sl_reg Socket::read(void* buf, sl_size size, sl_int32 timeout) const noexcept
	{
		return ReaderHelper::readWithRead32(this, buf, size, timeout);
	}

	sl_int32 Socket::sendTo(const SocketAddress& address, const void* buf, sl_size size) const noexcept
	{
		if (isOpened()) {
			sockaddr_storage addr;
			int sizeAddr = address.getSystemSocketAddress(&addr);
			if (sizeAddr) {
				sl_int32 ret = (sl_int32)(::sendto(m_socket, (char*)buf, (int)size, 0, (sockaddr*)&addr, sizeAddr));
				return _processResult(ret);
			} else {
				_setError(SocketError::Invalid);
			}
		} else {
			_setError(SocketError::Closed);
		}
		return SLIB_IO_ERROR;
	}

	// set `interfaceInex` as zero when you don't specify the interface index
	sl_int32 Socket::sendTo(sl_uint32 interfaceIndex, const IPAddress& src, const SocketAddress& dst, const void* buf, sl_size size) const noexcept
	{
#ifdef SLIB_PLATFORM_IS_WINDOWS
		static LPFN_WSASENDMSG fnSendMsg = NULL;
		if (!fnSendMsg) {
			SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (sock != (SOCKET)-1) {
				GUID guid = WSAID_WSASENDMSG;
				DWORD dwBytes = 0;
				LPFN_WSASENDMSG func;
				if (WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &func, sizeof(func), &dwBytes, NULL, NULL) != SOCKET_ERROR) {
					fnSendMsg = func;
				}
				closesocket(sock);
			}
			if (!fnSendMsg) {
				_setError(SocketError::NotSupported);
				return SLIB_IO_ERROR;
			}
		}
#endif
		if (!(isOpened())) {
			_setError(SocketError::Closed);
			return SLIB_IO_ERROR;
		}
		if (src.isNone()) {
			_setError(SocketError::Invalid);
			return SLIB_IO_ERROR;
		}
		sockaddr_storage addrDst;
		int sizeDst = dst.getSystemSocketAddress(&addrDst);
		if (!sizeDst) {
			_setError(SocketError::Invalid);
			return SLIB_IO_ERROR;
		}
		sl_bool flagIPv6 = src.isIPv6();
		if (flagIPv6 != dst.ip.isIPv6()) {
			_setError(SocketError::Invalid);
			return SLIB_IO_ERROR;
		}
		sl_uint8 info[sizeof(in6_pktinfo)] = { 0 };
		sl_uint32 sizeInfo;
		in_pktinfo& info4 = *((in_pktinfo*)info);
		in6_pktinfo& info6 = *((in6_pktinfo*)info);;
		sl_uint8 cbuf[CMSG_SPACE(sizeof(info6))] = { 0 };
		cmsghdr* cmsg = (cmsghdr*)cbuf;
		if (flagIPv6) {
			src.getIPv6().getBytes(&(info6.ipi6_addr));
			info6.ipi6_ifindex = interfaceIndex;
			sizeInfo = sizeof(info6);
			cmsg->cmsg_level = IPPROTO_IPV6;
			cmsg->cmsg_type = IPV6_PKTINFO;
		} else {
#ifdef SLIB_PLATFORM_IS_WINDOWS
			src.getIPv4().getBytes(&(info4.ipi_addr));
#else
			src.getIPv4().getBytes(&(info4.ipi_spec_dst));
#endif
			info4.ipi_ifindex = interfaceIndex;
			sizeInfo = sizeof(info4);
			cmsg->cmsg_level = IPPROTO_IP;
			cmsg->cmsg_type = IP_PKTINFO;
		}
		cmsg->cmsg_len = CMSG_LEN(sizeInfo);
		sl_uint32 sizeControl = (sl_uint32)(CMSG_SPACE(sizeInfo));
		Base::copyMemory(CMSG_DATA(cmsg), info, sizeInfo);
#ifdef SLIB_PLATFORM_IS_WINDOWS
		WSAMSG msg = { 0 };
		msg.name = (sockaddr*)&addrDst;
		msg.namelen = (int)sizeDst;
		WSABUF wbuf;
		wbuf.buf = (CHAR*)buf;
		wbuf.len = (ULONG)size;
		msg.lpBuffers = &wbuf;
		msg.dwBufferCount = 1;
		msg.Control.buf = (CHAR*)cbuf;
		msg.Control.len = (ULONG)sizeControl;
		DWORD dwSent = 0;
		if (fnSendMsg(m_socket, &msg, 0, &dwSent, NULL, NULL)) {
			return _processError();
		}
		return (sl_int32)dwSent;
#else
		msghdr msg = { 0 };
		msg.msg_name = (sockaddr*)&addrDst;
		msg.msg_namelen = (socklen_t)sizeDst;
		iovec iov;
		iov.iov_base = (void*)buf;
		iov.iov_len = (size_t)size;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		msg.msg_control = cbuf;
		msg.msg_controllen = (socklen_t)sizeControl;
		sl_int32 result = (sl_int32)(sendmsg(m_socket, &msg, 0));
		return _processResult(result);
#endif
	}

	sl_int32 Socket::sendTo(const IPAddress& src, const SocketAddress& dst, const void* buf, sl_size size) const noexcept
	{
		return sendTo(0, src, dst, buf, size);
	}

	sl_int32 Socket::sendTo(const DomainSocketPath& path, const void* buf, sl_size size) const noexcept
	{
		if (isOpened()) {
			sockaddr_un addr;
			sl_uint32 sizeAddr = path.getSystemSocketAddress(&addr);
			if (sizeAddr) {
				sl_int32 ret = (sl_int32)(::sendto(m_socket, (char*)buf, (int)size, 0, (sockaddr*)&addr, (int)sizeAddr));
				return _processResult(ret);
			} else {
				_setError(SocketError::Invalid);
			}
		} else {
			_setError(SocketError::Closed);
		}
		return SLIB_IO_ERROR;
	}

	sl_int32 Socket::receiveFrom(SocketAddress& address, void* buf, sl_size _size) const noexcept
	{
		if (isOpened()) {
			int size = (int)_size;
			if (!size) {
				return SLIB_IO_EMPTY_CONTENT;
			}
			sockaddr_storage addr = { 0 };
			socklen_t lenAddr = sizeof(addr);
			sl_int32 ret = (sl_int32)(::recvfrom(m_socket, (char*)buf, size, 0, (sockaddr*)&addr, &lenAddr));
			if (ret >= 0) {
				if (address.setSystemSocketAddress(&addr)) {
					return ret;
				} else {
					_setError(SocketError::Invalid);
					return SLIB_IO_ERROR;
				}
			}
			return _processError();
		} else {
			_setError(SocketError::Closed);
		}
		return SLIB_IO_ERROR;
	}

#ifdef SLIB_PLATFORM_IS_WINDOWS
	namespace winsock
	{
		LPFN_WSARECVMSG GetWSARecvMsg()
		{
			LPFN_WSARECVMSG ret = NULL;
			SOCKET sock = ::socket(AF_INET, SOCK_DGRAM, 0);
			if (sock != (SOCKET)-1) {
				GUID guid = WSAID_WSARECVMSG;
				DWORD dwBytes = 0;
				LPFN_WSARECVMSG func;
				if (WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &func, sizeof(func), &dwBytes, NULL, NULL) != SOCKET_ERROR) {
					ret = func;
				}
				closesocket(sock);
			}
			return ret;
		}
	}
#endif

	// setReceivingPacketInformation (setReceivingIPv6PacketInformation) is required
	sl_int32 Socket::receiveFrom(sl_uint32& interfaceIndex, IPAddress& dst, SocketAddress& src, void* buf, sl_size _size, sl_bool flagFallbackToGenericReceive) const noexcept
	{
#ifdef SLIB_PLATFORM_IS_WINDOWS
		static LPFN_WSARECVMSG fnRecvMsg = NULL;
		if (!fnRecvMsg) {
			fnRecvMsg = winsock::GetWSARecvMsg();
			if (!fnRecvMsg) {
				if (flagFallbackToGenericReceive) {
					interfaceIndex = 0;
					dst.setNone();
					return receiveFrom(src, buf, _size);
				}
				_setError(SocketError::NotSupported);
				return SLIB_IO_ERROR;
			}
		}
#endif
		if (!(isOpened())) {
			_setError(SocketError::Closed);
			return SLIB_IO_ERROR;
		}
		int size = (int)_size;
		if (!size) {
			return SLIB_IO_EMPTY_CONTENT;
		}
		sl_uint8 cbuf[1024];
		sockaddr_storage addrSrc = { 0 };
#ifdef SLIB_PLATFORM_IS_WINDOWS
		WSAMSG msg = { 0 };
		WSABUF wbuf;
		msg.name = (sockaddr*)&addrSrc;
		msg.namelen = sizeof(addrSrc);
		wbuf.buf = (CHAR*)buf;
		wbuf.len = (ULONG)size;
		msg.lpBuffers = &wbuf;
		msg.dwBufferCount = 1;
		msg.Control.buf = (CHAR*)cbuf;
		msg.Control.len = sizeof(cbuf);
		DWORD dwRecv = 0;
		if (fnRecvMsg(m_socket, &msg, &dwRecv, NULL, NULL)) {
			return _processError();
		}
		sl_uint32 sizeReceived = (sl_uint32)dwRecv;
		sl_uint32 sizeSrc = (sl_uint32)(msg.namelen);
#else
		msghdr msg = { 0 };
		msg.msg_name = (sockaddr*)&addrSrc;
		msg.msg_namelen = sizeof(addrSrc);
		iovec iov;
		iov.iov_base = buf;
		iov.iov_len = (size_t)size;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		msg.msg_control = cbuf;
		msg.msg_controllen = sizeof(cbuf);
		sl_int32 sizeReceived = (sl_int32)(recvmsg(m_socket, &msg, 0));
		if (sizeReceived < 0) {
			return _processError();
		}
		sl_uint32 sizeSrc = (sl_uint32)(msg.msg_namelen);
#endif
		if (!(src.setSystemSocketAddress(&addrSrc, sizeSrc))) {
			_setError(SocketError::Invalid);
			return SLIB_IO_ERROR;
		}
		cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
		while (cmsg) {
			if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO) {
				in_pktinfo info;
				Base::copyMemory(&info, CMSG_DATA(cmsg), sizeof(info));
				interfaceIndex = (sl_uint32)(info.ipi_ifindex);
				dst = IPv4Address((sl_uint8*)(&(info.ipi_addr)));
				return sizeReceived;
			} else if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO) {
				in6_pktinfo info;
				Base::copyMemory(&info, CMSG_DATA(cmsg), sizeof(info));
				interfaceIndex = (sl_uint32)(info.ipi6_ifindex);
				dst = IPv6Address((sl_uint8*)(&(info.ipi6_addr)));
				return sizeReceived;
			}
			cmsg = CMSG_NXTHDR(&msg, cmsg);
		}
		if (flagFallbackToGenericReceive) {
			interfaceIndex = 0;
			dst.setNone();
			return receiveFrom(src, buf, _size);
		}
		_setError(SocketError::NotSupported);
		return SLIB_IO_ERROR;
	}

	sl_int32 Socket::receiveFrom(DomainSocketPath& path, void* buf, sl_size _size) const noexcept
	{
		if (isOpened()) {
			int size = (int)_size;
			if (!size) {
				return SLIB_IO_EMPTY_CONTENT;
			}
			sockaddr_un addr = { 0 };
			socklen_t lenAddr = sizeof(addr);
			sl_int32 ret = (sl_int32)(::recvfrom(m_socket, (char*)buf, size, 0, (sockaddr*)&addr, &lenAddr));
			if (ret >= 0) {
				if (path.setSystemSocketAddress(&addr, (sl_uint32)lenAddr)) {
					return ret;
				} else {
					_setError(SocketError::Invalid);
					return SLIB_IO_ERROR;
				}
			}
			return _processError();
		} else {
			_setError(SocketError::Closed);
		}
		return SLIB_IO_ERROR;
	}

	sl_int32 Socket::sendPacket(const void* buf, sl_size _size, const L2PacketInfo& info) const noexcept
	{
#if defined(SLIB_PLATFORM_IS_LINUX)
		if (isOpened()) {
			int size = (int)_size;
			if (!size) {
				return SLIB_IO_EMPTY_CONTENT;
			}
			sockaddr_ll addr = { 0 };
			addr.sll_family = AF_PACKET;
			addr.sll_protocol = htons((sl_uint16)(info.protocol));
			addr.sll_ifindex = info.iface;
			addr.sll_pkttype = (unsigned char)(info.type);
			sl_uint32 na = info.lenHardwareAddress;
			if (na > 8) {
				na = 8;
			}
			addr.sll_halen = na;
			Base::copyMemory(addr.sll_addr, info.hardwareAddress, na);
			sl_int32 ret = (sl_int32)(::sendto(m_socket, (char*)buf, size, 0, (sockaddr*)&addr, sizeof(addr)));
			return _processResult(ret);
		} else {
			_setError(SocketError::Closed);
		}
#else
		_setError(SocketError::NotSupported);
#endif
		return SLIB_IO_ERROR;
	}

	sl_int32 Socket::receivePacket(const void* buf, sl_size _size, L2PacketInfo& info) const noexcept
	{
#if defined(SLIB_PLATFORM_IS_LINUX)
		if (isOpened()) {
			int size = (int)_size;
			if (!size) {
				return SLIB_IO_EMPTY_CONTENT;
			}
			sockaddr_ll addr = { 0 };
			socklen_t lenAddr = sizeof(addr);
			sl_int32 ret = (sl_int32)(::recvfrom(m_socket, (char*)buf, size, 0, (sockaddr*)&addr, &lenAddr));
			if (ret >= 0) {
				if (addr.sll_family == AF_PACKET) {
					info.iface = addr.sll_ifindex;
					info.protocol = (EtherType)(ntohs(addr.sll_protocol));
					info.type = (L2PacketType)(addr.sll_pkttype);
					sl_uint32 na = addr.sll_halen;
					if (na > 8) {
						na = 8;
					}
					Base::copyMemory(info.hardwareAddress, addr.sll_addr, na);
					info.lenHardwareAddress = na;
					return ret;
				} else {
					_setError(SocketError::Invalid);
					return SLIB_IO_ERROR;
				}
			}
			return _processError();
		} else {
			_setError(SocketError::Closed);
		}
#else
		_setError(SocketError::NotSupported);
#endif
		return SLIB_IO_ERROR;
	}

	namespace {
		static sl_bool SetNonBlocking(sl_socket fd, sl_bool flagEnable) noexcept
		{
#if defined(SLIB_PLATFORM_IS_WINDOWS)
			u_long flag = flagEnable ? 1 : 0;
			return !(ioctlsocket(fd, FIONBIO, &flag));
#else
#	if defined(FIONBIO)
			sl_int32 flag = flagEnable ? 1 : 0;
			return !(ioctl(fd, FIONBIO, &flag));
#	else
			return HandlePtr<File>(fd)->setNonBlocking(flagEnable);
#	endif
#endif
		}
	}

	sl_bool Socket::setNonBlockingMode(sl_bool flagEnable) const noexcept
	{
		if (isOpened()) {
			if (SetNonBlocking(m_socket, flagEnable)) {
				return sl_true;
			}
		}
		return sl_false;
	}

#if defined(SLIB_PLATFORM_IS_LINUX)
	namespace {
		static sl_bool SetPromiscuousMode(sl_socket fd, const char* deviceName, sl_bool flagEnable) noexcept
		{
			ifreq ifopts;
			Base::copyString(ifopts.ifr_name, deviceName, IFNAMSIZ - 1);
			int ret;
			ret = ioctl(fd, SIOCGIFFLAGS, &ifopts);
			if (!ret) {
				if (flagEnable) {
					ifopts.ifr_flags |= IFF_PROMISC;
				} else {
					ifopts.ifr_flags &= (~IFF_PROMISC);
				}
				ret = ioctl(fd, SIOCSIFFLAGS, &ifopts);
				if (!ret) {
					return sl_true;
				}
			}
			return sl_false;
		}
	}
#endif

	sl_bool Socket::setPromiscuousMode(const StringParam& _deviceName, sl_bool flagEnable) const noexcept
	{
#if defined(SLIB_PLATFORM_IS_LINUX)
		if (isOpened()) {
			StringCstr deviceName = _deviceName;
			if (deviceName.isNotEmpty()) {
				if (SetPromiscuousMode(m_socket, deviceName.getData(), flagEnable)) {
					return sl_true;
				}
			}
		}
#else
		_setError(SocketError::NotSupported);
#endif
		return sl_false;
	}

	sl_bool Socket::getLocalAddress(SocketAddress& out) const noexcept
	{
		if (isOpened()) {
			sockaddr_storage addr;
			socklen_t size = sizeof(addr);
			if (!(getsockname(m_socket, (sockaddr*)(&addr), &size))) {
				return out.setSystemSocketAddress(&addr);
			} else {
				_checkError();
			}
		} else {
			_setError(SocketError::Closed);
		}
		return sl_false;
	}

	sl_bool Socket::getRemoteAddress(SocketAddress& out) const noexcept
	{
		if (isOpened()) {
			sockaddr_storage addr;
			socklen_t size = sizeof(addr);
			if (!(getpeername(m_socket, (sockaddr*)(&addr), &size))) {
				return out.setSystemSocketAddress(&addr);
			} else {
				_checkError();
			}
		} else {
			_setError(SocketError::Closed);
		}
		return sl_false;
	}

	sl_bool Socket::getLocalPath(DomainSocketPath& out) const noexcept
	{
		if (isOpened()) {
			sockaddr_un addr;
			socklen_t size = sizeof(addr);
			if (!(getsockname(m_socket, (sockaddr*)(&addr), &size))) {
				return out.setSystemSocketAddress(&addr, (sl_uint32)size);
			} else {
				_checkError();
			}
		} else {
			_setError(SocketError::Closed);
		}
		return sl_false;
	}

	sl_bool Socket::getRemotePath(DomainSocketPath& out) const noexcept
	{
		if (isOpened()) {
			sockaddr_un addr;
			socklen_t size = sizeof(addr);
			if (!(getpeername(m_socket, (sockaddr*)(&addr), &size))) {
				return out.setSystemSocketAddress(&addr, (sl_uint32)size);
			} else {
				_checkError();
			}
		} else {
			_setError(SocketError::Closed);
		}
		return sl_false;
	}

	sl_bool Socket::setOption(int level, int option, const void* buf, sl_uint32 bufSize) const noexcept
	{
		if (isOpened()) {
			int ret = setsockopt(m_socket, level, option, (char*)buf, (int)bufSize);
			if (ret != SOCKET_ERROR) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool Socket::getOption(int level, int option, void* buf, sl_uint32 bufSize) const noexcept
	{
		if (isOpened()) {
			socklen_t len = (socklen_t)bufSize;
			int ret = getsockopt(m_socket, level, option, (char*)buf, &len);
			if (ret != SOCKET_ERROR) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool Socket::setOption(int level, int option, sl_uint32 value) const noexcept
	{
		return setOption(level, option, &value, 4);
	}

	sl_uint32 Socket::getOption(int level, int option) const noexcept
	{
		sl_uint32 v = 0;
		if (getOption(level, option, &v, 4)) {
			return v != 0;
		} else {
			return 0;
		}
	}

	sl_uint32 Socket::getSocketError() const noexcept
	{
		return getOption(SOL_SOCKET, SO_ERROR);
	}

	sl_bool Socket::setSendingBroadcast(sl_bool flagEnable) const noexcept
	{
		return setOption(SOL_SOCKET, SO_BROADCAST, flagEnable ? 1 : 0);
	}

	sl_bool Socket::isSendingBroadcast() const noexcept
	{
		return getOption(SOL_SOCKET, SO_BROADCAST) != 0;
	}

	sl_bool Socket::setUsingExclusiveAddress(sl_bool flagEnable) const noexcept
	{
#ifdef SO_EXCLUSIVEADDRUSE
		return setOption(SOL_SOCKET, SO_EXCLUSIVEADDRUSE, flagEnable ? 1 : 0);
#else
		return sl_false;
#endif
	}

	sl_bool Socket::isUsingExclusiveAddress() const noexcept
	{
#ifdef SO_EXCLUSIVEADDRUSE
		return getOption(SOL_SOCKET, SO_EXCLUSIVEADDRUSE) != 0;
#else
		return sl_false;
#endif
	}

	sl_bool Socket::setReusingAddress(sl_bool flagEnable) const noexcept
	{
		return setOption(SOL_SOCKET, SO_REUSEADDR, flagEnable ? 1 : 0);
	}

	sl_bool Socket::isReusingAddress() const noexcept
	{
		return getOption(SOL_SOCKET, SO_REUSEADDR) != 0;
	}

	sl_bool Socket::setReusingPort(sl_bool flagEnable) const noexcept
	{
#if defined(SLIB_PLATFORM_IS_WIN32) || defined(SLIB_PLATFORM_IS_ANDROID) || defined(SLIB_PLATFORM_IS_TIZEN)
		return setOption(SOL_SOCKET, SO_REUSEADDR, flagEnable ? 1 : 0);
#else
		return setOption(SOL_SOCKET, SO_REUSEPORT, flagEnable ? 1 : 0);
#endif
	}

	sl_bool Socket::isReusingPort() const noexcept
	{
#if defined(SLIB_PLATFORM_IS_WIN32) || defined(SLIB_PLATFORM_IS_ANDROID) || defined(SLIB_PLATFORM_IS_TIZEN)
		return getOption(SOL_SOCKET, SO_REUSEADDR) != 0;
#else
		return getOption(SOL_SOCKET, SO_REUSEPORT) != 0;
#endif
	}

	sl_bool Socket::setSendBufferSize(sl_uint32 size) const noexcept
	{
		return setOption(SOL_SOCKET, SO_SNDBUF, size);
	}

	sl_uint32 Socket::getSendBufferSize() const noexcept
	{
		return getOption(SOL_SOCKET, SO_SNDBUF);
	}

	sl_bool Socket::setReceiveBufferSize(sl_uint32 size) const noexcept
	{
		return setOption(SOL_SOCKET, SO_RCVBUF, size);
	}

	sl_uint32 Socket::getReceiveBufferSize() const noexcept
	{
		return getOption(SOL_SOCKET, SO_RCVBUF);
	}

	sl_bool Socket::setSendTimeout(sl_uint32 size) const noexcept
	{
		return setOption(SOL_SOCKET, SO_SNDTIMEO, size);
	}

	sl_bool Socket::setReceiveTimeout(sl_uint32 size) const noexcept
	{
		return setOption(SOL_SOCKET, SO_RCVTIMEO, size);
	}

	sl_bool Socket::setIPv6Only(sl_bool flagEnable) const noexcept
	{
		return setOption(IPPROTO_IPV6, IPV6_V6ONLY, flagEnable ? 1 : 0);
	}

	sl_bool Socket::isIPv6Only() const noexcept
	{
		return getOption(IPPROTO_IPV6, IPV6_V6ONLY) != 0;
	}

	sl_bool Socket::setTcpNoDelay(sl_bool flagEnable) const noexcept
	{
		return setOption(IPPROTO_TCP, TCP_NODELAY, flagEnable ? 1 : 0);
	}

	sl_bool Socket::isTcpNoDelay() const noexcept
	{
		return getOption(IPPROTO_TCP, TCP_NODELAY) != 0;
	}

	sl_bool Socket::setTTL(sl_uint32 ttl) const noexcept
	{
		if (ttl > 255) {
			return sl_false;
		}
		return setOption(IPPROTO_IP, IP_TTL, ttl);
	}

	sl_uint32 Socket::getTTL() const noexcept
	{
		return getOption(IPPROTO_IP, IP_TTL);
	}

	sl_bool Socket::setIncludingHeader(sl_bool flagEnable) const noexcept
	{
		return setOption(IPPROTO_IP, IP_HDRINCL, flagEnable ? 1 : 0);
	}

	sl_bool Socket::isIncludingHeader() const noexcept
	{
		return getOption(IPPROTO_IP, IP_HDRINCL) != 0;
	}

	sl_bool Socket::setReceivingPacketInformation(sl_bool flagEnable) const noexcept
	{
		return setOption(IPPROTO_IP, IP_PKTINFO, flagEnable ? 1 : 0);
	}

	sl_bool Socket::isReceivingPacketInformation() const noexcept
	{
		return getOption(IPPROTO_IP, IP_PKTINFO) != 0;
	}

	sl_bool Socket::setReceivingIPv6PacketInformation(sl_bool flagEnable) const noexcept
	{
		return setOption(IPPROTO_IPV6, IPV6_RECVPKTINFO, flagEnable ? 1 : 0);
	}

	sl_bool Socket::isReceivingIPv6PacketInformation() const noexcept
	{
		return getOption(IPPROTO_IPV6, IPV6_RECVPKTINFO) != 0;
	}

	sl_bool Socket::joinMulticast(const IPv4Address& ipMulticast, const IPv4Address& ipInterface) const noexcept
	{
		ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = htonl(ipMulticast.getInt());
		mreq.imr_interface.s_addr = htonl(ipInterface.getInt());
		return setOption(IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	}

	sl_bool Socket::joinMulticast(const IPv4Address& ipMulticast, sl_uint32 interfaceIndex) const noexcept
	{
		ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = htonl(ipMulticast.getInt());
		mreq.imr_interface.s_addr = htonl(interfaceIndex);
		return setOption(IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	}

	sl_bool Socket::joinMulticast(const IPv6Address& ipMulticast, sl_uint32 interfaceIndex) const noexcept
	{
		ipv6_mreq mreq;
		ipMulticast.getBytes(&(mreq.ipv6mr_multiaddr));
		mreq.ipv6mr_interface = interfaceIndex;
		return setOption(IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	}

	sl_bool Socket::leaveMulticast(const IPv4Address& ipMulticast, const IPv4Address& ipInterface) const noexcept
	{
		ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = htonl(ipMulticast.getInt());
		mreq.imr_interface.s_addr = htonl(ipInterface.getInt());
		return setOption(IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
	}

	sl_bool Socket::leaveMulticast(const IPv4Address& ipMulticast, sl_uint32 interfaceIndex) const noexcept
	{
		ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = htonl(ipMulticast.getInt());
		mreq.imr_interface.s_addr = htonl(interfaceIndex);
		return setOption(IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
	}

	sl_bool Socket::leaveMulticast(const IPv6Address& ipMulticast, sl_uint32 interfaceIndex) const noexcept
	{
		ipv6_mreq mreq;
		ipMulticast.getBytes(&(mreq.ipv6mr_multiaddr));
		mreq.ipv6mr_interface = interfaceIndex;
		return setOption(IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
	}

	sl_bool Socket::setMulticastLoop(sl_bool flag) const noexcept
	{
		return setOption(IPPROTO_IP, IP_MULTICAST_LOOP, flag ? 1 : 0);
	}

	sl_bool Socket::isMulticastLoop() const noexcept
	{
		return getOption(IPPROTO_IP, IP_MULTICAST_LOOP) != 0;
	}

	sl_bool Socket::setIPv6MulticastLoop(sl_bool flag) const noexcept
	{
		return setOption(IPPROTO_IPV6, IPV6_MULTICAST_LOOP, flag ? 1 : 0);
	}

	sl_bool Socket::isIPv6MulticastLoop() const noexcept
	{
		return getOption(IPPROTO_IPV6, IPV6_MULTICAST_LOOP) != 0;
	}

	sl_bool Socket::setMulticastTTL(sl_uint32 ttl) const noexcept
	{
		if (ttl > 255) {
			return sl_false;
		}
		return setOption(IPPROTO_IP, IP_MULTICAST_TTL, ttl);
	}

	sl_uint32 Socket::getMulticastTTL() const noexcept
	{
		return getOption(IPPROTO_IP, IP_MULTICAST_TTL);
	}

	sl_bool Socket::setIPv6MulticastTTL(sl_uint32 ttl) const noexcept
	{
		if (ttl > 255) {
			return sl_false;
		}
		return setOption(IPPROTO_IPV6, IPV6_MULTICAST_HOPS, ttl);
	}

	sl_uint32 Socket::getIPv6MulticastTTL() const noexcept
	{
		return getOption(IPPROTO_IPV6, IPV6_MULTICAST_HOPS);
	}

	void Socket::initializeSocket() noexcept
	{
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		static sl_bool flagInit = sl_false;
		if (!flagInit) {
			SLIB_STATIC_SPINLOCKER(lock)
			if (!flagInit) {
				WSADATA wsaData;
				int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
				if (err != 0) {
					LogError("SOCKET", "WSA Startup failed");
				}
				flagInit = sl_true;
			}
		}
#else
		//signal(SIGPIPE, SIG_IGN);
#endif
	}

	SocketError Socket::getLastError() noexcept
	{
		return (SocketError)(System::getLastError());
	}

	String Socket::getLastErrorMessage() noexcept
	{
		return getErrorMessage(getLastError());
	}

	String Socket::getErrorMessage(SocketError error) noexcept
	{
		switch (error) {
		case SocketError::None:
			return sl_null;
		case SocketError::NetworkDown:
			SLIB_RETURN_STRING("NETDOWN - Network is down")
		case SocketError::NetworkReset:
			SLIB_RETURN_STRING("NETRESET - Network dropped connection on reset")
		case SocketError::ConnectionReset:
			SLIB_RETURN_STRING("CONNRESET - Connection reset by peer")
		case SocketError::ConnectionAbort:
			SLIB_RETURN_STRING("CONNABORTED - Software caused connection abort")
		case SocketError::ConnectionRefused:
			SLIB_RETURN_STRING("CONNREFUSED - Connection refused")
		case SocketError::Timeout:
			SLIB_RETURN_STRING("TIMEOUT - Connection timed out")
		case SocketError::NotSocket:
			SLIB_RETURN_STRING("NOTSOCK - Socket operation on nonsocket")
		case SocketError::AddressAlreadyInUse:
			SLIB_RETURN_STRING("ADDRINUSE - Address already in use")
		case SocketError::NoBufs:
			SLIB_RETURN_STRING("NOBUFS - No buffer space available")
		case SocketError::NoMem:
			SLIB_RETURN_STRING("NOMEM - Insufficient memory available")
		case SocketError::InProgress:
			SLIB_RETURN_STRING("INPROGRESS - Operation now in progress")
		case SocketError::DestinationAddressRequired:
			SLIB_RETURN_STRING("DESTADDRREQ - Destination address required")
		case SocketError::ProtocolFamilyNotSupported:
			SLIB_RETURN_STRING("PFNOSUPPORT - Protocol family not supported")
		case SocketError::AddressFamilyNotSupported:
			SLIB_RETURN_STRING("AFNOSUPPORT - Address family not supported by protocol family")
		case SocketError::AddressNotAvailable:
			SLIB_RETURN_STRING("ADDRNOTAVAIL - Cannot assign requested address")
		case SocketError::NotConnected:
			SLIB_RETURN_STRING("NOTCONN - Socket is not connected")
		case SocketError::Shutdown:
			SLIB_RETURN_STRING("SHUTDOWN - Cannot send after socket shutdown")
		case SocketError::Access:
			SLIB_RETURN_STRING("ACCESS - Permission denied")
		case SocketError::NotPermitted:
			SLIB_RETURN_STRING("EPERM - Operation not permitted")
		case SocketError::Invalid:
			SLIB_RETURN_STRING("EINVAL - An invalid argument was supplied")
		case SocketError::Fault:
			SLIB_RETURN_STRING("EFAULT - Invalid pointer address")
		case SocketError::Interrupted:
			SLIB_RETURN_STRING("EINTR - Operation is interrupted")
		case SocketError::Closed:
			SLIB_RETURN_STRING("Closed Socket")
		case SocketError::UnexpectedResult:
			SLIB_RETURN_STRING("Unexpected Result")
		case SocketError::NotSupported:
			SLIB_RETURN_STRING("Not Supported")
		default:
			break;
		}
		if (error >= SocketError::Unknown) {
			return "Unknown System Error: " + String::fromUint32((sl_uint32)error - (sl_uint32)(SocketError::Unknown));
		}
		return "Not Defined Error: " + String::fromUint32((sl_uint32)error);
	}

	void Socket::clearError() noexcept
	{
		_setError(SocketError::None);
	}

	SocketError Socket::_setError(SocketError code) noexcept
	{
		System::setLastError((sl_uint32)code);
		return code;
	}

	SocketError Socket::_checkError() noexcept
	{
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		int err = WSAGetLastError();
#else
		int err = errno;
#endif
		SocketError ret = SocketError::None;

		switch (err) {
#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAEWOULDBLOCK:
#else
			case EWOULDBLOCK:
#	if (EWOULDBLOCK != EAGAIN)
			case EAGAIN:
#	endif
#endif
				ret = _setError(SocketError::WouldBlock);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAENETDOWN:
				ret = _setError(SocketError::NetworkDown);
				break;
#endif

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAENETRESET:
				ret = _setError(SocketError::NetworkReset);
				break;
#endif

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAECONNRESET:
#else
			case ECONNRESET:
#endif
				ret = _setError(SocketError::ConnectionReset);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAECONNABORTED:
#else
			case ECONNABORTED:
#endif
				ret = _setError(SocketError::ConnectionAbort);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAECONNREFUSED:
#else
			case ECONNREFUSED:
#endif
				ret = _setError(SocketError::ConnectionRefused);
				break;


#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAETIMEDOUT:
#else
			case ETIMEDOUT:
#endif
				ret = _setError(SocketError::Timeout);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAENOTSOCK:
#else
			case ENOTSOCK:
			case EBADF:
#endif
				ret = _setError(SocketError::NotSocket);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAEINVAL:
#else
			case EINVAL:
#endif
				ret = _setError(SocketError::Invalid);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAEFAULT:
#else
			case EFAULT:
#endif
				ret = _setError(SocketError::Fault);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAEADDRINUSE:
#else
			case EADDRINUSE:
#endif
				ret = _setError(SocketError::AddressAlreadyInUse);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAENOBUFS:
#else
			case ENOBUFS:
#endif
				ret = _setError(SocketError::NoBufs);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSA_NOT_ENOUGH_MEMORY:
#else
			case ENOMEM:
#endif
				ret = _setError(SocketError::NoMem);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAEINPROGRESS:
#else
			case EINPROGRESS:
#endif
				ret = _setError(SocketError::InProgress);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAEDESTADDRREQ:
#else
			case EDESTADDRREQ:
#endif
				ret = _setError(SocketError::DestinationAddressRequired);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAEPFNOSUPPORT:
#else
			case EPFNOSUPPORT:
#endif
				ret = _setError(SocketError::ProtocolFamilyNotSupported);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAEAFNOSUPPORT:
#else
			case EAFNOSUPPORT:
#endif
				ret = _setError(SocketError::AddressFamilyNotSupported);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAEADDRNOTAVAIL:
#else
			case EADDRNOTAVAIL:
#endif
				ret = _setError(SocketError::AddressNotAvailable);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAENOTCONN:
#else
			case ENOTCONN:
#endif
				ret = _setError(SocketError::NotConnected);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAESHUTDOWN:
#else
			case ESHUTDOWN:
#endif
				ret = _setError(SocketError::Shutdown);
				break;

#if defined(SLIB_PLATFORM_IS_WINDOWS)
			case WSAEACCES:
#else
			case EACCES:
#endif
				ret = _setError(SocketError::Access);
				break;

#if defined(SLIB_PLATFORM_IS_UNIX)
			case EPERM:
				ret = _setError(SocketError::NotPermitted);
				break;
			case EINTR:
				ret = _setError(SocketError::Interrupted);
				break;
#endif

			default:
				ret = _setError((SocketError)((sl_uint32)(SocketError::Unknown) + err));
				break;
		}
		return ret;
	}

	sl_int32 Socket::_processResult(sl_int32 ret) noexcept
	{
		if (ret > 0) {
			return ret;
		} else {
			if (ret) {
				return _processError();
			} else {
				return SLIB_IO_ENDED;
			}
		}
	}

	sl_int32 Socket::_processError() noexcept
	{
		SocketError err = _checkError();
		if (err == SocketError::WouldBlock || err == SocketError::Interrupted) {
			return SLIB_IO_WOULD_BLOCK;
		} else {
			return SLIB_IO_ERROR;
		}
	}

}
