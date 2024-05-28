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

#include "slib/network/arp.h"

#include "slib/network/ethernet.h"
#include "slib/core/mio.h"
#include "slib/core/base.h"

namespace slib
{

	NetworkHardwareType ArpPacket::getHardwareType() const
	{
		return (NetworkHardwareType)(MIO::readUint16BE(_hardwareType));
	}

	void ArpPacket::setHardwareType(NetworkHardwareType hardwareType)
	{
		MIO::writeUint16BE(_hardwareType, (sl_uint16)hardwareType);
	}

	EtherType ArpPacket::getProtocolType() const
	{
		return (EtherType)(MIO::readUint16BE(_protocolType));
	}

	void ArpPacket::setProtocolType(EtherType protocolType)
	{
		MIO::writeUint16BE(_protocolType, (sl_uint32)protocolType);
	}

	sl_uint8 ArpPacket::getHardwareAddressLength() const
	{
		return _hardwareAddressLength;
	}

	void ArpPacket::setHardwareAddressLength(sl_uint8 length)
	{
		_hardwareAddressLength = length;
	}

	sl_uint8 ArpPacket::getProtocolAddressLength() const
	{
		return _protocolAddressLength;
	}

	void ArpPacket::setProtocolAddressLength(sl_uint8 length)
	{
		_protocolAddressLength = length;
	}

	ArpOperation ArpPacket::getOperation() const
	{
		return (ArpOperation)(MIO::readUint16BE(_operation));
	}

	void ArpPacket::setOperation(ArpOperation operation)
	{
		MIO::writeUint16BE(_operation, (sl_uint32)operation);
	}

	const sl_uint8* ArpPacket::getSenderHardwareAddress() const
	{
		return ((const sl_uint8*)this) + 8;
	}

	sl_uint8* ArpPacket::getSenderHardwareAddress()
	{
		return ((sl_uint8*)this) + 8;
	}

	MacAddress ArpPacket::getSenderMacAddress() const
	{
		return MacAddress(((sl_uint8*)this) + 8);
	}

	void ArpPacket::setSenderMacAddress(const MacAddress& address)
	{
		Base::copyMemory(((sl_uint8*)this) + 8, address.m, 6);
	}

	const sl_uint8* ArpPacket::getSenderProtocolAddress() const
	{
		return ((const sl_uint8*)this) + 8 + (sl_uint32)(getHardwareAddressLength());
	}

	sl_uint8* ArpPacket::getSenderProtocolAddress()
	{
		return ((sl_uint8*)this) + 8 + (sl_uint32)(getHardwareAddressLength());
	}

	IPv4Address ArpPacket::getSenderIPv4Address() const
	{
		return IPv4Address(((sl_uint8*)this) + 14);
	}

	void ArpPacket::setSenderIPv4Address(const IPv4Address& address)
	{
		address.getBytes(((sl_uint8*)this) + 14);
	}

	const sl_uint8* ArpPacket::getTargetHardwareAddress() const
	{
		return ((const sl_uint8*)this) + 8 + (sl_uint32)(getHardwareAddressLength()) + (sl_uint32)(getProtocolAddressLength());
	}

	sl_uint8* ArpPacket::getTargetHardwareAddress()
	{
		return ((sl_uint8*)this) + 8 + (sl_uint32)(getHardwareAddressLength()) + (sl_uint32)(getProtocolAddressLength());
	}

	MacAddress ArpPacket::getTargetMacAddress() const
	{
		return MacAddress(((sl_uint8*)this) + 18);
	}

	void ArpPacket::setTargetMacAddress(const MacAddress& address)
	{
		Base::copyMemory(((sl_uint8*)this) + 18, address.m, 6);
	}

	const sl_uint8* ArpPacket::getTargetProtocolAddress() const
	{
		return ((const sl_uint8*)this) + 8 + (sl_uint32)(getHardwareAddressLength()) * 2 + (sl_uint32)(getProtocolAddressLength());
	}

	sl_uint8* ArpPacket::getTargetProtocolAddress()
	{
		return ((sl_uint8*)this) + 8 + (sl_uint32)(getHardwareAddressLength()) * 2 + (sl_uint32)(getProtocolAddressLength());
	}

	IPv4Address ArpPacket::getTargetIPv4Address() const
	{
		return IPv4Address(((sl_uint8*)this) + 24);
	}

	void ArpPacket::setTargetIPv4Address(const IPv4Address& address)
	{
		address.getBytes(((sl_uint8*)this) + 24);
	}

	sl_bool ArpPacket::isValidEthernetIPv4() const
	{
		return getHardwareType() == NetworkHardwareType::Ethernet && getProtocolType() == EtherType::IPv4 && getHardwareAddressLength() == 6 && getProtocolAddressLength() == 4;
	}

	void ArpPacket::setEthernetAddresses(const MacAddress& sender, const MacAddress& target)
	{
		setHardwareType(NetworkHardwareType::Ethernet);
		setHardwareAddressLength(6);
		setSenderMacAddress(sender);
		setTargetMacAddress(target);
	}

	void ArpPacket::setIPv4Addresses(const IPv4Address& sender, const IPv4Address& target)
	{
		setProtocolType(EtherType::IPv4);
		setProtocolAddressLength(4);
		setSenderIPv4Address(sender);
		setTargetIPv4Address(target);
	}

	void ArpPacket::buildEthernetIPv4RequestFrame(void* _frame, const IPv4Address& senderIp, const MacAddress senderMac, const IPv4Address& targetIp)
	{
		EthernetFrame& frame = *((EthernetFrame*)_frame);
		frame.setType(EtherType::ARP);
		frame.setSourceAddress(senderMac);
		frame.setDestinationAddress(MacAddress::getBroadcast());
		ArpPacket& arp = *((ArpPacket*)(frame.getContent()));
		arp.setProtocolType(EtherType::IPv4);
		arp.setHardwareType(NetworkHardwareType::Ethernet);
		arp.setHardwareAddressLength(6);
		arp.setProtocolAddressLength(4);
		arp.setOperation(ArpOperation::Request);
		arp.setSenderMacAddress(senderMac);
		arp.setSenderIPv4Address(senderIp);
		arp.setTargetMacAddress(MacAddress::zero());
		arp.setTargetIPv4Address(targetIp);
	}

	void ArpPacket::buildEthernetIPv4ReplyFrame(void* _frame, const IPv4Address& senderIp, const MacAddress senderMac, const IPv4Address& targetIp, const MacAddress& targetMac)
	{
		EthernetFrame& frame = *((EthernetFrame*)_frame);
		frame.setType(EtherType::ARP);
		frame.setSourceAddress(senderMac);
		frame.setDestinationAddress(targetMac);
		ArpPacket& arp = *((ArpPacket*)(frame.getContent()));
		arp.setProtocolType(EtherType::IPv4);
		arp.setHardwareType(NetworkHardwareType::Ethernet);
		arp.setHardwareAddressLength(6);
		arp.setProtocolAddressLength(4);
		arp.setOperation(ArpOperation::Reply);
		arp.setSenderMacAddress(senderMac);
		arp.setSenderIPv4Address(senderIp);
		arp.setTargetMacAddress(targetMac);
		arp.setTargetIPv4Address(targetIp);
	}

}
