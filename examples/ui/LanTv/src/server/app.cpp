#include "app.h"

#include "../common.h"

LanTvServerApp::LanTvServerApp()
{
}

void LanTvServerApp::onStart()
{
	auto table = New<TableLayout>();
	table->setWidthFilling();
	table->setHeightWrapping();
	table->setAlignParentTop();
	table->setAlignParentLeft();
	table->setMargin(10);
	table->setRowCount(3);
	table->setColumnCount(2);
	table->setColumnWidth(0, 100);

	// Row 0
	{
		auto label = New<LabelView>();
		label->setText("Video");
		label->setWidthFilling();
		label->setHeightWrapping();
		label->setCenterVertical();
		label->setGravity(Alignment::Center);
		table->setCell(0, 0, label);

		m_viewSelectVideoSource = New<SelectView>();
		m_viewSelectVideoSource->setWidthFilling();
		m_viewSelectVideoSource->setHeightWrapping();
		m_viewSelectVideoSource->setCenterVertical();
		m_viewSelectVideoSource->setMargin(5);
		List<String> values, titles;
		for (auto& cameraInfo : Camera::getCameras()) {
			values.add(cameraInfo.id);
			titles.add(cameraInfo.name);
		}
		m_viewSelectVideoSource->setItemCount((sl_uint32)(values.getCount()));
		m_viewSelectVideoSource->setValues(values);
		m_viewSelectVideoSource->setTitles(titles);
		table->setCell(0, 1, m_viewSelectVideoSource);
	}

	// Row 1
	{
		auto label = New<LabelView>();
		label->setText("Audio");
		label->setWidthFilling();
		label->setHeightWrapping();
		label->setCenterVertical();
		label->setGravity(Alignment::Center);
		table->setCell(1, 0, label);

		m_viewSelectAudioSource = New<SelectView>();
		m_viewSelectAudioSource->setWidthFilling();
		m_viewSelectAudioSource->setHeightWrapping();
		m_viewSelectAudioSource->setCenterVertical();
		m_viewSelectAudioSource->setMargin(5);
		List<String> values, titles;
		for (auto& audioInfo : AudioRecorder::getDevices()) {
			values.add(audioInfo.id);
			titles.add(audioInfo.name);
		}
		m_viewSelectAudioSource->setItemCount((sl_uint32)(values.getCount()));
		m_viewSelectAudioSource->setValues(values);
		m_viewSelectAudioSource->setTitles(titles);
		table->setCell(1, 1, m_viewSelectAudioSource);
	}

	// Row 2
	{
		m_btnRun = New<Button>();
		m_btnRun->setCreatingNativeWidget();
		m_btnRun->setText("Run");
		m_btnRun->setWidth(100);
		m_btnRun->setHeightWrapping();
		m_btnRun->setPadding(2);
		m_btnRun->setCenterHorizontal();
		table->setCell(2, 0, m_btnRun, 1, 2);
	}

	m_btnRun->setOnClick([this](View*) {
		if (m_thread.isNull()) {
			m_btnRun->setEnabled(sl_false);
			m_thread = Thread::start([this]() {
				m_btnRun->setEnabled();
				m_btnRun->setText("Stop");
				doRunServer();
				m_btnRun->setText("Run");
				m_thread.setNull();
			});
		} else {
			m_thread->finishAndWait();
			m_thread.setNull();
		}
	});

	auto window = New<Window>();
	window->setTitle("LAN TV Server");
	window->setWidth(500);
	window->setHeightWrapping();
	window->setCenterScreen();
	window->setOnDestroy([this](Window*) {
		if (m_thread.isNotNull()) {
			m_thread->finishAndWait();
			m_thread.setNull();
		}
		UIApp::quit();
	});
	window->addView(table);
	window->show();
	setMainWindow(window);
}

