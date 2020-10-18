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

#include "slib/ui/select_view.h"

#include "combo_box_gtk.h"

using namespace slib::priv::combo_box;

namespace slib
{

	namespace priv
	{
		namespace select_view
		{

			class SelectViewInstance : public GTK_ViewInstance, public ISelectViewInstance
			{
				SLIB_DECLARE_OBJECT
				
			public:
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

				void installControlEvents()
				{
					GtkComboBox* handle = (GtkComboBox*)m_handle;
					if (handle) {
						g_signal_connect(handle, "changed", G_CALLBACK(onChanged), handle);
					}
				}

				static void onChanged(GtkComboBox*, gpointer userinfo)
				{
					GtkComboBox* handle = (GtkComboBox*)userinfo;
					Ref<SelectViewInstance> instance = CastRef<SelectViewInstance>(UIPlatform::getViewInstance((GtkWidget*)handle));
					if (instance.isNotNull()) {
						Ref<SelectView> view = CastRef<SelectView>(instance->getView());
						if (view.isNotNull()) {
							int index = gtk_combo_box_get_active(handle);
							view->dispatchSelectItem(index);
						}
					}
				}

			};

			SLIB_DEFINE_OBJECT(SelectViewInstance, GTK_ViewInstance)
			
		}
	}

	using namespace priv::select_view;

	Ref<ViewInstance> SelectView::createNativeWidget(ViewInstance* parent)
	{
		GtkWidget* combobox = gtk_combo_box_new_text();
		Ref<SelectViewInstance> ret = GTK_ViewInstance::create<SelectViewInstance>(this, parent, combobox);
		if (ret.isNotNull()) {
			ret->refreshItems(this, sl_true);
			ret->installControlEvents();
			return ret;
		}
		return sl_null;
	}
	
	Ptr<ISelectViewInstance> SelectView::getSelectViewInstance()
	{
		return CastRef<SelectViewInstance>(getViewInstance());
	}

}

#endif
