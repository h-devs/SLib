/*
*   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/system_tray_icon.h"

#include "slib/graphics/image.h"
#include "slib/ui/platform.h"
#include "slib/ui/menu.h"
#include "slib/ui/notification.h"

namespace slib
{

	namespace priv
	{
		namespace system_tray_icon
		{

			class DbusTrayIconImpl : public SystemTrayIcon
			{
			public:
				static Ref<DbusTrayIconImpl> create(const SystemTrayIconParam& param)
				{
					GtkApplication* app = UIPlatform::getApp();
					if (!app) {
						return sl_null;
					}
					auto funcGetDBusConnection = gio::getApi_g_application_get_dbus_connection();
					if (!funcGetDBusConnection) {
						return sl_null;
					}
					return sl_null;
				}
			};

			class SystemTrayIconImpl : public SystemTrayIcon
			{
			public:
				GtkStatusIcon* m_handle;
				Ref<Image> m_icon;

			public:
				static Ref<SystemTrayIconImpl> create(const SystemTrayIconParam& param)
				{
					if (param.icon.isNotNull()) {
						GdkPixbuf* icon = UIPlatform::createPixbuf(param.icon->toImage());
						if (icon) {
							GtkStatusIcon* handle = gtk_status_icon_new_from_pixbuf(icon);
							if (handle) {
								g_object_ref_sink(handle);
								Ref<SystemTrayIconImpl> ret = new SystemTrayIconImpl;
								if (ret.isNotNull()) {
									ret->m_handle = handle;
									ret->_init(param);
									if (param.toolTip.isNotNull()) {
										ret->setToolTip_NI(param.toolTip);
									}
									gtk_status_icon_set_visible(handle, sl_true);
									g_signal_connect(handle, "activate", G_CALLBACK(_callback_activated), ret.get());
									g_signal_connect(handle, "popup-menu", G_CALLBACK(_callback_activated), ret.get());
									return ret;
								}
								g_object_unref(handle);
								return sl_null;
							}
							g_object_unref(icon);
						}
					}
					return sl_null;
				}

				void setIcon_NI(const Ref<Drawable>& icon, const String& name) override
				{
					if (icon.isNotNull()) {
						GdkPixbuf* pixbuf = UIPlatform::createPixbuf(icon->toImage());
						if (pixbuf) {
							gtk_status_icon_set_from_pixbuf(m_handle, pixbuf);
						}
					} else {
						gtk_status_icon_set_from_pixbuf(m_handle, sl_null);
					}
				}

				void setToolTip_NI(const String& _toolTip) override
				{
					StringCstr toolTip(_toolTip);
					gtk_status_icon_set_tooltip_text(m_handle, toolTip.getData());
					gtk_status_icon_set_title(m_handle, toolTip.getData());
				}

				void setMenu_NI(const Ref<Menu>& menu) override
				{
				}

				void notify_NI(const SystemTrayIconNotifyParam& param) override
				{
					UserNotificationMessage msg;
					msg.title = param.title;
					msg.content = param.message;
					msg.identifier = "system_tray_icon";
					UserNotification::add(msg);
				}

				static void _callback_activated(GtkStatusIcon*, gpointer user_data)
				{
					SystemTrayIconImpl* object = (SystemTrayIconImpl*)user_data;
					if (object){
						Ref<UIEvent> event = UIEvent::createUnknown(Time::now());
						object->dispatchClick(event.get());
					}
				}

				void _callback_popupMenu(GtkStatusIcon* handle, guint button, guint activate_time, gpointer user_data)
				{
					SystemTrayIconImpl* object = (SystemTrayIconImpl*)user_data;
					if (object){
						Ref<Menu> menu = object->m_menu;
						if (menu.isNotNull() && UIPlatform::isPopupMenu(menu.get())) {
							GtkMenu* hMenu = (GtkMenu*)(UIPlatform::getMenuHandle(menu.get()));
							gtk_menu_popup(hMenu, sl_null, sl_null, gtk_status_icon_position_menu, handle, button, activate_time);
						}
					}
				}

			};

		}
	}

	using namespace priv::system_tray_icon;

	Ref<SystemTrayIcon> SystemTrayIcon::create(const SystemTrayIconParam& param)
	{
		return Ref<SystemTrayIcon>::from(SystemTrayIconImpl::create(param));
	}

}

#endif
