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

#ifndef CHECKHEADER_SLIB_UI_COMBO_BOX
#define CHECKHEADER_SLIB_UI_COMBO_BOX

#include "definition.h"

#include "label_list.h"

namespace slib
{
	
	class IComboBoxInstance;

	class ComboBoxCell;

	class SLIB_EXPORT ComboBox : public View, public SingleSelectionViewBase<ComboBox, sl_int32>
	{
		SLIB_DECLARE_OBJECT
		
	public:
		ComboBox();
		
		~ComboBox();

	protected:
		void init() override;

	public:
		String getText();

		String getInstanceText();

		void setText(const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);

	public:
		SLIB_DECLARE_EVENT_HANDLER(ComboBox, SelectItem, sl_int32 index)		
		
		SLIB_DECLARE_EVENT_HANDLER(ComboBox, Change, String* pValue)

		SLIB_DECLARE_EVENT_HANDLER(ComboBox, ReturnKey)

	protected:
		void onUpdateLayout() override;
		
	public:
		void dispatchKeyEvent(UIEvent* ev) override;

	protected:
		Ref<ViewInstance> createNativeWidget(ViewInstance* parent) override;
		
		virtual Ptr<IComboBoxInstance> getComboBoxInstance();
	
	public:
		SLIB_DECLARE_SINGLE_SELECTION_VIEW_NOTIFY_FUNCTIONS(ComboBox, sl_int32)
	
	protected:
		AtomicString m_text;

		Ref<ComboBoxCell> m_cell;

	};
	
	class SLIB_EXPORT IComboBoxInstance
	{
	public:
		SLIB_DECLARE_SINGLE_SELECTION_VIEW_INSTANCE_NOTIFY_FUNCTIONS(ComboBox, sl_int32)

		virtual sl_bool getText(ComboBox* view, String& _out) = 0;

		virtual void setText(ComboBox* view, const String& text) = 0;

		virtual sl_ui_len measureHeight(ComboBox* view);

	};

	class ComboBoxCell : public SingleSelectionViewCellBase<sl_uint32>
	{
		SLIB_DECLARE_OBJECT

	public:
		AtomicString text;

	public:
		ComboBoxCell();

		~ComboBoxCell();

	};

}

#endif
