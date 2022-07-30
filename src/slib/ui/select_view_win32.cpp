/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#if defined(SLIB_UI_IS_WIN32)

#include "slib/ui/select_view.h"

#include "view_win32.h"

namespace slib
{

	namespace priv
	{
		namespace select_view
		{

			class SelectViewInstance : public Win32_ViewInstance, public ISelectViewInstance
			{
				SLIB_DECLARE_OBJECT

			public:
				void initialize(View* _view) override
				{
					SelectView* view = (SelectView*)_view;
					refreshItems(view);
				}

				void refreshItems(SelectView* view) override
				{
					HWND handle = m_handle;
					if (handle) {
						SendMessageW(handle, CB_RESETCONTENT, 0, 0);
						sl_uint32 n = view->getItemCount();
						for (sl_uint32 i = 0; i < n; i++) {
							StringCstr16 s = view->getItemTitle(i);
							SendMessageW(handle, CB_ADDSTRING, 0, (LPARAM)(s.getData()));
						}
						sl_uint32 indexSelected = view->getSelectedIndex();
						if (indexSelected < n) {
							if (SendMessageW(handle, CB_GETCURSEL, 0, 0) != (LRESULT)indexSelected) {
								SendMessageW(handle, CB_SETCURSEL, (WPARAM)indexSelected, 0);
							}
						}
					}
				}

				void insertItem(SelectView* view, sl_uint32 index, const String& _title) override
				{
					HWND handle = m_handle;
					if (handle) {
						StringCstr16 title = _title;
						SendMessageW(handle, CB_INSERTSTRING, (WPARAM)index, (LPARAM)(title.getData()));
					}
				}

				void removeItem(SelectView* view, sl_uint32 index) override
				{
					HWND handle = m_handle;
					if (handle) {
						SendMessageW(handle, CB_DELETESTRING, (WPARAM)index, 0);
					}
				}

				void setItemTitle(SelectView* view, sl_uint32 index, const String& _title) override
				{
					HWND handle = m_handle;
					if (handle) {
						StringCstr16 title = _title;
						SendMessageW(handle, CB_DELETESTRING, (WPARAM)index, 0);
						SendMessageW(handle, CB_INSERTSTRING, (WPARAM)index, (LPARAM)(title.getData()));
					}
				}

				void selectItem(SelectView* view, sl_uint32 index) override
				{
					HWND handle = m_handle;
					if (handle) {
						SendMessageW(handle, CB_SETCURSEL, (WPARAM)index, 0);
					}
				}

				sl_bool processCommand(SHORT code, LRESULT& result) override
				{
					if (code == CBN_SELCHANGE) {
						Ref<SelectView> helper = CastRef<SelectView>(getView());
						if (helper.isNotNull()) {
							sl_uint32 index = (sl_uint32)(SendMessageW(m_handle, CB_GETCURSEL, 0, 0));
							helper->dispatchSelectItem(index);
							result = 0;
							return sl_true;
						}
					}
					return sl_false;
				}

			};

			SLIB_DEFINE_OBJECT(SelectViewInstance, Win32_ViewInstance)

		}
	}

	using namespace priv::select_view;

	Ref<ViewInstance> SelectView::createNativeWidget(ViewInstance* parent)
	{
		UINT style = CBS_DROPDOWNLIST | WS_TABSTOP;
		return Win32_ViewInstance::create<SelectViewInstance>(this, parent, L"COMBOBOX", sl_null, style, 0);
	}

	Ptr<ISelectViewInstance> SelectView::getSelectViewInstance()
	{
		return CastRef<SelectViewInstance>(getViewInstance());
	}

}

#endif
