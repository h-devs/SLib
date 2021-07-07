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

#ifndef CHECKHEADER_SLIB_CORE_DL_LINUX_GLIB
#define CHECKHEADER_SLIB_CORE_DL_LINUX_GLIB

#include "../../definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)

#include "../../dl.h"

#include "glib/glib.h"
#include "glib/glib-object.h"
#include "glib/gio/gio.h"

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(glib, "libglib-2.0.so.0")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_free,
			void, ,
			gpointer mem
		)
		#define g_free slib::glib::getApi_g_free()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_slist_free,
			void, ,
			GSList * list
		)
		#define g_slist_free slib::glib::getApi_g_slist_free()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_idle_add,
			guint, ,
			GSourceFunc function,
			gpointer data
		)
		#define g_idle_add slib::glib::getApi_g_idle_add()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_idle_add_full,
			guint, ,
			gint priority,
			GSourceFunc function,
			gpointer data,
			GDestroyNotify notify
		)
		#define g_idle_add_full slib::glib::getApi_g_idle_add_full()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_timeout_add,
			guint, ,
			guint interval,
			GSourceFunc function,
			gpointer data
		)
		#define g_timeout_add slib::glib::getApi_g_timeout_add()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_timeout_add_full,
			guint, ,
			gint priority,
			guint interval,
			GSourceFunc function,
			gpointer data,
			GDestroyNotify notify
		)
		#define g_timeout_add_full slib::glib::getApi_g_timeout_add_full()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_sequence_iter_is_end,
			gboolean, ,
			GSequenceIter *iter
		)
		#define g_sequence_iter_is_end slib::glib::getApi_g_sequence_iter_is_end()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_sequence_iter_get_position,
			gint, ,
			GSequenceIter *iter
		)
		#define g_sequence_iter_get_position slib::glib::getApi_g_sequence_iter_get_position()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_list_length,
			guint, ,
			GList *list
		)
		#define g_list_length slib::glib::getApi_g_list_length()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_malloc,
			gpointer, ,
			gsize n_bytes
		)
		#define g_malloc slib::glib::getApi_g_malloc()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_main_context_wakeup,
			void, ,
			GMainContext *context
		)
		#define g_main_context_wakeup slib::glib::getApi_g_main_context_wakeup()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_intern_static_string,
			const gchar*, ,
			const gchar *string
		)
		#define g_intern_static_string slib::glib::getApi_g_intern_static_string()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_variant_new,
			GVariant*, ,
			const gchar *format_string,
			...
		)
		#define g_variant_new slib::glib::getApi_g_variant_new()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_variant_unref,
			void, ,
			GVariant *value
		)
		#define g_variant_unref slib::glib::getApi_g_variant_unref()
	SLIB_IMPORT_LIBRARY_END

	SLIB_IMPORT_LIBRARY_BEGIN(gobject, "libgobject-2.0.so.0")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_object_ref,
			gpointer, ,
			gpointer object
		)
		#define g_object_ref slib::gobject::getApi_g_object_ref()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_object_ref_sink,
			gpointer, ,
			gpointer object
		)
		#define g_object_ref_sink slib::gobject::getApi_g_object_ref_sink()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_object_unref,
			void, ,
			gpointer object
		)
		#define g_object_unref slib::gobject::getApi_g_object_unref()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_signal_connect_data,
			gulong, ,
			gpointer instance,
			const gchar * detailed_signal,
			GCallback c_handler,
			gpointer data,
			GClosureNotify destroy_data,
			GConnectFlags connect_flags
		)
		#define g_signal_connect_data slib::gobject::getApi_g_signal_connect_data()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_type_check_instance_is_a,
			gboolean, ,
			GTypeInstance *instance,
			GType iface_type
		)
		#define g_type_check_instance_is_a slib::gobject::getApi_g_type_check_instance_is_a()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_type_check_instance_cast,
			GTypeInstance*, ,
			GTypeInstance *instance,
			GType iface_type
		)
		#define g_type_check_instance_cast slib::gobject::getApi_g_type_check_instance_cast()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_type_class_peek_parent,
			gpointer, ,
			gpointer g_class
		)
		#define g_type_class_peek_parent slib::gobject::getApi_g_type_class_peek_parent()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_value_init,
			GValue*, ,
			GValue *value,
			GType g_type
		)
		#define g_value_init slib::gobject::getApi_g_value_init()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_value_set_string,
			void, ,
			GValue *value,
			const gchar *v_string
		)
		#define g_value_set_string slib::gobject::getApi_g_value_set_string()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_value_set_boolean,
			void, ,
			GValue *value,
			const gboolean v_boolean
		)
		#define g_value_set_boolean slib::gobject::getApi_g_value_set_boolean()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_object_set_data,
			void, ,
			GObject *object,
			const gchar	*key,
			gpointer data
		)
		#define g_object_set_data slib::gobject::getApi_g_object_set_data()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_object_get_data,
			gpointer, ,
			GObject *object,
			const gchar	*key
		)
		#define g_object_get_data slib::gobject::getApi_g_object_get_data()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_object_set_property,
			void, ,
			GObject        *object,
			const gchar    *property_name,
			const GValue   *value
		)
		#define g_object_set_property slib::gobject::getApi_g_object_set_property()
	    SLIB_IMPORT_LIBRARY_FUNCTION(
	        g_type_class_adjust_private_offset,
	        void, ,
			gpointer g_class,
			gint *private_size_or_offset
	    )
        #define g_type_class_adjust_private_offset slib::gobject::getApi_g_type_class_adjust_private_offset()
	    SLIB_IMPORT_LIBRARY_FUNCTION(
	        g_type_register_static_simple,
	        GType, ,
			GType parent_type,
			const gchar *type_name,
			guint class_size,
			GClassInitFunc class_init,
			guint instance_size,
			GInstanceInitFunc instance_init,
			GTypeFlags flags
	    )
        #define g_type_register_static_simple slib::gobject::getApi_g_type_register_static_simple()
	    SLIB_IMPORT_LIBRARY_FUNCTION(
	        g_type_add_interface_static,
	        void, ,
			GType instance_type,
			GType interface_type,
			const GInterfaceInfo *info
	    )
        #define g_type_add_interface_static slib::gobject::getApi_g_type_add_interface_static()
	    SLIB_IMPORT_LIBRARY_FUNCTION(
	        g_object_new,
	        gpointer, ,
			GType object_type,
			const gchar *first_property_name,
	        ...
	    )
        #define g_object_new slib::gobject::getApi_g_object_new()
	SLIB_IMPORT_LIBRARY_END

	SLIB_IMPORT_LIBRARY_BEGIN(gthread, "libgthread-2.0.so.0")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_thread_init,
			void, ,
			gpointer vtable
		)
		#define g_thread_init slib::gthread::getApi_g_thread_init()
	    SLIB_IMPORT_LIBRARY_FUNCTION(
	        g_once_init_enter,
	        gboolean, ,
			volatile void *location
	    )
        #ifdef g_once_init_enter
        #	undef g_once_init_enter
        #endif
        #define g_once_init_enter slib::gthread::getApi_g_once_init_enter()
	    SLIB_IMPORT_LIBRARY_FUNCTION(
	        g_once_init_leave,
	        void, ,
			volatile void *location,
			gsize result
	    )
        #ifdef g_once_init_leave
        #	undef g_once_init_leave
        #endif
        #define g_once_init_leave slib::gthread::getApi_g_once_init_leave()
	SLIB_IMPORT_LIBRARY_END

	SLIB_IMPORT_LIBRARY_BEGIN(gio, "libgio-2.0.so.0")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_application_get_default,
			GApplication *, ,
			void
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_application_new,
			GApplication *, ,
			const gchar* application_id,
			GApplicationFlags flags
		)
		#define g_application_new slib::gio::getApi_g_application_new()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_application_get_is_registered,
			gboolean, ,
			GApplication*
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_application_register,
			gboolean, ,
			GApplication*,
			GCancellable *cancellable,
			GError **error
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_application_get_dbus_connection,
			GDBusConnection *, ,
			GApplication*
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_dbus_connection_call_sync,
			GVariant *, ,
			GDBusConnection    *connection,
			const gchar        *bus_name,
			const gchar        *object_path,
			const gchar        *interface_name,
			const gchar        *method_name,
			GVariant           *parameters,
			const GVariantType *reply_type,
			GDBusCallFlags      flags,
			gint                timeout_msec,
			GCancellable       *cancellable,
			GError            **error
		)
	SLIB_IMPORT_LIBRARY_END

}
#endif

#endif


