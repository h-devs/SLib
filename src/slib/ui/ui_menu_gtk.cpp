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
				String m_accel;
				sl_bool m_flagCheckable;

			public:
				MenuItemImpl()
				{
					m_handle = sl_null;
					m_flagCheckable = sl_false;
				}

				~MenuItemImpl()
				{
					GtkMenuItem* handle = m_handle;
					if (handle) {
						if (m_accel.isNotNull()) {
							gtk_accel_map_add_entry(m_accel.getData(), 0, (GdkModifierType)0);
						}
						g_object_unref(handle);
						m_handle = handle;
					}
				}

			public:
				static Ref<MenuItemImpl> create(MenuImpl* parent, sl_uint32 index, const MenuItemParam& param);

			public:
				void setText(const String& text) override
				{
					MenuItem::setText(text);
					String _text = text.replaceAll('&', '_');
					gtk_menu_item_set_label(m_handle, _text.getData());
				}

				void setShortcutKey(const KeycodeAndModifiers& km) override
				{
					ObjectLocker lock(this);
					MenuItem::setShortcutKey(km);
					guint key = UIEvent::getSystemKeycode(km.getKeycode());
					int modifiers = 0;
					if (km.isAltKey()) {
						modifiers |= GDK_MOD1_MASK;
					}
					if (km.isWindowsKey()) {
						modifiers |= GDK_MOD4_MASK;
					}
					if (km.isShiftKey()) {
						modifiers |= GDK_SHIFT_MASK;
					}
					if (km.isControlKey()) {
						modifiers |= GDK_CONTROL_MASK;
					}
					if (m_accel.isNull()) {
						if (km.getKeycode() != Keycode::Unknown) {
							static sl_int32 _n = 0;
							sl_int32 n = Base::interlockedIncrement32(&_n);
							m_accel = "<slib>/<a" + String::fromUint32(n) + ">";
							gtk_accel_map_add_entry(m_accel.getData(), key, (GdkModifierType)modifiers);
							gtk_menu_item_set_accel_path(m_handle, m_accel.getData());
						}
					} else {
						if (km.getKeycode() == Keycode::Unknown) {
							gtk_accel_map_add_entry(m_accel.getData(), 0, (GdkModifierType)0);
						} else {
							gtk_accel_map_change_entry(m_accel.getData(), key, (GdkModifierType)modifiers, 1);
						}
					}
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

				static void _on_selected(GtkMenuItem*, gpointer user_data)
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

				static void _menu_position_func(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
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
						gtk_menu_popup((GtkMenu*)m_handle, sl_null, sl_null, &_menu_position_func, &pt, 0, event_time);
					}
				}

			};

			SLIB_DEFINE_OBJECT(MenuImpl, Menu)


			Ref<MenuItemImpl> MenuItemImpl::create(MenuImpl* parent, sl_uint32 index, const MenuItemParam& param)
			{
				String text = param.text.replaceAll('&', '_');
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
					ret->setAction(param.action);

					if (param.shortcutKey.getKeycode() != Keycode::Unknown) {
						ret->setShortcutKey(param.shortcutKey);
					}
					ret->m_secondShortcutKey = param.secondShortcutKey;

					g_signal_connect(item, "select", G_CALLBACK(_on_selected), ret.get());
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
