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

#include "slib/ui/slider.h"

#include "slib/ui/priv/view_state_map.h"
#include "slib/ui/cursor.h"
#include "slib/graphics/canvas.h"
#include "slib/core/safe_static.h"

namespace slib
{

	namespace {

		class StaticContext
		{
		public:
			Ref<Drawable> defaultTrack;
			Ref<Drawable> defaultProgress;
			Ref<Drawable> defaultProgress2;

			Ref<Drawable> defaultThumb;
			Ref<Drawable> defaultHoverThumb;
			Ref<Drawable> defaultPressedThumb;

		public:
			StaticContext()
			{
				defaultTrack = ColorDrawable::create(Color(0, 0, 0));
				defaultProgress = ColorDrawable::create(Color(0, 50, 250));
				defaultProgress2 = ColorDrawable::create(Color(0, 250, 50));
				defaultThumb = ColorDrawable::create(Color(50, 50, 50, 255));
				defaultHoverThumb = ColorDrawable::create(Color(0, 200, 150, 255));
				defaultPressedThumb = ColorDrawable::create(Color(0, 100, 250, 255));
			}

		public:
			const Ref<Drawable>& getThumb(ViewState state)
			{
				if (SLIB_VIEW_STATE_IS_PRESSED(state)) {
					return defaultPressedThumb;
				} else if (SLIB_VIEW_STATE_IS_HOVER(state)) {
					return defaultHoverThumb;
				} else {
					return defaultThumb;
				}
			}

		};

		SLIB_SAFE_STATIC_GETTER(StaticContext, GetStaticContext)

	}

	SLIB_DEFINE_OBJECT(Slider, ProgressBar)

	Slider::Slider(LayoutOrientation orientation) : ProgressBar(orientation)
	{
		StaticContext* s = GetStaticContext();
		if (s) {
			m_track = s->defaultTrack;
			m_progress = s->defaultProgress;
			m_progress2 = s->defaultProgress2;
		}

		m_thumbSize.x = 0;
		m_thumbSize.y = 0;

		m_indexHoverThumb = -1;
		m_indexPressedThumb = -1;

		setCursor(Cursor::getHand());
#if !defined(SLIB_PLATFORM_IS_MOBILE)
		setFocusable(sl_true);
#endif
		setPadding(1, UIUpdateMode::Init);
	}

	Slider::~Slider()
	{
	}

	void Slider::setValue(float value, UIUpdateMode mode)
	{
		_changeValue(value, sl_null, mode);
	}

	void Slider::setSecondaryValue(float value, UIUpdateMode mode)
	{
		_changeValue2(value, sl_null, mode);
	}

	Ref<Drawable> Slider::getThumb(ViewState state)
	{
		return m_thumbs.get(state);
	}

	void Slider::setThumb(const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode)
	{
		m_thumbs.set(state, drawable);
		invalidate(mode);
	}

	void Slider::setThumb(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_thumbs.defaultValue = drawable;
		invalidate(mode);
	}

	void Slider::setThumbColor(const Color& color, ViewState state, UIUpdateMode mode)
	{
		setThumb(ColorDrawable::create(color), state, mode);
	}

	void Slider::setThumbColor(const Color& color, UIUpdateMode mode)
	{
		setThumb(ColorDrawable::create(color), mode);
	}

	const UISize& Slider::getThumbSize()
	{
		return m_thumbSize;
	}

	void Slider::setThumbSize(const UISize& size, UIUpdateMode mode)
	{
		m_thumbSize = size;
		invalidate(mode);
	}

	void Slider::setThumbSize(sl_ui_len width, sl_ui_len height, UIUpdateMode mode)
	{
		setThumbSize(UISize(width, height), mode);
	}

	void Slider::setThumbSize(sl_ui_len size, UIUpdateMode mode)
	{
		setThumbSize(UISize(size, size), mode);
	}

	sl_ui_len Slider::getThumbWidth()
	{
		return m_thumbSize.x;
	}

	void Slider::setThumbWidth(sl_ui_len width, UIUpdateMode mode)
	{
		setThumbSize(UISize(width, m_thumbSize.y), mode);
	}

