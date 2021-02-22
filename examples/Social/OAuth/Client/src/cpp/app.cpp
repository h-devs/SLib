#include "app.h"

#include "MainWindow.h"

SLIB_DEFINE_APPLICATION(OAuthClientApp)

OAuthClientApp::OAuthClientApp()
{
}

void OAuthClientApp::onStart()
{
	Ref<MainWindow> window = new MainWindow;
	window->create();
	setMainWindow(window);
}
