/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_LIST_VIEW
#define CHECKHEADER_SLIB_UI_LIST_VIEW

#include "scroll_view.h"
#include "adapter.h"

namespace slib
{

	class SLIB_EXPORT ListView : public VerticalScrollView
	{
		SLIB_DECLARE_OBJECT

	public:
		ListView();

		~ListView();

	protected:
		void init() override;

	public:
		class RefreshParam
		{
		public:
			sl_bool flagScrollToLastItem;

		public:
			RefreshParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(RefreshParam)
		};

		void refreshItems(const RefreshParam& param);

		void refreshItems();

		Ref<ViewAdapter> getAdapter();

		class SLIB_EXPORT SetAdapterParam : public RefreshParam
		{
		public:
			Ref<ViewAdapter> adapter;

		public:
			SetAdapterParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SetAdapterParam)
		};

		void setAdapter(const SetAdapterParam& param);

		void setAdapter(const Ref<ViewAdapter>& adapter);

	public:
		void onScroll(ScrollEvent*) override;

		void onResize(sl_ui_len x, sl_ui_len y) override;

	protected:
		void _checkUpdateContent();

		void _initStatus();

		void _initHeights(ViewAdapter* adapter);

		Ref<View> _getView(ViewAdapter* adapter, sl_uint64 index, View* original);

		enum class LayoutCaller
		{
			None = 0,
			Draw = 1,
			Scroll = 2,
			Resize = 3
		};
		void _layoutItemViews(LayoutCaller caller, sl_bool flagRefresh);

		sl_ui_len _updateItemLayout(const Ref<View>& itemView, sl_ui_len widthList, sl_ui_len heightList);

	protected:
		AtomicRef<ViewAdapter> m_adapter;
		Ref<ViewAdapter> m_adapterCurrent;

		class ContentView;
		Ref<ContentView> m_contentView;
		sl_bool m_flagResetAdapter;
		sl_bool m_flagResetingAdapter;
		sl_bool m_flagRefreshItems;
		sl_bool m_flagScrollToLastItem;
		sl_bool m_flagSmoothScrollToLastItem;

		sl_uint64 m_countTotalItems;
		sl_uint64 m_indexFirstItem;
		sl_ui_pos m_yFirstItem;
		Ref<View>* m_viewsVisibleItems;
		Ref<View>* m_viewsGoDownItems;
		Ref<View>* m_viewsGoUpItems;
		Ref<View>* m_viewsFreeItems;
		sl_ui_len* m_heightsVisibleItems;
		sl_ui_len* m_heightsTopItems;
		sl_ui_len* m_heightsBottomItems;
		sl_ui_len* m_heightsGoDownItems;
		sl_ui_len* m_heightsGoUpItems;
		sl_ui_len m_averageItemHeight;
		double m_averageMidItemHeight;
		sl_ui_len m_heightTotalItems;
		sl_ui_pos m_lastScrollY;

		volatile sl_int32 m_lockCountLayouting;

	};

}

#endif
