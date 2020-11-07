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

#include "combo_box_gtk.h"

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
				static GtkEntry* getEntry(GtkComboBox* handle)
				{
					return (GtkEntry*)gtk_bin_get_child((GtkBin*)handle);
				}

				static String getTextFromHanlde(GtkComboBox* handle)
				{
					GtkEntry* entry = getEntry(handle);
					if (entry) {
						return gtk_entry_get_text(entry);
					}
					return sl_null;
				}

				void initialize(View* _view) override
				{
					ComboBox* view = (ComboBox*)_view;
					GtkComboBox* handle = (GtkComboBox*)m_handle;

					refreshItems(view, sl_true);
					String text = view->getText();
					if (text.isNotEmpty()) {
						setText(view, text);
					}

					g_signal_connect(handle, "changed", G_CALLBACK(onChanged), handle);

					GtkEntry* entry = getEntry(handle);
					if (entry) {
						g_signal_connect(entry, "focus-in-event", G_CALLBACK(eventCallback), handle);
						g_signal_connect(entry, "key-press-event", G_CALLBACK(eventCallback), handle);
						gtk_widget_set_events((GtkWidget*)entry, gtk_widget_get_events((GtkWidget*)entry) | GDK_KEY_PRESS_MASK | GDK_FOCUS_CHANGE_MASK);
					}
				}
				
				void refreshItems(ComboBox* view, sl_bool flagInit)
				{
					GtkComboBox* handle = (GtkComboBox*)m_handle;
					if (handle) {
						RefreshItems(handle, view, flagInit);
					}
				}

				void refreshItems(ComboBox* view) override
				{
					refreshItems(view, sl_false);
				}

				void insertItem(ComboBox* view, sl_int32 index, const String& title) override
				{
					GtkComboBox* handle = (GtkComboBox*)m_handle;
					if (handle) {
						InsertItem(handle, index, title);
					}
				}

				void removeItem(ComboBox* view, sl_int32 index) override
				{
					GtkComboBox* handle = (GtkComboBox*)m_handle;
					if (handle) {
						RemoveItem(handle, index);
					}
				}

				void setItemTitle(ComboBox* view, sl_int32 index, const String& title) override
				{
					GtkComboBox* handle = (GtkComboBox*)m_handle;
					if (handle) {
						SetItemTitle(handle, index, title);
					}
				}

				void selectItem(ComboBox* view, sl_int32 index) override
				{
					GtkComboBox* handle = (GtkComboBox*)m_handle;
					if (handle) {
						SelectItem(handle, index);
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
					if (handle) {
						GtkEntry* entry = getEntry(handle);
						if(entry){
							gtk_entry_set_text(entry, text.getData());
						}
					}
				}

				void setFont(View* view, const Ref<Font>& font) override
				{
					GtkComboBox* handle = (GtkComboBox*)m_handle;
					if (handle) {
						GtkEntry* entry = getEntry(handle);
						if(entry){
							UIPlatform::setWidgetFont((GtkWidget*)entry, font);
						}
					}
				}

				sl_ui_len measureHeight(ComboBox* view) override
				{
					Ref<Font> font = view->getFont();
					if (font.isNotNull()) {
						return (sl_ui_len)(font->getFontHeight() * 1.5f + 2);
					}
					return 0;
				}

				static void onChanged(GtkComboBox*, gpointer userinfo)
				{
					GtkComboBox* handle = (GtkComboBox*)userinfo;
					Ref<ComboBoxInstance> instance = CastRef<ComboBoxInstance>(UIPlatform::getViewInstance((GtkWidget*)handle));
					if (instance.isNotNull()) {
						Ref<ComboBox> view = CastRef<ComboBox>(instance->getView());
						if (view.isNotNull()) {
							int index = gtk_combo_box_get_active(handle);
							if (index != view->getSelectedIndex()) {
								view->dispatchSelectItem(index);
							}
							String text = getTextFromHanlde(handle);
							String textNew = text;
							view->dispatchChange(textNew);
							if (text != textNew) {
								instance->setText(view, textNew);
							}
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
		GtkWidget* handle = gtk_combo_box_entry_new_text();
		return GTK_ViewInstance::create<ComboBoxInstance>(this, parent, handle);
	}

	Ptr<IComboBoxInstance> ComboBox::getComboBoxInstance()
	{
		return CastRef<ComboBoxInstance>(getViewInstance());
	}

}

#endif
