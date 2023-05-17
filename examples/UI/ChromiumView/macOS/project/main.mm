#include "app.h"

int main(int argc, const char * argv[])
{
	Chromium::startup(argc, argv);
	ChromiumViewApp::main();
	Chromium::shutdown();
	return 0;
}
