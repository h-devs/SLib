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

#include "view.h"

namespace slib
{

	class SLIB_EXPORT ListBox : public View
	{
		SLIB_DECLARE_OBJECT
		
	public:
		ListBox();
		
		~ListBox();

	public:
		sl_uint64 getItemCount();

		virtual void setItemCount(sl_uint64 count, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getItemHeight();

		virtual void setItemHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isMultipleSelection();

		void setMultipleSelection(sl_bool flag, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isSelectedIndex(sl_int64 index);

		sl_int64 getSelectedIndex();

		void setSelectedIndex(sl_int64 index, UIUpdateMode mode = UIUpdateMode::Redraw);

		List<sl_uint64> getSelectedIndices();

		void setSelectedIndices(const ListParam<sl_uint64>& indices, UIUpdateMode mode = UIUpdateMode::Redraw);

		void selectItem(sl_int64 index, UIUpdateMode mode = UIUpdateMode::Redraw);

		void unselectItem(sl_int64 index, UIUpdateMode mode = UIUpdateMode::Redraw);

		void toggleItemSelection(sl_int64 index, UIUpdateMode mode = UIUpdateMode::Redraw);

		void selectItems(const ListParam<sl_uint64>& indices, UIUpdateMode mode = UIUpdateMode::Redraw);

		void unselectItems(const ListParam<sl_uint64>& indices, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setSelectedRange(sl_int64 from, sl_int64 to, UIUpdateMode mode = UIUpdateMode::Redraw);

		void selectRange(sl_int64 from, sl_int64 to, UIUpdateMode mode = UIUpdateMode::Redraw);

		void unselectAll(UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_int64 getHoverIndex();

		sl_int64 getItemIndexAt(const UIPoint& pt);

		Ref<Drawable> getItemBackground();

		void setItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);
	
		void setItemBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getSelectedItemBackground();

		void setSelectedItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setSelectedItemBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getHoverItemBackground();

		void setHoverItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setHoverItemBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getFocusedItemBackground();

		void setFocusedItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setFocusedItemBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

	public:
		SLIB_DECLARE_EVENT_HANDLER(ListBox, DrawItem, sl_uint64 itemIndex, Canvas* canvas, UIRect& rcItem)

		SLIB_DECLARE_EVENT_HANDLER(ListBox, ClickItem, sl_uint64 itemIndex, UIPoint& pos, UIEvent* ev)

		SLIB_DECLARE_EVENT_HANDLER(ListBox, RightButtonClickItem, sl_uint64 itemIndex, UIPoint& pos, UIEvent* ev)

		SLIB_DECLARE_EVENT_HANDLER(ListBox, DoubleClickItem, sl_uint64 itemIndex, UIPoint& pos, UIEvent* ev)

		SLIB_DECLARE_EVENT_HANDLER(ListBox, ChangedSelection, UIEvent* ev)

	protected:
		void onDraw(Canvas* canvas) override;

		void onClickEvent(UIEvent* ev) override;

		void onMouseEvent(UIEvent* ev) override;

		void onKeyEvent(UIEvent* ev) override;

	protected:
		sl_int64 m_countItems;
		sl_ui_len m_heightItem;
		sl_int64 m_indexHover;
		
		sl_bool m_flagMultipleSelection;
		sl_int64 m_indexSelected;
		sl_int64 m_indexFocused;
		sl_int64 m_indexLastSelected;
		CHashMap<sl_uint64, sl_bool> m_mapSelection;

		AtomicRef<Drawable> m_backgroundItem;
		AtomicRef<Drawable> m_backgroundSelectedItem;
		AtomicRef<Drawable> m_backgroundHoverItem;
		AtomicRef<Drawable> m_backgroundFocusedItem;

	};

}

#endif
