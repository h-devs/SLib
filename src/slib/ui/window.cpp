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

#include "slib/ui/window.h"

#include "slib/ui/core.h"
#include "slib/ui/screen.h"
#include "slib/core/variant.h"
#include "slib/core/timer.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(Window, Object)

	Window::Window()
	{
		m_frame.left = 100;
		m_frame.top = 100;
		m_frame.right = 500;
		m_frame.bottom = 400;

		m_backgroundColor = Color::zero();
		m_flagDefaultBackgroundColor = sl_true;

		m_alpha = 1.0;

		m_sizeMin.x = 0;
		m_sizeMin.y = 0;
		m_sizeMax.x = 0;
		m_sizeMax.y = 0;
		m_aspectRatioMinimum = 0;
		m_aspectRatioMaximum = 0;
		m_gravity = Alignment::Default;

		m_flagVisible = sl_true;
		m_flagMinimized = sl_false;
		m_flagMaximized = sl_false;
		m_flagFullScreen =
#if defined(SLIB_PLATFORM_IS_MOBILE)
			sl_true;
#else
			sl_false;
#endif

		m_flagAlwaysOnTop = sl_false;
		m_flagCloseButtonEnabled = sl_true;
		m_flagMinimizeButtonEnabled = sl_false;
		m_flagMaximizeButtonEnabled = sl_false;
		m_flagFullScreenButtonEnabled = sl_false;
		m_flagResizable = sl_false;
		m_flagLayered = sl_false;
		m_flagTransparent = sl_false;
		m_flagVisibleInTaskbar = sl_true;
		m_flagExcludingFromCapture = sl_false;

		m_flagModal = sl_false;
		m_flagSheet = sl_false;
		m_flagDialog = sl_false;
		m_flagBorderless = sl_false;
		m_flagShowTitleBar = sl_true;
		m_flagWidthWrapping = sl_false;
		m_flagHeightWrapping = sl_false;
		m_flagWidthFilling = sl_false;
		m_flagHeightFilling = sl_false;
		m_flagCloseOnOK = sl_false;
		m_flagCloseOnCancel = sl_true;

		m_flagStateResizingWidth = sl_false;
		m_flagStateDoModal = sl_false;
		m_flagStateClosing = sl_false;
		m_flagRequestClose = sl_false;
		m_flagDispatchedDestroy = sl_false;

		m_parentHandle = sl_null;
		m_viewContent = new WindowContentView;

		m_result = sl_null;

#if defined(SLIB_UI_IS_ANDROID)
		m_activity = sl_null;
#endif
	}

	Window::~Window()
	{
		if (m_result) {
			delete m_result;
		}
	}

	void Window::init()
	{
		Object::init();

		m_viewContent->setWindow(this);
	}

	void Window::close()
	{
		ObjectLocker lock(this);

		if (m_flagRequestClose) {
			m_flagStateClosing = sl_true;
			return;
		}

		Ref<WindowInstance> instance = m_instance;
		if (instance.isNull()) {
			return;
		}

		void (Window::*func)() = &Window::close;
		SLIB_VIEW_RUN_ON_UI_THREAD2(func)

		Ref<Window> window = this;
		detach();
		lock.unlock();

		_doDestroy();

		instance->close();
	}

	sl_bool Window::isClosed()
	{
		if (m_flagRequestClose && m_flagStateClosing) {
			return sl_true;
		}
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			return instance->isClosed();
		}
		return sl_true;
	}

	sl_bool Window::isOpened()
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			return !(instance->isClosed());
		}
		return sl_false;
	}

	Ref<Window> Window::getParent()
	{
		return m_parent;
	}

	void Window::setParent(const Ref<Window>& parent)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setParent, parent)
			m_parent = parent;
			if (parent.isNotNull()) {
				instance->setParent(parent->m_instance);
			} else {
				instance->setParent(Ref<WindowInstance>::null());
			}
		} else {
			m_parent = parent;
		}
	}

	Ref<WindowInstance> Window::getParentInstance()
	{
		if (m_parent.isNotNull()) {
			Ref<Window> parent = m_parent;
			if (parent.isNotNull()) {
				return parent->getWindowInstance();
			}
			return sl_null;
		} else {
			return m_parentInstance;
		}
	}

	void Window::setParentInstance(const Ref<WindowInstance>& parent)
	{
		if (m_parent.isNull()) {
			Ref<WindowInstance> instance = m_instance;
			if (instance.isNotNull()) {
				SLIB_VIEW_RUN_ON_UI_THREAD(setParentInstance, parent)
				m_parentInstance = parent;
				if (parent.isNotNull()) {
					instance->setParent(parent);
				} else {
					instance->setParent(Ref<WindowInstance>::null());
				}
				return;
			}
		}
		m_parentInstance = parent;
	}

	void* Window::getParentHandle(Ref<WindowInstance>& instance)
	{
		if (m_parent.isNotNull() || m_parentInstance.isNotNull()) {
			instance = getParentInstance();
			if (instance.isNotNull()) {
				return instance->getHandle();
			}
			return sl_null;
		} else {
			return m_parentHandle;
		}
	}

	void* Window::getParentHandle()
	{
		if (m_parent.isNotNull() || m_parentInstance.isNotNull()) {
			Ref<WindowInstance> instance = getParentInstance();
			if (instance.isNotNull()) {
				return instance->getHandle();
			}
			return sl_null;
		} else {
			return m_parentHandle;
		}
	}

	void Window::setParentHandle(void* parent)
	{
		if (m_parent.isNull()) {
			Ref<WindowInstance> instance = m_instance;
			if (instance.isNotNull()) {
				SLIB_VIEW_RUN_ON_UI_THREAD(setParentHandle, parent)
				m_parentHandle = parent;
				instance->setParentHandle(parent);
				return;
			}
		}
		m_parentHandle = parent;
	}

	Ref<Screen> Window::getScreen()
	{
		return m_screen;
	}

	void Window::setScreen(const Ref<Screen>& screen)
	{
		m_screen = screen;
	}

	const Ref<WindowContentView>& Window::getContentView()
	{
		return m_viewContent;
	}

	Ref<Menu> Window::getMenu()
	{
		return m_menu;
	}

	void Window::setMenu(const Ref<Menu>& menu)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setMenu, menu)
			m_menu = menu;
			instance->setMenu(menu);
		} else {
			m_menu = menu;
		}
	}

	sl_bool Window::isActive()
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			return instance->isActive();
		}
		return sl_false;
	}

	void Window::activate()
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(activate)
			instance->activate();
		}
	}

	UIRect Window::getFrame()
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			instance->getFrame(m_frame);
		}
		return m_frame;
	}

	void Window::setFrame(const UIRect& _frame)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			void (Window::*func)(const UIRect&) = &Window::setFrame;
			SLIB_VIEW_RUN_ON_UI_THREAD2(func, _frame)
		}
		UIRect frame = _frame;
		_constrainWindowSize(frame, frame.getWidth() > 0);
		m_frame = frame;
		if (instance.isNotNull()) {
			instance->setFrame(frame);
		}
	}

	void Window::setFrame(sl_ui_pos left, sl_ui_pos top, sl_ui_len width, sl_ui_len height)
	{
		UIRect rect;
		rect.left = left;
		rect.top = top;
		rect.setSize(width, height);
		setFrame(rect);
	}

	UIPoint Window::getLocation()
	{
		return getFrame().getLocation();
	}

	void Window::setLocation(const UIPoint& location)
	{
		UIRect frame = getFrame();
		frame.setLocation(location);
		setFrame(frame);
	}

	void Window::setLocation(sl_ui_pos x, sl_ui_pos y)
	{
		setLocation(UIPoint(x, y));
	}

	sl_ui_pos Window::getLeft()
	{
		return getFrame().left;
	}

	void Window::setLeft(sl_ui_pos x)
	{
		UIRect frame = getFrame();
		frame.left = x;
		setFrame(frame);
	}

	sl_ui_pos Window::getTop()
	{
		return getFrame().top;
	}

	void Window::setTop(sl_ui_pos y)
	{
		UIRect frame = getFrame();
		frame.top = y;
		setFrame(frame);
	}

	UISize Window::getSize()
	{
		return getFrame().getSize();
	}

	void Window::setSize(sl_ui_len width, sl_ui_len height)
	{
		UIRect frame = getFrame();
		frame.setSize(width, height);
		setFrame(frame);
	}

	void Window::setSize(const UISize& size)
	{
		setSize(size.x, size.y);
	}

	sl_ui_len Window::getWidth()
	{
		return getFrame().getWidth();
	}

	void Window::setWidth(sl_ui_len width)
	{
		UIRect frame = getFrame();
		frame.setWidth(width);
		setFrame(frame);
	}

	sl_ui_len Window::getHeight()
	{
		return getFrame().getHeight();
	}

	void Window::setHeight(sl_ui_len height)
	{
		UIRect frame = getFrame();
		frame.setHeight(height);
		setFrame(frame);
	}

	sl_bool Window::isWidthWrapping()
	{
		return m_flagWidthWrapping;
	}

	void Window::setWidthWrapping(sl_bool flag, UIUpdateMode mode)
	{
		m_flagWidthWrapping = flag;
		if (flag) {
			if (SLIB_UI_UPDATE_MODE_IS_UPDATE_LAYOUT(mode)) {
				_applyContentWrappingSize();
			}
		}
	}

	sl_bool Window::isHeightWrapping()
	{
		return m_flagHeightWrapping;
	}

	void Window::setHeightWrapping(sl_bool flag, UIUpdateMode mode)
	{
		m_flagHeightWrapping = flag;
		if (flag) {
			if (SLIB_UI_UPDATE_MODE_IS_UPDATE_LAYOUT(mode)) {
				_applyContentWrappingSize();
			}
		}
	}

	sl_bool Window::isWidthFilling()
	{
		return m_flagWidthFilling;
	}

	void Window::setWidthFilling(sl_bool flag, UIUpdateMode mode)
	{
		m_flagWidthFilling = flag;
		if (m_instance.isNotNull()) {
			updateFrame(mode);
		}
	}

	sl_bool Window::isHeightFilling()
	{
		return m_flagHeightFilling;
	}

	void Window::setHeightFilling(sl_bool flag, UIUpdateMode mode)
	{
		m_flagHeightFilling = flag;
		if (m_instance.isNotNull()) {
			updateFrame(mode);
		}
	}

	UIRect Window::getClientFrame()
	{
		return getClientFrameFromWindowFrame(getFrame());
	}

	void Window::setClientFrame(const UIRect& frame)
	{
		setFrame(getWindowFrameFromClientFrame(frame));
	}

	void Window::setClientFrame(sl_ui_pos left, sl_ui_pos top, sl_ui_len width, sl_ui_len height)
	{
		UIRect rect;
		rect.left = left;
		rect.top = top;
		rect.setSize(width, height);
		setClientFrame(rect);
	}

	UISize Window::getClientSize()
	{
		return getClientFrame().getSize();
	}

	void Window::setClientSize(sl_ui_len width, sl_ui_len height)
	{
		UIRect frame = getClientFrame();
		frame.setSize(width, height);
		setClientFrame(frame);
	}

	void Window::setClientSize(const UISize& size)
	{
		setClientSize(size.x, size.y);
	}

	sl_ui_len Window::getClientWidth()
	{
		return getClientFrame().getWidth();
	}

	void Window::setClientWidth(sl_ui_len width)
	{
		UIRect frame = getClientFrame();
		frame.setWidth(width);
		setClientFrame(frame);
	}

	sl_ui_len Window::getClientHeight()
	{
		return getClientFrame().getHeight();
	}

	void Window::setClientHeight(sl_ui_len height)
	{
		UIRect frame = getClientFrame();
		frame.setHeight(height);
		setClientFrame(frame);
	}

	String Window::getTitle()
	{
		return m_title;
	}

	void Window::setTitle(const String& title)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setTitle, title)
			m_title = title;
			instance->setTitle(title);
		} else {
			m_title = title;
		}
	}

	Ref<Drawable> Window::getIcon()
	{
		return m_icon;
	}

	void Window::setIcon(const Ref<Drawable>& icon)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setIcon, icon)
			m_icon = icon;
			instance->setIcon(icon);
		} else {
			m_icon = icon;
		}
	}

	String Window::getIconResource()
	{
		return m_iconResource;
	}

	void Window::setIconResource(const String& name)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setIconResource, name)
			m_iconResource = name;
			instance->setIcon(name);
		} else {
			m_iconResource = name;
		}
	}

	Color Window::getBackgroundColor()
	{
		return m_backgroundColor;
	}

	void Window::setBackgroundColor(const Color& color)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setBackgroundColor, color)
			m_flagDefaultBackgroundColor = sl_false;
			m_backgroundColor = color;
			instance->setBackgroundColor(color);
		} else {
			m_flagDefaultBackgroundColor = sl_false;
			m_backgroundColor = color;
		}
	}

	void Window::resetBackgroundColor()
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(resetBackgroundColor)
			m_flagDefaultBackgroundColor = sl_true;
			m_backgroundColor.setZero();
			instance->resetBackgroundColor();
		} else {
			m_flagDefaultBackgroundColor = sl_true;
			m_backgroundColor.setZero();
		}
	}

	sl_bool Window::isDefaultBackgroundColor()
	{
		return m_flagDefaultBackgroundColor;
	}

	sl_bool Window::isMinimized()
	{
		if (UI::isUiThread()) {
			Ref<WindowInstance> instance = m_instance;
			if (instance.isNotNull()) {
				sl_bool f = m_flagMinimized;
				instance->isMinimized(f);
				m_flagMinimized = f;
			}
		}
		return m_flagMinimized;
	}

	void Window::setMinimized(sl_bool flag)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setMinimized, flag)
			m_flagMinimized = flag;
			instance->setMinimized(flag);
		} else {
			m_flagMinimized = flag;
		}
	}

	sl_bool Window::isMaximized()
	{
		if (UI::isUiThread()) {
			Ref<WindowInstance> instance = m_instance;
			if (instance.isNotNull()) {
				sl_bool f = m_flagMaximized;
				instance->isMaximized(f);
				m_flagMaximized = f;
			}
		}
		return m_flagMaximized;
	}

	void Window::setMaximized(sl_bool flag)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setMaximized, flag)
			m_flagMaximized = flag;
			instance->setMaximized(flag);
		} else {
			m_flagMaximized = flag;
		}
	}

	sl_bool Window::isFullScreen()
	{
		if (UI::isUiThread()) {
			Ref<WindowInstance> instance = m_instance;
			if (instance.isNotNull()) {
				sl_bool f = m_flagFullScreen;
				instance->isFullScreen(f);
				m_flagFullScreen = f;
			}
		}
		return m_flagFullScreen;
	}

	void Window::setFullScreen(sl_bool flag)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setFullScreen, flag)
			m_flagFullScreen = flag;
			instance->setFullScreen(flag);
		} else {
			m_flagFullScreen = flag;
		}
	}

	sl_bool Window::isVisible()
	{
		return isOpened() && m_flagVisible;
	}

	void Window::setVisible(sl_bool flag)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setVisible, flag)
			m_flagVisible = flag;
			instance->setVisible(flag);
		} else {
			m_flagVisible = flag;
		}
	}

	sl_bool Window::isAlwaysOnTop()
	{
		return m_flagAlwaysOnTop;
	}

	void Window::setAlwaysOnTop(sl_bool flag)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setAlwaysOnTop, flag)
			m_flagAlwaysOnTop = flag;
			instance->setAlwaysOnTop(flag);
		} else {
			m_flagAlwaysOnTop = flag;
		}
	}

	sl_bool Window::isCloseButtonEnabled()
	{
		return m_flagCloseButtonEnabled;
	}

	void Window::setCloseButtonEnabled(sl_bool flag)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setCloseButtonEnabled, flag)
			m_flagCloseButtonEnabled = flag;
			instance->setCloseButtonEnabled(flag);
		} else {
			m_flagCloseButtonEnabled = flag;
		}
	}

	sl_bool Window::isMinimizeButtonEnabled()
	{
		return m_flagMinimizeButtonEnabled;
	}

	void Window::setMinimizeButtonEnabled(sl_bool flag)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setMinimizeButtonEnabled, flag)
			m_flagMinimizeButtonEnabled = flag;
			instance->setMinimizeButtonEnabled(flag);
		} else {
			m_flagMinimizeButtonEnabled = flag;
		}
	}

	sl_bool Window::isMaximizeButtonEnabled()
	{
		return m_flagMaximizeButtonEnabled;
	}

	void Window::setMaximizeButtonEnabled(sl_bool flag)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setMaximizeButtonEnabled, flag)
			m_flagMaximizeButtonEnabled = flag;
			instance->setMaximizeButtonEnabled(flag);
		} else {
			m_flagMaximizeButtonEnabled = flag;
		}
	}

	sl_bool Window::isFullScreenButtonEnabled()
	{
		return m_flagFullScreenButtonEnabled;
	}

	void Window::setFullScreenButtonEnabled(sl_bool flag)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setFullScreenButtonEnabled, flag)
			m_flagFullScreenButtonEnabled = flag;
			instance->setFullScreenButtonEnabled(flag);
		} else {
			m_flagFullScreenButtonEnabled = flag;
		}
	}

	sl_bool Window::isResizable()
	{
		return m_flagResizable;
	}

	void Window::setResizable(sl_bool flag)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setResizable, flag)
			m_flagResizable = flag;
			instance->setResizable(flag);
		} else {
			m_flagResizable = flag;
		}
	}

	sl_bool Window::isLayered()
	{
		return m_flagLayered;
	}

	void Window::setLayered(sl_bool flag)
	{
		m_flagLayered = flag;
	}

	sl_real Window::getAlpha()
	{
		return m_alpha;
	}

	void Window::setAlpha(sl_real alpha)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setAlpha, alpha)
			m_alpha = alpha;
			instance->setAlpha(alpha);
		} else {
			m_alpha = alpha;
		}
	}

	Color Window::getColorKey()
	{
		return m_colorKey;
	}

	void Window::setColorKey(const Color& color)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setColorKey, color)
			m_colorKey = color;
			instance->setColorKey(color);
		} else {
			m_colorKey = color;
		}
	}

	sl_bool Window::isTransparent()
	{
		return m_flagTransparent;
	}

	void Window::setTransparent(sl_bool flag)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setTransparent, flag)
			m_flagTransparent = flag;
			instance->setTransparent(flag);
		} else {
			m_flagTransparent = flag;
		}
	}

	sl_bool Window::isVisibleInTaskbar()
	{
		return m_flagVisibleInTaskbar;
	}

	void Window::setVisibleInTaskbar(sl_bool flag)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setVisibleInTaskbar, flag)
			m_flagVisibleInTaskbar = flag;
			instance->setVisibleInTaskbar(flag);
		} else {
			m_flagVisibleInTaskbar = flag;
		}
	}

	sl_bool Window::isExcludingFromCapture()
	{
		return m_flagExcludingFromCapture;
	}

	void Window::setExcludingFromCapture(sl_bool flag)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setExcludingFromCapture, flag)
			m_flagExcludingFromCapture = flag;
			instance->setExcludingFromCapture(flag);
		} else {
			m_flagExcludingFromCapture = flag;
		}
	}

	void Window::setSizeRange(const UISize& _sizeMinimum, const UISize& _sizeMaximum, float aspectRatioMinimum, float aspectRatioMaximum)
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setSizeRange, _sizeMinimum, _sizeMaximum, aspectRatioMinimum, aspectRatioMaximum)
		}

		UISize sizeMinimum = _sizeMinimum;
		if (sizeMinimum.x < 0) {
			sizeMinimum.x = 0;
		}
		if (sizeMinimum.y < 0) {
			sizeMinimum.y = 0;
		}
		m_sizeMin = sizeMinimum;
		UISize sizeMaximum = _sizeMaximum;
		if (sizeMaximum.x < 0) {
			sizeMaximum.x = 0;
		}
		if (sizeMaximum.y < 0) {
			sizeMaximum.y = 0;
		}
		m_sizeMax = sizeMaximum;
		if (aspectRatioMinimum < 0) {
			aspectRatioMinimum = 0;
		}
		m_aspectRatioMinimum = aspectRatioMinimum;
		if (aspectRatioMaximum < 0) {
			aspectRatioMaximum = 0;
		}
		m_aspectRatioMaximum = aspectRatioMaximum;

		if (instance.isNotNull()) {
			instance->setSizeRange(sizeMinimum, sizeMaximum, aspectRatioMinimum, m_aspectRatioMaximum);
		}

		UISize size = getClientSize();
		UISize sizeOld = size;
		_constrainClientSize(size, size.x > 0);
		if (!(size.isAlmostEqual(sizeOld))) {
			setClientSize(size);
		}
	}

	UIEdgeInsets Window::getClientInsets()
	{
		UIEdgeInsets ret;
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			if (instance->getClientInsets(ret)) {
				return ret;
			}
		}
		if (_getClientInsets(ret)) {
			return ret;
		}
		ret.left = 0;
		ret.top = 0;
		ret.right = 0;
		ret.bottom = 0;
		return ret;
	}

	UIRect Window::getWindowFrameFromClientFrame(const UIRect& frame)
	{
		UIEdgeInsets insets = getClientInsets();
		return UIRect(frame.left - insets.left, frame.top - insets.top, frame.right + insets.right, frame.bottom + insets.bottom);
	}

	UIRect Window::getClientFrameFromWindowFrame(const UIRect& frame)
	{
		UIEdgeInsets insets = getClientInsets();
		return UIRect(frame.left + insets.left, frame.top + insets.top, frame.right - insets.right, frame.bottom - insets.bottom);
	}

	UISize Window::getWindowSizeFromClientSize(const UISize& size)
	{
		UIEdgeInsets insets = getClientInsets();
		return UISize(size.x + insets.left + insets.right, size.y + insets.top + insets.bottom);
	}

	UISize Window::getClientSizeFromWindowSize(const UISize& size)
	{
		UIEdgeInsets insets = getClientInsets();
		return UISize(size.x - insets.left - insets.right, size.y - insets.top - insets.bottom);
	}

	UIPointF Window::convertCoordinateFromScreenToWindow(const UIPointF& pt)
	{
		UIPointF origin = getLocation();
		return UIPointF(pt.x - origin.x, pt.y - origin.y);
	}

	UIRectF Window::convertCoordinateFromScreenToWindow(const UIRectF& rect)
	{
		UIPointF origin = getLocation();
		return UIRectF(rect.left - origin.x, rect.top - origin.y, rect.right - origin.x, rect.bottom - origin.y);
	}

	UIPointF Window::convertCoordinateFromWindowToScreen(const UIPointF& pt)
	{
		UIPointF origin = getLocation();
		return UIPointF(pt.x + origin.x, pt.y + origin.y);
	}

	UIRectF Window::convertCoordinateFromWindowToScreen(const UIRectF& rect)
	{
		UIPointF origin = getLocation();
		return UIRectF(rect.left + origin.x, rect.top + origin.y, rect.right + origin.x, rect.bottom + origin.y);
	}

	UIPointF Window::convertCoordinateFromScreenToClient(const UIPointF& pt)
	{
		UIPointF origin = getClientFrame().getLocation();
		return UIPointF(pt.x - origin.x, pt.y - origin.y);
	}

	UIRectF Window::convertCoordinateFromScreenToClient(const UIRectF& rect)
	{
		UIPointF origin = getClientFrame().getLocation();
		return UIRectF(rect.left - origin.x, rect.top - origin.y, rect.right - origin.x, rect.bottom - origin.y);
	}

	UIPointF Window::convertCoordinateFromClientToScreen(const UIPointF& pt)
	{
		UIPointF origin = getClientFrame().getLocation();
		return UIPointF(pt.x + origin.x, pt.y + origin.y);
	}

	UIRectF Window::convertCoordinateFromClientToScreen(const UIRectF& rect)
	{
		UIPointF origin = getClientFrame().getLocation();
		return UIRectF(rect.left + origin.x, rect.top + origin.y, rect.right + origin.x, rect.bottom + origin.y);
	}

	UIPointF Window::convertCoordinateFromWindowToClient(const UIPointF& pt)
	{
		UIEdgeInsets insets = getClientInsets();
		UIPointF origin((sl_real)(insets.left), (sl_real)(insets.top));
		return UIPointF(pt.x - origin.x, pt.y - origin.y);
	}

	UIRectF Window::convertCoordinateFromWindowToClient(const UIRectF& rect)
	{
		UIEdgeInsets insets = getClientInsets();
		UIPointF origin((sl_real)(insets.left), (sl_real)(insets.top));
		return UIRectF(rect.left - origin.x, rect.top - origin.y, rect.right - origin.x, rect.bottom - origin.y);
	}

	UIPointF Window::convertCoordinateFromClientToWindow(const UIPointF& pt)
	{
		UIEdgeInsets insets = getClientInsets();
		UIPointF origin((sl_real)(insets.left), (sl_real)(insets.top));
		return UIPointF(pt.x + origin.x, pt.y + origin.y);
	}

	UIRectF Window::convertCoordinateFromClientToWindow(const UIRectF& rect)
	{
		UIEdgeInsets insets = getClientInsets();
		UIPointF origin((sl_real)(insets.left), (sl_real)(insets.top));
		return UIRectF(rect.left + origin.x, rect.top + origin.y, rect.right + origin.x, rect.bottom + origin.y);
	}

	UISize Window::getMinimumSize()
	{
		return m_sizeMin;
	}

	void Window::setMinimumSize(const UISize& sizeMinimum)
	{
		setSizeRange(sizeMinimum, m_sizeMax, m_aspectRatioMinimum, m_aspectRatioMaximum);
	}

	void Window::setMinimumSize(sl_ui_len width, sl_ui_len height)
	{
		setSizeRange(UISize(width, height), m_sizeMax, m_aspectRatioMinimum, m_aspectRatioMaximum);
	}

	sl_ui_len Window::getMinimumWidth()
	{
		return m_sizeMin.x;
	}

	void Window::setMinimumWidth(sl_ui_len width)
	{
		setSizeRange(UISize(width, m_sizeMin.y), m_sizeMax, m_aspectRatioMinimum, m_aspectRatioMaximum);
	}

	sl_ui_len Window::getMinimumHeight()
	{
		return m_sizeMin.y;
	}

	void Window::setMinimumHeight(sl_ui_len height)
	{
		setSizeRange(UISize(m_sizeMin.x, height), m_sizeMax, m_aspectRatioMinimum, m_aspectRatioMaximum);
	}

	UISize Window::getMaximumSize()
	{
		return m_sizeMax;
	}

	void Window::setMaximumSize(const UISize& sizeMaximum)
	{
		setSizeRange(m_sizeMin, sizeMaximum, m_aspectRatioMinimum, m_aspectRatioMaximum);
	}

	void Window::setMaximumSize(sl_ui_len width, sl_ui_len height)
	{
		setSizeRange(m_sizeMin, UISize(width, height), m_aspectRatioMinimum, m_aspectRatioMaximum);
	}

	sl_ui_len Window::getMaximumWidth()
	{
		return m_sizeMax.x;
	}

	void Window::setMaximumWidth(sl_ui_len width)
	{
		setSizeRange(m_sizeMin, UISize(width, m_sizeMax.y), m_aspectRatioMinimum, m_aspectRatioMaximum);
	}

	sl_ui_len Window::getMaximumHeight()
	{
		return m_sizeMax.y;
	}

	void Window::setMaximumHeight(sl_ui_len height)
	{
		setSizeRange(m_sizeMin, UISize(m_sizeMax.x, height), m_aspectRatioMinimum, m_aspectRatioMaximum);
	}

	float Window::getMinimumAspectRatio()
	{
		return m_aspectRatioMinimum;
	}

	void Window::setMinimumAspectRatio(float ratio)
	{
		setSizeRange(m_sizeMin, m_sizeMax, ratio, m_aspectRatioMaximum);
	}

	float Window::getMaximumAspectRatio()
	{
		return m_aspectRatioMaximum;
	}

	void Window::setMaximumAspectRatio(float ratio)
	{
		setSizeRange(m_sizeMin, m_sizeMax, m_aspectRatioMinimum, ratio);
	}

	void Window::setAspectRatio(float ratio)
	{
		setSizeRange(m_sizeMin, m_sizeMax, ratio, ratio);
	}

	sl_bool Window::isModal()
	{
		return m_flagModal;
	}

	void Window::setModal(sl_bool flag)
	{
		m_flagModal = flag;
	}

	sl_bool Window::isSheet()
	{
		return m_flagSheet;
	}

	void Window::setSheet(sl_bool flag)
	{
		m_flagSheet = flag;
	}

	sl_bool Window::isDialog()
	{
		return m_flagDialog;
	}

	void Window::setDialog(sl_bool flag)
	{
		m_flagDialog = flag;
	}

	sl_bool Window::isBorderless()
	{
		return m_flagBorderless;
	}

	void Window::setBorderless(sl_bool flag)
	{
		m_flagBorderless = flag;
	}

	sl_bool Window::isTitleBarVisible()
	{
		return m_flagShowTitleBar;
	}

	void Window::setTitleBarVisible(sl_bool flag)
	{
		m_flagShowTitleBar = flag;
	}

	const Alignment& Window::getGravity()
	{
		return m_gravity;
	}

	void Window::setGravity(const Alignment& align, UIUpdateMode mode)
	{
		m_gravity = align;
		if (m_instance.isNotNull()) {
			updateFrame(mode);
		}
	}

	sl_bool Window::isCenterScreen()
	{
		return m_gravity == Alignment::MiddleCenter;
	}

	void Window::setCenterScreen(sl_bool flag, UIUpdateMode mode)
	{
		if (flag) {
			setGravity(Alignment::MiddleCenter, mode);
		} else {
			if (m_gravity == Alignment::MiddleCenter) {
				setGravity(Alignment::Default, mode);
			}
		}
	}

	sl_ui_pos Window::getMarginLeft()
	{
		return m_margin.left;
	}

	void Window::setMarginLeft(sl_ui_pos margin, UIUpdateMode mode)
	{
		m_margin.left = margin;
		if (m_instance.isNotNull()) {
			updateFrame(mode);
		}
	}

	sl_ui_pos Window::getMarginTop()
	{
		return m_margin.top;
	}

	void Window::setMarginTop(sl_ui_pos margin, UIUpdateMode mode)
	{
		m_margin.top = margin;
		if (m_instance.isNotNull()) {
			updateFrame(mode);
		}
	}

	sl_ui_pos Window::getMarginRight()
	{
		return m_margin.right;
	}

	void Window::setMarginRight(sl_ui_pos margin, UIUpdateMode mode)
	{
		m_margin.right = margin;
		if (m_instance.isNotNull()) {
			updateFrame(mode);
		}
	}

	sl_ui_pos Window::getMarginBottom()
	{
		return m_margin.bottom;
	}

	void Window::setMarginBottom(sl_ui_pos margin, UIUpdateMode mode)
	{
		m_margin.bottom = margin;
		if (m_instance.isNotNull()) {
			updateFrame(mode);
		}
	}

	void Window::setMargin(sl_ui_pos left, sl_ui_pos top, sl_ui_pos right, sl_ui_pos bottom, UIUpdateMode mode)
	{
		m_margin.left = left;
		m_margin.top = top;
		m_margin.right = right;
		m_margin.bottom = bottom;
		if (m_instance.isNotNull()) {
			updateFrame(mode);
		}
	}

	void Window::setMargin(sl_ui_pos margin, UIUpdateMode mode)
	{
		m_margin.left = margin;
		m_margin.top = margin;
		m_margin.right = margin;
		m_margin.bottom = margin;
		if (m_instance.isNotNull()) {
			updateFrame(mode);
		}
	}

	const UIEdgeInsets& Window::getMargin()
	{
		return m_margin;
	}

	void Window::setMargin(const UIEdgeInsets& margin, UIUpdateMode mode)
	{
		m_margin = margin;
		if (m_instance.isNotNull()) {
			updateFrame(mode);
		}
	}

	void Window::updateFrame(UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_UPDATE_LAYOUT(mode)) {
			if (m_flagFullScreen || m_flagWidthFilling || m_flagHeightFilling || m_gravity != Alignment::Default) {
				UIRect frame = _makeFrame();
				if (!(m_frame.isAlmostEqual(frame))) {
					setFrame(frame);
				}
			}
		}
	}

	Function<WindowPart(sl_ui_pos x, sl_ui_pos y)> Window::getHitTester()
	{
		return m_hitTester;
	}

	void Window::setHitTester(const Function<WindowPart(sl_ui_pos x, sl_ui_pos y)>& tester)
	{
		m_hitTester = tester;
	}

	sl_bool Window::isCloseOnOK()
	{
		return m_flagCloseOnOK;
	}

	void Window::setCloseOnOK(sl_bool flag)
	{
		m_flagCloseOnOK = flag;
	}

	sl_bool Window::isCloseOnCancel()
	{
		return m_flagCloseOnCancel;
	}

	void Window::setCloseOnCancel(sl_bool flag)
	{
		m_flagCloseOnCancel = flag;
	}

	Variant Window::getResult()
	{
		SpinLocker lock(&m_lockResult);
		if (m_result) {
			return *m_result;
		} else {
			return sl_null;
		}
	}

	void Window::setResult(const Variant& result)
	{
		SpinLocker lock(&m_lockResult);
		if (m_result) {
			*m_result = result;
		} else {
			m_result = new Variant(result);
		}
	}

	void Window::close(const Variant& result)
	{
		setResult(result);
		close();
	}

	Time Window::getCreationTime()
	{
		return m_timeCreation;
	}

	void Window::setQuitOnDestroy()
	{
		setOnDestroy([](Window*) {
			UI::quitApp();
		});
	}

