#include <slib.h>

#include <slib/network/pcap.h>
#include <packet_analyzer/packet_analyzer.h>

using namespace slib;

PacketAnalyzer analyzer;

int main(int argc, const char * argv[])
{
	System::setDebugFlags();

	analyzer.setLogging();
	analyzer.setUdpEnabled();
	analyzer.setTcpEnabled();

	PcapParam param;
	param.onCapturePacket = [](NetCapture* capture, NetCapturePacket& input) {
		auto type = capture->getType();
		if (type == NetworkCaptureType::Ethernet) {
			analyzer.putEthernet(input.data, input.length, sl_null);
		} else if (type == NetworkCaptureType::Raw) {
			analyzer.putIP(input.data, input.length, sl_null);
		}
	};
	auto pcap = Pcap::createAny(param);
	for (;;) {
		if (Console::readChar() == 'x') {
			break;
		}
		System::sleep(10);
	}
	return 0;
}
