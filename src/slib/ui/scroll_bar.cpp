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

#include "slib/ui/scroll_bar.h"

#include "slib/ui/priv/view_state_map.h"
#include "slib/graphics/canvas.h"
#include "slib/core/safe_static.h"

namespace slib
{

	namespace {

		class StaticContext
		{
		public:
			Ref<Drawable> defaultThumb;
			Ref<Drawable> defaultPressedThumb;
			Ref<Drawable> defaultHoverThumb;

			Ref<Drawable> defaultHoverTrack;
			Ref<Drawable> defaultPressedTrack;

			StaticContext()
			{
				defaultThumb = ColorDrawable::create(Color(0, 0, 0, 150));
				defaultPressedThumb = ColorDrawable::create(Color(0, 0, 0, 200));
				defaultHoverThumb = ColorDrawable::create(Color(0, 0, 0, 180));

				defaultHoverTrack = ColorDrawable::create(Color(255, 255, 255, 50));
				defaultPressedTrack = ColorDrawable::create(Color(255, 255, 255, 100));
			}
		};

		SLIB_SAFE_STATIC_GETTER(StaticContext, GetStaticContext)

	}

	SLIB_DEFINE_OBJECT(ScrollBar, View)

	ScrollBar::ScrollBar(LayoutOrientation orientation)
	{
		setSavingCanvasState(sl_false);

		m_orientation = orientation;
		m_value = 0;
		m_page = 0;
		m_line = 0;
		m_value_min = 0;
		m_value_max = 1;

		m_valueDown = 0;
		m_posDown = 0;

		m_thumb_len_ratio_min = 2;

		m_flagHoverThumb = sl_false;

	}

	ScrollBar::~ScrollBar()
	{
	}

	LayoutOrientation ScrollBar::getOrientation()
	{
		return m_orientation;
	}

	void ScrollBar::setOrientation(LayoutOrientation orientation, UIUpdateMode mode)
	{
		m_orientation = orientation;
		invalidate(mode);
	}

	sl_bool ScrollBar::isVertical()
	{
		return m_orientation == LayoutOrientation::Vertical;
	}

	void ScrollBar::setVertical(UIUpdateMode mode)
	{
		setOrientation(LayoutOrientation::Vertical, mode);
	}

	sl_bool ScrollBar::isHorizontal()
	{
		return m_orientation == LayoutOrientation::Horizontal;
	}

	void ScrollBar::setHorizontal(UIUpdateMode mode)
	{
		setOrientation(LayoutOrientation::Horizontal, mode);
	}

	sl_scroll_pos ScrollBar::getValue()
	{
		return m_value;
	}

	void ScrollBar::setValue(sl_scroll_pos value, UIUpdateMode mode)
	{
		_changeValue(value, sl_null, mode);
	}

	void ScrollBar::setValueOfOutRange(sl_scroll_pos value, UIUpdateMode mode)
	{
		m_value = value;
		invalidate(mode);
	}

	sl_scroll_pos ScrollBar::getPage()
	{
		return m_page;
	}

	void ScrollBar::setPage(sl_scroll_pos page, UIUpdateMode mode)
	{
		m_page = page;
		invalidate(mode);
	}

	sl_scroll_pos ScrollBar::getLine()
	{
		return m_line;
	}

	void ScrollBar::setLine(sl_scroll_pos line, UIUpdateMode mode)
	{
		m_line = line;
		invalidate(mode);
	}

	sl_scroll_pos ScrollBar::getMinimumValue()
	{
		return m_value_min;
	}

	void ScrollBar::setMinimumValue(sl_scroll_pos value, UIUpdateMode mode)
	{
		m_value_min = value;
		setValue(m_value, UIUpdateMode::None);
		invalidate(mode);
	}

	sl_scroll_pos ScrollBar::getMaximumValue()
	{
		return m_value_max;
	}

	void ScrollBar::setMaximumValue(sl_scroll_pos value, UIUpdateMode mode)
	{
		m_value_max = value;
		setValue(m_value, UIUpdateMode::None);
		invalidate(mode);
	}

	sl_scroll_pos ScrollBar::getRange()
	{
		sl_scroll_pos range = m_value_max - m_value_min - m_page;
		if (range < 0) {
			range = 0;
		}
		return range;
	}

