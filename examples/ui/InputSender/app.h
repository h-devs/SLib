#pragma once

#include <slib.h>

using namespace slib;

class InputSenderApp : public UIApp
{
	SLIB_APPLICATION(InputSenderApp)
public:
	InputSenderApp();

protected:
	void onStart() override;

private:
	Ref<Timer> m_timerSendInput;

};
