#pragma once

#include <slib.h>

#include "../gen/resources.h"

using namespace slib;
using namespace sapp;

class ContactApp : public MobileApp
{
	SLIB_APPLICATION(ContactApp)
public:
	ContactApp();

protected:
	Ref<View> getStartupPage() override;

	void onStart() override;

};
