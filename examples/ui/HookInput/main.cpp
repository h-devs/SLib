#include <slib.h>
#include <slib/platform/win32/windows.h>

#include <winhook.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	System::setDebugFlags();

	HookInput::setDllName("hook_input/hook");

	if (!(HookInput::install())) {
		Println("Failed to install hook dll!");
		return -1;
	}

	sl_bool bExit = sl_false;

	HookInputParam param;
	param.onInput = [&bExit](UIEvent* ev) {
		auto action = ev->getAction();
		auto flagInjected = ev->getFlags() & UIEventFlags::Injected;
		auto strInjected = flagInjected ? "Injected" : "";
		switch (action) {
			case UIAction::KeyDown:
				{
					auto key = ev->getKeycode();
					Println("KeyDown: %s %s", UIEvent::getKeyName(key), strInjected);
					if (key == Keycode::X) {
						bExit = sl_true;
						break;
					}
				}
				break;
			case UIAction::KeyUp:
				Println("KeyUp: %s %s", UIEvent::getKeyName(ev->getKeycode()), ev->getKeycode());
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
	};
	param.flagBlockKeyboard = sl_true;
	
	if (!(HookInput::start(param))) {
		Println("Failed to start hook!");
		return -1;
	}

	Println("Press x to exit.");
	while (!bExit) {
		Thread::sleep(100);
	}

	HookInput::stop();

	return 0;
}
