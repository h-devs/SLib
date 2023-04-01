/*
 *   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_PROGRESS_BAR
#define CHECKHEADER_SLIB_UI_PROGRESS_BAR

#include "view.h"

namespace slib
{

	class SLIB_EXPORT ProgressBar : public View
	{
		SLIB_DECLARE_OBJECT

	public:
		ProgressBar(LayoutOrientation orientation = LayoutOrientation::Horizontal);

		~ProgressBar();

	public:
		LayoutOrientation getOrientation();

		void setOrientation(LayoutOrientation orientation, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isVertical();

		void setVertical(UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isHorizontal();

		void setHorizontal(UIUpdateMode mode = UIUpdateMode::Redraw);

		float getMinimumValue();

		void setMinimumValue(float value, UIUpdateMode mode = UIUpdateMode::Redraw);

		float getMaximumValue();

		void setMaximumValue(float value, UIUpdateMode mode = UIUpdateMode::Redraw);

		float getValue();

		virtual void setValue(float value, UIUpdateMode mode = UIUpdateMode::Redraw);

		float getSecondaryValue();

		virtual void setSecondaryValue(float value, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isDualValues();

		void setDualValues(sl_bool flagDualValues, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isDiscrete();

		void setDiscrete(sl_bool flagDiscrete, UIUpdateMode mode = UIUpdateMode::Redraw);

		float getStep();

		void setStep(float step, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isReversed();

		void setReversed(sl_bool flagReversed, UIUpdateMode mode = UIUpdateMode::Redraw);


		Ref<Drawable> getTrack();

		void setTrack(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTrackColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getProgress();

		void setProgress(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setProgressColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getSecondaryProgress();

		void setSecondaryProgress(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setSecondaryProgressColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

	public:
		void onDraw(Canvas* canvas) override;

	protected:
		void onUpdateLayout() override;

	protected:
		float refineValue(float value);

		float refineStep();

		sl_ui_pos getPositionFromValue(float value);

		void getProgressRegions(UIRect& outProgress, UIRect& outSecondaryProgress);

	protected:
		LayoutOrientation m_orientation;
		float m_value_min;
		float m_value_max;
		float m_value;
		float m_value2;
		sl_bool m_flagDualValues;
		sl_bool m_flagDiscrete;
		float m_step;
		sl_bool m_flagReversed;

		AtomicRef<Drawable> m_track;
		AtomicRef<Drawable> m_progress;
		AtomicRef<Drawable> m_progress2;

	};

}

#endif
