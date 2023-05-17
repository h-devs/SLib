#include "app.h"

#include "config.h"
#include "MainPage.h"

AlipayApp::AlipayApp()
{
	AlipaySDK::initialize(ALIPAY_APP_SCHEME);
}

Ref<View> AlipayApp::getStartupPage()
{
	return new MainPage;
}

void AlipayApp::onStart()
{
	setAvailableScreenOrientationsPortrait();
}
