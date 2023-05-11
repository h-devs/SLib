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

#include "slib/ui/tree_view.h"

#include "slib/ui/priv/view_state_map.h"
#include "slib/graphics/canvas.h"

namespace slib
{

	class TreeView::ContentView : public View
	{
	public:
		AtomicWeakRef<TreeView> m_tree;

	public:
		ContentView()
		{
#if !defined(SLIB_PLATFORM_IS_MOBILE)
			setFocusable(sl_true);
#endif
		}

	public:
		void onDraw(Canvas* canvas) override
		{
			Ref<TreeView> tree = m_tree;
			if (tree.isNotNull()) {
				tree->_drawContent(canvas);
			}
		}

		void onMouseEvent(UIEvent* ev) override
		{
			Ref<TreeView> tree = m_tree;
			if (tree.isNotNull()) {
				tree->_processMouseEvent(ev);
			}
		}

	};

	SLIB_DEFINE_OBJECT(TreeViewItem, Object)

	TreeViewItem::TreeViewItem()
	{
		m_level = 0;
		m_flagOpened = sl_false;
		m_height = 0;

		m_frame.setZero();
		m_iconWidth = 0;
		m_iconHeight = 0;

		m_bottomChildren = 0;
	}

	TreeViewItem::~TreeViewItem()
	{
	}

	String TreeViewItem::getId()
	{
		return m_id;
	}

	void TreeViewItem::setId(const String& _id)
	{
		m_id = _id;
	}

	Ref<TreeView> TreeViewItem::getTreeView()
	{
		return m_tree;
	}

	Ref<TreeViewItem> TreeViewItem::getParent()
	{
		return m_parent;
	}

	sl_uint32 TreeViewItem::getLevel()
	{
		return m_level;
	}

	Ref<TreeViewItem> TreeViewItem::getItemById(const String& _id)
	{
		if (String(m_id) == _id) {
			return this;
		}
		ListLocker< Ref<TreeViewItem> > children(m_children);
		for (sl_size i = 0; i < children.count; i++) {
			Ref<TreeViewItem> item = children[i];
			if (item.isNotNull()) {
				item = item->getItemById(_id);
				if (item.isNotNull()) {
					return item;
				}
			}
		}
		return sl_null;
	}

	List< Ref<TreeViewItem> > TreeViewItem::getChildren()
	{
		return m_children.duplicate();
	}

	sl_size TreeViewItem::getChildCount()
	{
		return m_children.getCount();
	}

	Ref<TreeViewItem> TreeViewItem::getChild(sl_size index)
	{
		return m_children.getValueAt(index);
	}

	void TreeViewItem::addChild(const Ref<TreeViewItem>& item, UIUpdateMode mode)
	{
		if (item.isNull()) {
			return;
		}
		if (m_children.add(item)) {
			_addChild(item.get(), mode);
		}
	}

	Ref<TreeViewItem> TreeViewItem::addChild(const String& text, const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		Ref<TreeViewItem> item = new TreeViewItem;
		if (item.isNotNull()) {
			item->setText(text);
			item->setIcon(icon);
			addChild(item, mode);
			return item;
		}
		return sl_null;
	}

	Ref<TreeViewItem> TreeViewItem::addChild(const String& text, UIUpdateMode mode)
	{
		return addChild(text, Ref<Drawable>::null(), mode);
	}

	void TreeViewItem::insertChild(sl_size index, const Ref<TreeViewItem>& item, UIUpdateMode mode)
	{
		if (item.isNull()) {
			return;
		}
		if (m_children.insert(index, item)) {
			_addChild(item.get(), mode);
		}
	}

	Ref<TreeViewItem> TreeViewItem::insertChild(sl_size index, const String& text, const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		Ref<TreeViewItem> item = new TreeViewItem;
		if (item.isNotNull()) {
			item->setText(text);
			item->setIcon(icon);
			insertChild(index, item, mode);
			return item;
		}
		return sl_null;
	}

	Ref<TreeViewItem> TreeViewItem::insertChild(sl_size index, const String& text, UIUpdateMode mode)
	{
		return insertChild(index, text, Ref<Drawable>::null(), mode);
	}

	void TreeViewItem::removeChild(sl_size index, UIUpdateMode mode)
	{
		Ref<TreeViewItem> item = m_children.getValueAt(index);
		if (item.isNull()) {
			return;
		}
		_removeChild(item.get());
		m_children.removeAt(index);
		if (isVisible()) {
			_relayoutTree(mode);
		}
	}

	void TreeViewItem::removeChild(const Ref<TreeViewItem>& item, UIUpdateMode mode)
	{
		if (item.isNull()) {
			return;
		}
		_removeChild(item.get());
		m_children.remove(item);
		if (isVisible()) {
			_relayoutTree(mode);
		}
	}

