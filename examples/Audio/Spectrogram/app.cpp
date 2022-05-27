#include "app.h"

#define BITMAP_WIDTH 1024
#define BITMAP_HEIGHT 256
#define FRAME_SIZE (BITMAP_HEIGHT*4)
#define SAMPLE_STEP (FRAME_SIZE/16)

SpectrogramApp::SpectrogramApp(): m_fft(FRAME_SIZE)
{
	m_bitmapAudio = Bitmap::create(BITMAP_WIDTH, BITMAP_HEIGHT);
	m_bitmapAudio->resetPixels(Color::Black);
	m_xBitmap = 0;
}

void SpectrogramApp::onStart()
{
	AudioRecorderParam rp;
	rp.samplesPerSecond = 16000;
	rp.frameLengthInMilliseconds = 32;
	rp.bufferLengthInMilliseconds = 1600;
	m_recorder = AudioRecorder::create(rp);
	
	auto window = New<Window>();
	window->setTitle("Spectrogram");
	window->setCenterScreen();
	window->setClientSize(BITMAP_WIDTH, BITMAP_HEIGHT);
	window->setResizable();
	window->setMaximizeButtonEnabled();
	window->setMinimizeButtonEnabled();
	window->setOnClose([this](Window* window, UIEvent* ev) {
		m_threadProcess->finishAndWait();
		UIApp::quit();
	});

	m_view = window->getContentView();
	m_view->setDoubleBuffer(sl_false);
	m_view->setOpaque();
	m_view->setOnDraw([this](View* view, Canvas* canvas) {
		onDraw(canvas, view->getWidth(), view->getHeight());
	});

	window->show();
	setMainWindow(window);

	m_threadProcess = Thread::start([this]() {
		runProcessAudio();
	});

}

void SpectrogramApp::runProcessAudio()
{
	float buf[FRAME_SIZE] = { 0 };

	while (Thread::isNotStoppingCurrent()) {

		float a[FRAME_SIZE];
		AudioData data;
		data.data = a;
		data.count = FRAME_SIZE;
		data.format = AudioFormat::Float_Mono;
		TimeCounter t;
		if (m_recorder->read(data)) {
			for (sl_uint32 i = 0; i < FRAME_SIZE; i += SAMPLE_STEP) {
				Base::moveMemory(buf, buf + SAMPLE_STEP, (FRAME_SIZE - SAMPLE_STEP) * sizeof(float));
				Base::copyMemory(buf + (FRAME_SIZE - SAMPLE_STEP), a + i, SAMPLE_STEP * sizeof(float));
				onProcessAudio(buf);
			}
		} else {
			m_view->invalidate();
			Thread::sleep(1);
		}
	}
}

void SpectrogramApp::onProcessAudio(float* a)
{
	Complex c[FRAME_SIZE];
	{
		for (sl_uint32 i = 0; i < FRAME_SIZE; i++) {
			c[i].real = a[i];
			// applying hamming window
			float w = 0.54f - 0.46f * Math::cos(SLIB_PI_DUAL * (float)i / (float)(FRAME_SIZE - 1));
			c[i].real *= w;
			c[i].imag = 0;
		}
	}
	m_fft.transform(c);

	sl_uint8 colors[BITMAP_HEIGHT];
	{
		for (sl_uint32 i = 0; i < BITMAP_HEIGHT; i++) {
			colors[i] = (sl_uint8)(Math::clamp0_255((int)(c[i].abs() * 20000)));
		}
	}

	BitmapData bd;
	bd.width = bd.pitch = 1;
	bd.height = BITMAP_HEIGHT;
	bd.data = colors;
	bd.format = BitmapFormat::Gray8;

	{
		ObjectLocker lock(this);
		m_bitmapAudio->writePixels(m_xBitmap, 0, bd);
		m_xBitmap++;
		if (m_xBitmap >= BITMAP_WIDTH) {
			m_xBitmap = 0;
		}
	}

}

void SpectrogramApp::onDraw(Canvas* canvas, sl_ui_len width, sl_ui_len height)
{
	ObjectLocker lock(this);
	sl_uint32 x = m_xBitmap;
	canvas->draw(
		Rectanglei(0, 0, width * (BITMAP_WIDTH - x) / BITMAP_WIDTH, height),
		m_bitmapAudio,
		Rectanglei(x, 0, BITMAP_WIDTH, BITMAP_HEIGHT)
		);
	canvas->draw(
		Rectanglei(width * (BITMAP_WIDTH - x) / BITMAP_WIDTH, 0, width, height),
		m_bitmapAudio,
		Rectanglei(0, 0, x, BITMAP_HEIGHT)
	);
}
