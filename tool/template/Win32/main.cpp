#include <slib/platform.h>

#include "app.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
	System::setDebugFlags();
	SLIB_TEMPLATE_APP_NAMEApp::main();
	return 0;
}
