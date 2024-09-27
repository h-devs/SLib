#include <slib.h>
#include <slib/network/dhcp.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	System::setDebugFlags();

	DhcpServerParam param;
	param.onBind = [](DhcpServer*, DhcpBindParam& param) {
		param.subnetMask = IPv4Address(255, 255, 0, 0);
		param.server = IPv4Address(172, 16, 200, 200);
		param.broadcastAddress = IPv4Address(172, 16, 255, 255);
		param.router = IPv4Address(172, 16, 200, 200);
		param.domainName = "test";
		param.searchDomain = "test.com";
		param.domainServers.add(IPv4Address(4, 4, 4, 4));
		param.domainServers.add(IPv4Address(8, 8, 8, 8));
		if (param.mac == MacAddress("A4-4C-C8-6D-4D-CE")) {
			param.ip = IPv4Address(172, 16, 200, 100);
		} else if (param.mac == MacAddress("F8-E4-3B-77-BB-42")) {
			param.ip = IPv4Address(172, 16, 200, 101);
		}
	};

	auto server = DhcpServer::create(param);
	if (server.isNull()) {
		Println("Failed to create server!");
		return 0;
	}

	Println("Press x to exit!");
	for (;;) {
		if (Console::readChar() == 'x') {
			break;
		}
		System::sleep(10);
	}
	return 0;
}
