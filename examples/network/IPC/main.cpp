#include <slib.h>

using namespace slib;

Ref<Thread> Run(const String& name, const String& target)
{
	IPC::ServerParam param;
	param.name = name;
	param.onReceiveMessage = [name](IPC::RequestMessage& request, IPC::ResponseMessage& response) {
		Println("%s received: %s", name, StringView((char*)(request.data), request.size));
		response.setString(String::format("%s %s", name, Time::now()));
	};
	auto ipc = IPC::createServer(param);
	if (ipc.isNull()) {
		Println("Failed to create IPC instance: %s", name);
		return sl_null;
	}
	return Thread::start([ipc, name, target]() {
		sl_uint32 index = 1;
		while (Thread::isNotStoppingCurrent()) {
			String msg = String::format("Request from %s: %d", name, index++);
			IPC::sendMessage(target, msg, [name](IPC::ResponseMessage& response) {
				if (response.size) {
					Println("Response to %s: %s", name, StringView((char*)(response.data), response.size));
				}
			});
			Thread::sleep(1000);
		}
	});
}

int main(int argc, const char * argv[])
{
	System::setDebugFlags();
	Ref<Thread> thread;
	if (argc == 2 && StringView(argv[1]) == "child") {
		Console::open();
		thread = Run("child", "parent");
	} else {
		Process::run(argv[0], "child");
		thread = Run("parent", "child");
	}
	if (thread.isNull()) {
		return -1;
	}
	Println("Press x to exit!");
	for (;;) {
		if (Console::readChar() == 'x') {
			break;
		}
		System::sleep(10);
	}
	thread->finishAndWait();
	return 0;
}
