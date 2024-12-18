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

#include "slib/ui/mobile_app.h"

#include "slib/ui/view.h"
#include "slib/ui/resource.h"
#include "slib/ui/animation.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(MobileApp, UIApp)

	MobileApp::MobileApp()
	{
		Ref<MobileMainWindow> window = new MobileMainWindow;
		setMainWindow(window);

		m_contentView = window->getContentView();

		m_navigationController = new ViewPageNavigationController;
		m_navigationController->setWidthFilling(1, UIUpdateMode::Init);
		m_navigationController->setHeightFilling(1, UIUpdateMode::Init);
		m_navigationController->setOpaque(sl_true, UIUpdateMode::Init);
		m_navigationController->setBackgroundColor(Color::White);
		m_navigationController->setVisibility(Visibility::Hidden, UIUpdateMode::Init);
		m_contentView->addChild(m_navigationController, UIUpdateMode::Init);
	}

	MobileApp::~MobileApp()
	{
		Locale::removeOnChangeCurrentLocale(m_callbackOnChangeLocale);
	}

	void MobileApp::init()
	{
		UIApp::init();

		m_callbackOnChangeLocale = SLIB_FUNCTION_MEMBER(this, handleChangeCurrentLocale);
		Locale::addOnChangeCurrentLocale(m_callbackOnChangeLocale);
	}

	Ref<MobileApp> MobileApp::getApp()
	{
		return CastRef<MobileApp>(Application::getApp());
	}

	Ref<MobileMainWindow> MobileApp::getMainWindow()
	{
		return CastRef<MobileMainWindow>(UIApp::getMainWindow());
	}

	sl_bool MobileApp::m_flagPaused = sl_false;

	sl_bool MobileApp::isPaused()
	{
		return m_flagPaused;
	}

	Ref<View> MobileApp::getRootView()
	{
		Ref<Window> window = UIApp::getMainWindow();
		if (window.isNotNull()) {
			return window->getContentView();
		}
		return sl_null;
	}

	Ref<View> MobileApp::getContentView()
	{
		return m_contentView;
	}

	Ref<ViewPageNavigationController> MobileApp::getNavigationController()
	{
		return m_navigationController;
	}

	Ref<View> MobileApp::getLoadingPage()
	{
		return getStartupPage();
	}

	Ref<View> MobileApp::getStartupPage()
	{
		return sl_null;
	}

	void MobileApp::addViewToRoot(const Ref<View>& view)
	{
		Ref<MobileMainWindow> window = getMainWindow();
		if (window.isNotNull()) {
			window->addView(view);
		}
	}

	void MobileApp::addViewToContent(const Ref<View>& view)
	{
		Ref<View> content = m_contentView;
		if (content.isNotNull()) {
			content->addChild(view);
		}
	}

	void MobileApp::openPage(const Ref<View>& page, const Transition& transition)
	{
		m_navigationController->push(page, transition);
	}

	void MobileApp::openPage(const Ref<View>& page)
	{
		m_navigationController->push(page);
	}

	void MobileApp::openHomePage(const Ref<View>& page, const Transition& transition)
	{
		m_navigationController->pushPageAfterPopAllPages(page, transition);
	}

	void MobileApp::openHomePage(const Ref<View>& page)
	{
		m_navigationController->pushPageAfterPopAllPages(page);
	}

	void MobileApp::closePage(const Ref<View>& page, const Transition& transition)
	{
		m_navigationController->pop(page, transition);
	}

	void MobileApp::closePage(const Ref<View>& page)
	{
		m_navigationController->pop(page);
	}

	void MobileApp::closePage(const Transition& transition)
	{
		m_navigationController->pop(transition);
	}

	void MobileApp::closePage()
	{
		m_navigationController->pop();
	}

	void MobileApp::popupPage(const Ref<ViewPage>& page, const Transition& transition, sl_bool flagFillParentBackground)
	{
		if (page.isNull()) {
			return;
		}
		Ref<View> content = m_contentView;
		if (content.isNotNull()) {
			page->popup(content, transition, flagFillParentBackground);
		}
	}

	void MobileApp::popupPage(const Ref<ViewPage>& page, sl_bool flagFillParentBackground)
	{
		if (page.isNull()) {
			return;
		}
		Ref<View> content = m_contentView;
		if (content.isNotNull()) {
			page->popup(content, flagFillParentBackground);
		}
	}

	void MobileApp::closePopup(const Ref<ViewPage>& page, const Transition& transition)
	{
		if (page.isNull()) {
			return;
		}
		ListLocker< Ref<ViewPage> > popups(m_popupPages);
		if (popups.count > 0) {
			if (page == popups[popups.count-1]) {
				page->close(transition);
			}
		}
	}

	void MobileApp::closePopup(const Ref<ViewPage>& page)
	{
		if (page.isNull()) {
			return;
		}
		ListLocker< Ref<ViewPage> > popups(m_popupPages);
		if (popups.count > 0) {
			if (page == popups[popups.count-1]) {
				page->close();
			}
		}
	}

	void MobileApp::closePopup(const Transition& transition)
	{
		ListLocker< Ref<ViewPage> > popups(m_popupPages);
		if (popups.count > 0) {
			Ref<ViewPage> page = popups[popups.count-1];
			if (page.isNotNull()) {
				page->close(transition);
			}
		}
	}

	void MobileApp::closePopup()
	{
		ListLocker< Ref<ViewPage> > popups(m_popupPages);
		if (popups.count > 0) {
			Ref<ViewPage> page = popups[popups.count-1];
			if (page.isNotNull()) {
				page->close();
			}
		}
	}

	void MobileApp::openStartupPage()
	{
		Ref<View> page = getStartupPage();
		if (page.isNotNull()) {
			openHomePage(page, TransitionType::None);
		}
	}

	void MobileApp::handleStart()
	{
		UIApp::handleStart();
#ifdef SLIB_PLATFORM_IS_DESKTOP
		Ref<MobileMainWindow> window = getMainWindow();
		if (window.isNotNull()) {
			window->forceCreate();
		}
#endif
	}

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(MobileApp, Pause, ())

	void MobileApp::onPause()
	{
		Ref<ViewPageNavigationController> controller = m_navigationController;
		if (controller.isNotNull()) {
			Ref<View> page = controller->getCurrentPage();
			if (ViewPage* _page = CastInstance<ViewPage>(page.get())) {
				_page->invokePause();
			}
		}
		ListLocker< Ref<ViewPage> > popups(m_popupPages);
		for (sl_size i = 0; i < popups.count; i++) {
			Ref<ViewPage> page = popups[i];
			if (page.isNotNull()) {
				page->invokePause();
			}
		}
	}

	void MobileApp::Current::invokePause()
	{
		m_flagPaused = sl_true;
		Ref<MobileApp> app = getApp();
		if (app.isNotNull()) {
			app->invokePause();
		}
		{
			Ref<UIAnimationLoop> al = UIAnimationLoop::getInstance();
			if (al.isNotNull()) {
				al->pause();
			}
		}
		{
			Ref<AnimationLoop> al = AnimationLoop::getDefault();
			if (al.isNotNull()) {
				al->pause();
			}
		}
	}

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(MobileApp, Resume, ())

	void MobileApp::onResume()
	{
		Ref<ViewPageNavigationController> controller = m_navigationController;
		if (controller.isNotNull()) {
			Ref<View> page = controller->getCurrentPage();
			if (ViewPage* _page = CastInstance<ViewPage>(page.get())) {
				_page->invokeResume();
			}
		}
		ListLocker< Ref<ViewPage> > popups(m_popupPages);
		for (sl_size i = 0; i < popups.count; i++) {
			Ref<ViewPage> page = popups[i];
			if (page.isNotNull()) {
				page->invokeResume();
			}
		}
	}

	void MobileApp::Current::invokeResume()
	{
		m_flagPaused = sl_false;
		Ref<MobileApp> app = getApp();
		if (app.isNotNull()) {
			app->invokeResume();
		}
		{
			Ref<UIAnimationLoop> al = UIAnimationLoop::getInstance();
			if (al.isNotNull()) {
				al->resume();
			}
		}
		{
			Ref<AnimationLoop> al = AnimationLoop::getDefault();
			if (al.isNotNull()) {
				al->resume();
			}
		}
	}

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(MobileApp, PressBack, (UIEvent* ev), ev)

	void MobileApp::onPressBack(UIEvent* ev)
	{
		{
			ListLocker< Ref<ViewPage> > popups(m_popupPages);
			if (popups.count > 0) {
				Ref<ViewPage> page = popups[popups.count-1];
				if (page.isNotNull()) {
					page->invokePressBack(ev);
				}
				return;
			}
		}
		Ref<ViewPageNavigationController> controller = m_navigationController;
		if (controller.isNotNull()) {
			Ref<View> _page = controller->getCurrentPage();
			if (ViewPage* page = CastInstance<ViewPage>(_page.get())) {
				page->invokePressBack(ev);
			}
		}
	}

	sl_bool MobileApp::Current::invokePressBack()
	{
		Ref<MobileApp> app = getApp();
		if (app.isNotNull()) {
			Ref<UIEvent> ev = UIEvent::createUnknown(Time::now());
			if (ev.isNotNull()) {
				app->invokePressBack(ev.get());
				if (ev->isAccepted()) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	SLIB_DEFINE_EVENT_HANDLER(MobileApp, CreateActivity, ())

	void MobileApp::handleCreateActivity()
	{
		invokeCreateActivity();
		Ref<MobileMainWindow> window = getMainWindow();
		if (window.isNotNull()) {
			window->forceCreate();
		}
	}

	void MobileApp::Current::invokeCreateActivity()
	{
		Ref<MobileApp> app = getApp();
		if (app.isNotNull()) {
			app->handleCreateActivity();
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(MobileApp, DestroyActivity, ())

	void MobileApp::Current::invokeDestroyActivity()
	{
		Ref<MobileApp> app = getApp();
		if (app.isNotNull()) {
			app->invokeDestroyActivity();
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(MobileApp, Resize, (sl_ui_len width, sl_ui_len height), width, height)

	void MobileApp::handleResize(sl_ui_len width, sl_ui_len height)
	{
		UIResource::updateDefaultScreenSize();

		invokeResize(width, height);

		if (m_navigationController->getPageCount() == 0) {
			Ref<View> page = getLoadingPage();
			if (page.isNotNull()) {
				m_navigationController->setVisibility(Visibility::Visible);
				openHomePage(page, TransitionType::None);
			}
		}
	}

	void MobileApp::Current::invokeResize(sl_ui_len width, sl_ui_len height)
	{
		Ref<MobileApp> app = getApp();
		if (app.isNotNull()) {
			app->handleResize(width, height);
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(MobileApp, ChangeCurrentLocale, ())

	void MobileApp::handleChangeCurrentLocale()
	{
		invokeChangeCurrentLocale();
		if (m_navigationController->getPageCount() > 0) {
			openStartupPage();
		}
	}

	SLIB_DEFINE_OBJECT(MobileMainWindow, Window)

	MobileMainWindow::MobileMainWindow()
	{
	}

	MobileMainWindow::~MobileMainWindow()
	{
	}

	void MobileMainWindow::onResize(sl_ui_len width, sl_ui_len height)
	{
		Window::onResize(width, height);
		MobileApp::Current::invokeResize(width, height);
	}


	namespace {
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicList<ScreenOrientation>, g_listAvailableScreenOrientations)
	}

	List<ScreenOrientation> MobileApp::getAvailableScreenOrientations()
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_listAvailableScreenOrientations)) {
			return sl_null;
		}
		return g_listAvailableScreenOrientations;
	}

	void MobileApp::setAvailableScreenOrientations(const List<ScreenOrientation>& orientations)
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_listAvailableScreenOrientations)) {
			return;
		}
		g_listAvailableScreenOrientations = orientations;
		attemptRotateScreenOrientation();
	}

	void MobileApp::setAvailableScreenOrientation(const ScreenOrientation& orientation)
	{
		setAvailableScreenOrientations(List<ScreenOrientation>::createFromElements(orientation));
	}

	void MobileApp::setAvailableScreenOrientationsPortrait()
	{
		setAvailableScreenOrientations(List<ScreenOrientation>::createFromElements(ScreenOrientation::Portrait, ScreenOrientation::PortraitUpsideDown));
	}

	void MobileApp::setAvailableScreenOrientationsLandscape()
	{
		setAvailableScreenOrientations(List<ScreenOrientation>::createFromElements(ScreenOrientation::LandscapeRight, ScreenOrientation::LandscapeLeft));
	}

	void MobileApp::setAvailableScreenOrientationsAll()
	{
		setAvailableScreenOrientations(sl_null);
	}

#if !defined(SLIB_UI_IS_IOS) && !defined(SLIB_UI_IS_ANDROID)
	ScreenOrientation MobileApp::getScreenOrientation()
	{
		return ScreenOrientation::Portrait;
	}

	void MobileApp::attemptRotateScreenOrientation()
	{
	}

	sl_ui_len MobileApp::getStatusBarHeight()
	{
		return 0;
	}

	void MobileApp::setStatusBarStyle(StatusBarStyle style)
	{
	}

	UIEdgeInsets MobileApp::getSafeAreaInsets()
	{
		UIEdgeInsets ret;
		ret.left = 0;
		ret.top = getStatusBarHeight();
		ret.right = 0;
		ret.bottom = 0;
		return ret;
	}
#endif

	namespace {
		static UIKeyboardAdjustMode g_keyboardAdjustMode = UIKeyboardAdjustMode::Pan;
	}

	UIKeyboardAdjustMode MobileApp::getKeyboardAdjustMode()
	{
		return g_keyboardAdjustMode;
	}

#if defined(SLIB_UI_IS_ANDROID)
	namespace priv
	{
		void UpdateKeyboardAdjustMode(UIKeyboardAdjustMode mode);
	}
#endif

	void MobileApp::setKeyboardAdjustMode(UIKeyboardAdjustMode mode)
	{
		g_keyboardAdjustMode = mode;
#if defined(SLIB_UI_IS_ANDROID)
		priv::UpdateKeyboardAdjustMode(mode);
#endif
	}

}