	sl_ui_len Slider::getThumbHeight()
	{
		return m_thumbSize.y;
	}

	void Slider::setThumbHeight(sl_ui_len height, UIUpdateMode mode)
	{
		setThumbSize(UISize(m_thumbSize.x, height), mode);
	}
	
	SLIB_DEFINE_EVENT_HANDLER(Slider, Changing, (float& value, UIEvent* ev), value, ev)

	SLIB_DEFINE_EVENT_HANDLER(Slider, Change, (float value, UIEvent* ev), value, ev)

	SLIB_DEFINE_EVENT_HANDLER(Slider, ChangingSecondary, (float& value, UIEvent* ev), value, ev)

	SLIB_DEFINE_EVENT_HANDLER(Slider, ChangeSecondary, (float value, UIEvent* ev), value, ev)

	void Slider::onDraw(Canvas* canvas)
	{
		StaticContext* context = GetStaticContext();
		if (!context) {
			return;
		}
		Ref<Drawable> progress = m_progress;
		Ref<Drawable> progress2 = m_progress2;

		ViewState state1 = _getThumbState(0);
		Ref<Drawable> thumb1 = m_thumbs.evaluate(state1);
		if (thumb1.isNull()) {
			thumb1 = context->getThumb(state1);
		}

		UIRect rcTrack, rcProgress, rcProgress2, rcThumb, rcThumb2;
		getRegions(rcTrack, rcProgress, rcProgress2, rcThumb, rcThumb2);
		if (rcTrack.isValidSize()) {
			drawTrack(canvas, m_track, rcTrack);
		}
		if (rcProgress2.isValidSize()) {
			drawTrack(canvas, progress2, rcProgress2);
		}
		if (rcProgress.isValidSize()) {
			drawTrack(canvas, progress, rcProgress);
		}
		if (rcThumb.isValidSize()) {
			drawThumb(canvas, thumb1, rcThumb);
		}
		if (isDualValues() && rcThumb2.isValidSize()) {
			ViewState state2 = _getThumbState(1);
			Ref<Drawable> thumb2 = m_thumbs.evaluate(state2);
			if (thumb2.isNull()) {
				thumb2 = context->getThumb(state2);
			}
			drawThumb(canvas, thumb2, rcThumb2);
		}
	}

	void Slider::onMouseEvent(UIEvent* ev)
	{
		UIAction action = ev->getAction();
		sl_ui_pos pos;
		if (isVertical()) {
			pos = (sl_ui_pos)(ev->getY());
		} else {
			pos = (sl_ui_pos)(ev->getX());
		}

		switch (action) {
			case UIAction::MouseMove:
			case UIAction::MouseEnter:
				{
					UIRect rcTrack, rcProgress, rcProgress2, rcThumb, rcThumb2;
					getRegions(rcTrack, rcProgress, rcProgress2, rcThumb, rcThumb2);
					if (isDualValues()) {
						if (rcThumb2.containsPoint(ev->getPoint())) {
							_setHoverThumb(1, action);
							return;
						}
					}
					if (rcThumb.containsPoint(ev->getPoint())) {
						_setHoverThumb(0, action);
						return;
					}
					_setHoverThumb(-1, action);
					return;
				}
			case UIAction::MouseLeave:
				_setHoverThumb(-1, action);
				return;
			case UIAction::LeftButtonDown:
			case UIAction::TouchBegin:
				if (isDualValues()) {
					UIRect rcTrack, rcProgress, rcProgress2, rcThumb, rcThumb2;
					getRegions(rcTrack, rcProgress, rcProgress2, rcThumb, rcThumb2);
					if (isVertical()) {
						if (pos >= (rcThumb.bottom + rcThumb2.top) / 2) {
							m_indexPressedThumb = 1;
						} else {
							m_indexPressedThumb = 0;
						}
					} else {
						if (pos >= (rcThumb.right + rcThumb2.left) / 2) {
							m_indexPressedThumb = 1;
						} else {
							m_indexPressedThumb = 0;
						}
					}
				} else {
					m_indexPressedThumb = 0;
				}
			case UIAction::LeftButtonDrag:
			case UIAction::TouchMove:
				if (m_indexPressedThumb >= 0) {
					float value = getValueFromPosition(pos);
					if (m_indexPressedThumb) {
						_changeValue2(value, ev);
					} else {
						_changeValue(value, ev);
					}
				}
				break;
			case UIAction::LeftButtonUp:
			case UIAction::TouchEnd:
			case UIAction::TouchCancel:
				m_indexPressedThumb = -1;
				invalidate();
				break;
			default:
				return;
		}

		ProgressBar::onMouseEvent(ev);

		ev->accept();
	}