#if defined(SLIB_UI_IS_ANDROID)
	void* Window::getActivity()
	{
		return m_activity;
	}

	void Window::setActivity(void* activity)
	{
		m_activity = activity;
	}
#endif

	Ref<WindowInstance> Window::getWindowInstance()
	{
		return m_instance;
	}

	void* Window::getWindowHandle()
	{
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			return instance->getHandle();
		}
		return sl_null;
	}

	void Window::create()
	{
		_create(sl_false);
	}

	void Window::createAndKeep()
	{
		_create(sl_true);
	}

	void Window::forceCreate()
	{
		SLIB_VIEW_RUN_ON_UI_THREAD(forceCreate)
		detach();
		create();
	}

	void Window::forceCreateAndKeep()
	{
		SLIB_VIEW_RUN_ON_UI_THREAD(forceCreateAndKeep)
		detach();
		createAndKeep();
	}

	void Window::attach(const Ref<WindowInstance>& instance, sl_bool flagAttachContent)
	{
		detach();
		if (instance.isNotNull()) {
			instance->setWindow(this);
			m_instance = instance;
			if (flagAttachContent) {
				_attachContent();
			}
		}
	}

	void Window::detach()
	{
		// save current window's properties
		getFrame();
		Ref<View> view = m_viewContent;
		if (view.isNotNull()) {
			view->_removeParent();
			view->_detachAll();
		}
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			instance->setWindow(Ref<Window>::null());
		}
		m_instance.setNull();
	}

	void Window::_create(sl_bool flagKeepReference)
	{
		if (!(UI::isRunningApp() && UI::isUiThread())) {
			if (flagKeepReference) {
				UI::dispatchToUiThread(SLIB_BIND_REF(void(), this, _create, flagKeepReference));
			} else {
				UI::dispatchToUiThread(SLIB_BIND_WEAKREF(void(), this, _create, flagKeepReference));
			}
			return;
		}

		if (m_instance.isNotNull()) {
			return;
		}

		if (m_flagWidthWrapping || m_flagHeightWrapping) {
			UISize sizeOld = getClientSize();
			UISize sizeMeasured = m_viewContent->measureLayoutWrappingSize(m_flagWidthWrapping, m_flagHeightWrapping);
			if (m_flagWidthWrapping) {
				if (sizeMeasured.x < 1) {
					sizeMeasured.x = 1;
				}
			} else {
				sizeMeasured.x = sizeOld.x;
			}
			if (m_flagHeightWrapping) {
				if (sizeMeasured.y < 1) {
					sizeMeasured.y = 1;
				}
			} else {
				sizeMeasured.y = sizeOld.y;
			}
			setClientSize(sizeMeasured);
		}

		Ref<WindowInstance> window = createWindowInstance();

		if (window.isNotNull()) {

			m_timeCreation = Time::now();

			if (flagKeepReference) {
				increaseReference();
				window->setKeepWindow(sl_true);
			}

			if (m_flagMinimized) {
				window->setMinimized(sl_true);
			}
			if (m_flagMaximized) {
#ifdef SLIB_UI_IS_MACOS
				UI::dispatchToUiThread(SLIB_BIND_WEAKREF(void(), this, setMaximized, sl_true));
#else
				window->setMaximized(sl_true);
#endif
			}

			attach(window, sl_false);
			m_flagDispatchedDestroy = sl_false;

			invokeCreate();
			window->doPostCreate();

			if (m_flagVisible) {
				window->setVisible(sl_true);
				window->activate();
			}

		} else {
			invokeCreateFailed();
		}

	}

	void Window::_attachContent()
	{
		SLIB_VIEW_RUN_ON_UI_THREAD(_attachContent)
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			Ref<ViewInstance> contentViewInstance = instance->getContentView();
			if (contentViewInstance.isNotNull()) {
				Ref<View> view = m_viewContent;
				if (view.isNotNull()) {
					view->_removeParent();
					view->_removeAllViewInstances();
					if (!(view->isEnabled())) {
						contentViewInstance->setEnabled(view.get(), sl_false);
					}
					contentViewInstance->setOpaque(view.get(), view->isOpaque());
					contentViewInstance->setDrawing(view.get(), view->isDrawing());
					if (view->isDropTarget()) {
						contentViewInstance->setDropTarget(view.get(), sl_true);
					}
					if (view->isUsingTouchEvent()) {
						contentViewInstance->setUsingTouchEvent(view.get(), sl_true);
					}
					view->_attach(contentViewInstance);
					instance->onAttachedContentView(view.get());
				}
			}
		}
	}

	Variant Window::doModal()
	{
		if (!(UI::isUiThread())) {
			return sl_null;
		}
		setDialog(sl_true);
		setModal(sl_true);
		if (m_result) {
			m_result->setUndefined();
		}
		forceCreate();
		Ref<WindowInstance> instance = m_instance;
		if (instance.isNotNull()) {
			if (instance->doModal()) {
				return getResult();
			}
			m_flagStateDoModal = sl_true;
			UI::runLoop();
			m_flagStateDoModal = sl_false;
			return getResult();
		}
		return sl_null;
	}

	void Window::showModal()
	{
		UI::dispatchToUiThread(SLIB_FUNCTION_REF(this, doModal));
	}

	void Window::show()
	{
		setVisible(sl_true);
		if (m_instance.isNull()) {
			create();
		}
	}

	void Window::showAndKeep()
	{
		setVisible(sl_true);
		if (m_instance.isNull()) {
			createAndKeep();
		}
	}

	void Window::hide()
	{
		setVisible(sl_false);
	}

	void Window::addView(const Ref<View>& child, UIUpdateMode mode)
	{
		if (child.isNotNull()) {
			Ref<View> view = m_viewContent;
			if (view.isNotNull()) {
				view->addChild(child, mode);
			}
		}
	}

	void Window::removeView(const Ref<View>& child, UIUpdateMode mode)
	{
		if (child.isNotNull()) {
			Ref<View> view = m_viewContent;
			if (view.isNotNull()) {
				view->removeChild(child, mode);
			}
		}
	}

	List< Ref<View> > Window::getViews()
	{
		Ref<View> view = m_viewContent;
		if (view.isNotNull()) {
			return view->getChildren();
		}
		return sl_null;
	}

	void Window::removeAllViews(UIUpdateMode mode)
	{
		Ref<View> view = m_viewContent;
		if (view.isNotNull()) {
			view->removeAllChildren(mode);
		}
	}

