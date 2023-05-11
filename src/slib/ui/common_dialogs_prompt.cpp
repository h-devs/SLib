/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/common_dialogs.h"

#include "slib/ui/core.h"
#include "slib/ui/label_view.h"
#include "slib/ui/edit_view.h"
#include "slib/core/variant.h"

#include "../resources.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PromptDialog)

	PromptDialog::PromptDialog()
	{
	}

	namespace {
		static Ref<ui::PromptDialog> CreateDialog(PromptDialog& param)
		{
			Ref<ui::PromptDialog> dlg = new ui::PromptDialog;
			if (dlg.isNotNull()) {
				dlg->setParent(param.parent);
				dlg->setTitle(param.caption);
				dlg->setCloseOnOK();
				dlg->label->setText(param.message);
				if (param.defaultValue.isNotEmpty()) {
					dlg->input->setText(param.defaultValue);
					dlg->input->selectAll();
				}
				return dlg;
			}
			return sl_null;
		}
	}

	String PromptDialog::run()
	{
		Ref<ui::PromptDialog> dlg = CreateDialog(*this);
		if (dlg.isNotNull()) {
			if (dlg->doModal() == DialogResult::OK) {
				return dlg->input->getText();
			}
		}
		return sl_null;
	}

	void PromptDialog::show()
	{
		Ref<ui::PromptDialog> dlg = CreateDialog(*this);
		if (dlg.isNotNull()) {
			dlg->increaseReference();
			const Function<void(String&)>& _onOK = onOK;
			const Function<void()>& _onCancel = onCancel;
			dlg->setOnDestroy([_onOK, _onCancel](Window* dlg, UIEvent*) {
				if (dlg->getResult() == DialogResult::OK) {
					String text = ((ui::PromptDialog*)dlg)->input->getText();
					_onOK(text);
				} else {
					_onCancel();
				}
				dlg->decreaseReference();
			});
			dlg->showModal();
		} else {
			onCancel();
		}
	}


	String UI::prompt(const StringParam& message)
	{
		return prompt(sl_null, sl_null, message, sl_null);
	}

	String UI::prompt(const StringParam& caption, const StringParam& message, const StringParam& defaultValue)
	{
		return prompt(sl_null, caption, message, defaultValue);
	}

	String UI::prompt(const Ref<Window>& parent, const StringParam& message)
	{
		return prompt(parent, sl_null, message, sl_null);
	}

	String UI::prompt(const Ref<Window>& parent, const StringParam& caption, const StringParam& message, const StringParam& defaultValue)
	{
		PromptDialog dlg;
		dlg.parent = parent;
		dlg.caption = caption.toString();
		dlg.message = message.toString();
		dlg.defaultValue = defaultValue.toString();
		return dlg.run();
	}

	void UI::showPrompt(const StringParam& message, const Function<void(String&)>& onResult)
	{
		showPrompt(sl_null, message, sl_null, onResult);
	}

	void UI::showPrompt(const StringParam& caption, const StringParam& message, const StringParam& defaultValue, const Function<void(String&)>& onResult)
	{
		showPrompt(sl_null, caption, message, defaultValue, onResult);
	}

	void UI::showPrompt(const Ref<Window>& parent, const StringParam& message, const Function<void(String&)>& onResult)
	{
		showPrompt(parent, sl_null, message, sl_null, onResult);
	}

	void UI::showPrompt(const Ref<Window>& parent, const StringParam& caption, const StringParam& message, const StringParam& defaultValue, const Function<void(String&)>& onResult)
	{
		PromptDialog dlg;
		dlg.parent = parent;
		dlg.caption = caption.toString();
		dlg.message = message.toString();
		dlg.defaultValue = defaultValue.toString();
		dlg.onOK = [onResult](String& value) {
			onResult(value);
		};
		dlg.onCancel = [onResult]() {
			String value;
			onResult(value);
		};
		dlg.show();
	}

}
