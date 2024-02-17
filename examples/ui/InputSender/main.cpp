#include <slib/platform.h>

#include <input_sender/input_sender.h>

#include "app.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
	System::setDebugFlags();
	if (InputSender::prepare()) {
		InputSenderApp::main();
	}
	return 0;
}
