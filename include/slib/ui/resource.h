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

#ifndef CHECKHEADER_SLIB_UI_RESOURCE
#define CHECKHEADER_SLIB_UI_RESOURCE

#include "menu.h"
#include "event.h"
#include "view.h"
#include "window.h"
#include "mobile_app.h"

#include "../core/resource.h"

#define SLIB_DECLARE_MENU_BEGIN(NAME) \
	class NAME : public slib::CRef { \
	public: \
		static const NAME* get(); \
		static slib::Ref<NAME> create(); \
		NAME(); \
		slib::Ref<slib::Menu> root; \
		slib::Ref<slib::Menu> root_menu; \
		void show(sl_ui_pos x, sl_ui_pos y) const; \
		void show(const slib::UIPoint& pt) const;

#define SLIB_DECLARE_SUBMENU(NAME) \
		slib::Ref<slib::Menu> NAME##_menu; \
		slib::Ref<slib::MenuItem> NAME;

#define SLIB_DECLARE_MENU_ITEM(NAME) \
		slib::Ref<slib::MenuItem> NAME;

#define SLIB_DECLARE_MENU_SEPARATOR(NAME) \
		slib::Ref<slib::MenuItem> NAME;

#define SLIB_DECLARE_MENU_END };


#define SLIB_DEFINE_MENU_BEGIN(NAME, ...) \
	const NAME* NAME::get() { \
		SLIB_SAFE_LOCAL_STATIC(NAME, ret); \
		if (SLIB_SAFE_STATIC_CHECK_FREED(ret)) { \
			return sl_null; \
		} \
		return &ret; \
	} \
	slib::Ref<NAME> NAME::create() { \
		return new NAME; \
	} \
	void NAME::show(sl_ui_pos x, sl_ui_pos y) const { \
		root->show(x, y); \
	} \
	void NAME::show(const slib::UIPoint& pt) const { \
		root->show(pt); \
	} \
	NAME::NAME() { \
		root = root_menu = slib::Menu::create(__VA_ARGS__); \
		if (root.isNull()) return;

#define SLIB_DEFINE_SUBMENU(PARENT, NAME, ...) \
	NAME##_menu = slib::Menu::createPopup(); \
	if (NAME##_menu.isNull()) return; \
	NAME = PARENT##_menu->addSubmenu(NAME##_menu, __VA_ARGS__);

#define SLIB_DEFINE_MENU_ITEM(PARENT, NAME, ...) \
	NAME = PARENT##_menu->addMenuItem(__VA_ARGS__);

#define SLIB_DEFINE_MENU_SEPARATOR(PARENT, NAME) \
	NAME = PARENT##_menu->addSeparator();

#define SLIB_DEFINE_MENU_SEPARATOR_NONAME(PARENT) \
	PARENT##_menu->addSeparator();

#define SLIB_DEFINE_MENU_END }


#define SLIB_DECLARE_UILAYOUT_BEGIN(NAME, BASE_CLASS) \
	class NAME : public BASE_CLASS \
	{ \
	public: \
		NAME(); \
	protected: \
		void init() override; \
		void initialize(); \
		void layoutViews(sl_ui_len width, sl_ui_len height) override; \
	public: \
		void setData(const slib::Variant& data, slib::UIUpdateMode mode = slib::UIUpdateMode::UpdateLayout);

#define SLIB_DECLARE_UILAYOUT_END \
	};

#define SLIB_DEFINE_UILAYOUT(NAME, BASE_CLASS) \
	NAME::NAME() {} \
	void NAME::init() \
	{ \
		BASE_CLASS::init(); \
		initialize(); \
		setInitialized(); \
		slib::UISize size = getContentSize(); \
		if (size.x > 0 && size.y > 0) { \
			layoutViews(size.x, size.y); \
		} \
	}


#define SLIB_DECLARE_WINDOW_LAYOUT_BEGIN(NAME) \
	SLIB_DECLARE_UILAYOUT_BEGIN(NAME, slib::WindowLayout)

#define SLIB_DECLARE_WINDOW_LAYOUT_END \
	SLIB_DECLARE_UILAYOUT_END

#define SLIB_DEFINE_WINDOW_LAYOUT(NAME) \
	SLIB_DEFINE_UILAYOUT(NAME, slib::WindowLayout)


#define SLIB_DECLARE_VIEW_LAYOUT_BEGIN(NAME) \
	SLIB_DECLARE_UILAYOUT_BEGIN(NAME, slib::ViewLayout)

#define SLIB_DECLARE_VIEW_LAYOUT_END \
	SLIB_DECLARE_UILAYOUT_END

#define SLIB_DEFINE_VIEW_LAYOUT(NAME) \
	SLIB_DEFINE_UILAYOUT(NAME, slib::ViewLayout)


#define SLIB_DECLARE_PAGE_LAYOUT_BEGIN(NAME) \
	SLIB_DECLARE_UILAYOUT_BEGIN(NAME, slib::PageLayout)

#define SLIB_DECLARE_PAGE_LAYOUT_END \
	SLIB_DECLARE_UILAYOUT_END

#define SLIB_DEFINE_PAGE_LAYOUT(NAME) \
	SLIB_DEFINE_UILAYOUT(NAME, slib::PageLayout)


#define SLIB_UILAYOUT_EVENT(NAME)  template <class CALLBACK> void setOn##NAME(CALLBACK&& callback)

#define SLIB_UILAYOUT_FORWARD_EVENT(CONTROL, EVENT) CONTROL->setOn##EVENT(slib::Forward<CALLBACK>(callback));

#define SLIB_UILAYOUT_ITERATE_VIEWS(PARENT, CHILD_LAYOUT, DATA, MODE) \
	{ \
		if (MODE != slib::UIUpdateMode::Init) { \
			PARENT->removeAllChildren(); \
		} \
		const slib::Variant& varData = DATA; \
		sl_size childCount = (sl_size)(varData.getElementCount()); \
		for (sl_size childIndex = 0; childIndex < childCount; childIndex++) { \
			auto child = slib::New<CHILD_LAYOUT>(); \
			child->setData(varData.getElement(childIndex), slib::UIUpdateMode::Init); \
			PARENT->addChild(Move(child), MODE); \
		} \
	}

#define SLIB_UILAYOUT_FORWARD_ITERATE_EVENT(ITERATE, EVENT) { for (auto&& child : ITERATE->getChildren()) { child->setOn##EVENT(slib::Forward<CALLBACK>(callback)); } }

namespace slib
{

	class UIResource
	{
	public:
		static void updateDefaultScreenSize();

		static sl_ui_len getScreenWidth();

		static void setScreenWidth(sl_ui_len width);

		static sl_ui_len getScreenHeight();

		static void setScreenHeight(sl_ui_len height);

		static sl_ui_len getScreenMinimum();

		static sl_ui_len getScreenMaximum();

		static double getScreenPPI();

		static void setScreenPPI(double ppi);

		static sl_ui_len getStatusBarHeight();

		static void setStatusBarHeight(sl_ui_len height);

		static sl_ui_len getSafeAreaInsetLeft();

		static void setSafeAreaInsetLeft(sl_ui_len left);

		static sl_ui_len getSafeAreaInsetTop();

		static void setSafeAreaInsetTop(sl_ui_len top);

		static sl_ui_len getSafeAreaInsetRight();

		static void setSafeAreaInsetRight(sl_ui_len right);

		static sl_ui_len getSafeAreaInsetBottom();

		static void setSafeAreaInsetBottom(sl_ui_len bottom);

		static sl_ui_len getSafeAreaWidth();

		static sl_ui_len getSafeAreaHeight();

		static sl_real pixelToInch(sl_real px);

		static sl_real inchToPixel(sl_real inch);

		static sl_real pixelToMeter(sl_real px);

		static sl_real meterToPixel(sl_real meters);

		static sl_real pixelToCentimeter(sl_real px);

		static sl_real centimeterToPixel(sl_real cm);

		static sl_real pixelToMillimeter(sl_real px);

		static sl_real millimeterToPixel(sl_real mm);

		static sl_real pixelToPoint(sl_real px);

		static sl_real pointToPixel(sl_real dp);

		static sl_real pixelToDp(sl_real px);

		static sl_real pixelToPicas(sl_real px);

		static sl_real picasToPixel(sl_real pc);

		static sl_real dpToPixel(sl_real dp);

		static sl_ui_pos toUiPos(sl_real f);

	};

	template <class LAYOUT>
	class UILayoutController : public Object
	{
	public:
		LAYOUT * ui;

	public:
		virtual void onInit() {}

	};

	class UILayoutResource
	{
	public:
		UILayoutResource();

		virtual ~UILayoutResource();

	public:
		Ref<View> getContent();

		sl_real getScaledPixel();

		void setScaledPixel(sl_real sp);

		CRef* getController()
		{
			return m_controller.get();
		}

		template <class LAYOUT>
		void setController(UILayoutController<LAYOUT>* controller)
		{
			m_controller = controller;
			controller->ui = (LAYOUT*)this;
			controller->onInit();
		}

	protected:
		virtual void layoutViews(sl_ui_len width, sl_ui_len height) = 0;

		virtual void onInit();

	protected:
		void _layoutViews_safe(sl_ui_len width, sl_ui_len height);

		sl_bool isInitialized();

		void setInitialized();

	protected:
		View* m_contentView;
		Ref<View> m_contentViewRef;
		Ref<CRef> m_controller;

		sl_real m_sp;

		volatile sl_int32 m_countRecursiveLayout;
		sl_bool m_flagInitialized;

	};

	class WindowLayout : public Window, public UILayoutResource
	{
		SLIB_DECLARE_OBJECT

	public:
		WindowLayout();

		~WindowLayout();

	public:
		UISize getContentSize();

	public:
		void onResize(sl_ui_len width, sl_ui_len height) override;

	protected:
		sl_ui_len m_contentWidth;
		sl_ui_len m_contentHeight;

	};

	class ViewLayout : public ViewGroup, public UILayoutResource
	{
		SLIB_DECLARE_OBJECT
	public:
		ViewLayout();

		~ViewLayout();

	protected:
		void init() override;

	public:
		UISize getContentSize();

	public:
		void onResize(sl_ui_len width, sl_ui_len height) override;

	};

	class PageLayout : public ViewPage, public UILayoutResource
	{
		SLIB_DECLARE_OBJECT

	public:
		PageLayout();

		~PageLayout();

	protected:
		void init() override;

	public:
		UISize getContentSize();

	public:
		void onResize(sl_ui_len width, sl_ui_len height) override;

	};

	class Button;
	class LabelView;
	class LineView;
	class CheckBox;
	class RadioButton;
	class RadioGroup;
	class EditView;
	class PasswordView;
	class TextArea;
	class ImageView;
	class SelectView;
	class SelectSwitch;
	class ComboBox;
	class ScrollView;
	class LinearLayout;
	template <class PARENT, class CHILD_LAYOUT> class IterateLayout;
	class ListView;
	class ListControl;
	class RenderView;
	class TabView;
	class TreeView;
	class WebView;
	class ChromiumView;
	class SplitLayout;
	class ProgressBar;
	class Slider;
	class SwitchView;
	class PickerView;
	class DatePicker;
	class ViewPager;
	class ViewPageNavigationController;
	class VideoView;
	class CameraView;
	class Drawer;
	class RefreshView;
	class CollectionView;
	class TableLayout;
	class ListBox;
	class LabelList;
	class TileLayout;
	class GridView;
	class GridViewColumn;
	class GridViewRow;
	class PdfView;
	class AudioView;

}

#endif
