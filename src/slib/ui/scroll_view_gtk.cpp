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

#include "slib/core/definition.h"

#if defined(SLIB_UI_IS_GTK)

#include "slib/ui/scroll_view.h"

#include "view_gtk.h"

namespace slib
{
	namespace priv
	{
		namespace scroll_view
		{
			class ScrollViewInstance;
		}
	}
}

namespace slib
{
	
	namespace priv
	{
		namespace scroll_view
		{
			
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
					setBorder(view, view->isBorder());
					setBackgroundColor(view, view->getBackgroundColor());
					setContentView(view, view->getContentView());
					scrollTo(view, view->getScrollX(), view->getScrollY(), sl_false);
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
				
				sl_bool getBounds(ScrollView* view, UIRect& _out) override
				{
					GtkWidget* handle = m_handle;
					if (handle) {
						int width, height;
						gtk_widget_get_size_request(handle, &width, &height);
						_out.left = 0;
						_out.top = 0;
						_out.right = (sl_ui_len)(width);
						_out.bottom = (sl_ui_len)(height);
						return sl_true;
					}
					return sl_false;
				}

				sl_bool getScrollPosition(ScrollView* view, ScrollPoint& _out) override
				{
					GtkScrolledWindow* handle = getHandle();
					if (handle) {
						GtkAdjustment* hAdjustments =  gtk_scrolled_window_get_hadjustment(handle);
						GtkAdjustment* vAdjustments =  gtk_scrolled_window_get_vadjustment(handle);
						_out.x = (sl_scroll_pos)(gtk_adjustment_get_value(hAdjustments));
						_out.y = (sl_scroll_pos)(gtk_adjustment_get_value(vAdjustments));
						return sl_true;
					}
					return sl_false;
				}
				
				sl_bool getScrollRange(ScrollView* view, ScrollPoint& _out) override
				{
					GtkScrolledWindow* handle = getHandle();
					if (handle) {
						GtkAdjustment* hAdjustments =  gtk_scrolled_window_get_hadjustment(handle);
						GtkAdjustment* vAdjustments =  gtk_scrolled_window_get_vadjustment(handle);
						_out.x = (sl_scroll_pos)(gtk_adjustment_get_upper(hAdjustments));
						_out.y = (sl_scroll_pos)(gtk_adjustment_get_upper(vAdjustments));
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
						GtkAdjustment* hAdjustments =  gtk_scrolled_window_get_hadjustment(handle);
						GtkAdjustment* vAdjustments =  gtk_scrolled_window_get_vadjustment(handle);
						gtk_adjustment_set_value(hAdjustments, x);
						gtk_adjustment_set_value(vAdjustments, y);
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
				
				void setBackgroundColor(View* view, const Color& color) override
				{
					GtkScrolledWindow* handle = getHandle();
					if (handle) {
						//_setBackgroundColor(handle, color);
					}
				}
				
				void setScrollBarsVisible(View* view, sl_bool flagHorizontal, sl_bool flagVertical) override
				{
					GtkScrolledWindow* handle = getHandle();
					if (handle) {
						GtkPolicyType hscrollbar_policy = flagHorizontal ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER;
						GtkPolicyType vscrollbar_policy = flagVertical ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER;
						gtk_scrolled_window_set_policy(handle, hscrollbar_policy, vscrollbar_policy);
					}
				}
				
				void onScroll(GtkScrolledWindow* sv)
				{
					/*
					NSClipView* clip = [sv contentView];
					if (clip != nil) {
						NSPoint pt = [clip bounds].origin;
						Ref<ScrollViewHelper> helper = CastRef<ScrollViewHelper>(getView());
						if (helper.isNotNull()) {
							helper->_onScroll_NW((sl_ui_pos)(pt.x), (sl_ui_pos)(pt.y));
						}
					}*/
				}
				
			};
			
			SLIB_DEFINE_OBJECT(ScrollViewInstance, GTK_ViewInstance)
			
		}
	}
	
	using namespace priv::scroll_view;

	Ref<ViewInstance> ScrollView::createNativeWidget(ViewInstance* parent)
	{
		GtkWidget *handle = gtk_scrolled_window_new (NULL, NULL);
		return GTK_ViewInstance::create<ScrollViewInstance>(this, parent, handle);
	}
	
	Ptr<IScrollViewInstance> ScrollView::getScrollViewInstance()
	{
		return CastRef<ScrollViewInstance>(getViewInstance());
	}

}

#endif
