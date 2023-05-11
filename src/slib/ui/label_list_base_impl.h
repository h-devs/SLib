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

#ifndef CHECKHEADER_SLIB_UI_LABEL_LIST_BASE_IMPL
#define CHECKHEADER_SLIB_UI_LABEL_LIST_BASE_IMPL

#include "slib/ui/core.h"

namespace slib
{

	template <class VIEW_CLASS, class INDEX_TYPE>
	Function<String(sl_uint64 index)> LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::getTitleFunction()
	{
		return m_functionTitle;
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::setTitleFunction(const Function<String(sl_uint64 index)>& func, UIUpdateMode mode)
	{
		if (((VIEW_CLASS*)this)->isNativeWidget()) {
			if (!(UI::isUiThread())) {
				UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef((VIEW_CLASS*)this, &VIEW_CLASS::setTitleFunction, func, mode));
				return;
			}
		}
		m_functionTitle = func;
		((VIEW_CLASS*)this)->notifyRefreshItems(mode);
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	List<String> LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::getTitles()
	{
		return m_titles;
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::setTitles(const List<String>& titles, UIUpdateMode mode)
	{
		if (((VIEW_CLASS*)this)->isNativeWidget()) {
			if (!(UI::isUiThread())) {
				UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef((VIEW_CLASS*)this, &VIEW_CLASS::setTitles, titles, mode));
				return;
			}
		}
		m_titles = titles;
		((VIEW_CLASS*)this)->notifyRefreshItems(mode);
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	List<String> LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::getValues()
	{
		return m_values;
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::setValues(const List<String>& values)
	{
		m_values = values;
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::refreshItems(UIUpdateMode mode)
	{
		if (((VIEW_CLASS*)this)->isNativeWidget()) {
			if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
				return;
			}
			if (!(UI::isUiThread())) {
				UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef((VIEW_CLASS*)this, &VIEW_CLASS::notifyRefreshItems, mode));
				return;
			}
		}
		((VIEW_CLASS*)this)->notifyRefreshItems(mode);
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::addItem(const String& value, const String& title, UIUpdateMode mode)
	{
		if (((VIEW_CLASS*)this)->isNativeWidget()) {
			if (!(UI::isUiThread())) {
				UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef((VIEW_CLASS*)this, &VIEW_CLASS::addItem, value, title, mode));
				return;
			}
		}
		ObjectLocker lock(((VIEW_CLASS*)this));
		List<String> titles(m_titles);
		List<String> values(m_values);
		sl_size nCount = (sl_size)(((VIEW_CLASS*)this)->getItemCount());
		if (titles.getCount() != nCount) {
			titles.setCount(nCount);
		}
		if (values.getCount() != nCount) {
			values.setCount(nCount);
		}
		titles.add(title);
		values.add(value);
		m_titles = Move(titles);
		m_values = Move(values);
		((VIEW_CLASS*)this)->notifyInsertItem((INDEX_TYPE)nCount, title, mode);
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::addTitle(const String& title, UIUpdateMode mode)
	{
		addItem(sl_null, title, mode);
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::insertItem(INDEX_TYPE _index, const String& value, const String& title, UIUpdateMode mode)
	{
		if (_index >= 0) {
			if (((VIEW_CLASS*)this)->isNativeWidget()) {
				if (!(UI::isUiThread())) {
					UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef((VIEW_CLASS*)this, &VIEW_CLASS::insertItem, _index, value, title, mode));
					return;
				}
			}
			sl_size index = (sl_size)_index;
			ObjectLocker lock(((VIEW_CLASS*)this));
			sl_size nCount = (sl_size)(((VIEW_CLASS*)this)->getItemCount());
			if (index > nCount) {
				return;
			}
			List<String> titles(m_titles);
			List<String> values(m_values);
			if (titles.getCount() != nCount) {
				titles.setCount(nCount);
			}
			if (values.getCount() != nCount) {
				values.setCount(nCount);
			}
			titles.insert(index, title);
			values.insert(index, value);
			m_titles = Move(titles);
			m_values = Move(values);
			((VIEW_CLASS*)this)->notifyInsertItem((INDEX_TYPE)index, title, mode);
		}
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::insertTitle(INDEX_TYPE index, const String& title, UIUpdateMode mode)
	{
		insertItem(index, sl_null, title, mode);
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::removeItem(INDEX_TYPE _index, UIUpdateMode mode)
	{
		if (_index >= 0) {
			if (((VIEW_CLASS*)this)->isNativeWidget()) {
				if (!(UI::isUiThread())) {
					UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef((VIEW_CLASS*)this, &VIEW_CLASS::removeItem, _index, mode));
					return;
				}
			}
			sl_size index = (sl_size)_index;
			ObjectLocker lock(((VIEW_CLASS*)this));
			sl_size nCount = (sl_size)(((VIEW_CLASS*)this)->getItemCount());
			if (index >= nCount) {
				return;
			}
			List<String>(m_titles).removeAt(index);
			List<String>(m_values).removeAt(index);
			((VIEW_CLASS*)this)->notifyRemoveItem((INDEX_TYPE)index, mode);
		}
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::removeAllItems(UIUpdateMode mode)
	{
		if (((VIEW_CLASS*)this)->isNativeWidget()) {
			if (!(UI::isUiThread())) {
				UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef((VIEW_CLASS*)this, &VIEW_CLASS::removeAllItems, mode));
				return;
			}
		}
		m_titles.setNull();
		m_values.setNull();
		((VIEW_CLASS*)this)->setItemCount(0, mode);
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	String LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::getItemValue(INDEX_TYPE index)
	{
		if (index >= 0) {
			return List<String>(m_values).getValueAt((sl_size)index);
		}
		return sl_null;
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::setItemValue(INDEX_TYPE _index, const String& value)
	{
		if (_index >= 0) {
			ObjectLocker lock(((VIEW_CLASS*)this));
			sl_size index = (sl_size)_index;
			sl_size nCount = (sl_size)(((VIEW_CLASS*)this)->getItemCount());
			if (index >= nCount) {
				return;
			}
			List<String> values(m_values);
			if (index >= values.getCount()) {
				values.setCount(index + 1);
			}
			values.setAt(index, value);
			m_values = Move(values);
		}
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	String LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::getItemTitle(INDEX_TYPE index)
	{
		if (index >= 0) {
			if (m_functionTitle.isNotNull()) {
				return m_functionTitle(index);
			}
			if (m_titles.isNotNull()) {
				return List<String>(m_titles).getValueAt((sl_size)index);
			}
		}
		return sl_null;
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::setItemTitle(INDEX_TYPE _index, const String& title, UIUpdateMode mode)
	{
		if (_index >= 0) {
			if (((VIEW_CLASS*)this)->isNativeWidget()) {
				if (!(UI::isUiThread())) {
					UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef((VIEW_CLASS*)this, &VIEW_CLASS::setItemTitle, _index, title, mode));
					return;
				}
			}
			ObjectLocker lock(((VIEW_CLASS*)this));
			sl_size index = (sl_size)_index;
			sl_size nCount = (sl_size)(((VIEW_CLASS*)this)->getItemCount());
			if (index >= nCount) {
				return;
			}
			List<String> titles(m_titles);
			if (index >= titles.getCount()) {
				titles.setCount(index + 1);
			}
			titles.setAt(index, title);
			m_titles = Move(titles);
			((VIEW_CLASS*)this)->notifySetItemTitle((INDEX_TYPE)index, title, mode);
		}
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	typename SignedType<INDEX_TYPE>::Type LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::findItemByValue(const StringView& value)
	{
		return (typename SignedType<INDEX_TYPE>::Type)(List<String>(m_values).indexOf(value));
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	typename SignedType<INDEX_TYPE>::Type LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::findItemByTitle(const StringView& title)
	{
		return (typename SignedType<INDEX_TYPE>::Type)(List<String>(m_titles).indexOf(title));
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::selectValue(const StringView& value, UIUpdateMode mode)
	{
		sl_reg index = List<String>(m_values).indexOf(value);
		if (index >= 0) {
			((VIEW_CLASS*)this)->selectItem((INDEX_TYPE)index, mode);
		}
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	String LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::getSelectedValue()
	{
		INDEX_TYPE index = ((VIEW_CLASS*)this)->getSelectedIndex();
		if (index >= 0) {
			return List<String>(m_values).getValueAt((sl_size)index);
		}
		return sl_null;
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	String LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::getSelectedTitle()
	{
		INDEX_TYPE index = ((VIEW_CLASS*)this)->getSelectedIndex();
		if (index >= 0) {
			return List<String>(m_titles).getValueAt((sl_size)index);
		}
		return sl_null;
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void LabelListViewBase<VIEW_CLASS, INDEX_TYPE>::sortByTitle(sl_bool flagAsc, UIUpdateMode mode)
	{
		if (((VIEW_CLASS*)this)->isNativeWidget()) {
			if (!(UI::isUiThread())) {
				UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef((VIEW_CLASS*)this, &VIEW_CLASS::sortByTitle, flagAsc, mode));
				return;
			}
		}
		ObjectLocker lock(((VIEW_CLASS*)this));
		ListLocker<String> values(m_values);
		ListLocker<String> titles(m_titles);
		sl_size n = SLIB_MIN(values.count, titles.count);
		if (n < 2) {
			return;
		}
		sl_size n1 = n - 1;
		for (sl_size i = 0; i < n1; i++) {
			sl_size sel = i;
			for (sl_size j = i + 1; j < n; j++) {
				if (flagAsc) {
					if (titles[sel] > titles[j]) {
						sel = j;
					}
				} else {
					if (titles[sel] < titles[j]) {
						sel = j;
					}
				}
			}
			if (sel != i) {
				Swap(titles[sel], titles[i]);
				Swap(values[sel], values[i]);
			}
		}
		((VIEW_CLASS*)this)->notifyRefreshItems(mode);
	}


	template <class INDEX_TYPE>
	LabelListViewCellBase<INDEX_TYPE>::LabelListViewCellBase()
	{
		itemCount = 0;
	}

	template <class INDEX_TYPE>
	template <class VIEW>
	void LabelListViewCellBase<INDEX_TYPE>::initLabelList(VIEW* view)
	{
		itemCount = (INDEX_TYPE)(view->getItemCount());
		WeakRef<VIEW> weak = view;
		titleGetter = [weak](INDEX_TYPE index) {
			Ref<VIEW> view = weak;
			if (view.isNotNull()) {
				return view->getItemTitle(index);
			}
			return String::null();
		};
	}


	template <class VIEW_CLASS, class INDEX_TYPE>
	SingleSelectionViewBase<VIEW_CLASS, INDEX_TYPE>::SingleSelectionViewBase()
	{
		m_countItems = 0;
		m_indexSelected = 0;
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	INDEX_TYPE SingleSelectionViewBase<VIEW_CLASS, INDEX_TYPE>::getItemCount()
	{
		return m_countItems;
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void SingleSelectionViewBase<VIEW_CLASS, INDEX_TYPE>::setItemCount(INDEX_TYPE n, UIUpdateMode  mode)
	{
		if (((VIEW_CLASS*)this)->isNativeWidget()) {
			if (!(UI::isUiThread())) {
				UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef((VIEW_CLASS*)this, &VIEW_CLASS::setItemCount, n, mode));
				return;
			}
		}
		m_countItems = n;
		if (m_indexSelected >= n) {
			selectItem(0, UIUpdateMode::None);
		}
		((VIEW_CLASS*)this)->notifyRefreshItems(mode);
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	INDEX_TYPE SingleSelectionViewBase<VIEW_CLASS, INDEX_TYPE>::getSelectedIndex()
	{
		return m_indexSelected;
	}

	template <class VIEW_CLASS, class INDEX_TYPE>
	void SingleSelectionViewBase<VIEW_CLASS, INDEX_TYPE>::selectItem(INDEX_TYPE index, UIUpdateMode mode)
	{
		if (index >= 0) {
			if (((VIEW_CLASS*)this)->isNativeWidget()) {
				if (!(UI::isUiThread())) {
					UI::dispatchToUiThreadUrgently(Function<void()>::bindWeakRef((VIEW_CLASS*)this, &VIEW_CLASS::selectItem, index, mode));
					return;
			}
				}
			if (index >= m_countItems) {
				return;
			}
			((VIEW_CLASS*)this)->notifySelectItem(index, sl_null, mode);
		}
	}

	template <class INDEX_TYPE>
	SingleSelectionViewCellBase<INDEX_TYPE>::SingleSelectionViewCellBase()
	{
		selectedIndex = 0;
	}

}

#define SLIB_DEFINE_LABEL_LIST_NOTIFY_FUNCTIONS(VIEW_CLASS, INDEX_TYPE) \
	template class LabelListViewBase<VIEW_CLASS, INDEX_TYPE>; \
	void VIEW_CLASS::notifyRefreshItems(UIUpdateMode mode) \
	{ \
		if (m_cell.isNotNull()) { \
			m_cell->itemCount = m_countItems; \
		} \
		invalidate(mode); \
	} \
	void VIEW_CLASS::notifyInsertItem(INDEX_TYPE index, const String& title, UIUpdateMode mode) \
	{ \
		m_countItems++; \
		if (m_cell.isNotNull()) { \
			m_cell->itemCount = m_countItems; \
		} \
		invalidate(mode); \
	} \
	void VIEW_CLASS::notifyRemoveItem(INDEX_TYPE index, UIUpdateMode mode) \
	{ \
		INDEX_TYPE n = m_countItems; \
		if (n <= 0) { \
			return; \
		} \
		m_countItems = n - 1; \
		if (m_cell.isNotNull()) { \
			m_cell->itemCount = m_countItems; \
		} \
		invalidate(mode); \
	} \
	void VIEW_CLASS::notifySetItemTitle(INDEX_TYPE index, const String& title, UIUpdateMode mode) \
	{ \
		invalidate(mode); \
	}

#define SLIB_DEFINE_LABEL_LIST_INSTANCE_NOTIFY_FUNCTIONS(VIEW_CLASS, INDEX_TYPE, INSTANCE_CLASS, INSTANCE_GETTER) \
	template class LabelListViewBase<VIEW_CLASS, INDEX_TYPE>; \
	void VIEW_CLASS::notifyRefreshItems(UIUpdateMode mode) \
	{ \
		if (m_cell.isNotNull()) { \
			m_cell->itemCount = m_countItems; \
		} \
		Ptr<INSTANCE_CLASS> instance = INSTANCE_GETTER(); \
		if (instance.isNotNull()) { \
			instance->refreshItems((VIEW_CLASS*)this); \
		} else { \
			invalidate(mode); \
		} \
	} \
	void VIEW_CLASS::notifyInsertItem(INDEX_TYPE index, const String& title, UIUpdateMode mode) \
	{ \
		m_countItems++; \
		if (m_cell.isNotNull()) { \
			m_cell->itemCount = m_countItems; \
		} \
		Ptr<INSTANCE_CLASS> instance = INSTANCE_GETTER(); \
		if (instance.isNotNull()) { \
			instance->insertItem((VIEW_CLASS*)this, index, title); \
		} else { \
			invalidate(mode); \
		} \
	} \
	void VIEW_CLASS::notifyRemoveItem(INDEX_TYPE index, UIUpdateMode mode) \
	{ \
		INDEX_TYPE n = m_countItems; \
		if (n <= 0) { \
			return; \
		} \
		m_countItems = n - 1; \
		if (m_cell.isNotNull()) { \
			m_cell->itemCount = m_countItems; \
		} \
		Ptr<INSTANCE_CLASS> instance = INSTANCE_GETTER(); \
		if (instance.isNotNull()) { \
			instance->removeItem((VIEW_CLASS*)this, index); \
		} else { \
			invalidate(mode); \
		} \
	} \
	void VIEW_CLASS::notifySetItemTitle(INDEX_TYPE index, const String& title, UIUpdateMode mode) \
	{ \
		Ptr<INSTANCE_CLASS> instance = INSTANCE_GETTER(); \
		if (instance.isNotNull()) { \
			instance->setItemTitle((VIEW_CLASS*)this, index, title); \
		} else { \
			invalidate(mode); \
		} \
	} \
	void INSTANCE_CLASS::insertItem(VIEW_CLASS* view, INDEX_TYPE index, const String& title) \
	{ \
		refreshItems(view); \
	} \
	void INSTANCE_CLASS::removeItem(VIEW_CLASS* view, INDEX_TYPE index) \
	{ \
		refreshItems(view); \
	} \
	void INSTANCE_CLASS::setItemTitle(VIEW_CLASS* view, INDEX_TYPE index, const String& title) \
	{ \
		refreshItems(view); \
	}

#define SLIB_DEFINE_SINGLE_SELECTION_VIEW_NOTIFY_FUNCTIONS(VIEW_CLASS, INDEX_TYPE) \
	SLIB_DEFINE_LABEL_LIST_NOTIFY_FUNCTIONS(VIEW_CLASS, INDEX_TYPE) \
	template class SingleSelectionViewBase<VIEW_CLASS, INDEX_TYPE>; \
	void VIEW_CLASS::notifySelectItem(INDEX_TYPE index, UIEvent* ev, UIUpdateMode mode) \
	{ \
		ObjectLocker locker(this); \
		INDEX_TYPE former = m_indexSelected; \
		if (former == index) { \
			return; \
		} \
		m_indexSelected = index; \
		if (m_cell.isNotNull()) { \
			m_cell->selectedIndex = index; \
		} \
		invalidate(mode); \
		locker.unlock(); \
		invokeSelectItem(index, former, ev); \
	}

#define SLIB_DEFINE_SINGLE_SELECTION_VIEW_INSTANCE_NOTIFY_FUNCTIONS(VIEW_CLASS, INDEX_TYPE, INSTANCE_CLASS, INSTANCE_GETTER) \
	SLIB_DEFINE_LABEL_LIST_INSTANCE_NOTIFY_FUNCTIONS(VIEW_CLASS, INDEX_TYPE, INSTANCE_CLASS, INSTANCE_GETTER) \
	template class SingleSelectionViewBase<VIEW_CLASS, INDEX_TYPE>; \
	void VIEW_CLASS::notifySelectItem(INDEX_TYPE index, UIEvent* ev, UIUpdateMode mode) \
	{ \
		ObjectLocker locker(this); \
		INDEX_TYPE former = m_indexSelected; \
		if (former == index) { \
			return; \
		} \
		m_indexSelected = index; \
		Ptr<INSTANCE_CLASS> instance = INSTANCE_GETTER(); \
		if (instance.isNotNull()) { \
			if (!ev) { \
				instance->selectItem((VIEW_CLASS*)this, index); \
			} \
		} else { \
			if (m_cell.isNotNull()) { \
				m_cell->selectedIndex = index; \
			} \
			invalidate(mode); \
		} \
		locker.unlock(); \
		invokeSelectItem(index, former, ev); \
	}

#endif