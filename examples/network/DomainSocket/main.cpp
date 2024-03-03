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
		if (!(server.bind(DomainSocketPath(SERVER_PATH)))) {
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
			socket.bind(AbstractDomainSocketPath("first client"));
			if (!(socket.connect(DomainSocketPath(SERVER_PATH)))) {
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

		DomainSocketPath path;
		auto socket = server.accept(path);
		if (socket.isNone()) {
			Println("%s", Socket::getLastErrorMessage());
			return;
		}
		Println("Accepted: %s, Abstract: %s", path.get(), path.flagAbstract);
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
