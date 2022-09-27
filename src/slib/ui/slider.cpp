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

#include "slib/ui/slider.h"

#include "slib/ui/cursor.h"
#include "slib/graphics/canvas.h"
#include "slib/core/safe_static.h"

namespace slib
{

	namespace priv
	{
		namespace slider
		{

			class StaticContext
			{
			public:
				Ref<Drawable> defaultTrack;
				Ref<Drawable> defaultProgress;
				Ref<Drawable> defaultProgress2;
				Ref<Drawable> defaultThumb;
				Ref<Drawable> defaultPressedThumb;
				Ref<Drawable> defaultHoverThumb;
				
				StaticContext()
				{
					defaultTrack = ColorDrawable::create(Color(0, 0, 0));
					defaultProgress = ColorDrawable::create(Color(0, 50, 250));
					defaultProgress2 = ColorDrawable::create(Color(0, 250, 50));
					defaultThumb = ColorDrawable::create(Color(50, 50, 50, 255));
					defaultPressedThumb = ColorDrawable::create(Color(0, 100, 250, 255));
					defaultHoverThumb = ColorDrawable::create(Color(0, 200, 150, 255));
				}
			};
			
			SLIB_SAFE_STATIC_GETTER(StaticContext, GetStaticContext)
			
			static Ref<Drawable> const& ResolveDrawable(const Ref<Drawable>& drawableOriginal, const Ref<Drawable>& drawableCommon, const Ref<Drawable>& drawableShared)
			{
				if (drawableOriginal.isNotNull()) {
					return drawableOriginal;
				}
				if (drawableCommon.isNotNull()) {
					if (drawableCommon->isColor()) {
						return drawableShared;
					}
					return drawableCommon;
				}
				return Ref<Drawable>::null();
			}

		}
	}

	using namespace priv::slider;

	SLIB_DEFINE_OBJECT(Slider, ProgressBar)
	
	Slider::Slider(LayoutOrientation orientation) : ProgressBar(orientation)
	{
		StaticContext* s = GetStaticContext();
		if (s) {
			m_track = s->defaultTrack;
			m_progress = s->defaultProgress;
			m_progress2 = s->defaultProgress2;
			m_thumb = s->defaultThumb;
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

	Ref<Drawable> Slider::getThumbDrawable()
	{
		return m_thumb;
	}
	
	void Slider::setThumbDrawable(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_thumb = drawable;
		invalidate(mode);
	}
	
	void Slider::setThumbColor(const Color& color, UIUpdateMode mode)
	{
		setThumbDrawable(ColorDrawable::create(color), mode);
	}
	
	Ref<Drawable> Slider::getPressedThumbDrawable()
	{
		return m_pressedThumb;
	}
	
	void Slider::setPressedThumbDrawable(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_pressedThumb = drawable;
		invalidate(mode);
	}
	
	void Slider::setPressedThumbColor(const Color& color, UIUpdateMode mode)
	{
		setPressedThumbDrawable(ColorDrawable::create(color), mode);
	}
	
	Ref<Drawable> Slider::getHoverThumbDrawable()
	{
		return m_hoverThumb;
	}
	
	void Slider::setHoverThumbDrawable(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_hoverThumb = drawable;
		invalidate(mode);
	}
	
	void Slider::setHoverThumbColor(const Color& color, UIUpdateMode mode)
	{
		setHoverThumbDrawable(ColorDrawable::create(color), mode);
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
	
	SLIB_DEFINE_EVENT_HANDLER(Slider, Change, float value)
	
	void Slider::dispatchChange(float value)
	{
		SLIB_INVOKE_EVENT_HANDLER(Change, value)
	}
	
	SLIB_DEFINE_EVENT_HANDLER(Slider, ChangeSecondary, float value)
	
	void Slider::dispatchChangeSecondary(float value)
	{
		SLIB_INVOKE_EVENT_HANDLER(ChangeSecondary, value)
	}

	void Slider::onDraw(Canvas* canvas)
	{
		StaticContext* s = GetStaticContext();
		if (!s) {
			return;
		}
		Ref<Drawable> progress = m_progress;
		Ref<Drawable> progress2 = m_progress2;
		Ref<Drawable> thumb;
		if (m_indexPressedThumb == 0) {
			thumb = ResolveDrawable(m_pressedThumb, m_thumb, s->defaultPressedThumb);
		} else if (m_indexHoverThumb == 0) {
			thumb = ResolveDrawable(m_hoverThumb, m_thumb, s->defaultHoverThumb);
		}
		if (thumb.isNull()) {
			thumb = m_thumb;
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
			drawThumb(canvas, thumb, rcThumb);
		}
		if (isDualValues() && rcThumb2.isValidSize()) {
			Ref<Drawable> thumb2;
			if (m_indexPressedThumb == 1) {
				thumb2 = ResolveDrawable(m_pressedThumb, m_thumb, s->defaultPressedThumb);
			} else if (m_indexHoverThumb == 1) {
				thumb2 = ResolveDrawable(m_hoverThumb, m_thumb, s->defaultHoverThumb);
			}
			if (thumb2.isNull()) {
				thumb2 = m_thumb;
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
						setHoverThumb(1);
						return;
					}
				}
				if (rcThumb.containsPoint(ev->getPoint())) {
					setHoverThumb(0);
					return;
				}
				setHoverThumb(-1);
				return;
			}
			case UIAction::MouseLeave:
				setHoverThumb(-1);
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
					if (m_indexPressedThumb == 0) {
						changeValue(value, sl_false);
					} else {
						changeValue(value, sl_true);
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
		
		ev->stopPropagation();
		
	}
	
	void Slider::onMouseWheelEvent(UIEvent* ev)
	{
		float step = refineStep();
		sl_real delta;
		if (isVertical()) {
			delta = ev->getDeltaY();
		} else {
			delta = ev->getDeltaX();
		}
		if (delta > SLIB_EPSILON) {
			changeValue(m_value - step, sl_false);
		} else if (delta < -SLIB_EPSILON) {
			changeValue(m_value + step, sl_false);
		}
		
		ev->stopPropagation();
	}
	
	void Slider::onKeyEvent(UIEvent* ev)
	{
		float step = refineStep();
		if (ev->getAction() == UIAction::KeyDown) {
			switch (ev->getKeycode()) {
				case Keycode::Left:
				case Keycode::Up:
					changeValue(m_value - step, sl_false);
					break;
				case Keycode::Right:
				case Keycode::Down:
					changeValue(m_value + step, sl_false);
					break;
				default:
					return;
			}
			ev->stopPropagation();
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
	
	void Slider::setHoverThumb(int index)
	{
		if (m_indexHoverThumb != index) {
			m_indexHoverThumb = index;
			invalidate();
		}
	}
	
	void Slider::changeValue(float v, sl_bool flagChange2)
	{
		float value, value2;
		if (flagChange2) {
			value = m_value;
			value2 = v;
		} else {
			value = v;
			value2 = m_value2;
		}
		int n = tryChangeValue(value, value2, flagChange2);
		m_value = value;
		if (n & 1) {
			dispatchChange(value);
		}
		if (isDualValues()) {
			m_value2 = value2;
			if (n & 2) {
				dispatchChangeSecondary(value2);
			}
		}
		invalidate();
	}
	
}