	void Slider::onMouseWheelEvent(UIEvent* ev)
	{
		ProgressBar::onMouseWheelEvent(ev);

		float step = refineStep();
		sl_real delta = ev->getDelta();
		if (delta > SLIB_EPSILON) {
			if (ev->isShiftKey()) {
				_changeValue2(m_value2 - step, ev);
			} else {
				_changeValue(m_value - step, ev);
			}
		} else if (delta < -SLIB_EPSILON) {
			if (ev->isShiftKey()) {
				_changeValue2(m_value2 + step, ev);
			} else {
				_changeValue(m_value + step, ev);
			}
		}

		ev->accept();
	}

	void Slider::onKeyEvent(UIEvent* ev)
	{
		ProgressBar::onKeyEvent(ev);

		float step = refineStep();
		if (ev->getAction() == UIAction::KeyDown) {
			switch (ev->getKeycode()) {
				case Keycode::Left:
				case Keycode::Up:
					if (ev->isShiftKey()) {
						_changeValue2(m_value2 - step, ev);
					} else {
						_changeValue(m_value - step, ev);
					}
					ev->accept();
					break;
				case Keycode::Right:
				case Keycode::Down:
					if (ev->isShiftKey()) {
						_changeValue2(m_value2 + step, ev);
					} else {
						_changeValue(m_value + step, ev);
					}
					ev->accept();
					break;
				default:
					return;
			}
		}
	}

	void Slider::drawTrack(Canvas* canvas, const Ref<Drawable>& track, const Rectangle& rectDst)
	{
		if (track.isNull()) {
			return;
		}
		Color color;
		if (ColorDrawable::check(track.get(), &color)) {
			Ref<Pen> pen = Pen::createSolidPen(1, color);
			if (isVertical()) {
				sl_real x = (rectDst.left + rectDst.right) / 2;
				canvas->drawLine(x, rectDst.top, x, rectDst.bottom, pen);
			} else {
				sl_real y = (rectDst.top + rectDst.bottom) / 2;
				canvas->drawLine(rectDst.left, y, rectDst.right, y, pen);
			}
		} else {
			canvas->draw(rectDst, track);
		}
	}

	void Slider::drawThumb(Canvas* canvas, const Ref<Drawable>& thumb, const Rectangle& rectDst)
	{
		if (thumb.isNull()) {
			return;
		}
		Color color;
		if (ColorDrawable::check(thumb.get(), &color)) {
			sl_bool flagAntiAlias = canvas->isAntiAlias();
			canvas->setAntiAlias(sl_true);
			if (Math::isAlmostZero(rectDst.getWidth() - rectDst.getHeight())) {
				canvas->fillEllipse(rectDst, color);
			} else {
				sl_real r = Math::min(rectDst.getWidth(), rectDst.getHeight()) / 8;
				canvas->fillRoundRect(rectDst, Size(r, r), color);
			}
			canvas->setAntiAlias(flagAntiAlias);
		} else {
			canvas->draw(rectDst, thumb);
		}
	}

	sl_ui_pos Slider::getStartPadding()
	{
		sl_ui_pos padding;
		if (isVertical()) {
			padding = getPaddingTop();
		} else {
			padding = getPaddingLeft();
		}
		return padding + getMinimumPadding();
	}

	sl_ui_pos Slider::getEndPadding()
	{
		sl_ui_pos padding;
		if (isVertical()) {
			padding = getPaddingBottom();
		} else {
			padding = getPaddingRight();
		}
		return padding + getMinimumPadding();
	}