	void TreeViewItem::removeAllChildren(UIUpdateMode mode)
	{
		ListLocker< Ref<TreeViewItem> > children(m_children);
		for (sl_size i = 0; i < children.count; i++) {
			_removeChild(children[i].get());
		}
		m_children.removeAll();
		if (isVisible()) {
			_relayoutTree(mode);
		}
	}

	sl_bool TreeViewItem::isRoot()
	{
		return m_parent.isNull();
	}

	sl_bool TreeViewItem::isLeaf()
	{
		return m_children.isEmpty();
	}

	sl_bool TreeViewItem::isOpened()
	{
		return m_flagOpened;
	}

	sl_bool TreeViewItem::isVisible()
	{
		Ref<TreeView> tree = m_tree;
		if (tree.isNull()) {
			return sl_false;
		}
		Ref<TreeViewItem> parent = m_parent;
		if (parent.isNull()) {
			return sl_true;
		}
		if (parent->m_flagOpened) {
			return parent->isVisible();
		} else {
			return sl_false;
		}
	}

	void TreeViewItem::open(UIUpdateMode mode)
	{
		m_flagOpened = sl_true;
		Ref<TreeViewItem> parent = m_parent;
		while (parent.isNotNull()) {
			parent->m_flagOpened = sl_true;
			parent = parent->m_parent;
		}
		if (m_children.isNotEmpty()) {
			_relayoutTree(mode);
		}
	}

	void TreeViewItem::close(UIUpdateMode mode)
	{
		m_flagOpened = sl_false;
		if (m_children.isNotEmpty()) {
			_relayoutTree(mode);
		}
	}

	void TreeViewItem::select(UIUpdateMode mode)
	{
		Ref<TreeView> tree = m_tree;
		if (tree.isNotNull()) {
			tree->selectItem(this, mode);
		}
	}

	String TreeViewItem::getText()
	{
		return m_text;
	}

	void TreeViewItem::setText(const String& text, UIUpdateMode mode)
	{
		m_text = text;
		_redrawTree(mode);
	}

	Ref<Font> TreeViewItem::getFont()
	{
		if (m_font.isNotNull()) {
			return m_font;
		}
		Ref<TreeViewItem> parent = m_parent;
		if (parent.isNotNull()) {
			return parent->getFont();
		}
		Ref<TreeView> view = m_tree;
		if (view.isNotNull()) {
			return view->getFont();
		}
		return sl_null;
	}

	void TreeViewItem::setFont(const Ref<Font>& font, UIUpdateMode mode)
	{
		m_font = font;
		_relayoutItem(mode);
	}

	void TreeViewItem::setFont(const FontDesc& desc, UIUpdateMode mode)
	{
		m_font = Font::create(desc, getFont());
		_relayoutItem(mode);
	}

	Ref<Drawable> TreeViewItem::getBackground(ViewState state)
	{
		return m_backgrounds.get(state);
	}

	void TreeViewItem::setBackground(const Ref<Drawable>& background, ViewState state, UIUpdateMode mode)
	{
		m_backgrounds.set(state, background);
		_redrawTree(mode);
	}

	void TreeViewItem::setBackground(const Ref<Drawable>& background, UIUpdateMode mode)
	{
		setBackground(background, ViewState::Default, mode);
	}

	void TreeViewItem::setBackgroundColor(const Color& color, ViewState state, UIUpdateMode mode)
	{
		setBackground(Drawable::fromColor(color), state, mode);
	}

