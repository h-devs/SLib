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

#ifndef CHECKHEADER_SLIB_NETWORK_PACKET_ANALYZER
#define CHECKHEADER_SLIB_NETWORK_PACKET_ANALYZER

#include "ethernet.h"
#include "arp.h"
#include "tcpip.h"
#include "icmp.h"
#include "dns.h"

namespace slib
{

	class SLIB_EXPORT PacketAnalyzer
	{
	public:
		PacketAnalyzer();

		~PacketAnalyzer();

	public:
		void putEthernet(const void* packet, sl_size size, void* userData);

		void putIP(const void* packet, sl_size size, void* userData);


		void setLogging(sl_bool flag = sl_true);


		void setArpEnabled(sl_bool flag = sl_true);

		void setTcpEnabled(sl_bool flag = sl_true);

		void setUdpEnabled(sl_bool flag = sl_true);

		void setIcmpEnabled(sl_bool flag = sl_true);

		void setAnalyzingHttp(sl_bool flag = sl_true);

		void setAnalyzingHttps(sl_bool flag = sl_true);

		void setAnalyzingDns(sl_bool flag = sl_true);

		void setIgnoringLocalPackets(sl_bool flag = sl_true);

		void setIgnoringUnknownPorts(sl_bool flag = sl_true);

	protected:
		virtual void onARP_IPv4(ArpPacket* packet, sl_bool flagRequest, void* userData);
	
		virtual void onTCP_IPv4(IPv4Packet* packet, TcpSegment* tcp, sl_uint8* data, sl_uint32 sizeData, void* userData);

		virtual void onUDP_IPv4(IPv4Packet* packet, UdpDatagram* udp, sl_uint8* data, sl_uint32 sizeData, void* userData);

		virtual void onICMP_IPv4(IPv4Packet* packet, IcmpHeaderFormat* icmp, sl_uint8* data, sl_uint32 sizeData, void* userData);

		virtual void onDNS_IPv4(IPv4Packet* packet, UdpDatagram* udp, DnsPacket* dns, void* userData);

		virtual void onHTTP_IPv4(IPv4Packet* packet, TcpSegment* tcp, const StringView& host, const StringView& uri, void* userData);

		virtual void onHTTPS_IPv4(IPv4Packet* packet, TcpSegment* tcp, const StringView& host, void* userData);

	protected:
		void analyzeTcpContent(IPv4Packet* packet, TcpSegment* tcp, sl_uint8* data, sl_uint32 sizeData, void* userData);

	protected:
		sl_bool m_flagLogging;

		sl_bool m_flagAnalyzeArp;
		sl_bool m_flagAnalyzeTcp;
		sl_bool m_flagAnalyzeUdp;
		sl_bool m_flagAnalyzeIcmp;
		sl_bool m_flagAnalyzeHttp;
		sl_bool m_flagAnalyzeHttps;
		sl_bool m_flagAnalyzeDns;

		sl_bool m_flagIgnoreLocalPackets;
		sl_bool m_flagIgnoreUnknownPorts;

		Ref<Referable> m_contentAnalyzer;
		Mutex m_lockContentAnalyzer;

	};

}

#endif
