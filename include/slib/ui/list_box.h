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

#ifndef CHECKHEADER_SLIB_UI_LIST_BOX
#define CHECKHEADER_SLIB_UI_LIST_BOX

#include "definition.h"

#include "view.h"

namespace slib
{

	class SLIB_EXPORT ListBox : public View
	{
		SLIB_DECLARE_OBJECT
		
	public:
		ListBox();
		
		~ListBox();

	protected:
		void init() override;

	public:
		sl_uint64 getItemsCount();

		virtual void setItemsCount(sl_uint64 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getItemHeight();

		virtual void setItemHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_int64 getSelectedIndex();

		virtual void selectItem(sl_int64 index, UIUpdateMode mode = UIUpdateMode::Redraw);

		void unselectItem(UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_int64 getHoverIndex();

		sl_int64 getItemIndexAt(const UIPoint& pt);

	public:
		SLIB_DECLARE_EVENT_HANDLER(ListBox, DrawItem, sl_uint64 itemIndex, Canvas* canvas, UIRect& rcItem)

		SLIB_DECLARE_EVENT_HANDLER(ListBox, ClickItem, sl_uint64 itemIndex, UIPoint& pos, UIEvent* ev)

		SLIB_DECLARE_EVENT_HANDLER(ListBox, RightButtonClickItem, sl_uint64 itemIndex, UIPoint& pos, UIEvent* ev)

		SLIB_DECLARE_EVENT_HANDLER(ListBox, DoubleClickItem, sl_uint64 itemIndex, UIPoint& pos, UIEvent* ev)

	protected:
		void onDraw(Canvas* canvas) override;

		void onClickEvent(UIEvent* ev) override;

		void onMouseEvent(UIEvent* ev) override;

	protected:
		sl_int64 m_countItems;
		sl_ui_len m_heightItem;
		sl_int64 m_indexSelected;
		sl_int64 m_indexHover;

	};

}

#endif