	void ScrollBar::setRange(sl_scroll_pos range, UIUpdateMode mode)
	{
		if (range < 0) {
			range = 0;
		}
		m_value_max = m_value_min + range;
		setValue(m_value, UIUpdateMode::None);
		invalidate(mode);
	}

	Ref<Drawable> ScrollBar::getThumb(ViewState state)
	{
		return m_thumbs.get(state);
	}

	void ScrollBar::setThumb(const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode)
	{
		m_thumbs.set(state, drawable);
		invalidate(mode);
	}

	void ScrollBar::setThumb(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_thumbs.defaultValue = drawable;
		invalidate(mode);
	}

	void ScrollBar::setThumbColor(const Color& color, ViewState state, UIUpdateMode mode)
	{
		setThumb(ColorDrawable::create(color), state, mode);
	}

	void ScrollBar::setThumbColor(const Color& color, UIUpdateMode mode)
	{
		setThumb(ColorDrawable::create(color), mode);
	}

	Ref<Drawable> ScrollBar::getTrack(ViewState state)
	{
		return m_tracks.get(state);
	}

	void ScrollBar::setTrack(const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode)
	{
		m_tracks.set(state, drawable);
		invalidate(mode);
	}

	void ScrollBar::setTrack(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_tracks.defaultValue = drawable;
		invalidate(mode);
	}

	void ScrollBar::setTrackColor(const Color& color, ViewState state, UIUpdateMode mode)
	{
		setTrack(ColorDrawable::create(color), state, mode);
	}

	void ScrollBar::setTrackColor(const Color& color, UIUpdateMode mode)
	{
		setTrack(ColorDrawable::create(color), mode);
	}

	float ScrollBar::getMinimumThumbLengthRatio()
	{
		return m_thumb_len_ratio_min;
	}

	void ScrollBar::setMinimumThumbLengthRatio(float ratio)
	{
		if (ratio < 0) {
			ratio = 0;
		}
		m_thumb_len_ratio_min = ratio;
	}

#define CHECK_STATUS(...) \
	sl_scroll_pos value = m_value; \
	sl_scroll_pos page = m_page; \
	sl_scroll_pos range_max = m_value_max; \
	sl_scroll_pos range_min = m_value_min; \
	sl_scroll_pos range = range_max - range_min; \
	if (page < 0) { \
		return __VA_ARGS__; \
	} \
	if (range - page < SLIB_EPSILON) { \
		return __VA_ARGS__; \
	} \
	sl_bool flagVertical = m_orientation == LayoutOrientation::Vertical; \
	sl_ui_pos width = getWidth() - getPaddingLeft() - getPaddingTop(); \
	if (width < 1) { \
		return __VA_ARGS__; \
	} \
	sl_ui_pos height = getHeight() - getPaddingTop() - getPaddingBottom(); \
	if (height < 1) { \
		return __VA_ARGS__; \
	} \
	sl_ui_len depth, length; \
	if (flagVertical) { \
		depth = width; \
		length = height; \
	} else { \
		depth = height; \
		length = width; \
	} \
	float f_min_thumb_len = (float)(m_thumb_len_ratio_min * (float)depth); \
	if (f_min_thumb_len < 0 || f_min_thumb_len >= (float)length) { \
		return __VA_ARGS__; \
	} \
	sl_ui_len min_thumb_len = (sl_ui_len)(f_min_thumb_len); \
	float f_thumb_len = (float)(page * (sl_scroll_pos)length / range); \
	if (f_thumb_len < 0 || f_thumb_len - (float)length > SLIB_EPSILON) { \
		return __VA_ARGS__; \
	} \
	sl_ui_len thumb_len = (sl_ui_len)(f_thumb_len); \
	if (thumb_len < min_thumb_len) { \
		thumb_len = min_thumb_len; \
	} \
	if (thumb_len > length) { \
		thumb_len = length; \
	} \
	sl_scroll_pos ratioValuePos; \
	if (thumb_len < length) { \
		ratioValuePos = (range - page) / (sl_scroll_pos)(length - thumb_len); \
	} else { \
		ratioValuePos = 0; \
	} \
	SLIB_UNUSED(ratioValuePos) \
	SLIB_UNUSED(value)


