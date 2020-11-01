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

#include "definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX) && defined(SLIB_PLATFORM_IS_DESKTOP)

#include "dl.h"

#include "glib/glib.h"
#include "glib/glib-object.h"

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
			GSequenceIter			*iter
		)
		#define g_sequence_iter_is_end slib::glib::getApi_g_sequence_iter_is_end()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_sequence_iter_get_position,
			gint, ,
			GSequenceIter			*iter
		)
		#define g_sequence_iter_get_position slib::glib::getApi_g_sequence_iter_get_position()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_list_length,
			guint, ,
			GList  *list
		)
		#define g_list_length slib::glib::getApi_g_list_length()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_malloc,
			gpointer, ,
			gsize	 n_bytes
		)
		#define g_malloc slib::glib::getApi_g_malloc()
	SLIB_IMPORT_LIBRARY_END

	SLIB_IMPORT_LIBRARY_BEGIN(gobject, "libgobject-2.0.so.0")
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
	SLIB_IMPORT_LIBRARY_END

	SLIB_IMPORT_LIBRARY_BEGIN(gthread, "libgthread-2.0.so.0")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			g_thread_init,
			void, ,
			gpointer vtable
		)
		#define g_thread_init slib::gthread::getApi_g_thread_init()
	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
