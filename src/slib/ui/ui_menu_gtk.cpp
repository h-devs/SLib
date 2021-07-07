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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_GTK)

#include "slib/ui/menu.h"

#include "slib/ui/platform.h"
#include "slib/core/hash_map.h"
#include "slib/core/safe_static.h"

#include "view_gtk.h"

namespace slib
{

	namespace priv
	{
		namespace menu
		{

			class MenuImpl;

			typedef CHashMap< GtkMenuShell*, WeakRef<MenuImpl> > MenuMap;
			SLIB_SAFE_STATIC_GETTER(MenuMap, GetMenuMap)

			class MenuItemImpl : public MenuItem
			{
				SLIB_DECLARE_OBJECT

			public:
				GtkMenuItem* m_handle;
				sl_bool m_flagCheckable;

				GtkAccelGroup* m_accelGroup;
				sl_uint32 m_accelKey;
				sl_uint32 m_accelMods;

			public:
				MenuItemImpl()
				{
					m_handle = sl_null;
					m_flagCheckable = sl_false;

					m_accelGroup = sl_null;
					m_accelKey = 0;
					m_accelMods = 0;
				}

				~MenuItemImpl()
				{
					if (m_handle) {
						g_object_unref(m_handle);
						m_handle = sl_null;
					}
					if (m_accelGroup) {
						g_object_unref(m_accelGroup);
						m_accelGroup = sl_null;
					}
				}

			public:
				static Ref<MenuItemImpl> create(MenuImpl* parent, sl_uint32 index, const MenuItemParam& param);

			public:
				void setText(const String& text) override
				{
					MenuItem::setText(text);
					StringCstr _text = text.replaceAll('&', '_');
					gtk_menu_item_set_label(m_handle, _text.getData());
				}

				void setShortcutKey(const KeycodeAndModifiers& km) override
				{
					MenuItem::setShortcutKey(km);
					if (!m_handle) {
						return;
					}
					ObjectLocker lock(this);
					if (!m_accelGroup) {
						m_accelGroup = gtk_accel_group_new();
						if (!m_accelGroup) {
							return;
						}
					}
					if (m_accelKey) {
						gtk_widget_remove_accelerator((GtkWidget*)m_handle, m_accelGroup, (guint)m_accelKey, (GdkModifierType)m_accelMods);
					}
					m_accelKey = UIEvent::getSystemKeycode(km.getKeycode());
					sl_uint32 mods = 0;
					if (km.isShiftKey()) {
						mods |= GDK_SHIFT_MASK;
					}
					if (km.isControlKey()) {
						mods |= GDK_CONTROL_MASK;
					}
					if (km.isAltKey()) {
						mods |= GDK_MOD1_MASK;
					}
					if (km.isWindowsKey()) {
						mods |= GDK_MOD4_MASK;
					}
					m_accelMods = mods;
					gtk_widget_add_accelerator((GtkWidget*)m_handle, "accel", m_accelGroup, (guint)m_accelKey, (GdkModifierType)mods, GTK_ACCEL_VISIBLE);
				}

				void setSecondShortcutKey(const KeycodeAndModifiers& km) override
				{
					MenuItem::setSecondShortcutKey(km);
				}

				void setEnabled(sl_bool flag) override
				{
					MenuItem::setEnabled(flag);
					gtk_widget_set_sensitive((GtkWidget*)m_handle, flag ? 1 : 0);
				}

				void setChecked(sl_bool flag) override
				{
					MenuItem::setChecked(flag);
					if (m_flagCheckable) {
						gtk_check_menu_item_set_active((GtkCheckMenuItem*)m_handle, flag ? 1 : 0);
					}
				}

				void setSubmenu(const Ref<Menu>& menu) override
				{
					MenuItem::setSubmenu(menu);
					GtkMenuShell* hSubmenu = UIPlatform::getMenuHandle(menu);
					gtk_menu_item_set_submenu(m_handle, (GtkWidget*)hSubmenu);
				}

				static void _callback_activated(GtkMenuItem*, gpointer user_data)
				{
					MenuItemImpl* menu = (MenuItemImpl*)user_data;
					if (menu){
						menu->getAction()();
					}
				}

			};

			SLIB_DEFINE_OBJECT(MenuItemImpl, MenuItem)


			class MenuImpl : public Menu
			{
				SLIB_DECLARE_OBJECT

			public:
				GtkMenuShell* m_handle;
				sl_bool m_flagPopup;

			public:
				MenuImpl()
				{
					m_handle = sl_null;
					m_flagPopup = sl_false;
				}

				~MenuImpl()
				{
					GtkMenuShell* handle = m_handle;
					if (handle) {
						MenuMap* map = GetMenuMap();
						if (map) {
							map->remove(handle);
						}
						g_object_unref(handle);
						m_handle = sl_null;
					}
				}

