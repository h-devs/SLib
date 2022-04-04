/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_NETWORK_FIREWALL
#define CHECKHEADER_SLIB_NETWORK_FIREWALL

#include "constants.h"
#include "ip_address.h"
#include "mac_address.h"

#include "../core/hash_set.h"
#include "../core/string.h"
#include "../core/nullable.h"

namespace slib
{

	class Json;

	class SLIB_EXPORT Firewall
	{
	public:
		// Works only on Win32
		static void allowApplication(const StringParam& path);

		// Works only on Win32
		static void disallowApplication(const StringParam& path);

	};


	enum class FirewallAction
	{
		Unknown = 0,
		Accept = 1,
		Drop = 2
	};

	/*
		Json Style

			mac: "[!]mac" or "[!]mac1,mac2,..."
			ip: "[!]ip" or "[!]ip1[-ip2],ip3[-ip4],..."
			port: "[!]port" or "[!]port1[-port2],port3[-port4],..."
	*/
	class SLIB_EXPORT FirewallAddressRule
	{
	public:
		HashSet<MacAddress> mac;
		sl_bool flagNotMac;
		List< Pair<IPv4Address, IPv4Address> > ip;
		sl_bool flagNotIp;
		List< Pair<sl_uint16, sl_uint16> > port;
		sl_bool flagNotPort;

	public:
		FirewallAddressRule();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FirewallAddressRule)

	public:
		Json toJson() const;

		void setJson(const Json& json);

		sl_bool matchMac(const MacAddress& mac) const;

		sl_bool matchIP(const IPv4Address& ip) const;

		sl_bool matchPort(sl_uint16 port) const;

	};

	class SLIB_EXPORT FirewallRule
	{
	public:
		FirewallAction action;
		NetworkInternetProtocol protocol;
		FirewallAddressRule source;
		FirewallAddressRule target;

	public:
		FirewallRule();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FirewallRule)

	public:
		Json toJson() const;

		void setJson(const Json& json);

	public:
		sl_bool matchIPv4Packet(const MacAddress& macSource, const MacAddress& macTarget, const void* packet, sl_size sizePacket);

	};

}

#endif
