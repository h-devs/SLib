#include "app.h"

#include "MainWindow.h"

SLIB_TEMPLATE_APP_NAMEApp::SLIB_TEMPLATE_APP_NAMEApp()
{
}

void SLIB_TEMPLATE_APP_NAMEApp::onStart()
{
	Ref<MainWindow> window = new MainWindow;
	window->create();
	setMainWindow(window);
}
