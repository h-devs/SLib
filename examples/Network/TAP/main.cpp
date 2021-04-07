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

#include <slib.h>

using namespace slib;

#define TAP_IP "10.0.0.10"
#define TAP_MASK "255.0.0.0"
#define UDP_PORT 45678
#define TAP_TARGET_IP IPv4Address(10, 0, 0, 20)
#define TAP_TARGET_MAC MacAddress(1, 2, 3, 4, 5, 6)

int main(int argc, const char * argv[])
{
	if (!(Process::isAdmin())) {
		Println("Run as administrator");
		return -1;
	}
	if (StringView(argv[1]) == "uninstall") {
		if (Tap::uninstall()) {
			Println("Uninstalled Tap driver!");
			return 0;
		} else {
			Println("Failed to uninstalled Tap driver!");
		}
		return -1;
	}
	if (!(Tap::install())) {
		Println("Failed to install Tap driver!");
		return -1;
	}
	if (StringView(argv[1]) == "install") {
		return 0;
	}

	auto tap = Tap::open();
	if (tap.isNull()) {
		Println("Failed to open tap device");
		return -1;
	}

	if (tap->setIpAddress(TAP_IP, TAP_MASK)) {
		Println("Device %s set to %s/%s", tap->getInterfaceName(), TAP_IP, TAP_MASK);
	} else {
		Println("Failed to set device ip address: %s, %s/%s", tap->getInterfaceName(), TAP_IP, TAP_MASK);
		return -1;
	}

	auto socket = Socket::openUdp();
	if (socket.isNull()) {
		Println("Failed to open UDP socket!");
		return -1;
	}
	if (!(socket->bind(UDP_PORT))) {
		Println("Failed to bind to UDP port: %s", UDP_PORT);
		return -1;
	}

	auto threadTapRead = Thread::start([tap]() {
		char buf[4096];
		auto thread = Thread::getCurrent();
		while (thread->isNotStopping()) {
			sl_int32 n = tap->read(buf, sizeof(buf));
			if (n > EthernetFrame::HeaderSize) {
				EthernetFrame* frame = (EthernetFrame*)buf;
				if (frame->getProtocol() == NetworkLinkProtocol::IPv4) {
					IPv4Packet* packet = (IPv4Packet*)(frame->getContent());
					sl_uint32 size = n - EthernetFrame::HeaderSize;
					if (IPv4Packet::check(packet, size)) {
						if (packet->getProtocol() == NetworkInternetProtocol::UDP) {
							UdpDatagram* udp = (UdpDatagram*)(packet->getContent());
							sl_uint32 sizeUdp = packet->getContentSize();
							if (udp->check(packet, sizeUdp) && udp->getDestinationPort() == UDP_PORT) {
								Println("TAP Received %s->%s: %s",
									packet->getSourceAddress().toString(),
									packet->getDestinationAddress().toString(),
									StringView((char*)(udp->getContent()), udp->getContentSize())
								);
							}
						} else if (packet->getProtocol() == NetworkInternetProtocol::ICMP) {
							IcmpHeaderFormat* icmp = (IcmpHeaderFormat*)(packet->getContent());
							sl_uint32 sizeIcmp = packet->getContentSize();
							if (icmp->check(sizeIcmp)) {
								if (icmp->getType() == IcmpType::Echo) {
									Println("TAP Received PING %s->%s",
										packet->getSourceAddress().toString(),
										packet->getDestinationAddress().toString()
									);
									if (packet->getDestinationAddress() == TAP_TARGET_IP) {
										frame->setDestinationAddress(frame->getSourceAddress());
										frame->setSourceAddress(TAP_TARGET_MAC);
										packet->setDestinationAddress(packet->getSourceAddress());
										packet->setSourceAddress(TAP_TARGET_IP);
										icmp->setType(IcmpType::EchoReply);
										icmp->updateChecksum(sizeIcmp);
										packet->updateChecksum();
										tap->write(frame, n);
									}
								}
							}
						}
					}
				} else if (frame->getProtocol() == NetworkLinkProtocol::ARP) {
					ArpPacket* arp = (ArpPacket*)(frame->getContent());
					sl_uint32 size = n - EthernetFrame::HeaderSize;
					if (size >= ArpPacket::SizeForIPv4) {
						if (arp->getOperation() == ArpOperation::Request) {
							if (arp->isValidEthernetIPv4()) {
								Println("ARP Request: %s,%s -> %s,%s",
									arp->getSenderIPv4Address().toString(),
									arp->getSenderMacAddress().toString(),
									arp->getTargetIPv4Address().toString(),
									arp->getTargetMacAddress().toString()
								);
								if (arp->getTargetIPv4Address() == TAP_TARGET_IP) {
									frame->setDestinationAddress(frame->getSourceAddress());
									frame->setSourceAddress(TAP_TARGET_MAC);
									arp->setOperation(ArpOperation::Reply);
									arp->setTargetIPv4Address(arp->getSenderIPv4Address());
									arp->setTargetMacAddress(arp->getSenderMacAddress());
									arp->setSenderIPv4Address(TAP_TARGET_IP);
									arp->setSenderMacAddress(TAP_TARGET_MAC);
									tap->write(frame, n);
								}
							}
						}
					}
				}
			}
		}
	});

	auto threadSend = Thread::start([socket]() {
		SocketAddress address(TAP_TARGET_IP, UDP_PORT);
		sl_uint32 no = 1;
		auto thread = Thread::getCurrent();
		while (thread->isNotStopping()) {
			String data = String::format("Packet %s", no++);
			socket->sendTo(address, data.getData(), (sl_uint32)(data.getLength()));
			Thread::sleep(1000);
		}
	});

	Println("Input x to exit!");
	for (;;) {
		if (Console::readChar() == 'x') {
			break;
		}
		System::sleep(100);
	}

	threadTapRead->finishAndWait();
	threadSend->finishAndWait();

	return 0;
}
