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

#include "slib/core/definition.h"

#if defined(SLIB_UI_IS_GTK)

#include "slib/ui/menu.h"

#include "slib/ui/platform.h"
#include "slib/ui/window.h"
#include "slib/core/safe_static.h"

#include "view_gtk.h"

namespace slib
{

	namespace priv
	{
		namespace menu
		{

			class MenuImpl;

			typedef CHashMap< GtkWidget*, WeakRef<MenuImpl> > MenuMap;
			SLIB_SAFE_STATIC_GETTER(MenuMap, GetMenuMap)

			class MenuItemImpl : public MenuItem
			{
				SLIB_DECLARE_OBJECT
			public:
				GtkWidget* m_menuItem;

			public:
				MenuItemImpl()
				{
					m_menuItem = NULL;
				}

				~MenuItemImpl()
				{
				}

			public:
				static Ref<MenuItemImpl> create(MenuImpl* parent, sl_uint32 index, const MenuItemParam& param);

				static String makeText(const String& title, const KeycodeAndModifiers& shortcutKey, const KeycodeAndModifiers& secondShortcutKey);

				static void onSelected(GtkMenuItem *menuitem, gpointer user_data);

				void _updateText();

				void setText(const String& text) override
				{
					MenuItem::setText(text);
					_updateText();
				}

				void setShortcutKey(const KeycodeAndModifiers& km) override
				{
					MenuItem::setShortcutKey(km);
					_updateText();
				}

				void setSecondShortcutKey(const KeycodeAndModifiers& km) override
				{
					MenuItem::setSecondShortcutKey(km);
					_updateText();
				}

				void _updateState();

				void setEnabled(sl_bool flag) override
				{
					MenuItem::setEnabled(flag);
					_updateState();
				}

				void setChecked(sl_bool flag) override
				{
					MenuItem::setChecked(flag);
					_updateState();
				}

				void setIcon(const Ref<Drawable>& icon) override;

				void setCheckedIcon(const Ref<Drawable>& icon) override;

				void setSubmenu(const Ref<Menu>& menu) override;

			};

			SLIB_DEFINE_OBJECT(MenuItemImpl, MenuItem)

			class MenuImpl : public Menu
			{
				SLIB_DECLARE_OBJECT

			public:
				GtkWidget* m_menu;

			public:
				MenuImpl()
				{
					m_menu = NULL;
				}

				~MenuImpl()
				{
					GtkWidget* _menu = m_menu;
					if (_menu) {
						MenuMap* map = GetMenuMap();
						if (map) {
							map->remove(m_menu);
						}
						m_menu = sl_null;
					}
				}

			public:
				static Ref<MenuImpl> create(sl_bool flagPopup)
				{
					GtkWidget* _menu = NULL;
					if (flagPopup) {
						_menu = gtk_menu_new();
					} else {
						_menu = gtk_menu_bar_new();
						gtk_menu_bar_set_child_pack_direction((GtkMenuBar*)_menu, GTK_PACK_DIRECTION_LTR);
					}
					if (_menu) {
						gtk_widget_show(_menu);
						Ref<MenuImpl> ret = new MenuImpl();
						if (ret.isNotNull()) {
							ret->m_menu = _menu;
							MenuMap* map = GetMenuMap();
							if (map) {
								map->put(_menu, ret);
							}
							return ret;
						}
					}
					return sl_null;
				}

				Ref<MenuItem> addMenuItem(const MenuItemParam& param) override
				{
					return insertMenuItem(SLIB_UINT32_MAX, param);
				}

				Ref<MenuItem> insertMenuItem(sl_uint32 index, const MenuItemParam& param) override
				{
					ObjectLocker lock(this);
					sl_uint32 n = (sl_uint32)(m_items.getCount());
					if (index > n) {
						index = n;
					}
					Ref<MenuItem> item = MenuItemImpl::create(this, index, param);
					if (item.isNotNull()) {
						m_items.insert(index, item);
						return item;
					}
					return sl_null;
				}

				Ref<MenuItem> addSeparator() override
				{
					return insertSeparator(SLIB_UINT32_MAX);
				}

				Ref<MenuItem> insertSeparator(sl_uint32 index) override
				{
					ObjectLocker lock(this);
					sl_uint32 n = (sl_uint32)(m_items.getCount());
					if (index > n) {
						index = n;
					}

					GtkWidget* mii = gtk_separator_menu_item_new();
					gtk_menu_shell_insert((GtkMenuShell*)m_menu, mii, index );
					Ref<MenuItem> item = MenuItem::createSeparator();
					if (item.isNotNull()) {
						m_items.insert(index, item);
						return item;
					}
					return sl_null;
				}

				void removeMenuItem(sl_uint32 index) override
				{
					ObjectLocker lock(this);
					if (index < m_items.getCount()) {
						Ref<MenuItemImpl> item = Ref<MenuItemImpl>::from(m_items.getValueAt(index));
						gtk_container_remove((GtkContainer*)m_menu, item->m_menuItem);
					}
				}

