#include <slib/platform.h>

#include "../app.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
	System::setDebugFlags();
	P2PMsgApp::main();
	return 0;
}
