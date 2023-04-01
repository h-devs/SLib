/*
 *   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_CHECK_BOX
#define CHECKHEADER_SLIB_UI_CHECK_BOX

#include "button.h"

namespace slib
{

	class ICheckBoxInstance;

	class SLIB_EXPORT CheckBox : public Button
	{
		SLIB_DECLARE_OBJECT

	public:
		CheckBox();

		~CheckBox();

	public:
		sl_bool isChecked();

		sl_bool isCheckedInstance();

		void setChecked(sl_bool flag, UIUpdateMode mode = UIUpdateMode::Redraw);

	public:
		SLIB_DECLARE_EVENT_HANDLER(CheckBox, Change, sl_bool value, UIEvent* ev /* nullable */)

	protected:
		Ref<ViewInstance> createNativeWidget(ViewInstance* parent) override;

		virtual Ptr<ICheckBoxInstance> getCheckBoxInstance();

		Ref<ButtonCell> createButtonCell() override;

	public:
		void onClickEvent(UIEvent* ev) override;

		void onMnemonic(UIEvent* ev) override;

	private:
		void _changeValue(sl_bool value, UIEvent* ev, UIUpdateMode mode);

	protected:
		sl_bool m_flagChecked;

	};

	class SLIB_EXPORT ICheckBoxInstance
	{
	public:
		virtual sl_bool getChecked(CheckBox* view, sl_bool& flag) = 0;

		virtual void setChecked(CheckBox* view, sl_bool flag) = 0;

	};

	class SLIB_EXPORT CheckBoxCell : public ButtonCell
	{
		SLIB_DECLARE_OBJECT

	public:
		CheckBoxCell();

		CheckBoxCell(const Array<ButtonCategory>& categories);

		~CheckBoxCell();

	};

}

#endif