	sl_bool ScrollBar::getThumbPositionRange(sl_ui_pos& _pos_begin, sl_ui_pos& _pos_end)
	{
		CHECK_STATUS(sl_false)
		sl_ui_pos pos_begin = (sl_ui_pos)((value - range_min) * (sl_scroll_pos)(length - thumb_len) / (range - page));
		sl_ui_pos pos_end = pos_begin + thumb_len;
		if (pos_end > (sl_ui_pos)length) {
			pos_end = length;
		}
		if (pos_begin > pos_end - (sl_ui_pos)min_thumb_len) {
			pos_begin = pos_end - min_thumb_len;
		}
		if (pos_begin < 0) {
			pos_begin = 0;
		}
		if (pos_end < pos_begin + (sl_ui_pos)min_thumb_len) {
			pos_end = pos_begin + min_thumb_len;
		}
		if (flagVertical) {
			sl_ui_pos padding = getPaddingTop();
			pos_begin += padding;
			pos_end += padding;
		} else {
			sl_ui_pos padding = getPaddingLeft();
			pos_begin += padding;
			pos_end += padding;
		}
		_pos_begin = pos_begin;
		_pos_end = pos_end;
		return sl_true;
	}

	sl_bool ScrollBar::getThumbRegion(UIRect& region)
	{
		sl_ui_pos pos_begin, pos_end;
		if (getThumbPositionRange(pos_begin, pos_end)) {
			if (isVertical()) {
				region.left = getPaddingLeft();
				region.right = getWidth() - getPaddingRight();
				region.top = pos_begin;
				region.bottom = pos_end;
			} else {
				region.top = getPaddingTop();
				region.bottom = getHeight() - getPaddingBottom();
				region.left = pos_begin;
				region.right = pos_end;
			}
			region.fixSizeError();
			return sl_true;
		}
		return sl_false;
	}

	sl_scroll_pos ScrollBar::getValueFromThumbPosition(sl_ui_pos pos)
	{
		CHECK_STATUS(range_min)
		if (flagVertical) {
			pos -= getPaddingTop();
		} else {
			pos -= getPaddingLeft();
		}
		return ((sl_scroll_pos)(pos - thumb_len / 2) * ratioValuePos) + m_value_min;
	}

	sl_bool ScrollBar::isValid()
	{
		CHECK_STATUS(sl_false)
		return sl_true;
	}

	void ScrollBar::onDraw(Canvas* canvas)
	{
		StaticContext* context = GetStaticContext();
		if (!context) {
			return;
		}
		ViewState state = getState();
		Ref<Drawable> track = m_tracks.evaluate(state);
		if (track.isNull()) {
			if (SLIB_VIEW_STATE_IS_PRESSED(state)) {
				track = context->defaultPressedTrack;
			} else if (SLIB_VIEW_STATE_IS_HOVER(state)) {
				track = context->defaultHoverTrack;
			}
		}
		if (track.isNotNull()) {
			canvas->draw(getBoundsInnerPadding(), track);
		}
		UIRect thumbRegion;
		if (getThumbRegion(thumbRegion)) {
			if (!m_flagHoverThumb && state == ViewState::Hover) {
				state = ViewState::Normal;
			}
			Ref<Drawable> thumb = m_thumbs.evaluate(state);
			if (thumb.isNull()) {
				if (SLIB_VIEW_STATE_IS_PRESSED(state)) {
					thumb = context->defaultPressedThumb;
				} else if (SLIB_VIEW_STATE_IS_HOVER(state)) {
					thumb = context->defaultHoverThumb;
				} else {
					thumb = context->defaultThumb;
				}
			}
			if (thumb.isNotNull()) {
				Color color;
				if (ColorDrawable::check(thumb, &color)) {
					sl_bool flagAntiAlias = canvas->isAntiAlias();
					canvas->setAntiAlias(sl_true);
					sl_real r = Math::min(thumbRegion.getWidth(), thumbRegion.getHeight()) * 0.5f;
					sl_real padding = 2;
					r -= padding;
					canvas->fillRoundRect(Rectangle(thumbRegion.left + padding - 1, thumbRegion.top + padding, thumbRegion.right - padding, thumbRegion.bottom - padding), Size(r, r), color);
					canvas->setAntiAlias(flagAntiAlias);
				} else {
					canvas->draw(thumbRegion, thumb);
				}
			}
		}
	}

