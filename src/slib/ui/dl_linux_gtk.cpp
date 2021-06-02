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

#include "slib/ui/dl/linux/gtk.h"

namespace slib
{
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

		GdkWindow * wrap_gtk_widget_get_window(GtkWidget *widget)
		{
			auto func = getApi_gtk_widget_get_window();
			if (func) {
				return func(widget);
			}
			return sl_null;
		}

		void wrap_gtk_file_chooser_set_create_folders(GtkFileChooser *chooser, gboolean create_folders)
		{
			auto func = getApi_gtk_file_chooser_set_create_folders();
			if (func) {
				return func(chooser, create_folders);
			}
		}

	}
}
