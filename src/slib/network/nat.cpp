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

#include "slib/network/nat.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(NatTableParam)

	NatTableParam::NatTableParam()
	{
		targetAddress.setZero();

		tcpPortBegin = 1024;
		tcpPortEnd = 65535;

		udpPortBegin = 1024;
		udpPortEnd = 65535;

		icmpEchoIdentifier = 30000;
	}


	NatTable::NatTable()
	{
		m_icmpEchoSequenceCurrent = 0;
	}

	NatTable::~NatTable()
	{
	}

	sl_bool NatTable::initialize(const NatTableParam& param)
	{
		if (m_targetAddress.isNotZero()) {
			return sl_true;
		}
		if (param.targetAddress.isZero()) {
			return sl_false;
		}
		if (!(m_mappingTcp.initialize(param.tcpPortBegin, param.tcpPortEnd))) {
			return sl_false;
		}
		if (!(m_mappingUdp.initialize(param.udpPortBegin, param.udpPortEnd))) {
			return sl_false;
		}
		m_targetAddress = param.targetAddress;
		m_icmpEchoIdentifier = param.icmpEchoIdentifier;
		return sl_true;
	}

	namespace
	{
		static sl_uint16 OneComplementMinus(sl_uint16 a, sl_uint16 b)
		{
			b = ~b;
			sl_uint32 sum = a;
			sum += b;
			while (sum >> 16) {
				sum = (sum >> 16) + (sum & 0xffff);
			}
			return sum;
		}

		static sl_uint16 GetUpdatedChecksum(sl_uint16 original, const IPv4Address& oldIP, const IPv4Address& newIP, sl_uint16 oldPort, sl_uint16 newPort)
		{
			sl_uint32 sum = (sl_uint16)(~original);
			sum += (sl_uint16)(~SLIB_MAKE_WORD(oldIP.a, oldIP.b));
			sum += (sl_uint16)(~SLIB_MAKE_WORD(oldIP.c, oldIP.d));
			sum += SLIB_MAKE_WORD(newIP.a, newIP.b);
			sum += SLIB_MAKE_WORD(newIP.c, newIP.d);
			sum += (sl_uint16)(~oldPort);
			sum += newPort;
			while (sum >> 16) {
				sum = (sum >> 16) + (sum & 0xffff);
			}
			return (sl_uint16)(~sum);
		}
	}

	sl_bool NatTable::translateOutgoingPacket(IPv4Packet* header, void* content, sl_uint32 sizeContent, sl_uint64 currentTick)
	{
		InternetProtocol protocol = header->getProtocol();
		if (protocol == InternetProtocol::TCP) {
			if (!(header->getFragmentOffset())) {
				if (!(TcpSegment::checkSize(content, sizeContent))) {
					return sl_false;
				}
				TcpSegment* tcp = (TcpSegment*)content;
				IPv4Address srcIP = header->getSourceAddress();
				sl_uint16 srcPort = tcp->getSourcePort();
				sl_uint16 targetPort;
				if (!(m_mappingTcp.mapToExternal(srcIP, srcPort, targetPort, currentTick))) {
					return sl_false;
				}
				tcp->setChecksum(GetUpdatedChecksum(tcp->getChecksum(), srcIP, m_targetAddress, srcPort, targetPort));
				tcp->setSourcePort(targetPort);
			}
			header->setSourceAddress(m_targetAddress);
			header->updateChecksum();
			return sl_true;
		} else if (protocol == InternetProtocol::UDP) {
			if (!(header->getFragmentOffset())) {
				if (sizeContent < UdpDatagram::HeaderSize) {
					return sl_false;
				}
				UdpDatagram* udp = (UdpDatagram*)content;
				IPv4Address srcIP = header->getSourceAddress();
				sl_uint16 srcPort = udp->getSourcePort();
				sl_uint16 targetPort;
				if (!(m_mappingUdp.mapToExternal(srcIP, srcPort, targetPort, currentTick))) {
					return sl_false;
				}
				udp->setChecksum(GetUpdatedChecksum(udp->getChecksum(), srcIP, m_targetAddress, srcPort, targetPort));
				udp->setSourcePort(targetPort);
			}
			header->setSourceAddress(m_targetAddress);
			header->updateChecksum();
			return sl_true;
		} else if (protocol == InternetProtocol::ICMP) {
			if (header->getFragmentOffset() || header->isMF()) {
				return sl_false;
			}
			if (sizeContent < sizeof(IcmpHeaderFormat)) {
				return sl_false;
			}
			IcmpHeaderFormat* icmp = (IcmpHeaderFormat*)content;
			IcmpType type = icmp->getType();
			if (type == IcmpType::Echo) {
				IcmpEchoAddress address;
				address.ip = header->getSourceAddress();
				address.identifier = icmp->getEchoIdentifier();
				address.sequenceNumber = icmp->getEchoSequenceNumber();
				sl_uint16 sn = getMappedIcmpEchoSequenceNumber(address);
				icmp->setEchoIdentifier(m_icmpEchoIdentifier);
				icmp->setEchoSequenceNumber(sn);
				icmp->updateChecksum(sizeContent);
				header->setSourceAddress(m_targetAddress);
				header->updateChecksum();
				return sl_true;
			} else if (type == IcmpType::DestinationUnreachable || type == IcmpType::TimeExceeded) {
				IPv4Packet* ipOrig = (IPv4Packet*)(icmp->getContent());
				sl_uint32 sizeOrig = sizeContent - sizeof(IcmpHeaderFormat);
				IPv4Address addressSrc = header->getSourceAddress();
				if (!(IPv4Packet::checkHeaderSize(ipOrig, sizeOrig)) || sizeOrig < (sl_uint32)(ipOrig->getHeaderSize() + 8) || ipOrig->getDestinationAddress() != addressSrc) {
					return sl_false;
				}
				InternetProtocol protocolOrig = ipOrig->getProtocol();
				if (protocolOrig == InternetProtocol::TCP) {
					TcpSegment* tcp = (TcpSegment*)(ipOrig->getContent());
					sl_uint16 targetPort;
					if (!(m_mappingTcp.mapToExternal(addressSrc, tcp->getDestinationPort(), targetPort, currentTick))) {
						return sl_false;
					}
					tcp->setDestinationPort(targetPort);
					tcp->setChecksum(0);
					ipOrig->setDestinationAddress(m_targetAddress);
					ipOrig->updateChecksum();
					icmp->updateChecksum(sizeContent);
					header->setSourceAddress(m_targetAddress);
					header->updateChecksum();
					return sl_true;
				} else if (protocolOrig == InternetProtocol::UDP) {
					UdpDatagram* udp = (UdpDatagram*)(ipOrig->getContent());
					sl_uint16 targetPort;
					if (!(m_mappingUdp.mapToExternal(addressSrc, udp->getDestinationPort(), targetPort, currentTick))) {
						return sl_false;
					}
					udp->setDestinationPort(targetPort);
					udp->setChecksum(0);
					ipOrig->setDestinationAddress(m_targetAddress);
					ipOrig->updateChecksum();
					icmp->updateChecksum(sizeContent);
					header->setSourceAddress(m_targetAddress);
					header->updateChecksum();
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool NatTable::translateIncomingPacket(IPv4Packet* header, void* content, sl_uint32 sizeContent, sl_uint64 currentTick)
	{
		IPv4Address dstIP = header->getDestinationAddress();
		if (dstIP != m_targetAddress) {
			return sl_false;
		}
		InternetProtocol protocol = header->getProtocol();
		if (protocol == InternetProtocol::TCP) {
			if (header->getFragmentOffset()) {
				IPv4Address& ip = m_tcpFragmentTable[header->getIdentification()];
				if (ip.isZero()) {
					return sl_false;
				}
				header->setDestinationAddress(ip);
				if (!(header->isMF())) {
					ip.setZero();
				}
			} else {
				if (!(TcpSegment::checkSize(content, sizeContent))) {
					return sl_false;
				}
				TcpSegment* tcp = (TcpSegment*)content;
				IPv4Address internalIP;
				sl_uint16 internalPort;
				sl_uint16 dstPort = tcp->getDestinationPort();
				if (!(m_mappingTcp.mapToInternal(dstPort, internalIP, internalPort, currentTick))) {
					return sl_false;
				}
				tcp->setChecksum(GetUpdatedChecksum(tcp->getChecksum(), dstIP, internalIP, dstPort, internalPort));
				tcp->setDestinationPort(internalPort);
				header->setDestinationAddress(internalIP);
				if (header->isMF()) {
					m_tcpFragmentTable[header->getIdentification()] = internalIP;
				}
			}
			header->updateChecksum();
			return sl_true;
		} else if (protocol == InternetProtocol::UDP) {
			if (header->getFragmentOffset()) {
				IPv4Address& ip = m_udpFragmentTable[header->getIdentification()];
				if (ip.isZero()) {
					return sl_false;
				}
				header->setDestinationAddress(ip);
				if (!(header->isMF())) {
					ip.setZero();
				}
			} else {
				if (sizeContent < UdpDatagram::HeaderSize) {
					return sl_false;
				}
				UdpDatagram* udp = (UdpDatagram*)content;
				IPv4Address internalIP;
				sl_uint16 internalPort;
				sl_uint16 dstPort = udp->getDestinationPort();
				if (!(m_mappingUdp.mapToInternal(dstPort, internalIP, internalPort, currentTick))) {
					return sl_false;
				}
				udp->setChecksum(GetUpdatedChecksum(udp->getChecksum(), dstIP, internalIP, dstPort, internalPort));
				udp->setDestinationPort(internalPort);
				header->setDestinationAddress(internalIP);
				if (header->isMF()) {
					m_udpFragmentTable[header->getIdentification()] = internalIP;
				}
			}
			header->updateChecksum();
			return sl_true;
		} else if (protocol == InternetProtocol::ICMP) {
			if (header->getFragmentOffset() || header->isMF()) {
				return sl_false;
			}
			if (sizeContent < sizeof(IcmpHeaderFormat)) {
				return sl_false;
			}
			IcmpHeaderFormat* icmp = (IcmpHeaderFormat*)content;
			IcmpType type = icmp->getType();
			if (type == IcmpType::EchoReply) {
				if (icmp->getEchoIdentifier() != m_icmpEchoIdentifier) {
					return sl_false;
				}
				IcmpEchoElement element;
				if (!(m_mapIcmpEchoIncoming.get_NoLock(icmp->getEchoSequenceNumber(), &element))) {
					return sl_false;
				}
				icmp->setEchoIdentifier(element.addressSource.identifier);
				icmp->setEchoSequenceNumber(element.addressSource.sequenceNumber);
				icmp->updateChecksum(sizeContent);
				header->setDestinationAddress(element.addressSource.ip);
				header->updateChecksum();
				return sl_true;
			} else if (type == IcmpType::DestinationUnreachable || type == IcmpType::TimeExceeded) {
				IPv4Packet* ipOrig = (IPv4Packet*)(icmp->getContent());
				sl_uint32 sizeOrig = sizeContent - sizeof(IcmpHeaderFormat);
				if (!(IPv4Packet::checkHeaderSize(ipOrig, sizeOrig)) || sizeOrig < (sl_uint32)(ipOrig->getHeaderSize() + 8) || ipOrig->getSourceAddress() != m_targetAddress) {
					return sl_false;
				}
				InternetProtocol protocolOrig = ipOrig->getProtocol();
				if (protocolOrig == InternetProtocol::TCP) {
					TcpSegment* tcp = (TcpSegment*)(ipOrig->getContent());
					IPv4Address internalIP;
					sl_uint16 internalPort;
					if (!(m_mappingTcp.mapToInternal(tcp->getSourcePort(), internalIP, internalPort, currentTick))) {
						return sl_false;
					}
					tcp->setSourcePort(internalPort);
					tcp->setChecksum(0);
					ipOrig->setSourceAddress(internalIP);
					ipOrig->updateChecksum();
					icmp->updateChecksum(sizeContent);
					header->setDestinationAddress(internalIP);
					header->updateChecksum();
					return sl_true;
				} else if (protocolOrig == InternetProtocol::UDP) {
					UdpDatagram* udp = (UdpDatagram*)(ipOrig->getContent());
					IPv4Address internalIP;
					sl_uint16 internalPort;
					if (!(m_mappingUdp.mapToInternal(udp->getSourcePort(), internalIP, internalPort, currentTick))) {
						return sl_false;
					}
					udp->setSourcePort(internalPort);
					udp->setChecksum(0);
					ipOrig->setSourceAddress(internalIP);
					ipOrig->updateChecksum();
					icmp->updateChecksum(sizeContent);
					header->setDestinationAddress(internalIP);
					header->updateChecksum();
					return sl_true;
				} else if (protocolOrig == InternetProtocol::ICMP) {
					IcmpHeaderFormat* icmpOrig = (IcmpHeaderFormat*)(ipOrig->getContent());
					if (icmpOrig->getType() == IcmpType::Echo) {
						if (icmpOrig->getEchoIdentifier() != m_icmpEchoIdentifier) {
							return sl_false;
						}
						IcmpEchoElement element;
						if (!(m_mapIcmpEchoIncoming.get_NoLock(icmp->getEchoSequenceNumber(), &element))) {
							return sl_false;
						}
						icmpOrig->setEchoIdentifier(element.addressSource.identifier);
						icmpOrig->setEchoSequenceNumber(element.addressSource.sequenceNumber);
						icmpOrig->setChecksum(0);
						ipOrig->setSourceAddress(element.addressSource.ip);
						ipOrig->updateChecksum();
						icmp->updateChecksum(sizeContent);
						header->setDestinationAddress(element.addressSource.ip);
						header->updateChecksum();
						return sl_true;
					}
				}
			}
		}
		return sl_false;
	}

	sl_uint16 NatTable::getMappedIcmpEchoSequenceNumber(const IcmpEchoAddress& address)
	{
		IcmpEchoElement element;
		if (m_mapIcmpEchoOutgoing.get_NoLock(address, &element)) {
			return element.sequenceNumberTarget;
		}
		sl_uint16 sn = ++ m_icmpEchoSequenceCurrent;
		if (m_mapIcmpEchoIncoming.get_NoLock(sn, &element)) {
			m_mapIcmpEchoOutgoing.remove_NoLock(element.addressSource);
		}
		element.addressSource = address;
		element.sequenceNumberTarget = sn;
		m_mapIcmpEchoOutgoing.put_NoLock(address, element);
		m_mapIcmpEchoIncoming.put_NoLock(sn, element);
		return sn;
	}


	NatTableMapping::NatTableMapping()
	{
		m_ports = sl_null;
		m_nPorts = 0;
		m_pos = 0;

		m_portBegin = 0;
		m_portEnd = 0;
	}

	NatTableMapping::~NatTableMapping()
	{
		if (m_ports) {
			Base::freeMemory(m_ports);
		}
	}

	sl_bool NatTableMapping::initialize(sl_uint16 portBegin, sl_uint16 portEnd)
	{
		if (m_ports) {
			return sl_true;
		}
		if (portEnd < portBegin) {
			return sl_false;
		}
		m_portBegin = portBegin;
		m_portEnd = portEnd;
		m_nPorts = portEnd - portBegin + 1;
		sl_size size = sizeof(NatTablePort) * m_nPorts;
		m_ports = (NatTablePort*)(Base::createZeroMemory(size));
		return m_ports != sl_null;
	}

	namespace
	{
		static sl_uint64 GetAddressKey(const IPv4Address& ip, sl_uint16 port)
		{
			sl_uint64 n = ip.toInt();
			return (n << 16) | port;
		}
	}

	sl_bool NatTableMapping::mapToExternal(const IPv4Address& internalIP, sl_uint16 internalPort, sl_uint16& externalPort, sl_uint64 currentTick)
	{
		sl_uint64 key = GetAddressKey(internalIP, internalPort);
		if (m_mapTranslation.get_NoLock(key, &externalPort)) {
			m_ports[externalPort - m_portBegin].lastAccessTick = currentTick;
			return sl_true;
		}
		sl_uint16 pos = m_pos;
		sl_uint64 timeAccessMin = 0;
		sl_uint64 timeAccessMax = 0;
		sl_uint32 n = m_nPorts << 1;
		for (sl_uint32 i = 0; i < n; i++) {
			{
				NatTablePort& port = m_ports[pos];
				if (port.flagActive) {
					sl_uint64 t = port.lastAccessTick;
					if (t > timeAccessMax) {
						timeAccessMax = t;
					}
					if (!timeAccessMin || t < timeAccessMin) {
						timeAccessMin = t;
					}
				} else {
					externalPort = pos + m_portBegin;
					port.flagActive = sl_true;
					port.sourceIP = internalIP.toInt();
					port.sourcePort = internalPort;
					port.lastAccessTick = currentTick;
					m_mapTranslation.put_NoLock(key, externalPort);
					m_pos = (pos + 1) % m_nPorts;
					return sl_true;
				}
			}
			pos = (pos + 1) % m_nPorts;
			if (i == m_nPorts) {
				sl_uint64 mid = (timeAccessMin + timeAccessMax) / 2;
				for (sl_uint16 k = 0; k < m_nPorts; k++) {
					NatTablePort& port = m_ports[k];
					if (port.lastAccessTick <= mid) {
						port.flagActive = sl_false;
						m_mapTranslation.remove_NoLock(GetAddressKey(port.sourceIP, port.sourcePort));
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool NatTableMapping::mapToInternal(sl_uint16 externalPort, IPv4Address& internalIP, sl_uint16& internalPort, sl_uint64 currentTick)
	{
		if (externalPort >= m_portBegin && externalPort <= m_portEnd) {
			sl_uint32 k = externalPort - m_portBegin;
			NatTablePort& port = m_ports[k];
			if (port.flagActive) {
				port.lastAccessTick = currentTick;
				internalIP = port.sourceIP;
				internalPort = port.sourcePort;
				return sl_true;
			}
		}
		return sl_false;
	}

}
