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

#ifndef CHECKHEADER_SLIB_NETWORK_SOCKET_ADDRESS
#define CHECKHEADER_SLIB_NETWORK_SOCKET_ADDRESS

#include "ip_address.h"

#include "../core/parse.h"

namespace slib
{

	class SLIB_EXPORT SocketAddress
	{
	public:
		IPAddress ip;
		sl_uint16 port;
		
	public:
		SocketAddress() noexcept: port(0) {}
		
		SocketAddress(const SocketAddress& other) = default;
		
		SocketAddress(sl_uint16 _port) noexcept: port(_port) {}
		
		SocketAddress(const IPAddress& _ip, sl_uint16 _port) noexcept: ip(_ip), port(_port) {}
		
		SocketAddress(const StringParam& str) noexcept;
		
	public:
		static const SocketAddress& none() noexcept
		{
			return *(reinterpret_cast<SocketAddress const*>(&_none));
		}
		
		void setNone() noexcept;
		
		sl_bool isValid() const noexcept;
		
		sl_bool isInvalid() const noexcept;
		
		sl_uint32 getSystemSocketAddress(void* addr) noexcept;
		
		sl_bool setSystemSocketAddress(const void* addr, sl_uint32 size = 0) noexcept;
		
		// HostName:port
		sl_bool setHostAddress(const StringParam& address) noexcept;		

	public:
		/*
		 Address Format
			IPv4 - a.b.c.d:port
			Ipv6 - [s0:s1:s2:s3:s4:s5:s6:s7]:port
		 */
		SLIB_DECLARE_CLASS_COMMON_MEMBERS(SocketAddress)

		static sl_bool parseIPv4Range(const String& str, IPv4Address* from = sl_null, IPv4Address* to = sl_null) noexcept;
		
		static sl_bool parsePortRange(const String& str, sl_uint16* from = sl_null, sl_uint16* to = sl_null) noexcept;
		
	public:
		SocketAddress& operator=(const SocketAddress& other) = default;
		
		SocketAddress& operator=(const String& str) noexcept;
		
	private:
		struct _socket_address
		{
			struct {
				IPAddressType type;
				sl_uint8 data[PRIV_SLIB_NET_IPADDRESS_SIZE];
			} ip;
			sl_uint16 port;
		};
		
		static const _socket_address _none;
		
	};

}

#endif
