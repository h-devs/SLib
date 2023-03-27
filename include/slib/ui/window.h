/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_WINDOW
#define CHECKHEADER_SLIB_UI_WINDOW

#include "view.h"
#include "event.h"
#include "menu.h"

#include "../core/string.h"
#include "../core/function.h"
#include "../graphics/color.h"
#include "../math/rectangle.h"

namespace slib
{

	class Screen;
	class Window;
	class WindowInstance;
	class WindowContentView;

	class SLIB_EXPORT Window : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		Window();

		~Window();

	protected:
		void init() override;

	public:
		void close();

		sl_bool isClosed();

		sl_bool isOpened();


		Ref<Window> getParent();

		void setParent(const Ref<Window>& parent);

		Ref<Screen> getScreen();

		void setScreen(const Ref<Screen>& screen);

		const Ref<WindowContentView>& getContentView();

		Ref<Menu> getMenu();

		void setMenu(const Ref<Menu>& menu);


		sl_bool isActive();

		void activate();

		UIRect getFrame();

		void setFrame(const UIRect& frame);

		void setFrame(sl_ui_pos left, sl_ui_pos top, sl_ui_len width, sl_ui_len height);

		UIPoint getLocation();

		void setLocation(const UIPoint& location);

		void setLocation(sl_ui_pos x, sl_ui_pos y);

		sl_ui_pos getLeft();

		void setLeft(sl_ui_pos x);

		sl_ui_pos getTop();

		void setTop(sl_ui_pos y);

		UISize getSize();

		void setSize(sl_ui_len width, sl_ui_len height);

		void setSize(const UISize& size);

		sl_ui_len getWidth();

		void setWidth(sl_ui_len width);

		sl_ui_len getHeight();

		void setHeight(sl_ui_len height);

		sl_bool isWidthWrapping();

