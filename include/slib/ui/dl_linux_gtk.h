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

#ifndef CHECKHEADER_SLIB_UI_DL_LINUX_GTK
#define CHECKHEADER_SLIB_UI_DL_LINUX_GTK

#include "definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX) && defined(SLIB_PLATFORM_IS_DESKTOP)

#include "../core/dl.h"
#include "gtk/gtk.h"

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(gtk, "libgtk-x11-2.0.so.0")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_init_check, gboolean, ,
			int *argc, char ***argv
		)
		#define gtk_init_check	slib::gtk::getApi_gtk_init_check()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_main, void, , void
		)
		#define gtk_main	slib::gtk::getApi_gtk_main()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_main_quit, void, , void
		)
		#define gtk_main_quit	slib::gtk::getApi_gtk_main_quit()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_show_uri, gboolean, ,
			GdkScreen   *screen,
			const gchar *uri,
			guint32      timestamp,
			GError     **error
		)
		#define gtk_show_uri	slib::gtk::getApi_gtk_show_uri()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_object_get_type, GType, , void
		)
		#define gtk_object_get_type	slib::gtk::getApi_gtk_object_get_type()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_fixed_new, GtkWidget*, , void
		)
		#define gtk_fixed_new	slib::gtk::getApi_gtk_fixed_new()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_fixed_get_type, GType, , void
		)
		#define gtk_fixed_get_type	slib::gtk::getApi_gtk_fixed_get_type()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_fixed_put, void, ,
			GtkFixed       *fixed,
			GtkWidget      *widget,
			gint            x,
			gint            y
		)
		#define gtk_fixed_put	slib::gtk::getApi_gtk_fixed_put()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_fixed_move, void, ,
			GtkFixed       *fixed,
			GtkWidget      *widget,
			gint            x,
			gint            y
		)
		#define gtk_fixed_move	slib::gtk::getApi_gtk_fixed_move()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_container_get_type, GType, , void
		)
		#define gtk_container_get_type	slib::gtk::getApi_gtk_container_get_type()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_container_add, void, ,
			GtkContainer   *container,
			GtkWidget	   *widget
		)
		#define gtk_container_add	slib::gtk::getApi_gtk_container_add()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_container_remove, void, ,
			GtkContainer   *container,
			GtkWidget	   *widget
		)
		#define gtk_container_remove	slib::gtk::getApi_gtk_container_remove()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_drawing_area_new, GtkWidget*, ,
			void
		)
		#define gtk_drawing_area_new	slib::gtk::getApi_gtk_drawing_area_new()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_drawing_area_get_type, GType, , void
		)
		#define gtk_drawing_area_get_type	slib::gtk::getApi_gtk_drawing_area_get_type()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_clipboard_get, GtkClipboard *, ,
			GdkAtom       selection
		)
		#define gtk_clipboard_get	slib::gtk::getApi_gtk_clipboard_get()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_clipboard_clear, void, ,
			GtkClipboard          *clipboard
		)
		#define gtk_clipboard_clear	slib::gtk::getApi_gtk_clipboard_clear()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_clipboard_set_text, void, ,
			GtkClipboard          *clipboard,
			const gchar           *text,
			gint                   len
		)
		#define gtk_clipboard_set_text	slib::gtk::getApi_gtk_clipboard_set_text()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_clipboard_wait_for_text, gchar *, ,
			GtkClipboard  *clipboard
		)
		#define gtk_clipboard_wait_for_text	slib::gtk::getApi_gtk_clipboard_wait_for_text()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_clipboard_wait_is_text_available, gboolean, ,
			GtkClipboard  *clipboard
		)
		#define gtk_clipboard_wait_is_text_available	slib::gtk::getApi_gtk_clipboard_wait_is_text_available()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_message_dialog_new, GtkWidget*, ,
			GtkWindow      *parent,
			GtkDialogFlags  flags,
			GtkMessageType  type,
			GtkButtonsType  buttons,
			const gchar    *message_format,
            ...
		)
		#define gtk_message_dialog_new	slib::gtk::getApi_gtk_message_dialog_new()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_dialog_add_button, GtkWidget*, ,
			GtkDialog   *dialog,
			const gchar *button_text,
			gint         response_id
		)
		#define gtk_dialog_add_button	slib::gtk::getApi_gtk_dialog_add_button()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_dialog_set_alternative_button_order, void, ,
			GtkDialog *dialog,
			gint       first_response_id,
			...
		)
		#define gtk_dialog_set_alternative_button_order	slib::gtk::getApi_gtk_dialog_set_alternative_button_order()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_dialog_run, gint, ,
			GtkDialog *dialog
		)
		#define gtk_dialog_run	slib::gtk::getApi_gtk_dialog_run()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_new, GtkWidget*, ,
			GtkWindowType        type
		)
		#define gtk_window_new	slib::gtk::getApi_gtk_window_new()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_set_resizable, void, ,
			GtkWindow           *window,
			gboolean             resizable
		)
		#define gtk_window_set_resizable	slib::gtk::getApi_gtk_window_set_resizable()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_get_resizable, gboolean, ,
			GtkWindow           *window
		)
		#define gtk_window_get_resizable	slib::gtk::getApi_gtk_window_get_resizable()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_resize, void, ,
			GtkWindow   *window,
			gint         width,
			gint         height
		)
		#define gtk_window_resize	slib::gtk::getApi_gtk_window_resize()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_get_size, void, ,
			GtkWindow   *window,
			gint        *width,
			gint        *height
		)
		#define gtk_window_get_size	slib::gtk::getApi_gtk_window_get_size()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_move, void, ,
			GtkWindow   *window,
			gint         x,
			gint         y
		)
		#define gtk_window_move	slib::gtk::getApi_gtk_window_move()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_get_position, void, ,
			GtkWindow   *window,
			gint        *root_x,
			gint        *root_y
		)
		#define gtk_window_get_position	slib::gtk::getApi_gtk_window_get_position()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_set_transient_for, void, ,
			GtkWindow           *window, 
			GtkWindow           *parent
		)
		#define gtk_window_set_transient_for	slib::gtk::getApi_gtk_window_set_transient_for()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_set_decorated, void, ,
			GtkWindow *window,
			gboolean   setting
		)
		#define gtk_window_set_decorated	slib::gtk::getApi_gtk_window_set_decorated()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_set_type_hint, void, ,
			GtkWindow           *window, 
			GdkWindowTypeHint    hint
		)
		#define gtk_window_set_type_hint	slib::gtk::getApi_gtk_window_set_type_hint()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_set_modal, void, ,
			GtkWindow *window,
			gboolean   modal
		)
		#define gtk_window_set_modal	slib::gtk::getApi_gtk_window_set_modal()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_set_title, void, ,
			GtkWindow           *window,
			const gchar         *title
		)
		#define gtk_window_set_title	slib::gtk::getApi_gtk_window_set_title()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_set_screen, void, ,
			GtkWindow	    *window,
			GdkScreen	    *screen
		)
		#define gtk_window_set_screen	slib::gtk::getApi_gtk_window_set_screen()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_set_opacity, void, ,
			GtkWindow           *window, 
			gdouble              opacity
		)
		#define gtk_window_set_opacity	slib::gtk::getApi_gtk_window_set_opacity()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_set_geometry_hints, void, ,
			GtkWindow           *window,
			GtkWidget           *geometry_widget,
			GdkGeometry         *geometry,
			GdkWindowHints       geom_mask
		)
		#define gtk_window_set_geometry_hints	slib::gtk::getApi_gtk_window_set_geometry_hints()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_set_keep_above, void, ,
			GtkWindow *window, gboolean setting
		)
		#define gtk_window_set_keep_above	slib::gtk::getApi_gtk_window_set_keep_above()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_set_deletable, void, ,
			GtkWindow *window,
			gboolean   setting
		)
		#define gtk_window_set_deletable	slib::gtk::getApi_gtk_window_set_deletable()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_is_active, gboolean, ,
			GtkWindow           *window
		)
		#define gtk_window_is_active	slib::gtk::getApi_gtk_window_is_active()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_present, void, ,
			GtkWindow *window
		)
		#define gtk_window_present	slib::gtk::getApi_gtk_window_present()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_iconify, void, ,
			GtkWindow *window
		)
		#define gtk_window_iconify	slib::gtk::getApi_gtk_window_iconify()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_deiconify, void, ,
			GtkWindow *window
		)
		#define gtk_window_deiconify	slib::gtk::getApi_gtk_window_deiconify()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_maximize, void, ,
			GtkWindow *window
		)
		#define gtk_window_maximize	slib::gtk::getApi_gtk_window_maximize()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_unmaximize, void, ,
			GtkWindow *window
		)
		#define gtk_window_unmaximize	slib::gtk::getApi_gtk_window_unmaximize()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_window_fullscreen, void, ,
			GtkWindow *window
		)
		#define gtk_window_fullscreen	slib::gtk::getApi_gtk_window_fullscreen()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_widget_set_sensitive, void, ,
			GtkWidget    *widget,
			gboolean      sensitive
		)
		#define gtk_widget_set_sensitive	slib::gtk::getApi_gtk_widget_set_sensitive()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_widget_set_app_paintable, void, ,
			GtkWidget    *widget,
			gboolean      app_paintable
		)
		#define gtk_widget_set_app_paintable	slib::gtk::getApi_gtk_widget_set_app_paintable()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_widget_set_size_request, void, ,
			GtkWidget           *widget,
			gint                 width,
			gint                 height
		)
		#define gtk_widget_set_size_request	slib::gtk::getApi_gtk_widget_set_size_request()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_widget_set_events, void, ,
			GtkWidget       *widget,
			gint			events
		)
		#define gtk_widget_set_events	slib::gtk::getApi_gtk_widget_set_events()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_widget_get_parent, GtkWidget           *, ,
			GtkWidget    *widget
		)
		#define gtk_widget_get_parent	slib::gtk::getApi_gtk_widget_get_parent()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_widget_get_window, GdkWindow           *, ,
			GtkWidget    *widget
		)
		#define gtk_widget_get_window	slib::gtk::getApi_gtk_widget_get_window()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_widget_show, void, ,
			GtkWidget	       *widget
		)
		#define gtk_widget_show	slib::gtk::getApi_gtk_widget_show()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_widget_hide, void, ,
			GtkWidget	       *widget
		)
		#define gtk_widget_hide	slib::gtk::getApi_gtk_widget_hide()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_widget_realize, void, ,
			GtkWidget	       *widget
		)
		#define gtk_widget_realize	slib::gtk::getApi_gtk_widget_realize()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_widget_grab_focus, void, ,
			GtkWidget           *widget
		)
		#define gtk_widget_grab_focus	slib::gtk::getApi_gtk_widget_grab_focus()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_widget_queue_draw, void, ,
			GtkWidget	       *widget
		)
		#define gtk_widget_queue_draw	slib::gtk::getApi_gtk_widget_queue_draw()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_widget_queue_draw_area, void, ,
			GtkWidget	       *widget,
			gint                 x,
			gint                 y,
			gint                 width,
			gint                 height
		)
		#define gtk_widget_queue_draw_area	slib::gtk::getApi_gtk_widget_queue_draw_area()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_widget_modify_bg, void, ,
			GtkWidget            *widget,
			GtkStateType          state,
			const GdkColor       *color
		)
		#define gtk_widget_modify_bg	slib::gtk::getApi_gtk_widget_modify_bg()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_widget_destroy, void, ,
			GtkWidget	       *widget
		)
		#define gtk_widget_destroy	slib::gtk::getApi_gtk_widget_destroy()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_file_chooser_dialog_new, GtkWidget *, ,
			const gchar          *title,
			GtkWindow            *parent,
			GtkFileChooserAction  action,
			const gchar          *first_button_text,
			...
		)
		#define gtk_file_chooser_dialog_new	slib::gtk::getApi_gtk_file_chooser_dialog_new()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_file_chooser_get_type, GType, ,
			void
		)
		#define gtk_file_chooser_get_type	slib::gtk::getApi_gtk_file_chooser_get_type()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_file_chooser_set_select_multiple, void, ,
			GtkFileChooser       *chooser,
			gboolean              select_multiple
		)
		#define gtk_file_chooser_set_select_multiple	slib::gtk::getApi_gtk_file_chooser_set_select_multiple()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_file_chooser_set_show_hidden, void, ,
			GtkFileChooser       *chooser,
			gboolean              show_hidden
		)
		#define gtk_file_chooser_set_show_hidden	slib::gtk::getApi_gtk_file_chooser_set_show_hidden()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_file_chooser_set_create_folders, void, ,
			GtkFileChooser       *chooser,
			gboolean               create_folders
		)
		#define gtk_file_chooser_set_create_folders	slib::gtk::getApi_gtk_file_chooser_set_create_folders()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_file_chooser_get_uri, gchar *, ,
			GtkFileChooser *chooser
		)
		#define gtk_file_chooser_get_uri	slib::gtk::getApi_gtk_file_chooser_get_uri()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_file_chooser_set_uri, gboolean, ,
			GtkFileChooser *chooser,
			const char     *uri
		)
		#define gtk_file_chooser_set_uri	slib::gtk::getApi_gtk_file_chooser_set_uri()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_file_chooser_get_uris, GSList *, ,
			GtkFileChooser *chooser
		)
		#define gtk_file_chooser_get_uris	slib::gtk::getApi_gtk_file_chooser_get_uris()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_file_chooser_set_current_folder_uri, gboolean, ,
			GtkFileChooser *chooser,
			const gchar    *uri
		)
		#define gtk_file_chooser_set_current_folder_uri	slib::gtk::getApi_gtk_file_chooser_set_current_folder_uri()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_file_chooser_set_current_name, void, ,
			GtkFileChooser *chooser,
			const gchar    *name
		)
		#define gtk_file_chooser_set_current_name	slib::gtk::getApi_gtk_file_chooser_set_current_name()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_file_chooser_add_filter, void, ,
			GtkFileChooser *chooser,
			GtkFileFilter  *filter
		)
		#define gtk_file_chooser_add_filter	slib::gtk::getApi_gtk_file_chooser_add_filter()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_file_filter_new, GtkFileFilter *, ,
			void
		)
		#define gtk_file_filter_new	slib::gtk::getApi_gtk_file_filter_new()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_file_filter_set_name, void, ,
			GtkFileFilter *filter,
			const gchar   *name
		)
		#define gtk_file_filter_set_name	slib::gtk::getApi_gtk_file_filter_set_name()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			gtk_file_filter_add_pattern, void, ,
			GtkFileFilter      *filter,
			const gchar        *pattern
		)
		#define gtk_file_filter_add_pattern	slib::gtk::getApi_gtk_file_filter_add_pattern()
	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
