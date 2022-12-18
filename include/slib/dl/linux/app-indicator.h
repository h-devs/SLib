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

#ifndef CHECKHEADER_SLIB_DL_LINUX_APP_INDICATOR
#define CHECKHEADER_SLIB_DL_LINUX_APP_INDICATOR

#include "../dl.h"

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)

#include "app-indicator/app-indicator.h"

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(app_indicator, "libappindicator3.so.1")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			app_indicator_new,
			AppIndicator*, ,
			const gchar *id,
			const gchar *icon_name,
			AppIndicatorCategory category
		)
		#define app_indicator_new slib::app_indicator::getApi_app_indicator_new()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			app_indicator_set_icon,
			void, ,
			AppIndicator *self,
			const gchar *icon_name
		)
		#define app_indicator_set_icon slib::app_indicator::getApi_app_indicator_set_icon()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			app_indicator_set_menu,
			void, ,
			AppIndicator *self,
			GtkMenu *menu
		)
		#define app_indicator_set_menu slib::app_indicator::getApi_app_indicator_set_menu()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			app_indicator_set_status,
			void, ,
			AppIndicator *self,
			AppIndicatorStatus status
		)
		#define app_indicator_set_status slib::app_indicator::getApi_app_indicator_set_status()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			app_indicator_set_title,
			void, ,
			AppIndicator *self,
			const gchar *title
		)
		#define app_indicator_set_title slib::app_indicator::getApi_app_indicator_set_title()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			app_indicator_set_icon_full,
			void, ,
			AppIndicator *self,
			const gchar *icon_name,
			const gchar *icon_desc
		)
		#define app_indicator_set_icon_full slib::app_indicator::getApi_app_indicator_set_icon_full()

	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
