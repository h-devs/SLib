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

#include "slib/ui/tab_view.h"

#include "view_gtk.h"

namespace slib
{

	namespace {

		class TabViewHelper : public TabView
		{
		public:
			void applyTabCount(GtkNotebook* handle)
			{
				ObjectLocker lock(this);
				sl_uint32 nNew = (sl_uint32)(m_items.getCount());
				sl_uint32 nOrig = (sl_uint32)(gtk_notebook_get_n_pages(handle));
				if (nOrig == nNew) {
					return;
				}
				if (nOrig > nNew) {
					for (sl_uint32 i = nOrig; i > nNew; i--) {
						gtk_notebook_remove_page(handle, i - 1);
					}
				} else {
					for (sl_uint32 i = nOrig; i < nNew; i++) {
						GtkWidget* child = gtk_event_box_new();
						gtk_widget_show(child);
						gtk_notebook_append_page(handle, child, sl_null);
					}
				}
			}

			void copyTabs(GtkNotebook* handle)
			{
				ListLocker<Item> items(m_items);
				applyTabCount(handle);
				for (sl_uint32 i = 0; i < items.count; i++) {
					GtkWidget* child = gtk_notebook_get_nth_page(handle, i);
					if (child){
						StringCstr label = items[i].label;
						gtk_notebook_set_tab_label_text (handle, child, label.getData());
						setTabContentView(handle, i, items[i].contentView);
					}
				}
				if (gtk_notebook_get_n_pages(handle) > 0) {
					gtk_notebook_set_current_page(handle, m_indexSelected);
				}
			}

			void setTabContentView(GtkNotebook* handle, sl_uint32 index, const Ref<View>& view)
			{
				GtkWidget *page = gtk_notebook_get_nth_page(handle, index);
				if (!page) {
					return;
				}
				GtkWidget* content = sl_null;
				if (view.isNotNull()) {
					Ref<ViewInstance> instance = view->getViewInstance();
					if (instance.isNull()) {
						instance = view->attachToNewInstance(sl_null);
					}
					content = UIPlatform::getViewHandle(instance.get());
					int width = 0, height = 0;
					gtk_widget_get_size_request(page, &width, &height);
					if (width != -1 && height != -1) {
						view->setFrame((sl_ui_pos)0, (sl_ui_pos)0, (sl_ui_pos)(width), (sl_ui_pos)(height));
					}
					view->setParent(this);
				}
				if (content) {
					gtk_widget_show(content);
					gtk_container_add((GtkContainer*)page, content);
				}
			}

			void updateContentViewSize(GtkNotebook* handle)
			{
				int width = 0, height = 0;
				gtk_widget_get_size_request((GtkWidget*)handle, &width, &height);
				UIRect frame;
				frame.left = 0;
				frame.top = 0;
				frame.right = (sl_ui_pos)(width);
				frame.bottom = (sl_ui_pos)(height);
				ListLocker<Item> items(m_items);
				for (sl_size i = 0; i < items.count; i++) {
					Ref<View> view = items[i].contentView;
					if (view.isNotNull()) {
						view->setFrame(frame);
					}
				}
			}

			using TabView::_onSelectTab_NW;
		};

		class TabViewInstance : public GTK_ViewInstance, public ITabViewInstance
		{
			SLIB_DECLARE_OBJECT

		public:
			GtkNotebook* getHandle()
			{
				return (GtkNotebook*)m_handle;
			}

			Ref<TabViewHelper> getHelper()
			{
				return CastRef<TabViewHelper>(getView());
			}

			void initialize(View* _view) override
			{
				TabViewHelper* view = (TabViewHelper*)_view;
				GtkNotebook* handle = getHandle();
				view->copyTabs(handle);
				g_signal_connect((GtkNotebook*)handle, "switch-page", G_CALLBACK(onSelectTab), handle);
			}

			void refreshTabCount(TabView* view) override
			{
				GtkNotebook* handle = getHandle();
				if (handle) {
					static_cast<TabViewHelper*>(view)->applyTabCount(handle);
				}
			}

			void refreshSize(TabView* view) override
			{
			}

			void setTabLabel(TabView* view, sl_uint32 index, const String& _text) override
			{
				GtkNotebook* handle = getHandle();
				if (handle) {
					GtkWidget* page = gtk_notebook_get_nth_page(handle, index);
					if (page) {
						StringCstr text(_text);
						gtk_notebook_set_tab_label_text(handle, page, text.getData());
					}
				}
			}

			void setTabContentView(TabView* view, sl_uint32 index, const Ref<View>& content) override
			{
				GtkNotebook* handle = getHandle();
				if (handle) {
					(static_cast<TabViewHelper*>(view))->setTabContentView(handle, index, content);
				}
			}

			void selectTab(TabView* view, sl_uint32 index) override
			{
				GtkNotebook* handle = getHandle();
				if (handle) {
					gtk_notebook_set_current_page(handle, index);
				}
			}

			sl_bool getContentViewSize(TabView* view, UISize& _out) override
			{
				GtkNotebook* handle = getHandle();
				if (handle) {
					int width = 0, height = 0;
					gtk_widget_get_size_request((GtkWidget*)handle, &width, &height);
					_out.x = (sl_ui_pos)(width);
					_out.y = (sl_ui_pos)(height);
					return sl_true;
				}
				return sl_false;
			}

			static void onSelectTab(GtkNotebook *notebook, GtkWidget   *page, guint page_num, gpointer user_data)
			{
				Ref<TabViewInstance> instance = CastRef<TabViewInstance>(UIPlatform::getViewInstance((GtkWidget*)notebook));
				if (instance.isNotNull()) {
					Ref<TabViewHelper> helper = CastRef<TabViewHelper>(instance->getView());
					if (helper.isNotNull()) {
						helper->_onSelectTab_NW(instance.get(), (sl_uint32)(page_num));
					}
				}
			}

		};

		SLIB_DEFINE_OBJECT(TabViewInstance, GTK_ViewInstance)

	}

	Ref<ViewInstance> TabView::createNativeWidget(ViewInstance* parent)
	{
		GtkWidget* handle = gtk_notebook_new();
		return GTK_ViewInstance::create<TabViewInstance>(this, parent, handle);
	}

	Ptr<ITabViewInstance> TabView::getTabViewInstance()
	{
		return CastRef<TabViewInstance>(getViewInstance());
	}

}

#endif
