#include <slib.h>

#include "../common.h"

using namespace slib;

int main(int argc, const char * argv[])
{
	Printf("Input the server address [localhost:%d]: ", DEFAULT_SERVER_PORT);
	String serverAddress = Console::readLine().trim();
	if (serverAddress.isEmpty()) {
		serverAddress = String::format("localhost:%d", DEFAULT_SERVER_PORT);
	}

	Printf("Input the user id [0]: ");
	String userId = Console::readLine().trim();
	if (userId.isEmpty()) {
		userId = "0";
	}

	auto timer = Dispatch::setInterval([serverAddress, userId](Timer*) {
		auto image = ScreenCapture::takeScreenshot();
		if (image) {
			auto jpeg = image->saveJpeg();
			if (jpeg) {
				UrlRequest::sendSynchronous(HttpMethod::PUT, String::format("http://%s/screen/%s", serverAddress, userId), jpeg);
			}
		}
	}, 200);

	for (;;) {
		Console::println("\nPress x to exit!!!");
		if (Console::readChar() == 'x') {
			break;
		}
	}

	return 0;
}
