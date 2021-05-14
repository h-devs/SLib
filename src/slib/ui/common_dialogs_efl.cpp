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

#if defined(SLIB_UI_IS_EFL)

#include "slib/ui/common_dialogs.h"
#include "slib/ui/core.h"
#include "slib/ui/window.h"
#include "slib/ui/platform.h"

#include <Elementary.h>
#include <efl_extension.h>

namespace slib
{

	namespace priv
	{
		namespace alert_dialog
		{

			struct AlertDialogContainer
			{
				Evas_Object* popup;
				Ref<AlertDialog> alert;
				DialogResult result;
			};

			static void alert_dialog_ok_cb(void *data, Evas_Object *obj, void *event_info)
			{
				AlertDialogContainer* container = reinterpret_cast<AlertDialogContainer*>(data);
				container->result = DialogResult::Ok;
				elm_popup_dismiss(container->popup);
			}

			static void alert_dialog_yes_cb(void *data, Evas_Object *obj, void *event_info)
			{
				AlertDialogContainer* container = reinterpret_cast<AlertDialogContainer*>(data);
				container->result = DialogResult::Yes;
				elm_popup_dismiss(container->popup);
			}

			static void alert_dialog_cancel_cb(void *data, Evas_Object *obj, void *event_info)
			{
				AlertDialogContainer* container = reinterpret_cast<AlertDialogContainer*>(data);
				container->result = DialogResult::Cancel;
				elm_popup_dismiss(container->popup);
			}

			static void alert_dialog_no_cb(void *data, Evas_Object *obj, void *event_info)
			{
				AlertDialogContainer* container = reinterpret_cast<AlertDialogContainer*>(data);
				container->result = DialogResult::No;
				elm_popup_dismiss(container->popup);
			}
			
			static void alert_dialog_dismissed_cb(void *data, Evas_Object *obj, void *event_info)
			{
				AlertDialogContainer* container = reinterpret_cast<AlertDialogContainer*>(data);
				evas_object_del(container->popup);
				switch (container->result) {
					case DialogResult::Ok:
						container->alert->_onResult(DialogResult::Ok);
						break;
					case DialogResult::Yes:
						container->alert->_onResult(DialogResult::Yes);
						break;
					case DialogResult::No:
						container->alert->_onResult(DialogResult::No);
						break;
					default:
						container->alert->_onResult(DialogResult::Cancel);
						break;
				}
				delete container;
			}

		}
	}

	using priv::alert_dialog;

	DialogResult AlertDialog::run()
	{
		return _runByShow();
	}
	
	DialogResult AlertDialog::_run()
	{
		return DialogResult::Cancel;
	}
	
	void AlertDialog::show()
	{
		_showOnUiThread();
	}

	sl_bool AlertDialog::_show()
	{
		AlertDialogButtons buttons = this->buttons;

		Evas_Object* win;
		Ref<Window> parent = this->parent;
		Ref<WindowInstance> instance;
		if (parent.isNotNull()) {
			if (parent.isNotNull()) {
				instance = parent->getWindowInstance();
			}
			win = UIPlatform::getWindowHandle(instance.get());
		} else {
			win = UIPlatform::getMainWindow();
		}

		if (!win) {
			return sl_false;
		}

		Evas_Object* popup = elm_popup_add(win);
		if (!popup) {
			return sl_false;
		}

		AlertDialogContainer* container = new AlertDialogContainer;
		if (!container) {
			return sl_false;
		}
		container->popup = popup;
		container->alert = this;
		container->result = DialogResult::Cancel;

		evas_object_layer_set(popup, EVAS_LAYER_MAX);
		elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
		evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

		StringCstr _caption(caption);
		elm_object_part_text_set(popup, "title,text", _caption.getData());
		StringCstr _text(text);
		elm_object_text_set(popup, _text.getData());

		StringCstr titleOk = this->titleOk;
		if (titleOk.isEmpty()) {
			titleOk = "OK";
		}
		StringCstr titleCancel = this->titleCancel;
		if (titleCancel.isEmpty()) {
			titleCancel = "Cancel";
		}
		StringCstr titleYes = this->titleYes;
		if (titleYes.isEmpty()) {
			titleYes = "Yes";
		}
		StringCstr titleNo = this->titleNo;
		if (titleNo.isEmpty()) {
			titleNo = "No";
		}
		
		if (buttons == AlertDialogButtons::OkCancel) {
			Evas_Object* button1 = elm_button_add(popup);
			elm_object_text_set(button1, titleOk.getData());
			evas_object_smart_callback_add(button1, "clicked", alert_dialog_ok_cb, container);
			elm_object_part_content_set(popup, "button1", button1);
			Evas_Object* button2 = elm_button_add(popup);
			elm_object_text_set(button2, titleCancel.getData());
			evas_object_smart_callback_add(button2, "clicked", alert_dialog_cancel_cb, container);
			elm_object_part_content_set(popup, "button2", button2);
			eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, alert_dialog_cancel_cb, container);
			evas_object_smart_callback_add(popup, "block,clicked", alert_dialog_cancel_cb, container);
		} else if (buttons == AlertDialogButtons::YesNo) {
			Evas_Object* button1 = elm_button_add(popup);
			elm_object_text_set(button1, titleYes.getData());
			evas_object_smart_callback_add(button1, "clicked", alert_dialog_yes_cb, container);
			elm_object_part_content_set(popup, "button1", button1);
			Evas_Object* button2 = elm_button_add(popup);
			elm_object_text_set(button2, titleNo.getData());
			evas_object_smart_callback_add(button2, "clicked", alert_dialog_no_cb, container);
			elm_object_part_content_set(popup, "button2", button2);
		} else if (buttons == AlertDialogButtons::YesNoCancel) {
			Evas_Object* button1 = elm_button_add(popup);
			elm_object_text_set(button1, titleYes.getData());
			evas_object_smart_callback_add(button1, "clicked", alert_dialog_yes_cb, container);
			elm_object_part_content_set(popup, "button1", button1);
			Evas_Object* button2 = elm_button_add(popup);
			elm_object_text_set(button2, titleNo.getData());
			evas_object_smart_callback_add(button2, "clicked", alert_dialog_no_cb, container);
			elm_object_part_content_set(popup, "button2", button2);
			Evas_Object* button3 = elm_button_add(popup);
			elm_object_text_set(button3, titleCancel.getData());
			evas_object_smart_callback_add(button3, "clicked", alert_dialog_cancel_cb, container);
			elm_object_part_content_set(popup, "button3", button3);
			eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, alert_dialog_cancel_cb, container);
			evas_object_smart_callback_add(popup, "block,clicked", alert_dialog_cancel_cb, container);
		} else {
			Evas_Object* button1 = elm_button_add(popup);
			elm_object_text_set(button1, titleOk.getData());
			evas_object_smart_callback_add(button1, "clicked", alert_dialog_ok_cb, container);
			elm_object_part_content_set(popup, "button1", button1);
			eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, alert_dialog_ok_cb, container);
			evas_object_smart_callback_add(popup, "block,clicked", alert_dialog_ok_cb, container);
		}
		
		evas_object_smart_callback_add(popup, "dismissed", alert_dialog_dismissed_cb, &container);
		evas_object_show(popup);

		return sl_true;

	}
	
}

#endif