	sl_ui_pos Slider::getMinimumPadding()
	{
		sl_ui_pos padding;
		if (isVertical()) {
			padding = m_thumbSize.y;
		} else {
			padding = m_thumbSize.x;
		}
		if (padding != 0) {
			return padding / 2 + 2;
		}
		if (isVertical()) {
			padding = getWidth() - getPaddingLeft() - getPaddingRight();
		} else {
			padding = getHeight() - getPaddingTop() - getPaddingBottom();
		}
		if (padding > 0) {
			return padding / 2 + 2;
		} else {
			return 0;
		}
	}

	sl_ui_pos Slider::getPositionFromValue(float value)
	{
		float range = m_value_max - m_value_min;
		if (range < SLIB_EPSILON) {
			return 0;
		}
		sl_ui_pos paddingStart = getStartPadding();
		sl_ui_pos paddingEnd = getEndPadding();
		sl_ui_pos len;
		if (isVertical()) {
			len = getHeight() - paddingStart - paddingEnd;
		} else {
			len = getWidth() - paddingStart - paddingEnd;
		}
		sl_ui_pos pos = (sl_ui_pos)(len * (value - m_value_min) / range);
		if (isReversed()) {
			return paddingStart + len - pos;
		} else {
			return pos + paddingStart;
		}
	}

	float Slider::getValueFromPosition(sl_ui_pos pos)
	{
		float range = m_value_max - m_value_min;
		if (range < SLIB_EPSILON) {
			return 0;
		}
		sl_ui_pos paddingStart = getStartPadding();
		sl_ui_pos paddingEnd = getEndPadding();
		sl_ui_pos len;
		if (isVertical()) {
			len = getHeight() - paddingStart - paddingEnd;
		} else {
			len = getWidth() - paddingStart - paddingEnd;
		}
		if (len <= 0) {
			return 0;
		}
		if (isReversed()) {
			pos = paddingStart + len - pos;
		} else {
			pos -= paddingStart;
		}
		return ((float)pos * range / (float)len) + getMinimumValue();
	}

	void Slider::getRegions(UIRect& outTrack, UIRect& outProgress, UIRect& outSecondaryProgress, UIRect& outThumb, UIRect& outSecondaryThumb)
	{
		sl_ui_pos pos1 = getPositionFromValue(m_value);
		sl_ui_pos pos2 = 0;
		if (m_value2 > m_value) {
			pos2 = getPositionFromValue(m_value2);
		} else {
			pos2 = pos1;
		}
		sl_ui_pos thumbWidth = m_thumbSize.x;
		sl_ui_pos thumbHeight = m_thumbSize.y;
		if (thumbWidth <= 0 || thumbHeight <= 0) {
			sl_ui_pos minThumbSize;
			if (isVertical()) {
				minThumbSize = getWidth() - getPaddingLeft() - getPaddingRight();
			} else {
				minThumbSize = getHeight() - getPaddingTop() - getPaddingBottom();
			}
			if (minThumbSize < 0) {
				minThumbSize = 0;
			}
			if (thumbWidth <= 0) {
				thumbWidth = minThumbSize;
			}
			if (thumbHeight <= 0) {
				thumbHeight = minThumbSize;
			}
		}
		if (isVertical()) {
			outTrack.top = getStartPadding();
			outTrack.bottom = getHeight() - getEndPadding();
			outTrack.left = getPaddingLeft();
			outTrack.right = getWidth() - getPaddingRight();
			if (isReversed()) {
				outProgress.top = pos1;
				outProgress.bottom = outTrack.right;
				outSecondaryProgress.top = pos2;
				outSecondaryProgress.bottom = outProgress.top;
			} else {
				outProgress.top = outTrack.top;
				outProgress.bottom = pos1;
				outSecondaryProgress.top = outProgress.bottom;
				outSecondaryProgress.bottom = pos2;
			}
			outProgress.left = outTrack.left;
			outProgress.right = outTrack.right;
			outSecondaryProgress.left = outTrack.left;
			outSecondaryProgress.right = outTrack.right;

			outThumb.top = pos1 - thumbHeight / 2;
			outThumb.left = (outTrack.left + outTrack.right) / 2 - thumbWidth / 2;
			outThumb.bottom = outThumb.top + thumbHeight;
			outThumb.right = outThumb.left + thumbWidth;
			if (isDualValues()) {
				outSecondaryThumb.top = pos2 - thumbHeight / 2;
				outSecondaryThumb.left = outThumb.left;
				outSecondaryThumb.bottom = outSecondaryThumb.top + thumbHeight;
				outSecondaryThumb.right = outThumb.right;
			} else {
				outSecondaryThumb.setZero();
			}
		} else {
			outTrack.left = getStartPadding();
			outTrack.right = getWidth() - getEndPadding();
			outTrack.top = getPaddingTop();
			outTrack.bottom = getHeight() - getPaddingBottom();
			if (isReversed()) {
				outProgress.left = pos1;
				outProgress.right = outTrack.right;
				outSecondaryProgress.left = pos2;
				outSecondaryProgress.right = outProgress.left;
			} else {
				outProgress.left = outTrack.left;
				outProgress.right = pos1;
				outSecondaryProgress.left = outProgress.right;
				outSecondaryProgress.right = pos2;
			}
			outProgress.top = outTrack.top;
			outProgress.bottom = outTrack.bottom;
			outSecondaryProgress.top = outTrack.top;
			outSecondaryProgress.bottom = outTrack.bottom;

			outThumb.left = pos1 - thumbWidth / 2;
			outThumb.top = (outTrack.top + outTrack.bottom) / 2 - thumbHeight / 2;
			outThumb.right = outThumb.left + thumbWidth;
			outThumb.bottom = outThumb.top + thumbHeight;
			if (isDualValues()) {
				outSecondaryThumb.left = pos2 - thumbWidth / 2;
				outSecondaryThumb.top = outThumb.top;
				outSecondaryThumb.right = outSecondaryThumb.left + thumbWidth;
				outSecondaryThumb.bottom = outThumb.bottom;
			} else {
				outSecondaryThumb.setZero();
			}
		}
	}