	void TreeViewItem::setBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setBackground(Drawable::fromColor(color), mode);
	}

	Ref<Drawable> TreeViewItem::getOpenedIcon(ViewState state)
	{
		return m_openedIcons.get(state);
	}

	void TreeViewItem::setOpenedIcon(const Ref<Drawable>& icon, ViewState state, UIUpdateMode mode)
	{
		m_openedIcons.set(state, icon);
		if (m_iconHeight) {
			_redrawTree(mode);
		} else {
			_relayoutItem(mode);
		}
	}

	void TreeViewItem::setOpenedIcon(const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		setOpenedIcon(icon, ViewState::Default, mode);
	}

	Ref<Drawable> TreeViewItem::getClosedIcon(ViewState state)
	{
		return m_closedIcons.get(state);
	}

	void TreeViewItem::setClosedIcon(const Ref<Drawable>& icon, ViewState state, UIUpdateMode mode)
	{
		m_closedIcons.set(state, icon);
		if (m_iconHeight) {
			_redrawTree(mode);
		} else {
			_relayoutItem(mode);
		}
	}

	void TreeViewItem::setClosedIcon(const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		setClosedIcon(icon, ViewState::Default, mode);
	}

	void TreeViewItem::setIcon(const Ref<Drawable>& icon, ViewState state, UIUpdateMode mode)
	{
		m_openedIcons.set(state, icon);
		m_closedIcons.set(state, icon);
		if (m_iconHeight) {
			_redrawTree(mode);
		} else {
			_relayoutItem(mode);
		}
	}

	void TreeViewItem::setIcon(const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		setIcon(icon, ViewState::Default, mode);
	}

	sl_ui_len TreeViewItem::getIconWidth()
	{
		return m_iconWidth;
	}

	void TreeViewItem::setIconWidth(sl_ui_len width, UIUpdateMode mode)
	{
		if (m_iconWidth != width) {
			m_iconWidth = width;
			_redrawTree(mode);
		}
	}

	sl_ui_len TreeViewItem::getIconHeight()
	{
		return m_iconHeight;
	}

	void TreeViewItem::setIconHeight(sl_ui_len height, UIUpdateMode mode)
	{
		if (m_iconHeight != height) {
			m_iconHeight = height;
			_relayoutItem(mode);
		}
	}

	void TreeViewItem::setIconSize(sl_ui_len width, sl_ui_len height, UIUpdateMode mode)
	{
		if (m_iconWidth != width && m_iconHeight != height) {
			m_iconWidth = width;
			m_iconHeight = height;
			_relayoutItem(mode);
		}
	}

	void TreeViewItem::setIconSize(sl_ui_len size, UIUpdateMode mode)
	{
		if (m_iconWidth != size && m_iconHeight != size) {
			m_iconWidth = size;
			m_iconHeight = size;
			_relayoutItem(mode);
		}
	}

	Color TreeViewItem::getTextColor(ViewState state)
	{
		return m_textColors.get(state);
	}

	void TreeViewItem::setTextColor(const Color& color, ViewState state, UIUpdateMode mode)
	{
		m_textColors.set(state, color);
		_redrawTree(mode);
	}

	void TreeViewItem::setTextColor(const Color& color, UIUpdateMode mode)
	{
		m_textColors.defaultValue = color;
		_redrawTree(mode);
	}

	sl_ui_len TreeViewItem::getHeight()
	{
		return m_height;
	}

	void TreeViewItem::setHeight(sl_ui_len height, UIUpdateMode mode)
	{
		if (m_height != height) {
			m_height = height;
			_relayoutTree(mode);
		}
	}

	void TreeViewItem::_addChild(TreeViewItem* item, UIUpdateMode mode)
	{
		item->m_parent = this;
		Ref<TreeView> tree = m_tree;
		if (tree.isNotNull()) {
			item->_setTreeViewHierarchy(tree.get(), m_level + 1);
		}
		if (isVisible()) {
			_relayoutTree(mode);
		}
	}

	void TreeViewItem::_removeChild(TreeViewItem* item)
	{
		item->m_parent.setNull();
		item->m_tree.setNull();
	}

	void TreeViewItem::_setTreeViewHierarchy(TreeView* view, sl_uint32 level)
	{
		m_level = level;
		m_tree = view;
		ListLocker< Ref<TreeViewItem> > children(m_children);
		for (sl_size i = 0; i < children.count; i++) {
			const Ref<TreeViewItem>& child = children[i];
			if (child.isNotNull()) {
				child->_setTreeViewHierarchy(view, level + 1);
			}
		}
	}

	void TreeViewItem::_relayoutTree(UIUpdateMode mode)
	{
		Ref<TreeView> tree = m_tree;
		if (tree.isNotNull()) {
			tree->_relayoutContent(mode);
		}
	}

	void TreeViewItem::_relayoutItem(UIUpdateMode mode)
	{
		Ref<TreeView> tree = m_tree;
		if (tree.isNotNull()) {
			if (m_height || tree->m_itemHeight) {
				tree->_redrawContent(mode);
			} else {
				tree->_relayoutContent(mode);
			}
		}
	}

	void TreeViewItem::_redrawTree(UIUpdateMode mode)
	{
		Ref<TreeView> tree = m_tree;
		if (tree.isNotNull()) {
			tree->_redrawContent(mode);
		}
	}


	namespace {

		class DefaultIndentIcon : public Drawable
		{
		public:
			Ref<Brush> m_brush;
			Point m_pts[3];

		public:
			DefaultIndentIcon(const Color& color, sl_bool flagCollapse)
			{
				m_brush = Brush::createSolidBrush(color);
				if (flagCollapse) {
					m_pts[0] = Point(0.33f, 0.34f);
					m_pts[1] = Point(0.67f, 0.51f);
					m_pts[2] = Point(0.33f, 0.68f);
				} else {
					m_pts[0] = Point(0.3f, 0.35f);
					m_pts[1] = Point(0.5f, 0.65f);
					m_pts[2] = Point(0.7f, 0.35f);
				}
			}

		public:
			sl_real getDrawableWidth() override
			{
				return 16;
			}

			sl_real getDrawableHeight() override
			{
				return 16;
			}

			void onDrawAll(Canvas* canvas, const Rectangle& rectDst, const DrawParam& param) override
			{
				if (m_brush.isNotNull()) {
					Point pts[3];
					for (int i = 0; i < 3; i++) {
						pts[i].x = rectDst.left + rectDst.getWidth() * m_pts[i].x;
						pts[i].y = rectDst.top + rectDst.getHeight() * m_pts[i].y;
					}
					canvas->fillPolygon(pts, 3, m_brush);
				}
			}

		};

	}

	SLIB_DEFINE_OBJECT(TreeView, ScrollView)

	TreeView::TreeView()
	{
		setCreatingInstance(sl_false);
		setCreatingNativeWidget(sl_false);
		setUsingFont(sl_true);
		setSavingCanvasState(sl_false);

		m_flagInvalidTreeLayout = sl_true;

		m_itemBackgrounds.set(ViewState::Selected, Drawable::fromColor(Color(0, 0, 0, 50)));
		m_itemTextColors.defaultValue = Color::Black;
		m_itemTextColors.set(ViewState::Hover, Color(0, 0, 200));
		m_itemTextColors.set(ViewState::Selected, Color(0, 0, 200));

		m_itemHeight = 0;

		setAntiAlias(sl_true, UIUpdateMode::Init);
		ScrollView::setPadding(6, 6, 6, 6, UIUpdateMode::Init);
		m_itemPadding = 8;
		m_itemIndent = 16;
		m_textIndent = 4;

		m_itemIconWidth = 0;
		m_itemIconHeight = 0;

		m_iconCollapsed = new DefaultIndentIcon(Color(50, 50, 50), sl_true);
		m_iconExpanded = new DefaultIndentIcon(Color(50, 50, 50), sl_false);
		
	}

	TreeView::~TreeView()
	{
	}

	void TreeView::init()
	{
		ScrollView::init();

		_createRootItem();
		_createContentView();
	}

	Ref<TreeViewItem> TreeView::getRootItem()
	{
		return m_root;
	}

	Ref<TreeViewItem> TreeView::getItemById(const String& _id)
	{
		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			return root->getItemById(_id);
		}
		return sl_null;
	}

	List< Ref<TreeViewItem> > TreeView::getItems()
	{
		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			return root->getChildren();
		}
		return sl_null;
	}

	sl_size TreeView::getItemCount()
	{
		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			return root->getChildCount();
		}
		return 0;
	}

	Ref<TreeViewItem> TreeView::getItem(sl_size index)
	{
		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			return root->getChild(index);
		}
		return sl_null;
	}

	void TreeView::addItem(const Ref<TreeViewItem>& item, UIUpdateMode mode)
	{
		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			root->addChild(item, mode);
		}
	}

	Ref<TreeViewItem> TreeView::addItem(const String& text, const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			return root->addChild(text, icon, mode);
		}
		return sl_null;
	}

	Ref<TreeViewItem> TreeView::addItem(const String& text, UIUpdateMode mode)
	{
		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			return root->addChild(text, mode);
		}
		return sl_null;
	}

	void TreeView::insertItem(sl_size index, const Ref<TreeViewItem>& item, UIUpdateMode mode)
	{
		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			root->insertChild(index, item, mode);
		}
	}

	Ref<TreeViewItem> TreeView::insertItem(sl_size index, const String& text, const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			return root->insertChild(index, text, icon, mode);
		}
		return sl_null;
	}

	Ref<TreeViewItem> TreeView::insertItem(sl_size index, const String& text, UIUpdateMode mode)
	{
		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			return root->insertChild(index, text, mode);
		}
		return sl_null;
	}

	void TreeView::removeItem(sl_size index, UIUpdateMode mode)
	{
		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			root->removeChild(index, mode);
		}
	}

	void TreeView::removeItem(const Ref<TreeViewItem>& item, UIUpdateMode mode)
	{
		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			root->removeChild(item, mode);
		}
	}

	void TreeView::removeAllItems(UIUpdateMode mode)
	{
		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			root->removeAllChildren(mode);
		}
	}

	Ref<TreeViewItem> TreeView::getSelectedItem()
	{
		return m_itemSelected;
	}

	void TreeView::selectItem(const Ref<TreeViewItem>& item, UIUpdateMode mode)
	{
		_selectItem(item, sl_null, UIUpdateMode::None);
		item->open(UIUpdateMode::Redraw);
	}

	Ref<Drawable> TreeView::getOpenedItemIcon(ViewState state)
	{
		return m_openedItemIcons.get(state);
	}

	void TreeView::setOpenedItemIcon(const Ref<Drawable>& icon, ViewState state, UIUpdateMode mode)
	{
		m_openedItemIcons.set(state, icon);
		_relayoutContent(mode);
	}

	void TreeView::setOpenedItemIcon(const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		setOpenedItemIcon(icon, ViewState::Default, mode);
	}

	Ref<Drawable> TreeView::getClosedItemIcon(ViewState state)
	{
		return m_closedItemIcons.get(state);
	}

	void TreeView::setClosedItemIcon(const Ref<Drawable>& icon, ViewState state, UIUpdateMode mode)
	{
		m_closedItemIcons.set(state, icon);
		_relayoutContent(mode);
	}

	void TreeView::setClosedItemIcon(const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		setClosedItemIcon(icon, ViewState::Default, mode);
	}

	void TreeView::setItemIcon(const Ref<Drawable>& icon, ViewState state, UIUpdateMode mode)
	{
		m_openedItemIcons.set(state, icon);
		m_closedItemIcons.set(state, icon);
		_relayoutContent(mode);
	}

	void TreeView::setItemIcon(const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		setItemIcon(icon, ViewState::Default, mode);
	}

	Ref<Drawable> TreeView::getCollapsedIcon()
	{
		return m_iconCollapsed;
	}

	void TreeView::setCollapsedIcon(const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		Color c;
		if (ColorDrawable::check(icon, &c)) {
			m_iconCollapsed = new DefaultIndentIcon(c, sl_true);
		} else {
			m_iconCollapsed = icon;
		}
		_relayoutContent(mode);
	}

	Ref<Drawable> TreeView::getExpandedIcon()
	{
		return m_iconExpanded;
	}

	void TreeView::setExpandedIcon(const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		Color c;
		if (ColorDrawable::check(icon, &c)) {
			m_iconExpanded = new DefaultIndentIcon(c, sl_false);
		} else {
			m_iconExpanded = icon;
		}
		_relayoutContent(mode);
	}

	Ref<Drawable> TreeView::getItemBackground(ViewState state)
	{
		return m_itemBackgrounds.get(state);
	}

	void TreeView::setItemBackground(const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode)
	{
		m_itemBackgrounds.set(state, drawable);
		_redrawContent(mode);
	}

	void TreeView::setItemBackground(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		m_itemBackgrounds.defaultValue = drawable;
		_redrawContent(mode);
	}

	void TreeView::setItemBackgroundColor(const Color& color, ViewState state, UIUpdateMode mode)
	{
		setItemBackground(Drawable::fromColor(color), state, mode);
	}

	void TreeView::setItemBackgroundColor(const Color& color, UIUpdateMode mode)
	{
		setItemBackground(Drawable::fromColor(color), mode);
	}

	Color TreeView::getItemTextColor(ViewState state)
	{
		return m_itemTextColors.get(state);
	}

	void TreeView::setItemTextColor(const Color& color, ViewState state, UIUpdateMode mode)
	{
		m_itemTextColors.set(state, color);
		_redrawContent(mode);
	}

	void TreeView::setItemTextColor(const Color& color, UIUpdateMode mode)
	{
		m_itemTextColors.defaultValue = color;
		_redrawContent(mode);
	}

	sl_ui_len TreeView::getItemIconWidth()
	{
		return m_itemIconWidth;
	}

	void TreeView::setItemIconWidth(sl_ui_len width, UIUpdateMode mode)
	{
		m_itemIconWidth = width;
		_relayoutContent(mode);
	}

	sl_ui_len TreeView::getItemIconHeight()
	{
		return m_itemIconHeight;
	}

	void TreeView::setItemIconHeight(sl_ui_len height, UIUpdateMode mode)
	{
		m_itemIconHeight = height;
		_relayoutContent(mode);
	}

	void TreeView::setItemIconSize(sl_ui_len width, sl_ui_len height, UIUpdateMode mode)
	{
		m_itemIconWidth = width;
		m_itemIconHeight = height;
		_relayoutContent(mode);
	}

	void TreeView::setItemIconSize(sl_ui_len size, UIUpdateMode mode)
	{
		m_itemIconWidth = size;
		m_itemIconHeight = size;
		_relayoutContent(mode);
	}

	sl_ui_len TreeView::getItemHeight()
	{
		return m_itemHeight;
	}

	void TreeView::setItemHeight(sl_ui_len height, UIUpdateMode mode)
	{
		m_itemHeight = height;
		_relayoutContent(mode);
	}

	sl_ui_pos TreeView::getItemPadding()
	{
		return m_itemPadding;
	}

	void TreeView::setItemPadding(sl_ui_pos padding, UIUpdateMode mode)
	{
		m_itemPadding = padding;
		_relayoutContent(mode);
	}

	sl_ui_pos TreeView::getItemIndent()
	{
		return m_itemIndent;
	}

	void TreeView::setItemIndent(sl_ui_pos indent, UIUpdateMode mode)
	{
		m_itemIndent = indent;
		_relayoutContent(mode);
	}

	sl_ui_pos TreeView::getTextIndent()
	{
		return m_textIndent;
	}

	void TreeView::setTextIndent(sl_ui_pos indent, UIUpdateMode mode)
	{
		m_textIndent = indent;
		_redrawContent(mode);
	}

	void TreeView::onDraw(Canvas* canvas)
	{
		if (m_flagInvalidTreeLayout) {
			dispatchToDrawingThread(SLIB_FUNCTION_WEAKREF(this, _makeLayoutContent));
		}
		ScrollView::onDraw(canvas);
	}

	void TreeView::onResize(sl_ui_len width, sl_ui_len height)
	{
		ScrollView::onResize(width, height);
		Ref<ContentView> content = m_content;
		if (content.isNotNull()) {
			content->setWidth(width, UIUpdateMode::Redraw);
			if (m_flagInvalidTreeLayout) {
				dispatchToDrawingThread(SLIB_FUNCTION_WEAKREF(this, _makeLayoutContent));
			}
		}
	}

	void TreeView::onChangePadding(UIUpdateMode mode)
	{
		ScrollView::onChangePadding(mode);
		mode = SLIB_UI_UPDATE_MODE_IS_INIT(mode) ? UIUpdateMode::Init : UIUpdateMode::None;
		_relayoutContent(mode);
	}

	void TreeView::onUpdateFont(const Ref<Font>& font)
	{
		ScrollView::onUpdateFont(font);
		_relayoutContent(UIUpdateMode::Redraw);
	}

	SLIB_DEFINE_EVENT_HANDLER(TreeView, SelectItem, (TreeViewItem* item, TreeViewItem* former, UIEvent* ev), item, former, ev)

	SLIB_DEFINE_EVENT_HANDLER_WITHOUT_ON(TreeView, ClickItem, (TreeViewItem* item, UIEvent* ev), item, ev)

	void TreeView::onClickItem(TreeViewItem* item, UIEvent* ev)
	{
		_selectItem(ToRef(&item), ev, UIUpdateMode::None);
	}

	void TreeView::_createRootItem()
	{
		Ref<TreeViewItem> item = new TreeViewItem;
		if (item.isNotNull()) {
			item->m_tree = this;
			item->m_flagOpened = sl_true;
			m_root = item;
		}
	}

	void TreeView::_createContentView()
	{
		Ref<ContentView> view = new ContentView;
		if (view.isNotNull()) {
			view->m_tree = this;
			m_content = view;
			setContentView(view);
		}
	}

	void TreeView::_relayoutContent(UIUpdateMode mode)
	{
		Ref<ContentView> content = m_content;
		if (content.isNotNull()) {
			m_flagInvalidTreeLayout = sl_true;
			if (SLIB_UI_UPDATE_MODE_IS_REDRAW(mode)) {
				dispatchToDrawingThread(SLIB_FUNCTION_WEAKREF(this, _makeLayoutContent));
			}
		}
	}

	void TreeView::_redrawContent(UIUpdateMode mode)
	{
		Ref<ContentView> view = m_content;
		if (view.isNotNull()) {
			view->invalidate(mode);
		}
	}

	void TreeView::_drawContent(Canvas* canvas)
	{
		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			_drawItem(canvas, root.get(), getFont(), sl_true);
		}
	}

	void TreeView::_makeLayoutContent()
	{
		if (!m_flagInvalidTreeLayout) {
			return;
		}
		m_flagInvalidTreeLayout = sl_false;

		Ref<Font> font = getFont();
		sl_ui_pos fontHeight = 0;
		if (font.isNotNull()) {
			fontHeight = (sl_ui_pos)(font->getFontHeight());
		}

		Ref<TreeViewItem> root = m_root;
		if (root.isNotNull()) {
			sl_ui_pos top = getPaddingTop();
			sl_ui_pos left = getPaddingLeft();
			sl_ui_pos right = getWidth() - getPaddingRight();
			if (m_iconCollapsed.isNotNull() || m_iconExpanded.isNotNull()) {
				left += m_itemIndent;
			}
			_makeLayoutItem(root.get(), top, left, right, fontHeight, sl_true);
			top += getPaddingBottom();
			if (top < 0) {
				top = 0;
			}
			Ref<ContentView> content = m_content;
			if (content.isNotNull()) {
				if (content->getHeight() != top) {
					content->setHeight(top, UIUpdateMode::Redraw);
				}
			}
		}

	}

	void TreeView::_makeLayoutItem(TreeViewItem* item, sl_ui_pos& top, sl_ui_pos left, sl_ui_pos right, sl_ui_len defaultTextHeight, sl_bool flagRoot)
	{
		sl_ui_len textHeight = defaultTextHeight;
		if (!flagRoot) {
			item->m_frame.left = left;
			item->m_frame.right = right;
			item->m_frame.top = top;
			item->m_frame.bottom = top + _getItemHeight(item, textHeight);
			top = item->m_frame.bottom;
			left += m_itemIndent;
		}
		if (item->m_flagOpened) {
			ListLocker< Ref<TreeViewItem> > children(item->m_children);
			for (sl_size i = 0; i < children.count; i++) {
				const Ref<TreeViewItem>& child = children[i];
				if (child.isNotNull()) {
					_makeLayoutItem(child.get(), top, left, right, textHeight, sl_false);
				}
			}
		}
		item->m_bottomChildren = top;
	}

	sl_ui_len TreeView::_getItemHeight(TreeViewItem* item, sl_ui_len& textHeight)
	{
		if (item->m_font.isNotNull()) {
			Ref<Font> font = item->m_font;
			if (font.isNotNull()) {
				textHeight = (sl_ui_pos)(font->getFontHeight());
			}
		}
		sl_ui_len height = item->m_height;
		if (height) {
			return height;
		}
		height = m_itemHeight;
		if (height) {
			return height;
		}
		sl_ui_len iconHeight = item->m_iconHeight;
		if (!iconHeight) {
			iconHeight = m_itemIconHeight;
			if (!iconHeight) {
				Ref<Drawable> iconDraw;
				sl_bool flagOpened = item->m_children.getCount() ? item->m_flagOpened : sl_true;
				if (flagOpened) {
					if (item->m_openedIcons.defaultValue.isNotNull()) {
						iconDraw = item->m_openedIcons.defaultValue;
					} else {
						iconDraw = m_openedItemIcons.defaultValue;
					}
				} else {
					if (item->m_closedIcons.defaultValue.isNotNull()) {
						iconDraw = item->m_closedIcons.defaultValue;
					} else {
						iconDraw = m_closedItemIcons.defaultValue;
					}
				}
				if (iconDraw.isNotNull()) {
					iconHeight = (sl_ui_pos)(iconDraw->getDrawableHeight());
				}
			}
		}
		return Math::max(iconHeight, textHeight);
	}

	ViewState TreeView::_getItemState(TreeViewItem* item)
	{
		ViewState state;
		if (item == m_itemHover) {
			if (isPressedState()) {
				state = ViewState::Pressed;
			} else {
				state = ViewState::Hover;
			}
		} else {
			state = ViewState::Normal;
		}
		if (item == m_itemSelected) {
			return (ViewState)((int)(state)+(int)(ViewState::Selected));
		} else {
			return state;
		}
	}

	void TreeView::_drawItem(Canvas* canvas, TreeViewItem* item, const Ref<Font>& parentFont, sl_bool flagRoot)
	{
		Ref<Font> font = item->m_font;
		if (font.isNull()) {
			font = parentFont;
		}
		if (!flagRoot) {
			sl_ui_pos left = item->m_frame.left;
			sl_ui_pos right = item->m_frame.right;
			sl_ui_pos top = item->m_frame.top;
			sl_ui_pos bottom = item->m_frame.bottom;
			ViewState state = _getItemState(item);
			Ref<Drawable> background = item->m_backgrounds.evaluate(state);
			if (background.isNull()) {
				background = m_itemBackgrounds.evaluate(state);
			}
			if (background.isNotNull()) {
				canvas->draw(UIRect(0, top, getWidth(), bottom), background);
			}
			if (item->m_children.getCount() > 0) {
				if (item->m_flagOpened) {
					Ref<Drawable> icon = m_iconExpanded;
					if (icon.isNotNull()) {
						canvas->draw(UIRect(left - m_itemIndent, top, left, bottom), icon, ScaleMode::None, Alignment::MiddleCenter);
					}
				} else {
					Ref<Drawable> icon = m_iconCollapsed;
					if (icon.isNotNull()) {
						canvas->draw(UIRect(left - m_itemIndent, top, left, bottom), icon, ScaleMode::None, Alignment::MiddleCenter);
					}
				}
			}
			sl_bool flagOpened = item->m_children.getCount() ? item->m_flagOpened : sl_true;
			Ref<Drawable> icon;
			if (flagOpened) {
				icon = item->m_openedIcons.evaluate(state);
				if (icon.isNull()) {
					icon = m_openedItemIcons.evaluate(state);
				}
			} else {
				icon = item->m_closedIcons.evaluate(state);
				if (icon.isNull()) {
					icon = m_closedItemIcons.evaluate(state);
				}
			}
			sl_ui_len iconWidth = item->m_iconWidth;
			if (!iconWidth) {
				iconWidth = m_itemIconWidth;
				if (!iconWidth) {
					if (icon.isNotNull()) {
						iconWidth = (sl_ui_pos)(icon->getDrawableWidth());
					}
				}
			}
			sl_ui_len iconHeight = item->m_iconHeight;
			if (!iconHeight) {
				iconHeight = m_itemIconHeight;
				if (!iconHeight) {
					if (icon.isNotNull()) {
						iconHeight = (sl_ui_pos)(icon->getDrawableHeight());
					}
				}
			}
			if (icon.isNotNull()) {
				sl_ui_pos iconTop = (top + bottom - iconHeight) / 2;
				canvas->draw(UIRect(left, iconTop, left + iconWidth, iconTop + iconHeight), icon);
				left += iconWidth;
				left += m_textIndent;
			}
			String text = item->m_text;
			if (text.isNotEmpty()) {
				Color colorText = item->m_textColors.evaluate(state);
				if (colorText.isZero()) {
					colorText = m_itemTextColors.evaluate(state);
				}
				canvas->drawText(text, UIRect(left, top, right, bottom), font, colorText, Alignment::MiddleLeft);
			}
		}
		if (item->m_flagOpened) {
			ListLocker< Ref<TreeViewItem> > children(item->m_children);
			for (sl_size i = 0; i < children.count; i++) {
				const Ref<TreeViewItem>& child = children[i];
				if (child.isNotNull()) {
					_drawItem(canvas, child.get(), font, sl_false);
				}
			}
		}
	}

	void TreeView::_processMouseEvent(UIEvent* ev)
	{
		UIAction action = ev->getAction();
		if (action == UIAction::MouseLeave) {
			m_itemHover.setNull();
			_redrawContent(UIUpdateMode::Redraw);
			return;
		}
		if (action == UIAction::LeftButtonDown || action == UIAction::TouchBegin) {
			Ref<ContentView> content = m_content;
			if (content.isNotNull()) {
				m_pointBeginTapping = content->convertCoordinateToParent(ev->getPoint());
				m_flagBeginTapping = sl_true;
			}
		} else if (action == UIAction::LeftButtonUp || action == UIAction::TouchEnd) {
			if (m_flagBeginTapping) {
				Ref<ContentView> content = m_content;
				if (content.isNotNull()) {
					if (content->convertCoordinateToParent(ev->getPoint()).getLength2p(m_pointBeginTapping) < 25) {
						Ref<TreeViewItem> root = m_root;
						if (root.isNotNull()) {
							_processMouseEventItem(ev, sl_true, root.get(), sl_true);
						}
					}
				}
			}
		} else if (action == UIAction::MouseMove) {
			if (m_flagBeginTapping) {
				Ref<ContentView> content = m_content;
				if (content.isNotNull()) {
					if (content->convertCoordinateToParent(ev->getPoint()).getLength2p(m_pointBeginTapping) > 25) {
						m_flagBeginTapping = sl_false;
					}
				}
			}
			Ref<TreeViewItem> root = m_root;
			if (root.isNotNull()) {
				_processMouseEventItem(ev, sl_false, root.get(), sl_true);
			}
		}
	}

	void TreeView::_processMouseEventItem(UIEvent* ev, sl_bool flagClick, const Ref<TreeViewItem>& item, sl_bool flagRoot)
	{
		sl_ui_pos y = (sl_ui_pos)(ev->getY());
		UIAction action = ev->getAction();
		if (!flagRoot) {
			if (item->m_frame.top <= y && y < item->m_frame.bottom) {
				if (flagClick) {
					if (item->isOpened()) {
						item->close();
					} else {
						item->open();
					}
					_clickItem(item, ev);
					_redrawContent(UIUpdateMode::Redraw);
				} else {
					if (action == UIAction::MouseMove) {
						if (m_itemHover != item) {
							m_itemHover = item;
							_redrawContent(UIUpdateMode::Redraw);
						}
					}
				}
				return;
			}
		}
		if (item->m_flagOpened) {
			ListLocker< Ref<TreeViewItem> > children(item->m_children);
			for (sl_size i = 0; i < children.count; i++) {
				const Ref<TreeViewItem>& child = children[i];
				if (child.isNotNull()) {
					if (child->m_frame.top <= y && y < child->m_bottomChildren) {
						_processMouseEventItem(ev, flagClick, child.get(), sl_false);
						return;
					}
				}
			}
		}
	}

	void TreeView::_selectItem(const Ref<TreeViewItem>& item, UIEvent* ev, UIUpdateMode mode)
	{
		ObjectLocker locker(this);
		Ref<TreeViewItem> former = m_itemSelected;
		if (former == item) {
			return;
		}
		m_itemSelected = item;
		invalidate(mode);
		locker.unlock();
		invokeSelectItem(item.get(), former.get(), ev);
		(item->getOnSelect())(item.get(), former.get(), ev);
	}

	void TreeView::_clickItem(const Ref<TreeViewItem>& item, UIEvent* ev)
	{
		invokeClickItem(item.get(), ev);
		(item->getOnClick())(item.get(), ev);
	}

}
