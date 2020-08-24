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

#ifndef CHECKHEADER_SLIB_UI_LABEL_LIST
#define CHECKHEADER_SLIB_UI_LABEL_LIST

#include "definition.h"

#include "list_box.h"
#include "label_view.h"
#include "core.h"

namespace slib
{

	template <class VIEW_CLASS, class INDEX_TYPE>
	class LabelListViewBase
	{
	public:
		Function<String(sl_uint64 index)> getTitleFunction();

		void setTitleFunction(const Function<String(sl_uint64 index)>& func, UIUpdateMode mode = UIUpdateMode::Redraw);

		List<String> getTitles();

		void setTitles(const List<String>& titles, UIUpdateMode mode = UIUpdateMode::Redraw);

		List<String> getValues();

		void setValues(const List<String>& values);

		void addItem(const String& value, const String& title, UIUpdateMode mode = UIUpdateMode::Redraw);

		void addTitle(const String& title, UIUpdateMode mode = UIUpdateMode::Redraw);

		void insertItem(INDEX_TYPE index, const String& value, const String& title, UIUpdateMode mode = UIUpdateMode::Redraw);

		void insertTitle(INDEX_TYPE index, const String& title, UIUpdateMode mode = UIUpdateMode::Redraw);

		void removeItem(INDEX_TYPE index, UIUpdateMode mode = UIUpdateMode::Redraw);

		void removeAllItems(UIUpdateMode mode = UIUpdateMode::Redraw);

		String getItemValue(INDEX_TYPE index);

		void setItemValue(INDEX_TYPE index, const String& value);

		String getItemTitle(INDEX_TYPE index);

		void setItemTitle(INDEX_TYPE _index, const String& title, UIUpdateMode mode = UIUpdateMode::Redraw);

		void selectValue(const String& value, UIUpdateMode mode = UIUpdateMode::Redraw);

		String getSelectedValue();

		String getSelectedTitle();

	protected:
		AtomicFunction<String(sl_uint64 index)> m_functionTitle;
		AtomicList<String> m_titles;
		AtomicList<String> m_values;

	};

	template <class INDEX_TYPE>
	class LabelListViewCellBase : public ViewCell
	{
	public:
		LabelListViewCellBase();

	public:
		template <class VIEW>
		void initLabelList(VIEW* view);

	public:
		INDEX_TYPE itemsCount;
		Function<String(INDEX_TYPE index)> titleGetter;

	};

#define SLIB_DECLARE_LABEL_LIST_NOTIFY_FUNCTIONS(VIEW_CLASS, INDEX_TYPE) \
protected: \
	virtual void notifyRefreshItems(UIUpdateMode mode); \
	virtual void notifyInsertItem(INDEX_TYPE index, const String& title, UIUpdateMode mode); \
	virtual void notifyRemoveItem(INDEX_TYPE index, UIUpdateMode mode); \
	virtual void notifySetItemTitle(INDEX_TYPE index, const String& title, UIUpdateMode mode); \
	friend class LabelListViewBase<VIEW_CLASS, INDEX_TYPE>;

#define SLIB_DECLARE_LABEL_LIST_INSTANCE_NOTIFY_FUNCTIONS(VIEW_CLASS, INDEX_TYPE) \
public: \
	virtual void refreshItems(VIEW_CLASS* view) = 0; \
	virtual void insertItem(VIEW_CLASS* view, INDEX_TYPE index, const String& title); \
	virtual void removeItem(VIEW_CLASS* view, INDEX_TYPE index); \
	virtual void setItemTitle(VIEW_CLASS* view, INDEX_TYPE index, const String& title);


	template <class VIEW_CLASS, class INDEX_TYPE>
	class SingleSelectionViewBase : public LabelListViewBase<VIEW_CLASS, INDEX_TYPE>
	{
	public:
		SingleSelectionViewBase();

	public:
		INDEX_TYPE getItemsCount();

		void setItemsCount(INDEX_TYPE n, UIUpdateMode  mode = UIUpdateMode::Redraw);

		INDEX_TYPE getSelectedIndex();

		void selectItem(INDEX_TYPE index, UIUpdateMode mode = UIUpdateMode::Redraw);
		
	protected:
		INDEX_TYPE m_countItems;
		INDEX_TYPE m_indexSelected;

	};

	template <class INDEX_TYPE>
	class SingleSelectionViewCellBase : public LabelListViewCellBase<INDEX_TYPE>
	{
	public:
		SingleSelectionViewCellBase();

	public:
		INDEX_TYPE selectedIndex;

	};

#define SLIB_DECLARE_SINGLE_SELECTION_VIEW_NOTIFY_FUNCTIONS(VIEW_CLASS, INDEX_TYPE) \
	SLIB_DECLARE_LABEL_LIST_NOTIFY_FUNCTIONS(VIEW_CLASS, INDEX_TYPE) \
	virtual void notifySelectItem(INDEX_TYPE index, UIUpdateMode mode); \
	friend class SingleSelectionViewBase<VIEW_CLASS, INDEX_TYPE>;

#define SLIB_DECLARE_SINGLE_SELECTION_VIEW_INSTANCE_NOTIFY_FUNCTIONS(VIEW_CLASS, INDEX_TYPE) \
	SLIB_DECLARE_LABEL_LIST_INSTANCE_NOTIFY_FUNCTIONS(VIEW_CLASS, INDEX_TYPE) \
	virtual void selectItem(VIEW_CLASS* view, INDEX_TYPE index) = 0;

	class SLIB_EXPORT LabelList : public ListBox, public LabelAppearanceViewBase<LabelList>, public LabelListViewBase<LabelList, sl_int64>
	{
		SLIB_DECLARE_OBJECT
		
	public:
		LabelList();
		
		~LabelList();
		
	protected:
		void init() override;

	public:
		void setItemHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw) override;

		float getLineHeightWeight();

		void setLineHeightWeight(float weight, UIUpdateMode mode = UIUpdateMode::Redraw);

		Color getSelectedTextColor();

		void setSelectedTextColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Color getHoverTextColor();

		void setHoverTextColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Color getFocusedTextColor();

		void setFocusedTextColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		void invalidateLabelAppearance(UIUpdateMode mode);

	public:
		void dispatchDrawItem(sl_uint64 itemIndex, Canvas* canvas, UIRect& rcItem) override;

	protected:
		void onUpdateFont(const Ref<Font>& font) override;

		void onDraw(Canvas* canvas) override;

	public:
		SLIB_DECLARE_LABEL_LIST_NOTIFY_FUNCTIONS(LabelList, sl_int64)
	
	protected:
		sl_ui_len _getFontHeight();

	protected:
		sl_bool m_flagUseFontHeight;
		sl_ui_len m_heightFont;
		float m_lineHeightWeight;
		
		Color m_textColorSelected;
		Color m_textColorHover;
		Color m_textColorFocused;

	};

}

#endif
