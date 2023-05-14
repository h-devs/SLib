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

#include "slib/ui/view_page.h"

#include "slib/ui/mobile_app.h"
#include "slib/ui/core.h"

namespace slib
{

	namespace {
		static TransitionType g_defaultOpeningPopupTransitionType = TransitionType::Zoom;
		static TransitionDirection g_defaultOpeningPopupTransitionDirection = TransitionDirection::FromBottomToTop;
		static float g_defaultOpeningPopupTransitionDuration = 0.25f;
		static AnimationCurve g_defaultOpeningPopupTransitionCurve = AnimationCurve::EaseOut;
		static TransitionType g_defaultClosingPopupTransitionType = TransitionType::Fade;
		static TransitionDirection g_defaultClosingPopupTransitionDirection = TransitionDirection::FromTopToBottom;
		static float g_defaultClosingPopupTransitionDuration = 0.2f;
		static AnimationCurve g_defaultClosingPopupTransitionCurve = AnimationCurve::Linear;
	}

	SLIB_DEFINE_OBJECT(ViewPage, ViewGroup)

	ViewPage::ViewPage()
	{
		setCreatingInstance(sl_true);

		m_popupState = PopupState::None;
		m_popupBackgroundColor = Color::zero();

		m_countActiveTransitionAnimations = 0;

		setBackgroundColor(Color::White, UIUpdateMode::Init);
	}

	ViewPage::~ViewPage()
	{
	}

	Ref<ViewPageNavigationController> ViewPage::getNavigationController()
	{
		return m_navigationController;
	}

	void ViewPage::setNavigationController(const Ref<ViewPageNavigationController>& controller)
	{
		m_navigationController = controller;
	}

	void ViewPage::setTransition(const Transition& opening, const Transition& closing)
	{
		m_openingTransition = opening;
		m_closingTransition = closing;
	}

	void ViewPage::open(const Ref<ViewPageNavigationController>& controller, const Transition& transition)
	{
		if (controller.isNotNull()) {
			controller->push(this, transition);
		}
	}

	void ViewPage::open(const Ref<ViewPageNavigationController>& controller)
	{
		if (controller.isNotNull()) {
			controller->push(this);
		}
	}

	void ViewPage::openHome(const Ref<ViewPageNavigationController>& controller, const Transition& transition)
	{
		if (controller.isNotNull()) {
			controller->pushPageAfterPopAllPages(this, transition);
		}
	}

	void ViewPage::openHome(const Ref<ViewPageNavigationController>& controller)
	{
		if (controller.isNotNull()) {
			controller->pushPageAfterPopAllPages(this);
		}
	}

	void ViewPage::close(const Transition& transition)
	{
		ObjectLocker lock(this);
		if (m_popupState == PopupState::ShowWindow) {
			m_popupState = PopupState::None;
			Ref<Window> window = getWindow();
			lock.unlock();
			invokePause();
			invokeClose();
			if (window.isNotNull()) {
				window->close();
			}
		} else if (m_popupState == PopupState::Popup) {
			m_popupState = PopupState::ClosingPopup;
			Ref<MobileApp> mobile = MobileApp::getApp();
			if (mobile.isNotNull()) {
				mobile->m_popupPages.remove(this);
			}
			if (isDrawingThread()) {
				_closePopup(transition);
			} else {
				dispatchToDrawingThread(SLIB_BIND_WEAKREF(void(), this, _closePopup, transition));
			}
		} else {
			Ref<ViewPageNavigationController> controller = getNavigationController();
			if (controller.isNotNull()) {
				controller->pop(this, transition);
			}
		}
	}

	void ViewPage::close()
	{
		close(m_closingTransition);
	}

	void ViewPage::goToPage(const Ref<View>& page, const Transition& transition)
	{
		Ref<ViewPageNavigationController> controller = m_navigationController;
		if (controller.isNotNull()) {
			controller->push(page, transition);
		}
	}

	void ViewPage::goToPage(const Ref<View>& page)
	{
		Ref<ViewPageNavigationController> controller = m_navigationController;
		if (controller.isNotNull()) {
			controller->push(page);
		}
	}

	void ViewPage::goToHomePage(const Ref<View>& page, const Transition& transition)
	{
		Ref<ViewPageNavigationController> controller = m_navigationController;
		if (controller.isNotNull()) {
			controller->pushPageAfterPopAllPages(page, transition);
		}
	}

	void ViewPage::goToHomePage(const Ref<View>& page)
	{
		Ref<ViewPageNavigationController> controller = m_navigationController;
		if (controller.isNotNull()) {
			controller->pushPageAfterPopAllPages(page);
		}
	}

