#pragma once

#include <slib.h>

#include "../gen/resources.h"

using namespace slib;
using namespace sapp;

class AlipayApp : public MobileApp
{
	SLIB_APPLICATION(AlipayApp)
public:
	AlipayApp();

protected:
	Ref<View> getStartupPage() override;

	void onStart() override;

};
