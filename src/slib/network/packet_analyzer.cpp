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

#include "slib/network/packet_analyzer.h"

#include "slib/core/log.h"

#define CONTENT_LOOKUP_SIZE 1024
#define CONNECTION_COUNT 512

namespace slib
{

	namespace priv
	{
		namespace packet_analyzer
		{

			class PacketAnalyzerHelper : public PacketAnalyzer
			{
				friend class ContentAnalyzer;
			};

			struct Connection
			{
				sl_uint32 address;
				sl_uint16 port;
				sl_bool flagChecked;
				sl_uint32 startSequenceNumber;
				sl_uint32 sizeContent;
				sl_uint8 content[CONTENT_LOOKUP_SIZE];
			};

			class ContentAnalyzer : public Referable
			{
			public:
				Connection m_connections[CONNECTION_COUNT];

			public:
				void analyze(PacketAnalyzerHelper* parent, IPv4Packet* packet, TcpSegment* tcp, sl_uint8* data, sl_uint32 sizeData, void* userData)
				{

				}

			};

		}
	}

	using namespace priv::packet_analyzer;

	PacketAnalyzer::PacketAnalyzer()
	{
		m_flagLogging = sl_false;
		
		m_flagAnalyzeArp = sl_false;
		m_flagAnalyzeTcp = sl_false;
		m_flagAnalyzeUdp = sl_false;
		m_flagAnalyzeIcmp = sl_false;
		m_flagAnalyzeHttp = sl_false;
		m_flagAnalyzeHttps = sl_false;
		m_flagAnalyzeDns = sl_false;
	}

	PacketAnalyzer::~PacketAnalyzer()
	{
	}

	void PacketAnalyzer::putEthernet(const void* packet, sl_size size, void* userData)
	{
		if (size <= EthernetFrame::HeaderSize) {
			return;
		}
		EthernetFrame* frame = (EthernetFrame*)packet;
		NetworkLinkProtocol protocol = frame->getProtocol();
		if (protocol == NetworkLinkProtocol::IPv4) {
			putIP((sl_uint8*)packet + EthernetFrame::HeaderSize, size - EthernetFrame::HeaderSize, userData);
		} else if (protocol == NetworkLinkProtocol::ARP) {
			if (m_flagAnalyzeArp) {
				if (size >= EthernetFrame::HeaderSize + ArpPacket::SizeForIPv4) {
					ArpPacket* arp = (ArpPacket*)((sl_uint8*)packet + EthernetFrame::HeaderSize);
					if (arp->isValidEthernetIPv4()) {
						ArpOperation op = arp->getOperation();
						if (op == ArpOperation::Request) {
							onARP_IPv4(arp, sl_true, userData);
						} else if (op == ArpOperation::Reply) {
							onARP_IPv4(arp, sl_false, userData);
						}
					}
				}
			}
		}
	}

