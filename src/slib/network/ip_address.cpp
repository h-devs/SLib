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

#include "slib/network/ip_address.h"

#include "slib/network/os.h"
#include "slib/core/math.h"

namespace slib
{

	SLIB_ALIGN(8) const sl_uint8 IPv4Address::_zero[4] = { 0, 0, 0, 0 };

	IPv4Address::IPv4Address(const StringParam& address) noexcept
	{
		if (!(parse(address))) {
			setZero();
		}
	}

	void IPv4Address::setElements(sl_uint8 _a, sl_uint8 _b, sl_uint8 _c, sl_uint8 _d) noexcept
	{
		a = _a;
		b = _b;
		c = _c;
		d = _d;
	}

	void IPv4Address::getBytes(void* _bytes) const noexcept
	{
		sl_uint8* bytes = (sl_uint8*)_bytes;
		bytes[0] = a;
		bytes[1] = b;
		bytes[2] = c;
		bytes[3] = d;
	}

	void IPv4Address::setBytes(const void* _bytes) noexcept
	{
		const sl_uint8* bytes = (const sl_uint8*)_bytes;
		a = bytes[0];
		b = bytes[1];
		c = bytes[2];
		d = bytes[3];
	}

	void IPv4Address::setZero() noexcept
	{
		a = 0;
		b = 0;
		c = 0;
		d = 0;
	}

	sl_bool IPv4Address::isLoopback() const noexcept
	{
		return a == 127;
	}

	sl_bool IPv4Address::isLinkLocal() const noexcept
	{
		return a == 169 && b == 254;
	}

	sl_bool IPv4Address::isMulticast() const noexcept
	{
		return a >= 224 && a <= 239;
	}

	sl_bool IPv4Address::isBroadcast() const noexcept
	{
		return getInt() == 0xFFFFFFFF;
	}

	sl_bool IPv4Address::isHost() const noexcept
	{
		sl_uint32 n = getInt();
		return n != 0 && a < 224 && a != 127;
	}

	sl_bool IPv4Address::isPrivate() const noexcept
	{
		sl_uint32 n = getInt();
		// 10.0.0.0 - 10.255.255.255
		if (n >= 0x0A000000 && n <= 0x0AFFFFFF) {
			return sl_true;
		}
		// 172.16.0.0 - 172.31.255.255
		if (n >= 0xAC100000 && n <= 0xAC1FFFFF) {
			return sl_true;
		}
		// 192.168.0.0 - 192.168.255.255
		if (n >= 0xC0A80000 && n <= 0xC0A8FFFF) {
			return sl_true;
		}
		return sl_false;
	}

	sl_bool IPv4Address::isSpecial() const noexcept
	{
		if (!a || a == 10 || a == 127) {
			// 0.0.0.0-0.255.255.255		Software (Current network)
			// 10.0.0.0-10.255.255.255		Private
			// 127.0.0.0-127.255.255.255	Loopback
			return sl_true;
		}
		sl_uint32 n = getInt();
		if ((n & 0xFFC00000) == 0x64400000) {
			// 100.64.0.0/10 (100.64.0.0-100.127.255.255)
			// Private (Shared address space for communications between a service provider and its subscribers when using a carrier-grade NAT)
			return sl_true;
		}
		sl_uint32 n16 = n >> 16;
		if (n16 == 0xA9FE) {
			// 169.254.0.0-169.254.255.255	Link-Local
			return sl_true;
		}
		if ((n & 0xFFF00000) == 0xAC100000) {
			// 172.16.0.0/12 (172.16.0.0-172.31.255.255)	Private
			return sl_true;
		}
		sl_uint32 n24 = n >> 8;
		if (n24 == 0xC00000 || n24 == 0xC00002 || n24 == 0xC05863) {
			// 192.0.0.0-192.0.0.255		Private (IETF Protocol Assignments)
			// 192.0.2.0-192.0.2.255		Documentation (Assigned as TEST-NET-1, documentation and examples)
			// 192.88.99.0-192.88.99.255	Reserved
			return sl_true;
		}
		if (n16 == 0xC0A8) {
			// 192.168.0.0-192.168.255.255	Private
			return sl_true;
		}
		if ((n & 0xFFFE0000) == 0xC6120000) {
			// 198.18.0.0/15 (198.18.0.0-198.19.255.255)
			// Private (Used for benchmark testing of inter-network communications between two separate subnets)
			return sl_true;
		}
		if (n24 == 0xC63364 || n24 == 0xCB0071) {
			// 198.51.100.0-198.51.100.255	Documentation (Assigned as TEST-NET-2, documentation and examples)
			// 203.0.113.0-203.0.113.255	Documentation (Assigned as TEST-NET-3, documentation and examples)
			return sl_true;
		}
		if (a >= 224) {
			// 224.0.0.0-239.255.255.255	Multicast
			// 240.0.0.0-255.255.255.254	Reserved
			// 255.255.255.255				Broadcast
			return sl_true;
		}
		return sl_false;
	}

