#include "app.h"

#include "MainPage.h"

SLIB_DEFINE_APPLICATION(ExampleContactApp)

ExampleContactApp::ExampleContactApp()
{
}

Ref<View> ExampleContactApp::getStartupPage()
{
	return new MainPage;
}

void ExampleContactApp::onStart()
{
	UI::setAvailableScreenOrientationsPortrait();
}
