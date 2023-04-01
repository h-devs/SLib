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

#include "slib/ui/select_view.h"

#include "combo_box_gtk.h"

namespace slib
{

	namespace {

		class SelectViewHelper : public SelectView
		{
		public:
			using SelectView::_onSelectItem_NW;
		};

		class SelectViewInstance : public GTK_ViewInstance, public ISelectViewInstance
		{
			SLIB_DECLARE_OBJECT

		public:
			void initialize(View* _view) override
			{
				SelectView* view = (SelectView*)_view;
				GtkComboBox* handle = (GtkComboBox*)m_handle;

				refreshItems(view, sl_true);

				g_signal_connect(handle, "changed", G_CALLBACK(onChanged), handle);
			}

			void refreshItems(SelectView* view, sl_bool flagInit)
			{
				GtkComboBox* handle = (GtkComboBox*)m_handle;
				if (handle) {
					RefreshItems(handle, view, flagInit);
				}
			}

			void refreshItems(SelectView* view) override
			{
				refreshItems(view, sl_false);
			}

			void insertItem(SelectView* view, sl_uint32 index, const String& title) override
			{
				GtkComboBox* handle = (GtkComboBox*)m_handle;
				if (handle) {
					InsertItem(handle, index, title);
				}
			}

			void removeItem(SelectView* view, sl_uint32 index) override
			{
				GtkComboBox* handle = (GtkComboBox*)m_handle;
				if (handle) {
					RemoveItem(handle, index);
				}
			}

			void setItemTitle(SelectView* view, sl_uint32 index, const String& title) override
			{
				GtkComboBox* handle = (GtkComboBox*)m_handle;
				if (handle) {
					SetItemTitle(handle, index, title);
				}
			}

			void selectItem(SelectView* view, sl_uint32 index) override
			{
				GtkComboBox* handle = (GtkComboBox*)m_handle;
				if (handle) {
					SelectItem(handle, index);
				}
			}

			static void onChanged(GtkComboBox*, gpointer userinfo)
			{
				GtkComboBox* handle = (GtkComboBox*)userinfo;
				Ref<SelectViewHelper> view = CastRef<SelectViewHelper>(UIPlatform::getView((GtkWidget*)handle));
				if (view.isNotNull()) {
					int index = gtk_combo_box_get_active(handle);
					view->_onSelectItem_NW(index);
				}
			}

		};

		SLIB_DEFINE_OBJECT(SelectViewInstance, GTK_ViewInstance)

	}

	Ref<ViewInstance> SelectView::createNativeWidget(ViewInstance* parent)
	{
		GtkWidget* handle = gtk_combo_box_new_text();
		return GTK_ViewInstance::create<SelectViewInstance>(this, parent, handle);
	}

	Ptr<ISelectViewInstance> SelectView::getSelectViewInstance()
	{
		return CastRef<SelectViewInstance>(getViewInstance());
	}

}

#endif
