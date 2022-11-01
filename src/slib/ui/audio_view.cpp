/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#include "slib/ui/audio_view.h"

#include "slib/graphics/canvas.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(AudioView, View)

	AudioView::AudioView()
	{
		m_nSamplesPerFrame = 50;
		m_color = Color::Blue;

		setFramesPerWindow(500);
	}

	AudioView::~AudioView()
	{
	}

	sl_uint32 AudioView::getSamplesPerFrame()
	{
		return m_nSamplesPerFrame;
	}

	void AudioView::setSamplesPerFrame(sl_uint32 n)
	{
		m_nSamplesPerFrame = n;
	}

	sl_uint32 AudioView::getFramesPerWindow()
	{
		return m_nFramesPerWindow;
	}

	void AudioView::setFramesPerWindow(sl_uint32 n)
	{
		Array<sl_uint16> arr = Array<sl_uint16>::create(n);
		if (arr.isNull()) {
			return;
		}
		Array<Point> pts = Array<Point>::create(n << 1);
		if (pts.isNull()) {
			return;
		}
		ObjectLocker lock(this);
		if (m_queueFrames.setQueueSize(n)) {
			m_bufWindow = Move(arr);
			m_ptsWindow = Move(pts);
			m_nFramesPerWindow = n;
		}
	}

	const Color& AudioView::getColor()
	{
		return m_color;
	}

	void AudioView::setColor(const Color& color, UIUpdateMode mode)
	{
		m_color = color;
		invalidate(mode);
	}

	void AudioView::pushFrames(const AudioData& data, UIUpdateMode mode)
	{
		sl_uint32 nSamplesPerFrame = m_nSamplesPerFrame;
		if (!nSamplesPerFrame) {
			return;
		}
		if (!(data.count)) {
			return;
		}

		ObjectLocker lock(this);
		if (data.count > m_bufProcess.getCount()) {
			m_bufProcess = Array<sl_uint16>::create(((data.count - 1) | 127) + 1);
			if (m_bufProcess.isNull()) {
				return;
			}
		}

		AudioData dataProcess;
		dataProcess.format = AudioFormat::Int16_Mono;
		dataProcess.data = m_bufProcess.getData();
		dataProcess.count = data.count;

		dataProcess.copySamplesFrom(data);

		sl_int16* samples = (sl_int16*)(dataProcess.data);
		sl_uint32 nFrames = (sl_uint32)(data.count / nSamplesPerFrame);
		for (sl_uint32 iFrame = 0; iFrame < nFrames; iFrame++) {
			sl_uint32 sum = 0;
			for (sl_uint32 i = 0; i < nSamplesPerFrame; i++) {
				sum += (sl_uint32)(Math::abs(samples[i]));
			}
			m_queueFrames.push((sl_uint16)(sum / nSamplesPerFrame));
			samples += nSamplesPerFrame;
		}

		invalidate(mode);
	}

	void AudioView::onDraw(Canvas* canvas)
	{
		ObjectLocker lock(this);

		sl_uint32 nFramesPerWindow = m_nFramesPerWindow;
		if (!nFramesPerWindow) {
			return;
		}
		Array<sl_uint16> bufWindow = m_bufWindow;
		if (bufWindow.isNull()) {
			return;
		}
		Array<Point> ptsWindow = m_ptsWindow;
		if (bufWindow.isNull()) {
			return;
		}

		sl_uint16* window = bufWindow.getData();
		Point* pts = ptsWindow.getData();

		sl_uint32 nFrames = (sl_uint32)(m_queueFrames.read(window, nFramesPerWindow));
		sl_uint32 iStart = nFramesPerWindow - nFrames;
		sl_uint32 iEndPts = (nFramesPerWindow << 1) - 1;

		UIRect bounds = getBoundsInnerPadding();
		sl_uint32 width = bounds.getWidth();
		sl_uint32 height = bounds.getHeight();
		sl_uint32 h2 = height >> 1;

		{
			for (sl_uint32 i = 0; i < iStart; i++) {
				pts[i].x = (sl_real)(bounds.left + i * width / nFramesPerWindow);
				pts[i].y = (sl_real)(bounds.top + h2);
				pts[iEndPts - i].x = pts[i].x;
				pts[iEndPts - i].y = pts[i].y + 1;
			}
		}
		{
			for (sl_uint32 i = iStart; i < nFramesPerWindow; i++) {
				sl_int32 y = h2 - (sl_uint32)(window[i - iStart]) * height / 0x10000;
				pts[i].x = (sl_real)(bounds.left + i * width / nFramesPerWindow);
				pts[i].y = (sl_real)(bounds.top + y);
				pts[iEndPts - i].x = pts[i].x;
				pts[iEndPts - i].y = (sl_real)(bounds.top + ((h2 << 1) + 1) - y);
			}
		}
		canvas->fillPolygon(pts, nFramesPerWindow << 1, m_color);
	}

}
