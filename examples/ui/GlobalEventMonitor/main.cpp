#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	System::setDebugFlags();
	auto monitor = GlobalEventMonitor::create([](UIEvent* ev) {
		auto action = ev->getAction();
		auto flagInjected = ev->getFlags() & UIEventFlags::Injected;
		auto strInjected = flagInjected ? "Injected" : "";
		switch (action) {
			case UIAction::KeyDown:
				Println("KeyDown: %s %s", UIEvent::getKeyName(ev->getKeycode()), strInjected);
				break;
			case UIAction::KeyUp:
				Println("KeyUp: %s %s", UIEvent::getKeyName(ev->getKeycode()), strInjected);
				break;
			case UIAction::LeftButtonDown:
				Println("LBDown: (%s, %s) %s", ev->getX(), ev->getY(), strInjected);
				break;
			case UIAction::LeftButtonUp:
				Println("LBUp: (%s, %s) %s", ev->getX(), ev->getY(), strInjected);
				break;
			case UIAction::RightButtonDown:
				Println("RBDown: (%s, %s) %s", ev->getX(), ev->getY(), strInjected);
				break;
			case UIAction::RightButtonUp:
				Println("RBUp: (%s, %s) %s", ev->getX(), ev->getY(), strInjected);
				break;
			case UIAction::MiddleButtonDown:
				Println("MBDown: (%s, %s) %s", ev->getX(), ev->getY(), strInjected);
				break;
			case UIAction::MiddleButtonUp:
				Println("MBUp: (%s, %s) %s", ev->getX(), ev->getY(), strInjected);
				break;
			case UIAction::MouseMove:
				Println("Move: (%s, %s) %s", ev->getX(), ev->getY(), strInjected);
				break;
			case UIAction::MouseWheel:
				Println("Wheel: (%s, %s), Delta(%s, %s) %s", ev->getX(), ev->getY(), ev->getDeltaX(), ev->getDeltaY(), strInjected);
				break;
			default:
				break;
		}
	});
	if (!monitor) {
		Println("Failed to create monitor!");
		return -1;
	}
	Println("Press x to exit!");
	for (;;) {
		if (Console::readChar() == 'x') {
			break;
		}
		System::sleep(10);
	}
	return 0;
}