#if !defined(SLIB_UI_IS_WIN32) && !defined(SLIB_UI_IS_MACOS) && !defined(SLIB_UI_IS_IOS) && !defined(SLIB_UI_IS_GTK)
	Ref<Window> Window::getActiveWindow()
	{
		return sl_null;
	}
#endif

	SLIB_DEFINE_EVENT_HANDLER(Window, Create, ())

	SLIB_DEFINE_EVENT_HANDLER(Window, CreateFailed, ())

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(Window, Close, ())

	void Window::onClose()
	{
		close(DialogResult::Cancel);
	}

	// returns true for destroying
	sl_bool Window::_doClose()
	{
		ObjectLocker lock(this);
		m_flagRequestClose = sl_true;
		invokeClose();
		m_flagRequestClose = sl_false;
		if (m_flagStateClosing) {
			lock.unlock();
			detach();
			_doDestroy();
			return sl_true;
		} else {
			return sl_false;
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(Window, Destroy, ())

	void Window::_doDestroy()
	{
		if (!m_flagDispatchedDestroy) {
			m_flagDispatchedDestroy = sl_true;
			invokeDestroy();
		}
		if (m_flagStateDoModal) {
			m_flagStateDoModal = sl_false;
			UI::quitLoop();
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(Window, Activate, ())

	SLIB_DEFINE_EVENT_HANDLER(Window, Deactivate, ())

	SLIB_DEFINE_EVENT_HANDLER(Window, Move, (sl_ui_pos x, sl_ui_pos y), x, y)

	SLIB_DEFINE_EVENT_HANDLER(Window, Resizing, (UISize& clientSize), clientSize)

	void Window::_doResizing(UISize& clientSize)
	{
		sl_bool flagWrappingWidth = isWidthWrapping();
		sl_bool flagWrappingHeight = isHeightWrapping();
		if (flagWrappingWidth || flagWrappingHeight) {
			UISize sizeOld = getClientSize();
			if (flagWrappingWidth) {
				clientSize.x = sizeOld.x;
			}
			if (flagWrappingHeight) {
				clientSize.y = sizeOld.y;
			}
		}
		_constrainClientSize(clientSize, m_flagStateResizingWidth);
		invokeResizing(clientSize);
	}

	SLIB_DEFINE_EVENT_HANDLER(Window, Resize, (sl_ui_len clientWidth, sl_ui_len clientHeight), clientWidth, clientHeight)

	void Window::_doResize(sl_ui_len clientWidth, sl_ui_len clientHeight)
	{
		_refreshClientSize(UISize(clientWidth, clientHeight));
		if (clientWidth > 0 && clientHeight > 0) {
			Ref<View> viewContent = m_viewContent;
			if (viewContent.isNotNull()) {
				if (!(viewContent->isInstance())) {
					_attachContent();
				}
			}
		}
		invokeResize(clientWidth, clientHeight);
	}

	SLIB_DEFINE_EVENT_HANDLER(Window, Minimize, ())

	SLIB_DEFINE_EVENT_HANDLER(Window, Deminimize, ())

	SLIB_DEFINE_EVENT_HANDLER(Window, Maximize, ())

	SLIB_DEFINE_EVENT_HANDLER(Window, Demaximize, ())

	SLIB_DEFINE_EVENT_HANDLER(Window, EnterFullScreen, ())

	SLIB_DEFINE_EVENT_HANDLER(Window, ExitFullScreen, ())

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(Window, OK, ())
		
	void Window::onOK()
	{
		if (m_flagCloseOnOK) {
			close(DialogResult::OK);
		}
	}

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(Window, Cancel, ())

	void Window::onCancel()
	{
		if (m_flagCloseOnCancel) {
			invokeClose();
		}
	}

	void Window::_refreshClientSize(const UISize& size)
	{
		Ref<View> view = m_viewContent;
		if (view.isNotNull()) {
			UIRect rect;
			rect.setLeftTop(UIPoint::zero());
			rect.setSize(size);
			view->setFrame(rect);
		}
	}

	void Window::_constrainClientSize(UISize& size, sl_bool flagAdjustHeight)
	{
		sl_ui_len minX = m_sizeMin.x;
		sl_ui_len minY = m_sizeMin.y;
		sl_ui_len maxX = m_sizeMax.x;
		if (maxX <= 0) {
			maxX = 1000000;
		}
		sl_ui_len maxY = m_sizeMax.y;
		if (maxY <= 0) {
			maxY = 1000000;
		}

		size.x = Math::clamp(size.x, minX, maxX);
		size.y = Math::clamp(size.y, minY, maxY);

		float minAspect = m_aspectRatioMinimum;
		float maxAspect = m_aspectRatioMaximum;
		if (minAspect > 0 || maxAspect > 0) {
			if (minAspect > 0) {
				if (flagAdjustHeight) {
					sl_ui_len ay = (sl_ui_len)(Math::min(size.x / minAspect, 1000000.0f));
					if (size.y > ay) {
						if (ay > minY) {
							size.y = ay;
						} else {
							size.y = minY;
							size.x = (sl_ui_len)(Math::min(minY * minAspect, 1000000.0f));
						}
					}
				} else {
					sl_ui_len ax = (sl_ui_len)(Math::min(size.y * minAspect, 1000000.0f));
					if (size.x < ax) {
						if (ax < maxX) {
							size.x = ax;
						} else {
							size.x = maxX;
							size.y = (sl_ui_len)(Math::min(maxX / minAspect, 1000000.0f));
						}
					}
				}
			}
			if (maxAspect > 0) {
				if (flagAdjustHeight) {
					sl_ui_len ay = (sl_ui_len)(Math::min(size.x / maxAspect, 1000000.0f));
					if (size.y < ay) {
						if (ay < maxY) {
							size.y = ay;
						} else {
							size.y = maxY;
							size.x = (sl_ui_len)(Math::min(maxY * maxAspect, 1000000.0f));
						}
					}
				} else {
					sl_ui_len ax = (sl_ui_len)(size.y * maxAspect);
					if (size.x > ax) {
						if (ax > minX) {
							size.x = ax;
						} else {
							size.x = minX;
							size.y = (sl_ui_len)(Math::min(minX / maxAspect, 1000000.0f));
						}
					}
				}
			}
		}
	}

	void Window::_constrainClientSize(UIRect& frame, sl_bool flagAdjustHeight)
	{
		UISize size = frame.getSize();
		_constrainClientSize(size, flagAdjustHeight);
		frame.setSize(size);
	}

	void Window::_constrainWindowSize(UISize& size, sl_bool flagAdjustHeight)
	{
		UISize clientSize = getClientSizeFromWindowSize(size);
		_constrainClientSize(clientSize, flagAdjustHeight);
		size = getWindowSizeFromClientSize(clientSize);
	}

	void Window::_constrainWindowSize(UIRect& frame, sl_bool flagAdjustHeight)
	{
		UISize size = frame.getSize();
		_constrainWindowSize(size, flagAdjustHeight);
		frame.setSize(size);
	}

	void Window::_applyContentWrappingSize()
	{
		m_viewContent->applyWrappingContentSize();
	}

	UIRect Window::_makeFrame()
	{
		UIRect frame = getFrame();
		_adjustFrame(frame);
		return frame;
	}

	void Window::_adjustFrame(UIRect& frame)
	{
		if (m_flagFullScreen) {
			frame = UI::getScreenRegion(m_screen);
		} else {
			Alignment gravity = m_gravity;
			Alignment horz = gravity & Alignment::HorizontalMask;
			Alignment vert = gravity & Alignment::VerticalMask;
			UIRect rectScreen = UI::getScreenWorkingRegion(m_screen);
			UIEdgeInsets& margin = m_margin;
			if (m_flagWidthFilling) {
				horz = 0;
				frame.left = rectScreen.left + margin.left;
				frame.right = rectScreen.right - margin.right;
			}
			if (m_flagHeightFilling) {
				vert = 0;
				frame.top = rectScreen.top + margin.top;
				frame.bottom = rectScreen.bottom - margin.bottom;
			}
			if (gravity != Alignment::Default) {
				if (horz == Alignment::Left) {
					frame.setLocationLeft(rectScreen.left + margin.left);
				} else if (horz == Alignment::Right) {
					frame.setLocationRight(rectScreen.right - margin.right);
				} else if (horz == Alignment::Center) {
					frame.setLocationLeft((rectScreen.right - margin.right + rectScreen.left + margin.left - frame.getWidth()) / 2);
				}
				if (vert == Alignment::Top) {
					frame.setLocationTop(rectScreen.top + margin.top);
				} else if (vert == Alignment::Bottom) {
					frame.setLocationBottom(rectScreen.bottom - margin.bottom);
				} else if (vert == Alignment::Middle) {
					frame.setLocationTop((rectScreen.bottom - margin.bottom + rectScreen.top + margin.top - frame.getHeight()) / 2);
				}
			}
			frame.fixSizeError();
		}
	}

#if !(defined(SLIB_UI_IS_WIN32) || defined(SLIB_UI_IS_MACOS) || defined(SLIB_UI_IS_GTK))
	sl_bool Window::_getClientInsets(UIEdgeInsets& _out)
	{
		return sl_false;
	}
#endif


	SLIB_DEFINE_OBJECT(WindowInstance, Object)

	WindowInstance::WindowInstance(): m_flagKeepWindow(sl_false)
	{
	}

	WindowInstance::~WindowInstance()
	{
		if (m_flagKeepWindow) {
			Ref<Window> window = m_window;
			if (window.isNotNull()) {
				window->decreaseReference();
			}
		}
	}

	Ref<Window> WindowInstance::getWindow()
	{
		return m_window;
	}

	void WindowInstance::setWindow(const Ref<Window>& window)
	{
		m_window = window;
	}

	void WindowInstance::setKeepWindow(sl_bool flag)
	{
		m_flagKeepWindow = flag;
	}

	void WindowInstance::setParent(const Ref<WindowInstance>& parent)
	{
		if (parent.isNotNull()) {
			setParentHandle(parent->getHandle());
		} else {
			setParentHandle(sl_null);
		}
	}

	void WindowInstance::setTitle(const String& title)
	{
	}

	void WindowInstance::setIcon(const Ref<Drawable>& icon)
	{
	}

	void WindowInstance::setIcon(const String& resourceName)
	{
	}

	void WindowInstance::setMenu(const Ref<Menu>& menu)
	{
	}

	sl_bool WindowInstance::isActive()
	{
		return sl_true;
	}

	void WindowInstance::activate()
	{
	}

	void WindowInstance::setBackgroundColor(const Color& color)
	{
	}

	void WindowInstance::resetBackgroundColor()
	{
		setBackgroundColor(Color::zero());
	}

	void WindowInstance::isMinimized(sl_bool& _out)
	{
	}

	void WindowInstance::setMinimized(sl_bool flag)
	{
	}

	void WindowInstance::isMaximized(sl_bool& _out)
	{
	}

	void WindowInstance::setMaximized(sl_bool flag)
	{
	}

	void WindowInstance::isFullScreen(sl_bool& _out)
	{
	}

	void WindowInstance::setFullScreen(sl_bool flag)
	{
	}

	void WindowInstance::setVisible(sl_bool flag)
	{
	}

	void WindowInstance::setAlwaysOnTop(sl_bool flag)
	{
	}

	void WindowInstance::setCloseButtonEnabled(sl_bool flag)
	{
	}

	void WindowInstance::setMinimizeButtonEnabled(sl_bool flag)
	{
	}

	void WindowInstance::setMaximizeButtonEnabled(sl_bool flag)
	{
	}

	void WindowInstance::setFullScreenButtonEnabled(sl_bool flag)
	{
	}

	void WindowInstance::setResizable(sl_bool flag)
	{
	}

	void WindowInstance::setAlpha(sl_real alpha)
	{
	}

	void WindowInstance::setColorKey(const Color& color)
	{
	}

	void WindowInstance::setTransparent(sl_bool flag)
	{
	}

	void WindowInstance::setVisibleInTaskbar(sl_bool flag)
	{
	}

	void WindowInstance::setExcludingFromCapture(sl_bool flag)
	{
	}

	sl_bool WindowInstance::getClientInsets(UIEdgeInsets& _out)
	{
		return sl_false;
	}

	void WindowInstance::setSizeRange(const UISize& sizeMinimum, const UISize& sizeMaximum, float aspectRatioMinimum, float aspectRatioMaximum)
	{
	}

	sl_bool WindowInstance::doModal()
	{
		return sl_false;
	}

	void WindowInstance::doPostCreate()
	{
	}

	sl_bool WindowInstance::onClose()
	{
		Ref<Window> window = getWindow();
		if (window.isNotNull()) {
			return window->_doClose();
		}
		return sl_true;
	}

	void WindowInstance::onActivate()
	{
		Ref<Window> window = getWindow();
		if (window.isNotNull()) {
			window->invokeActivate();
		}
	}

	void WindowInstance::onDeactivate()
	{
		Ref<Window> window = getWindow();
		if (window.isNotNull()) {
			window->invokeDeactivate();
		}
	}

	void WindowInstance::onMove(sl_ui_pos x, sl_ui_pos y)
	{
		Ref<Window> window = getWindow();
		if (window.isNotNull()) {
			window->invokeMove(x, y);
		}
	}

	void WindowInstance::onResizing(UISize& size, sl_bool flagResizingWidth)
	{
		Ref<Window> window = getWindow();
		if (window.isNotNull()) {
			window->m_flagStateResizingWidth = flagResizingWidth;
			window->_doResizing(size);
		}
	}

	void WindowInstance::onResize(sl_ui_len clientWidth, sl_ui_len clientHeight)
	{
		Ref<Window> window = getWindow();
		if (window.isNotNull()) {
			window->_doResize(clientWidth, clientHeight);
		}
	}

	void WindowInstance::onMinimize()
	{
		Ref<Window> window = getWindow();
		if (window.isNotNull()) {
			window->m_flagMinimized = sl_true;
			window->invokeMinimize();
		}
	}

	void WindowInstance::onDeminimize()
	{
		Ref<Window> window = getWindow();
		if (window.isNotNull()) {
			window->m_flagMinimized = sl_false;
			window->invokeDeminimize();
		}
	}

	void WindowInstance::onMaximize()
	{
		Ref<Window> window = getWindow();
		if (window.isNotNull()) {
			window->m_flagMaximized = sl_true;
			window->_refreshClientSize(window->getClientSize());
			window->invokeMaximize();
		}
	}

	void WindowInstance::onDemaximize()
	{
		Ref<Window> window = getWindow();
		if (window.isNotNull()) {
			window->m_flagMaximized = sl_false;
			window->_refreshClientSize(window->getClientSize());
			window->invokeDemaximize();
		}
	}

	void WindowInstance::onEnterFullScreen()
	{
		Ref<Window> window = getWindow();
		if (window.isNotNull()) {
			window->m_flagFullScreen = sl_true;
			window->_refreshClientSize(window->getClientSize());
			window->invokeEnterFullScreen();
		}
	}

	void WindowInstance::onExitFullScreen()
	{
		Ref<Window> window = getWindow();
		if (window.isNotNull()) {
			window->m_flagFullScreen = sl_false;
			window->_refreshClientSize(window->getClientSize());
			window->invokeExitFullScreen();
		}
	}

	void WindowInstance::onAttachedContentView(View* content)
	{
	}


	SLIB_DEFINE_OBJECT(WindowContentView, ViewGroup)

	WindowContentView::WindowContentView()
	{
	}

	WindowContentView::~WindowContentView()
	{
	}

	void WindowContentView::applyWrappingContentSize()
	{
		Ref<Window> window = getWindow();
		if (window.isNull()) {
			return;
		}
		sl_bool flagHorz = window->isWidthWrapping();
		sl_bool flagVert = window->isHeightWrapping();
		if (!flagHorz && !flagVert) {
			return;
		}
		UISize sizeOld = window->getClientSize();
		UISize sizeNew = sizeOld;
		UISize sizeMeasured = measureLayoutWrappingSize(flagHorz, flagVert);
		if (flagHorz) {
			sizeNew.x = sizeMeasured.x;
		}
		if (flagVert) {
			sizeNew.y = sizeMeasured.y;
		}
		if (sizeNew.isAlmostEqual(sizeOld)) {
			return;
		}
		if (window->getWindowInstance().isNotNull()) {
			if (window->getGravity() != Alignment::Default && (Time::now() - window->getCreationTime()).getMillisecondCount() < 500) {
				UIRect frame(window->getFrame());
				frame.setSize(window->getWindowSizeFromClientSize(sizeNew));
				window->_adjustFrame(frame);
				window->setFrame(frame);
				return;
			}
		}
		window->setClientSize(sizeNew);
	}

	void WindowContentView::onResizeChild(View* child, sl_ui_len width, sl_ui_len height)
	{
		applyWrappingContentSize();
	}

}
