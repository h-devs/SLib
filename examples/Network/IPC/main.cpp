#include <slib.h>

using namespace slib;

void Run(const String& name, const String& target)
{
	IPCParam param;
	param.name = name;
	param.onReceiveMessage = [name](sl_uint8* data, sl_uint32 size, MemoryOutput* output) {
		Println("%s received: %s", name, StringView((char*)data, size));
		output->write(String::format("%s %s", name, Time::now()).toMemory());
	};
	auto ipc = IPC::createDomainSocket(param);
	if (ipc.isNull()) {
		Println("Failed to create IPC instance: %s", name);
		return;
	}
	Thread::start([ipc, name, target]() {
		sl_uint32 index = 1;
		for (;;) {
			String msg = String::format("Request from %s: %d", name, index++);
			ipc->sendMessage(target, msg.toMemory(), [name](sl_uint8* data, sl_uint32 size) {
				Println("Response to %s: %s", name, StringView((char*)data, size));
			});
			Thread::sleep(1000);
		}
	});
}

int main(int argc, const char * argv[])
{
	if (argc == 1 && StringView(argv[1]) == "child") {
		Run("child", "parent");
	} else {
		Process::exec(argv[0], "child");
		Run("parent", "child");
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
