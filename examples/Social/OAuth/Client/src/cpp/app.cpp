#include "app.h"

#include "MainWindow.h"

OAuthClientApp::OAuthClientApp()
{
}

void OAuthClientApp::onStart()
{
	Ref<MainWindow> window = new MainWindow;
	window->create();
	setMainWindow(window);
}
