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

#include "../../../core/definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)

#include "../../../core/dl.h"

#include "gtk/gtk.h"

// GTK2 Compatible Support
#define GdkDrawable void
#define GdkColormap void
#define Display void
#define XID unsigned long

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(gdk, "libgdk-3.so.0", "libgdk-x11-2.0.so.0")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_threads_init,
			void,
		)
		#define gdk_threads_init slib::gdk::getApi_gdk_threads_init()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_threads_enter,
			void,
		)
		#define gdk_threads_enter slib::gdk::getApi_gdk_threads_enter()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_threads_leave,
			void,
		)
		#define gdk_threads_leave slib::gdk::getApi_gdk_threads_leave()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_threads_add_idle_full,
			guint, ,
			gint priority,
			GSourceFunc function,
			gpointer data,
			GDestroyNotify notify
		)
		#define gdk_threads_add_idle_full slib::gdk::getApi_gdk_threads_add_idle_full()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_threads_add_idle,
			guint, ,
			GSourceFunc function,
			gpointer data
		)
		#define gdk_threads_add_idle slib::gdk::getApi_gdk_threads_add_idle()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_threads_add_timeout_full,
			guint, ,
			gint priority,
			guint interval,
			GSourceFunc function,
			gpointer data,
			GDestroyNotify notify
		)
		#define gdk_threads_add_timeout_full slib::gdk::getApi_gdk_threads_add_timeout_full()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_threads_add_timeout,
			guint, ,
			guint interval,
			GSourceFunc function,
			gpointer data
		)
		#define gdk_threads_add_timeout slib::gdk::getApi_gdk_threads_add_timeout()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_display_manager_get,
			GdkDisplayManager *, ,
			void
		)
		#define gdk_display_manager_get slib::gdk::getApi_gdk_display_manager_get()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_display_manager_list_displays,
			GSList *, ,
			GdkDisplayManager *display_manager
		)
		#define gdk_display_manager_list_displays slib::gdk::getApi_gdk_display_manager_list_displays()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_display_get_n_screens,
			gint, ,
			GdkDisplay  *display
		)
		#define gdk_display_get_n_screens slib::gdk::getApi_gdk_display_get_n_screens()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_display_get_screen,
			GdkScreen *, ,
			GdkDisplay  *display,
			gint         screen_num
		)
		#define gdk_display_get_screen slib::gdk::getApi_gdk_display_get_screen()
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
			gdk_screen_get_root_window,
			GdkWindow *, ,
			GdkScreen *screen
		)
		#define gdk_screen_get_root_window slib::gdk::getApi_gdk_screen_get_root_window()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_screen_get_active_window,
			GdkWindow *, ,
			GdkScreen *screen
		)
		#define gdk_screen_get_active_window slib::gdk::getApi_gdk_screen_get_active_window()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_drawable_get_size,
			void, ,
			GdkDrawable *,
			gint *width,
			gint *height
		)
		#define gdk_drawable_get_size slib::gdk::getApi_gdk_drawable_get_size()
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
			gdk_get_default_root_window,
			GdkWindow *, ,
			void
		)
		#define gdk_get_default_root_window slib::gdk::getApi_gdk_get_default_root_window()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_window_get_parent,
			GdkWindow *, ,
			GdkWindow *window
		)
		#define gdk_window_get_parent slib::gdk::getApi_gdk_window_get_parent()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_window_get_origin,
			gint, ,
			GdkWindow *window,
			gint *x,
			gint *y
		)
		#define gdk_window_get_origin slib::gdk::getApi_gdk_window_get_origin()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_window_get_root_origin,
			void, ,
			GdkWindow *window,
			gint *x,
			gint *y
		)
		#define gdk_window_get_root_origin slib::gdk::getApi_gdk_window_get_root_origin()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_window_get_frame_extents,
			void, ,
			GdkWindow *window,
			GdkRectangle *rect
		)
		#define gdk_window_get_frame_extents slib::gdk::getApi_gdk_window_get_frame_extents()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_window_get_geometry,
			void, ,
			GdkWindow *window,
			gint *x,
			gint *y,
			gint *width,
			gint *height,
			gint *depth
		)
		#define gdk_window_get_geometry slib::gdk::getApi_gdk_window_get_geometry()
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
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_window_get_display,
			GdkDisplay *, ,
			GdkWindow *window
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_x11_display_get_xdisplay,
			Display *, ,
			GdkDisplay  *display
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_window_invalidate_rect,
			void, ,
			GdkWindow *window,
			const GdkRectangle *rect,
			gboolean invalidate_children
		)
		#define gdk_window_invalidate_rect slib::gdk::getApi_gdk_window_invalidate_rect()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_display_put_event,
			void, ,
			GdkDisplay *display,
			const GdkEvent *event
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_event_handler_set,
			void, ,
			GdkEventFunc func,
			gpointer data,
			GDestroyNotify notify
		)
		#define gdk_event_handler_set slib::gdk::getApi_gdk_event_handler_set()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_event_put,
			void, ,
			const GdkEvent *event
		)
		#define gdk_event_put slib::gdk::getApi_gdk_event_put()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_pixbuf_get_from_drawable,
			GdkPixbuf *, ,
			GdkPixbuf   *dest,
			GdkDrawable *src,
			GdkColormap *cmap,
			int          src_x,
			int          src_y,
			int          dest_x,
			int          dest_y,
			int          width,
			int          height
		)
		#define gdk_pixbuf_get_from_drawable slib::gdk::getApi_gdk_pixbuf_get_from_drawable()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_pixbuf_get_colorspace,
			GdkColorspace, ,
			const GdkPixbuf *pixbuf
		)
		#define gdk_pixbuf_get_colorspace slib::gdk::getApi_gdk_pixbuf_get_colorspace()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_pixbuf_get_n_channels,
			int, ,
			const GdkPixbuf *pixbuf
		)
		#define gdk_pixbuf_get_n_channels slib::gdk::getApi_gdk_pixbuf_get_n_channels()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_pixbuf_get_has_alpha,
			gboolean, ,
			const GdkPixbuf *pixbuf
		)
		#define gdk_pixbuf_get_has_alpha slib::gdk::getApi_gdk_pixbuf_get_has_alpha()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_pixbuf_get_bits_per_sample,
			int, ,
			const GdkPixbuf *pixbuf
		)
		#define gdk_pixbuf_get_bits_per_sample slib::gdk::getApi_gdk_pixbuf_get_bits_per_sample()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_pixbuf_get_pixels,
			guchar*, ,
			const GdkPixbuf *pixbuf
		)
		#define gdk_pixbuf_get_pixels slib::gdk::getApi_gdk_pixbuf_get_pixels()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_pixbuf_get_width,
			int, ,
			const GdkPixbuf *pixbuf
		)
		#define gdk_pixbuf_get_width slib::gdk::getApi_gdk_pixbuf_get_width()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_pixbuf_get_height,
			int, ,
			const GdkPixbuf *pixbuf
		)
		#define gdk_pixbuf_get_height slib::gdk::getApi_gdk_pixbuf_get_height()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_pixbuf_get_rowstride,
			int, ,
			const GdkPixbuf *pixbuf
		)
		#define gdk_pixbuf_get_rowstride slib::gdk::getApi_gdk_pixbuf_get_rowstride()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_pixbuf_new_from_data,
			GdkPixbuf *, ,
			const guchar *data,
			GdkColorspace colorspace,
			gboolean has_alpha,
			int bits_per_sample,
			int width, int height,
			int rowstride,
			GdkPixbufDestroyNotify destroy_fn,
			gpointer destroy_fn_data
		)
		#define gdk_pixbuf_new_from_data slib::gdk::getApi_gdk_pixbuf_new_from_data()

		// GTK3
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_window_get_width,
			int, ,
			GdkWindow *window
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_window_get_height,
			int, ,
			GdkWindow *window
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_pixbuf_get_from_window,
			GdkPixbuf *, ,
			GdkWindow *window,
			gint src_x,
			gint src_y,
			gint width,
			gint height
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_cairo_get_clip_rectangle,
			gboolean, ,
			cairo_t *cr,
			GdkRectangle *rect
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gdk_x11_window_get_xid,
			XID, ,
			GdkWindow   *window
		)

	SLIB_IMPORT_LIBRARY_END

}

#undef GdkDrawable
#undef GdkColormap
#undef Display
#undef XID

#endif

#endif
