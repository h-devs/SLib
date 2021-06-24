#include <slib/core/platform.h>

#include "../app.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
	System::setDebugFlags();
	MicMonApp::main();
	return 0;
}
