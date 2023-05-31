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

#include "slib/network/nat.h"

#include "slib/core/new_helper.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(NatTableParam)

	NatTableParam::NatTableParam()
	{
		targetAddress.setZero();

		tcpPortBegin = 30000;
		tcpPortEnd = 60000;

		udpPortBegin = 30000;
		udpPortEnd = 60000;

		icmpEchoIdentifier = 30000;
	}


	SLIB_DEFINE_OBJECT(NatTable, Object)

	NatTable::NatTable()
	{
		m_icmpEchoSequenceCurrent = 0;
	}

	NatTable::~NatTable()
	{
	}

	const NatTableParam& NatTable::getParam() const
	{
		return m_param;
	}

	void NatTable::setup(const NatTableParam& param)
	{
		ObjectLocker lock(this);
		m_param = param;
		m_mappingTcp.setup(param.tcpPortBegin, param.tcpPortEnd);
		m_mappingUdp.setup(param.udpPortBegin, param.udpPortEnd);
	}

	sl_bool NatTable::translateOutgoingPacket(IPv4Packet* ipHeader, void* ipContent, sl_uint32 sizeContent)
	{
		IPv4Address addressTarget = m_param.targetAddress;
		if (addressTarget.isZero()) {
			return sl_false;
		}
		InternetProtocol protocol = ipHeader->getProtocol();
		if (protocol == InternetProtocol::TCP) {
			TcpSegment* tcp = (TcpSegment*)(ipContent);
			if (tcp->check(ipHeader, sizeContent)) {
				sl_uint16 targetPort;
				if (m_mappingTcp.mapToExternalPort(SocketAddress(ipHeader->getSourceAddress(), tcp->getSourcePort()), targetPort)) {
					tcp->setSourcePort(targetPort);
					ipHeader->setSourceAddress(addressTarget);
					tcp->updateChecksum(ipHeader, sizeContent);
					ipHeader->updateChecksum();
					return sl_true;
				}
			}
		} else if (protocol == InternetProtocol::UDP) {
			UdpDatagram* udp = (UdpDatagram*)(ipContent);
			if (udp->check(ipHeader, sizeContent)) {
				sl_uint16 targetPort;
				if (m_mappingUdp.mapToExternalPort(SocketAddress(ipHeader->getSourceAddress(), udp->getSourcePort()), targetPort)) {
					udp->setSourcePort(targetPort);
					ipHeader->setSourceAddress(addressTarget);
					udp->updateChecksum(ipHeader);
					ipHeader->updateChecksum();
					return sl_true;
				}
			}
		} else if (protocol == InternetProtocol::ICMP) {
			IcmpHeaderFormat* icmp = (IcmpHeaderFormat*)(ipContent);
			if (icmp->check(sizeContent)) {
				if (icmp->getType() == IcmpType::Echo) {
					IcmpEchoAddress address;
					address.ip = ipHeader->getSourceAddress();
					address.identifier = icmp->getEchoIdentifier();
					address.sequenceNumber = icmp->getEchoSequenceNumber();
					sl_uint16 sn = getMappedIcmpEchoSequenceNumber(address);
					icmp->setEchoIdentifier(m_param.icmpEchoIdentifier);
					icmp->setEchoSequenceNumber(sn);
					ipHeader->setSourceAddress(addressTarget);
					icmp->updateChecksum(sizeContent);
					ipHeader->updateChecksum();
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool NatTable::translateIncomingPacket(IPv4Packet* ipHeader, void* ipContent, sl_uint32 sizeContent)
	{
		IPv4Address addressTarget = m_param.targetAddress;
		if (addressTarget.isZero()) {
			return sl_false;
		}
		if (ipHeader->getDestinationAddress() != addressTarget) {
			return sl_false;
		}
		InternetProtocol protocol = ipHeader->getProtocol();
		if (protocol == InternetProtocol::TCP) {
			TcpSegment* tcp = (TcpSegment*)(ipContent);
			if (tcp->check(ipHeader, sizeContent)) {
				SocketAddress addressSource;
				if (m_mappingTcp.mapToInternalAddress(tcp->getDestinationPort(), addressSource)) {
					ipHeader->setDestinationAddress(addressSource.ip.getIPv4());
					tcp->setDestinationPort(addressSource.port);
					tcp->updateChecksum(ipHeader, sizeContent);
					ipHeader->updateChecksum();
					return sl_true;
				}
			}
		} else if (protocol == InternetProtocol::UDP) {
			UdpDatagram* udp = (UdpDatagram*)(ipHeader->getContent());
			if (udp->check(ipHeader, sizeContent)) {
				SocketAddress addressSource;
				if (m_mappingUdp.mapToInternalAddress(udp->getDestinationPort(), addressSource)) {
					ipHeader->setDestinationAddress(addressSource.ip.getIPv4());
					udp->setDestinationPort(addressSource.port);
					udp->updateChecksum(ipHeader);
					ipHeader->updateChecksum();
					return sl_true;
				}
			}
		} else if (protocol == InternetProtocol::ICMP) {
			IcmpHeaderFormat* icmp = (IcmpHeaderFormat*)(ipContent);
			if (icmp->check(sizeContent)) {
				IcmpType type = icmp->getType();
				if (type == IcmpType::EchoReply) {
					if (icmp->getEchoIdentifier() == m_param.icmpEchoIdentifier) {
						IcmpEchoElement element;
						if (m_mapIcmpEchoIncoming.get(icmp->getEchoSequenceNumber(), &element)) {
							ipHeader->setDestinationAddress(element.addressSource.ip);
							icmp->setEchoIdentifier(element.addressSource.identifier);
							icmp->setEchoSequenceNumber(element.addressSource.sequenceNumber);
							icmp->updateChecksum(sizeContent);
							ipHeader->updateChecksum();
							return sl_true;
						}
					}
				} else if (type == IcmpType::DestinationUnreachable || type == IcmpType::TimeExceeded) {
					IPv4Packet* ipOrig = (IPv4Packet*)(icmp->getContent());
					sl_uint32 sizeOrig = sizeContent - sizeof(IcmpHeaderFormat);
					if (sizeOrig == sizeof(IPv4Packet)+8 && IPv4Packet::checkHeader(ipOrig, sizeOrig) && ipOrig->getDestinationAddress() == addressTarget) {
						InternetProtocol protocolOrig = ipOrig->getProtocol();
						if (protocolOrig == InternetProtocol::TCP) {
							TcpSegment* tcp = (TcpSegment*)(ipOrig->getContent());
							SocketAddress addressSource;
							if (m_mappingTcp.mapToInternalAddress(tcp->getDestinationPort(), addressSource)) {
								ipOrig->setDestinationAddress(addressSource.ip.getIPv4());
								tcp->setDestinationPort(addressSource.port);
								ipOrig->updateChecksum();
								ipHeader->setDestinationAddress(addressSource.ip.getIPv4());
								icmp->updateChecksum(sizeContent);
								ipHeader->updateChecksum();
								return sl_true;
							}
						} else if (protocolOrig == InternetProtocol::UDP) {
							UdpDatagram* udp = (UdpDatagram*)(ipOrig->getContent());
							SocketAddress addressSource;
							if (m_mappingUdp.mapToInternalAddress(udp->getDestinationPort(), addressSource)) {
								ipOrig->setDestinationAddress(addressSource.ip.getIPv4());
								udp->setDestinationPort(addressSource.port);
								udp->setChecksum(0);
								ipOrig->updateChecksum();
								ipHeader->setDestinationAddress(addressSource.ip.getIPv4());
								icmp->updateChecksum(sizeContent);
								ipHeader->updateChecksum();
								return sl_true;
							}
						}
					}
				}
			}
		}
		return sl_false;
	}

	sl_uint16 NatTable::getMappedIcmpEchoSequenceNumber(const IcmpEchoAddress& address)
	{
		IcmpEchoElement element;
		if (m_mapIcmpEchoOutgoing.get(address, &element)) {
			return element.sequenceNumberTarget;
		}
		sl_uint16 sn = ++ m_icmpEchoSequenceCurrent;
		if (m_mapIcmpEchoIncoming.get(sn, &element)) {
			m_mapIcmpEchoOutgoing.removeItems(element.addressSource);
		}
		element.addressSource = address;
		element.sequenceNumberTarget = sn;
		m_mapIcmpEchoOutgoing.put(address, element);
		m_mapIcmpEchoIncoming.put(sn, element);
		return sn;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(NatTablePort)

	NatTablePort::NatTablePort()
	{
		flagActive = sl_false;
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
		NewHelper<NatTablePort>::free(m_ports, m_nPorts);
	}

	void NatTableMapping::setup(sl_uint16 portBegin, sl_uint16 portEnd)
	{
		ObjectLocker lock(this);

		m_mapPorts.removeAll_NoLock();

		if (m_ports) {
			NewHelper<NatTablePort>::free(m_ports, m_nPorts);
			m_ports = sl_null;
		}
		m_pos = 0;
		m_nPorts = 0;

		m_portBegin = portBegin;
		m_portEnd = portEnd;
		if (portEnd >= portBegin) {
			m_nPorts = portEnd - portBegin + 1;
			m_ports = NewHelper<NatTablePort>::create(m_nPorts);
		}
	}

	sl_bool NatTableMapping::mapToExternalPort(const SocketAddress& address, sl_uint16& _port)
	{
		ObjectLocker lock(this);

		if (!m_ports) {
			return sl_false;
		}
		sl_uint32 n = m_nPorts;
		if (!n) {
			return sl_false;
		}

		sl_uint16 port;
		if (m_mapPorts.get_NoLock(address, &port)) {
			if (port >= m_portBegin && port <= m_portEnd) {
				m_ports[port - m_portBegin].timeLastAccess = Time::now();
				_port = port;
				return sl_true;
			} else {
				m_mapPorts.remove_NoLock(address);
			}
		}

		sl_uint16 pos = m_pos;
		n *= 2;
		sl_int64 timeAccessMin = 0;
		sl_int64 timeAccessMax = 0;
		for (sl_uint32 i = 0; i < n; i++) {
			if (!(m_ports[pos].flagActive)) {
				port = pos + m_portBegin;
				m_ports[pos].flagActive = sl_true;
				m_ports[pos].addressSource = address;
				m_ports[pos].timeLastAccess = Time::now();
				m_mapPorts.put_NoLock(address, port);
				_port = port;
				m_pos = (pos + 1) % m_nPorts;
				return sl_true;
			} else {
				sl_int64 t = m_ports[pos].timeLastAccess.toInt();
				if (t > timeAccessMax) {
					timeAccessMax = t;
				}
				if (timeAccessMin == 0 || t < timeAccessMin) {
					timeAccessMin = t;
				}
			}
			pos = (pos + 1) % m_nPorts;
			if (i == m_nPorts) {
				sl_int64 mid = (timeAccessMin + timeAccessMax) / 2;
				for (sl_uint16 k = 0; k < m_nPorts; k++) {
					if (m_ports[k].flagActive) {
						if (m_ports[k].timeLastAccess.toInt() <= mid) {
							m_ports[k].flagActive = sl_false;
							m_mapPorts.remove_NoLock(m_ports[k].addressSource);
						}
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool NatTableMapping::mapToInternalAddress(sl_uint16 port, SocketAddress& address)
	{
		ObjectLocker lock(this);
		if (!m_ports) {
			return sl_false;
		}
		if (!m_nPorts) {
			return sl_false;
		}
		if (port >= m_portBegin && port <= m_portEnd) {
			sl_uint32 k = port - m_portBegin;
			if (m_ports[k].flagActive) {
				m_ports[k].timeLastAccess = Time::now();
				address = m_ports[k].addressSource;
				return sl_true;
			}
		}
		return sl_false;
	}

}
