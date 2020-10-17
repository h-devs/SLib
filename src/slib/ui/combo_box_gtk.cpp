/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/combo_box.h"
#include "view_gtk.h"

namespace slib
{

	namespace priv
	{
		namespace combo_box
		{

			class ComboBoxInstance : public GTK_ViewInstance, public IComboBoxInstance
			{
				SLIB_DECLARE_OBJECT

			public:
				void refreshItems(ComboBox* view) override
				{
					refreshItems(view, sl_false);
				}
				static String getTextFromHanlde(GtkComboBox* handle)
				{
					GtkEntry* entry = (GtkEntry*)gtk_bin_get_child ((GtkBin*)handle);
					gchar *text = gtk_entry_get_text(entry);
					String tmp = text;
					return tmp;
				}
				void refreshItems(ComboBox* view, sl_bool flagInit)
				{
					GtkWidget* handle = m_handle;
					GtkComboBox* combo_box = (GtkComboBox*)handle;
					String text = view->getText();
					if (handle) {
						if (flagInit) {
							if (text.isNotEmpty()) {
								setText(view, text);
							}
						} else {
							gtk_combo_box_set_title (combo_box, text.getData());
						}
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

				void insertItem(ComboBox* view, sl_int32 index, const String& title) override
				{
					GtkWidget* handle = m_handle;
					GtkComboBox* combo_box = (GtkComboBox*)handle;
					if (handle) {
						gtk_combo_box_insert_text(combo_box, index, title.getData());
					}
				}

				void removeItem(ComboBox* view, sl_int32 index)
				{
					GtkWidget* handle = m_handle;
					GtkComboBox* combo_box = (GtkComboBox*)handle;
					if (handle) {
						gtk_combo_box_remove_text(combo_box, index);
					}
				}

				void setItemTitle(ComboBox* view, sl_int32 index, const String& title) override
				{
					GtkComboBox* handle = (GtkComboBox*)m_handle;
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

				void selectItem(ComboBox* view, sl_int32 index) override
				{
					GtkComboBox* handle = (GtkComboBox*)m_handle;
					if (handle) {
						gtk_combo_box_set_active(handle, index);
					}
				}

				sl_bool getText(ComboBox* view, String& _out) override
				{
					GtkComboBox* handle = (GtkComboBox*)m_handle;
					if (handle) {
						_out = getTextFromHanlde(handle);
						return sl_true;
					}
					return sl_false;
				}

				void setText(ComboBox* view, const String& text) override
				{
					GtkComboBox* handle = (GtkComboBox*)m_handle;
					GtkEntry* entry = (GtkEntry*)gtk_bin_get_child ((GtkBin*)handle);
					if(entry){
						gtk_entry_set_text(entry, text.getData());
					}
				}

				sl_ui_len measureHeight(ComboBox* view) override
				{
					/*
					HWND handle = m_handle;
					if (handle) {
						Ref<Font> font = m_font;
						if (font.isNotNull()) {
							sl_ui_len height = (sl_ui_len)(font->getFontHeight());
							height += 4;
							if (view->isBorder()) {
								height += 2;
							}
							return height;
						}
					}*/
					return 0;
				}

				void installEventHandlers()
				{
					GtkComboBox* handle = (GtkComboBox*)m_handle;
					g_signal_connect(handle, "changed", G_CALLBACK(onSelectedItem), handle);
				}
				static void onSelectedItem (GtkComboBox *widget, gpointer handle)
				{
					Ref<GTK_ViewInstance> instance = Ref<GTK_ViewInstance>::from(UIPlatform::getViewInstance((GtkWidget*)widget));
					Ref<ComboBoxInstance> comboBoxInstance = CastRef<ComboBoxInstance>(instance);
					if (comboBoxInstance.isNotNull()) {
						Ref<ComboBox> view = CastRef<ComboBox>(comboBoxInstance->getView());
						int index = gtk_combo_box_get_active(widget);
						view->dispatchSelectItem(index);

						String text = getTextFromHanlde(widget);
						String textNew = text;
						view->dispatchChange(&textNew);
						if (text != textNew) {
							comboBoxInstance->setText(view, textNew);
						}
					}
				}
			};

			SLIB_DEFINE_OBJECT(ComboBoxInstance, GTK_ViewInstance)
		}
	}

	using namespace priv::combo_box;

	Ref<ViewInstance> ComboBox::createNativeWidget(ViewInstance* parent)
	{
		GtkWidget* combobox = gtk_combo_box_entry_new_text();
		Ref<ComboBoxInstance> ret = GTK_ViewInstance::create<ComboBoxInstance>(this, parent, combobox);
		if (ret.isNotNull()) {
			ret->refreshItems(this, sl_true);
			ret->installEventHandlers();
			return ret;
		}
		return sl_null;
	}

	Ptr<IComboBoxInstance> ComboBox::getComboBoxInstance()
	{
		return CastRef<ComboBoxInstance>(getViewInstance());
	}

}

#endif
