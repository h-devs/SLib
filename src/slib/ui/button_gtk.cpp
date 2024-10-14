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

#include "slib/ui/button.h"

#include "button_gtk.h"

namespace slib
{

	namespace priv
	{

		SLIB_DEFINE_OBJECT(ButtonInstance, GTK_ViewInstance)

		ButtonInstance::ButtonInstance()
		{
		}

		ButtonInstance::~ButtonInstance()
		{
		}

		void ButtonInstance::initialize(View* _view)
		{
			Button* view = (Button*)(_view);
			GtkButton* handle = (GtkButton*)m_handle;

			setText(view, view->getText());
			setDefaultButton(view, view->isDefaultButton());

			g_signal_connect(handle, "clicked", G_CALLBACK(onClicked), handle);
		}

		void ButtonInstance::setText(Button* view, const String& _text)
		{
			GtkButton* handle = (GtkButton*)m_handle;
			if (handle) {
				if (view->isMnemonic()) {
					StringCstr text = _text.replaceAll('&', '_');
					gtk_button_set_label(handle, text.getData());
				} else {
					StringCstr text = _text;
					gtk_button_set_label(handle, text.getData());
				}
			}
		}

		void ButtonInstance::setDefaultButton(Button* view, sl_bool flag)
		{
			GtkWidget* handle = m_handle;
			if (handle) {
				gtk_widget_set_can_default(handle, flag);
			}
		}

		sl_bool ButtonInstance::measureSize(Button* view, UISize& _out)
		{
			GtkWidget* handle = m_handle;
			if(handle){
				Ref<Font> font = view->getFont();
				if (font.isNotNull()) {
					_out = font->getTextAdvance(view->getText());
					_out.x += view->getPaddingLeft() + view->getPaddingRight();
					_out.x += 32;
					_out.y += view->getPaddingTop() + view->getPaddingBottom();
					_out.y += 16;
					return true;
				}
			}
			return sl_false;
		}

		void ButtonInstance::onClicked(GtkButton*, gpointer userinfo)
		{
			GtkButton* handle = (GtkButton*)userinfo;
			Ref<View> view = UIPlatform::getView((GtkWidget*)handle);
			if (view.isNotNull()) {
				view->invokeClickEvent();
			}
		}

	}

	using namespace priv;

	Ref<ViewInstance> Button::createNativeWidget(ViewInstance* parent)
	{
		GtkWidget *handle;
		if (isMnemonic()) {
			handle = gtk_button_new_with_mnemonic("");
		} else {
			handle = gtk_button_new();
		}
		return GTK_ViewInstance::create<ButtonInstance>(this, parent, handle);
	}

	Ptr<IButtonInstance> Button::getButtonInstance()
	{
		return CastRef<ButtonInstance>(getViewInstance());
	}

}

#endif
