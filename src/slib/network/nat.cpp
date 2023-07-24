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

#include "slib/core/system.h"
#include "slib/core/new_helper.h"

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
		NetworkInternetProtocol protocol = ipHeader->getProtocol();
		if (protocol == NetworkInternetProtocol::TCP) {
			TcpSegment* tcp = (TcpSegment*)(ipContent);
			if (tcp->checkSize(sizeContent)) {
				sl_uint16 targetPort;
				if (m_mappingTcp.mapToExternalPort(SocketAddress(ipHeader->getSourceAddress(), tcp->getSourcePort()), targetPort)) {
					tcp->setSourcePort(targetPort);
					ipHeader->setSourceAddress(addressTarget);
					tcp->updateChecksum(ipHeader, sizeContent);
					ipHeader->updateChecksum();
					return sl_true;
				}
			}
		} else if (protocol == NetworkInternetProtocol::UDP) {
			UdpDatagram* udp = (UdpDatagram*)(ipContent);
			if (udp->checkSize(sizeContent)) {
				sl_uint16 targetPort;
				if (m_mappingUdp.mapToExternalPort(SocketAddress(ipHeader->getSourceAddress(), udp->getSourcePort()), targetPort)) {
					udp->setSourcePort(targetPort);
					ipHeader->setSourceAddress(addressTarget);
					udp->updateChecksum(ipHeader);
					ipHeader->updateChecksum();
					return sl_true;
				}
			}
		} else if (protocol == NetworkInternetProtocol::ICMP) {
			IcmpHeaderFormat* icmp = (IcmpHeaderFormat*)(ipContent);
			if (sizeContent >= sizeof(IcmpHeaderFormat)) {
				IcmpType type = icmp->getType();
				if (type == IcmpType::Echo) {
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
				} else if (type == IcmpType::DestinationUnreachable || type == IcmpType::TimeExceeded) {
					IPv4Packet* ipOrig = (IPv4Packet*)(icmp->getContent());
					sl_uint32 sizeOrig = sizeContent - sizeof(IcmpHeaderFormat);
					IPv4Address addressSrc = ipHeader->getSourceAddress();
					if (IPv4Packet::checkHeaderSize(ipOrig, sizeOrig) && sizeOrig >= ipOrig->getHeaderSize() + 8 && ipOrig->getDestinationAddress() == addressSrc) {
						NetworkInternetProtocol protocolOrig = ipOrig->getProtocol();
						if (protocolOrig == NetworkInternetProtocol::TCP) {
							TcpSegment* tcp = (TcpSegment*)(ipOrig->getContent());
							sl_uint16 targetPort;
							if (m_mappingTcp.mapToExternalPort(SocketAddress(addressSrc, tcp->getDestinationPort()), targetPort)) {
								ipOrig->setDestinationAddress(addressTarget);
								tcp->setDestinationPort(targetPort);
								tcp->setChecksum(0);
								ipOrig->updateChecksum();
								ipHeader->setSourceAddress(addressTarget);
								icmp->updateChecksum(sizeContent);
								ipHeader->updateChecksum();
								return sl_true;
							}
						} else if (protocolOrig == NetworkInternetProtocol::UDP) {
							UdpDatagram* udp = (UdpDatagram*)(ipOrig->getContent());
							sl_uint16 targetPort;
							if (m_mappingUdp.mapToExternalPort(SocketAddress(addressSrc, udp->getDestinationPort()), targetPort)) {
								ipOrig->setDestinationAddress(addressTarget);
								udp->setDestinationPort(targetPort);
								udp->setChecksum(0);
								ipOrig->updateChecksum();
								ipHeader->setSourceAddress(addressTarget);
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

	sl_bool NatTable::translateIncomingPacket(IPv4Packet* ipHeader, void* ipContent, sl_uint32 sizeContent)
	{
		IPv4Address addressTarget = m_param.targetAddress;
		if (addressTarget.isZero()) {
			return sl_false;
		}
		if (ipHeader->getDestinationAddress() != addressTarget) {
			return sl_false;
		}
		NetworkInternetProtocol protocol = ipHeader->getProtocol();
		if (protocol == NetworkInternetProtocol::TCP) {
			TcpSegment* tcp = (TcpSegment*)(ipContent);
			if (tcp->checkSize(sizeContent)) {
				SocketAddress addressSource;
				if (m_mappingTcp.mapToInternalAddress(tcp->getDestinationPort(), addressSource)) {
					ipHeader->setDestinationAddress(addressSource.ip.getIPv4());
					tcp->setDestinationPort(addressSource.port);
					tcp->updateChecksum(ipHeader, sizeContent);
					ipHeader->updateChecksum();
					return sl_true;
				}
			}
		} else if (protocol == NetworkInternetProtocol::UDP) {
			UdpDatagram* udp = (UdpDatagram*)(ipHeader->getContent());
			if (udp->checkSize(sizeContent)) {
				SocketAddress addressSource;
				if (m_mappingUdp.mapToInternalAddress(udp->getDestinationPort(), addressSource)) {
					ipHeader->setDestinationAddress(addressSource.ip.getIPv4());
					udp->setDestinationPort(addressSource.port);
					udp->updateChecksum(ipHeader);
					ipHeader->updateChecksum();
					return sl_true;
				}
			}
		} else if (protocol == NetworkInternetProtocol::ICMP) {
			IcmpHeaderFormat* icmp = (IcmpHeaderFormat*)(ipContent);
			if (sizeContent >= sizeof(IcmpHeaderFormat)) {
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
					if (IPv4Packet::checkHeaderSize(ipOrig, sizeOrig) && sizeOrig >= ipOrig->getHeaderSize() + 8 && ipOrig->getSourceAddress() == addressTarget) {
						NetworkInternetProtocol protocolOrig = ipOrig->getProtocol();
						if (protocolOrig == NetworkInternetProtocol::TCP) {
							TcpSegment* tcp = (TcpSegment*)(ipOrig->getContent());
							SocketAddress addressSource;
							if (m_mappingTcp.mapToInternalAddress(tcp->getSourcePort(), addressSource)) {
								ipOrig->setSourceAddress(addressSource.ip.getIPv4());
								tcp->setSourcePort(addressSource.port);
								tcp->setChecksum(0);
								ipOrig->updateChecksum();
								ipHeader->setDestinationAddress(addressSource.ip.getIPv4());
								icmp->updateChecksum(sizeContent);
								ipHeader->updateChecksum();
								return sl_true;
							}
						} else if (protocolOrig == NetworkInternetProtocol::UDP) {
							UdpDatagram* udp = (UdpDatagram*)(ipOrig->getContent());
							SocketAddress addressSource;
							if (m_mappingUdp.mapToInternalAddress(udp->getSourcePort(), addressSource)) {
								ipOrig->setSourceAddress(addressSource.ip.getIPv4());
								udp->setSourcePort(addressSource.port);
								udp->setChecksum(0);
								ipOrig->updateChecksum();
								ipHeader->setDestinationAddress(addressSource.ip.getIPv4());
								icmp->updateChecksum(sizeContent);
								ipHeader->updateChecksum();
								return sl_true;
							}
						} else if (protocolOrig == NetworkInternetProtocol::ICMP) {
							IcmpHeaderFormat* icmpOrig = (IcmpHeaderFormat*)(ipOrig->getContent());
							if (icmpOrig->getType() == IcmpType::Echo) {
								if (icmpOrig->getEchoIdentifier() == m_param.icmpEchoIdentifier) {
									IcmpEchoElement element;
									if (m_mapIcmpEchoIncoming.get(icmp->getEchoSequenceNumber(), &element)) {
										ipOrig->setSourceAddress(element.addressSource.ip);
										icmpOrig->setEchoIdentifier(element.addressSource.identifier);
										icmpOrig->setEchoSequenceNumber(element.addressSource.sequenceNumber);
										icmpOrig->setChecksum(0);
										ipOrig->updateChecksum();
										ipHeader->setDestinationAddress(element.addressSource.ip);
										icmp->updateChecksum(sizeContent);
										ipHeader->updateChecksum();
										return sl_true;
									}
								}
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
				m_ports[port - m_portBegin].timeLastAccess = System::getTickCount64();
				_port = port;
				return sl_true;
			} else {
				m_mapPorts.remove_NoLock(address);
			}
		}

		sl_uint16 pos = m_pos;
		n *= 2;
		sl_uint64 timeAccessMin = 0;
		sl_uint64 timeAccessMax = 0;
		for (sl_uint32 i = 0; i < n; i++) {
			if (!(m_ports[pos].flagActive)) {
				port = pos + m_portBegin;
				m_ports[pos].flagActive = sl_true;
				m_ports[pos].addressSource = address;
				m_ports[pos].timeLastAccess = System::getTickCount64();
				m_mapPorts.put_NoLock(address, port);
				_port = port;
				m_pos = (pos + 1) % m_nPorts;
				return sl_true;
			} else {
				sl_uint64 t = m_ports[pos].timeLastAccess;
				if (t > timeAccessMax) {
					timeAccessMax = t;
				}
				if (!timeAccessMin || t < timeAccessMin) {
					timeAccessMin = t;
				}
			}
			pos = (pos + 1) % m_nPorts;
			if (i == m_nPorts) {
				sl_uint64 mid = (timeAccessMin + timeAccessMax) / 2;
				for (sl_uint16 k = 0; k < m_nPorts; k++) {
					if (m_ports[k].flagActive) {
						if (m_ports[k].timeLastAccess <= mid) {
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
				m_ports[k].timeLastAccess = System::getTickCount64();
				address = m_ports[k].addressSource;
				return sl_true;
			}
		}
		return sl_false;
	}

}
