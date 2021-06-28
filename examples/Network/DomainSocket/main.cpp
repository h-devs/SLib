#include <slib.h>

using namespace slib;

#define SERVER_PATH "test"

int main(int argc, const char * argv[])
{
	// Listening Thread
	Thread::start([]() {
		auto server = Socket::openDomainStream();
		if (server.isNone()) {
			Println("Domain socket is not supported!");
			return;
		}
		if (!(server.bindDomain(SERVER_PATH))) {
			Println("Bind: %s", Socket::getLastErrorMessage());
			return;
		}
		if (!(server.listen())) {
			Println("Listen: %s", Socket::getLastErrorMessage());
			return;
		}

		// Connecting Thread
		Thread::start([]() {
			auto socket = Socket::openDomainStream();
			socket.bindAbstractDomain("first client");
			if (!(socket.connectDomain(SERVER_PATH))) {
				Println("Connect: %s", Socket::getLastErrorMessage());
				return;
			}
			int index = 1;
			for (;;) {
				String msg = String::format("Message %d", index++);
				Serialize(&socket, msg);
				Thread::sleep(1000);
			}
		});

		String pathClient;
		sl_bool flagAbstract;
		auto socket = server.acceptDomain(pathClient, &flagAbstract);
		if (socket.isNone()) {
			Println("%s", Socket::getLastErrorMessage());
			return;
		}
		Println("Accepted: %s, Abstract: %s", pathClient, flagAbstract);
		for (;;) {
			String msg;
			Deserialize(&socket, msg);
			Println(msg);
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
