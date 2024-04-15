#include "app.h"

SLIB_TEMPLATE_APP_NAMEService::SLIB_TEMPLATE_APP_NAMEService()
{
}

String SLIB_TEMPLATE_APP_NAMEService::getServiceId()
{
	SLIB_RETURN_STRING("SLIB_TEMPLATE_APP_NAME")
}

sl_bool SLIB_TEMPLATE_APP_NAMEService::onStartService()
{
	return sl_true;
}

void SLIB_TEMPLATE_APP_NAMEService::onStopService()
{
}
