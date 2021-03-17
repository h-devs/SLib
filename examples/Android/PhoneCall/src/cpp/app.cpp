#include "app.h"

#include "MainPage.h"

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
