#include "app.h"

#define SAMPLES_PER_SECOND 16000

MicMonApp::MicMonApp()
{
}

void MicMonApp::onStart()
{
	auto audio = New<AudioView>();
	audio->setWidthFilling(1, UIUpdateMode::Init);
	audio->setHeightFilling(1, UIUpdateMode::Init);

	auto line = New<LineView>();
	line->setWidthFilling(1, UIUpdateMode::Init);
	line->setHeightWrapping(UIUpdateMode::Init);

	auto btnRecord = New<Button>();
	btnRecord->setCreatingNativeWidget();
	btnRecord->setWidthWrapping(UIUpdateMode::Init);
	btnRecord->setHeightWrapping(UIUpdateMode::Init);
	btnRecord->setCenterHorizontal(UIUpdateMode::Init);
	btnRecord->setMargin(10, UIUpdateMode::Init);
	btnRecord->setPadding(10, UIUpdateMode::Init);
	btnRecord->setText("Record", UIUpdateMode::Init);

	auto group = New<LinearLayout>();
	group->setWidthFilling(1, UIUpdateMode::Init);
	group->setHeightFilling(1, UIUpdateMode::Init);
	group->addChild(audio, UIUpdateMode::Init);
	group->addChild(line, UIUpdateMode::Init);
	group->addChild(btnRecord, UIUpdateMode::Init);

	AudioRecorderParam rp;
	rp.samplesPerSecond = SAMPLES_PER_SECOND;
	rp.onRecordAudio = [this, audio](AudioRecorder*, AudioData& frame) {
		audio->pushFrames(frame);
		Ref<FileIO> file = m_fileRecording;
		if (file) {
			// 16-bit signed integer (Little Endian)
			file->writeFully(frame.data, frame.getTotalSize());
		}
	};
	m_recorder = AudioRecorder::create(rp);

	btnRecord->setOnClick([this](View* view) {
		Button* btn = (Button*)view;
		if (m_fileRecording.isNotNull()) {
			m_fileRecording.setNull();
			btn->setText("Record");
		} else {
			String path = FileDialog::saveFile(getMainWindow());
			if (path.isNull()) {
				return;
			}
			Ref<FileIO> file = FileIO::openForWrite(path);
			if (file.isNull()) {
				UI::alert(getMainWindow(), AlertIcon::Error, "Error", "Failed to write file!");
				return;
			}
			m_fileRecording = Move(file);
			btn->setText("Stop");
		}
	});

	Ref<Window> window = new Window;
	window->setTitle("MicMon");
	window->setFrame(100, 100, 600, 400);
	window->addView(group);
	window->show();
	setMainWindow(window);
}