	void PacketAnalyzer::putIP(const void* packet, sl_size size, void* userData)
	{
		if (size <= IPv4Packet::HeaderSizeBeforeOptions) {
			return;
		}
		sl_bool flagAnalyzeTcp = m_flagAnalyzeTcp || m_flagAnalyzeHttp || m_flagAnalyzeHttps;
		sl_bool flagAnalyzeUdp = m_flagAnalyzeUdp || m_flagAnalyzeDns;
		if (flagAnalyzeTcp || flagAnalyzeUdp || m_flagAnalyzeIcmp) {
			IPv4Packet* ip = (IPv4Packet*)packet;
			sl_uint8 sizeHeader = ip->getHeaderSize();
			sl_uint16 sizeTotal = ip->getTotalSize();
			if (sizeTotal < size && sizeHeader < sizeTotal) {
				return;
			}
			sl_uint8* content = ip->getContent();
			sl_uint16 sizeContent = sizeTotal - sizeHeader;
			NetworkInternetProtocol protocol = ip->getProtocol();
			if (protocol == NetworkInternetProtocol::TCP) {
				if (flagAnalyzeTcp) {
					if (sizeContent < TcpSegment::HeaderSizeBeforeOptions) {
						return;
					}
					TcpSegment* tcp = (TcpSegment*)content;
					sizeHeader = tcp->getHeaderSize();
					if (sizeContent < sizeHeader) {
						return;
					}
					if (m_flagAnalyzeTcp) {
						onTCP_IPv4(ip, tcp, tcp->getContent(), sizeContent - sizeHeader, userData);
					}
					if (m_flagAnalyzeHttp || m_flagAnalyzeHttps) {
						analyzeTcpContent(ip, tcp, tcp->getContent(), sizeContent - sizeHeader, userData);
					}
				}
			} else if (protocol == NetworkInternetProtocol::UDP) {
				if (flagAnalyzeUdp) {
					if (sizeContent < UdpDatagram::HeaderSize) {
						return;
					}
					UdpDatagram* udp = (UdpDatagram*)content;
					if (m_flagAnalyzeUdp) {
						onUDP_IPv4(ip, udp, udp->getContent(), sizeContent - UdpDatagram::HeaderSize, userData);
					}
					if (m_flagAnalyzeDns) {
						if (udp->getSourcePort() == 53 || udp->getDestinationPort() == 53) {
							DnsPacket dns;
							if (dns.parsePacket(udp->getContent(), sizeContent - UdpDatagram::HeaderSize)) {
								onDNS_IPv4(ip, udp, &dns, userData);
							}
						}
					}
				}
			} else if (protocol == NetworkInternetProtocol::ICMP) {
				if (m_flagAnalyzeIcmp) {
					if (sizeContent < sizeof(IcmpHeaderFormat)) {
						return;
					}
					IcmpHeaderFormat* icmp = (IcmpHeaderFormat*)content;
					onICMP_IPv4(ip, icmp, icmp->getContent(), sizeContent - sizeof(IcmpHeaderFormat), userData);
				}
			}
		}
	}

	void PacketAnalyzer::setLogging(sl_bool flag)
	{
		m_flagLogging = flag;
	}

	void PacketAnalyzer::setArpEnabled(sl_bool flag)
	{
		m_flagAnalyzeArp = flag;
	}

	void PacketAnalyzer::setTcpEnabled(sl_bool flag)
	{
		m_flagAnalyzeTcp = flag;
	}

	void PacketAnalyzer::setUdpEnabled(sl_bool flag)
	{
		m_flagAnalyzeUdp = flag;
	}

	void PacketAnalyzer::setIcmpEnabled(sl_bool flag)
	{
		m_flagAnalyzeIcmp = flag;
	}

	void PacketAnalyzer::setAnalyzingHttp(sl_bool flag)
	{
		m_flagAnalyzeHttp = flag;
	}

	void PacketAnalyzer::setAnalyzingHttps(sl_bool flag)
	{
		m_flagAnalyzeHttps = flag;
	}

	void PacketAnalyzer::setAnalyzingDns(sl_bool flag)
	{
		m_flagAnalyzeDns = flag;
	}

	void PacketAnalyzer::onARP_IPv4(ArpPacket* packet, sl_bool flagRequest, void* userData)
	{
		if (m_flagLogging) {
			const char* op = flagRequest ? "Request" : "Reply";
			Log("ARP", "%s: %s(%s)->%s(%s)", op, packet->getSenderIPv4Address().toString(), packet->getSenderMacAddress().toString(), packet->getTargetIPv4Address().toString(), packet->getTargetMacAddress().toString());
		}
	}

	void PacketAnalyzer::onTCP_IPv4(IPv4Packet* packet, TcpSegment* tcp, sl_uint8* data, sl_uint32 sizeData, void* userData)
	{
		if (m_flagLogging) {
			StringBuffer buf;
			if (tcp->isNS()) {
				buf.addStatic("NS ");
			}
			if (tcp->isCWR()) {
				buf.addStatic("CWR ");
			}
			if (tcp->isECE()) {
				buf.addStatic("ECE ");
			}
			if (tcp->isURG()) {
				buf.addStatic("URG ");
			}
			if (tcp->isACK()) {
				buf.addStatic("ACK ");
			}
			if (tcp->isPSH()) {
				buf.addStatic("PSH ");
			}
			if (tcp->isRST()) {
				buf.addStatic("RST ");
			}
			if (tcp->isSYN()) {
				buf.addStatic("SYN ");
			}
			if (tcp->isFIN()) {
				buf.addStatic("FIN ");
			}
			Log("TCP", "%s:%s->%s:%s %s(%s Bytes)", packet->getSourceAddress().toString(), tcp->getSourcePort(), packet->getDestinationAddress().toString(), tcp->getDestinationPort(), buf.merge(), sizeData);
		}
	}

