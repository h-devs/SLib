#include <slib.h>

using namespace slib;

//#define USE_IPV6
#define PORT 44444

int main(int argc, const char * argv[])
{
	// Receiving Thread
	Thread::start([]() {
#ifdef USE_IPV6
		auto socket = Socket::openUdp_IPv6();
#else
		auto socket = Socket::openUdp();
#endif
		if (!(socket.bind(SocketAddress(PORT)))) {
			Println("%s", Socket::getLastErrorMessage());
			return;
		}
		for (;;) {
			char buf[1024];
			SocketAddress address;
			sl_int32 n = socket.receiveFrom(address, buf, sizeof(buf));
			if (n > 0) {
				Println("Received from %s: %s", address.toString(), StringView(buf, n));
			} else {
				Thread::sleep(10);
			}
		}
	});

	// Sending Thread
	Thread::start([]() {
#ifdef USE_IPV6
		auto socket = Socket::openUdp_IPv6();
#else
		auto socket = Socket::openUdp();
#endif
		int index = 1;
		for (;;) {
			String msg = String::format("Message %d", index++);
#ifdef USE_IPV6
			sl_int32 n = socket.sendTo(SocketAddress(IPv6Address::getLoopback(), PORT), msg.getData(), msg.getLength());
#else
			sl_int32 n = socket.sendTo(SocketAddress(IPv4Address::Loopback, PORT), msg.getData(), msg.getLength());
#endif
			if (n <= 0) {
				Println("%s", Socket::getLastErrorMessage());
			}
			Thread::sleep(1000);
		}
	});

	Println("Press x to exit!");
	for (;;) {
		if (Console::readChar() == 'x') {
			break;
		}
		System::sleep(10);
	}
	return 0;
}