		void setWidthWrapping(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isHeightWrapping();

		void setHeightWrapping(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isWidthFilling();

		void setWidthFilling(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isHeightFilling();

		void setHeightFilling(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::UpdateLayout);


		UIRect getClientFrame();

		void setClientFrame(const UIRect& rect);

		void setClientFrame(sl_ui_pos left, sl_ui_pos top, sl_ui_len width, sl_ui_len height);

		UISize getClientSize();

		void setClientSize(sl_ui_len width, sl_ui_len height);

		void setClientSize(const UISize& size);

		sl_ui_len getClientWidth();

		void setClientWidth(sl_ui_len width);

		sl_ui_len getClientHeight();

		void setClientHeight(sl_ui_len height);


		String getTitle();

		void setTitle(const String& title);

		Ref<Drawable> getIcon();

		void setIcon(const Ref<Drawable>& icon);

		Color getBackgroundColor();

		void setBackgroundColor(const Color& color);

		// set to default
		void resetBackgroundColor();

		sl_bool isDefaultBackgroundColor();


		sl_bool isMinimized();

		void setMinimized(sl_bool flag = sl_true);

		sl_bool isMaximized();

		void setMaximized(sl_bool flag = sl_true);

		sl_bool isFullScreen();

		void setFullScreen(sl_bool flag = sl_true);

		sl_bool isVisible();

		void setVisible(sl_bool flag = sl_true);

		sl_bool isAlwaysOnTop();

		void setAlwaysOnTop(sl_bool flag = sl_true);


		sl_bool isCloseButtonEnabled();

		void setCloseButtonEnabled(sl_bool flag = sl_true);

		sl_bool isMinimizeButtonEnabled();

		void setMinimizeButtonEnabled(sl_bool flag = sl_true);

		sl_bool isMaximizeButtonEnabled();

		void setMaximizeButtonEnabled(sl_bool flag = sl_true);

		sl_bool isFullScreenButtonEnabled();

		void setFullScreenButtonEnabled(sl_bool flag = sl_true);

		sl_bool isResizable();

		void setResizable(sl_bool flag = sl_true);


		sl_bool isLayered();

		void setLayered(sl_bool flag = sl_true);

		sl_real getAlpha();

		void setAlpha(sl_real alpha);

		Color getColorKey();

		void setColorKey(const Color& color);

		sl_bool isTransparent();

		void setTransparent(sl_bool flag = sl_true);


		// aspect = width / height
		void setSizeRange(const UISize& sizeMinimum, const UISize& sizeMaximum, float aspectRatioMinimum = 0, float aspectRatioMaximum = 0);

		UIEdgeInsets getClientInsets();

		UIRect getWindowFrameFromClientFrame(const UIRect& frame);

		UIRect getClientFrameFromWindowFrame(const UIRect& frame);

		UISize getWindowSizeFromClientSize(const UISize& sizeClient);

		UISize getClientSizeFromWindowSize(const UISize& sizeWindow);

		UIPointF convertCoordinateFromScreenToWindow(const UIPointF& ptScreen);

		UIRectF convertCoordinateFromScreenToWindow(const UIRectF& rect);

		UIPointF convertCoordinateFromWindowToScreen(const UIPointF& ptWindow);

		UIRectF convertCoordinateFromWindowToScreen(const UIRectF& rect);

		UIPointF convertCoordinateFromScreenToClient(const UIPointF& ptScreen);

		UIRectF convertCoordinateFromScreenToClient(const UIRectF& rect);

		UIPointF convertCoordinateFromClientToScreen(const UIPointF& ptClient);

		UIRectF convertCoordinateFromClientToScreen(const UIRectF& rect);

		UIPointF convertCoordinateFromWindowToClient(const UIPointF& ptWindow);

		UIRectF convertCoordinateFromWindowToClient(const UIRectF& rect);

		UIPointF convertCoordinateFromClientToWindow(const UIPointF& ptClient);

		UIRectF convertCoordinateFromClientToWindow(const UIRectF& rect);


		// For client size
		UISize getMinimumSize();

		// For client size
		void setMinimumSize(const UISize& sizeMinimum);

		// For client size
		void setMinimumSize(sl_ui_len width, sl_ui_len height);

		// For client size
		sl_ui_len getMinimumWidth();

		// For client size
		void setMinimumWidth(sl_ui_len width);

		// For client size
		sl_ui_len getMinimumHeight();

		// For client size
		void setMinimumHeight(sl_ui_len height);

		// For client size
		UISize getMaximumSize();

		// For client size
		void setMaximumSize(const UISize& sizeMaximum);

		// For client size
		void setMaximumSize(sl_ui_len width, sl_ui_len height);

		// For client size
		sl_ui_len getMaximumWidth();

		// For client size
		void setMaximumWidth(sl_ui_len width);

		// For client size
		sl_ui_len getMaximumHeight();

		// For client size
		void setMaximumHeight(sl_ui_len height);

		// For client size
		float getMinimumAspectRatio();

		// For client size
		void setMinimumAspectRatio(float ratio);

		// For client size
		float getMaximumAspectRatio();

		// For client size
		void setMaximumAspectRatio(float ratio);

		// For client size
		void setAspectRatio(float ratio);


		const Alignment& getGravity();

		void setGravity(const Alignment& align, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_bool isCenterScreen();

		void setCenterScreen(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_pos getMarginLeft();

		void setMarginLeft(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_pos getMarginTop();

		void setMarginTop(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_pos getMarginRight();

		void setMarginRight(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		sl_ui_pos getMarginBottom();

		void setMarginBottom(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setMargin(sl_ui_pos left, sl_ui_pos top, sl_ui_pos right, sl_ui_pos bottom, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void setMargin(sl_ui_pos margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		const UIEdgeInsets& getMargin();

		void setMargin(const UIEdgeInsets& margin, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void updateFrame(UIUpdateMode mode = UIUpdateMode::UpdateLayout);


		sl_bool isModal();

		// Call before creating window
		void setModal(sl_bool flag);

		sl_bool isSheet();

		// Call before creating window
		void setSheet(sl_bool flag = sl_true);

		sl_bool isDialog();

		// Call before creating window
		void setDialog(sl_bool flag = sl_true);

		sl_bool isBorderless();

		// Call before creating window
		void setBorderless(sl_bool flag = sl_true);

		sl_bool isTitleBarVisible();

		// Call before creating window
		void setTitleBarVisible(sl_bool flag = sl_true);


		sl_bool isCloseOnOK();

		void setCloseOnOK(sl_bool flag = sl_true);

		Variant getResult();

		void setResult(const Variant& result);

		void close(const Variant& result);

		Time getCreationTime();

		void setQuitOnDestroy();


#if defined(SLIB_UI_IS_ANDROID)
		void* getActivity();

		void setActivity(void* activity);
#endif

#if defined(SLIB_UI_GTK)
		static void setDefaultIcon(const Ref<Drawable>& icon);
#endif

	public:
		Ref<WindowInstance> getWindowInstance();

		void create();

		void createAndKeep();

		void forceCreate();

		void forceCreateAndKeep();

		Variant doModal();

		void showModal();

		void show();

		void showAndKeep();

		void hide();


		void addView(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		void removeView(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::UpdateLayout);

		List< Ref<View> > getViews();

		void removeAllViews(UIUpdateMode mode = UIUpdateMode::UpdateLayout);

	public:
		static Ref<Window> getActiveWindow();

	public:
		SLIB_DECLARE_EVENT_HANDLER(Window, Create)
		SLIB_DECLARE_EVENT_HANDLER(Window, CreateFailed)
		SLIB_DECLARE_EVENT_HANDLER(Window, Close, UIEvent* ev)
		SLIB_DECLARE_EVENT_HANDLER(Window, Destroy, UIEvent* ev /* nullable */)
		SLIB_DECLARE_EVENT_HANDLER(Window, Activate)
		SLIB_DECLARE_EVENT_HANDLER(Window, Deactivate)
		SLIB_DECLARE_EVENT_HANDLER(Window, Move, sl_ui_pos x, sl_ui_pos y)
		SLIB_DECLARE_EVENT_HANDLER(Window, Resizing, UISize& clientSize)
		SLIB_DECLARE_EVENT_HANDLER(Window, Resize, sl_ui_len clientWidth, sl_ui_len clientHeight)
		SLIB_DECLARE_EVENT_HANDLER(Window, Minimize)
		SLIB_DECLARE_EVENT_HANDLER(Window, Deminimize)
		SLIB_DECLARE_EVENT_HANDLER(Window, Maximize)
		SLIB_DECLARE_EVENT_HANDLER(Window, Demaximize)
		SLIB_DECLARE_EVENT_HANDLER(Window, EnterFullScreen)
		SLIB_DECLARE_EVENT_HANDLER(Window, ExitFullScreen)
		SLIB_DECLARE_EVENT_HANDLER(Window, OK, UIEvent* ev)
		SLIB_DECLARE_EVENT_HANDLER(Window, Cancel, UIEvent* ev)

	public:
		void dispatchOK();

		void dispatchCancel();

	protected:
		Ref<WindowInstance> createWindowInstance();

		void attach(const Ref<WindowInstance>& instance, sl_bool flagAttachContent = sl_true);

		void detach();

	protected:
		void _create(sl_bool flagKeepReference);

		void _attachContent();

		void _refreshClientSize(const UISize& size);

		void _constrainClientSize(UISize& size, sl_bool flagAdjustHeight);

		void _constrainClientSize(UIRect& frame, sl_bool flagAdjustHeight);

		void _constrainWindowSize(UISize& size, sl_bool flagAdjustHeight);

		void _constrainWindowSize(UIRect& frame, sl_bool flagAdjustHeight);

		void _applyContentWrappingSize();

		UIRect _makeFrame();

		sl_bool _getClientInsets(UIEdgeInsets& _out);

	protected:
		AtomicRef<WindowInstance> m_instance;
		AtomicWeakRef<Window> m_parent;
		Ref<WindowContentView> m_viewContent;
		AtomicRef<Screen> m_screen;
		AtomicRef<Menu> m_menu;

		AtomicString m_title;
		AtomicRef<Drawable> m_icon;
		Color m_backgroundColor;
		sl_bool m_flagDefaultBackgroundColor;

		sl_real m_alpha;
		Color m_colorKey;

		UIRect m_frame;
		UISize m_sizeMin;
		UISize m_sizeMax;
		float m_aspectRatioMinimum;
		float m_aspectRatioMaximum;
		Alignment m_gravity;
		UIEdgeInsets m_margin;

		sl_bool m_flagVisible : 1;
		sl_bool m_flagMinimized : 1;
		sl_bool m_flagMaximized : 1;
		sl_bool m_flagFullScreen : 1;

		sl_bool m_flagAlwaysOnTop : 1;
		sl_bool m_flagCloseButtonEnabled : 1;
		sl_bool m_flagMinimizeButtonEnabled : 1;
		sl_bool m_flagMaximizeButtonEnabled : 1;
		sl_bool m_flagFullScreenButtonEnabled : 1;
		sl_bool m_flagResizable : 1;
		sl_bool m_flagLayered: 1;
		sl_bool m_flagTransparent : 1;

		sl_bool m_flagModal : 1;
		sl_bool m_flagSheet : 1;
		sl_bool m_flagDialog : 1;
		sl_bool m_flagBorderless: 1;
		sl_bool m_flagShowTitleBar : 1;
		sl_bool m_flagWidthWrapping : 1;
		sl_bool m_flagHeightWrapping : 1;
		sl_bool m_flagWidthFilling : 1;
		sl_bool m_flagHeightFilling : 1;
		sl_bool m_flagCloseOnOK : 1;

		sl_bool m_flagStateResizingWidth : 1;
		sl_bool m_flagStateDoModal : 1;
		sl_bool m_flagDispatchedDestroy : 1;

		Variant* m_result;
		SpinLock m_lockResult;

		Time m_timeCreation;

#if defined(SLIB_UI_IS_ANDROID)
		// jobject
		void* m_activity;
#endif

		friend class WindowInstance;

	};


	class SLIB_EXPORT WindowInstance : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		WindowInstance();

		~WindowInstance();

	public:
		Ref<Window> getWindow();

		void setWindow(const Ref<Window>& window);

		void setKeepWindow(sl_bool flag);

	public:
		virtual void close() = 0;

		virtual sl_bool isClosed() = 0;


		virtual void setParent(const Ref<WindowInstance>& parent) = 0;

		virtual Ref<ViewInstance> getContentView() = 0;


		virtual sl_bool getFrame(UIRect& _out) = 0;

		virtual void setFrame(const UIRect& frame) = 0;


		virtual void setTitle(const String& title);

		virtual void setIcon(const Ref<Drawable>& icon);

		virtual void setMenu(const Ref<Menu>& menu);

		virtual sl_bool isActive();

		virtual void activate();

		virtual void setBackgroundColor(const Color& color);

		virtual void resetBackgroundColor();

		virtual void isMinimized(sl_bool& _out);

		virtual void setMinimized(sl_bool flag);

		virtual void isMaximized(sl_bool& _out);

		virtual void setMaximized(sl_bool flag);

		virtual void isFullScreen(sl_bool& _out);

		virtual void setFullScreen(sl_bool flag);

		virtual void setVisible(sl_bool flag);

		virtual void setAlwaysOnTop(sl_bool flag);

		virtual void setCloseButtonEnabled(sl_bool flag);

		virtual void setMinimizeButtonEnabled(sl_bool flag);

		virtual void setMaximizeButtonEnabled(sl_bool flag);

		virtual void setFullScreenButtonEnabled(sl_bool flag);

		virtual void setResizable(sl_bool flag);

		virtual void setLayered(sl_bool flag);

		virtual void setAlpha(sl_real alpha);

		virtual void setColorKey(const Color& color);

		virtual void setTransparent(sl_bool flag);


		virtual sl_bool getClientInsets(UIEdgeInsets& _out);

		virtual void setSizeRange(const UISize& sizeMinimum, const UISize& sizeMaximum, float aspectRatioMinimum, float aspectRatioMaximum);


		virtual sl_bool doModal();


		virtual void doPostCreate();

	public:
		sl_bool onClose();

		void onActivate();

		void onDeactivate();

		void onMove(sl_ui_pos x, sl_ui_pos y);

		void onResizing(UISize& clientSize, sl_bool flagResizingWidth);

		void onResize(sl_ui_len clientWidth, sl_ui_len clientHeight);

		void onMinimize();

		void onDeminimize();

		void onMaximize();

		void onDemaximize();

		void onEnterFullScreen();

		void onExitFullScreen();

	public:
		virtual void onAttachedContentView();

	protected:
		AtomicWeakRef<Window> m_window;
		sl_bool m_flagKeepWindow;

	};

	class SLIB_EXPORT WindowContentView : public ViewGroup
	{
		SLIB_DECLARE_OBJECT

	public:
		WindowContentView();

		~WindowContentView();

	public:
		void applyWrappingContentSize();

	protected:
		void onResizeChild(View* child, sl_ui_len width, sl_ui_len height) override;

	};

}

#endif
