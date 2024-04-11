#include "app.h"

#include <input_sender/input_sender.h>

InputSenderApp::InputSenderApp()
{
}

void InputSenderApp::onStart()
{
	auto window = New<Window>();
	window->setTitle("InputSender");
	window->setFrame(100, 100, 400, 300);
	window->show();
	setMainWindow(window);

	m_timerSendInput = Timer::start([](Timer*) {
		InputSender::sendKeyEvent(UIAction::KeyDown, Keycode::Right);
		InputSender::sendMouseEvent(UIAction::MouseMove, 10, 10, sl_false);
	}, 1000);
}