	void Slider::_changeValue(float value, UIEvent* ev, UIUpdateMode mode)
	{
		ObjectLocker locker(this);
		value = refineValue(value);
		if (ev && m_flagDualValues) {
			if (value > m_value2) {
				value = m_value2;
			}
		}
		if (Math::isAlmostZero(value - m_value)) {
			m_value = value;
			return;
		}
		invokeChanging(value, ev);
		value = refineValue(value);
		if (m_flagDualValues) {
			if (value > m_value2) {
				m_value2 = value;
			}
		}
		if (Math::isAlmostZero(value - m_value)) {
			m_value = value;
			return;
		}
		m_value = value;
		invalidate(mode);
		locker.unlock();
		invokeChange(value, ev);
	}

	void Slider::_changeValue2(float value, UIEvent* ev, UIUpdateMode mode)
	{
		ObjectLocker locker(this);
		if (!m_flagDualValues) {
			return;
		}
		value = refineValue(value);
		if (ev) {
			if (value < m_value) {
				value = m_value;
			}
		}
		if (Math::isAlmostZero(value - m_value2)) {
			m_value2 = value;
			return;
		}
		invokeChangingSecondary(value, ev);
		value = refineValue(value);
		if (value < m_value) {
			m_value = value;
		}
		if (Math::isAlmostZero(value - m_value2)) {
			m_value2 = value;
			return;
		}
		m_value2 = value;
		invalidate(mode);
		locker.unlock();
		invokeChangeSecondary(value, ev);
	}

	ViewState Slider::_getThumbState(int index)
	{
		ViewState state;
		if (m_indexPressedThumb == index) {
			state = ViewState::Pressed;
		} else if (m_indexHoverThumb == index) {
			state = ViewState::Hover;
		} else {
			state = ViewState::Normal;
		}
		if (isFocused()) {
			return (ViewState)((int)state + (int)(ViewState::Focused));
		} else {
			return state;
		}
	}

	void Slider::_setHoverThumb(int index, UIAction action)
	{
		if (action == UIAction::MouseMove) {
			if (m_indexHoverThumb == index) {
				return;
			}
		}
		m_indexHoverThumb = index;
		invalidate();
	}

}