	Ref<Window> ViewPage::createNavigationWindow()
	{
		Ref<ViewPageNavigationController> pager = new ViewPageNavigationController;
		if (pager.isNull()) {
			return sl_null;
		}
		Ref<Window> window = new Window;
		if (window.isNull()) {
			return sl_null;
		}
		pager->setWidthFilling(1, UIUpdateMode::Init);
		pager->setHeightFilling(1, UIUpdateMode::Init);
		pager->push(this);
		window->addView(pager, UIUpdateMode::Init);
		window->setOnCancel([pager](Window* window) {
			if (pager->getPageCount() > 1) {
				pager->pop();
			}
		});
		return window;
	}

	namespace {
		class PopupBackground : public ViewGroup
		{
			SLIB_DECLARE_OBJECT
		};

		SLIB_DEFINE_OBJECT(PopupBackground, ViewGroup)
	}

	void ViewPage::_openPopup(const Ref<View>& parent, Transition transition, sl_bool flagFillParentBackground)
	{
		ObjectLocker lock(this);

		if (m_countActiveTransitionAnimations) {
			dispatchToDrawingThread(SLIB_BIND_WEAKREF(void(), this, _openPopup, parent, transition, flagFillParentBackground), 100);
			return;
		}

		Ref<View> viewAdd;
		if (flagFillParentBackground) {
			Ref<PopupBackground> back = new PopupBackground;
			back->setCreatingInstance(sl_true);
			Color color = m_popupBackgroundColor;
			if (color.isZero()) {
				color = getDefaultPopupBackgroundColor();
			}
			back->setBackgroundColor(color);
			back->setWidthFilling(1, UIUpdateMode::Init);
			back->setHeightFilling(1, UIUpdateMode::Init);
			WeakRef<ViewPage> page = this;
			back->setOnClickEvent([page](View* view, UIEvent* ev) {
				if (view->getChildAt(ev->getPoint()).isNotNull()) {
					return;
				}
				Ref<ViewPage> _page = page;
				if (_page.isNotNull()) {
					_page->invokeClickBackground(ev);
				}
			});
			back->setOnTouchEvent([](View* view, UIEvent* ev) {
				ev->accept();
			});
			back->addChild(this, UIUpdateMode::Init);
			viewAdd = back;
		} else {
			viewAdd = this;
		}

		setVisibility(Visibility::Hidden, UIUpdateMode::None);
		setTranslation(0, 0, UIUpdateMode::Init);
		setScale(1, 1, UIUpdateMode::Init);
		setRotation(0, UIUpdateMode::None);
		setAlpha(1, UIUpdateMode::None);

		_applyDefaultOpeningPopupTransition(transition);

		setEnabled(sl_false, UIUpdateMode::None);

		Ref<Animation> animation = Transition::createPopupAnimation(this, transition, UIPageAction::Push, SLIB_BIND_WEAKREF(void(), this, _finishPopupAnimation, UIPageAction::Push));

		parent->addChild(viewAdd);

		Base::interlockedIncrement(&m_countActiveTransitionAnimations);

		invokeOpen();

		invokeResume();

		if (animation.isNotNull()) {
			animation->invokeStartFrame();
		}

		setVisibility(Visibility::Visible);

		if (animation.isNull()) {
			_finishPopupAnimation(UIPageAction::Push);
		} else {
			ViewPageNavigationController::_runAnimationProc(this, [animation]() {
				animation->start();
			});
		}
	}

	void ViewPage::_closePopup(Transition transition)
	{

		ObjectLocker lock(this);

		if (m_countActiveTransitionAnimations) {
			dispatchToDrawingThread(SLIB_BIND_WEAKREF(void(), this, _closePopup, transition), 100);
			return;
		}

#if defined(SLIB_UI_IS_ANDROID)
		UI::dismissKeyboard();
#endif

		_applyDefaultClosingPopupTransition(transition);

		setEnabled(sl_false, UIUpdateMode::None);

		Ref<View> parent = getParent();
		if (parent.isNotNull()) {
			if (IsInstanceOf<PopupBackground>(parent)) {
				parent->setBackgroundColor(Color::zero());
			}
		}

		Ref<Animation> animation = Transition::createPopupAnimation(this, transition, UIPageAction::Pop, SLIB_BIND_WEAKREF(void(), this, _finishPopupAnimation, UIPageAction::Pop));

		Base::interlockedIncrement(&m_countActiveTransitionAnimations);

		invokePause();

		invokeClose();

		if (animation.isNotNull()) {
			animation->start();
		}

		if (animation.isNull()) {
			_finishPopupAnimation(UIPageAction::Pop);
		}

	}