	void IPv4Address::makeNetworkMask(sl_uint32 networkPrefixLength) noexcept
	{
		int p = networkPrefixLength;
		if (p >= 32) {
			a = 255;
			b = 255;
			c = 255;
			d = 255;
		} else if (p == 31) {
			a = 255;
			b = 255;
			c = 255;
			d = 254;
		} else if (p == 0) {
			a = 0;
			b = 0;
			c = 0;
			d = 0;
		} else {
			int t = ((1 << (p + 1)) - 1) << (32 - p);
			a = (t >> 24) & 255;
			b = (t >> 16) & 255;
			c = (t >> 8) & 255;
			d = t & 255;
		}
	}

	sl_uint32 IPv4Address::getNetworkPrefixLengthFromMask() const noexcept
	{
		sl_uint32 t = getInt();
		if (t == 0) {
			return 0;
		}
		return 32 - Math::getLeastSignificantBits(t);
	}

	sl_bool IPv4Address::setHostName(const StringParam& hostName) noexcept
	{
		*this = Network::getIPv4AddressFromHostName(hostName);
		return isNotZero();
	}

	String IPv4Address::toString() const noexcept
	{
		sl_char8 buf[16];
		sl_char8* p = buf;
		const sl_uint8* v = &a;
		for (int i = 0; i < 4; i++) {
			if (i > 0) {
				*(p++) = '.';
			}
			sl_uint8 n = v[i];
			if (n >= 100) {
				*(p++) = '0' + (n / 100);
				n %= 100;
				*(p++) = '0' + (n / 10);
				n %= 10;
			} else if (n >= 10) {
				*(p++) = '0' + (n / 10);
				n %= 10;
			}
			*(p++) = '0' + n;
		}
		return String(buf, p - buf);
	}

	namespace priv
	{
		namespace ipv4address
		{

			template <class CT>
			SLIB_INLINE static sl_reg Parse(IPv4Address* obj, const CT* sz, sl_size i, sl_size n) noexcept
			{
				if (i >= n) {
					return SLIB_PARSE_ERROR;
				}
				int v[4];
				for (int k = 0; k < 4; k++) {
					int t = 0;
					int s = 0;
					for (; i < n; i++) {
						int h = sz[i];
						if (h >= '0' && h <= '9') {
							s = s * 10 + (h - '0');
							if (s > 255) {
								return SLIB_PARSE_ERROR;
							}
							t++;
						} else {
							break;
						}
					}
					if (t == 0) {
						return SLIB_PARSE_ERROR;
					}
					if (k < 3) {
						if (i >= n || sz[i] != '.') {
							return SLIB_PARSE_ERROR;
						}
						i++;
					}
					v[k] = s;
				}
				if (obj) {
					obj->a = (sl_uint8)(v[0]);
					obj->b = (sl_uint8)(v[1]);
					obj->c = (sl_uint8)(v[2]);
					obj->d = (sl_uint8)(v[3]);
				}
				return i;
			}

			template <class VIEW>
			static sl_bool ParseRange(const VIEW& str, IPv4Address* _from, IPv4Address* _to) noexcept
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

		}
	}

