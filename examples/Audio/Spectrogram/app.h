#pragma once

#include <slib.h>

using namespace slib;

class SpectrogramApp : public UIApp
{
	SLIB_APPLICATION(SpectrogramApp)
public:
	SpectrogramApp();
	
protected:
	void onStart() override;

private:
	void runProcessAudio();

	void onProcessAudio(float* a);

	void onDraw(Canvas* canvas, sl_ui_len width, sl_ui_len height);

private:
	Ref<AudioRecorder> m_recorder;
	Ref<View> m_view;
	Ref<Thread> m_threadProcess;
	
	FFT m_fft;
	Ref<Bitmap> m_bitmapAudio;
	sl_uint32 m_xBitmap;

};