	void PacketAnalyzer::onUDP_IPv4(IPv4Packet* packet, UdpDatagram* udp, sl_uint8* data, sl_uint32 sizeData, void* userData)
	{
		if (m_flagLogging) {
			Log("UDP", "%s:%s->%s:%s (%s Bytes)", packet->getSourceAddress().toString(), udp->getSourcePort(), packet->getDestinationAddress().toString(), udp->getDestinationPort(), sizeData);
		}
	}

	void PacketAnalyzer::onICMP_IPv4(IPv4Packet* packet, IcmpHeaderFormat* icmp, sl_uint8* data, sl_uint32 sizeData, void* userData)
	{
		if (m_flagLogging) {
			const char* type;
			switch (icmp->getType()) {
			case IcmpType::Echo:
				type = "Echo";
				break;
			case IcmpType::EchoReply:
				type = "EchoReply";
				break;
			case IcmpType::DestinationUnreachable:
				type = "DestinationUnreachable";
				break;
			case IcmpType::Redirect:
				type = "Redirect";
				break;
			case IcmpType::TimeExceeded:
				type = "TimeExceeded";
				break;
			case IcmpType::ParameterProblem:
				type = "ParameterProblem";
				break;
			case IcmpType::Timestamp:
				type = "Timestamp";
				break;
			case IcmpType::TimestampReply:
				type = "TimestampReply";
				break;
			case IcmpType::AddressMaskRequest:
				type = "AddressMaskRequest";
				break;
			case IcmpType::AddressMaskReply:
				type = "AddressMaskReply";
				break;
			default:
				type = "Uknown Type";
				break;
			}
			Log("ICMP", "%s %s->%s", type, packet->getSourceAddress().toString(), packet->getDestinationAddress().toString());
		}
	}

	void PacketAnalyzer::onDNS_IPv4(IPv4Packet* packet, UdpDatagram* udp, DnsPacket* dns, void* userData)
	{
		if (m_flagLogging) {
			if (dns->flagQuestion) {
				if (dns->questions.isNotEmpty()) {
					DnsPacket::Question* question = dns->questions.getPointerAt(0);
					Log("DNS", "%s:%s->%s:%s Question: %s", packet->getSourceAddress().toString(), udp->getSourcePort(), packet->getDestinationAddress().toString(), udp->getDestinationPort(), question->name);
				}
			} else {
				StringBuffer buf;
				for (auto& addr : dns->addresses) {
					buf.add(String::format("%s(%s) ", addr.name, addr.address.toString()));
				}
				Log("DNS", "%s:%s->%s:%s Response: %s", packet->getSourceAddress().toString(), udp->getSourcePort(), packet->getDestinationAddress().toString(), udp->getDestinationPort(), buf.merge());
			}
		}
	}

	void PacketAnalyzer::analyzeTcpContent(IPv4Packet* packet, TcpSegment* tcp, sl_uint8* data, sl_uint32 sizeData, void* userData)
	{
		sl_uint16 port = tcp->getDestinationPort();
		if (port == 80 || port == 443) {
			if (m_contentAnalyzer.isNull()) {
				MutexLocker lock(&m_lockContentAnalyzer);
				if (m_contentAnalyzer.isNull()) {
					m_contentAnalyzer = new ContentAnalyzer;
					if (m_contentAnalyzer.isNull()) {
						return;
					}
				}
			}
			((ContentAnalyzer*)(m_contentAnalyzer.get()))->analyze((PacketAnalyzerHelper*)this, packet, tcp, data, sizeData, userData);
		}
	}

}
