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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_GTK)

#include "slib/ui/scroll_view.h"

#include "view_gtk.h"

namespace slib
{

	namespace {

		class ScrollViewHelper : public ScrollView
		{
		public:
			using ScrollView::_onScroll_NW;

		};

		class ScrollViewInstance : public GTK_ViewInstance, public IScrollViewInstance
		{
			SLIB_DECLARE_OBJECT

		public:
			GtkScrolledWindow* getHandle()
			{
				return (GtkScrolledWindow*)m_handle;
			}

			void initialize(View* _view) override
			{
				ScrollView* view = (ScrollView*)_view;
				GtkScrolledWindow* handle = getHandle();

				setScrollBarsVisible(view, view->isHorizontalScrollBarVisible(), view->isVerticalScrollBarVisible());
				setBorder(view, view->hasBorder());
				setBackgroundColor(view, view->getBackgroundColor());
				setContentView(view, view->getContentView());
				scrollTo(view, view->getScrollX(), view->getScrollY(), sl_false);

				GtkAdjustment* adjust_h = gtk_scrolled_window_get_hadjustment(handle);
				g_signal_connect(adjust_h, "value-changed", G_CALLBACK(onScrollHorz), handle);

				GtkAdjustment* adjust_v = gtk_scrolled_window_get_vadjustment(handle);
				g_signal_connect(adjust_v, "value-changed", G_CALLBACK(onScrollVert), handle);
			}

			void refreshContentSize(ScrollView* view) override
			{
			}

			void setContentView(ScrollView* view, const Ref<View>& content) override
			{
				GtkScrolledWindow* handle = getHandle();
				GtkWidget* handleContent = NULL;
				if (handle) {
					if (content.isNotNull()) {
						content->attachToNewInstance(this);
					}
				}
			}

			sl_bool getClientSize(View* view, UISize& _out) override
			{
				GtkWidget* handle = m_handle;
				if (handle) {
					int width, height;
					gtk_widget_get_size_request(handle, &width, &height);
					_out.x = (sl_ui_len)(width);
					_out.y = (sl_ui_len)(height);
					return sl_true;
				}
				return sl_false;
			}

			sl_bool getScrollPosition(View* view, ScrollPosition& _out) override
			{
				GtkScrolledWindow* handle = getHandle();
				if (handle) {
					GtkAdjustment* adjust_h = gtk_scrolled_window_get_hadjustment(handle);
					GtkAdjustment* adjust_v = gtk_scrolled_window_get_vadjustment(handle);
					_out.x = (sl_scroll_pos)(gtk_adjustment_get_value(adjust_h));
					_out.y = (sl_scroll_pos)(gtk_adjustment_get_value(adjust_v));
					return sl_true;
				}
				return sl_false;
			}

			sl_bool getScrollRange(View* view, ScrollPosition& _out) override
			{
				GtkScrolledWindow* handle = getHandle();
				if (handle) {
					GtkAdjustment* adjust_h = gtk_scrolled_window_get_hadjustment(handle);
					GtkAdjustment* adjust_v = gtk_scrolled_window_get_vadjustment(handle);
					_out.x = (sl_scroll_pos)(gtk_adjustment_get_upper(adjust_h));
					_out.y = (sl_scroll_pos)(gtk_adjustment_get_upper(adjust_v));
					if (_out.x < 0) {
						_out.x = 0;
					}
					if (_out.y < 0) {
						_out.y = 0;
					}
					return sl_true;
				}
				return sl_false;
			}

			void scrollTo(View* view, sl_scroll_pos x, sl_scroll_pos y, sl_bool flagAnimate) override
			{
				GtkScrolledWindow* handle = getHandle();
				if (handle) {
					GtkAdjustment* adjust_h = gtk_scrolled_window_get_hadjustment(handle);
					GtkAdjustment* adjust_v = gtk_scrolled_window_get_vadjustment(handle);
					gtk_adjustment_set_value(adjust_h, x);
					gtk_adjustment_set_value(adjust_v, y);
				}
			}

			void setBorder(View* view, sl_bool flag) override
			{
				GtkScrolledWindow* handle = getHandle();
				if (handle) {
					if(flag){
						gtk_scrolled_window_set_shadow_type(handle, GTK_SHADOW_ETCHED_IN);
					}else{
						gtk_scrolled_window_set_shadow_type(handle, GTK_SHADOW_NONE);
					}
				}
			}

			void setScrollBarsVisible(View* view, sl_bool flagHorizontal, sl_bool flagVertical) override
			{
				GtkScrolledWindow* handle = getHandle();
				if (handle) {
					GtkPolicyType policy_h = flagHorizontal ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER;
					GtkPolicyType policy_v = flagVertical ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER;
					gtk_scrolled_window_set_policy(handle, policy_h, policy_v);
				}
			}

			static void onScrollHorz(GtkAdjustment*, gpointer user_data)
			{
				onScroll(user_data, sl_true);
			}

			static void onScrollVert(GtkAdjustment*, gpointer user_data)
			{
				onScroll(user_data, sl_false);
			}

			static void onScroll(gpointer user_data, sl_bool flagHorz)
			{
				GtkScrolledWindow* handle = (GtkScrolledWindow*)user_data;
				Ref<ScrollViewHelper> view = CastRef<ScrollViewHelper>(UIPlatform::getView((GtkWidget*)handle));
				if (view.isNotNull()) {
					GtkAdjustment* adjust_h = gtk_scrolled_window_get_hadjustment(handle);
					GtkAdjustment* adjust_v = gtk_scrolled_window_get_vadjustment(handle);
					sl_scroll_pos x = (sl_scroll_pos)(gtk_adjustment_get_value(adjust_h));
					sl_scroll_pos y = (sl_scroll_pos)(gtk_adjustment_get_value(adjust_v));
					view->_onScroll_NW(x, y);
				}
			}

		};

		SLIB_DEFINE_OBJECT(ScrollViewInstance, GTK_ViewInstance)

	}

	Ref<ViewInstance> ScrollView::createNativeWidget(ViewInstance* parent)
	{
		GtkWidget *handle = gtk_scrolled_window_new(sl_null, sl_null);
		return GTK_ViewInstance::create<ScrollViewInstance>(this, parent, handle);
	}

	Ptr<IScrollViewInstance> ScrollView::getScrollViewInstance()
	{
		return CastRef<ScrollViewInstance>(getViewInstance());
	}

}

#endif
