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

#ifndef CHECKHEADER_SLIB_UI_AUDIO_VIEW
#define CHECKHEADER_SLIB_UI_AUDIO_VIEW

#include "view.h"

#include "../media/audio_data.h"
#include "../data/loop_queue.h"

namespace slib
{

	class SLIB_EXPORT AudioView : public View
	{
		SLIB_DECLARE_OBJECT

	public:
		AudioView();

		~AudioView();

	public:
		sl_uint32 getFramesPerPacket();

		void setFramesPerPacket(sl_uint32 n);

		sl_uint32 getPacketsPerWindow();

		void setPacketsPerWindow(sl_uint32 n);

		const Color& getAmplitudeColor();

		void setAmplitudeColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		float getAmplitudeScale();

		void setAmplitudeScale(float scale, UIUpdateMode mode = UIUpdateMode::Redraw);

		void pushFrames(const AudioData& data, UIUpdateMode mode = UIUpdateMode::Redraw);

		void clearFrames(UIUpdateMode mode = UIUpdateMode::Redraw);

	public:
		void onDraw(Canvas* canvas) override;

	protected:
		sl_uint32 m_nFramesPerPacket;
		sl_uint32 m_nPacketsPerWindow;
		Color m_colorAmplitude;
		float m_scaleAmplitude;

		LoopQueue<sl_uint16> m_queuePackets;
		Array<sl_uint16> m_bufProcess;
		Array<sl_uint16> m_bufWindow;
		Array<Point> m_ptsWindow;

	};

}

#endif