			public:
				static Ref<MenuImpl> create(sl_bool flagPopup)
				{
					GtkWidget* widget;
					if (flagPopup) {
						widget = gtk_menu_new();
					} else {
						widget = gtk_menu_bar_new();
						gtk_menu_bar_set_child_pack_direction((GtkMenuBar*)widget, GTK_PACK_DIRECTION_LTR);
					}
					if (widget) {
						g_object_ref_sink(widget);
						Ref<MenuImpl> ret = new MenuImpl();
						if (ret.isNotNull()) {
							ret->m_handle = (GtkMenuShell*)widget;
							ret->m_flagPopup = flagPopup;
							MenuMap* map = GetMenuMap();
							if (map) {
								map->put((GtkMenuShell*)widget, ret);
							}
							gtk_widget_show(widget);
							return ret;
						}
						g_object_unref(widget);
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
					GtkWidget* handle = gtk_separator_menu_item_new();
					if (handle) {
						gtk_widget_show(handle);
						gtk_menu_shell_insert(m_handle, handle, index);
						Ref<MenuItem> item = MenuItem::createSeparator();
						if (item.isNotNull()) {
							m_items.insert(index, item);
							return item;
						}
					}
					return sl_null;
				}

				void removeMenuItem(sl_uint32 index) override
				{
					ObjectLocker lock(this);
					if (index < m_items.getCount()) {
						Ref<MenuItemImpl> item = Ref<MenuItemImpl>::from(m_items.getValueAt(index));
						if (item.isNotNull()) {
							gtk_container_remove((GtkContainer*)m_handle, (GtkWidget*)(item->m_handle));
							m_items.removeAt(index);
						}
					}
				}

				void removeMenuItem(const Ref<MenuItem>& item) override
				{
					ObjectLocker lock(this);
					sl_reg index = m_items.indexOf(item);
					if (index >= 0) {
						removeMenuItem((sl_uint32)index);
					}
				}

				static void _callback_menu_position(GtkMenu*, gint *x, gint *y, gboolean *push_in, gpointer user_data)
				{
					UIPoint *pt = (UIPoint*)user_data;
					*x = (gint)(pt->x);
					*y = (gint)(pt->y);
					*push_in = true;
				}

				void show(sl_ui_pos x, sl_ui_pos y) override
				{
					if (m_flagPopup) {
						UIPoint pt = {x, y};
						int event_time = gtk_get_current_event_time();
						gtk_menu_popup((GtkMenu*)m_handle, sl_null, sl_null, &_callback_menu_position, &pt, 0, event_time);
					}
				}

			};

			SLIB_DEFINE_OBJECT(MenuImpl, Menu)


			Ref<MenuItemImpl> MenuItemImpl::create(MenuImpl* parent, sl_uint32 index, const MenuItemParam& param)
			{
				StringCstr text = param.text.replaceAll('&', '_');
				GtkWidget* widget;
				if (param.flagCheckable) {
					widget = gtk_check_menu_item_new_with_mnemonic(text.getData());
				} else {
					widget = gtk_menu_item_new_with_mnemonic(text.getData());
				}
				if (!widget) {
					return sl_null;
				}

				g_object_ref_sink(widget);

				Ref<MenuItemImpl> ret = new MenuItemImpl;
				if (ret.isNotNull()) {

					GtkMenuItem* item = (GtkMenuItem*)widget;
					if (!(param.flagEnabled)) {
						gtk_widget_set_sensitive(widget, 0);
					}
					if (param.flagCheckable) {
						gtk_check_menu_item_set_active((GtkCheckMenuItem*)item, param.flagChecked ? 1 : 0);
					}
					if (param.submenu.isNotNull()) {
						GtkMenuShell* submenu = UIPlatform::getMenuHandle(param.submenu);
						if(submenu){
							gtk_menu_item_set_submenu(item, (GtkWidget*)submenu);
						}
					}

					gtk_menu_shell_insert(parent->m_handle, widget, index);

					ret->m_handle = item;
					ret->m_parent = parent;
					ret->m_text = param.text;
					ret->m_flagCheckable = param.flagCheckable;
					ret->m_flagEnabled = param.flagEnabled;
					ret->m_flagChecked = param.flagChecked;
					ret->m_icon = param.icon;
					ret->m_checkedIcon = param.checkedIcon;
					ret->m_submenu = param.submenu;
					ret->m_secondShortcutKey = param.secondShortcutKey;
					ret->setShortcutKey(param.shortcutKey);
					ret->setAction(param.action);

					g_signal_connect(item, "activate", G_CALLBACK(_callback_activated), ret.get());
					gtk_widget_show(widget);

					return ret;
				}
				g_object_unref(widget);
				return sl_null;
			}

		}
	}

	using namespace priv::menu;

	Ref<Menu> Menu::create(sl_bool flagPopup)
	{
		return MenuImpl::create(flagPopup);
	}


	GtkMenuShell* UIPlatform::getMenuHandle(const Ref<Menu>& _menu)
	{
		if (MenuImpl* menu = CastInstance<MenuImpl>(_menu.get())) {
			return menu->m_handle;
		}
		return sl_null;
	}

	Ref<Menu> UIPlatform::getMenu(GtkMenuShell* menu)
	{
		MenuMap* map = GetMenuMap();
		if (map) {
			return map->getValue(menu, WeakRef<MenuImpl>::null());
		}
		return sl_null;
	}

	sl_bool UIPlatform::isPopupMenu(const Ref<Menu>& _menu)
	{
		if (MenuImpl* menu = CastInstance<MenuImpl>(_menu.get())) {
			return menu->m_flagPopup;
		}
		return sl_false;
	}

}

#endif
