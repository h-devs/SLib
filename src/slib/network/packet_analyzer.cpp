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

#include "slib/core/rw_lock.h"
#include "slib/core/thread.h"
#include "slib/core/system.h"
#include "slib/core/log.h"
#include "slib/crypto/tls.h"

#define CONTENT_LOOKUP_SIZE 1024
#define CONNECTION_TABLE_LENGTH 4096
#define CONNECTION_TIMEOUT 30000

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

			class TcpConnection
			{
			public:
				sl_uint64 startTime;
				sl_uint32 startSequenceNumber;
				sl_uint32 sizeContent;
				char content[CONTENT_LOOKUP_SIZE];

				sl_bool flagNotHttp;
				sl_bool flagNotHttps;

			public:
				TcpConnection() noexcept : startSequenceNumber(0), sizeContent(0)
				{
					startTime = System::getHighResolutionTickCount();
					flagNotHttp = sl_false;
					flagNotHttps = sl_false;
				}

			public:
				sl_bool addContent(TcpSegment* tcp, sl_uint8* data, sl_uint32 sizeData) noexcept
				{
					sl_uint32 offset = tcp->getSequenceNumber() - startSequenceNumber;
					if (!offset) {
						return sl_true;
					}
					offset--;
					if (offset > CONTENT_LOOKUP_SIZE) {
						return sl_false;
					}
					if (!sizeData) {
						return sl_true;
					}
					if (sizeData > CONTENT_LOOKUP_SIZE - offset) {
						sizeData = CONTENT_LOOKUP_SIZE - offset;
					}
					Base::copyMemory(content + offset, data, sizeData);
					if (sizeContent < offset + sizeData) {
						sizeContent = offset + sizeData;
					}
					return sl_true;
				}

			};

			class TcpConnectionTableEntry
			{
			public:
				Shared<TcpConnection> get(const IPv4Address& address, sl_uint16 port) noexcept
				{
					if (m_table.isEmpty()) {
						return sl_null;
					}
					sl_uint64 key = toKey(address, port);
					ReadLocker locker(&m_lock);
					return m_table.getValue(key);
				}

				Shared<TcpConnection> create(const IPv4Address& address, sl_uint16 port) noexcept
				{
					sl_uint64 key = toKey(address, port);
					Shared<TcpConnection> connection = Shared<TcpConnection>::create();
					if (connection.isNotNull()) {
						WriteLocker locker(&m_lock);
						if (m_table.put(key, connection)) {
							return connection;
						}
					}
					return sl_null;
				}

				void remove(const IPv4Address& address, sl_uint16 port) noexcept
				{
					if (m_table.isEmpty()) {
						return;
					}
					sl_uint64 key = toKey(address, port);
					WriteLocker locker(&m_lock);
					m_table.remove(key);
				}

				void removeOldConnections(sl_uint64 now) noexcept
				{
					CList<sl_uint64> listRemove;
					sl_bool flagValidTable = sl_false;
					{
						ReadLocker lock(&m_lock);
						for (auto& item : m_table) {
							if (item.value->startTime + CONNECTION_TIMEOUT < now) {
								listRemove.add_NoLock(item.key);
							} else {
								flagValidTable = sl_true;
							}
						}
					}
					if (flagValidTable) {
						ListElements<sl_uint64> list(listRemove);
						for (sl_size i = 0; i < list.count; i++) {
							WriteLocker lock(&m_lock);
							Shared<TcpConnection> old;
							if (m_table.remove(list[i], &old)) {
								if (old->startTime + CONNECTION_TIMEOUT > now) {
									m_table.put(list[i], Move(old));
								}
							}
						}
					} else {
						WriteLocker lock(&m_lock);
						m_table.removeAll();
					}
				}

			private:
				sl_uint64 toKey(const IPv4Address& address, sl_uint16 port) noexcept
				{
					return SLIB_MAKE_QWORD4(address.getInt(), port);
				}

			private:
				HashTable< sl_uint64, Shared<TcpConnection> > m_table;
				ReadWriteLock m_lock;
			};

			class TcpConnectionTable
			{
			public:
				TcpConnectionTable()
				{
					m_threadGC = Thread::start(SLIB_FUNCTION_MEMBER(TcpConnectionTable, onRunGC, this));
				}

				~TcpConnectionTable()
				{
					if (m_threadGC.isNotNull()) {
						m_threadGC->finishAndWait();
					}
				}

			public:
				Shared<TcpConnection> get(const IPv4Address& address, sl_uint16 port) noexcept
				{
					return m_entries[port & (CONNECTION_TABLE_LENGTH - 1)].get(address, port);
				}

				Shared<TcpConnection> create(const IPv4Address& address, sl_uint16 port) noexcept
				{
					return m_entries[port & (CONNECTION_TABLE_LENGTH - 1)].create(address, port);
				}

				void remove(const IPv4Address& address, sl_uint16 port) noexcept
				{
					m_entries[port & (CONNECTION_TABLE_LENGTH - 1)].remove(address, port);
				}

				void onRunGC()
				{
					Thread* thread = Thread::getCurrent();
					if (!thread) {
						return;
					}
					while (thread->isNotStopping()) {
						sl_uint64 now = System::getHighResolutionTickCount();
						for (sl_size i = 0; i < CONNECTION_TABLE_LENGTH && thread->isNotStopping(); i++) {
							m_entries[i].removeOldConnections(now);
						}
						thread->wait(20000);
					}
				}

			private:
				TcpConnectionTableEntry m_entries[CONNECTION_TABLE_LENGTH];
				Ref<Thread> m_threadGC;

			};

			class ContentAnalyzer : public Referable
			{
			public:
				void analyze(PacketAnalyzerHelper* parent, IPv4Packet* packet, TcpSegment* tcp, sl_uint8* data, sl_uint32 sizeData, void* userData)
				{
					Shared<TcpConnection> connection;
					if (tcp->isSYN()) {
						if (tcp->isACK()) {
							return;
						}
						connection = m_table.create(packet->getSourceAddress(), tcp->getSourcePort());
						if (connection.isNull()) {
							return;
						}
						connection->startSequenceNumber = tcp->getSequenceNumber();
					} else if (tcp->isFIN()) {
						m_table.remove(packet->getSourceAddress(), tcp->getSourcePort());
						m_table.remove(packet->getDestinationAddress(), tcp->getDestinationPort());
						return;
					} else {
						connection = m_table.get(packet->getSourceAddress(), tcp->getSourcePort());
						if (connection.isNull()) {
							return;
						}
					}
					if (!sizeData) {
						return;
					}
					TcpConnection* c = connection.get();
					if (!(c->addContent(tcp, data, sizeData))) {
						m_table.remove(packet->getSourceAddress(), tcp->getSourcePort());
						return;
					}
					sl_bool flagNotHttp = sl_true;
					if (parent->m_flagAnalyzeHttp) {
						flagNotHttp = c->flagNotHttp;
						if (!flagNotHttp) {
							ContentResult result = analyzeHttp(parent, packet, tcp, c->content, c->sizeContent, userData);
							if (result == ContentResult::Success) {
								m_table.remove(packet->getSourceAddress(), tcp->getSourcePort());
							} else if (result == ContentResult::Error) {
								c->flagNotHttp = sl_true;
								flagNotHttp = sl_true;
							}
						}
					}
					sl_bool flagNotHttps = sl_true;
					if (parent->m_flagAnalyzeHttps) {
						flagNotHttps = c->flagNotHttps;
						if (!flagNotHttps) {
							ContentResult result = analyzeHttps(parent, packet, tcp, c->content, c->sizeContent, userData);
							if (result == ContentResult::Success) {
								m_table.remove(packet->getSourceAddress(), tcp->getSourcePort());
							} else if (result == ContentResult::Error) {
								c->flagNotHttps = sl_true;
								flagNotHttps = sl_true;
							}
						}
					}
					if (flagNotHttp && flagNotHttps) {
						m_table.remove(packet->getSourceAddress(), tcp->getSourcePort());
					}
				}

				enum class ContentResult
				{
					InProgress = 0,
					Success = 1,
					Error = 2
				};

				ContentResult analyzeHttp(PacketAnalyzerHelper* parent, IPv4Packet* packet, TcpSegment* tcp, char* content, sl_uint32 size, void* userData)
				{
					if (size < 4) {
						return ContentResult::InProgress;
					}
					if (!(Base::equalsStringIgnoreCase(content, "GET ", 4))) {
						return ContentResult::Error;
					}
					char* uri = content + 4;
					sl_size n = size - 4;
					char* strFirstLine = (char*)(Base::findMemory(uri, n, '\r'));
					if (!strFirstLine) {
						if (size > 512) {
							return ContentResult::Error;
						}
						return ContentResult::InProgress;
					}
					if (strFirstLine < uri + 9) {
						return ContentResult::Error;
					}
					if (!(Base::equalsStringIgnoreCase(strFirstLine - 9, " HTTP/1.", 8))) {
						return ContentResult::Error;
					}
					if (*(strFirstLine - 1) != '1' && *(strFirstLine - 1) != '0') {
						return ContentResult::Error;
					}
					sl_size lenUri = strFirstLine - 9 - uri;
					n = content + size - strFirstLine;
					if (n < 2) {
						return ContentResult::InProgress;
					}
					if (strFirstLine[1] != '\n') {
						return ContentResult::Error;
					}
					char* startHeader = strFirstLine + 2;
					n -= 2;
					char* s = startHeader;
					for (;;) {
						if (!n) {
							return ContentResult::InProgress;
						}
						char* line = (char*)(Base::findMemory(s, n, '\r'));
						if (!line) {
							return ContentResult::InProgress;
						}
						n = content + size - line;
						if (n >= 2) {
							if (line[1] != '\n') {
								return ContentResult::Error;
							}
						}
						sl_size m = line - s;
						if (!m) {
							return ContentResult::Error;
						}
						char* sep = (char*)(Base::findMemory(s, m, ':'));
						if (sep) {
							if (sep == s + 4 && Base::equalsStringIgnoreCase(s, "Host", 4)) {
								char* host = sep + 1;
								if (host < line && *host == ' ') {
									host++;
								}
								sl_size lenHost = line - host;
								parent->onHTTP_IPv4(packet, tcp, StringView(host, lenHost), StringView(uri, lenUri), userData);
								return ContentResult::Success;
							}
						}
						n -= 2;
						s = line + 2;
					}
				}

				ContentResult analyzeHttps(PacketAnalyzerHelper* parent, IPv4Packet* packet, TcpSegment* tcp, char* content, sl_uint32 size, void* userData)
				{
					if (size <= sizeof(TlsRecordHeader) + sizeof(TlsHandshakeProtocolHeader)) {
						return ContentResult::InProgress;
					}
					TlsRecordHeader* record = (TlsRecordHeader*)content;
					if (record->getType() != TlsRecordType::Handshake) {
						return ContentResult::Error;
					}
					content += sizeof(TlsRecordHeader);
					size -= sizeof(TlsRecordHeader);
					sl_uint16 sizeProtocol = record->getContentLength();
					if (size > sizeProtocol) {
						size = sizeProtocol;
					}
					TlsHandshakeProtocolHeader* protocol = (TlsHandshakeProtocolHeader*)content;
					if (protocol->getType() != TlsHandshakeType::ClientHello) {
						return ContentResult::Error;
					}
					content += sizeof(TlsHandshakeProtocolHeader);
					size -= sizeof(TlsHandshakeProtocolHeader);
					sl_uint32 sizeMessage = protocol->getContentLength();
					if (size > sizeMessage) {
						size = sizeMessage;
					}
					TlsClientHelloMessage msg;
					sl_int32 iResult = msg.parse(content, size);
					if (iResult >= 0) {
						ListElements<TlsExtension> extensions(msg.extensions);
						for (sl_size i = 0; i < extensions.count; i++) {
							if (extensions[i].type == TlsExtensionType::ServerName) {
								TlsServerNameIndicationExtension sni;
								if (sni.parse(extensions[i].data, extensions[i].length)) {
									parent->onHTTPS_IPv4(packet, tcp, sni.serverNames.getValueAt(0), userData);
								}
								return ContentResult::Success;
							}
						}
						if (iResult) {
							return ContentResult::Success;
						} else {
							return ContentResult::InProgress;
						}
					}
					return ContentResult::Error;
				}

			private:
				TcpConnectionTable m_table;

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

		m_flagIgnoreLocalPackets = sl_false;
		m_flagIgnoreUnknownPorts = sl_false;
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
			if (m_flagIgnoreLocalPackets) {
				if (ip->getSourceAddress().isSpecial() && ip->getDestinationAddress().isSpecial()) {
					return;
				}
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

	void PacketAnalyzer::setIgnoringLocalPackets(sl_bool flag)
	{
		m_flagIgnoreLocalPackets = flag;
	}

	void PacketAnalyzer::setIgnoringUnknownPorts(sl_bool flag)
	{
		m_flagIgnoreUnknownPorts = flag;
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
			Log("TCP", "%s:%s->%s:%s, %s (%s Bytes)", packet->getSourceAddress().toString(), tcp->getSourcePort(), packet->getDestinationAddress().toString(), tcp->getDestinationPort(), buf.merge(), sizeData);
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
			Log("ICMP", "%s: %s->%s", type, packet->getSourceAddress().toString(), packet->getDestinationAddress().toString());
		}
	}

	void PacketAnalyzer::onDNS_IPv4(IPv4Packet* packet, UdpDatagram* udp, DnsPacket* dns, void* userData)
	{
		if (m_flagLogging) {
			if (dns->flagQuestion) {
				if (dns->questions.isNotEmpty()) {
					DnsPacket::Question* question = dns->questions.getPointerAt(0);
					Log("DNS", "%s:%s->%s:%s, Question: %s", packet->getSourceAddress().toString(), udp->getSourcePort(), packet->getDestinationAddress().toString(), udp->getDestinationPort(), question->name);
				}
			} else {
				StringBuffer buf;
				for (auto& addr : dns->addresses) {
					buf.add(String::format("%s(%s) ", addr.name, addr.address.toString()));
				}
				Log("DNS", "%s:%s->%s:%s, Response: %s", packet->getSourceAddress().toString(), udp->getSourcePort(), packet->getDestinationAddress().toString(), udp->getDestinationPort(), buf.merge());
			}
		}
	}

	void PacketAnalyzer::onHTTP_IPv4(IPv4Packet* packet, TcpSegment* tcp, const StringView& host, const StringView& uri, void* userData)
	{
		if (m_flagLogging) {
			Log("HTTP", "%s:%s->%s:%s, Host: %s, URI: %s", packet->getSourceAddress().toString(), tcp->getSourcePort(), packet->getDestinationAddress().toString(), tcp->getDestinationPort(), host, uri);
		}
	}

	void PacketAnalyzer::onHTTPS_IPv4(IPv4Packet* packet, TcpSegment* tcp, const StringView& host, void* userData)
	{
		if (m_flagLogging) {
			Log("HTTPS", "%s:%s->%s:%s, Host: %s", packet->getSourceAddress().toString(), tcp->getSourcePort(), packet->getDestinationAddress().toString(), tcp->getDestinationPort(), host);
		}
	}

	void PacketAnalyzer::analyzeTcpContent(IPv4Packet* packet, TcpSegment* tcp, sl_uint8* data, sl_uint32 sizeData, void* userData)
	{
		if (m_flagIgnoreUnknownPorts) {
			sl_uint16 port = tcp->getDestinationPort();
			if (port != 80 && port != 443) {
				return;
			}
		}
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
