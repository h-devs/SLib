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

#include "view_gtk.h"

namespace slib
{
	namespace priv
	{
		namespace select_view
		{
			class SelectViewInstance;
		}
	}
}


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
				GtkComboBox* getHandle()
				{
					return (GtkComboBox*)m_handle;
				}
				
				void selectItem(SelectView* view, sl_uint32 index) override
				{
					GtkComboBox* handle = getHandle();
					if (handle) {
						gtk_combo_box_set_active(handle, index);
					}
				}
				
				void refreshItems(SelectView* view) override
				{
					GtkWidget* handle = m_handle;
					GtkComboBox* combo_box = (GtkComboBox*)handle;
					if (handle) {
						sl_uint32 n = view->getItemsCount();
						for (sl_uint32 i = 0; i < n; i++) {
							String s = view->getItemTitle(i);
							gtk_combo_box_append_text((GtkComboBox*)combo_box, (const char*)s.getData());
						}
						sl_int32 indexSelected = view->getSelectedIndex();
						if (indexSelected >= 0 && (sl_uint32)indexSelected < n) {

							if (gtk_combo_box_get_active(combo_box) != indexSelected) {
								gtk_combo_box_set_active(combo_box, indexSelected);
							}
						}
						if(indexSelected == -1)
						{
							gtk_combo_box_set_active(combo_box, 0);
						}
					}
				}
				
				void insertItem(SelectView* view, sl_uint32 index, const String& title) override
				{
					GtkWidget* handle = m_handle;
					GtkComboBox* combo_box = (GtkComboBox*)handle;
					if (handle) {
						gtk_combo_box_insert_text(combo_box, index, title.getData());
					}

				}
				
				void removeItem(SelectView* view, sl_uint32 index) override
				{
					GtkComboBox* handle = getHandle();
					if (handle) {
						gtk_combo_box_remove_text(handle, index);
					}
				}
				
				void setItemTitle(SelectView* view, sl_uint32 index, const String& title) override
				{
					GtkComboBox* handle = getHandle();
					if (handle) {
						GtkTreeModel *model = gtk_combo_box_get_model(handle);
						GtkTreePath *path = gtk_tree_path_new_from_indices(index);
						if(path){
							GtkTreeIter iter;
							gtk_tree_model_get_iter (model, &iter, path);
							gtk_tree_path_free (path);
							gtk_list_store_set ((GtkListStore*)model, &iter, 0, title.getData(), -1);
						}
					}
				}
				
				sl_bool measureSize(SelectView* view, UISize& _out) override
				{
					return false;
					//return UIPlatform::measureNativeWidgetFittingSize(this, _out);
				}
				
				static void onSelectedItem (GtkComboBox *widget, gpointer handle)
				{
					Ref<GTK_ViewInstance> instance = Ref<GTK_ViewInstance>::from(UIPlatform::getViewInstance((GtkWidget*)handle));
					Ref<SelectViewInstance> selectViewInstance = CastRef<SelectViewInstance>(instance);
					if (selectViewInstance.isNotNull()) {
						Ref<SelectView> view = CastRef<SelectView>(selectViewInstance->getView());
						int index = gtk_combo_box_get_active((GtkComboBox*)handle);
						view->dispatchSelectItem(index);
					}
				}
				void installEventHandlers()
				{
					GtkComboBox* handle = (GtkComboBox*)m_handle;
					g_signal_connect(handle, "changed", G_CALLBACK(onSelectedItem), handle);
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
			ret->refreshItems(this);
			ret->installEventHandlers();
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
