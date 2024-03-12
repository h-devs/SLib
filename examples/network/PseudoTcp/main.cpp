#include <slib.h>

#include <pseudo_tcp_message.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	System::setDebugFlags();

	auto msg1 = New<PseudoTcpMessage>();
	auto msg2 = New<PseudoTcpMessage>();

	StringView text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

	auto onRequest = [](sl_uint8* data, sl_size size, MemoryOutput* output) {
		Println("Request: %s", StringView((char*)data, size));
		output->writeAllBytes("success");
	};

	auto onResponse = [](sl_uint8* data, sl_size size) {
		Println("Response: %s", StringView((char*)data, size));
	};

	auto wmsg1 = ToWeakRef(msg1);
	auto onProcessPacket = [wmsg1, msg2, onRequest](sl_uint8* data, sl_size size) {
		msg2->notifyPacketForListeningMessage("test", data, size, onRequest, [wmsg1](sl_uint8* data, sl_uint32 size) {
			auto msg1 = ToRef(wmsg1);
			if (msg1.isNotNull()) {
				msg1->notifyPacketForSendingMessage(data, size);
			}
		});
	};

	auto thread = Thread::start([=]() {
		while (Thread::isNotStoppingCurrent()) {
			msg1->sendMessage(text.getData(), text.getLength(), onResponse, onProcessPacket, 5000);
			Thread::sleep(3000);
		}
	});

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
