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

#ifndef CHECKHEADER_SLIB_NETWORK_IP_ADDRESS
#define CHECKHEADER_SLIB_NETWORK_IP_ADDRESS

#include "definition.h"

#include "../core/common_members.h"
#include "../core/hash.h"

namespace slib
{

	class SLIB_EXPORT IPv4Address
	{
	public:
		sl_uint8 a;
		sl_uint8 b;
		sl_uint8 c;
		sl_uint8 d;
		
	public:
		enum
		{
			Any = 0, // 0.0.0.0
			Maximum = 0xFFFFFFFF, // 255.255.255.255
			Broadcast = 0xFFFFFFFF, // 255.255.255.255
			Loopback = 0x7F000001, // 127.0.0.1
			MulticastBegin = 0xE0000000, // 224.0.0.0
			MulticastEnd = 0xEFFFFFFF, // 239.255.255.255
		};
		
	public:
		SLIB_CONSTEXPR IPv4Address(): a(0), b(0), c(0), d(0) {}
		
		SLIB_CONSTEXPR IPv4Address(const IPv4Address& other): a(other.a), b(other.b), c(other.c), d(other.d) {}
		
		SLIB_CONSTEXPR IPv4Address(sl_uint8 const addr[4]): a(addr[0]), b(addr[1]), c(addr[2]), d(addr[3]) {}
		
		SLIB_CONSTEXPR IPv4Address(sl_uint8 _a, sl_uint8 _b, sl_uint8 _c, sl_uint8 _d): a(_a), b(_b), c(_c), d(_d) {}
		
		SLIB_CONSTEXPR IPv4Address(sl_uint32 addr): a((sl_uint8)(addr >> 24)), b((sl_uint8)(addr >> 16)), c((sl_uint8)(addr >> 8)), d((sl_uint8)(addr)) {}
		
		IPv4Address(const StringParam& address) noexcept;
		
	public:
		void setElements(sl_uint8 a, sl_uint8 b, sl_uint8 c, sl_uint8 d) noexcept;
		
		SLIB_CONSTEXPR sl_uint32 getInt() const
		{
			return ((sl_uint32)(a) << 24) | ((sl_uint32)(b) << 16) | ((sl_uint32)(c) << 8) | ((sl_uint32)(d));
		}
		
		void setInt(sl_uint32 addr) noexcept
		{
			a = (sl_uint8)(addr >> 24);
			b = (sl_uint8)(addr >> 16);
			c = (sl_uint8)(addr >> 8);
			d = (sl_uint8)(addr);
		}
		
		void getBytes(void* bytes) const noexcept;
		
		void setBytes(const void* bytes) noexcept;
		
		static const IPv4Address& zero() noexcept
		{
			return *(reinterpret_cast<IPv4Address const*>(&_zero));
		}
		
		SLIB_CONSTEXPR sl_bool isZero() const
		{
			return !(getInt());
		}
		
		SLIB_CONSTEXPR sl_bool isNotZero() const
		{
			return getInt() != 0;
		}
		
		void setZero() noexcept;
		
		sl_bool isLoopback() const noexcept;
		
		sl_bool isLinkLocal() const noexcept;
		
		sl_bool isMulticast() const noexcept;
		
		sl_bool isBroadcast() const noexcept;
		
		sl_bool isHost() const noexcept;
		
		sl_bool isPrivate() const noexcept;

		sl_bool isSpecial() const noexcept;
		
		void makeNetworkMask(sl_uint32 networkPrefixLength) noexcept;
		
		sl_uint32 getNetworkPrefixLengthFromMask() const noexcept;
		
		sl_bool setHostName(const StringParam& hostName) noexcept;

	public:
		SLIB_CONSTEXPR sl_compare_result compare(const IPv4Address& other) const
		{
			return ComparePrimitiveValues(getInt(), other.getInt());
		}

		SLIB_CONSTEXPR sl_compare_result compare(sl_uint32 addr) const
		{
			return ComparePrimitiveValues(getInt(), addr);
		}

		SLIB_CONSTEXPR sl_bool equals(const IPv4Address& other) const
		{
			return getInt() == other.getInt();
		}
		
		SLIB_CONSTEXPR sl_bool equals(sl_uint32 addr) const
		{
			return getInt() == addr;
		}
		
		SLIB_CONSTEXPR sl_size getHashCode() const
		{
			return HashPrimitiveValue(getInt());
		}

		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS
		SLIB_DECLARE_CLASS_JSON_SERIALIZE_MEMBERS
		SLIB_DECLARE_CLASS_STRING_MEMBERS(IPv4Address)

	public:
		IPv4Address& operator=(const IPv4Address& other) = default;
		
		IPv4Address& operator=(sl_uint32 addr) noexcept
		{
			a = (sl_uint8)(addr >> 24);
			b = (sl_uint8)(addr >> 16);
			c = (sl_uint8)(addr >> 8);
			d = (sl_uint8)(addr);
			return *this;
		}
		
		IPv4Address& operator=(const StringParam& address) noexcept;
		
	private:
		static const sl_uint8 _zero[4];
		
	};
	
	class SLIB_EXPORT IPv4AddressInfo
	{
	public:
		IPv4Address address;
		sl_uint32 networkPrefixLength;
		
