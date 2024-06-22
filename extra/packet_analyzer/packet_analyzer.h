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

#ifndef CHECKHEADER_SLIB_EXTRA_PACKET_ANALYZER
#define CHECKHEADER_SLIB_EXTRA_PACKET_ANALYZER

#include <slib/network/ethernet.h>
#include <slib/network/arp.h>
#include <slib/network/tcpip.h>
#include <slib/network/icmp.h>
#include <slib/network/dns.h>
#include <slib/network/capture.h>

#include <slib/core/rw_lock.h>

namespace slib
{

	class NetCapture;

	enum class TcpConnectionType
	{
		None = 0,
		HTTP = 1,
		HTTPS = 2
	};

	struct SLIB_EXPORT TcpConnectionInfo
	{
		TcpConnectionType type;
		String host;
	};

	class SLIB_EXPORT PacketAnalyzer
	{
	public:
		PacketAnalyzer();

		~PacketAnalyzer();

	public:
		void putCapturedPacket(NetCapture* capture, NetworkCaptureType type, const void* frame, sl_size size, void* userData);

		void putCapturedPacket(NetCapture* capture, const void* frame, sl_size size, void* userData);

		void putEthernet(NetCapture* capture, const void* frame, sl_size size, void* userData);

		void putEthernet(const void* frame, sl_size size, void* userData);

		void putIP(NetCapture* capture, const void* packet, sl_size size, void* userData);

		void putIP(const void* packet, sl_size size, void* userData);


		sl_bool getTcpConnectionInfo(const IPv4Address& sourceIp, sl_uint16 sourcePort, TcpConnectionInfo& info);

		String getDnsHost(const IPv4Address& ip);


		void setLogging(sl_bool flag = sl_true);


		void setIPv4Enabled(sl_bool flag = sl_true);

		void setArpEnabled(sl_bool flag = sl_true);

		void setTcpEnabled(sl_bool flag = sl_true);

		void setUdpEnabled(sl_bool flag = sl_true);

		void setIcmpEnabled(sl_bool flag = sl_true);

		void setCapturingUnknownFrames(sl_bool flag = sl_true);

		void setAnalyzingHttp(sl_bool flag = sl_true);

		void setAnalyzingHttps(sl_bool flag = sl_true);

		void setAnalyzingDns(sl_bool flag = sl_true);

		void setGatheringHostInfo(sl_bool flag = sl_true);

		void setIgnoringLocalPackets(sl_bool flag = sl_true);

		void setIgnoringUnknownPorts(sl_bool flag = sl_true);

		void setBlockingTcpConnections(sl_bool flag = sl_true);

	protected:
		virtual void onIPv4(IPv4Packet* packet, void* userData);

		virtual void onARP_IPv4(EthernetFrame* frame, ArpPacket* packet, sl_bool flagRequest, void* userData);

		virtual void onTCP_IPv4(IPv4Packet* packet, TcpSegment* tcp, sl_uint8* data, sl_uint32 sizeData, void* userData);

		virtual void onUDP_IPv4(IPv4Packet* packet, UdpDatagram* udp, sl_uint8* data, sl_uint32 sizeData, void* userData);

		virtual void onICMP_IPv4(IPv4Packet* packet, IcmpHeaderFormat* icmp, sl_uint8* data, sl_uint32 sizeData, void* userData);

		virtual void onDNS_IPv4(IPv4Packet* packet, UdpDatagram* udp, DnsPacket* dns, void* userData);

		virtual void onHTTP_IPv4(IPv4Packet* packet, TcpSegment* tcp, const StringView& host, const StringView& uri, void* userData);

		virtual void onHTTPS_IPv4(IPv4Packet* packet, TcpSegment* tcp, const StringView& host, void* userData);

		virtual void onUnknownFrame(EthernetFrame* frame, sl_uint8* data, sl_uint32 sizeData, void* userData);

		virtual sl_bool shouldBlockTcpConnection(IPv4Packet* packet, TcpSegment* tcp, TcpConnectionType type, const String& host, void* userData);

	protected:
		struct PacketParam
		{
			NetCapture* capture;
			NetworkCaptureType type;
			sl_uint8* frame;
			sl_uint32 sizeFrame;
			sl_uint8* packet;
			sl_uint32 sizePacket;
			void* userData;
		};

		void analyzeIP(const PacketParam& param);

		void analyzeTcpContent(IPv4Packet* packet, TcpSegment* tcp, sl_uint8* data, sl_uint32 sizeData, void* userData);

		void registerHostInfo(IPv4Packet* packet, TcpSegment* tcp, TcpConnectionType type, const String& host);

		void resetHostInfo(IPv4Packet* packet, TcpSegment* tcp);

		void sendBlockingIPv4TcpPacket(const PacketParam& param, TcpSegment* tcp);

	protected:
		sl_bool m_flagLogging;

		sl_bool m_flagAnalyzeIPv4;
		sl_bool m_flagAnalyzeArp;
		sl_bool m_flagAnalyzeTcp;
		sl_bool m_flagAnalyzeUdp;
		sl_bool m_flagAnalyzeIcmp;
		sl_bool m_flagAnalyzeHttp;
		sl_bool m_flagAnalyzeHttps;
		sl_bool m_flagAnalyzeDns;

		sl_bool m_flagGatheringHostInfo;
		sl_bool m_flagIgnoreLocalPackets;
		sl_bool m_flagIgnoreUnknownPorts;
		sl_bool m_flagCaptureUnknownFrames;
		sl_bool m_flagBlockingTcpConnections;

		Ref<CRef> m_contentAnalyzer;
		Mutex m_lockContentAnalyzer;

		HashTable<sl_uint64, TcpConnectionInfo> m_tableTcpConnectionInfo;
		ReadWriteLock m_lockTcpConnectionInfo;

		HashTable<IPv4Address, String> m_tableDnsInfo;
		ReadWriteLock m_lockDnsInfo;

	};

}

#endif
