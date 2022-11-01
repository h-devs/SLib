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

#include "slib/network/socket_address.h"

#include "slib/core/setting.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	pragma comment(lib, "ws2_32.lib")
#else
#	include <unistd.h>
#	include <sys/socket.h>
#	if defined(SLIB_PLATFORM_IS_LINUX)
#		include <linux/tcp.h>
#	else
#		include <netinet/tcp.h>
#	endif
#	include <netinet/in.h>
#endif

namespace slib
{

	namespace priv
	{
		namespace socket_address
		{

			template <class CHAR>
			static sl_reg ParseAddress(SocketAddress* obj, const CHAR* str, sl_size pos, sl_size posEnd) noexcept
			{
				if (pos >= posEnd) {
					return SLIB_PARSE_ERROR;
				}
				IPAddress ip;
				if (*str == '[') {
					IPv6Address addr;
					pos++;
					pos = IPv6Address::parse(&addr, str, pos, posEnd);
					if (pos == SLIB_PARSE_ERROR || pos >= posEnd) {
						return SLIB_PARSE_ERROR;
					}
					if (str[pos] != ']') {
						return SLIB_PARSE_ERROR;
					}
					pos++;
					ip = addr;
				} else {
					IPv4Address addr;
					pos = IPv4Address::parse(&addr, str, pos, posEnd);
					if (pos == SLIB_PARSE_ERROR) {
						return SLIB_PARSE_ERROR;
					}
					ip = addr;
				}
				if (pos >= posEnd) {
					return SLIB_PARSE_ERROR;
				}
				if (str[pos] != ':') {
					return SLIB_PARSE_ERROR;
				}
				pos++;
				sl_uint32 port;
				pos = StringTypeFromCharType<CHAR>::Type::parseUint32(10, &port, str, pos, posEnd);
				if (pos == SLIB_PARSE_ERROR) {
					return SLIB_PARSE_ERROR;
				}
				if (port >> 16) {
					return SLIB_PARSE_ERROR;
				}
				if (obj) {
					obj->ip = ip;
					obj->port = (sl_uint16)port;
				}
				return pos;
			}

			template <class VIEW>
			static sl_bool SetHostAddress(SocketAddress& addr, const VIEW& str)
			{
				sl_reg index = str.lastIndexOf(':');
				if (index < 0) {
					addr.port = 0;
					return addr.ip.setHostName(str);
				} else {
					sl_uint32 _port = str.substring(index + 1).parseUint32();
					if (_port >> 16) {
						return sl_false;
					}
					addr.port = (sl_uint16)(_port);
					return addr.ip.setHostName(str.substring(0, index));
				}
			}

		}
	}

	using namespace priv::socket_address;

	const SocketAddress::_socket_address SocketAddress::_none = { { IPAddressType::None, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, 0 };

	SocketAddress::SocketAddress(const StringParam& str) noexcept
	{
		if (!(parse(str))) {
			setNone();
		}
	}

	void SocketAddress::setNone() noexcept
	{
		ip.setNone();
		port = 0;
	}

	sl_bool SocketAddress::isValid() const noexcept
	{
		return ip.isNotNone() && port != 0;
	}

	sl_bool SocketAddress::isInvalid() const noexcept
	{
		return ip.isNone() || !port;
	}

	sl_uint32 SocketAddress::getSystemSocketAddress(void* addr) const noexcept
	{
		if (ip.isIPv4()) {
			sockaddr_in& out = *((sockaddr_in*)addr);
			Base::zeroMemory(&out, sizeof(sockaddr_in));
			out.sin_family = AF_INET;
			ip.getIPv4().getBytes(&(out.sin_addr));
			out.sin_port = htons(port);
			return sizeof(sockaddr_in);
		} else if (ip.isIPv6()) {
			sockaddr_in6& out = *((sockaddr_in6*)addr);
			Base::zeroMemory(&out, sizeof(sockaddr_in6));
			out.sin6_family = AF_INET6;
			ip.getIPv6().getBytes(&(out.sin6_addr));
			out.sin6_port = htons(port);
			return sizeof(sockaddr_in6);
		}
		return 0;
	}

	sl_bool SocketAddress::setSystemSocketAddress(const void* _in, sl_uint32 size) noexcept
	{
		sockaddr_storage& in = *((sockaddr_storage*)_in);
		if (in.ss_family == AF_INET) {
			if (size == 0 || size == sizeof(sockaddr_in)) {
				sockaddr_in& addr = *((sockaddr_in*)&in);
				ip = IPv4Address((sl_uint8*)&(addr.sin_addr));
				port = ntohs(addr.sin_port);
				return sl_true;
			}
		} else if (in.ss_family == AF_INET6) {
			if (size == 0 || size == sizeof(sockaddr_in6)) {
				sockaddr_in6& addr = *((sockaddr_in6*)&in);
				ip = IPv6Address((sl_uint8*)&(addr.sin6_addr));
				port = ntohs(addr.sin6_port);
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool SocketAddress::setHostAddress(const StringParam& address) noexcept
	{
		if (address.isEmpty()) {
			return sl_false;
		}
		if (address.is8BitsStringType()) {
			return SetHostAddress(*this, StringData(address));
		} else if (address.is16BitsStringType()) {
			return SetHostAddress(*this, StringData16(address));
		} else {
			return SetHostAddress(*this, StringData32(address));
		}
	}

	sl_compare_result SocketAddress::compare(const SocketAddress& other) const noexcept
	{
		sl_compare_result c = ip.compare(other.ip);
		if (c == 0) {
			return ComparePrimitiveValues(port, other.port);
		}
		return c;
	}

	sl_bool SocketAddress::equals(const SocketAddress& other) const noexcept
	{
		return port == other.port && ip.equals(other.ip);
	}

	sl_size SocketAddress::getHashCode() const noexcept
	{
		return Rehash64ToSize((((sl_uint64)port) << 32) ^ ip.getHashCode());
	}

	String SocketAddress::toString() const noexcept
	{
		if (ip.isIPv4()) {
			if (port) {
				return String::concat(ip.toString(), ":", String::fromUint32(port));
			} else {
				return ip.toString();
			}
		} else if (ip.isIPv6()) {
			if (port) {
				return String::concat("[", ip.toString(), "]:", String::fromUint32(port));
			} else {
				return ip.toString();
			}
		} else {
			if (port) {
				return ":" + String::fromUint32(port);
			} else {
				return String::null();
			}
		}
	}

	SLIB_DEFINE_CLASS_PARSE_MEMBERS(SocketAddress, ParseAddress)

	sl_bool SocketAddress::parseIPv4Range(const StringParam& str, IPv4Address* from, IPv4Address* to) noexcept
	{
		return IPv4Address::parseRange(str, from, to);
	}

	sl_bool SocketAddress::parsePortRange(const StringParam& str, sl_uint16* from, sl_uint16* to) noexcept
	{
		sl_uint32 n1, n2;
		if (SettingUtil::parseUint32Range(str, &n1, &n2)) {
			if (n1 >> 16) {
				return sl_false;
			}
			if (n2 >> 16) {
				return sl_false;
			}
			if (from) {
				*from = (sl_uint16)n1;
			}
			if (to) {
				*to = (sl_uint16)n2;
			}
			return sl_true;
		}
		return sl_false;
	}

	SocketAddress& SocketAddress::operator=(const StringParam& str) noexcept
	{
		if (!(parse(str))) {
			setNone();
		}
		return *this;
	}

}
