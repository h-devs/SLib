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

#ifndef CHECKHEADER_SLIB_UI_DL_LINUX_GDK
#define CHECKHEADER_SLIB_UI_DL_LINUX_GDK

#include "definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX) && defined(SLIB_PLATFORM_IS_DESKTOP)

#include "../core/dl.h"

#include "gtk/gtk.h"
#include "gtk/gdk/x11/gdkx.h"

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(gdk, "libgdk-x11-2.0.so.0")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_screen_get_default,
			GdkScreen *, ,
			void
		)
		#define gdk_screen_get_default slib::gdk::getApi_gdk_screen_get_default()
		SLIB_IMPORT_LIBRARY_WRAP_FUNCTION(
			gdk_screen_get_resolution,
			gdouble, ,
			GdkScreen *screen
		)
		#define gdk_screen_get_resolution slib::gdk::wrap_gdk_screen_get_resolution
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_screen_get_width,
			gint, ,
			GdkScreen *screen
		)
		#define gdk_screen_get_width slib::gdk::getApi_gdk_screen_get_width()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_screen_get_height,
			gint, ,
			GdkScreen *screen
		)
		#define gdk_screen_get_height slib::gdk::getApi_gdk_screen_get_height()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_keymap_get_default,
			GdkKeymap *, ,
			void
		)
		#define gdk_keymap_get_default slib::gdk::getApi_gdk_keymap_get_default()
		SLIB_IMPORT_LIBRARY_WRAP_FUNCTION(
			gdk_keymap_get_caps_lock_state,
			gboolean, ,
			GdkKeymap *keymap
		)
		#define gdk_keymap_get_caps_lock_state slib::gdk::wrap_gdk_keymap_get_caps_lock_state
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_display_get_default,
			GdkDisplay *, ,
			void
		)
		#define gdk_display_get_default slib::gdk::getApi_gdk_display_get_default()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_display_get_pointer,
			void, ,
			GdkDisplay *display,
			GdkScreen **screen,
			gint *x,
			gint *y,
			GdkModifierType *mask
		)
		#define gdk_display_get_pointer slib::gdk::getApi_gdk_display_get_pointer()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_window_get_origin,
			gint, ,
			GdkWindow *window,
			gint *x,
			gint *y
		)
		#define gdk_window_get_origin slib::gdk::getApi_gdk_window_get_origin()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_window_raise,
			void, ,
			GdkWindow *window
		)
		#define gdk_window_raise slib::gdk::getApi_gdk_window_raise()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_cairo_create,
			cairo_t *, ,
			GdkDrawable *drawable
		)
		#define gdk_cairo_create slib::gdk::getApi_gdk_cairo_create()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_x11_drawable_get_xdisplay,
			Display *, ,
			GdkDrawable *drawable
		)
		#define gdk_x11_drawable_get_xdisplay slib::gdk::getApi_gdk_x11_drawable_get_xdisplay()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_x11_drawable_get_xid,
			XID, ,
			GdkDrawable *drawable
		)
		#define gdk_x11_drawable_get_xid slib::gdk::getApi_gdk_x11_drawable_get_xid()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_x11_window_get_drawable_impl,
			GdkDrawable *, ,
			GdkWindow *window
		)
		#define gdk_x11_window_get_drawable_impl slib::gdk::getApi_gdk_x11_window_get_drawable_impl()
	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
