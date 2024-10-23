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

#include "slib/ui/radio_button.h"

#include "button_gtk.h"

namespace slib
{

	using namespace priv;

	namespace {
		static void OnClickRadio(GtkButton* button)
		{
			GtkButtonClass* clsRadio = GTK_BUTTON_GET_CLASS(button);
			GtkButtonClass* clsCheck = (GtkButtonClass*)(g_type_class_peek_parent(clsRadio));
			if (!(gtk_toggle_button_get_active((GtkToggleButton*)button))) {
				clsCheck->clicked(button);
			}
		}
	}

	Ref<ViewInstance> RadioButton::createNativeWidget(ViewInstance* parent)
	{
		GtkWidget* handle = gtk_radio_button_new_with_mnemonic(NULL, "");
		Ref<CheckBoxInstance> ret = PlatformViewInstance::create<CheckBoxInstance>(this, parent, handle);
		if (ret.isNotNull()) {
			GtkButtonClass* clsRadio = GTK_BUTTON_GET_CLASS(handle);
			clsRadio->clicked = OnClickRadio;
			return ret;
		}
		return sl_null;
	}

}

#endif
