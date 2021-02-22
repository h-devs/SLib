#include "app.h"

#include "MainPage.h"

SLIB_DEFINE_APPLICATION(PhoneCallApp)

PhoneCallApp::PhoneCallApp()
{
}

Ref<View> PhoneCallApp::getStartupPage()
{
	return MainPage::getInstance();
}

void PhoneCallApp::onStart()
{
	UI::setAvailableScreenOrientationsPortrait();

	MainPage::getInstance()->initPage();
}
