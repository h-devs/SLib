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

#ifndef CHECKHEADER_SLIB_UI_SWITCH_VIEW
#define CHECKHEADER_SLIB_UI_SWITCH_VIEW

#include "view.h"
#include "view_state_map.h"
#include "motion_tracker.h"

#include "../core/string.h"

namespace slib
{
	enum class SwitchValue
	{
		Off = 0, // Left
		On = 1 // Right
	};

	class SLIB_EXPORT SwitchView : public View
	{
		SLIB_DECLARE_OBJECT

	public:
		SwitchView();

		~SwitchView();

	public:
		SwitchValue getValue();

		virtual void setValue(SwitchValue value, UIUpdateMode mode = UIUpdateMode::Animate);


		sl_bool isTextInButton();

		void setTextInButton(sl_bool flag, UIUpdateMode mode = UIUpdateMode::Redraw);


		String getText(SwitchValue value);

		void setText(SwitchValue value, const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);


		Color getTextColor(SwitchValue value);

		void setTextColor(SwitchValue value, const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTextColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);


		Ref<Drawable> getThumb(SwitchValue value, ViewState state = ViewState::Default);

		void setThumb(const Ref<Drawable>& drawable, SwitchValue value, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setThumb(const Ref<Drawable>& drawable, SwitchValue value, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setThumb(const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setThumb(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setThumbColor(const Color& color, SwitchValue value, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setThumbColor(const Color& color, SwitchValue value, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setThumbColor(const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setThumbColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);


		Ref<Drawable> getTrack(SwitchValue value, ViewState state = ViewState::Default);

		void setTrack(const Ref<Drawable>& drawable, SwitchValue value, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTrack(const Ref<Drawable>& drawable, SwitchValue value, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTrack(const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTrack(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTrackColor(const Color& color, SwitchValue value, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTrackColor(const Color& color, SwitchValue value, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTrackColor(const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTrackColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

	public:
		SLIB_DECLARE_EVENT_HANDLER(SwitchView, Change, SwitchValue newValue)

	protected:
		void onDraw(Canvas* canvas) override;

		void onUpdateLayout() override;

		void onMouseEvent(UIEvent* ev) override;

	protected:
		virtual sl_bool calculateSwitchRegion(UIRect& _out);

		virtual void drawTrack(Canvas* canvas, const Ref<Drawable>& track, const Rectangle& rectDst);

		virtual void drawThumb(Canvas* canvas, const Ref<Drawable>& thumb, const Rectangle& rectDst);

	private:
		void _changeValue(SwitchValue value);

		void _onTimerAnimation(Timer* timer);

	protected:
		SwitchValue m_value;
		sl_bool m_flagTextInButton;

		AtomicString m_texts[2];
		Color m_textColors[2];
		ViewStateMap< Ref<Drawable> > m_thumbs[2];
		ViewStateMap< Ref<Drawable> > m_tracks[2];

		sl_real m_posThumb;
		Ref<Timer> m_timer;
		MotionTracker m_tracker;
		Point m_ptMouseDown;
		Time m_timeMouseDown;
		sl_real m_posMouseDown;
		sl_bool m_flagTapping;

	};

}

#endif
