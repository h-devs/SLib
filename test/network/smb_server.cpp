#include <slib.h>
#include <slib/network/smb.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	System::setDebugFlags();

	SmbServerParam param;
	param.bindAddress = IPv6Address::getLoopback();
	param.addFileShare("Test1", "D:");
	param.addFileShare("Test2", "E:", "Test Share");
	param.addFileShare("Get$", "C:");

	auto server = SmbServer::create(param);
	if (server.isNull()) {
		return -1;
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
