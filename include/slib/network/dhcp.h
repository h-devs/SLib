/*
 *   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_NETWORK_DHCP
#define CHECKHEADER_SLIB_NETWORK_DHCP

#include "constants.h"
#include "socket_address.h"
#include "async.h"

#include "../core/string.h"

/********************************************************************
	DHCP Specification from RFC 2131, 2132

	Format of DHCP message

	0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|     op (1)    |   htype (1)   |   hlen (1)    |   hops (1)    |
	+---------------+---------------+---------------+---------------+
	|                            xid (4)                            |
	+-------------------------------+-------------------------------+
	|           secs (2)            |           flags (2)           |
	+-------------------------------+-------------------------------+
	|                          ciaddr  (4)                          |
	+---------------------------------------------------------------+
	|                          yiaddr  (4)                          |
	+---------------------------------------------------------------+
	|                          siaddr  (4)                          |
	+---------------------------------------------------------------+
	|                          giaddr  (4)                          |
	+---------------------------------------------------------------+
	|                                                               |
	|                          chaddr  (16)                         |
	|                                                               |
	|                                                               |
	+---------------------------------------------------------------+
	|                                                               |
	|                          sname   (64)                         |
	+---------------------------------------------------------------+
	|                                                               |
	|                          file    (128)                        |
	+---------------------------------------------------------------+
	|                                                               |
	|                          options (variable)                   |
	+---------------------------------------------------------------+

********************************************************************/

#define SLIB_NETWORK_DHCP_SERVER_PORT 67
#define SLIB_NETWORK_DHCP_CLIENT_PORT 68
#define SLIB_NETWORK_DHCP_MAGIC_COOKIE 0x63825363

namespace slib
{

	enum class DhcpOpcode
	{
		Request = 1,
		Reply = 2
	};

	enum class DhcpOptionCode
	{
		Pad = 0,
		SubnetMask = 1, // Must be sent after the Router(3) option if both are included
		TimeOffset = 2,
		Router = 3,
		TimeServer = 4,
		NameServer = 5,
		DomainNameServer = 6,
		LogServer = 7,
		CookieServer = 8,
		LPRServer = 9,
		ImpressServer = 10,
		ResourceLocationServer = 11,
		HostName = 12,
		BootFileSize = 13,
		MeritDumpFile = 14,
		DomainName = 15,
		SwapServer = 16,
		RootPath = 17,
		ExtensionPath = 18,
		BroadcastAddress = 28,
		TcpDefaultTTL = 37,
		RequestedIpAddress = 50,
		IpAddressLeaseTime = 51,
		DhcpMessageType = 53,
		ServerIdentifier = 54,
		ParameterRequestList = 55,
		Message = 56,
		MaximumDhcpMessageSize = 57,
		RenewalTimeValue = 58,
		RebindingTimeValue = 59,
		VendorClassIdentifier = 60,
		ClientIdentifier = 61,
		TFTPServerName = 66,
		BootFileName = 67,
		DomainSearch = 119,
		End = 255
	};

	enum class DhcpMessageType
	{
		None = 0,
		Discover = 1,
		Offer = 2,
		Request = 3,
		Decline = 4,
		Ack = 5,
		Nak = 6,
		Release = 7,
		Inform = 8
	};

	class SLIB_EXPORT DhcpHeader
	{
	public:
		DhcpOpcode getOpcode() const;

		void setOpcode(DhcpOpcode op);

		NetworkHardwareType getHardwareType() const;

		void setHardwareType(NetworkHardwareType type);

		sl_uint8 getHardwareAddressLength() const;

		void setHardwareAddressLength(sl_uint8 len);

		sl_uint8 getHops() const;

		void setHops(sl_uint8 hops);

		sl_uint32 getXid() const;

		void setXid(sl_uint32 xid);

		sl_uint16 getElapsedSeconds() const;

		void setElapsedSeconds(sl_uint32 secs);

