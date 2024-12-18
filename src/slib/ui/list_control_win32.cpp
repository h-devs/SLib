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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_WIN32)

#include "slib/ui/list_control.h"

#include "view_win32.h"

#include <commctrl.h>

namespace slib
{

	namespace {

		static int TranslateAlignment(Alignment _align)
		{
			Alignment align = _align & Alignment::HorizontalMask;
			if (align == Alignment::Left) {
				return LVCFMT_LEFT;
			} else if (align == Alignment::Right) {
				return LVCFMT_RIGHT;
			} else {
				return LVCFMT_CENTER;
			}
		}

		class ListControlHelper : public ListControl
		{
		public:
			static sl_uint32 getColumnCountFromListView(HWND hWnd)
			{
				HWND hWndHeader = (HWND)(SendMessageW(hWnd, LVM_GETHEADER, 0, 0));
				if (hWndHeader) {
					return (sl_uint32)(SendMessageW(hWndHeader, HDM_GETITEMCOUNT, 0, 0));
				}
				return 0;
			}

			void applyColumnCount(HWND hWnd)
			{
				ObjectLocker lock(this);
				ListElements<Column> columns(m_columns);
				sl_uint32 nNew = (sl_uint32)(columns.count);
				sl_uint32 nOrig = getColumnCountFromListView(hWnd);
				if (nOrig == nNew) {
					return;
				}
				if (nOrig > nNew) {
					for (sl_uint32 i = nOrig; i > nNew; i--) {
						SendMessageW(hWnd, LVM_DELETECOLUMN, (WPARAM)(i - 1), 0);
					}
				} else {
					LVCOLUMNW lvc;
					Base::zeroMemory(&lvc, sizeof(lvc));
					lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;
					for (sl_uint32 i = nOrig; i < nNew; i++) {
						Column& column = columns[i];
						StringCstr16 title(column.title);
						lvc.pszText = (LPWSTR)(title.getData());
						int width = (int)(column.width);
						if (width < 0) {
							width = 0;
						}
						lvc.cx = width;
						lvc.fmt = TranslateAlignment(column.align);
						lvc.iSubItem = i;
						SendMessageW(hWnd, LVM_INSERTCOLUMNW, (WPARAM)i, (LPARAM)&lvc);
					}
				}
			}

			void applyRowCount(HWND hWnd)
			{
				sl_uint32 nNew = getRowCount();
				SendMessageW(hWnd, LVM_SETITEMCOUNT, (WPARAM)nNew, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
			}

			using ListControl::_onSelectRow_NW;
			using ListControl::_onClickRow_NW;
			using ListControl::_onRightButtonClickRow_NW;
			using ListControl::_onDoubleClickRow_NW;
			using ListControl::_onClickHeader_NW;
		};

		class ListControlInstance : public PlatformViewInstance, public IListControlInstance
		{
			SLIB_DECLARE_OBJECT

		public:
			void initialize(View* _view) override
			{
				ListControlHelper* view = (ListControlHelper*)_view;
				HWND handle = getHandle();

				UINT exStyle = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_ONECLICKACTIVATE | LVS_EX_DOUBLEBUFFER;
				SendMessageW(handle, LVM_SETEXTENDEDLISTVIEWSTYLE, exStyle, exStyle);
				view->applyColumnCount(handle);
				view->applyRowCount(handle);
			}

			void refreshColumnCount(ListControl* view) override
			{
				HWND handle = m_handle;
				if (handle) {
					(static_cast<ListControlHelper*>(view))->applyColumnCount(handle);
				}
			}

			void refreshRowCount(ListControl* view) override
			{
				HWND handle = m_handle;
				if (handle) {
					(static_cast<ListControlHelper*>(view))->applyRowCount(handle);
					InvalidateRect(handle, NULL, TRUE);
				}
			}

			void setHeaderText(ListControl* view, sl_uint32 iCol, const String& _text) override
			{
				HWND handle = m_handle;
				if (handle) {
					LVCOLUMNW lvc;
					Base::zeroMemory(&lvc, sizeof(lvc));
					lvc.mask = LVCF_TEXT;
					StringCstr16 text(_text);
					lvc.pszText = (LPWSTR)(text.getData());
					SendMessageW(handle, LVM_SETCOLUMNW, (WPARAM)iCol, (LPARAM)(&lvc));
				}
			}

			void setColumnWidth(ListControl* view, sl_uint32 iCol, sl_ui_len width) override
			{
				HWND handle = m_handle;
				if (handle) {
					if (width < 0) {
						width = 0;
					}
					SendMessageW(handle, LVM_SETCOLUMNWIDTH, (WPARAM)iCol, (LPARAM)(width));
				}
			}

