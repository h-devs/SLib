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

#include "slib/network/capture.h"

#include "slib/system/system.h"
#include "slib/core/thread_service.h"
#include "slib/core/mio.h"
#include "slib/core/log.h"
#include "slib/network/os.h"
#include "slib/network/socket.h"
#include "slib/network/event.h"
#include "slib/network/tcpip.h"
#include "slib/network/ethernet.h"

#define TAG "NetCapture"

#define MAX_PACKET_SIZE 65535

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(NetCapturePacket)

	NetCapturePacket::NetCapturePacket(): data(sl_null), length(0)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(NetCaptureParam)

	NetCaptureParam::NetCaptureParam()
	{
		preferedType = NetworkCaptureType::Ethernet;
		flagPromiscuous = sl_false;

		flagAutoStart = sl_true;
	}


	SLIB_DEFINE_OBJECT(NetCapture, Object)

	NetCapture::NetCapture()
	{
		m_timeDeviceAddress = 0;
		m_timeIP = 0;
		m_timeDisplayName = 0;
		m_timeIndex = 0;
		m_index = 0;
	}

	NetCapture::~NetCapture()
	{
	}

	sl_bool NetCapture::setType(NetworkCaptureType type)
	{
		return sl_false;
	}

	String NetCapture::getErrorMessage()
	{
		return sl_null;
	}

	const String& NetCapture::getDeviceName()
	{
		return m_deviceName;
	}

	namespace
	{
		static sl_bool GetDeviceInfo(const StringView& name, NetworkInterfaceInfo& info)
		{
#ifdef SLIB_PLATFORM_IS_WIN32
			// Remove prefix: \Device\NPF_
			sl_reg index = name.indexOf('{');
			if (index > 0) {
				return Network::findInterface(name.substring(index), &info);
			}
#endif
			return Network::findInterface(name, &info);
		}
	}

	const MacAddress& NetCapture::getDeviceAddress()
	{
		sl_uint64 now = System::getTickCount64();
		if (m_timeDeviceAddress && now >= m_timeDeviceAddress && now < m_timeDeviceAddress + 10000) {
			return m_deviceAddress;
		}
		NetworkInterfaceInfo info;
		if (GetDeviceInfo(m_deviceName, info)) {
			m_deviceAddress = info.macAddress;
		} else {
			m_deviceAddress.setZero();
		}
		m_timeDeviceAddress = now;
		return m_deviceAddress;
	}

	const IPv4Address& NetCapture::getIPv4Address()
	{
		sl_uint64 now = System::getTickCount64();
		if (m_timeIP && now >= m_timeIP && now < m_timeIP + 5000) {
			return m_ip;
		}
		NetworkInterfaceInfo info;
		if (GetDeviceInfo(m_deviceName, info)) {
			m_ip = info.addresses_IPv4.getValueAt_NoLock(0).address;
		} else {
			m_ip.setZero();
		}
		m_timeIP = now;
		return m_ip;
	}

	String NetCapture::getDisplayName()
	{
		sl_uint64 now = System::getTickCount64();
		if (m_timeDisplayName && now >= m_timeDisplayName && now < m_timeDisplayName + 5000) {
			return m_displayName;
		}
		String name;
		NetworkInterfaceInfo info;
		if (GetDeviceInfo(m_deviceName, info)) {
			name = info.displayName;
		}
		if (name.isEmpty()) {
			name = m_deviceName;
		}
		m_displayName = name;
		m_timeDisplayName = now;
		return name;
	}

	sl_uint32 NetCapture::getInterfaceIndex()
	{
		sl_uint64 now = System::getTickCount64();
		if (m_timeIndex && now >= m_timeIndex && now < m_timeIndex + 5000) {
			return m_index;
		}
		NetworkInterfaceInfo info;
		if (GetDeviceInfo(m_deviceName, info)) {
			m_index = info.index;
		} else {
			m_index = 0;
		}
		m_timeIndex = now;
		return m_index;
	}

	void NetCapture::_initWithParam(const NetCaptureParam& param)
	{
		m_deviceName = param.deviceName.toString();
		m_onCapturePacket = param.onCapturePacket;
		m_onError = param.onError;
	}

	void NetCapture::_onCapturePacket(NetCapturePacket& packet)
	{
		m_onCapturePacket(this, packet);
	}

	void NetCapture::_onError()
	{
		m_onError(this);
	}

	namespace
	{
		class RawPacketCapture : public NetCapture, public ThreadService
		{
		public:
			SocketAndEvent m_socketAndEvent;

			NetworkCaptureType m_deviceType = NetworkCaptureType::Ethernet;
			sl_uint32 m_ifaceIndex = 0;
			Memory m_bufPacket;

		public:
			RawPacketCapture()
			{
				m_serviceLock = getLocker();
				m_onRunService = SLIB_FUNCTION_MEMBER(this, _run);
				m_onReleaseService = SLIB_FUNCTION_MEMBER(this, _release);
			}

			~RawPacketCapture()
			{
				release();
			}

		public:
			static Ref<RawPacketCapture> create(const NetCaptureParam& param)
			{
				sl_uint32 iface = 0;
				StringCstr deviceName = param.deviceName;
				if (deviceName.isNotEmpty()) {
					iface = Network::getInterfaceIndexFromName(deviceName);
					if (iface == 0) {
						LogError(TAG, "Failed to find the interface index of device: %s", deviceName);
						return sl_null;
					}
				}
				Socket socket;
				NetworkCaptureType deviceType = param.preferedType;
				if (deviceType == NetworkCaptureType::Raw) {
					socket = Socket::openPacketDatagram(EtherType::All);
				} else {
					deviceType = NetworkCaptureType::Ethernet;
					socket = Socket::openPacketRaw(EtherType::All);
				}
				if (socket.isOpened()) {
					if (iface > 0) {
						if (param.flagPromiscuous) {
							if (!(socket.setPromiscuousMode(deviceName, sl_true))) {
								Log(TAG, "Failed to set promiscuous mode to the network device: %s", deviceName);
							}
						}
						if (!(socket.bindToDevice(deviceName))) {
							Log(TAG, "Failed to bind the network device: %s", deviceName);
						}
					}
					SocketAndEvent socketAndEvent;
					if (socketAndEvent.initialize(Move(socket), SocketEvent::Read)) {
						Memory mem = Memory::create(MAX_PACKET_SIZE);
						if (mem.isNotNull()) {
							Ref<RawPacketCapture> ret = new RawPacketCapture;
							if (ret.isNotNull()) {
								ret->_initWithParam(param);
								ret->m_bufPacket = Move(mem);
								ret->m_socketAndEvent = Move(socketAndEvent);
								ret->m_deviceType = deviceType;
								ret->m_ifaceIndex = iface;
								if (param.flagAutoStart) {
									ret->start();
								}
								return ret;
							}
						}
					} else {
						LogError(TAG, "Failed to create socket event");
					}
				} else {
					LogError(TAG, "Failed to create packet socket");
				}
				return sl_null;
			}

			void release() override
			{
				ThreadService::release();
			}

			void start() override
			{
				ThreadService::start();
			}

			sl_bool isRunning() override
			{
				return ThreadService::isRunning();
			}

			Function<void()> _release()
			{
				MoveT<SocketAndEvent> se = Move(m_socketAndEvent);
				return [se]() {
					se.get().free();
				};
			}

			void _run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}

				sl_uint8* buf = (sl_uint8*)(m_bufPacket.getData());
				sl_uint32 sizeBuf = (sl_uint32)(m_bufPacket.getSize());

				while (thread->isNotStopping()) {
					L2PacketInfo info;
					sl_int32 n = m_socketAndEvent.socket.receivePacket(buf, sizeBuf, info);
					if (n >= 0) {
						NetCapturePacket packet;
						packet.data = buf;
						packet.length = n;
						packet.time = 0;
						_onCapturePacket(packet);
					} else if (n == SLIB_IO_WOULD_BLOCK) {
						m_socketAndEvent.event->wait();
					} else {
						break;
					}
				}
			}

			NetworkCaptureType getType() override
			{
				return m_deviceType;
			}

			sl_bool sendPacket(const void* buf, sl_uint32 size) override
			{
				if (isReleased()) {
					return sl_false;
				}
				if (!m_ifaceIndex) {
					return sl_false;
				}
				L2PacketInfo info;
				info.type = L2PacketType::OutGoing;
				info.iface = m_ifaceIndex;
				if (m_deviceType == NetworkCaptureType::Ethernet) {
					EthernetFrame* frame = (EthernetFrame*)buf;
					if (size < EthernetFrame::HeaderSize) {
						return sl_false;
					}
					info.protocol = frame->getType();
					info.setMacAddress(frame->getDestinationAddress());
				} else {
					info.protocol = EtherType::IPv4;
					info.clearAddress();
				}
				sl_uint32 ret = m_socketAndEvent.socket.sendPacket(buf, size, info);
				return ret == size;
			}
		};
	}

	Ref<NetCapture> NetCapture::createRawPacket(const NetCaptureParam& param)
	{
		return RawPacketCapture::create(param);
	}

	namespace
	{
		class RawIPv4Capture : public NetCapture, public ThreadService
		{
		public:
			SocketAndEvent m_tcp;
			SocketAndEvent m_udp;
			SocketAndEvent m_icmp;

			Memory m_bufPacket;

		public:
			RawIPv4Capture()
			{
				m_serviceLock = getLocker();
				m_onRunService = SLIB_FUNCTION_MEMBER(this, _run);
				m_onReleaseService = SLIB_FUNCTION_MEMBER(this, _release);
			}

			~RawIPv4Capture()
			{
				release();
			}

		public:
			static Ref<RawIPv4Capture> create(const NetCaptureParam& param)
			{
				Socket socketTCP = Socket::openRaw(InternetProtocol::TCP);
				Socket socketUDP = Socket::openRaw(InternetProtocol::UDP);
				Socket socketICMP = Socket::openRaw(InternetProtocol::ICMP);
				if (socketTCP.isOpened() && socketUDP.isOpened() && socketICMP.isOpened()) {
					socketTCP.setIncludingHeader(sl_true);
					socketUDP.setIncludingHeader(sl_true);
					socketICMP.setIncludingHeader(sl_true);
					SocketAndEvent tcp, udp, icmp;
					if (tcp.initialize(Move(socketTCP), SocketEvent::Read) && udp.initialize(Move(socketUDP), SocketEvent::Read) && icmp.initialize(Move(socketICMP), SocketEvent::Read)) {
						Memory mem = Memory::create(MAX_PACKET_SIZE);
						if (mem.isNotNull()) {
							Ref<RawIPv4Capture> ret = new RawIPv4Capture;
							if (ret.isNotNull()) {
								ret->_initWithParam(param);
								ret->m_bufPacket = Move(mem);
								ret->m_tcp = Move(tcp);
								ret->m_udp = Move(udp);
								ret->m_icmp = Move(icmp);
								if (param.flagAutoStart) {
									ret->start();
								}
								return ret;
							}
						}
					}
				}
				return sl_null;
			}

			void release() override
			{
				ThreadService::release();
			}

			void start() override
			{
				ThreadService::start();
			}

			sl_bool isRunning() override
			{
				return ThreadService::isRunning();
			}

			Function<void()> _release()
			{
				MoveT<SocketAndEvent> tcp = Move(m_tcp);
				MoveT<SocketAndEvent> udp = Move(m_udp);
				MoveT<SocketAndEvent> icmp = Move(m_icmp);
				return [tcp, udp, icmp]() {
					tcp.get().free();
					udp.get().free();
					icmp.get().free();
				};
			}

			void _run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}

				SocketEvent* events[3];
				events[0] = m_tcp.event.get();
				events[1] = m_udp.event.get();
				events[2] = m_icmp.event.get();

				sl_uint8* buf = (sl_uint8*)(m_bufPacket.getData());
				sl_uint32 sizeBuf = (sl_uint32)(m_bufPacket.getSize());

				while (thread->isNotStopping()) {
					SocketAddress address;
					sl_int32 n = m_tcp.socket.receiveFrom(address, buf, sizeBuf);
					if (n >= 0) {
						NetCapturePacket packet;
						packet.data = buf;
						packet.length = n;
						packet.time = 0;
						_onCapturePacket(packet);
					} else if (n != SLIB_IO_WOULD_BLOCK) {
						break;
					} else {
						n = m_udp.socket.receiveFrom(address, buf, sizeBuf);
						if (n >= 0) {
							NetCapturePacket packet;
							packet.data = buf;
							packet.length = n;
							packet.time = 0;
							_onCapturePacket(packet);
						} else if (n != SLIB_IO_WOULD_BLOCK) {
							break;
						} else {
							n = m_icmp.socket.receiveFrom(address, buf, sizeBuf);
							if (n >= 0) {
								NetCapturePacket packet;
								packet.data = buf;
								packet.length = n;
								packet.time = 0;
								_onCapturePacket(packet);
							} else if (n != SLIB_IO_WOULD_BLOCK) {
								break;
							} else {
								SocketEvent::waitMultipleEvents(events, sl_null, 3);
							}
						}
					}
				}
			}

			NetworkCaptureType getType() override
			{
				return NetworkCaptureType::Raw;
			}

			sl_bool sendPacket(const void* buf, sl_uint32 size) override
			{
				if (isReleased()) {
					return sl_false;
				}
				SocketAddress address;
				if (IPv4Packet::checkHeaderSize(buf, size)) {
					IPv4Packet* ip = (IPv4Packet*)buf;
					address.ip = ip->getDestinationAddress();
					address.port = 0;
					Socket* socket;
					InternetProtocol protocol = ip->getProtocol();
					if (protocol == InternetProtocol::TCP) {
						socket = &(m_tcp.socket);
					} else if (protocol == InternetProtocol::UDP) {
						socket = &(m_udp.socket);
					} else if (protocol == InternetProtocol::ICMP) {
						socket = &(m_icmp.socket);
					} else {
						return sl_false;
					}
					sl_uint32 ret = socket->sendTo(address, buf, size);
					if (ret == size) {
						return sl_true;
					}
				}
				return sl_false;
			}
		};
	}

	Ref<NetCapture> NetCapture::createRawIPv4(const NetCaptureParam& param)
	{
		return RawIPv4Capture::create(param);
	}


	LinuxCookedPacketType LinuxCookedFrame::getPacketType() const
	{
		return (LinuxCookedPacketType)(MIO::readUint16BE(m_packetType));
	}

	void LinuxCookedFrame::setPacketType(LinuxCookedPacketType type)
	{
		MIO::writeUint16BE(m_packetType, (sl_uint32)type);
	}

	NetworkCaptureType LinuxCookedFrame::getDeviceType() const
	{
		return (NetworkCaptureType)(MIO::readUint16BE(m_deviceType));
	}

	void LinuxCookedFrame::setDeviceType(NetworkCaptureType type)
	{
		MIO::writeUint16BE(m_deviceType, (sl_uint32)type);
	}

	sl_uint16 LinuxCookedFrame::getAddressLength() const
	{
		return MIO::readUint16BE(m_lenAddress);
	}

	void LinuxCookedFrame::setAddressLength(sl_uint16 len)
	{
		MIO::writeUint16BE(m_lenAddress, len);
	}

	const sl_uint8* LinuxCookedFrame::getAddress() const
	{
		return m_address;
	}

	sl_uint8* LinuxCookedFrame::getAddress()
	{
		return m_address;
	}

	EtherType LinuxCookedFrame::getProtocolType() const
	{
		return (EtherType)(MIO::readUint16BE(m_protocol));
	}

	void LinuxCookedFrame::setProtocolType(EtherType type)
	{
		MIO::writeUint16BE(m_protocol, (sl_uint32)type);
	}

	const sl_uint8* LinuxCookedFrame::getContent() const
	{
		return ((const sl_uint8*)this) + HeaderSize;
	}

	sl_uint8* LinuxCookedFrame::getContent()
	{
		return ((sl_uint8*)this) + HeaderSize;
	}

}
