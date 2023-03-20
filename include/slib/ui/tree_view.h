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

#ifndef CHECKHEADER_SLIB_UI_TREE_VIEW
#define CHECKHEADER_SLIB_UI_TREE_VIEW

#include "scroll_view.h"
#include "view_state_map.h"

#include "../core/string.h"
#include "../core/function.h"

namespace slib
{

	class TreeView;

	class SLIB_EXPORT TreeViewItem : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		TreeViewItem();

		~TreeViewItem();

	public:
		String getId();

		void setId(const String& _id);

		Ref<TreeView> getTreeView();

		Ref<TreeViewItem> getParent();

		sl_uint32 getLevel();

		Ref<TreeViewItem> getItemById(const String& _id);

		List< Ref<TreeViewItem> > getChildren();

		sl_size getChildCount();

		Ref<TreeViewItem> getChild(sl_size index);

		void addChild(const Ref<TreeViewItem>& item, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<TreeViewItem> addChild(const String& text, const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<TreeViewItem> addChild(const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);

		void insertChild(sl_size index, const Ref<TreeViewItem>& item, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<TreeViewItem> insertChild(sl_size index, const String& text, const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<TreeViewItem> insertChild(sl_size index, const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);

		void removeChild(sl_size index, UIUpdateMode mode = UIUpdateMode::Redraw);

		void removeChild(const Ref<TreeViewItem>& item, UIUpdateMode mode = UIUpdateMode::Redraw);

		void removeAllChildren(UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isRoot();

		sl_bool isLeaf();

		sl_bool isOpened();

		sl_bool isVisible();

		void open(UIUpdateMode mode = UIUpdateMode::Redraw);

		void close(UIUpdateMode mode = UIUpdateMode::Redraw);

		String getText();

		void setText(const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getIcon();

		void setIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getOpenedIcon();

		void setOpenedIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getClosedIcon();

		void setClosedIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<View> getCustomView();

		void setCustomView(const Ref<View>& view, UIUpdateMode mode = UIUpdateMode::Redraw);

		Color getTextColor(ViewState state = ViewState::Default);

		void setTextColor(const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTextColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getHeight();

		void setHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

	public:
		SLIB_PROPERTY_FUNCTION(void(TreeViewItem*), OnSelect)

	private:
		void _addChild(TreeViewItem* item, UIUpdateMode mode);

		void _removeChild(TreeViewItem* item);

		void _setTreeViewHierarchy(TreeView* view, sl_uint32 level);

		void _relayoutTree(UIUpdateMode mode);

		void _redrawTree(UIUpdateMode mode);

	private:
		AtomicString m_id;
		AtomicWeakRef<TreeView> m_tree;
		AtomicWeakRef<TreeViewItem> m_parent;
		sl_uint32 m_level;
		CList< Ref<TreeViewItem> > m_children;
		sl_bool m_flagOpened;
		AtomicRef<Drawable> m_icon;
		AtomicRef<Drawable> m_iconOpened;
		AtomicRef<Drawable> m_iconClosed;
		AtomicString m_text;
		AtomicRef<View> m_customView;
		ViewStateMap<Color> m_textColors;
		sl_ui_len m_height;

		UIRect m_frame;
		sl_ui_pos m_bottomChildren;
		AtomicRef<Drawable> m_iconDrawing;

		friend class TreeView;
	};


	class SLIB_EXPORT TreeView : public ScrollView
	{
		SLIB_DECLARE_OBJECT

	public:
		TreeView();

		~TreeView();

	protected:
		void init() override;

	public:
		Ref<TreeViewItem> getRootItem();

		Ref<TreeViewItem> getItemById(const String& _id);

		List< Ref<TreeViewItem> > getItems();

		sl_size getItemCount();

		Ref<TreeViewItem> getItem(sl_size index);

		void addItem(const Ref<TreeViewItem>& item, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<TreeViewItem> addItem(const String& text, const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<TreeViewItem> addItem(const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);

		void insertItem(sl_size index, const Ref<TreeViewItem>& item, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<TreeViewItem> insertItem(sl_size index, const String& text, const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<TreeViewItem> insertItem(sl_size index, const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);

		void removeItem(sl_size index, UIUpdateMode mode = UIUpdateMode::Redraw);

		void removeItem(const Ref<TreeViewItem>& item, UIUpdateMode mode = UIUpdateMode::Redraw);

		void removeAllItems(UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<TreeViewItem> getSelectedItem();

		Ref<Drawable> getItemIcon();

		void setItemIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getOpenedItemIcon();

		void setOpenedItemIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getClosedItemIcon();

		void setClosedItemIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getCollapsedIcon();

		void setCollapsedIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getExpandedIcon();

		void setExpandedIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getItemBackground(ViewState state = ViewState::Default);

		void setItemBackground(const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setItemBackgroundColor(const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setItemBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Color getItemTextColor(ViewState state);

		void setItemTextColor(const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setItemTextColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getItemHeight();

		void setItemHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_pos getItemPadding();

		void setItemPadding(sl_ui_pos padding, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_pos getItemIndent();

		void setItemIndent(sl_ui_pos indent, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_pos getTextIndent();

		void setTextIndent(sl_ui_pos indent, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setFont(const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::UpdateLayout) override;

	public:
		SLIB_DECLARE_EVENT_HANDLER(TreeView, SelectItem, TreeViewItem* item)

	protected:
		void onResize(sl_ui_len width, sl_ui_len height) override;

		void onChangePadding(UIUpdateMode mode) override;

	private:
		void _createRootItem();

		void _createContentView();

		void _relayoutContent(UIUpdateMode mode);

		void _redrawContent(UIUpdateMode mode);

		void _drawContent(Canvas* canvas);

		void _makeLayoutContent();

		void _makeLayoutItem(TreeViewItem* item, sl_ui_pos& top, sl_ui_pos left, sl_ui_pos right, sl_bool flagRoot);

		ViewState _getItemState(TreeViewItem* item);

		void _drawItem(Canvas* canvas, TreeViewItem* item, sl_bool flagRoot);

		void _processMouseEvent(UIEvent* ev);

		void _processMouseEventItem(UIEvent* ev, sl_bool flagClick, TreeViewItem* item, sl_bool flagRoot);

		void _processClickItem(TreeViewItem* item);

	private:
		class ContentView;
		Ref<ContentView> m_content;

		sl_bool m_flagInvalidTreeLayout;

		Ref<TreeViewItem> m_root;

		AtomicRef<Drawable> m_itemIcon;
		AtomicRef<Drawable> m_itemIconOpened;
		AtomicRef<Drawable> m_itemIconClosed;
		AtomicRef<Drawable> m_iconCollapsed;
		AtomicRef<Drawable> m_iconExpanded;

		ViewStateMap< Ref<Drawable> > m_itemBackgrounds;
		ViewStateMap<Color> m_itemTextColors;

		sl_ui_len m_itemHeight;
		sl_ui_pos m_itemPadding;
		sl_ui_pos m_itemIndent;
		sl_ui_pos m_textIndent;

		sl_ui_len m_layoutTextHeight;

		AtomicRef<TreeViewItem> m_itemHover;
		AtomicRef<TreeViewItem> m_itemSelected;

		sl_bool m_flagBeginTapping;
		UIPointf m_pointBeginTapping;

		friend class TreeViewItem;

	};

}

#endif
