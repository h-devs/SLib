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

#define SLIB_IMPLEMENT_DYNAMIC_LIBRARY

#include "slib/dl/linux/gtk.h"

namespace slib
{

	namespace gobject
	{
		typedef GTypeInstance* (*DL_FUNC_TYPE_g_type_check_instance_cast)(GTypeInstance *instance, GType iface_type);
		DL_FUNC_TYPE_g_type_check_instance_cast getApi_g_type_check_instance_cast();
		#define g_type_check_instance_cast slib::gobject::getApi_g_type_check_instance_cast()
	}

	namespace gtk
	{

		gboolean wrap_gtk_show_uri(
				GdkScreen *screen,
				const gchar *uri,
				guint32 timestamp,
				GError **error
		)
		{
			auto func = getApi_gtk_show_uri();
			if (func) {
				return func(screen, uri, timestamp, error);
			}
			return sl_false;
		}

		void wrap_gtk_window_set_opacity(GtkWindow *window, gdouble opacity)
		{
			auto func = getApi_gtk_window_set_opacity();
			if (func) {
				return func(window, opacity);
			}
		}

		void wrap_gtk_window_set_deletable(GtkWindow *window, gboolean setting)
		{
			auto func = getApi_gtk_window_set_deletable();
			if (func) {
				return func(window, setting);
			}
		}

		void wrap_gtk_file_chooser_set_create_folders(GtkFileChooser *chooser, gboolean create_folders)
		{
			auto func = getApi_gtk_file_chooser_set_create_folders();
			if (func) {
				return func(chooser, create_folders);
			}
		}


		// GTK2 Support
		struct Gtk2Object
		{
			GInitiallyUnowned parent_instance;
			guint32 flags;
		};

		struct Gtk2Widget
		{
			Gtk2Object object;
			guint16 private_flags;
			guint8 state;
			guint8 saved_state;
			gchar *name;
			GtkStyle *style;
			GtkRequisition requisition;
			GtkAllocation allocation;
			GdkWindow *window;
			GtkWidget *parent;
		};

#define GTK_NO_WINDOW (1 << 5)
#define GTK_CAN_FOCUS (1 << 11)

#define GTK_WIDGET_SET_FLAGS(w, f) (reinterpret_cast<Gtk2Object*>(w))->flags |= (f)
#define GTK_WIDGET_UNSET_FLAGS(w, f) (reinterpret_cast<Gtk2Object*>(w))->flags &= ~(f)

		void wrap_gtk_widget_set_can_focus(GtkWidget *widget, gboolean can_focus)
		{
			auto func = getApi_gtk_widget_set_can_focus();
			if (func) {
				func(widget, can_focus);
			} else {
				if (can_focus) {
					GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
				} else {
					GTK_WIDGET_UNSET_FLAGS(widget, GTK_CAN_FOCUS);
				}
			}
		}

		void wrap_gtk_widget_set_has_window(GtkWidget *widget, gboolean has_window)
		{
			auto func = getApi_gtk_widget_set_has_window();
			if (func) {
				func(widget, has_window);
			} else {
				if (has_window) {
					GTK_WIDGET_UNSET_FLAGS(widget, GTK_NO_WINDOW);
				} else {
					GTK_WIDGET_SET_FLAGS(widget, GTK_NO_WINDOW);
				}
			}
		}

		GdkWindow* wrap_gtk_widget_get_window(GtkWidget *widget)
		{
			auto func = getApi_gtk_widget_get_window();
			if (func) {
				return func(widget);
			} else {
				return (reinterpret_cast<Gtk2Widget*>(widget))->window;
			}
		}

		void wrap_gtk_widget_get_allocation(GtkWidget *widget, GtkAllocation* allocation)
		{
			auto func = getApi_gtk_widget_get_allocation();
			if (func) {
				func(widget, allocation);
			} else {
				*allocation = (reinterpret_cast<Gtk2Widget*>(widget))->allocation;
			}
		}

	}
}
