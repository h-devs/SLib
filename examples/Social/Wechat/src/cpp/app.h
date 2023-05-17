#pragma once

#include <slib.h>

#include "../gen/resources.h"

using namespace slib;
using namespace sapp;

class WechatApp : public MobileApp
{
	SLIB_APPLICATION(WechatApp)
public:
	WechatApp();

protected:
	Ref<View> getStartupPage() override;

	void onStart() override;

};
