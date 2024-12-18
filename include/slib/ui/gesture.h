/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_GESTURE
#define CHECKHEADER_SLIB_UI_GESTURE

#include "constants.h"
#include "motion_tracker.h"

#include "../core/object.h"

namespace slib
{

	class GestureEvent
	{
	public:
		GestureType type;
	};

	class View;
	class UIEvent;

	class GestureRecognizer;

	class GestureDetector : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		GestureDetector(const Ref<View>& view);

		~GestureDetector();

	public:
		void enable(GestureType type);

		void enableNative();

		void processEvent(UIEvent* ev);

	private:
		static sl_bool _enableNative(const Ref<View>& view, GestureType type);

	protected:
		WeakRef<View> m_view;
		MotionTracker m_tracker;
		Ref<GestureRecognizer> m_recognizers[(int)(GestureType::Count)];

		friend class GestureRecognizer;
	};

}

#endif
