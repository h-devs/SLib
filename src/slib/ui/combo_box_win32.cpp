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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_WIN32)

#include "slib/ui/combo_box.h"

#include "view_win32.h"

#include <commctrl.h>

namespace slib
{

	namespace {

		class ComboBoxHelper : public ComboBox
		{
		public:
			void onChange(HWND handle)
			{
				String text = UIPlatform::getWindowText(handle);
				String textNew = text;
				dispatchChange(textNew);
				if (text != textNew) {
					UIPlatform::setWindowText(handle, textNew);
				}
			}

		};

		LRESULT CALLBACK EditChildSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
		{
			if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) {
				Ref<Win32_ViewInstance> instance = Ref<Win32_ViewInstance>::from(UIPlatform::getViewInstance(GetParent(hWnd)));
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
				if (String16::from(sz, (sl_size)n).equals_IgnoreCase(SLIB_UNICODE("EDIT"))) {
					SetWindowSubclass(hWnd, EditChildSubclassProc, 0, 0);
				}
			}
			return TRUE;
		}

		class ComboBoxInstance : public Win32_ViewInstance, public IComboBoxInstance
		{
			SLIB_DECLARE_OBJECT

		public:
			void initialize(View* _view) override
			{
				ComboBox* view = (ComboBox*)_view;

				EnumChildWindows(getHandle(), SubclassEditChild, 0);
				String text = view->getText();
				if (text.isNotEmpty()) {
					Win32_ViewInstance::setText(text);
				}
				refreshItems(view, sl_true);
			}

			void refreshItems(ComboBox* view, sl_bool flagInit)
			{
				HWND handle = m_handle;
				if (handle) {
					if (!flagInit) {
						SendMessageW(handle, CB_RESETCONTENT, 0, 0);
					}
					sl_uint32 n = view->getItemCount();
					for (sl_uint32 i = 0; i < n; i++) {
						StringCstr16 s = view->getItemTitle(i);
						SendMessageW(handle, CB_ADDSTRING, 0, (LPARAM)(s.getData()));
					}
					sl_int32 indexSelected = view->getSelectedIndex();
					if (indexSelected >= 0 && (sl_uint32)indexSelected < n) {
						if (SendMessageW(handle, CB_GETCURSEL, 0, 0) != (LRESULT)indexSelected) {
							SendMessageW(handle, CB_SETCURSEL, (WPARAM)indexSelected, 0);
							UI::dispatchToUiThread([handle]() {
								SendMessageW(handle, CB_SETEDITSEL, 0, SLIB_MAKE_DWORD2(-1, -1));
							});
						}
					}
				}
			}

			void refreshItems(ComboBox* view) override
			{
				refreshItems(view, sl_false);
			}

			void insertItem(ComboBox* view, sl_int32 index, const String& title) override
			{
				HWND handle = m_handle;
				if (handle) {
					StringCstr16 s = title;
					SendMessageW(handle, CB_INSERTSTRING, (WPARAM)index, (LPARAM)(s.getData()));
				}
			}

			void removeItem(ComboBox* view, sl_int32 index)
			{
				HWND handle = m_handle;
				if (handle) {
					SendMessageW(handle, CB_DELETESTRING, (WPARAM)index, 0);
				}
			}

			void setItemTitle(ComboBox* view, sl_int32 index, const String& title) override
			{
				HWND handle = m_handle;
				if (handle) {
					StringCstr16 s = title;
					SendMessageW(handle, CB_DELETESTRING, (WPARAM)index, 0);
					SendMessageW(handle, CB_INSERTSTRING, (WPARAM)index, (LPARAM)(s.getData()));
				}
			}

			void selectItem(ComboBox* view, sl_int32 index) override
			{
				HWND handle = m_handle;
				if (handle) {
					SendMessageW(handle, CB_SETCURSEL, (WPARAM)index, 0);
				}
			}

			sl_bool getText(ComboBox* view, String& _out) override
			{
				HWND handle = m_handle;
				if (handle) {
					_out = UIPlatform::getWindowText(handle);
					return sl_true;
				}
				return sl_false;
			}

			void setText(ComboBox* view, const String& text) override
			{
				Win32_ViewInstance::setText(text);
			}

			sl_ui_len measureHeight(ComboBox* view) override
			{
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
				}
				return 0;
			}

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

		};

		SLIB_DEFINE_OBJECT(ComboBoxInstance, Win32_ViewInstance)

	}

	Ref<ViewInstance> ComboBox::createNativeWidget(ViewInstance* parent)
	{
		UINT style = CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_TABSTOP;
		return Win32_ViewInstance::create<ComboBoxInstance>(this, parent, L"COMBOBOX", sl_null, style, 0);
	}

	Ptr<IComboBoxInstance> ComboBox::getComboBoxInstance()
	{
		return CastRef<ComboBoxInstance>(getViewInstance());
	}

}

#endif
