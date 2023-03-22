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

#ifndef CHECKHEADER_SLIB_UI_TAB_VIEW
#define CHECKHEADER_SLIB_UI_TAB_VIEW

#include "view.h"
#include "view_state_map.h"

#include "../core/string.h"

namespace slib
{

	class ITabViewInstance;

	class SLIB_EXPORT TabView : public ViewGroup
	{
		SLIB_DECLARE_OBJECT

	public:
		TabView();

		~TabView();

	public:
		sl_uint32 getTabCount();

		virtual void setTabCount(sl_uint32 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		String getTabLabel(sl_uint32 index);

		virtual void setTabLabel(sl_uint32 index, const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getTabIcon(sl_uint32 index);

		virtual void setTabIcon(sl_uint32 index, const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<View> getTabContentView(sl_uint32 index);

		virtual void setTabContentView(sl_uint32 index, const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_uint32 getSelectedTabIndex();

		virtual void selectTab(sl_uint32 index, UIUpdateMode mode = UIUpdateMode::Redraw);

		UISize getContentViewSize();

		LayoutOrientation getOrientation();

		void setOrientation(LayoutOrientation orientation, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_real getTabWidth();

		virtual void setTabWidth(sl_real width, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_real getTabHeight();

		virtual void setTabHeight(sl_real height, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getBarBackground();

		void setBarBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setBarBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getContentBackground();

		void setContentBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setContentBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getTabBackground(ViewState state = ViewState::Default);

		void setTabBackground(const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTabBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTabBackgroundColor(const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTabBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Color getLabelColor(ViewState state = ViewState::Default);

		void setLabelColor(const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setLabelColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Alignment getTabAlignment();

		virtual void setTabAlignment(const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);

		virtual void setTabPadding(sl_ui_pos left, sl_ui_pos top, sl_ui_pos right, sl_ui_pos bottom, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTabPadding(sl_ui_pos padding, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_pos getTabPaddingLeft();

		void setTabPaddingLeft(sl_ui_pos padding, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_pos getTabPaddingTop();

		void setTabPaddingTop(sl_ui_pos padding, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_pos getTabPaddingRight();

		void setTabPaddingRight(sl_ui_pos padding, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_pos getTabPaddingBottom();

		void setTabPaddingBottom(sl_ui_pos padding, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_pos getTabSpaceSize();

		virtual void setTabSpaceSize(sl_ui_pos size, UIUpdateMode mode = UIUpdateMode::Redraw);

		UISize getIconSize();

		virtual void setIconSize(const UISize& size, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setIconSize(sl_ui_len width, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setIconSize(sl_ui_len size, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getIconWidth();

		void setIconWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getIconHeight();

		void setIconHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		virtual UIRect getTabBarRegion();

		virtual UIRect getTabRegion(sl_uint32 index);

		virtual UIRect getWholeContentRegion();

		virtual UIRect getTabContentRegion();

	public:
		SLIB_DECLARE_EVENT_HANDLER_WITHOUT_DISPATCH(TabView, SelectTab, sl_uint32& index, UIEvent* ev /* nullable */)
		SLIB_DECLARE_EVENT_HANDLER_WITHOUT_DISPATCH(TabView, SelectedTab, UIEvent* ev /* nullable */)

	protected:
		Ref<ViewInstance> createNativeWidget(ViewInstance* parent) override;

		virtual Ptr<ITabViewInstance> getTabViewInstance();

	public:
		void notifySelectTab(ITabViewInstance* instance, sl_uint32 index);

	protected:
		void onClickEvent(UIEvent* ev) override;

		void onMouseEvent(UIEvent* ev) override;

		void onSetCursor(UIEvent* ev) override;

		void onDraw(Canvas* canvas) override;

		void onResize(sl_ui_len width, sl_ui_len height) override;

		virtual void onDrawTab(Canvas* canvas, const UIRect& rect, sl_uint32 index, const Ref<Drawable>& icon, const String& label);

	private:
		void _selectTab(ITabViewInstance* instance, sl_uint32 index, UIEvent* ev, UIUpdateMode mode);

		void _invalidateTabBar(UIUpdateMode mode);

		void _refreshSize();

		void _relayout(UIUpdateMode mode);

		sl_int32 _getTabIndexAt(const UIPoint& pt);

		ViewState _getTabState(sl_uint32 index);

	protected:
		class Item
		{
		public:
			String label;
			Ref<Drawable> icon;
			Ref<View> contentView;

		public:
			Item();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Item)
		};

		CList<Item> m_items;
		sl_uint32 m_indexSelected;
		sl_int32 m_indexHover;

		LayoutOrientation m_orientation;
		sl_real m_tabWidth;
		sl_real m_tabHeight;

		AtomicRef<Drawable> m_barBackground;
		AtomicRef<Drawable> m_contentBackground;
		ViewStateMap< Ref<Drawable> > m_tabBackgrounds;
		ViewStateMap<Color> m_labelColors;

		Alignment m_tabAlignment;
		sl_ui_pos m_tabPaddingLeft;
		sl_ui_pos m_tabPaddingTop;
		sl_ui_pos m_tabPaddingRight;
		sl_ui_pos m_tabPaddingBottom;
		sl_ui_pos m_tabSpaceSize;
		sl_ui_len m_iconWidth;
		sl_ui_len m_iconHeight;

	};

	class SLIB_EXPORT ITabViewInstance
	{
	public:
		virtual void refreshTabCount(TabView* view) = 0;

		virtual void refreshSize(TabView* view) = 0;

		virtual void setTabLabel(TabView* view, sl_uint32 index, const String& text) = 0;

		virtual void setTabContentView(TabView* view, sl_uint32 index, const Ref<View>& content) = 0;

		virtual sl_bool getContentViewSize(TabView* view, UISize& _out) = 0;

		virtual void selectTab(TabView* view, sl_uint32 index) = 0;

	};

}

#endif