			void setHeaderAlignment(ListControl* view, sl_uint32 iCol, const Alignment& align) override
			{
			}

			void setColumnAlignment(ListControl* view, sl_uint32 iCol, const Alignment& align) override
			{
				HWND handle = m_handle;
				if (handle) {
					LVCOLUMNW lvc;
					Base::zeroMemory(&lvc, sizeof(lvc));
					lvc.mask = LVCF_FMT;
					lvc.fmt = TranslateAlignment(align);
					SendMessageW(handle, LVM_SETCOLUMNW, (WPARAM)iCol, (LPARAM)(&lvc));
				}
			}

			sl_bool getSelectedRow(ListControl* view, sl_int32& _out) override
			{
				HWND handle = m_handle;
				if (handle) {
					_out = (sl_int32)(::SendMessageW(handle, LVM_GETNEXTITEM, (WPARAM)(-1), LVNI_SELECTED));
					return sl_true;
				}
				return sl_false;
			}

			sl_bool processNotify(NMHDR* nmhdr, LRESULT& result) override
			{
				Ref<ListControlHelper> helper = CastRef<ListControlHelper>(getView());
				if (helper.isNotNull()) {
					UINT code = nmhdr->code;
					if (code == LVN_GETDISPINFOW) {
						NMLVDISPINFOW* disp = (NMLVDISPINFOW*)nmhdr;
						String16 s = String16::from(helper->getItemText(disp->item.iItem, disp->item.iSubItem));
						sl_uint32 n = (sl_uint32)(s.getLength());
						if (n > 0) {
							sl_uint32 m = (sl_uint32)(disp->item.cchTextMax);
							if (m > 0) {
								if (n >= m) {
									n = m - 1;
								}
								Base::copyMemory(disp->item.pszText, s.getData(), n * 2);
								(disp->item.pszText)[n] = 0;
							}
						}
						return sl_true;
					} else if (code == LVN_ITEMCHANGED) {
						NMLISTVIEW* v = (NMLISTVIEW*)nmhdr;
						if (v->hdr.hwndFrom == getHandle()) {
							if (!(v->uOldState & LVIS_SELECTED) && (v->uNewState & LVIS_SELECTED)) {
								helper->_onSelectRow_NW(this, v->iItem);
							}
						}
						return sl_true;
					} else if (code == NM_CLICK || code == NM_DBLCLK || code == NM_RCLICK) {
						NMITEMACTIVATE* nm = (NMITEMACTIVATE*)nmhdr;
						LVHITTESTINFO lvhi;
						Base::zeroMemory(&lvhi, sizeof(lvhi));
						lvhi.pt.x = (LONG)(nm->ptAction.x);
						lvhi.pt.y = (LONG)(nm->ptAction.y);
						UIPoint pt((sl_ui_pos)(nm->ptAction.x), (sl_ui_pos)(nm->ptAction.y));
						sl_int32 n = (sl_int32)(::SendMessageW(getHandle(), LVM_HITTEST, 0, (LPARAM)(&lvhi)));
						if (n >= 0) {
							if (code == NM_CLICK) {
								helper->_onClickRow_NW(n, pt);
							} else if (code == NM_RCLICK) {
								helper->_onRightButtonClickRow_NW(n, pt);
							} else if (code == NM_DBLCLK) {
								helper->_onDoubleClickRow_NW(n, pt);
							}
						}
						return sl_true;
					} else if (code == LVN_COLUMNCLICK) {
						NMLISTVIEW* nm = (NMLISTVIEW*)nmhdr;
						helper->_onClickHeader_NW((sl_uint32)(nm->iSubItem), UIPoint((sl_ui_pos)(nm->ptAction.x), (sl_ui_pos)(nm->ptAction.y)));
					}
				}
				return sl_false;
			}
		};

		SLIB_DEFINE_OBJECT(ListControlInstance, PlatformViewInstance)

	}

	Ref<ViewInstance> ListControl::createNativeWidget(ViewInstance* parent)
	{
		DWORD style = LVS_REPORT | LVS_SINGLESEL | LVS_OWNERDATA | WS_TABSTOP | WS_BORDER;
		return PlatformViewInstance::create<ListControlInstance>(this, parent, L"SysListView32", sl_null, style, 0);
	}

	Ptr<IListControlInstance> ListControl::getListControlInstance()
	{
		return CastRef<ListControlInstance>(getViewInstance());
	}

}

#endif
