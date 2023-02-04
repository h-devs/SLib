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

#include "slib/network/capture.h"

#include "slib/core/thread.h"
#include "slib/core/system.h"
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
		preferedLinkDeviceType = NetworkLinkDeviceType::Ethernet;
		flagPromiscuous = sl_false;

		flagAutoStart = sl_true;
	}


	SLIB_DEFINE_OBJECT(NetCapture, Object)

	NetCapture::NetCapture()
	{
		m_timeDeviceAddress = 0;
	}

	NetCapture::~NetCapture()
	{
	}

	sl_bool NetCapture::setLinkType(sl_uint32 type)
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

	const MacAddress& NetCapture::getDeviceAddress()
	{
		sl_uint64 now = System::getTickCount64();
		if (m_timeDeviceAddress && now >= m_timeDeviceAddress && now < m_timeDeviceAddress + 10000) {
			return m_deviceAddress;
		}
		StringView name = m_deviceName;
		NetworkInterfaceInfo info;
#ifdef SLIB_PLATFORM_IS_WIN32
		// Remove prefix: \Device\NPF_
		sl_reg index = name.indexOf('{');
		if (index > 0) {
			name = name.substring(index);
		}
#endif
		if (Network::findInterface(name, &info)) {
			m_deviceAddress = info.macAddress;
		} else {
			m_deviceAddress.setZero();
		}
		m_timeDeviceAddress = now;
		return m_deviceAddress;
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

	namespace {
		class RawPacketCapture : public NetCapture
		{
		public:
			Socket m_socket;

			NetworkLinkDeviceType m_deviceType;
			sl_uint32 m_ifaceIndex;
			Memory m_bufPacket;
			Ref<Thread> m_thread;

			sl_bool m_flagInit;
			sl_bool m_flagRunning;

		public:
			RawPacketCapture()
			{
				m_deviceType = NetworkLinkDeviceType::Ethernet;
				m_ifaceIndex = 0;

				m_flagInit = sl_false;
				m_flagRunning = sl_false;
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
				NetworkLinkDeviceType deviceType = param.preferedLinkDeviceType;
				if (deviceType == NetworkLinkDeviceType::Raw) {
					socket = Socket::openPacketDatagram(NetworkLinkProtocol::All);
				} else {
					deviceType = NetworkLinkDeviceType::Ethernet;
					socket = Socket::openPacketRaw(NetworkLinkProtocol::All);
				}
				if (socket.isOpened()) {
					if (iface > 0) {
						if (param.flagPromiscuous) {
							if (!(socket.setPromiscuousMode(deviceName, sl_true))) {
								Log(TAG, "Failed to set promiscuous mode to the network device: %s", deviceName);
							}
						}
						if (!(socket.setOption_bindToDevice(deviceName))) {
							Log(TAG, "Failed to bind the network device: %s", deviceName);
						}
					}
					Memory mem = Memory::create(MAX_PACKET_SIZE);
					if (mem.isNotNull()) {
						Ref<RawPacketCapture> ret = new RawPacketCapture;
						if (ret.isNotNull()) {
							ret->_initWithParam(param);
							ret->m_bufPacket = Move(mem);
							ret->m_socket = Move(socket);
							ret->m_deviceType = deviceType;
							ret->m_ifaceIndex = iface;
							ret->m_thread = Thread::create(SLIB_FUNCTION_MEMBER(ret.get(), _run));
							if (ret->m_thread.isNotNull()) {
								ret->m_flagInit = sl_true;
								if (param.flagAutoStart) {
									ret->start();
								}
								return ret;
							} else {
								LogError(TAG, "Failed to create thread");
							}
						}
					}
				} else {
					LogError(TAG, "Failed to create Packet socket");
				}
				return sl_null;
			}

			void release()
			{
				ObjectLocker lock(this);
				if (!m_flagInit) {
					return;
				}
				m_flagInit = sl_false;

				m_flagRunning = sl_false;
				if (m_thread.isNotNull()) {
					m_thread->finishAndWait();
					m_thread.setNull();
				}
				m_socket.setNone();
			}

			void start()
			{
				ObjectLocker lock(this);
				if (!m_flagInit) {
					return;
				}

				if (m_flagRunning) {
					return;
				}
				if (m_thread.isNotNull()) {
					if (m_thread->start()) {
						m_flagRunning = sl_true;
					}
				}
			}

			sl_bool isRunning()
			{
				return m_flagRunning;
			}

			void _run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}

				NetCapturePacket packet;

				Socket& socket = m_socket;

				socket.setNonBlockingMode();
				Ref<SocketEvent> event = SocketEvent::createRead(socket);
				if (event.isNull()) {
					return;
				}

				sl_uint8* buf = (sl_uint8*)(m_bufPacket.getData());
				sl_uint32 sizeBuf = (sl_uint32)(m_bufPacket.getSize());

				while (thread->isNotStopping()) {
					L2PacketInfo info;
					sl_int32 n = socket.receivePacket(buf, sizeBuf, info);
					if (n >= 0) {
						packet.data = buf;
						packet.length = n;
						packet.time = 0;
						_onCapturePacket(packet);
					} else if (n == SLIB_IO_WOULD_BLOCK) {
						event->wait();
					} else {
						break;
					}
				}
			}

			NetworkLinkDeviceType getLinkType()
			{
				return m_deviceType;
			}

			sl_bool sendPacket(const void* buf, sl_uint32 size)
			{
				if (m_ifaceIndex == 0) {
					return sl_false;
				}
				if (m_flagInit) {
					L2PacketInfo info;
					info.type = L2PacketType::OutGoing;
					info.iface = m_ifaceIndex;
					if (m_deviceType == NetworkLinkDeviceType::Ethernet) {
						EthernetFrame* frame = (EthernetFrame*)buf;
						if (size < EthernetFrame::HeaderSize) {
							return sl_false;
						}
						info.protocol = frame->getProtocol();
						info.setMacAddress(frame->getDestinationAddress());
					} else {
						info.protocol = NetworkLinkProtocol::IPv4;
						info.clearAddress();
					}
					sl_uint32 ret = m_socket.sendPacket(buf, size, info);
					if (ret == size) {
						return sl_true;
					}
				}
				return sl_false;
			}
		};
	}

	Ref<NetCapture> NetCapture::createRawPacket(const NetCaptureParam& param)
	{
		return RawPacketCapture::create(param);
	}

	namespace {
		class RawIPv4Capture : public NetCapture
		{
		public:
			Socket m_socketTCP;
			Socket m_socketUDP;
			Socket m_socketICMP;

			Memory m_bufPacket;
			Ref<Thread> m_thread;

			sl_bool m_flagInit;
			sl_bool m_flagRunning;

		public:
			RawIPv4Capture()
			{
				m_flagInit = sl_false;
				m_flagRunning = sl_false;
			}

			~RawIPv4Capture()
			{
				release();
			}

		public:
			static Ref<RawIPv4Capture> create(const NetCaptureParam& param)
			{
				Socket socketTCP = Socket::openRaw(NetworkInternetProtocol::TCP);
				Socket socketUDP = Socket::openRaw(NetworkInternetProtocol::UDP);
				Socket socketICMP = Socket::openRaw(NetworkInternetProtocol::ICMP);
				if (socketTCP.isOpened() && socketUDP.isOpened() && socketICMP.isOpened()) {
					socketTCP.setOption_IncludeIpHeader(sl_true);
					socketUDP.setOption_IncludeIpHeader(sl_true);
					socketICMP.setOption_IncludeIpHeader(sl_true);
					Memory mem = Memory::create(MAX_PACKET_SIZE);
					if (mem.isNotNull()) {
						Ref<RawIPv4Capture> ret = new RawIPv4Capture;
						if (ret.isNotNull()) {
							ret->_initWithParam(param);
							ret->m_bufPacket = Move(mem);
							ret->m_socketTCP = Move(socketTCP);
							ret->m_socketUDP = Move(socketUDP);
							ret->m_socketICMP = Move(socketICMP);
							ret->m_thread = Thread::create(SLIB_FUNCTION_MEMBER(ret.get(), _run));
							if (ret->m_thread.isNotNull()) {
								ret->m_flagInit = sl_true;
								if (param.flagAutoStart) {
									ret->start();
								}
								return ret;
							} else {
								LogError(TAG, "Failed to create thread");
							}
						}
					}
				}
				return sl_null;
			}

			void release()
			{
				ObjectLocker lock(this);
				if (!m_flagInit) {
					return;
				}
				m_flagInit = sl_false;

				m_flagRunning = sl_false;
				if (m_thread.isNotNull()) {
					m_thread->finishAndWait();
					m_thread.setNull();
				}
				m_socketTCP.setNone();
				m_socketUDP.setNone();
				m_socketICMP.setNone();
			}

			void start()
			{
				ObjectLocker lock(this);
				if (!m_flagInit) {
					return;
				}

				if (m_flagRunning) {
					return;
				}

				if (m_thread.isNotNull()) {
					if (m_thread->start()) {
						m_flagRunning = sl_true;
					}
				}
			}

			sl_bool isRunning()
			{
				return m_flagRunning;
			}

			void _run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}

				NetCapturePacket packet;

				Socket& socketTCP = m_socketTCP;
				if (socketTCP.isNone()) {
					return;
				}
				socketTCP.setNonBlockingMode();
				Ref<SocketEvent> eventTCP = SocketEvent::createRead(socketTCP);
				if (eventTCP.isNull()) {
					return;
				}

				Socket& socketUDP = m_socketUDP;
				if (socketUDP.isNone()) {
					return;
				}
				socketUDP.setNonBlockingMode();
				Ref<SocketEvent> eventUDP = SocketEvent::createRead(socketUDP);
				if (eventUDP.isNull()) {
					return;
				}

				Socket& socketICMP = m_socketICMP;
				if (socketICMP.isNone()) {
					return;
				}
				socketICMP.setNonBlockingMode();
				Ref<SocketEvent> eventICMP = SocketEvent::createRead(socketICMP);
				if (eventICMP.isNull()) {
					return;
				}

				SocketEvent* events[3];
				events[0] = eventTCP.get();
				events[1] = eventUDP.get();
				events[2] = eventICMP.get();

				sl_uint8* buf = (sl_uint8*)(m_bufPacket.getData());
				sl_uint32 sizeBuf = (sl_uint32)(m_bufPacket.getSize());

				while (thread->isNotStopping()) {
					SocketAddress address;
					sl_int32 n = socketTCP.receiveFrom(address, buf, sizeBuf);
					if (n >= 0) {
						packet.data = buf;
						packet.length = n;
						packet.time = 0;
						_onCapturePacket(packet);
					} else if (n != SLIB_IO_WOULD_BLOCK) {
						break;
					} else {
						n = socketUDP.receiveFrom(address, buf, sizeBuf);
						if (n >= 0) {
							packet.data = buf;
							packet.length = n;
							packet.time = 0;
							_onCapturePacket(packet);
						} else if (n != SLIB_IO_WOULD_BLOCK) {
							break;
						} else {
							n = socketICMP.receiveFrom(address, buf, sizeBuf);
							if (n >= 0) {
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

			NetworkLinkDeviceType getLinkType()
			{
				return NetworkLinkDeviceType::Raw;
			}

			sl_bool sendPacket(const void* buf, sl_uint32 size)
			{
				if (m_flagInit) {
					SocketAddress address;
					if (IPv4Packet::check(buf, size)) {
						IPv4Packet* ip = (IPv4Packet*)buf;
						address.ip = ip->getDestinationAddress();
						address.port = 0;
						Socket* socket;
						NetworkInternetProtocol protocol = ip->getProtocol();
						if (protocol == NetworkInternetProtocol::TCP) {
							socket = &m_socketTCP;
						} else if (protocol == NetworkInternetProtocol::UDP) {
							socket = &m_socketUDP;
						} else if (protocol == NetworkInternetProtocol::ICMP) {
							socket = &m_socketICMP;
						} else {
							return sl_false;
						}
						sl_uint32 ret = socket->sendTo(address, buf, size);
						if (ret == size) {
							return sl_true;
						}
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

	NetworkLinkDeviceType LinuxCookedFrame::getDeviceType() const
	{
		return (NetworkLinkDeviceType)(MIO::readUint16BE(m_deviceType));
	}

	void LinuxCookedFrame::setDeviceType(NetworkLinkDeviceType type)
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

	NetworkLinkProtocol LinuxCookedFrame::getProtocolType() const
	{
		return (NetworkLinkProtocol)(MIO::readUint16BE(m_protocol));
	}

	void LinuxCookedFrame::setProtocolType(NetworkLinkProtocol type)
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
