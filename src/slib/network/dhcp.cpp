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

#include "slib/network/dhcp.h"

#include "slib/network/event.h"
#include "slib/io/memory_output.h"
#include "slib/core/scoped_buffer.h"
#include "slib/core/mio.h"
#include "slib/core/log.h"

namespace slib
{

	DhcpOpcode DhcpHeader::getOpcode() const
	{
		return (DhcpOpcode)_op;
	}

	void DhcpHeader::setOpcode(DhcpOpcode op)
	{
		_op = (sl_uint8)op;
	}

	NetworkHardwareType DhcpHeader::getHardwareType() const
	{
		return (NetworkHardwareType)_htype;
	}

	void DhcpHeader::setHardwareType(NetworkHardwareType type)
	{
		_htype = (sl_uint8)type;
	}

	sl_uint8 DhcpHeader::getHardwareAddressLength() const
	{
		return _hlen;
	}

	void DhcpHeader::setHardwareAddressLength(sl_uint8 len)
	{
		_hlen = len;
	}

	sl_uint8 DhcpHeader::getHops() const
	{
		return _hops;
	}

	void DhcpHeader::setHops(sl_uint8 hops)
	{
		_hops = hops;
	}

	sl_uint32 DhcpHeader::getXid() const
	{
		return MIO::readUint32BE(_xid);
	}

	void DhcpHeader::setXid(sl_uint32 xid)
	{
		MIO::writeUint32BE(_xid, xid);
	}

	sl_uint16 DhcpHeader::getElapsedSeconds() const
	{
		return MIO::readUint16BE(_secs);
	}

	void DhcpHeader::setElapsedSeconds(sl_uint32 secs)
	{
		MIO::writeUint16BE(_secs, secs);
	}

	sl_uint16 DhcpHeader::getFlags() const
	{
		return MIO::readUint16BE(_flags);
	}

	void DhcpHeader::setFlags(sl_uint16 flags)
	{
		MIO::writeUint16BE(_flags, flags);
	}

	IPv4Address DhcpHeader::getClientIP()
	{
		return IPv4Address(_ciaddr);
	}

	void DhcpHeader::setClientIP(const IPv4Address& ip)
	{
		ip.getBytes(_ciaddr);
	}

	IPv4Address DhcpHeader::getYourIP()
	{
		return IPv4Address(_yiaddr);
	}

	void DhcpHeader::setYourIP(const IPv4Address& ip)
	{
		ip.getBytes(_yiaddr);
	}

	IPv4Address DhcpHeader::getNextServer()
	{
		return IPv4Address(_siaddr);
	}

	void DhcpHeader::setNextServer(const IPv4Address& ip)
	{
		ip.getBytes(_siaddr);
	}

	IPv4Address DhcpHeader::getRelayAgent()
	{
		return IPv4Address(_giaddr);
	}

	void DhcpHeader::setRelayAgent(const IPv4Address& ip)
	{
		ip.getBytes(_giaddr);
	}

	MacAddress DhcpHeader::getClientMacAddress()
	{
		return MacAddress(_chaddr);
	}

	void DhcpHeader::setClientMacAddress(const MacAddress& address)
	{
		address.getBytes(_chaddr);
	}

	sl_bool DhcpHeader::isValidMagicCookie()
	{
		return getMagicCookie() == SLIB_NETWORK_DHCP_MAGIC_COOKIE;
	}

	sl_uint32 DhcpHeader::getMagicCookie()
	{
		return MIO::readUint32BE(_magicCookie);
	}

