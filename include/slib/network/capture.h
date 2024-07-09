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

#ifndef CHECKHEADER_SLIB_NETWORK_CAPTURE
#define CHECKHEADER_SLIB_NETWORK_CAPTURE

#include "constants.h"
#include "mac_address.h"

#include "../core/object.h"
#include "../core/time.h"
#include "../core/function.h"
#include "../core/string.h"

namespace slib
{

	enum class NetworkCaptureType
	{
		Null = 0, //  BSD loopback encapsulation; the link layer header is a 4-byte field, in host byte order, containing a PF_ value from socket.h for the network-layer protocol of the packet.
		Ethernet = 1, // 6 bytes destination mac, 6 bytes source mac, 2 bytes Ethertype
		PPP = 9, // Point-to-Point Protocol Datalink layer
		IEEE802_11 = 105, // IEEE 802.11 wireless LAN
		Linux = 113, //  Linux "cooked" capture encapsulation. (for "any" or PPP devices)

		Raw = 0x8001, // Raw IP; the packet begins with an IPv4 or IPv6 header, with the "version" field of the header indicating whether it's an IPv4 or IPv6 header.

		Any = 0xffff

	};

	class SLIB_EXPORT NetCapturePacket
	{
	public:
		sl_uint8* data;
		sl_uint32 length;
		Time time;

	public:
		NetCapturePacket();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(NetCapturePacket)

	};

	class NetCapture;

	class SLIB_EXPORT NetCaptureParam
	{
	public:
		StringParam deviceName; // <null> or <empty string> for any devices

		sl_bool flagPromiscuous; // ignored for "any devices" mode
		NetworkCaptureType preferedType; // used in Packet Socket mode. now supported Ethernet and Raw

		sl_bool flagAutoStart; // default: true

		Function<void(NetCapture*, NetCapturePacket&)> onCapturePacket;
		Function<void(NetCapture*)> onError;

	public:
		NetCaptureParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(NetCaptureParam)

	};

	class SLIB_EXPORT NetCapture : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		NetCapture();

		~NetCapture();

	public:
		// linux packet datagram socket
		static Ref<NetCapture> createRawPacket(const NetCaptureParam& param);

		// raw socket
		static Ref<NetCapture> createRawIPv4(const NetCaptureParam& param);

	public:
		virtual void release() = 0;

		virtual void start() = 0;

		virtual sl_bool isRunning() = 0;

		virtual NetworkCaptureType getType() = 0;

		virtual sl_bool setType(NetworkCaptureType type);

		// send a L2-packet
		virtual sl_bool sendPacket(const void* data, sl_uint32 size) = 0;

		virtual String getErrorMessage();

	public:
		const String& getDeviceName();

		const MacAddress& getDeviceAddress();

		const IPv4Address& getIPv4Address();

		sl_uint32 getInterfaceIndex();

	protected:
		void _initWithParam(const NetCaptureParam& param);

		void _onCapturePacket(NetCapturePacket& packet);

		void _onError();

	protected:
		String m_deviceName;

		sl_uint64 m_timeDeviceAddress;
		MacAddress m_deviceAddress;
		sl_uint64 m_timeIP;
		IPv4Address m_ip;
		sl_uint32 m_index;

		Function<void(NetCapture*, NetCapturePacket&)> m_onCapturePacket;
		Function<void(NetCapture*)> m_onError;

	};

	enum class LinuxCookedPacketType
	{
		Host = 0,
		Broadcast = 1,
		Multicast = 2,
		OtherHost = 3,
		OutGoing = 4
	};

	class SLIB_EXPORT LinuxCookedFrame
	{
	public:
		enum
		{
			HeaderSize = 16
		};

	public:
		LinuxCookedPacketType getPacketType() const;

		void setPacketType(LinuxCookedPacketType type);

		NetworkCaptureType getDeviceType() const;

		void setDeviceType(NetworkCaptureType type);

		sl_uint16 getAddressLength() const;

		void setAddressLength(sl_uint16 len);

		const sl_uint8* getAddress() const;

		sl_uint8* getAddress();

		EtherType getProtocolType() const;

		void setProtocolType(EtherType type);

		const sl_uint8* getContent() const;

		sl_uint8* getContent();

	private:
		sl_uint8 m_packetType[2];
		sl_uint8 m_deviceType[2];
		sl_uint8 m_lenAddress[2];
		sl_uint8 m_address[8];
		sl_uint8 m_protocol[2];

	};

}

#endif
