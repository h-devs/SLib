#include "app.h"

#include "MainPage.h"

ExampleContactApp::ExampleContactApp()
{
}

Ref<View> ExampleContactApp::getStartupPage()
{
	return new MainPage;
}

void ExampleContactApp::onStart()
{
	setAvailableScreenOrientationsPortrait();
}
