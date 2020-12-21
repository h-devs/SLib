#pragma once

#include <slib.h>

using namespace slib;

class LanTvServerApp : public UIApp
{
	SLIB_DECLARE_APPLICATION(LanTvServerApp)
public:
	LanTvServerApp();
	
protected:
	void onStart() override;
	
private:
	void doRunServer();

private:
	Ref<Thread> m_thread;

	Ref<Button> m_btnRun;
	Ref<SelectView> m_viewSelectVideoSource;
	Ref<SelectView> m_viewSelectAudioSource;
};