	void ScrollBar::onMouseEvent(UIEvent* ev)
	{
		View::onMouseEvent(ev);

		CHECK_STATUS()

		sl_ui_pos pos_begin, pos_end;
		if (!(getThumbPositionRange(pos_begin, pos_end))) {
			ev->preventDefault();
			return;
		}

		UIAction action = ev->getAction();
		sl_ui_pos pos;
		if (isVertical()) {
			pos = (sl_ui_pos)(ev->getY());
		} else {
			pos = (sl_ui_pos)(ev->getX());
		}

		if (action == UIAction::MouseLeave) {
			_setHoverThumb(sl_false, UIUpdateMode::None);
			invalidate();
			return;
		} else {
			sl_bool flagHoverThumb = sl_false;
			UIRect region;
			if (getThumbRegion(region)) {
				if (region.containsPoint(ev->getPoint())) {
					if (action == UIAction::MouseMove) {
						flagHoverThumb = sl_true;
					}
				}
			}
			if (action == UIAction::MouseMove) {
				_setHoverThumb(flagHoverThumb, UIUpdateMode::Redraw);
				return;
			} else {
				_setHoverThumb(flagHoverThumb, UIUpdateMode::None);
			}
		}
		switch (action) {
			case UIAction::LeftButtonDown:
			case UIAction::TouchBegin:
				m_posDown = pos;
				if (pos < pos_begin) {
					m_valueDown = getValueFromThumbPosition(pos);
					if (page > 0) {
						_changeValue(value - page, ev, UIUpdateMode::None);
					} else {
						_changeValue(m_valueDown, ev, UIUpdateMode::None);
					}
				} else if (pos <= pos_end) {
					m_valueDown = value;
				} else {
					m_valueDown = getValueFromThumbPosition(pos);
					if (page > 0) {
						_changeValue(value + page, ev, UIUpdateMode::None);
					} else {
						_changeValue(m_valueDown, ev, UIUpdateMode::None);
					}
				}
				invalidate();
				break;
			case UIAction::LeftButtonDrag:
			case UIAction::TouchMove:
				if (isPressedState()) {
					_changeValue(m_valueDown + (sl_scroll_pos)(pos - m_posDown) * ratioValuePos, ev, UIUpdateMode::Redraw);
				}
				break;
			case UIAction::LeftButtonUp:
			case UIAction::TouchEnd:
			case UIAction::TouchCancel:
				if (isPressedState()) {
					if (m_posDown != pos) {
						_changeValue(m_valueDown + (sl_scroll_pos)(pos - m_posDown) * ratioValuePos, ev, UIUpdateMode::None);
					}
					invalidate();
				}
				break;
			default:
				return;
		}

		ev->stopPropagation();

	}

	void ScrollBar::onMouseWheelEvent(UIEvent* ev)
	{
		View::onMouseWheelEvent(ev);

		CHECK_STATUS()

		sl_scroll_pos line = m_line;
		if (line < SLIB_EPSILON) {
			if (page > 0) {
				line = page / 20;
			} else {
				line = range / 20;
			}
		}
		sl_real delta = ev->getDelta();
		if (delta > SLIB_EPSILON) {
			_changeValue(value - line, ev, UIUpdateMode::Redraw);
		} else if (delta < -SLIB_EPSILON) {
			_changeValue(value + line, ev, UIUpdateMode::Redraw);
		}

		ev->stopPropagation();
	}


	SLIB_DEFINE_EVENT_HANDLER(ScrollBar, Changing, (sl_scroll_pos& value, UIEvent* ev), value, ev)

	SLIB_DEFINE_EVENT_HANDLER(ScrollBar, Change, (sl_scroll_pos value, UIEvent* ev), value, ev)

	sl_scroll_pos ScrollBar::_normalizeValue(sl_scroll_pos value)
	{
		sl_scroll_pos _max = m_value_max - m_page;
		if (value > _max) {
			value = _max;
		}
		if (value < m_value_min) {
			value = m_value_min;
		}
		return value;
	}

	void ScrollBar::_changeValue(sl_scroll_pos value, UIEvent* ev, UIUpdateMode mode)
	{
		value = _normalizeValue(value);
		if (Math::isAlmostZero(value - m_value)) {
			m_value = value;
			return;
		}
		invokeChanging(value, ev);
		value = _normalizeValue(value);
		if (Math::isAlmostZero(value - m_value)) {
			m_value = value;
			return;
		}
		m_value = value;
		invalidate(mode);
		invokeChange(value, ev);
	}

	void ScrollBar::_setHoverThumb(sl_bool flag, UIUpdateMode mode)
	{
		if (m_flagHoverThumb != flag) {
			m_flagHoverThumb = flag;
			invalidate(mode);
		}
	}

}
