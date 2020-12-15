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

#ifndef CHECKHEADER_SLIB_UI_AUDIO_VIEW
#define CHECKHEADER_SLIB_UI_AUDIO_VIEW

#include "definition.h"

#include "view.h"

#include "../media/audio_data.h"
#include "../core/loop_queue.h"

namespace slib
{

	class SLIB_EXPORT AudioView : public View
	{
		SLIB_DECLARE_OBJECT
		
	public:
		AudioView();
		
		~AudioView();

	public:
		sl_uint32 getSamplesPerFrame();

		void setSamplesPerFrame(sl_uint32 n);

		sl_uint32 getFramesPerWindow();

		void setFramesPerWindow(sl_uint32 n);

		const Color& getColor();

		void setColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		void pushFrames(const AudioData& data, UIUpdateMode mode = UIUpdateMode::Redraw);

	protected:
		void onDraw(Canvas* canvas) override;

	protected:
		sl_uint32 m_nSamplesPerFrame;
		sl_uint32 m_nFramesPerWindow;

		Color m_color;

		LoopQueue<sl_uint16> m_queueFrames;
		Array<sl_uint16> m_bufProcess;
		Array<sl_uint16> m_bufWindow;
		Array<Point> m_ptsWindow;
		
	};

}

#endif
