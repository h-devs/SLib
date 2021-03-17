#pragma once

#include <slib.h>

using namespace slib;

class MicMonApp : public UIApp
{
	SLIB_APPLICATION(MicMonApp)
public:
	MicMonApp();
	
protected:
	void onStart() override;

private:
	Ref<AudioRecorder> m_recorder;
	
};