	public:
		IPv4Address getNetworkMask() const noexcept;
		
		void setNetworkMask(const IPv4Address& mask) noexcept;
		
	public:
		sl_compare_result compare(const IPv4AddressInfo& other) const noexcept;

		sl_bool equals(const IPv4AddressInfo& other) const noexcept;

	};
	
	class SLIB_EXPORT IPv6Address
	{
	public:
		sl_uint8 m[16];
		
	public:
		IPv6Address() = default;
		
		IPv6Address(const IPv6Address& other) = default;
		
		// 8 elements
		IPv6Address(const sl_uint16* s) noexcept;
		
		IPv6Address(sl_uint16 s0, sl_uint16 s1, sl_uint16 s2, sl_uint16 s3, sl_uint16 s4, sl_uint16 s5, sl_uint16 s6, sl_uint16 s7) noexcept;
		
		// 16 elements
		IPv6Address(const sl_uint8* b) noexcept;
		
		IPv6Address(const IPv4Address& ipv4) noexcept;
		
		IPv6Address(const StringParam& address) noexcept;
		
	public:
		// not checking index bound (0~7)
		sl_uint16 getElement(int index) const noexcept;
		
		// not checking index bound (0~7)
		void setElement(int index, sl_uint16 s) noexcept;
		
		// 8 elements
		void getElements(sl_uint16* s) const noexcept;
		
		// 8 elements
		void setElements(const sl_uint16* s) noexcept;
		
		void setElements(sl_uint16 s0, sl_uint16 s1, sl_uint16 s2, sl_uint16 s3, sl_uint16 s4, sl_uint16 s5, sl_uint16 s6, sl_uint16 s7) noexcept;
		
		// 16 elements
		void getBytes(void* bytes) const noexcept;
		
		// 16 elements
		void setBytes(const void* bytes) noexcept;
		
		static const IPv6Address& zero() noexcept
		{
			return *(reinterpret_cast<IPv6Address const*>(&_zero));
		}
		
		void setZero() noexcept;
		
		sl_bool isZero() const noexcept;
		
		sl_bool isNotZero() const noexcept;
		
		static const IPv6Address& getLoopback() noexcept
		{
			return *(reinterpret_cast<IPv6Address const*>(&_loopback));
		}
		
		sl_bool isLoopback() const noexcept;
		
		sl_bool isLinkLocal() const noexcept;
		
		IPv4Address getIPv4Transition() const noexcept;
		
		void setIPv4Transition(const IPv4Address& ipv4) noexcept;
		
		sl_bool isIPv4Transition() const noexcept;
		
		sl_bool setHostName(const StringParam& hostName) noexcept;
		
	public:
		SLIB_DECLARE_CLASS_COMMON_MEMBERS(IPv6Address)

	public:
		IPv6Address& operator=(const IPv6Address& other) = default;
		
		IPv6Address& operator=(const StringParam& address) noexcept;
		
	private:
		static const sl_uint8 _zero[16];
		static const sl_uint8 _loopback[16];
		static const sl_uint8 _loopback_linkLocal[16];

	};
	
#define PRIV_SLIB_NET_IPADDRESS_SIZE 16
	
	enum class IPAddressType
	{
		None = 0,
		IPv4 = 1,
		IPv6 = 2
	};
	
	class SLIB_EXPORT IPAddress
	{
	public:
		IPAddressType type;
		sl_uint8 m[PRIV_SLIB_NET_IPADDRESS_SIZE];
		
	public:
		IPAddress() noexcept: type(IPAddressType::None) {}
		
		IPAddress(const IPAddress& other) = default;
		
		IPAddress(const IPv4Address& other) noexcept;
		
		IPAddress(const IPv6Address& other) noexcept;
		
		IPAddress(const StringParam& address) noexcept;
		
	public:
		static const IPAddress& none() noexcept
		{
			return *(reinterpret_cast<IPAddress const*>(&_none));
		}
		
		void setNone() noexcept
		{
			type = IPAddressType::None;
		}
		
		SLIB_CONSTEXPR sl_bool isNone() const
		{
			return type == IPAddressType::None;
		}
		
		SLIB_CONSTEXPR sl_bool isNotNone() const
		{
			return type != IPAddressType::None;
		}
		
		sl_bool isIPv4() const noexcept;
		
		const IPv4Address& getIPv4() const noexcept;
		
		void setIPv4(const IPv4Address& addr) noexcept;
		
		sl_bool isIPv6() const noexcept;
		
		const IPv6Address& getIPv6() const noexcept;
		
		void setIPv6(const IPv6Address& addr) noexcept;
		
		sl_bool setHostName(const StringParam& hostName) noexcept;

	public:
		SLIB_DECLARE_CLASS_COMMON_MEMBERS(IPAddress)

	public:
		IPAddress& operator=(const IPAddress& other) = default;
		
		IPAddress& operator=(const IPv4Address& other) noexcept;
		
		IPAddress& operator=(const IPv6Address& other) noexcept;
		
		IPAddress& operator=(const StringParam& address) noexcept;
		
	private:
		struct _ipaddress
		{
			IPAddressType type;
			sl_uint8 m[PRIV_SLIB_NET_IPADDRESS_SIZE];
		};
		static const _ipaddress _none;
		
	};
	
}

#endif
