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
	setAvailableScreenOrientationsPortrait();

	MainPage::getInstance()->initPage();
}
