/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/network/firewall.h"

#include "slib/system/system.h"
#include "slib/system/process.h"
#include "slib/io/file.h"
#include "slib/data/json.h"
#include "slib/network/socket_address.h"
#include "slib/network/tcpip.h"

namespace slib
{

	void Firewall::allowApplication(const StringParam& path)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		String name = File::getFileName(path);
		ProcessParam param;
		param.executable = System::getSystemDirectory() + "\\netsh.exe";
		param.argumentString = String::format("advfirewall firewall add rule name=\"%s\" dir=in action=allow program=\"%s\"", name, path);
		param.flags = ProcessFlags::HideWindow;
		if (Process::isCurrentProcessAdmin()) {
			Process::run(param);
		} else {
			Process::runAsAdmin(param);
		}
#endif
	}

	void Firewall::disallowApplication(const StringParam& path)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		String name = File::getFileName(path);
		ProcessParam param;
		param.executable = System::getSystemDirectory() + "\\netsh.exe";
		param.argumentString = String::format("advfirewall firewall delete rule name=\"%s\" program=\"%s\"", name, path);
		param.flags = ProcessFlags::HideWindow;
		if (Process::isCurrentProcessAdmin()) {
			Process::run(param);
		} else {
			Process::runAsAdmin(param);
		}
#endif
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FirewallAddressRule)

	FirewallAddressRule::FirewallAddressRule(): flagNotMac(sl_false), flagNotIp(sl_false), flagNotPort(sl_false)
	{
	}

	Json FirewallAddressRule::toJson() const
	{
		Json ret;
		if (mac.isNotEmpty()) {
			StringBuffer sb;
			if (flagNotMac) {
				sb.addStatic("!");
			}
			sl_bool flagFirst = sl_true;
			for (auto&& item : mac) {
				if (flagFirst) {
					flagFirst = sl_false;
				} else {
					sb.addStatic(",");
				}
				sb.add(item.toString());
			}
			ret.putItem("mac", sb.merge());
		}
		if (ip.isNotNull()) {
			StringBuffer sb;
			if (flagNotIp) {
				sb.addStatic("!");
			}
			sl_bool flagFirst = sl_true;
			for (auto&& item : ip) {
				if (flagFirst) {
					flagFirst = sl_false;
				} else {
					sb.addStatic(",");
				}
				sb.add(item.first.toString());
				if (item.second.isNotZero()) {
					sb.addStatic("-");
					sb.add(item.second.toString());
				}
			}
			ret.putItem("ip", sb.merge());
		}
		if (port.isNotNull()) {
			StringBuffer sb;
			if (flagNotPort) {
				sb.addStatic("!");
			}
			sl_bool flagFirst = sl_true;
			for (auto&& item : port) {
				if (flagFirst) {
					flagFirst = sl_false;
				} else {
					sb.addStatic(",");
				}
				sb.add(String::fromUint32(item.first));
				if (item.second) {
					sb.addStatic("-");
					sb.add(String::fromUint32(item.second));
				}
			}
			ret.putItem("port", sb.merge());
		}
		return ret;
	}

	void FirewallAddressRule::setJson(const Json& json)
	{
		if (json.isUndefined()) {
			return;
		}

		Json _mac = json.getItem("mac");
		if (_mac.isNotUndefined()) {
			mac.setNull();
			flagNotMac = sl_false;
			String strMac = _mac.getString();
			if (strMac.startsWith('!')) {
				flagNotMac = sl_true;
				strMac = strMac.substring(1);
			}
			for (auto&& item : strMac.split(",")) {
				mac.put_NoLock(MacAddress(item.trim()));
			}
		}
		Json _ip = json.getItem("ip");
		if (_ip.isNotUndefined()) {
			ip.setNull();
			flagNotIp = sl_false;
			String strIp = _ip.getString();
			if (strIp.startsWith('!')) {
				flagNotIp = sl_true;
				strIp = strIp.substring(1);
			}
			for (auto&& item : strIp.split(",")) {
				IPv4Address start, end;
				if (IPv4Address::parseRange(item, &start, &end)) {
					if (start == end) {
						end.setZero();
					}
					ip.add_NoLock(start, end);
				}
			}
		}
		Json _port = json.getItem("port");
		if (_port.isNotUndefined()) {
			port.setNull();
			flagNotPort = sl_false;
			String strPort = _port.getString();
			if (strPort.startsWith('!')) {
				flagNotPort = sl_true;
				strPort = strPort.substring(1);
			}
			for (auto&& item : strPort.split(",")) {
				sl_uint16 start, end;
				if (SocketAddress::parsePortRange(item, &start, &end)) {
					if (start == end) {
						end = 0;
					}
					port.add_NoLock(start, end);
				}
			}
		}
	}

	sl_bool FirewallAddressRule::matchMac(const MacAddress& _mac) const
	{
		if (mac.find_NoLock(_mac)) {
			return !flagNotMac;
		} else {
			return flagNotMac;
		}
	}

	sl_bool FirewallAddressRule::matchIP(const IPv4Address& _ip) const
	{
		ListElements< Pair<IPv4Address, IPv4Address> > list(ip);
		for (sl_size i = 0; i < list.count; i++) {
			Pair<IPv4Address, IPv4Address>& item = list[i];
			if (item.second.isNotZero()) {
				if (_ip >= item.first && _ip <= item.second) {
					return !flagNotIp;
				}
			} else {
				if (_ip == item.first) {
					return !flagNotIp;
				}
			}
		}
		return flagNotIp;
	}

	sl_bool FirewallAddressRule::matchPort(sl_uint16 _port) const
	{
		ListElements< Pair<sl_uint16, sl_uint16> > list(port);
		for (sl_size i = 0; i < list.count; i++) {
			Pair<sl_uint16, sl_uint16>& item = list[i];
			if (item.second) {
				if (_port >= item.first && _port <= item.second) {
					return !flagNotPort;
				}
			} else {
				if (_port == item.first) {
					return !flagNotPort;
				}
			}
		}
		return flagNotPort;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FirewallRule)

	FirewallRule::FirewallRule()
	{
		action = FirewallAction::Unknown;
		protocol = InternetProtocol::Unknown;
	}

	Json FirewallRule::toJson() const
	{
		Json ret;
		switch (action) {
		case FirewallAction::Accept:
			ret.putItem("action", "accept");
			break;
		case FirewallAction::Drop:
			ret.putItem("action", "drop");
			break;
		default:
			return sl_null;
		}
		switch (protocol) {
		case InternetProtocol::TCP:
			ret.putItem("protocol", "tcp");
			break;
		case InternetProtocol::UDP:
			ret.putItem("protocol", "udp");
			break;
		case InternetProtocol::RDP:
			ret.putItem("protocol", "rdp");
			break;
		case InternetProtocol::ICMP:
			ret.putItem("protocol", "icmp");
			break;
		case InternetProtocol::IGMP:
			ret.putItem("protocol", "igmp");
			break;
		default:
			return sl_null;
		}
		ret.putItem("source", Json(source));
		ret.putItem("target", Json(target));
		return ret;
	}

	void FirewallRule::setJson(const Json& json)
	{
		if (json.isUndefined()) {
			return;
		}
		Json _action = json.getItem("action");
		if (_action.isNotUndefined()) {
			StringData strAction(_action.getStringParam());
			if (strAction == "accept") {
				action = FirewallAction::Accept;
			} else if (strAction == "drop") {
				action = FirewallAction::Drop;
			} else {
				action = FirewallAction::Unknown;
			}
		}
		Json _protocol = json.getItem("protocol");
		if (_protocol.isNotUndefined()) {
			StringData strProtocol(_protocol.getStringParam());
			if (strProtocol == "tcp") {
				protocol = InternetProtocol::TCP;
			} else if (strProtocol == "udp") {
				protocol = InternetProtocol::UDP;
			} else if (strProtocol == "rdp") {
				protocol = InternetProtocol::RDP;
			} else if (strProtocol == "icmp") {
				protocol = InternetProtocol::ICMP;
			} else if (strProtocol == "igmp") {
				protocol = InternetProtocol::IGMP;
			} else {
				action = FirewallAction::Unknown;
				protocol = InternetProtocol::Unknown;
			}
		}
		source.setJson(json.getItem("source"));
		target.setJson(json.getItem("target"));
	}

	sl_bool FirewallRule::matchIPv4Packet(const MacAddress& macSource, const MacAddress& macTarget, const void* _packet, sl_size sizePacket)
	{
		if (sizePacket <= IPv4Packet::HeaderSizeBeforeOptions) {
			return sl_false;
		}
		IPv4Packet* packet = (IPv4Packet*)_packet;
		InternetProtocol _protocol = packet->getProtocol();
		if (protocol != InternetProtocol::Unknown) {
			if (_protocol != protocol) {
				return sl_false;
			}
		}
		if (source.mac.isNotNull()) {
			if (!(source.matchMac(macSource))) {
				return sl_false;
			}
		}
		if (target.mac.isNotNull()) {
			if (!(target.matchMac(macTarget))) {
				return sl_false;
			}
		}
		if (source.ip.isNotNull() || target.ip.isNotNull() || source.port.isNotNull() || target.port.isNotNull()) {
			if (!(IPv4Packet::check(packet, sizePacket))) {
				return sl_false;
			}
			if (source.ip.isNotNull()) {
				if (!(source.matchIP(packet->getSourceAddress()))) {
					return sl_false;
				}
			}
			if (target.ip.isNotNull()) {
				if (!(target.matchIP(packet->getDestinationAddress()))) {
					return sl_false;
				}
			}
			if (source.port.isNotNull() || target.port.isNotNull()) {
				sl_size sizeSegment = sizePacket - packet->getHeaderSize();
				if (_protocol == InternetProtocol::TCP) {
					TcpSegment* segment = (TcpSegment*)(packet->getContent());
					if (sizeSegment < TcpSegment::HeaderSizeBeforeOptions) {
						return sl_false;
					}
					if (source.port.isNotNull()) {
						if (!(source.matchPort(segment->getSourcePort()))) {
							return sl_false;
						}
					}
					if (target.port.isNotNull()) {
						if (!(target.matchPort(segment->getDestinationPort()))) {
							return sl_false;
						}
					}
				} else if (_protocol == InternetProtocol::UDP) {
					UdpDatagram* datagram = (UdpDatagram*)(packet->getContent());
					if (sizeSegment < UdpDatagram::HeaderSize) {
						return sl_false;
					}
					if (source.port.isNotNull()) {
						if (!(source.matchPort(datagram->getSourcePort()))) {
							return sl_false;
						}
					}
					if (target.port.isNotNull()) {
						if (!(target.matchPort(datagram->getDestinationPort()))) {
							return sl_false;
						}
					}
				} else {
					return sl_false;
				}
			}
		}
		return sl_true;
	}

}