	void ViewPage::_finishPopupAnimation(UIPageAction action)
	{
		ObjectLocker lock(this);

		invokeEndPageAnimation(sl_null, action);

		if (action == UIPageAction::Pop) {

			Ref<View> parent = getParent();
			if (parent.isNotNull()) {
				if (IsInstanceOf<PopupBackground>(parent)) {
					Ref<View> parent2 = parent->getParent();
					if (parent2.isNotNull()) {
						parent2->removeChild(parent);
					}
				} else {
					parent->removeChild(this);
				}
			}

			m_popupState = PopupState::None;

		} else {
			setEnabled(sl_true, UIUpdateMode::None);
		}

		Base::interlockedDecrement(&m_countActiveTransitionAnimations);
	}

	void ViewPage::popup(const Ref<View>& parent, const Transition& transition, sl_bool flagFillParentBackground)
	{
		if (parent.isNull()) {
			return;
		}
		ObjectLocker lock(this);
		if (m_popupState != PopupState::None) {
			return;
		}

		Ref<MobileApp> mobile = MobileApp::getApp();
		if (mobile.isNotNull()) {
			mobile->m_popupPages.add(this);
		}

		if (isDrawingThread()) {
			_openPopup(parent, transition, flagFillParentBackground);
		} else {
			dispatchToDrawingThread(SLIB_BIND_WEAKREF(void(), this, _openPopup, parent, transition, flagFillParentBackground));
		}
		m_popupState = PopupState::Popup;
	}

	void ViewPage::popup(const Ref<View>& parent, sl_bool flagFillParentBackground)
	{
		popup(parent, m_openingTransition, flagFillParentBackground);
	}

	Ref<Window> ViewPage::popupWindow(const Ref<Window>& parent, sl_ui_len width, sl_ui_len height)
	{
		ObjectLocker lock(this);

		if (m_popupState != PopupState::None) {
			return sl_null;
		}

		Ref<Window> window = new Window;

		if (window.isNotNull()) {
			if (isWidthWrapping()) {
				window->setWidthWrapping(sl_true, UIUpdateMode::Init);
			}
			if (isHeightWrapping()) {
				window->setHeightWrapping(sl_true, UIUpdateMode::Init);
			}
			sl_ui_len contentWidth = width;
			if (!contentWidth) {
				if (isWidthFilling() || isWidthWeight()) {
					contentWidth = UI::getScreenWidth();
				} else {
					contentWidth = getWidth();
				}
			}
			sl_ui_len contentHeight = height;
			if (!contentHeight) {
				if (isHeightFilling() || isHeightWeight()) {
					contentHeight = UI::getScreenHeight();
				} else {
					contentHeight = getHeight();
				}
			}
			window->setClientSize(contentWidth, contentHeight);
			if (!width || !height) {
				if (isWidthFilling() && isHeightFilling()) {
					window->setFullScreen(sl_true);
				}
			}
			window->addView(this, UIUpdateMode::Init);
			window->setParent(parent);
			window->setDialog(sl_true);
			if (isCenterVertical() && isCenterHorizontal()) {
				window->setCenterScreen(sl_true);
			} else {
				window->setLeft(getLeft());
				window->setTop(getTop());
			}
			window->setModal(sl_true);
			window->setOnClose(SLIB_FUNCTION_WEAKREF(this, _onClosePopupWindow));

			window->create();

			m_popupState = PopupState::ShowWindow;

			lock.unlock();

			invokeOpen();

			invokeResume();

			return window;

		}

		return sl_null;
	}

	void ViewPage::_onClosePopupWindow(Window* window, UIEvent* ev)
	{
		ObjectLocker lock(this);
		if (m_popupState == PopupState::ShowWindow) {
			invokeBack(ev);
			if (ev->isPreventedDefault()) {
				return;
			}
			m_popupState = PopupState::None;
			lock.unlock();
			invokePause();
			invokeClose();
		}
	}

	sl_bool ViewPage::isPopup()
	{
		return m_popupState == PopupState::Popup;
	}

	Color ViewPage::getPopupBackgroundColor()
	{
		return m_popupBackgroundColor;
	}

	void ViewPage::setPopupBackgroundColor(const Color& color)
	{
		m_popupBackgroundColor = color;
	}

	void ViewPage::setCloseOnClickBackground()
	{
		setOnClickBackground([](ViewPage* page, UIEvent* ev) {
			page->invokeBack(ev);
			if (ev->isPreventedDefault()) {
				return;
			}
			page->close();
		});
	}

