#include <slib.h>

#include <slib/data/expiring_map.h>

using namespace slib;

#define PORT 30001

int main(int argc, const char * argv[])
{
	System::setDebugFlags();

	if (StringView(argv[1]).isEmpty()) {

		Println("Binding server port %d", PORT);

		static ExpiringMap< AsyncSocketStream*, Ref<AsyncSocketStream> > clients;
		clients.setExpiringMilliseconds(30000);

		AsyncTcpServerParam param;
		param.bindAddress.port = PORT;

		param.onAccept = [](AsyncTcpServer*, Socket& socket, const SocketAddress& address) {

			auto client = AsyncSocketStream::create(Move(socket));
			if (client.isNull()) {
				Println("Failed to create client socket!");
				return;
			}

			Println("Connected client: %s", address.toString());
			clients.put(client.get(), client);

			client->receive(Memory::create(100), [](AsyncStreamResult& result) {
				AsyncSocketStream* client = (AsyncSocketStream*)(result.stream);
				if (result.isError()) {
					Println("Client Error!");
					clients.remove(client);
					return;
				}
				if (result.isEnded()) {
					Println("Client Ended!");
					clients.remove(client);
					return;
				}
				Println("Received: %s", StringView((char*)(result.data), result.size));
				clients.get(client);
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

		auto socket = AsyncTcpSocket::create();
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

		socket->connect(address, [timer](AsyncTcpSocket*, sl_bool flagError) {
			if (!flagError) {
				timer->start();
			}
		});

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