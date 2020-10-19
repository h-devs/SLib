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

#include "slib/ui/check_box.h"

#include "button_gtk.h"

namespace slib
{

	namespace priv
	{
		namespace button
		{

			SLIB_DEFINE_OBJECT(CheckBoxInstance, ButtonInstance)

			CheckBoxInstance::CheckBoxInstance()
			{
			}

			CheckBoxInstance::~CheckBoxInstance()
			{
			}

			void CheckBoxInstance::initialize(View* _view)
			{
				CheckBox* view = (CheckBox*)_view;
				GtkToggleButton* handle = (GtkToggleButton*)m_handle;

				ButtonInstance::initialize(view);
				gtk_toggle_button_set_active(handle, view->isChecked());

				g_signal_connect(handle, "toggled", G_CALLBACK(onChanged), handle);
			}

			sl_bool CheckBoxInstance::getChecked(CheckBox* view, sl_bool& _out)
			{
				GtkWidget* handle = m_handle;
				if (handle) {
					_out = gtk_toggle_button_get_active((GtkToggleButton *)handle);
					return sl_true;
				}
				return sl_false;
			}

			void CheckBoxInstance::setChecked(CheckBox* view, sl_bool flag)
			{
				GtkWidget* handle = m_handle;
				if (handle) {
					gtk_toggle_button_set_active((GtkToggleButton *)handle, flag);
				}
			}

			sl_bool CheckBoxInstance::measureSize(Button* view, UISize& _out)
			{
				GtkWidget* handle = m_handle;
				if(handle){
					Ref<Font> font = view->getFont();
					if (font.isNotNull()) {
						_out = font->measureText(" " + view->getText());
						return true;
					}
				}
				return sl_false;
			}
			
			void CheckBoxInstance::onChanged(GtkToggleButton *, gpointer userinfo)
			{
				GtkCheckButton* handle = (GtkCheckButton*)userinfo;
				Ref<CheckBox> view = CastRef<CheckBox>(UIPlatform::getView((GtkWidget*)handle));
				if (view.isNotNull()) {
					bool isChecked = gtk_toggle_button_get_active((GtkToggleButton*)handle);
					view->dispatchChange(isChecked);
				}
			}

		}
	}

	using namespace priv::button;

	Ref<ViewInstance> CheckBox::createNativeWidget(ViewInstance* parent)
	{
		GtkWidget* handle = gtk_check_button_new_with_mnemonic("");
		return GTK_ViewInstance::create<CheckBoxInstance>(this, parent, handle);
	}

	Ptr<ICheckBoxInstance> CheckBox::getCheckBoxInstance()
	{
		return CastRef<CheckBoxInstance>(getViewInstance());
	}

}

#endif