void LanTvServerApp::doRunServer()
{
	Shared<Socket> socketAudio = Socket::openUdp();
	Shared<Socket> socketVideo = Socket::openUdp();

	String videoId = m_viewSelectVideoSource->getSelectedValue();
	String audioId = m_viewSelectAudioSource->getSelectedValue();

	Ref<OpusEncoder> encoderAudio;
	{
		OpusEncoderParam param;
		param.type = OpusEncoderType::Music;
		param.bitsPerSecond = AUDIO_BITS_PER_SECOND;
		param.channelCount = 1;
		param.samplesPerSecond = AUDIO_SAMPLES_PER_SECOND;
		encoderAudio = OpusEncoder::create(param);
	}

	Ref<AudioRecorder> recorderAudio;
	{
		Memory packet = Memory::create(PACKET_SIZE);
		AudioRecorderParam param;
		param.deviceId = audioId;
		param.channelCount = 1;
		param.samplesPerSecond = AUDIO_SAMPLES_PER_SECOND;
		param.frameLengthInMilliseconds = AUDIO_FRAME_MS;
		param.bufferLengthInMilliseconds = AUDIO_BUFFER_MS;
		param.onRecordAudio = [socketAudio, encoderAudio, packet](AudioRecorder*, AudioData& data) {
			Memory mem = encoderAudio->encode(data);
			if (mem.isNotNull() && (mem.getSize() < PACKET_CONTENT_SIZE)) {
				sl_uint8* buf = (sl_uint8*)(packet.getData());
				MIO::writeUint64LE(buf, Time::now().toInt());
				Base::copyMemory(buf + 8, mem.getData(), mem.getSize());
				socketAudio->sendTo(SocketAddress(IPv4Address(MULTICAST_ADDR), UDP_PORT_AUDIO), buf, (sl_uint32)(8 + mem.getSize()));
			}
		};
		recorderAudio = AudioRecorder::create(param);
		if (recorderAudio.isNull()) {
			return;
		}
	}

	Ref<VpxEncoder> encoderVideo;
	Ref<Camera> camera;
	{
		Memory packet = Memory::create(PACKET_SIZE);
		CameraParam param;
		param.deviceId = videoId;
		param.onCaptureVideoFrame = [socketVideo, packet, &encoderVideo](VideoCapture*, VideoCaptureFrame& frame) {
			sl_uint32 width = frame.image.width;
			sl_uint32 height = frame.image.height;
			if (encoderVideo.isNull()) {
				VpxEncoderParam param;
				param.width = width;
				param.height = height;
				param.bitrate = VIDEO_BITS_PER_SECOND / 1000;
				encoderVideo = VpxEncoder::create(param);
				if (encoderVideo.isNull()) {
					return;
				}
			}
			Memory mem = encoderVideo->encode(frame);
			if (mem.isNotNull() && mem.getSize() < PACKET_CONTENT_SIZE) {
				sl_uint8* buf = (sl_uint8*)(packet.getData());
				MIO::writeUint16LE(buf, width);
				MIO::writeUint16LE(buf + 2, height);
				Base::copyMemory(buf + 8, mem.getData(), mem.getSize());
				socketVideo->sendTo(SocketAddress(IPv4Address(MULTICAST_ADDR), UDP_PORT_VIDEO), buf, (sl_uint32)(8 + mem.getSize()));
			}
		};
		camera = Camera::create(param);
		if (camera.isNull()) {
			return;
		}

	}

	socketAudio->setNonBlockingMode(sl_true);
	socketAudio->bind(SocketAddress(UDP_PORT_AUDIO + 2));
	socketAudio->setOption_Broadcast(sl_true);
	socketAudio->setOption_IpAddMembership(IPv4Address(MULTICAST_ADDR), IPv4Address::Any);
	socketAudio->setOption_IpMulticastLoop(sl_true);
	socketAudio->setOption_SendBufferSize(PACKET_SIZE);
	socketAudio->setOption_ReceiveBufferSize(PACKET_SIZE);

	socketVideo->setNonBlockingMode(sl_true);
	socketVideo->bind(SocketAddress(UDP_PORT_VIDEO + 2));
	socketVideo->setOption_Broadcast(sl_true);
	socketAudio->setOption_IpAddMembership(IPv4Address(MULTICAST_ADDR), IPv4Address::Any);
	socketAudio->setOption_IpMulticastLoop(sl_true);
	socketVideo->setOption_SendBufferSize(PACKET_SIZE);
	socketVideo->setOption_ReceiveBufferSize(PACKET_SIZE);

	while (Thread::isNotStoppingCurrent()) {
		Thread::sleep(1000);
	}
}
