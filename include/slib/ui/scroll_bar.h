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

#ifndef CHECKHEADER_SLIB_UI_SCROLL_BAR
#define CHECKHEADER_SLIB_UI_SCROLL_BAR

#include "view.h"

#include "view_state_map.h"

namespace slib
{

	class SLIB_EXPORT ScrollBar : public View
	{
		SLIB_DECLARE_OBJECT

	public:
		ScrollBar(LayoutOrientation orientation = LayoutOrientation::Horizontal);

		~ScrollBar();

	public:
		LayoutOrientation getOrientation();

		void setOrientation(LayoutOrientation orientation, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isVertical();

		void setVertical(UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isHorizontal();

		void setHorizontal(UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_scroll_pos getValue();

		virtual void setValue(sl_scroll_pos value, UIUpdateMode mode = UIUpdateMode::Redraw);

		virtual void setValueOfOutRange(sl_scroll_pos value, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_scroll_pos getPage();

		void setPage(sl_scroll_pos page, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_scroll_pos getLine();

		void setLine(sl_scroll_pos line, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_scroll_pos getMinimumValue();

		void setMinimumValue(sl_scroll_pos value, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_scroll_pos getMaximumValue();

		void setMaximumValue(sl_scroll_pos value, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_scroll_pos getRange();

		void setRange(sl_scroll_pos range, UIUpdateMode mode = UIUpdateMode::Redraw);


		Ref<Drawable> getThumb(ViewState state = ViewState::Default);

		void setThumb(const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setThumb(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setThumbColor(const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setThumbColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);


		Ref<Drawable> getTrack(ViewState state = ViewState::Default);

		void setTrack(const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTrack(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTrackColor(const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTrackColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);


		float getMinimumThumbLengthRatio();

		void setMinimumThumbLengthRatio(float ratio);


		sl_bool getThumbPositionRange(sl_ui_pos& begin, sl_ui_pos& end);

		sl_bool getThumbRegion(UIRect& _out);

		sl_scroll_pos getValueFromThumbPosition(sl_ui_pos pos);

		sl_bool isValid();

	public:
		SLIB_DECLARE_EVENT_HANDLER(ScrollBar, Changing, sl_scroll_pos& value, UIEvent* ev /* nullable */)
		SLIB_DECLARE_EVENT_HANDLER(ScrollBar, Change, sl_scroll_pos value, UIEvent* ev /* nullable */)

	public:
		void onDraw(Canvas* canvas) override;

		void onMouseEvent(UIEvent* ev) override;

		void onMouseWheelEvent(UIEvent* ev) override;

		void onSetCursor(UIEvent* ev) override;

	protected:
		sl_scroll_pos _normalizeValue(sl_scroll_pos value);

		void _changeValue(sl_scroll_pos value, UIEvent* ev, UIUpdateMode mode);

		void _setHoverThumb(sl_bool flag, UIUpdateMode mode);

	protected:
		LayoutOrientation m_orientation;
		sl_scroll_pos m_value;
		sl_scroll_pos m_page;
		sl_scroll_pos m_line;
		sl_scroll_pos m_value_min;
		sl_scroll_pos m_value_max;

		ViewStateMap< Ref<Drawable> > m_thumbs;
		ViewStateMap< Ref<Drawable> > m_tracks;

		float m_thumb_len_ratio_min;

		sl_ui_pos m_posDown;
		sl_scroll_pos m_valueDown;
		sl_bool m_flagHoverThumb;

	};

}

#endif
