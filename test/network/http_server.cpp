#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	System::setDebugFlags();

	HttpServerParam param;
	param.port = 10080;
	param.flagUseWebRoot = sl_true;
	param.webRootPath = "D:\\";

	auto server = HttpServer::create(param);
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