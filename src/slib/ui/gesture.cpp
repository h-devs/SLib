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

#include "slib/ui/gesture.h"

#include "slib/ui/view.h"
#include "slib/ui/resource.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(GestureDetector, Object)

	class GestureRecognizer : public CRef
	{
	public:
		GestureDetector* m_detector;

	public:
		GestureRecognizer(GestureDetector* detector) : m_detector(detector)
		{
		}

		virtual void processEvent(UIEvent* ev, const Ref<View>& view, MotionTracker* tracker)
		{
		}

	};

	class SwipeGestureRecognizer : public GestureRecognizer
	{
	public:
		GestureType type;
		sl_bool flagHorizontal;
		sl_bool flagPositive;

	public:
		SwipeGestureRecognizer(GestureDetector* detector, GestureType _type)
		: GestureRecognizer(detector)
		{
			type = _type;
			switch (type) {
				case GestureType::SwipeLeft:
					flagHorizontal = sl_true;
					flagPositive = sl_false;
					break;
				case GestureType::SwipeRight:
					flagHorizontal = sl_true;
					flagPositive = sl_true;
					break;
				case GestureType::SwipeUp:
					flagHorizontal = sl_false;
					flagPositive = sl_false;
					break;
				default:
					flagHorizontal = sl_false;
					flagPositive = sl_true;
					break;
			}
		}

		void processEvent(UIEvent* ev, const Ref<View>& view, MotionTracker* tracker) override
		{
			UIAction action = ev->getAction();
			if (action == UIAction::LeftButtonUp || action == UIAction::TouchEnd || action == UIAction::TouchCancel) {
				sl_real t = (sl_real)(UIResource::getScreenMinimum()) * 0.2f;
				if (t < 5) {
					return;
				}
				sl_real vx, vy;
				if (tracker->getVelocity(&vx, &vy)) {
					if (flagHorizontal) {
						if (Math::abs(vx) > Math::abs(vy)) {
							vx /= t;
							if (!flagPositive) {
								vx = -vx;
							}
							if (vx > 1) {
								GestureEvent ev;
								ev.type = type;
								view->invokeSwipe(&ev);
							}
						}
					} else {
						if (Math::abs(vy) > Math::abs(vx)) {
							vy /= t;
							if (!flagPositive) {
								vy = -vy;
							}
							if (vy > 1) {
								GestureEvent ev;
								ev.type = type;
								view->invokeSwipe(&ev);
							}
						}
					}
				}
			}
		}
	};

	GestureDetector::GestureDetector(const Ref<View>& view) : m_view(view)
	{
	}

	GestureDetector::~GestureDetector()
	{
	}

	void GestureDetector::enable(GestureType type)
	{
		ObjectLocker lock(this);
		Ref<View> view = m_view;
		if (view.isNull()) {
			return;
		}
		_enableNative(view, type);
		if (m_recognizers[(int)type].isNotNull()) {
			return;
		}
		Ref<GestureRecognizer> recognizer;
		switch (type) {
			case GestureType::SwipeLeft:
			case GestureType::SwipeRight:
			case GestureType::SwipeUp:
			case GestureType::SwipeDown:
				recognizer = new SwipeGestureRecognizer(this, type);
				break;
			default:
				break;
		}
		m_recognizers[(int)type] = recognizer;
	}

	void GestureDetector::enableNative()
	{
		Ref<View> view = m_view;
		if (view.isNull()) {
			return;
		}
		int nRecognizers = (int)(GestureType::Count);
		for (int i = 0; i < nRecognizers; i++) {
			if (m_recognizers[i].isNotNull()) {
				_enableNative(view, (GestureType)i);
			}
		}
	}

	void GestureDetector::processEvent(UIEvent* ev)
	{
		ObjectLocker lock(this);
		Ref<View> view = m_view;
		if (view.isNull()) {
			return;
		}
		int i;
		int nRecognizers = (int)(GestureType::Count);
		sl_bool flagProcess = sl_false;
		sl_bool flagNative[(int)(GestureType::Count)];
		for (i = 0; i < nRecognizers; i++) {
			if (m_recognizers[i].isNotNull()) {
				flagNative[i] = _enableNative(view, (GestureType)i);
				if (!(flagNative[i])) {
					flagProcess = sl_true;
				}
			}
		}
		if (!flagProcess) {
			return;
		}
		UIAction action = ev->getAction();
		switch (action) {
			case UIAction::LeftButtonDown:
			case UIAction::TouchBegin:
				m_tracker.clearMovements();
				m_tracker.addMovement(ev->getPoint());
				break;
			case UIAction::LeftButtonDrag:
			case UIAction::TouchMove:
			case UIAction::LeftButtonUp:
			case UIAction::TouchEnd:
				m_tracker.addMovement(ev->getPoint());
				break;
			case UIAction::TouchCancel:
				m_tracker.clearMovements();
				return;
			default:
				break;
		}
		for (i = 0; i < nRecognizers; i++) {
			if (m_recognizers[i].isNotNull() && !(flagNative[i])) {
				m_recognizers[i]->processEvent(ev, view, &m_tracker);
			}
		}
		if (action == UIAction::LeftButtonUp || action == UIAction::TouchEnd) {
			m_tracker.clearMovements();
		}
	}

#if !(defined(SLIB_UI_IS_IOS)) && !(defined(SLIB_UI_IS_ANDROID))
	sl_bool GestureDetector::_enableNative(const Ref<View>& view, GestureType type)
	{
		return sl_false;
	}
#endif

}
