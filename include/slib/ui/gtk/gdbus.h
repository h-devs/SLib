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

#ifndef CHECKHEADER_SLIB_UI_GTK_GDBUS
#define CHECKHEADER_SLIB_UI_GTK_GDBUS

#include "../definition.h"

#ifdef SLIB_PLATFORM_IS_LINUX_DESKTOP

#include "platform.h"

namespace slib
{
	namespace gtk
	{

		class GDBus
		{
		public:
			static GDBusConnection* getDefaultConnection()
			{
				GtkApplication* app = UIPlatform::getApp();
				if (app) {
					auto funcGetDBusConnection = gio::getApi_g_application_get_dbus_connection();
					if (funcGetDBusConnection) {
						GDBusConnection* connection = funcGetDBusConnection((GApplication*)app);
						if (connection) {
							g_object_ref(connection);
							return connection;
						}
					}
				}
				auto funcGetSync = gio::getApi_g_bus_get_sync();
				if (funcGetSync) {
					return funcGetSync(G_BUS_TYPE_SESSION, sl_null, sl_null);
				}
				return sl_null;
			}

		};

	}
}

#endif

#endif
