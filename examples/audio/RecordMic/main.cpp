#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	if (argc == 1) {
		Println("Usage: RecordMic DeviceIndex");
		Println("Devices:");
		int index = 1;
		for (auto&& dev : AudioRecorder::getDevices()) {
			Println("[%s] %s", index, dev.name);
			index++;
		}
		return 0;
	}

	int index = StringView(argv[1]).parseUint32();
	auto devInfo = AudioRecorder::getDevices().getValueAt(index - 1);
	if (devInfo.id) {
		Println("Selected Device: %s", devInfo.name);
	} else {
		Println("Device is not found at index: %s", index);
		return -1;
	}

	AudioRecorderParam param;
	param.deviceId = devInfo.id;
	param.channelCount = 1;
	param.samplesPerSecond = 8000; // 8kHz
	param.framesPerCallback = 256;
	param.flagAutoStart = sl_false;

	// Save recorded samples to D drive (D:\1.pcm)
	param.onRecordAudio = [](AudioRecorder*, AudioData& data) {
		short* samples = (short*)(data.data);
		int n = (int)(data.count);
		File::appendAllBytes("D:\\1.pcm", samples, n * sizeof(short));
	};

	auto recorder = AudioRecorder::create(param);
	if (!recorder) {
		Println("Failed to start recorder");
		return -1;
	}
	recorder->start();

	Println("Press x to exit!");
	for (;;) {
		if (Console::readChar() == 'x') {
			break;
		}
		System::sleep(10);
	}
	return 0;
}
