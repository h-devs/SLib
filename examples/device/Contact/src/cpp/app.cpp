#include "app.h"

#include "MainPage.h"

ContactApp::ContactApp()
{
}

Ref<View> ContactApp::getStartupPage()
{
	return new MainPage;
}

void ContactApp::onStart()
{
	setAvailableScreenOrientationsPortrait();
}
