#include <slib.h>

using namespace slib;

#define PORT 30001

int main(int argc, const char * argv[])
{
	System::setDebugFlags();

	if (StringView(argv[1]).isEmpty()) {

		Println("Binding server port %d", PORT);

		ExpiringMap< AsyncTcpSocket*, Ref<AsyncTcpSocket> > sockets;
		sockets.setExpiringMilliseconds(30000);

		AsyncTcpServerParam param;
		param.bindAddress.port = PORT;

		param.onAccept = [&sockets](AsyncTcpServer*, Socket& socket, const SocketAddress& address) {

			AsyncTcpSocketParam param;
			param.socket = Move(socket);

			auto client = AsyncTcpSocket::create(param);
			if (client.isNull()) {
				Println("Failed to create client socket!");
				return;
			}

			Println("Connected client: %s", address.toString());
			sockets.put(client.get(), client);

			client->receive(Memory::create(100), [&sockets](AsyncStreamResult& result) {
				AsyncTcpSocket* client = (AsyncTcpSocket*)(result.stream);
				if (result.isError()) {
					Println("Client Error!");
					sockets.remove(client);
					return;
				}
				if (result.isEnded()) {
					Println("Client Ended!");
					sockets.remove(client);
					return;
				}
				Println("Received: %s", StringView((char*)(result.data), result.size));
				sockets.get(client);
				client->receive(result.data, result.requestSize, result.callback, result.userObject);
			});
		};

		param.onError = [](AsyncTcpServer*) {
			Println("Server Error!");
		};

		auto server = AsyncTcpServer::create(param);
		if (server.isNull()) {
			Println("Failed to start server!");
			return -1;
		}

		Println("Press x to exit!");
		for (;;) {
			sl_char16 c = Console::readChar();
			if (c == 'x') {
				break;
			}
			System::sleep(10);
		}
		return 0;

	} else {

		SocketAddress address;
		if (!(address.ip.parse(argv[1]))) {
			Println("Failed to parse ip address!");
			return -1;
		}
		address.port = PORT;
		
		Println("Connecting to %s", address.toString());

		AsyncTcpSocketParam param;
		param.connectAddress = address;

		auto socket = AsyncTcpSocket::create(param);
		if (socket.isNull()) {
			Println("Failed to start socket!");
			return -1;
		}

		auto timer = Timer::start([socket](Timer*) {
			static int n = 0;
			n++;
			String s = String::format("Message %d", n);
			socket->send(s.toMemory(), [](AsyncStreamResult& result) {
				if (result.isError()) {
					Println("Server Error!");
					return;
				}
				if (result.isEnded()) {
					Println("Server Ended!");
					return;
				}
				Println("Sent: %s", StringView((char*)(result.data), result.size));
			});
		}, 1000);

		Println("Press x to exit!");
		for (;;) {
			sl_char16 c = Console::readChar();
			if (c == 'x') {
				break;
			}
			System::sleep(10);
		}

		timer->stopAndWait();

		return 0;
	}
	
}