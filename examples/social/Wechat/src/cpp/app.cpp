#include "app.h"

#include "MainPage.h"
#include "config.h"

WechatApp::WechatApp()
{
}

Ref<View> WechatApp::getStartupPage()
{
	return new MainPage;
}

void WechatApp::onStart()
{
	setAvailableScreenOrientationsPortrait();

	WeChatSDK::initialize(WECHAT_APP_ID, WECHAT_UNIVERSAL_LINK);

	// This should be used on server side. Here, added for testing purpose
	WeChat::initialize(WECHAT_APP_ID, WECHAT_APP_SECRET, WECHAT_UNIVERSAL_LINK);
}
