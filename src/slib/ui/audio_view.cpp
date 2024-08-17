/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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
		m_nFramesPerPacket = 50;
		m_colorAmplitude = Color::Blue;
		m_scaleAmplitude = 1.0f;

		setPacketsPerWindow(500);
	}

	AudioView::~AudioView()
	{
	}

	sl_uint32 AudioView::getFramesPerPacket()
	{
		return m_nFramesPerPacket;
	}

	void AudioView::setFramesPerPacket(sl_uint32 n)
	{
		m_nFramesPerPacket = n;
	}

	sl_uint32 AudioView::getPacketsPerWindow()
	{
		return m_nPacketsPerWindow;
	}

	void AudioView::setPacketsPerWindow(sl_uint32 n)
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
		if (m_queuePackets.setQueueSize(n)) {
			m_bufWindow = Move(arr);
			m_ptsWindow = Move(pts);
			m_nPacketsPerWindow = n;
		}
	}

	const Color& AudioView::getAmplitudeColor()
	{
		return m_colorAmplitude;
	}

	void AudioView::setAmplitudeColor(const Color& color, UIUpdateMode mode)
	{
		m_colorAmplitude = color;
		invalidate(mode);
	}

	float AudioView::getAmplitudeScale()
	{
		return m_scaleAmplitude;
	}

	void AudioView::setAmplitudeScale(float scale, UIUpdateMode mode)
	{
		m_scaleAmplitude = scale;
		invalidate(mode);
	}

	void AudioView::pushFrames(const AudioData& data, UIUpdateMode mode)
	{
		sl_uint32 nFramesPerPacket = m_nFramesPerPacket;
		if (!nFramesPerPacket) {
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
		sl_uint32 nPackets = (sl_uint32)(data.count / nFramesPerPacket);
		for (sl_uint32 iFrame = 0; iFrame < nPackets; iFrame++) {
			sl_int32 avg = 0;
			sl_uint32 i;
			for (i = 0; i < nFramesPerPacket; i++) {
				avg += samples[i];
			}
			avg /= (sl_int32)nFramesPerPacket;
			sl_uint32 sum = 0;
			for (i = 0; i < nFramesPerPacket; i++) {
				sum += (sl_uint32)(Math::abs((sl_int32)(samples[i]) - avg));
			}
			sum /= m_nFramesPerPacket;
			if (sum >> 16) {
				sum = 0xffff;
			}
			m_queuePackets.push(sum);
			samples += nFramesPerPacket;
		}

		invalidate(mode);
	}

	void AudioView::clearFrames(UIUpdateMode mode)
	{
		ObjectLocker lock(this);
		m_queuePackets.removeAll();
		invalidate(mode);
	}

	void AudioView::onDraw(Canvas* canvas)
	{
		ObjectLocker lock(this);

		sl_uint32 nPacketsPerWindow = m_nPacketsPerWindow;
		if (!nPacketsPerWindow) {
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

		sl_uint32 nPackets = (sl_uint32)(m_queuePackets.read(window, nPacketsPerWindow));
		sl_uint32 iStart = nPacketsPerWindow - nPackets;
		sl_uint32 iEndPts = (nPacketsPerWindow << 1) - 1;

		UIRect bounds = getBoundsInnerPadding();
		sl_uint32 width = bounds.getWidth();
		sl_uint32 height = bounds.getHeight();
		sl_uint32 h2 = height >> 1;

		{
			for (sl_uint32 i = 0; i < iStart; i++) {
				if (i + 1 == nPacketsPerWindow) {
					pts[i].x = (sl_real)(bounds.left + width);
				} else {
					pts[i].x = (sl_real)(bounds.left + i * width / nPacketsPerWindow);
				}
				pts[i].y = (sl_real)(bounds.top + h2);
				pts[iEndPts - i].x = pts[i].x;
				pts[iEndPts - i].y = pts[i].y + 1;
			}
		}
		{
			float scale = m_scaleAmplitude;
			sl_bool flagScale = !(Math::isAlmostZero(scale - 1.0f));
			for (sl_uint32 i = iStart; i < nPacketsPerWindow; i++) {
				sl_uint16 value = window[i - iStart];
				if (flagScale) {
					sl_int32 s = (sl_int32)(((float)value) * scale);
					value = (sl_uint16)(Math::clamp0_65535(s));
				}
				sl_int32 y = h2 - ((sl_uint32)value) * h2 / 0x10000;
				if (i + 1 == nPacketsPerWindow) {
					pts[i].x = (sl_real)(bounds.left + width);
				} else {
					pts[i].x = (sl_real)(bounds.left + i * width / nPacketsPerWindow);
				}
				pts[i].y = (sl_real)(bounds.top + y);
				pts[iEndPts - i].x = pts[i].x;
				pts[iEndPts - i].y = (sl_real)(bounds.bottom - y);
			}
		}
		canvas->fillPolygon(pts, nPacketsPerWindow << 1, m_colorAmplitude);
	}

}
