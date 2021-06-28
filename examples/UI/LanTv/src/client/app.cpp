#include "app.h"

#include "../common.h"

LanTvClientApp::LanTvClientApp()
{
}

void LanTvClientApp::onStart()
{
	auto viewVideo = New<VideoView>();
	viewVideo->setWidthFilling();
	viewVideo->setHeightFilling();
	viewVideo->setOnMouseEvent([](View* view, UIEvent* ev) {
		static UIRect lastFrame = {};
		if (ev->getAction() == UIAction::LeftButtonDoubleClick) {
			auto window = view->getWindow();
			if (window->isMaximized()) {
				window->setFrame(lastFrame);
				window->setFullScreen(sl_false);
				window->setMaximized(sl_false);
			} else {
				lastFrame = window->getFrame();
				window->setFullScreen();
				window->setMaximized(sl_true);
			}
			window->forceCreate();
		}
	});

	Ref<Thread> threadAudio = Thread::start([]() {

		Ref<AudioPlayer> audioPlayer;
		{
			AudioPlayerParam param;
			param.channelsCount = 1;
			param.samplesPerSecond = AUDIO_SAMPLES_PER_SECOND;
			param.frameLengthInMilliseconds = AUDIO_FRAME_MS;
			param.maxBufferLengthInMilliseconds = AUDIO_BUFFER_MS;
			param.flagAutoStart = sl_true;
			audioPlayer = AudioPlayer::create(param);
			if (audioPlayer.isNull()) {
				return;
			}
		}
		
		Ref<OpusDecoder> decoderAudio;
		{
			OpusDecoderParam param;
			param.channelsCount = 1;
			param.samplesPerSecond = AUDIO_SAMPLES_PER_SECOND;
			decoderAudio = OpusDecoder::create(param);
		}

		Socket socket = Socket::openUdp();

		socket.setNonBlockingMode(sl_true);
		socket.bind(SocketAddress(UDP_PORT_AUDIO));
		socket.setOption_Broadcast(sl_true);
		socket.setOption_IpAddMembership(IPv4Address(MULTICAST_ADDR), IPv4Address::Any);
		socket.setOption_IpMulticastLoop(sl_true);
		socket.setOption_SendBufferSize(PACKET_SIZE);
		socket.setOption_ReceiveBufferSize(PACKET_SIZE);

		auto event = SocketEvent::createRead(socket);

		Memory mem = Memory::create(PACKET_SIZE);
		sl_uint8* buf = (sl_uint8*)(mem.getData());

		SocketAddress addr;

		while (Thread::isNotStoppingCurrent()) {
			sl_int32 nRead = socket.receiveFrom(addr, buf, PACKET_SIZE);
			if (nRead > 8) {
				sl_int16 audioSamples[AUDIO_SAMPLES_PER_SECOND * AUDIO_FRAME_MS / 1000];
				AudioData audioOutput;
				audioOutput.count = sizeof(audioSamples) / sizeof(sl_int16);
				audioOutput.format = AudioFormat::Int16_Mono;
				audioOutput.data = audioSamples;
				sl_uint32 n = decoderAudio->decode(buf + 8, nRead - 8, audioOutput);
				if (n) {
					AudioData data = audioOutput;
					data.count = n;
					audioPlayer->write(data);
				}
			}
			event.wait();
		}

	});

	Ref<Thread> threadVideo = Thread::start([viewVideo]() {

		Ref<VpxDecoder> decoderVideo;
		sl_uint32 width = 0;
		sl_uint32 height = 0;

		Socket socket = Socket::openUdp();

		socket.setNonBlockingMode(sl_true);
		socket.bind(SocketAddress(UDP_PORT_VIDEO));
		socket.setOption_Broadcast(sl_true);
		socket.setOption_IpAddMembership(IPv4Address(MULTICAST_ADDR), IPv4Address::Any);
		socket.setOption_IpMulticastLoop(sl_true);
		socket.setOption_SendBufferSize(PACKET_SIZE);
		socket.setOption_ReceiveBufferSize(PACKET_SIZE);

		auto event = SocketEvent::createRead(socket);

		Memory mem = Memory::create(PACKET_SIZE);
		sl_uint8* buf = (sl_uint8*)(mem.getData());

		SocketAddress addr;

		while (Thread::isNotStoppingCurrent()) {
			sl_int32 nRead = socket.receiveFrom(addr, buf, PACKET_SIZE);
			if (nRead > 8) {
				sl_uint32 w = MIO::readUint16LE(buf);
				sl_uint32 h = MIO::readUint16LE(buf + 2);
				if (decoderVideo.isNull() || w != width || h != height) {
					VpxDecoderParam param;
					param.width = w;
					param.height = h;
					decoderVideo = VpxDecoder::create(param);
					if (decoderVideo.isNotNull()) {
						width = w;
						height = h;
					}
				}
				if (decoderVideo.isNotNull()) {
					decoderVideo->decode(buf + 8, nRead - 8, [viewVideo](VideoFrame& frame) {
						viewVideo->updateCurrentFrame(frame);
					});
				}
			}
			event.wait();
		}

	});

	Ref<Window> window = new Window;
	window->setTitle("LanTvClient");
	window->setFrame(100, 100, 400, 300);
	window->setOnDestroy([threadAudio, threadVideo](Window*) {
		threadAudio->finishAndWait();
		threadVideo->finishAndWait();
		UIApp::quit();
	});
	window->setResizable();
	window->setMaximizeButtonEnabled();
	window->setMinimizeButtonEnabled();
	window->addView(viewVideo);
	window->show();
	setMainWindow(window);
}