	SLIB_DEFINE_CLASS_PARSE_MEMBERS(IPv4Address, priv::ipv4address::Parse)

	sl_bool IPv4Address::parseRange(const StringParam& str, IPv4Address* from, IPv4Address* to) noexcept
	{
		if (str.isEmpty()) {
			return sl_false;
		}
		if (str.is8BitsStringType()) {
			return priv::ipv4address::ParseRange(StringData(str), from, to);
		} else if (str.is16BitsStringType()) {
			return priv::ipv4address::ParseRange(StringData16(str), from, to);
		} else {
			return priv::ipv4address::ParseRange(StringData32(str), from, to);
		}
	}

	IPv4Address& IPv4Address::operator=(const StringParam& address) noexcept
	{
		if (!(parse(address))) {
			setZero();
		}
		return *this;
	}


	IPv4Address IPv4AddressInfo::getNetworkMask() const noexcept
	{
		IPv4Address ret;
		ret.makeNetworkMask(networkPrefixLength);
		return ret;
	}

	void IPv4AddressInfo::setNetworkMask(const IPv4Address& mask) noexcept
	{
		networkPrefixLength = mask.getNetworkPrefixLengthFromMask();
	}

	sl_compare_result IPv4AddressInfo::compare(const IPv4AddressInfo& b) const noexcept
	{
		return address.compare(b.address);
	}

	sl_bool IPv4AddressInfo::equals(const IPv4AddressInfo& b) const noexcept
	{
		return address.equals(b.address);
	}


