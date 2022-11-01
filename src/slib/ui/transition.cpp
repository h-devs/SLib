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

#include "slib/ui/transition.h"

#include "slib/ui/view.h"
#include "slib/ui/core.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(Transition)

	Transition::Transition():
		type(TransitionType::Default),
		direction(TransitionDirection::Default),
		duration(0),
		curve(AnimationCurve::Default)
	{
	}

	Transition::Transition(TransitionType _type):
		type(_type),
		direction(TransitionDirection::Default),
		duration(0),
		curve(AnimationCurve::Default)
	{
	}

	Transition::Transition(TransitionType _type, TransitionDirection _direction, float _duration, AnimationCurve _curve):
		type(_type),
		direction(_direction),
		duration(_duration),
		curve(_curve)
	{
	}

	Ref<Animation> Transition::start(const Ref<View>& view, const Transition& transition, UIPageAction pageAction, const Function<void()>& onStop)
	{
		if (view.isNull()) {
			return sl_null;
		}
		Ref<Animation> animation = createAnimation(view, transition, pageAction, onStop);
		if (animation.isNotNull()) {
			animation->start();
			return animation;
		} else {
			onStop();
			return sl_null;
		}
	}

	Ref<Animation> Transition::createAnimation(const Ref<View>& view, const Transition& transition, UIPageAction pageAction, const Function<void()>& onStop)
	{
		if (view.isNull()) {
			return sl_null;
		}

		TransitionType type = transition.type;
		if (type == TransitionType::Default || type == TransitionType::None) {
			return sl_null;
		}

		float duration = transition.duration;
		if (duration < SLIB_EPSILON) {
			return sl_null;
		}

		TransitionDirection direction = transition.direction;
		AnimationCurve curve = transition.curve;

		Ref<Animation> animation;
		switch (type) {
			case TransitionType::None:
			case TransitionType::Default:
				break;
			case TransitionType::Push:
			case TransitionType::Slide:
			case TransitionType::Cover:
				{
					sl_real factor;
					if (type == TransitionType::Push) {
						factor = 1;
					} else if (type == TransitionType::Slide) {
						if (pageAction == UIPageAction::Push || pageAction == UIPageAction::Pop) {
							factor = 1;
						} else {
							factor = 0.5f;
						}
					} else if (type == TransitionType::Cover) {
						if (pageAction == UIPageAction::Push || pageAction == UIPageAction::Pop) {
							factor = 1;
						} else {
							factor = 0;
						}
					} else {
						factor = 0;
					}
					sl_real startFactor;
					sl_real endFactor;
					if (pageAction == UIPageAction::Push || pageAction == UIPageAction::Resume) {
						startFactor = factor;
						endFactor = 0;
					} else {
						startFactor = 0;
						endFactor = -factor;
					}
					if (direction == TransitionDirection::FromLeftToRight) {
						direction = TransitionDirection::FromRightToLeft;
						startFactor = -startFactor;
						endFactor = -endFactor;
					} else if (direction == TransitionDirection::FromTopToBottom) {
						direction = TransitionDirection::FromBottomToTop;
						startFactor = -startFactor;
						endFactor = -endFactor;
					}
					if (Math::isAlmostZero(factor)) {
						animation = view->createAnimation(duration);
						if (animation.isNotNull()) {
							animation->setOnStop(onStop);
						}
					} else {
						if (direction == TransitionDirection::FromRightToLeft) {
							sl_real width = (sl_real)(view->getWidth());
							animation = view->createTranslateAnimation(Vector2(width * startFactor, 0), Vector2(width * endFactor, 0), duration, onStop, curve);
						} else {
							sl_real height = (sl_real)(view->getHeight());
							animation = view->createTranslateAnimation(Vector2(0, height * startFactor), Vector2(0, height * endFactor), duration, onStop, curve);
						}
					}
				}
				break;
			case TransitionType::Zoom:
				if (pageAction == UIPageAction::Push || pageAction == UIPageAction::Resume) {
					animation = view->createScaleAnimation(0.5f, 1.0f, duration, onStop, curve);
				} else {
					animation = view->createScaleAnimation(1.0f, 0.5f, duration, onStop, curve);
				}
				break;
			case TransitionType::Fade:
				if (pageAction == UIPageAction::Push) {
					animation = view->createAlphaAnimation(0.01f, 1.0f, duration, onStop, curve);
				} else if (pageAction == UIPageAction::Pop) {
					animation = view->createAlphaAnimation(1.0f, 0.01f, duration, onStop, curve);
				} else {
					animation = view->createAnimation(duration);
					if (animation.isNotNull()) {
						animation->setOnStop(onStop);
					}
				}
				break;
			case TransitionType::FadeFrontAndBack:
				if (pageAction == UIPageAction::Push || pageAction == UIPageAction::Resume) {
					animation = view->createAlphaAnimation(0.01f, 1.0f, duration, onStop, curve);
				} else {
					animation = view->createAlphaAnimation(1.0f, 0.01f, duration, onStop, curve);
				}
				break;
		}
		return animation;
	}

	Ref<Animation> Transition::startPopup(const Ref<View>& view, const Transition& transition, UIPageAction pageAction, const Function<void()>& onStop)
	{
		if (view.isNull()) {
			return sl_null;
		}
		Ref<Animation> animation = createPopupAnimation(view, transition, pageAction, onStop);
		if (animation.isNotNull()) {
			animation->start();
			return animation;
		} else {
			onStop();
			return sl_null;
		}
	}

	Ref<Animation> Transition::createPopupAnimation(const Ref<View>& view, const Transition& transition, UIPageAction pageAction, const Function<void()>& onStop)
	{
		if (view.isNull()) {
			return sl_null;
		}

		TransitionType type = transition.type;
		if (type == TransitionType::Default || type == TransitionType::None) {
			return sl_null;
		}

		float duration = transition.duration;
		if (duration < SLIB_EPSILON) {
			return sl_null;
		}

		if (pageAction != UIPageAction::Push && pageAction != UIPageAction::Pop) {
			return sl_null;
		}

		TransitionDirection direction = transition.direction;
		AnimationCurve curve = transition.curve;

		Ref<Animation> animation;
		switch (type) {
			case TransitionType::None:
			case TransitionType::Default:
				break;
			case TransitionType::Push:
			case TransitionType::Slide:
			case TransitionType::Cover:
			{
				sl_real startFactor;
				sl_real endFactor;
				if (pageAction == UIPageAction::Push) {
					startFactor = 1;
					endFactor = 0;
				} else {
					startFactor = 0;
					endFactor = -1;
				}
				if (direction == TransitionDirection::FromLeftToRight) {
					direction = TransitionDirection::FromRightToLeft;
					startFactor = -startFactor;
					endFactor = -endFactor;
				} else if (direction == TransitionDirection::FromTopToBottom) {
					direction = TransitionDirection::FromBottomToTop;
					startFactor = -startFactor;
					endFactor = -endFactor;
				}
				if (direction == TransitionDirection::FromRightToLeft) {
					sl_real width = (sl_real)(UI::getScreenWidth());
					animation = view->createTranslateAnimation(Vector2(width * startFactor, 0), Vector2(width * endFactor, 0), duration, onStop, curve);
				} else {
					sl_real height = (sl_real)(UI::getScreenHeight());
					animation = view->createTranslateAnimation(Vector2(0, height * startFactor), Vector2(0, height * endFactor), duration, onStop, curve);
				}
			}
				break;
			case TransitionType::Zoom:
				if (pageAction == UIPageAction::Push) {
					animation = view->createScaleAnimation(0.5f, 1.0f, duration, onStop, curve);
				} else {
					animation = view->createScaleAnimation(1.0f, 0.5f, duration, onStop, curve);
				}
				break;
			case TransitionType::Fade:
			case TransitionType::FadeFrontAndBack:
				if (pageAction == UIPageAction::Push) {
					animation = view->createAlphaAnimation(0.01f, 1.0f, duration, onStop, curve);
				} else {
					animation = view->createAlphaAnimation(1.0f, 0.01f, duration, onStop, curve);
				}
				break;
		}
		return animation;
	}

}
