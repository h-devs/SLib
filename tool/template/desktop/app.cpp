#include "app.h"

SLIB_TEMPLATE_APP_NAMEApp::SLIB_TEMPLATE_APP_NAMEApp()
{
}

void SLIB_TEMPLATE_APP_NAMEApp::onStart()
{
	auto window = New<Window>();
	window->setTitle("SLIB_TEMPLATE_APP_NAME");
	window->setFrame(100, 100, 400, 300);
	window->setOnClose([](Window* window, UIEvent* ev) {
		UIApp::quit();
	});
	window->show();
	setMainWindow(window);
}
