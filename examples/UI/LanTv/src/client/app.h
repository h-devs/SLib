#pragma once

#include <slib.h>

using namespace slib;

class LanTvClientApp : public UIApp
{
	SLIB_APPLICATION(LanTvClientApp)
public:
	LanTvClientApp();

protected:
	void onStart() override;

};