				void removeMenuItem(const Ref<MenuItem>& item) override
				{
					ObjectLocker lock(this);
					sl_reg index = m_items.indexOf(item);
					if (index >= 0) {
						m_items.removeAt(index);
						Ref<MenuItemImpl> itemImpl = Ref<MenuItemImpl>::from(item);
						gtk_container_remove((GtkContainer*)m_menu, itemImpl->m_menuItem);
					}
				}

				static void _menuPositioningFunc (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
				{
					UIPoint *pt = (UIPoint*)user_data;
					*x = pt->x;
					*y = pt->y;
					*push_in = true;
				}
				void show(sl_ui_pos x, sl_ui_pos y) override
				{
					UIPoint pt = {x , y};
					int event_time = gtk_get_current_event_time();
					gtk_menu_popup ((GtkMenu*)m_menu, NULL, NULL, &_menuPositioningFunc, &pt, 0, event_time);
				}

				friend class MenuItemImpl;
			};

			SLIB_DEFINE_OBJECT(MenuImpl, Menu)

			Ref<MenuItemImpl> MenuItemImpl::create(MenuImpl* parent, sl_uint32 index, const MenuItemParam& param)
			{
				String text = makeText(param.text, param.shortcutKey, param.secondShortcutKey);
				GtkWidget* item = gtk_menu_item_new_with_mnemonic(text.getData());
				if (!item) {
					return sl_null;
				}
				gtk_widget_show(item);
				gtk_widget_set_sensitive(item, param.flagEnabled);
				if (param.submenu.isNotNull()) {
					GtkWidget* subMenu = UIPlatform::getMenuHandle(param.submenu);
					if(subMenu){
						gtk_menu_item_set_submenu((GtkMenuItem*)item, subMenu);
					}
				}

				gtk_menu_shell_insert((GtkMenuShell*)parent->m_menu, item, index);
				Ref<MenuItemImpl> ret = new MenuItemImpl;
				if (ret.isNotNull()) {
					ret->m_menuItem = item;
					ret->m_parent = parent;
					ret->m_text = param.text;
					ret->m_shortcutKey = param.shortcutKey;
					ret->m_secondShortcutKey = param.secondShortcutKey;
					ret->m_flagEnabled = param.flagEnabled;
					ret->m_flagChecked = param.flagChecked;
					ret->m_icon = param.icon;
					ret->m_checkedIcon = param.checkedIcon;
					ret->m_submenu = param.submenu;
					ret->setAction(param.action);
					g_signal_connect(item, "select", G_CALLBACK(onSelected), ret.get());
					return ret;
				}
				g_object_unref(item);
				return sl_null;
			}

			String MenuItemImpl::makeText(const String& title, const KeycodeAndModifiers& shortcutKey, const KeycodeAndModifiers& secondShortcutKey)
			{
				String text = title.replaceAll('&', '_');
				if (shortcutKey.getKeycode() != Keycode::Unknown) {
					text += "\t";
					text += shortcutKey.toString();
					if (secondShortcutKey.getKeycode() != Keycode::Unknown) {
						text += ", ";
						text += secondShortcutKey.toString();
					}
				} else {
					if (secondShortcutKey.getKeycode() != Keycode::Unknown) {
						text += "\t";
						text += secondShortcutKey.toString();
					}
				}
				return text;
			}

			void MenuItemImpl::_updateText()
			{
				String text = makeText(m_text, m_shortcutKey, m_secondShortcutKey);
				gtk_menu_item_set_label((GtkMenuItem*)m_menuItem, text.getData());
			}

			void MenuItemImpl::_updateState()
			{
			}

			void MenuItemImpl::setIcon(const Ref<Drawable>& icon)
			{
			}

			void MenuItemImpl::setCheckedIcon(const Ref<Drawable>& icon)
			{
			}

			void MenuItemImpl::setSubmenu(const Ref<Menu>& menu)
			{
				MenuItem::setSubmenu(menu);
				GtkWidget* subMenu = UIPlatform::getMenuHandle(menu);
				gtk_menu_item_set_submenu((GtkMenuItem*)m_menuItem, subMenu);
			}

			void MenuItemImpl::onSelected(GtkMenuItem *menuitem, gpointer user_data)
			{
				MenuItemImpl* menu = (MenuItemImpl*)user_data;
				if(menu){
					menu->getAction();
				}
			}

		}
	}

	using namespace priv::menu;

	Ref<Menu> Menu::create(sl_bool flagPopup)
	{
		return MenuImpl::create(flagPopup);
	}


	GtkWidget* UIPlatform::getMenuHandle(const Ref<Menu>& menu)
	{
		if (MenuImpl* _menu = CastInstance<MenuImpl>(menu.get())) {
			return _menu->m_menu;
		}
		return NULL;
	}

	Ref<Menu> UIPlatform::getMenu(GtkWidget* menu)
	{
		MenuMap* map = GetMenuMap();
		if (map) {
			return map->getValue(menu, WeakRef<MenuImpl>::null());
		}
		return sl_null;
	}

}

#endif
