#pragma once

#include <slib.h>

using namespace slib;

class SLIB_TEMPLATE_APP_NAMEService : public Service
{
	SLIB_APPLICATION(SLIB_TEMPLATE_APP_NAMEService)

public:
	SLIB_TEMPLATE_APP_NAMEService();

protected:
	String getServiceId() override;

	sl_bool onStartService() override;

	void onStopService() override;

};

