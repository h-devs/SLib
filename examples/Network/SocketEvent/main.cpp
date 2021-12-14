#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	System::setDebugFlags();

	auto threadReceive1 = Thread::start([]() {
		Println("Started Thread1");
		auto sock = Socket::openUdp(10001);
		if (sock.isNone()) {
			Println("Thread1: Failed to create socket!");
			return;
		}
		auto ev = SocketEvent::createRead(sock);
		if (ev.isNull()) {
			Println("Thread1: Failed to create event!");
			return;
		}
		char buf[100];
		while (Thread::isNotStoppingCurrent()) {
			sl_uint32 status = ev->waitEvents();
			if (status) {
				Println("Thread1: Events: %d", status);
				if (status & SocketEvent::Read) {
					SocketAddress address;
					int n = sock.receiveFrom(address, buf, sizeof(buf));
					if (n > 0) {
						Println("Thread1: Received: %s", String(buf, n));
					} else {
						Println("Thread1: Error: %d", n);
					}
				}
			} else {
				Println("Thread1: Interruped");
			}
		}
		Println("Stopped Thread1");
	});
	
	auto threadReceive2 = Thread::start([]() {
		Println("Started Thread2");
		auto sock1 = Socket::openUdp(10002);
		if (sock1.isNone()) {
			Println("Thread2: Failed to create 1st socket!");
			return;
		}
		auto ev1 = SocketEvent::createRead(sock1);
		if (ev1.isNull()) {
			Println("Thread2: Failed to create 1st event!");
			return;
		}
		auto sock2 = Socket::openUdp(10003);
		if (sock2.isNone()) {
			Println("Thread2: Failed to create 2nd socket!");
			return;
		}
		auto ev2 = SocketEvent::createRead(sock2);
		if (ev2.isNull()) {
			Println("Thread2: Failed to create 2nd event!");
			return;
		}

		sl_uint32 status[2];
		SocketEvent* events[] = { ev1.get(), ev2.get() };
		char buf[100];
		while (Thread::isNotStoppingCurrent()) {
			if (SocketEvent::waitMultipleEvents(events, status, 2)) {
				Println("Thread2: Events: %d %d", status[0], status[1]);
				if (status[0] & SocketEvent::Read) {
					SocketAddress address;
					int n = sock1.receiveFrom(address, buf, sizeof(buf));
					if (n > 0) {
						Println("Thread2: Socket1 Received: %s", String(buf, n));
					} else {
						Println("Thread2: Socket1 Error: %d", n);
					}
				}
				if (status[1] & SocketEvent::Read) {
					SocketAddress address;
					int n = sock2.receiveFrom(address, buf, sizeof(buf));
					if (n > 0) {
						Println("Thread2: Socket2 Received: %s", String(buf, n));
					} else {
						Println("Thread2: Socket2 Error: %d", n);
					}
				}
			} else {
				Println("Thread2: Interruped");
			}
		}
		Println("Stopped Thread2");
	});

	auto threadSend = Thread::start([]() {
		Println("Started Thread3");
		auto sock = Socket::openUdp();
		sl_uint32 n = 0;
		while (Thread::isNotStoppingCurrent()) {
			{
				Thread::sleep(1000);
				auto s = String::format("Test Packet %s", ++n);
				sock.sendTo(SocketAddress(IPv4Address::Loopback, 10001), s.getData(), s.getLength());
			}
			{
				Thread::sleep(1000);
				auto s = String::format("Test Packet %s", ++n);
				sock.sendTo(SocketAddress(IPv4Address::Loopback, 10002), s.getData(), s.getLength());
			}
			{
				Thread::sleep(1000);
				auto s = String::format("Test Packet %s", ++n);
				sock.sendTo(SocketAddress(IPv4Address::Loopback, 10003), s.getData(), s.getLength());
			}
		}
		Println("Stopped Thread3");
	});

	Println("Press x to exit!");
	for (;;) {
		if (Console::readChar() == 'x') {
			break;
		}
		System::sleep(10);
	}

	threadSend->finishAndWait();
	threadReceive1->finishAndWait();
	threadReceive2->finishAndWait();

	return 0;
}
