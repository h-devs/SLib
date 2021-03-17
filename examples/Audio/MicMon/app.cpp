#include "app.h"

MicMonApp::MicMonApp()
{
}

void MicMonApp::onStart()
{
	auto view = New<AudioView>();
	view->setWidthFilling(sl_true);
	view->setHeightFilling(sl_true);

	AudioRecorderParam rp;
	rp.onRecordAudio = [view](AudioRecorder*, AudioData& frame) {
		view->pushFrames(frame);
	};
	m_recorder = AudioRecorder::create(rp);


	Ref<Window> window = new Window;
	window->setTitle("MicMon");
	window->setFrame(100, 100, 600, 400);
	window->setOnClose([](Window* window, UIEvent* ev) {
		UIApp::quit();
	});
	window->addView(view);
	window->show();
	setMainWindow(window);
}
