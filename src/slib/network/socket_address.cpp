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

#include "slib/network/socket_address.h"

#include "slib/network/json_conv.h"
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
	const SocketAddress::_socket_address SocketAddress::_none = { { IPAddressType::None, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, 0 };
	
	
	SocketAddress::SocketAddress(const String& str) noexcept
	{
		setString(str);
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
		return ip.isNone() || port == 0;
	}
	
	sl_compare_result SocketAddress::compare(const SocketAddress& other) const noexcept
	{
		sl_compare_result c = ip.compare(other.ip);
		if (c == 0) {
			return ComparePrimitiveValues(port, other.port);
		}
		return c;
	}
	
	sl_size SocketAddress::getHashCode() const noexcept
	{
		return Rehash64ToSize((((sl_uint64)port) << 32) ^ ip.getHashCode());
	}
	
	String SocketAddress::toString() const noexcept
	{
		if (ip.isIPv4()) {
			if (port) {
				return ip.toString() + ":" + String::fromUint32(port);
			} else {
				return ip.toString();
			}
		} else if (ip.isIPv6()) {
			if (port) {
				return "[" + ip.toString() + "]:" + String::fromUint32(port);
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
	
	sl_bool SocketAddress::setString(const String& str) noexcept
	{
		if (parse(str)) {
			return sl_true;
		} else {
			setNone();
			return sl_false;
		}
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
			IPv6Address ipv6 = ip.getIPv6();
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
	
	sl_bool SocketAddress::setHostAddress(const String& address) noexcept
	{
		sl_reg index = address.lastIndexOf(':');
		if (index < 0) {
			port = 0;
			return ip.setHostName(address);
		} else {
			sl_uint32 _port = address.substring(index + 1).parseUint32();
			if (_port >> 16) {
				return sl_false;
			}
			port = (sl_uint16)(_port);
			return ip.setHostName(address.substring(0, index));
		}
	}
	
	namespace priv
	{
		namespace socket_address
		{
			template <class CT>
			static sl_reg parse(SocketAddress* obj, const CT* sz, sl_size pos, sl_size posEnd) noexcept
			{
				if (pos >= posEnd) {
					return SLIB_PARSE_ERROR;
				}
				IPAddress ip;
				if (sz[0] == '[') {
					IPv6Address addr;
					pos++;
					pos = Parser<IPv6Address, CT>::parse(&addr, sz, pos, posEnd);
					if (pos == SLIB_PARSE_ERROR || pos >= posEnd) {
						return SLIB_PARSE_ERROR;
					}
					if (sz[pos] != ']') {
						return SLIB_PARSE_ERROR;
					}
					pos++;
					ip = addr;
				} else {
					IPv4Address addr;
					pos = Parser<IPv4Address, CT>::parse(&addr, sz, pos, posEnd);
					if (pos == SLIB_PARSE_ERROR) {
						return SLIB_PARSE_ERROR;
					}
					ip = addr;
				}
				if (pos >= posEnd) {
					return SLIB_PARSE_ERROR;
				}
				if (sz[pos] != ':') {
					return SLIB_PARSE_ERROR;
				}
				pos++;
				sl_uint32 port;
				pos = StringTypeFromCharType<CT>::Type::parseUint32(10, &port, sz, pos, posEnd);
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
		}
	}
	
	template <>
	sl_reg Parser<SocketAddress, sl_char8>::parse(SocketAddress* _out, const sl_char8 *sz, sl_size posBegin, sl_size posEnd) noexcept
	{
		return priv::socket_address::parse(_out, sz, posBegin, posEnd);
	}
	
	template <>
	sl_reg Parser<SocketAddress, sl_char16>::parse(SocketAddress* _out, const sl_char16 *sz, sl_size posBegin, sl_size posEnd) noexcept
	{
		return priv::socket_address::parse(_out, sz, posBegin, posEnd);
	}
	
	sl_bool SocketAddress::parseIPv4Range(const String& str, IPv4Address* _from, IPv4Address* _to) noexcept
	{
		IPv4Address from;
		IPv4Address to;
		sl_reg index = str.indexOf('-');
		if (index > 0) {
			if (from.parse(str.substring(0, index))) {
				if (to.parse(str.substring(index + 1))) {
					if (to >= from) {
						if (_from) {
							*_from = from;
						}
						if (_to) {
							*_to = to;
						}
						return sl_true;
					}
				}
			}
		} else {
			if (from.parse(str)) {
				to = from;
				if (_from) {
					*_from = from;
				}
				if (_to) {
					*_to = to;
				}
				return sl_true;
			}
		}
		return sl_false;
	}
	
	sl_bool SocketAddress::parsePortRange(const String& str, sl_uint16* from, sl_uint16* to) noexcept
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
		}
		return sl_false;
	}
	
	SocketAddress& SocketAddress::operator=(const String& str) noexcept
	{
		setString(str);
		return *this;
	}
	
	sl_bool SocketAddress::operator==(const SocketAddress& other) const noexcept
	{
		return port == other.port && ip == other.ip;
	}
	
	sl_bool SocketAddress::operator!=(const SocketAddress& other) const noexcept
	{
		return port != other.port || ip != other.ip;
	}
	
	sl_compare_result Compare<SocketAddress>::operator()(const SocketAddress& a, const SocketAddress& b) const noexcept
	{
		return a.compare(b);
	}
	
	sl_bool Equals<SocketAddress>::operator()(const SocketAddress& a, const SocketAddress& b) const noexcept
	{
		return a == b;
	}
	
	sl_size Hash<SocketAddress>::operator()(const SocketAddress& a) const noexcept
	{
		return a.getHashCode();
	}
	

	void FromJson(const Json& json, MacAddress& _out)
	{
		_out.setString(json.getString());
	}

	void ToJson(Json& json, const MacAddress& _in)
	{
		json = _in.toString();
	}

	void FromJson(const Json& json, IPv4Address& _out)
	{
		_out.setString(json.getString());
	}

	void ToJson(Json& json, const IPv4Address& _in)
	{
		json = _in.toString();
	}

	void FromJson(const Json& json, IPv6Address& _out)
	{
		_out.setString(json.getString());
	}

	void ToJson(Json& json, const IPv6Address& _in)
	{
		json = _in.toString();
	}

	void FromJson(const Json& json, IPAddress& _out)
	{
		_out.setString(json.getString());
	}

	void ToJson(Json& json, const IPAddress& _in)
	{
		json = _in.toString();
	}

	void FromJson(const Json& json, SocketAddress& _out)
	{
		_out.setString(json.getString());
	}
	
	void ToJson(Json& json, const SocketAddress& _in)
	{
		json = _in.toString();
	}

}
