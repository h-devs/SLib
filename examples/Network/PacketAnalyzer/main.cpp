#include <slib.h>

#include <slib/network/packet_analyzer.h>
#include <slib/network/pcap.h>

using namespace slib;

PacketAnalyzer analyzer;

int main(int argc, const char * argv[])
{
	System::setDebugFlags();

	analyzer.setLogging();
	//analyzer.setUdpEnabled();
	//analyzer.setTcpEnabled();
	analyzer.setAnalyzingHttp();
	analyzer.setAnalyzingHttps();
	analyzer.setIgnoringLocalPackets();

	PcapParam param;
	param.onCapturePacket = [](NetCapture* capture, NetCapturePacket& input) {
		auto link = capture->getLinkType();
		if (capture->getLinkType() == NetworkLinkDeviceType::Ethernet) {
			analyzer.putEthernet(input.data, input.length, sl_null);
		} else if (link == NetworkLinkDeviceType::Raw) {
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