	void DhcpHeader::setMagicCookie(sl_uint32 value)
	{
		MIO::writeUint32BE(_magicCookie, value);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(DhcpBindParam)

	DhcpBindParam::DhcpBindParam()
	{
		type = DhcpMessageType::None;
		leaseTime = 43200; // 12 Hours
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(DhcpServerParam)

	DhcpServerParam::DhcpServerParam()
	{
		port = SLIB_NETWORK_DHCP_SERVER_PORT;
		flagAutoStart = sl_true;
	}


	SLIB_DEFINE_OBJECT(DhcpServer, Object)

	DhcpServer::DhcpServer()
	{
		m_flagInit = sl_false;
		m_flagRunning = sl_false;
	}

	DhcpServer::~DhcpServer()
	{
		release();
	}

#define TAG_SERVER "DhcpServer"

	Ref<DhcpServer> DhcpServer::create(const DhcpServerParam& param)
	{
		Ref<DhcpServer> ret = new DhcpServer;

		if (ret.isNotNull()) {

			AsyncUdpSocketParam up;
			up.ioLoop = param.ioLoop;
			up.bindDevice = param.bindDevice;
			up.bindAddress.port = param.port;
			up.onReceiveFrom = SLIB_FUNCTION_WEAKREF(ret, _onReceiveFrom);
			up.packetSize = 4096;
			up.flagBroadcast = sl_true;
			up.flagAutoStart = sl_false;

			Ref<AsyncUdpSocket> socket = AsyncUdpSocket::create(up);
			if (socket.isNull()) {
				LogError(TAG_SERVER, "Failed to bind to port %d", param.port);
				return sl_null;
			}

			ret->m_socket = Move(socket);
			ret->m_onBind = param.onBind;
			ret->m_flagInit = sl_true;
			if (param.flagAutoStart) {
				ret->start();
			}
			return ret;
		}
		return sl_null;
	}

	void DhcpServer::release()
	{
		ObjectLocker lock(this);
		if (!m_flagInit) {
			return;
		}
		m_flagInit = sl_false;

		m_flagRunning = sl_false;
		if (m_socket.isNotNull()) {
			m_socket->close();
		}
	}

	void DhcpServer::start()
	{
		ObjectLocker lock(this);
		if (!m_flagInit) {
			return;
		}
		if (m_flagRunning) {
			return;
		}
		if (m_socket.isNotNull()) {
			m_socket->start();
		}
		m_flagRunning = sl_true;
	}

	sl_bool DhcpServer::isRunning()
	{
		return m_flagRunning;
	}

	namespace {

		struct Option
		{
			DhcpOptionCode code;
			sl_uint8 len;
			sl_uint8* content;
		};

		class OptionReader
		{
		public:
			sl_uint8* current;
			sl_uint8* end;

			sl_bool flagEnd = sl_false;
			sl_bool flagError = sl_false;

		public:
			sl_bool read(Option& option)
			{
				if (flagEnd) {
					return sl_false;
				}
				if (current >= end) {
					_setError();
					return sl_false;
				}
				option.code = (DhcpOptionCode)(*(current++));
				option.content = sl_null;
				if (option.code == DhcpOptionCode::Pad) {
					option.len = 0;
					return sl_true;
				} else if (option.code == DhcpOptionCode::End) {
					option.len = 0;
					flagEnd = sl_true;
					return sl_true;
				} else {
					if (current >= end) {
						_setError();
						return sl_false;
					}
					option.len = *(current++);
					if (!(option.len)) {
						return sl_true;
					}
					option.content = current;
					sl_uint8* next = current + option.len;
					if (next > end) {
						_setError();
						return sl_false;
					}
					current = next;
					return sl_true;
				}
			}

		private:
			void _setError()
			{
				flagError = sl_true;
				flagEnd = sl_true;
			}

		};

		static sl_bool WriteOptionHeader(MemoryOutput& output, DhcpOptionCode code, sl_uint8 len)
		{
			if (!(output.writeUint8((sl_uint8)code))) {
				return sl_false;
			}
			if (!(output.writeUint8(len))) {
				return sl_false;
			}
			return sl_true;
		}

		static sl_bool WriteOption8(MemoryOutput& output, DhcpOptionCode code, sl_uint8 content)
		{
			if (!(WriteOptionHeader(output, code, 1))) {
				return sl_false;
			}
			if (!(output.write(&content, 1))) {
				return sl_false;
			}
			return sl_true;
		}

		static sl_bool WriteOption32(MemoryOutput& output, DhcpOptionCode code, sl_uint32 content)
		{
			if (!(WriteOptionHeader(output, code, 4))) {
				return sl_false;
			}
			sl_uint8 v[4];
			MIO::writeUint32BE(v, content);
			if (!(output.write(v, 4))) {
				return sl_false;
			}
			return sl_true;
		}

		static sl_bool WriteOption(MemoryOutput& output, DhcpOptionCode code, const Memory& mem)
		{
			sl_size size = mem.getSize();
			if (!size) {
				return sl_true;
			}
			if (size > 255) {
				size = 255;
			}
			if (!(WriteOptionHeader(output, code, (sl_uint8)size))) {
				return sl_false;
			}
			if (!(output.writeFully(mem.getData(), size))) {
				return sl_false;
			}
			return sl_true;
		}

		static sl_bool WriteOption(MemoryOutput& output, DhcpOptionCode code, const String& str)
		{
			sl_size len = str.getLength();
			if (!len) {
				return sl_true;
			}
			if (len > 255) {
				len = 255;
			}
			if (!(WriteOptionHeader(output, code, (sl_uint8)len))) {
				return sl_false;
			}
			if (!(output.writeFully(str.getData(), len))) {
				return sl_false;
			}
			return sl_true;
		}

		static sl_bool WriteOption(MemoryOutput& output, DhcpOptionCode code, const IPv4Address& ip)
		{
			if (ip.isZero()) {
				return sl_true;
			}
			if (!(WriteOptionHeader(output, code, 4))) {
				return sl_false;
			}
			sl_uint8 v[4];
			ip.getBytes(v);
			if (!(output.writeFully(v, 4))) {
				return sl_false;
			}
			return sl_true;
		}

		static sl_bool WriteOption(MemoryOutput& output, DhcpOptionCode code, const List<IPv4Address>& _list)
		{
			ListLocker<IPv4Address> list(_list);
			if (!(list.count)) {
				return sl_true;
			}
			if (list.count > 63) {
				list.count = 63;
			}
			if (!(WriteOptionHeader(output, code, (sl_uint8)(list.count << 2)))) {
				return sl_false;
			}
			for (sl_size i = 0; i < list.count; i++) {
				sl_uint8 v[4];
				list[i].getBytes(v);
				if (!(output.writeFully(v, 4))) {
					return sl_false;
				}
			}
			return sl_true;
		}

		static Memory BuildDomainSearch(const String& domainSearch)
		{
			if (domainSearch.isEmpty()) {
				return sl_null;
			}
			ListElements<String> items(domainSearch.split('.'));
			if (!(items.count)) {
				return sl_null;
			}
			MemoryOutput output;
			for (sl_size i = 0; i < items.count; i++) {
				String& item = items[i];
				sl_size len = item.getLength();
				if (len > 255) {
					len = 255;
				}
				if (!(output.writeUint8((sl_uint8)len))) {
					return sl_null;
				}
				if (!(output.writeFully(item.getData(), len))) {
					return sl_null;
				}
			}
			if (!(output.writeUint8(0))) {
				return sl_null;
			}
			return output.merge();
		}

		static Memory BuildBindPacket(DhcpBindParam& param, DhcpMessageType type, DhcpHeader& request, const IPv4Address& clientIp, sl_bool flagUseClientId)
		{
			MemoryOutput output;
			DhcpHeader header;
			Base::zeroMemory(&header, sizeof(header));
			header.setOpcode(DhcpOpcode::Reply);
			header.setHardwareType(NetworkHardwareType::Ethernet);
			header.setHardwareAddressLength(6);
			header.setClientIP(clientIp);
			header.setClientMacAddress(request.getClientMacAddress());
			header.setXid(request.getXid());
			header.setYourIP(param.ip);
			header.setNextServer(param.server);
			header.setMagicCookie();
			if (!(output.writeFully(&header, sizeof(header)))) {
				return sl_null;
			}
			if (!(WriteOption8(output, DhcpOptionCode::DhcpMessageType, (sl_uint8)type))) {
				return sl_null;
			}
			if (flagUseClientId) {
				if (!(WriteOptionHeader(output, DhcpOptionCode::ClientIdentifier, 7))) {
					return sl_null;
				}
				if (!(output.writeUint8(1))) {
					return sl_null;
				}
				sl_uint8 v[6];
				param.mac.getBytes(v);
				if (!(output.writeFully(v, 6))) {
					return sl_null;
				}
			}
			if (!(WriteOption(output, DhcpOptionCode::ServerIdentifier, param.server))) {
				return sl_null;
			}
			if (param.routers.isNotNull()) {
				if (!(WriteOption(output, DhcpOptionCode::Router, param.routers))) {
					return sl_null;
				}
			} else {
				if (!(WriteOption(output, DhcpOptionCode::Router, param.router))) {
					return sl_null;
				}
			}
			if (!(WriteOption(output, DhcpOptionCode::SubnetMask, param.subnetMask))) {
				return sl_null;
			}
			if (!(WriteOption(output, DhcpOptionCode::BroadcastAddress, param.broadcastAddress))) {
				return sl_null;
			}
			if (!(WriteOption(output, DhcpOptionCode::DomainName, param.domainName))) {
				return sl_null;
			}
			if (!(WriteOption(output, DhcpOptionCode::DomainSearch, BuildDomainSearch(param.searchDomain)))) {
				return sl_null;
			}
			if (!(WriteOption(output, DhcpOptionCode::DomainNameServer, param.domainServers))) {
				return sl_null;
			}
			if (param.leaseTime) {
				if (!(WriteOption32(output, DhcpOptionCode::IpAddressLeaseTime, param.leaseTime))) {
					return sl_null;
				}
			}
			if (!(output.writeUint8((sl_uint8)(DhcpOptionCode::End)))) {
				return sl_null;
			}
			return output.merge();
		}

	}

	void DhcpServer::_onReceiveFrom(AsyncUdpSocket* socket, const SocketAddress& addressFrom, void* _data, sl_uint32 size)
	{
		if (size < sizeof(DhcpHeader)) {
			return;
		}
		sl_uint8* data = (sl_uint8*)_data;
		DhcpHeader* header = (DhcpHeader*)data;
		if (header->getOpcode() == DhcpOpcode::Request && header->isValidMagicCookie() && header->getHardwareType() == NetworkHardwareType::Ethernet) {
			_processRequest(addressFrom, *header, data + sizeof(DhcpHeader), size - sizeof(DhcpHeader));
		}
	}

	void DhcpServer::_onBind(DhcpBindParam& param)
	{
		m_onBind(this, param);
	}

	void DhcpServer::_processRequest(const SocketAddress& addressFrom, DhcpHeader& header, sl_uint8* pOptions, sl_size sizeOptions)
	{
		MacAddress clientMac = header.getClientMacAddress();

		OptionReader options;
		options.current = pOptions;
		options.end = pOptions + sizeOptions;

		DhcpMessageType type = DhcpMessageType::None;
		sl_bool flagClientId = sl_false;

		IPv4Address preferredIp = header.getClientIP();

		Option option;
		while (options.read(option)) {
			if (option.code == DhcpOptionCode::DhcpMessageType) {
				if (option.len != 1) {
					return;
				}
				type = (DhcpMessageType)(*(option.content));
			} else if (option.code == DhcpOptionCode::ClientIdentifier) {
				if (option.len < 2) {
					return;
				}
				if (*(option.content) == 1) { // Ethernet
					if (option.len != 7) {
						return;
					}
					flagClientId = sl_true;
					clientMac.setBytes(option.content + 1);
				}
			} else if (option.code == DhcpOptionCode::RequestedIpAddress) {
				if (option.len != 4) {
					return;
				}
				preferredIp.setBytes(option.content);
			}
		}
		if (options.flagError) {
			return;
		}
		if (type == DhcpMessageType::Discover || type == DhcpMessageType::Request) {
			DhcpBindParam param;
			param.type = type;
			param.mac = clientMac;
			_onBind(param);
			if (param.ip.isNotZero()) {
				if (type == DhcpMessageType::Discover) {
					type = DhcpMessageType::Offer;
				} else {
					type = DhcpMessageType::Ack;
					if (preferredIp.isNotZero()) {
						if (preferredIp != param.ip) {
							type = DhcpMessageType::Nak;
						}
					}
				}
				Memory packet = BuildBindPacket(param, type, header, preferredIp, flagClientId);
				if (packet.isNotNull()) {
					if (addressFrom.ip.getIPv4().isNotZero()) {
						m_socket->sendTo(addressFrom, packet);
					} else {
						m_socket->sendTo(SocketAddress(IPv4Address::Broadcast, addressFrom.port), packet);
					}
				}
			}
		}
	}

}