	void ViewPage::setDefaultPopupTransition(const Transition& opening, const Transition& closing)
	{
		if (opening.type != TransitionType::Default) {
			g_defaultOpeningPopupTransitionType = opening.type;
		}
		if (opening.direction != TransitionDirection::Default) {
			g_defaultOpeningPopupTransitionDirection = opening.direction;
		}
		if (opening.duration > 0) {
			g_defaultOpeningPopupTransitionDuration = opening.duration;
		}
		if (opening.curve != AnimationCurve::Default) {
			g_defaultOpeningPopupTransitionCurve = opening.curve;
		}
		if (closing.type != TransitionType::Default) {
			g_defaultClosingPopupTransitionType = closing.type;
		}
		if (closing.direction != TransitionDirection::Default) {
			g_defaultClosingPopupTransitionDirection = closing.direction;
		}
		if (closing.duration > 0) {
			g_defaultClosingPopupTransitionDuration = closing.duration;
		}
		if (closing.curve != AnimationCurve::Default) {
			g_defaultClosingPopupTransitionCurve = closing.curve;
		}
	}

	SLIB_STATIC_COLOR(g_defaultPopupBackgroundColor, 0, 0, 0, 120)

	Color ViewPage::getDefaultPopupBackgroundColor()
	{
		return g_defaultPopupBackgroundColor;
	}

	void ViewPage::setDefaultPopupBackgroundColor(const Color& color)
	{
		g_defaultPopupBackgroundColor = color;
	}

	void ViewPage::_applyDefaultOpeningPopupTransition(Transition& transition)
	{
		if (transition.type == TransitionType::Default) {
			transition.type = g_defaultOpeningPopupTransitionType;
		}
		if (transition.direction == TransitionDirection::Default) {
			transition.direction = g_defaultOpeningPopupTransitionDirection;
		}
		if (transition.duration <= 0) {
			transition.duration = g_defaultOpeningPopupTransitionDuration;
		}
		if (transition.curve == AnimationCurve::Default) {
			transition.curve = g_defaultOpeningPopupTransitionCurve;
		}
	}

	void ViewPage::_applyDefaultClosingPopupTransition(Transition& transition)
	{
		if (transition.type == TransitionType::Default) {
			transition.type = g_defaultClosingPopupTransitionType;
		}
		if (transition.direction == TransitionDirection::Default) {
			transition.direction = g_defaultClosingPopupTransitionDirection;
		}
		if (transition.duration < SLIB_EPSILON) {
			transition.duration = g_defaultClosingPopupTransitionDuration;
		}
		if (transition.curve == AnimationCurve::Default) {
			transition.curve = g_defaultClosingPopupTransitionCurve;
		}
	}


	SLIB_DEFINE_EVENT_HANDLER(ViewPage, Open, ())

	SLIB_DEFINE_EVENT_HANDLER(ViewPage, Close, ())

	SLIB_DEFINE_EVENT_HANDLER(ViewPage, Resume, ())

	SLIB_DEFINE_EVENT_HANDLER(ViewPage, Pause, ())

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(ViewPage, PageAction, (ViewPageNavigationController* controller, UIPageAction action), controller, action)

	void ViewPage::onPageAction(ViewPageNavigationController* controller, UIPageAction action)
	{
		switch (action) {
			case UIPageAction::Push:
				invokeOpen();
				break;
			case UIPageAction::Pop:
				invokeClose();
				break;
			case UIPageAction::Resume:
				invokeResume();
				break;
			case UIPageAction::Pause:
				invokePause();
				break;
		}
	}

	void ViewPage::handlePageAction(ViewPageNavigationController* controller, UIPageAction action)
	{
		m_navigationController = controller;
		invokePageAction(controller, action);
	}

	SLIB_DEFINE_EVENT_HANDLER(ViewPage, EndPageAnimation, (ViewPageNavigationController* controller, UIPageAction action), controller, action)

	void ViewPage::handleEndPageAnimation(ViewPageNavigationController* controller, UIPageAction action)
	{
		m_navigationController = controller;
		if (action == UIPageAction::Resume || action == UIPageAction::Push) {
			Ref<View> focus = getFocalDescendant();
			if (focus.isNotNull()) {
				focus->setFocus();
			}
		}
		invokeEndPageAnimation(controller, action);
	}

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(ViewPage, PressBack, (UIEvent* ev), ev)

	void ViewPage::onPressBack(UIEvent* ev)
	{
		invokeBack(ev);
	}

	SLIB_DEFINE_EVENT_HANDLER(ViewPage, Back, (UIEvent* ev), ev)

	SLIB_DEFINE_EVENT_HANDLER(ViewPage, ClickBackground, (UIEvent* ev), ev)

	void ViewPage::onCancel()
	{
		Ref<UIEvent> ev = UIEvent::createUnknown(Time::now());
		if (ev.isNull()) {
			return;
		}
		invokeBack(ev.get());
		if (ev->isPreventedDefault()) {
			return;
		}
		Ref<ViewPageNavigationController> controller = getNavigationController();
		if (controller.isNotNull()) {
			if (controller->getPageCount() > 1) {
				close();
				return;
			}
		}
		ViewGroup::onCancel();
	}

}
