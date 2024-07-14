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

#include "slib/ui/switch_view.h"

#include "slib/ui/core.h"
#include "slib/ui/cursor.h"
#include "slib/ui/priv/view_state_map.h"
#include "slib/graphics/canvas.h"
#include "slib/core/timer.h"
#include "slib/core/safe_static.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(SwitchView, View)

	SwitchView::SwitchView():
		m_value(sl_false),
		m_flagTextInButton(sl_false),
		m_posThumb(0)
	{
		setCursor(Cursor::getHand());
		setRedrawingOnChangeState();
	}

	SwitchView::~SwitchView()
	{
	}

	void SwitchView::init()
	{
		View::init();
		setContentAntiAlias(sl_true, UIUpdateMode::Init);
	}

	SwitchValue SwitchView::getValue()
	{
		return m_value;
	}

	void SwitchView::setValue(SwitchValue value, UIUpdateMode mode)
	{
		_changeValue(value, sl_null, mode);
	}

	sl_bool SwitchView::isTextInButton()
	{
		return m_flagTextInButton;
	}

	void SwitchView::setTextInButton(sl_bool flag, UIUpdateMode mode)
	{
		m_flagTextInButton = flag;
		invalidate(mode);
	}

	String SwitchView::getText(SwitchValue value)
	{
		return m_texts[(int)value];
	}

	void SwitchView::setText(SwitchValue value, const String& text, UIUpdateMode mode)
	{
		m_texts[(int)value] = text;
		invalidate(mode);
	}

	void SwitchView::setText(const String& text, UIUpdateMode mode)
	{
		m_texts[0] = text;
		m_texts[1] = text;
		invalidate(mode);
	}

	Color SwitchView::getTextColor(SwitchValue value)
	{
		return m_textColors[(int)value];
	}

	void SwitchView::setTextColor(SwitchValue value, const Color& color, UIUpdateMode mode)
	{
		m_textColors[(int)value] = color;
		invalidate(mode);
	}

	void SwitchView::setTextColor(const Color& color, UIUpdateMode mode)
	{
		m_textColors[0] = color;
		m_textColors[1] = color;
		invalidate(mode);
	}

	Ref<Drawable> SwitchView::getThumb(SwitchValue value, ViewState state)
	{
		return m_thumbs[(int)value].get(state);
	}

	void SwitchView::setThumb(SwitchValue value, const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode)
	{
		m_thumbs[(int)value].set(state, drawable);
		invalidate(mode);
	}

	void SwitchView::setThumb(SwitchValue value, const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_thumbs[(int)value].set(drawable);
		invalidate(mode);
	}

	void SwitchView::setThumb(const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode)
	{
		m_thumbs[0].set(state, drawable);
		m_thumbs[1].set(state, drawable);
		invalidate(mode);
	}

	void SwitchView::setThumb(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_thumbs[0].set(drawable);
		m_thumbs[1].set(drawable);
		invalidate(mode);
	}

	void SwitchView::setThumbColor(SwitchValue value, const Color& color, ViewState state, UIUpdateMode mode)
	{
		setThumb(value, Drawable::fromColor(color), state, mode);
	}

	void SwitchView::setThumbColor(SwitchValue value, const Color& color, UIUpdateMode mode)
	{
		setThumb(value, Drawable::fromColor(color), mode);
	}

	void SwitchView::setThumbColor(const Color& color, ViewState state, UIUpdateMode mode)
	{
		setThumb(Drawable::fromColor(color), state, mode);
	}

	void SwitchView::setThumbColor(const Color& color, UIUpdateMode mode)
	{
		setThumb(Drawable::fromColor(color), mode);
	}

	Ref<Drawable> SwitchView::getTrack(SwitchValue value, ViewState state)
	{
		return m_tracks[(int)value].get(state);
	}

	void SwitchView::setTrack(SwitchValue value, const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode)
	{
		m_tracks[(int)value].set(state, drawable);
		invalidate(mode);
	}

	void SwitchView::setTrack(SwitchValue value, const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_tracks[(int)value].set(drawable);
		invalidate(mode);
	}

	void SwitchView::setTrack(const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode)
	{
		m_tracks[0].set(state, drawable);
		m_tracks[1].set(state, drawable);
		invalidate(mode);
	}

	void SwitchView::setTrack(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_tracks[0].set(drawable);
		m_tracks[1].set(drawable);
		invalidate(mode);
	}

	void SwitchView::setTrackColor(SwitchValue value, const Color& color, ViewState state, UIUpdateMode mode)
	{
		setTrack(value, Drawable::fromColor(color), state, mode);
	}

	void SwitchView::setTrackColor(SwitchValue value, const Color& color, UIUpdateMode mode)
	{
		setTrack(value, Drawable::fromColor(color), mode);
	}

	void SwitchView::setTrackColor(const Color& color, ViewState state, UIUpdateMode mode)
	{
		setTrack(Drawable::fromColor(color), state, mode);
	}

	void SwitchView::setTrackColor(const Color& color, UIUpdateMode mode)
	{
		setTrack(Drawable::fromColor(color), mode);
	}

	
	SLIB_DEFINE_EVENT_HANDLER(SwitchView, Change, (SwitchValue value, UIEvent* ev),  value, ev)

	namespace {
		class DrawContext
		{
		public:
			Color textColors[2];

			Ref<Drawable> thumbs[2];
			Ref<Drawable> tracks[2];
			Ref<Drawable> pressedThumbs[2];
			Ref<Drawable> pressedTracks[2];
			Ref<Drawable> hoverThumbs[2];
			Ref<Drawable> hoverTracks[2];

		public:
			DrawContext(sl_bool flagLabel)
			{
				if (flagLabel) {
					textColors[0] = Color::White;
					textColors[1] = Color::Black;
					thumbs[0] = ColorDrawable::create(Color(255, 255, 255));
					tracks[0] = ColorDrawable::create(Color(130, 130, 130));
					pressedThumbs[0] = ColorDrawable::create(Color(255, 255, 255));
					pressedTracks[0] = ColorDrawable::create(Color(100, 100, 100));
					hoverThumbs[0] = ColorDrawable::create(Color(255, 255, 255));
					hoverTracks[0] = ColorDrawable::create(Color(120, 120, 120));
					thumbs[1] = thumbs[0];
					tracks[1] = tracks[0];
					pressedThumbs[1] = pressedThumbs[0];
					pressedTracks[1] = pressedTracks[0];
					hoverThumbs[1] = hoverThumbs[0];
					hoverTracks[1] = hoverTracks[0];
				} else {
					textColors[0] = Color::Black;
					textColors[1] = Color::Black;
					thumbs[0] = ColorDrawable::create(Color(255, 255, 255));
					tracks[0] = ColorDrawable::create(Color(120, 120, 120));
					pressedThumbs[0] = ColorDrawable::create(Color(255, 255, 255));
					pressedTracks[0] = ColorDrawable::create(Color(0, 70, 210));
					hoverThumbs[0] = ColorDrawable::create(Color(255, 255, 255));
					hoverTracks[0] = ColorDrawable::create(Color(90, 90, 90));
					thumbs[1] = ColorDrawable::create(Color(255, 255, 255));
					tracks[1] = ColorDrawable::create(Color(0, 80, 230));
					pressedThumbs[1] = ColorDrawable::create(Color(255, 255, 255));
					pressedTracks[1] = ColorDrawable::create(Color(0, 70, 210));
					hoverThumbs[1] = ColorDrawable::create(Color(255, 255, 255));
					hoverTracks[1] = ColorDrawable::create(Color(30, 90, 210));
				}
			}

		public:
			Ref<Drawable> getTrack(sl_uint32 value, ViewState state)
			{
				if (SLIB_VIEW_STATE_IS_PRESSED(state)) {
					return pressedTracks[(int)value];
				}
				if (SLIB_VIEW_STATE_IS_HOVER(state)) {
					return hoverTracks[(int)value];
				}
				return tracks[value];
			}

			Ref<Drawable> getThumb(sl_uint32 value, ViewState state)
			{
				if (SLIB_VIEW_STATE_IS_PRESSED(state)) {
					return pressedThumbs[value];
				}
				if (SLIB_VIEW_STATE_IS_HOVER(state)) {
					return hoverThumbs[value];
				}
				return thumbs[value];
			}

		};

		SLIB_SAFE_STATIC_GETTER(DrawContext, GetDrawContext, sl_false)
		SLIB_SAFE_STATIC_GETTER(DrawContext, GetDrawLabelContext, sl_true)
	}

	void SwitchView::onDraw(Canvas* canvas)
	{
		DrawContext* s;
		if (m_flagTextInButton) {
			s = GetDrawLabelContext();
		} else {
			s = GetDrawContext();
		}
		if (!s) {
			return;
		}
		UIRect rect;
		if (!(calculateSwitchRegion(rect))) {
			return;
		}
		sl_uint32 value = m_value ? 1 : 0;
		ViewState state = getState();
		Ref<Drawable> track = m_tracks[value].evaluate(state);
		if (track.isNull()) {
			track = s->getTrack(value, state);
		}
		Ref<Drawable> thumb = m_thumbs[value].evaluate(state);
		if (thumb.isNull()) {
			thumb = s->getThumb(value, state);
		}

		sl_ui_len widthTrack = rect.getWidth();
		sl_ui_len widthThumb = widthTrack / 2;

		drawTrack(canvas, track, rect);

		if (thumb.isNotNull()) {
			sl_real f = m_posThumb;
			if (f < 0) {
				f = 0;
			} else if (f > 1) {
				f = 1;
			}
			UIRect rectThumb = rect;
			rectThumb.left += (sl_ui_pos)(f * (widthTrack - widthThumb));
			rectThumb.setWidth(widthThumb);
			drawThumb(canvas, thumb, rectThumb);
		}
		if (m_flagTextInButton) {
			Ref<Font> font = getFont();
			if (font.isNotNull()) {
				Color textColors[2] = { m_textColors[0], m_textColors[1] };
				if (textColors[0].isZero()) {
					textColors[0] = s->textColors[0];
				}
				if (textColors[1].isZero()) {
					textColors[1] = s->textColors[1];
				}
				UIRect rectThumb = rect;
				rectThumb.setWidth(widthThumb);
				canvas->drawText(m_texts[0], rectThumb, font, textColors[!value || isPressedState() ? 1 : 0], Alignment::MiddleCenter);
				rectThumb.left = rect.left + widthThumb;
				rectThumb.setWidth(widthThumb);
				canvas->drawText(m_texts[1], rectThumb, font, textColors[value || isPressedState() ? 1 : 0], Alignment::MiddleCenter);
			}
		} else {
			String text = m_texts[value];
			if (text.isNotEmpty()) {
				Ref<Font> font = getFont();
				if (font.isNotNull()) {
					Color textColor = m_textColors[value];
					if (textColor.isZero()) {
						textColor = s->textColors[value];
					}
					canvas->drawText(text, (sl_real)(getPaddingLeft()), (sl_real)(rect.top + (rect.getHeight() - font->getFontHeight()) / 2), font, textColor);
				}
			}
		}
	}

	void SwitchView::onUpdateLayout()
	{
		sl_bool flagHorizontal = isLastWidthWrapping();
		sl_bool flagVertical = isLastHeightWrapping();

		sl_ui_len paddingWidth = getPaddingLeft() + getPaddingRight();
		sl_ui_len paddingHeight = getPaddingTop() + getPaddingBottom();

		if (flagVertical) {
			sl_ui_len h = (sl_ui_len)(getFontSize() * 1.5f);
			setLayoutHeight(h + paddingHeight);
		}

		if (flagHorizontal) {
			sl_ui_len heightSwitch = getLayoutHeight() - paddingHeight;
			if (heightSwitch < 0) {
				heightSwitch = 0;
			}
			sl_ui_len widthText = 0;
			String texts[2] = { m_texts[0], m_texts[1] };
			if (texts[0].isNotEmpty() || texts[1].isNotEmpty()) {
				Ref<Font> font = getFont();
				if (font.isNotNull()) {
					widthText = (sl_ui_len)(Math::max(font->measureText(texts[0]).x, font->measureText(texts[1]).x));
				}
			}
			if (m_flagTextInButton) {
				if (widthText > heightSwitch * 3 / 2) {
					setLayoutWidth(widthText * 2 + heightSwitch * 3 / 2 + paddingWidth);
				} else {
					setLayoutWidth(heightSwitch * 3 + paddingWidth);
				}
			} else {
				if (widthText > 0) {
					setLayoutWidth(widthText + (sl_ui_len)(getFontSize() * 0.5f) + heightSwitch * 2 + paddingWidth);
				} else {
					setLayoutWidth(heightSwitch * 2 + paddingWidth);
				}
			}
		}
	}

	void SwitchView::onMouseEvent(UIEvent* ev)
	{
		sl_real dimUnit = Math::ceil(UI::dpToPixel(1));
		if (dimUnit < 1) {
			dimUnit = 1;
		}
		UIAction action = ev->getAction();
		if (action != UIAction::LeftButtonDrag && action != UIAction::TouchMove) {
			Ref<View> parent = getParent();
			if (parent.isNotNull()) {
				parent->setLockScroll(sl_false);
			}
		}
		switch (action) {
			case UIAction::LeftButtonDown:
			case UIAction::TouchBegin:
				{
					m_timeMouseDown = ev->getTime();
					m_ptMouseDown = ev->getPoint();
					m_posMouseDown = m_posThumb;
					m_flagTapping = sl_true;
					m_tracker.clearMovements();
					ObjectLocker lock(this);
					m_timer.setNull();
				}
				break;
			case UIAction::LeftButtonDrag:
			case UIAction::TouchMove:
				if (isPressedState()) {
					Point pt = ev->getPoint();
					m_tracker.addMovement(pt);
					sl_real dx = Math::abs(pt.x - m_ptMouseDown.x);
					if (dx > 5 * dimUnit) {
						cancelPressedStateOfChildren();
						sl_real dy = Math::abs(pt.y - m_ptMouseDown.y);
						if (dy < dx) {
							Ref<View> parent = getParent();
							if (parent.isNotNull()) {
								parent->setLockScroll(sl_true);
							}
						}
					}
					if (m_flagTapping) {
						if ((pt - m_ptMouseDown).getLength2p() > dimUnit * dimUnit * 30) {
							m_flagTapping = sl_false;
						}
					}
					UIRect rect;
					if (calculateSwitchRegion(rect)) {
						sl_real w = (sl_real)(rect.getWidth());
						sl_real f = m_posMouseDown + (pt.x - m_ptMouseDown.x) / w * 2;
						m_posThumb = Math::clamp(f, 0.0f, 1.0f);
						invalidate();
					}
				}
				break;
			case UIAction::LeftButtonUp:
			case UIAction::TouchEnd:
				if (isPressedState()) {
					if (m_flagTapping && (ev->getTime() - m_timeMouseDown).getMillisecondCount() < 250) {
						_changeValue(!m_value, ev);
					} else {
						sl_real v = 0;
						m_tracker.getVelocity(&v, sl_null);
						sl_real t = dimUnit * 10;
						if (v > t) {
							_changeValue(sl_true, ev);
						} else if (v < -t) {
							_changeValue(sl_false, ev);
						} else {
							if (m_flagTapping) {
								_changeValue(!m_value, ev);
							} else {
								if (m_posThumb > 0.5f) {
									_changeValue(sl_true, ev);
								} else {
									_changeValue(sl_false, ev);
								}
							}
						}
					}
				}
				m_tracker.clearMovements();
				break;
			case UIAction::TouchCancel:
				setValue(m_value);
				m_tracker.clearMovements();
				break;
			default:
				break;
		}

		View::onMouseEvent(ev);
	}

	sl_bool SwitchView::calculateSwitchRegion(UIRect& rect)
	{
		rect = getBoundsInnerPadding();
		if (!(rect.isValidSize())) {
			return sl_false;
		}
		if (!m_flagTextInButton) {
			rect.left = rect.right - rect.getHeight() * 2;
		}
		return sl_true;
	}

	void SwitchView::drawTrack(Canvas* canvas, const Ref<Drawable>& track, const Rectangle& rectDst)
	{
		if (track.isNull()) {
			return;
		}
		Color color;
		if (ColorDrawable::check(track.get(), &color)) {
			sl_real height = rectDst.getHeight();
			sl_real width = rectDst.getWidth();
			if (height < width) {
				height /= 2;
				canvas->fillRoundRect(rectDst, Size(height, height), color);
			} else {
				canvas->fillRectangle(rectDst, color);
			}
		} else {
			canvas->draw(rectDst, track);
		}
	}

	void SwitchView::drawThumb(Canvas* canvas, const Ref<Drawable>& thumb, const Rectangle& rectDst)
	{
		if (thumb.isNull()) {
			return;
		}
		Color color;
		if (ColorDrawable::check(thumb.get(), &color)) {
			sl_real width = rectDst.getWidth();
			sl_real height = rectDst.getHeight();
			sl_real padding = height / 20;
			if (padding < 1) {
				padding = 1;
			}
			Rectangle rect = rectDst;
			if (width > padding * 2 && height > padding * 2) {
				rect.left += padding;
				rect.right -= padding;
				rect.top += padding;
				rect.bottom -= padding;
				width = rect.getWidth();
				height = rect.getHeight();
			}
			if (height < width) {
				height /= 2;
				canvas->fillRoundRect(rect, Size(height, height), color);
			} else {
				rect.top = (rect.top + rect.bottom - width) / 2;
				rect.setHeight(width);
				canvas->fillEllipse(rect, color);
			}
		} else {
			canvas->draw(rectDst, thumb);
		}
	}

	void SwitchView::_changeValue(SwitchValue value, UIEvent* ev, UIUpdateMode mode)
	{
		ObjectLocker locker(this);
		SwitchValue former = m_value;
		m_value = value;
		sl_real posThumb = value ? 1.0f : 0.0f;
		if (!(Math::isAlmostZero(posThumb - m_posThumb))) {
			if (SLIB_UI_UPDATE_MODE_IS_ANIMATE(mode)) {
				if (m_timer.isNull()) {
					m_timer = startTimer(SLIB_FUNCTION_WEAKREF(this, _onTimerAnimation), 10);
				}
			} else {
				m_posThumb = posThumb;
				m_timer.setNull();
				invalidate(mode);
			}
		}
		locker.unlock();
		if (value != former) {
			invokeChange(value, ev);
		}
	}

	void SwitchView::_onTimerAnimation(Timer* timer)
	{
		sl_real target = m_value ? 1.0f : 0.0f;
		sl_real pos = m_posThumb;
		sl_real d = 0.1f;
		sl_real a = Math::abs(target - pos);
		if (a < d || a > 2) {
			m_posThumb = target;
			ObjectLocker lock(this);
			m_timer.setNull();
		} else {
			if (target > m_posThumb) {
				m_posThumb += d;
			} else {
				m_posThumb -= d;
			}
		}
		invalidate();
	}

}