	SLIB_ALIGN(8) const sl_uint8 IPv6Address::_zero[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	SLIB_ALIGN(8) const sl_uint8 IPv6Address::_loopback[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
	SLIB_ALIGN(8) const sl_uint8 IPv6Address::_loopback_linkLocal[16] = { 0xFE, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

	IPv6Address::IPv6Address() noexcept
	{
		Base::zeroMemory(m, 16);
	}

	IPv6Address::IPv6Address(sl_null_t) noexcept
	{
		Base::zeroMemory(m, 16);
	}

	IPv6Address::IPv6Address(const sl_uint16* s) noexcept
	{
		for (int i = 0; i < 8; i++) {
			m[i * 2] = (sl_uint8)(s[i] >> 8);
			m[i * 2 + 1] = (sl_uint8)(s[i]);
		}
	}

	IPv6Address::IPv6Address(sl_uint16 s0, sl_uint16 s1, sl_uint16 s2, sl_uint16 s3, sl_uint16 s4, sl_uint16 s5, sl_uint16 s6, sl_uint16 s7) noexcept
	{
		setElements(s0, s1, s2, s3, s4, s5, s6, s7);
	}

	IPv6Address::IPv6Address(const sl_uint8* b) noexcept
	{
		Base::copyMemory(m, b, 16);
	}

	IPv6Address::IPv6Address(const StringParam& address) noexcept
	{
		if (!(parse(address))) {
			setZero();
		}
	}

	IPv6Address::IPv6Address(const IPv4Address& ip) noexcept
	{
		setIPv4Transition(ip);
	}

	sl_uint16 IPv6Address::getElement(int index) const noexcept
	{
		int k = index << 1;
		return SLIB_MAKE_WORD(m[k], m[k + 1]);
	}

	void IPv6Address::setElement(int index, sl_uint16 s) noexcept
	{
		int k = index << 1;
		m[k] = (sl_uint8)(s >> 8);
		m[k + 1] = (sl_uint8)(s);
	}

	void IPv6Address::getElements(sl_uint16* s) const noexcept
	{
		for (int i = 0; i < 8; i++) {
			int k = i << 1;
			s[i] = SLIB_MAKE_WORD(m[k], m[k + 1]);
		}
	}

	void IPv6Address::setElements(const sl_uint16* s) noexcept
	{
		for (int i = 0; i < 8; i++) {
			int k = i << 1;
			m[k] = (sl_uint8)(s[i] >> 8);
			m[k + 1] = (sl_uint8)(s[i]);
		}
	}

	void IPv6Address::setElements(sl_uint16 s0, sl_uint16 s1, sl_uint16 s2, sl_uint16 s3, sl_uint16 s4, sl_uint16 s5, sl_uint16 s6, sl_uint16 s7) noexcept
	{
		m[0] = (sl_uint8)(s0 >> 8);
		m[1] = (sl_uint8)(s0);
		m[2] = (sl_uint8)(s1 >> 8);
		m[3] = (sl_uint8)(s1);
		m[4] = (sl_uint8)(s2 >> 8);
		m[5] = (sl_uint8)(s2);
		m[6] = (sl_uint8)(s3 >> 8);
		m[7] = (sl_uint8)(s3);
		m[8] = (sl_uint8)(s4 >> 8);
		m[9] = (sl_uint8)(s4);
		m[10] = (sl_uint8)(s5 >> 8);
		m[11] = (sl_uint8)(s5);
		m[12] = (sl_uint8)(s6 >> 8);
		m[13] = (sl_uint8)(s6);
		m[14] = (sl_uint8)(s7 >> 8);
		m[15] = (sl_uint8)(s7);
	}

	void IPv6Address::getBytes(void* bytes) const noexcept
	{
		Base::copyMemory(bytes, m, 16);
	}

	void IPv6Address::setBytes(const void* bytes) noexcept
	{
		Base::copyMemory(m, bytes, 16);
	}

	void IPv6Address::setZero() noexcept
	{
		Base::zeroMemory(m, 16);
	}

	sl_bool IPv6Address::isZero() const noexcept
	{
		return Base::equalsMemoryZero(m, 16);
	}

	sl_bool IPv6Address::isNotZero() const noexcept
	{
		return !(Base::equalsMemoryZero(m, 16));
	}

	sl_bool IPv6Address::isLoopback() const noexcept
	{
		return Base::equalsMemory(_loopback, m, 16) || Base::equalsMemory(_loopback_linkLocal, m, 16);
	}

	sl_bool IPv6Address::isLinkLocal() const noexcept
	{
		return m[0] == 0xFE && (m[1] & 0xC0) == 0x80;
	}
	
	IPv4Address IPv6Address::getIPv4Transition() const noexcept
	{
		if (isIPv4Transition()) {
			return {m[12], m[13], m[14], m[15]};
		} else {
			return {0, 0, 0, 0};
		}
	}

	void IPv6Address::setIPv4Transition(const IPv4Address& ip) noexcept
	{
		setElements(0, 0, 0, 0, 0, 0xFFFF, (sl_uint16)((ip.a << 8) | ip.b), (sl_uint16)((ip.c << 8) | ip.d));
	}

	sl_bool IPv6Address::isIPv4Transition() const noexcept
	{
		return m[0] == 0 && m[1] == 0 && m[2] == 0 && m[3] == 0 && m[4] == 0 && m[5] == 0 && m[6] == 0 && m[7] == 0 && m[8] == 0 && m[9] == 0 && m[10] == 255 && m[11] == 255;
	}

	sl_bool IPv6Address::setHostName(const StringParam& hostName) noexcept
	{
		*this = Network::getIPv6AddressFromHostName(hostName);
		return isNotZero();
	}

	sl_compare_result IPv6Address::compare(const IPv6Address& other) const noexcept
	{
		return Base::compareMemory(m, other.m, 16);
	}

	sl_bool IPv6Address::equals(const IPv6Address& other) const noexcept
	{
		return Base::equalsMemory(m, other.m, 16);
	}

	sl_size IPv6Address::getHashCode() const noexcept
	{
		return Rehash64ToSize(SLIB_MAKE_QWORD(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7]) ^ SLIB_MAKE_QWORD(m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]));
	}

	String IPv6Address::toString() const noexcept
	{
		sl_char8 buf[40];
		sl_char8* p = buf;
		const char* hex = "0123456789abcdef";
		for (int i = 0; i < 8; i++) {
			if (i > 0) {
				*(p++) = ':';
			}
			sl_uint16 n = getElement(i);
			*(p++) = hex[n >> 12];
			*(p++) = hex[(n >> 8) & 15];
			*(p++) = hex[(n >> 4) & 15];
			*(p++) = hex[n & 15];
		}
		return String(buf, p - buf);
	}

	namespace priv
	{
		namespace ipv6address
		{
			template <class CT>
			SLIB_INLINE static sl_reg Parse(IPv6Address* obj, const CT* sz, sl_size i, sl_size n) noexcept
			{
				if (i >= n) {
					return SLIB_PARSE_ERROR;
				}
				int k = 0;
				sl_uint16 v[8];
				int skip_START = -1;
				for (k = 0; k < 8;) {
					int t = 0;
					int s = 0;
					for (; i < n; i++) {
						int h = sz[i];
						int x = 0;
						if (h >= '0' && h <= '9') {
							x = h - '0';
						} else if (h >= 'A' && h <= 'F') {
							x = h - ('A' - 10);
						} else if (h >= 'a' && h <= 'f') {
							x = h - ('a' - 10);
						} else {
							break;
						}
						s = (s << 4) | x;
						if (s > 0x10000) {
							return SLIB_PARSE_ERROR;
						}
						t++;
					}
					if (i >= n || sz[i] != ':') {
						if (t == 0) {
							if (skip_START != k) {
								return SLIB_PARSE_ERROR;
							}
						} else {
							v[k] = (sl_uint16)s;
							k++;
						}
						break;
					}
					if (t == 0) {
						if (k == 0) {
							if (i < n - 1 && sz[i + 1] == ':') {
								skip_START = 0;
								i += 2;
							} else {
								return SLIB_PARSE_ERROR;
							}
						} else {
							if (skip_START >= 0) {
								return SLIB_PARSE_ERROR;
							}
							skip_START = k;
							i++;
						}
					} else {
						v[k] = (sl_uint16)s;
						k++;
						i++;
					}
				}
				if (k == 8) {
					if (skip_START >= 0) {
						return SLIB_PARSE_ERROR;
					} else {
						if (obj) {
							for (int q = 0; q < 8; q++) {
								obj->setElement(q, v[q]);
							}
						}
					}
				} else {
					if (skip_START < 0) {
						return SLIB_PARSE_ERROR;
					} else {
						if (obj) {
							int q;
							for (q = 0; q < skip_START; q++) {
								obj->setElement(q, v[q]);
							}
							int x = skip_START + 8 - k;
							for (; q < x; q++) {
								obj->setElement(q, 0);
							}
							for (; q < 8; q++) {
								obj->setElement(q, v[q - 8 + k]);
							}
						}
					}
				}
				return i;
			}
		}
	}

	SLIB_DEFINE_CLASS_PARSE_MEMBERS(IPv6Address, priv::ipv6address::Parse)

	IPv6Address& IPv6Address::operator=(sl_null_t) noexcept
	{
		setZero();
		return *this;
	}

	IPv6Address& IPv6Address::operator=(const StringParam& address) noexcept
	{
		if (!(parse(address))) {
			setZero();
		}
		return *this;
	}


	const IPAddress::_ipaddress IPAddress::_none = { IPAddressType::None, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };

	IPAddress::IPAddress(const IPv4Address& other) noexcept
	{
		setIPv4(other);
	}

	IPAddress::IPAddress(const IPv6Address& other) noexcept
	{
		setIPv6(other);
	}

	IPAddress::IPAddress(const StringParam& address) noexcept
	{
		if (!(parse(address))) {
			type = IPAddressType::None;
		}
	}

	sl_bool IPAddress::isIPv4() const noexcept
	{
		return type == IPAddressType::IPv4;
	}

	const IPv4Address& IPAddress::getIPv4() const noexcept
	{
		return *(reinterpret_cast<IPv4Address const*>(m));
	}

	void IPAddress::setIPv4(const IPv4Address& addr) noexcept
	{
		type = IPAddressType::IPv4;
		*(reinterpret_cast<IPv4Address*>(m)) = addr;
	}

	sl_bool IPAddress::isIPv6() const noexcept
	{
		return type == IPAddressType::IPv6;
	}

	const IPv6Address& IPAddress::getIPv6() const noexcept
	{
		return *(reinterpret_cast<IPv6Address const*>(m));
	}

	void IPAddress::setIPv6(const IPv6Address& addr) noexcept
	{
		type = IPAddressType::IPv6;
		*(reinterpret_cast<IPv6Address*>(m)) = addr;
	}

	sl_bool IPAddress::setHostName(const StringParam& hostName) noexcept
	{
		*this = Network::getIPAddressFromHostName(hostName);
		return isNotNone();
	}

	sl_compare_result IPAddress::compare(const IPAddress& other) const noexcept
	{
		if (type < other.type) {
			return -1;
		}
		if (type > other.type) {
			return 1;
		}
		switch (type) {
			case IPAddressType::None:
				return 0;
			case IPAddressType::IPv4:
				return (reinterpret_cast<IPv4Address const*>(m))->compare(*(reinterpret_cast<IPv4Address const*>(other.m)));
			case IPAddressType::IPv6:
				return (reinterpret_cast<IPv6Address const*>(m))->compare(*(reinterpret_cast<IPv6Address const*>(other.m)));
		}
		return 0;
	}

	sl_bool IPAddress::equals(const IPAddress& other) const noexcept
	{
		if (type != other.type) {
			return sl_false;
		}
		switch (type) {
			case IPAddressType::None:
				return sl_true;
			case IPAddressType::IPv4:
				return (reinterpret_cast<IPv4Address const*>(m))->equals(*(reinterpret_cast<IPv4Address const*>(other.m)));
			case IPAddressType::IPv6:
				return (reinterpret_cast<IPv6Address const*>(m))->equals(*(reinterpret_cast<IPv6Address const*>(other.m)));
		}
		return sl_false;
	}

	sl_size IPAddress::getHashCode() const noexcept
	{
		switch (type) {
			case IPAddressType::None:
				return 0;
			case IPAddressType::IPv4:
				return (reinterpret_cast<IPv4Address const*>(m))->getHashCode();
			case IPAddressType::IPv6:
				return (reinterpret_cast<IPv6Address const*>(m))->getHashCode();
		}
		return 0;
	}

	String IPAddress::toString() const noexcept
	{
		switch (type) {
			case IPAddressType::IPv4:
				return (*(reinterpret_cast<IPv4Address const*>(m))).toString();
			case IPAddressType::IPv6:
				return (*(reinterpret_cast<IPv6Address const*>(m))).toString();
			default:
				return sl_null;
		}
	}

	namespace priv
	{
		namespace ipaddress
		{
			template <class CT>
			SLIB_INLINE static sl_reg Parse(IPAddress* obj, const CT* sz, sl_size posStart, sl_size posEnd) noexcept
			{
				if (posStart >= posEnd) {
					return SLIB_PARSE_ERROR;
				}
				sl_reg index;
				IPv4Address a4;
				index = IPv4Address::parse(&a4, sz, posStart, posEnd);
				if (index != SLIB_PARSE_ERROR) {
					if (obj) {
						*obj = a4;
					}
					return index;
				}
				IPv6Address a6;
				index = IPv6Address::parse(&a6, sz, posStart, posEnd);
				if (index != SLIB_PARSE_ERROR) {
					if (obj) {
						*obj = a6;
					}
					return index;
				}
				return SLIB_PARSE_ERROR;
			}
		}
	}

	SLIB_DEFINE_CLASS_PARSE_MEMBERS(IPAddress, priv::ipaddress::Parse)

	IPAddress& IPAddress::operator=(const IPv4Address& other) noexcept
	{
		type = IPAddressType::IPv4;
		*(reinterpret_cast<IPv4Address*>(m)) = other;
		return *this;
	}

	IPAddress& IPAddress::operator=(const IPv6Address& other) noexcept
	{
		type = IPAddressType::IPv6;
		*(reinterpret_cast<IPv6Address*>(m)) = other;
		return *this;
	}

	IPAddress& IPAddress::operator=(const StringParam& address) noexcept
	{
		if (!(parse(address))) {
			setNone();
		}
		return *this;
	}

}
