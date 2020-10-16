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

			class ComboBoxHelper : public ComboBox
			{
			public:
				/*
				void onChange(HWND handle)
				{
					String text = Windows::getWindowText(handle);
					String textNew = text;
					dispatchChange(&textNew);
					if (text != textNew) {
						Windows::setWindowText(handle, textNew);
					}
				}
				*/
				using ComboBox::m_text;

			};
			enum {
			  COLUMN_STRING,
			  COLUMN_INT,
			  COLUMN_BOOLEAN,
			  N_COLUMNS
			};
			class ComboBoxInstance : public GTK_ViewInstance, public IComboBoxInstance
			{
				SLIB_DECLARE_OBJECT

			public:
				void refreshItems(ComboBox* view) override
				{
					refreshItems(view, sl_false);
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
						//GtkTreeModel *list_store;
						//GtkTreeIter iter;
						//list_store = (GtkTreeModel *)gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
						for (sl_uint32 i = 0; i < n; i++) {
							String16 s = String16::from(view->getItemTitle(i));
							//gtk_list_store_append ((GtkListStore*)list_store, &iter);
							/*gtk_list_store_set ((GtkListStore*)list_store, &iter,
													  COLUMN_STRING, s.getData(),
													  COLUMN_INT, i,
													  COLUMN_BOOLEAN,  FALSE,
													  -1);*/
							gtk_combo_box_append_text((GtkComboBox*)combo_box, (const char*)s.getData());
						}
						//gtk_combo_box_set_model(combo_box, list_store);
						sl_int32 indexSelected = view->getSelectedIndex();
						if (indexSelected >= 0 && (sl_uint32)indexSelected < n) {

							if (gtk_combo_box_get_active(combo_box) != indexSelected) {
								gtk_combo_box_set_active(combo_box, indexSelected);
								((ComboBoxHelper*)view)->m_text = view->getItemTitle(indexSelected);
								/*
								UI::dispatchToUiThread([handle]() {
									SendMessageW(handle, CB_SETEDITSEL, 0, SLIB_MAKE_DWORD2(-1, -1));
								});*/
							}
						}
						if(indexSelected == -1)
						{
							gtk_combo_box_set_active(combo_box, 0);
							((ComboBoxHelper*)view)->m_text = view->getItemTitle(0);
						}
					}
				}

				void insertItem(ComboBox* view, sl_int32 index, const String& title) override
				{
					GtkWidget* handle = m_handle;
					GtkComboBox* combo_box = (GtkComboBox*)handle;
					if (handle) {
						String16 s = String16::from(title);
						GtkTreeModel *list_store = gtk_combo_box_get_model(combo_box);
						GtkTreeIter iter;
						/*
						gtk_list_store_insert((GtkListStore*)list_store, &iter, index);
						gtk_list_store_set ((GtkListStore*)list_store, &iter,
												  COLUMN_STRING, s.getData(),
												  COLUMN_INT, index,
												  COLUMN_BOOLEAN,  FALSE,
												  -1);*/
						//gtk_combo_box_insert_text(combo_box, index, s.getData());
					}
				}

				void removeItem(ComboBox* view, sl_int32 index)
				{
					GtkWidget* handle = m_handle;
					GtkComboBox* combo_box = (GtkComboBox*)handle;
					if (handle) {
						GtkTreeModel *list_store = gtk_combo_box_get_model(combo_box);

						//gtk_combo_box_remove_text(combo_box, index);
					}
				}

				void setItemTitle(ComboBox* view, sl_int32 index, const String& title) override
				{
					/*
					HWND handle = m_handle;
					if (handle) {
						String16 s = String16::from(title);
						SendMessageW(handle, CB_DELETESTRING, (WPARAM)index, 0);
						SendMessageW(handle, CB_INSERTSTRING, (WPARAM)index, (LPARAM)(s.getData()));
					}*/
				}

				void selectItem(ComboBox* view, sl_int32 index) override
				{
					/*
					HWND handle = m_handle;
					if (handle) {
						SendMessageW(handle, CB_SETCURSEL, (WPARAM)index, 0);
					}*/
				}

				sl_bool getText(ComboBox* view, String& _out) override
				{
					GtkWidget* handle = m_handle;
					GtkComboBox* combo_box = (GtkComboBox*)handle;
					if (handle) {
						//_out = gtk_combo_box_get_title(combo_box);
						_out = view->getText();
						return sl_true;
					}
					return sl_false;
				}

				void setText(ComboBox* view, const String& text) override
				{
					view->setText(text);
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
/*
				sl_bool processCommand(SHORT code, LRESULT& result) override
				{
					if (code == CBN_SELCHANGE) {
						Ref<ComboBoxHelper> helper = CastRef<ComboBoxHelper>(getView());
						if (helper.isNotNull()) {
							sl_uint32 index = (sl_uint32)(SendMessageW(m_handle, CB_GETCURSEL, 0, 0));
							helper->dispatchSelectItem(index);							
							result = 0;
							return sl_true;
						}
					} else if (code == CBN_EDITCHANGE) {
						Ref<ComboBoxHelper> helper = CastRef<ComboBoxHelper>(getView());
						if (helper.isNotNull()) {
							helper->onChange(m_handle);
							result = 0;
							return sl_true;
						}
					}
					return sl_false;
				}
*/
			};

			SLIB_DEFINE_OBJECT(ComboBoxInstance, GTK_ViewInstance)
/*
			LRESULT CALLBACK EditChildSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
			{
				if (uMsg == WM_KEYDOWN) {
					Ref<GTK_ViewInstance> instance = Ref<GTK_ViewInstance>::from(UIPlatform::getViewInstance(GetParent(hWnd)));
					if (instance.isNotNull()) {
						return instance->processSubclassMessage(uMsg, wParam, lParam);
					}
				}
				return DefSubclassProc(hWnd, uMsg, wParam, lParam);
			}

			BOOL CALLBACK SubclassEditChild(HWND hWnd, LPARAM lParam)
			{
				WCHAR sz[16];
				int n = GetClassNameW(hWnd, sz, 16);
				if (n) {
					if (String16::from(sz, (sl_size)n).equalsIgnoreCase(SLIB_UNICODE("EDIT"))) {
						SetWindowSubclass(hWnd, EditChildSubclassProc, 0, 0);
					}
				}
				return TRUE;
			}
*/
		}
	}

	using namespace priv::combo_box;

	Ref<ViewInstance> ComboBox::createNativeWidget(ViewInstance* parent)
	{
		//UINT style = CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_TABSTOP;
		GtkWidget* combobox = gtk_combo_box_entry_new_text();
		Ref<ComboBoxInstance> ret = GTK_ViewInstance::create<ComboBoxInstance>(this, parent, combobox);
		if (ret.isNotNull()) {
			//EnumChildWindows(ret->getHandle(), SubclassEditChild, 0);
			ret->refreshItems(this, sl_true);
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
