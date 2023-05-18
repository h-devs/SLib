#include "app.h"

SLIB_TEMPLATE_APP_NAMEApp::SLIB_TEMPLATE_APP_NAMEApp()
{
}

void SLIB_TEMPLATE_APP_NAMEApp::onStart()
{
	auto window = New<Window>();
	window->setTitle("SLIB_TEMPLATE_APP_NAME");
	window->setFrame(100, 100, 400, 300);
	window->show();
	setMainWindow(window);
}
