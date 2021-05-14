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

#ifndef CHECKHEADER_SLIB_UI_COMBO_BOX_GTK
#define CHECKHEADER_SLIB_UI_COMBO_BOX_GTK

#include "view_gtk.h"

namespace slib
{

	namespace priv
	{
		namespace combo_box
		{

			template <class VIEW>
			static void RefreshItems(GtkComboBox* handle, VIEW* view, sl_bool flagInit)
			{
				if (!flagInit) {
					GtkListStore* model = (GtkListStore*)(gtk_combo_box_get_model(handle));
					gtk_list_store_clear(model);
				}
				sl_uint32 n = view->getItemsCount();
				for (sl_uint32 i = 0; i < n; i++) {
					StringCstr s = view->getItemTitle(i);
					gtk_combo_box_append_text(handle, (const gchar*)(s.getData()));
				}
				sl_int32 indexSelected = view->getSelectedIndex();
				if (indexSelected >= 0 && (sl_uint32)indexSelected < n) {
					if (gtk_combo_box_get_active(handle) != indexSelected) {
						gtk_combo_box_set_active(handle, indexSelected);
					}
				}
			}
			
			static void InsertItem(GtkComboBox* handle, sl_int32 index, const String& _title)
			{
				StringCstr title(_title);
				gtk_combo_box_insert_text(handle, index, title.getData());
			}

			static void RemoveItem(GtkComboBox* handle, sl_int32 index)
			{
				gtk_combo_box_remove_text(handle, index);
			}

			static void SetItemTitle(GtkComboBox* handle, sl_int32 index, const String& _title)
			{
				GtkTreeModel* model = gtk_combo_box_get_model(handle);
				GtkTreePath* path = gtk_tree_path_new_from_indices(index);
				if(path){
					GtkTreeIter iter;
					gtk_tree_model_get_iter(model, &iter, path);
					gtk_tree_path_free(path);
					StringCstr title(_title);
					gtk_list_store_set((GtkListStore*)model, &iter, 0, title.getData(), -1);
				}
			}

			static void SelectItem(GtkComboBox* handle, sl_int32 index)
			{
				gtk_combo_box_set_active(handle, index);
			}

		}
	}

}

#endif
