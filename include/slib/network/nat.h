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

#ifndef CHECKHEADER_SLIB_NETWORK_NAT
#define CHECKHEADER_SLIB_NETWORK_NAT

#include "tcpip.h"
#include "icmp.h"

#include "../core/hash_map.h"

/*
	If you are using kernel-mode NAT on linux (for example on port range 40000~60000), following configuration will avoid to conflict with kernel-networking.

		iptables -A INPUT -p tcp --dport 40000:60000 -j DROP
		sysctl -w net.ipv4.ip_local_port_range="30000 39000"
*/ 

namespace slib
{

	struct NatTablePort
	{
		sl_uint32 sourceIP;
		sl_uint16 sourcePort;
		sl_uint8 flagActive;
		sl_uint64 lastAccessTick;
	};

	class SLIB_EXPORT NatTableMapping
	{
	public:
		NatTableMapping();

		~NatTableMapping();

	public:
		sl_bool initialize(sl_uint16 portBegin, sl_uint16 portEnd);

		sl_bool mapToExternal(const IPv4Address& internalIP, sl_uint16 internalPort, sl_uint16& externalPort, sl_uint64 currentTick);

		sl_bool mapToInternal(sl_uint16 externalPort, IPv4Address& internalIP, sl_uint16& internalPort, sl_uint64 currentTick);

	protected:
		CHashMap<sl_uint64, sl_uint16> m_mapTranslation;

		NatTablePort* m_ports;
		sl_uint16 m_nPorts;
		sl_uint16 m_pos;

		sl_uint16 m_portBegin;
		sl_uint16 m_portEnd;

	};

	class SLIB_EXPORT NatTableParam
	{
	public:
		IPv4Address targetAddress;

		sl_uint16 tcpPortBegin;
		sl_uint16 tcpPortEnd;

		sl_uint16 udpPortBegin;
		sl_uint16 udpPortEnd;

		sl_uint16 icmpEchoIdentifier;

	public:
		NatTableParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(NatTableParam)

	};

	class SLIB_EXPORT NatTable
	{
	public:
		NatTable();

		~NatTable();

	public:
		sl_bool initialize(const NatTableParam& param);

	public:
		sl_bool translateOutgoingPacket(IPv4Packet* header, void* content, sl_uint32 sizeContent, sl_uint64 currentTick);

		sl_bool translateIncomingPacket(IPv4Packet* header, void* content, sl_uint32 sizeContent, sl_uint64 currentTick);

		sl_uint16 getMappedIcmpEchoSequenceNumber(const IcmpEchoAddress& address);

	protected:
		NatTableMapping m_mappingTcp;
		NatTableMapping m_mappingUdp;

		IPv4Address m_targetAddress;
		sl_uint32 m_icmpEchoIdentifier;
		sl_uint16 m_icmpEchoSequenceCurrent;

		IPv4Address m_tcpFragmentTable[0x10000];
		IPv4Address m_udpFragmentTable[0x10000];

		struct IcmpEchoElement
		{
			IcmpEchoAddress addressSource;
			sl_uint16 sequenceNumberTarget;
		};
		CHashMap<IcmpEchoAddress, IcmpEchoElement> m_mapIcmpEchoOutgoing;
		CHashMap<sl_uint32, IcmpEchoElement> m_mapIcmpEchoIncoming;

	};

}

#endif
