#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	System::setDebugFlags();
	GlobalEventMonitor::addMonitor([](UIEvent* ev) {
		UIAction action = ev->getAction();
		if (action == UIAction::KeyDown) {
			Println("Key Pressed: %s", UIEvent::getKeyName(ev->getKeycode()));
		} else if (action == UIAction::LeftButtonDown) {
			Println("Left Mouse Button Pressed: (%s, %s)", ev->getX(), ev->getY());
		}
	});
	Println("Press x to exit!");
	for (;;) {
		if (Console::readChar() == 'x') {
			break;
		}
		System::sleep(10);
	}
	GlobalEventMonitor::removeAllMonitors();
	return 0;
}