		sl_uint16 getFlags() const;

		void setFlags(sl_uint16 flags);

		IPv4Address getClientIP();

		void setClientIP(const IPv4Address& ip);

		IPv4Address getYourIP();

		void setYourIP(const IPv4Address& ip);

		IPv4Address getServerIP();

		void setServerIP(const IPv4Address& ip);

		IPv4Address getRelayAgent();

		void setRelayAgent(const IPv4Address& ip);

		MacAddress getClientMacAddress();

		void setClientMacAddress(const MacAddress& address);

		sl_bool isValidMagicCookie();

		sl_uint32 getMagicCookie();

		void setMagicCookie(sl_uint32 v = SLIB_NETWORK_DHCP_MAGIC_COOKIE);

	private:
		sl_uint8 _op;
		sl_uint8 _htype; // Hardware address type. 1 for Ethernet
		sl_uint8 _hlen;
		sl_uint8 _hops; // Client sets to zero, optionally used by relay agents when booting via a relay agent
		sl_uint8 _xid[4]; // Transaction ID, a random number chosen by the client, used by the client and server to associate messages and responses between a client and a server
		sl_uint8 _secs[2]; // Filled in by client, seconds elapsed since client began address acquisition or renewal process
		sl_uint8 _flags[2];
		sl_uint8 _ciaddr[4]; // Client IP address; only filled in if client is in BOUND, RENEW or REBINDING state and can respond to ARP requests
		sl_uint8 _yiaddr[4]; // 'your' (client) IP address
		sl_uint8 _siaddr[4]; // IP address of next server to use in bootstrap; returned in DHCPOFFER, DHCPACK by server
		sl_uint8 _giaddr[4]; // Relay agent IP address, used in booting via a relay agent
		sl_uint8 _chaddr[16]; // Client hardware address
		sl_uint8 _sname[64]; // Optional server host name, null terminated string
		sl_uint8 _file[128]; // Boot file name, null terminated string; "generic" name or null in DHCPDISCOVER, fully qualified directory - path name in DHCPOFFER
		sl_uint8 _magicCookie[4];

	};

	class DhcpServer;

	class SLIB_EXPORT DhcpBindParam
	{
	public:
		// Input
		DhcpMessageType type;
		MacAddress mac;

		// Output
		IPv4Address ip; // Required
		IPv4Address subnetMask; // Required
		IPv4Address server;
		IPv4Address broadcastAddress;
		IPv4Address router; // Ignored when `routers` is used
		List<IPv4Address> routers;
		String domainName;
		String searchDomain;
		List<IPv4Address> domainServers;
		sl_uint32 leaseTime; // Seconds

	public:
		DhcpBindParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DhcpBindParam)

	};

	class SLIB_EXPORT DhcpServerParam
	{
	public:
		String bindDevice;
		sl_uint16 port;

		Ref<AsyncIoLoop> ioLoop;
		sl_bool flagAutoStart;

		Function<void(DhcpServer*, DhcpBindParam&)> onBind;

	public:
		DhcpServerParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DhcpServerParam)

	};

	class SLIB_EXPORT DhcpServer : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		DhcpServer();

		~DhcpServer();

	public:
		static Ref<DhcpServer> create(const DhcpServerParam& param);

	public:
		void release();

		void start();

		sl_bool isRunning();

	protected:
		void _onReceiveFrom(AsyncUdpSocket* socket, const SocketAddress& address, void* data, sl_uint32 sizeReceive);

		void _onBind(DhcpBindParam& param);

		void _processRequest(const SocketAddress& addressFrom, DhcpHeader& header, sl_uint8* options, sl_size sizeOptions);

	private:
		sl_bool m_flagInit;
		sl_bool m_flagRunning;

		Ref<AsyncUdpSocket> m_socket;

		Function<void(DhcpServer*, DhcpBindParam&)> m_onBind;

	};

}

#endif
