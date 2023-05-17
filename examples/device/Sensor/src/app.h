#pragma once

#include <slib.h>

using namespace slib;

class SensorApp : public MobileApp
{
	SLIB_APPLICATION(SensorApp)
public:
	SensorApp();

protected:
	void onStart() override;

private:
	Ref<Sensor> m_sensor;

};
