#include "app.h"

#include "LaunchScreen.h"

SLIB_TEMPLATE_APP_NAMEApp::SLIB_TEMPLATE_APP_NAMEApp()
{
}

Ref<View> SLIB_TEMPLATE_APP_NAMEApp::getStartupPage()
{
	return new LaunchScreen;
}

void SLIB_TEMPLATE_APP_NAMEApp::onStart()
{
	UI::setAvailableScreenOrientationsPortrait();
}
