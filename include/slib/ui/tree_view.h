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

		sl_bool isVisible();

		void setVisible(sl_bool flagVisible = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isOpened();

		sl_bool isVisibleState();

		void open(UIUpdateMode mode = UIUpdateMode::Redraw);

		void close(UIUpdateMode mode = UIUpdateMode::Redraw);

		void select(UIUpdateMode mode = UIUpdateMode::Redraw);

		String getText();

		void setText(const String& text, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Font> getFont();

		void setFont(const Ref<Font>& font, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setFont(const FontDesc& desc, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getBackground(ViewState state = ViewState::Default);

		void setBackground(const Ref<Drawable>& background, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setBackground(const Ref<Drawable>& background, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setBackgroundColor(const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setBackgroundColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getOpenedIcon(ViewState state = ViewState::Default);

		void setOpenedIcon(const Ref<Drawable>& icon, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setOpenedIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getClosedIcon(ViewState state = ViewState::Default);

		void setClosedIcon(const Ref<Drawable>& icon, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setClosedIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setIcon(const Ref<Drawable>& icon, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getIconWidth();

		void setIconWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getIconHeight();

		void setIconHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setIconSize(sl_ui_len width, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setIconSize(sl_ui_len size, UIUpdateMode mode = UIUpdateMode::Redraw);

		Color getTextColor(ViewState state = ViewState::Default);

		void setTextColor(const Color& color, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setTextColor(const Color& color, UIUpdateMode mode = UIUpdateMode::Redraw);

		String getToolTip();

		void setToolTip(const String& toolTip);

		sl_ui_len getHeight();

		void setHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Cursor> getCursor();

		void setCursor(const Ref<Cursor>& cursor);

	public:
		SLIB_PROPERTY_FUNCTION(void(TreeViewItem* item, TreeViewItem* former, UIEvent* /* nullable */), OnSelect)
		SLIB_PROPERTY_FUNCTION(void(TreeViewItem* item, UIEvent*), OnClick)
		SLIB_PROPERTY_FUNCTION(void(TreeViewItem* item, UIEvent*), OnRightButtonClick)

	private:
		void _addChild(TreeViewItem* item, UIUpdateMode mode);

		void _removeChild(TreeViewItem* item);

		void _setTreeViewHierarchy(TreeView* view, sl_uint32 level);

		void _relayoutTree(UIUpdateMode mode);

		void _relayoutItem(UIUpdateMode mode);

		void _redrawTree(UIUpdateMode mode);

	private:
		sl_bool m_flagVisible : 1;
		sl_bool m_flagOpened : 1;

		AtomicString m_id;
		AtomicWeakRef<TreeView> m_tree;
		AtomicWeakRef<TreeViewItem> m_parent;
		sl_uint32 m_level;
		CList< Ref<TreeViewItem> > m_children;
		ViewStateMap< Ref<Drawable> > m_backgrounds;
		ViewStateMap< Ref<Drawable> > m_closedIcons;
		ViewStateMap< Ref<Drawable> > m_openedIcons;
		sl_ui_len m_iconWidth;
		sl_ui_len m_iconHeight;
		AtomicString m_text;
		AtomicRef<Font> m_font;
		ViewStateMap<Color> m_textColors;
		AtomicString m_toolTip;
		sl_ui_len m_height;
		AtomicRef<Cursor> m_cursor;

		UIRect m_frame;
		sl_ui_pos m_bottomChildren;

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

		void selectItem(const Ref<TreeViewItem>& item, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getOpenedItemIcon(ViewState state = ViewState::Default);

		void setOpenedItemIcon(const Ref<Drawable>& icon, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setOpenedItemIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Drawable> getClosedItemIcon(ViewState state = ViewState::Default);

		void setClosedItemIcon(const Ref<Drawable>& icon, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setClosedItemIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setItemIcon(const Ref<Drawable>& icon, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setItemIcon(const Ref<Drawable>& icon, UIUpdateMode mode = UIUpdateMode::Redraw);

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

		sl_ui_len getItemIconWidth();

		void setItemIconWidth(sl_ui_len width, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getItemIconHeight();

		void setItemIconHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setItemIconSize(sl_ui_len width, sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setItemIconSize(sl_ui_len size, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getItemHeight();

		void setItemHeight(sl_ui_len height, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_pos getItemPadding();

		void setItemPadding(sl_ui_pos padding, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_pos getItemIndent();

		void setItemIndent(sl_ui_pos indent, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_pos getTextIndent();

		void setTextIndent(sl_ui_pos indent, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<Cursor> getItemCursor();

		void setItemCursor(const Ref<Cursor>& cursor);

	public:
		SLIB_DECLARE_EVENT_HANDLER(TreeView, SelectItem, TreeViewItem* item, TreeViewItem* former, UIEvent* ev /* nullable */)
		SLIB_DECLARE_EVENT_HANDLER(TreeView, ClickItem, TreeViewItem* item, UIEvent* ev)
		SLIB_DECLARE_EVENT_HANDLER(TreeView, RightButtonClickItem, TreeViewItem* item, UIEvent* ev)

	public:
		void onDraw(Canvas* canvas) override;

		void onResize(sl_ui_len width, sl_ui_len height) override;

	protected:
		void onChangePadding(UIUpdateMode mode) override;

		void onUpdateFont(const Ref<Font>& font) override;

	private:
		void _createRootItem();

		void _createContentView();

		void _relayoutContent(UIUpdateMode mode);

		void _redrawContent(UIUpdateMode mode);

		void _drawContent(Canvas* canvas);

		void _makeLayoutContent();

		void _makeLayoutItem(TreeViewItem* item, sl_ui_pos& top, sl_ui_pos left, sl_ui_pos right, sl_ui_len defaultTextHeight, sl_bool flagRoot);

		sl_ui_len _getItemHeight(TreeViewItem* item, sl_ui_len& textHeight);

		ViewState _getItemState(TreeViewItem* item);

		void _drawItem(Canvas* canvas, TreeViewItem* item, const Ref<Font>& parentFont, sl_bool flagRoot);

		void _processMouseEvent(UIEvent* ev);

		void _processMouseEventItem(UIEvent* ev, sl_bool flagClick, TreeViewItem* item, sl_bool flagRoot);

		void _selectItem(TreeViewItem* item, UIEvent* ev, UIUpdateMode mode);

		void _clickItem(TreeViewItem* item, UIEvent* ev);

		void _rightButtonClickItem(TreeViewItem* item, UIEvent* ev);

	private:
		class ContentView;
		Ref<ContentView> m_content;

		sl_bool m_flagInvalidTreeLayout;

		Ref<TreeViewItem> m_root;

		ViewStateMap< Ref<Drawable> > m_openedItemIcons;
		ViewStateMap< Ref<Drawable> > m_closedItemIcons;
		AtomicRef<Drawable> m_iconCollapsed;
		AtomicRef<Drawable> m_iconExpanded;
		sl_ui_len m_itemIconWidth;
		sl_ui_len m_itemIconHeight;

		ViewStateMap< Ref<Drawable> > m_itemBackgrounds;
		ViewStateMap<Color> m_itemTextColors;

		sl_ui_len m_itemHeight;
		sl_ui_pos m_itemPadding;
		sl_ui_pos m_itemIndent;
		sl_ui_pos m_textIndent;
		AtomicRef<Cursor> m_itemCursor;

		AtomicRef<TreeViewItem> m_itemHover;
		AtomicRef<TreeViewItem> m_itemSelected;

		sl_bool m_flagBeginTapping;
		UIPointF m_pointBeginTapping;

		friend class TreeViewItem;

	};

}

#endif
