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

#ifndef CHECKHEADER_SLIB_UI_GTK_PLATFORM
#define CHECKHEADER_SLIB_UI_GTK_PLATFORM

#include "../definition.h"

#if defined(SLIB_UI_IS_GTK)

#include "../platform_common.h"
#include "../event.h"

#include "../dl/linux/gtk.h"
#include "../dl/linux/gdk.h"
#include "../../core/dl/linux/glib.h"

namespace slib
{

	class Screen;
	class Menu;
	class UIEvent;
	
	class SLIB_EXPORT UIPlatform
	{
		PRIV_SLIB_DECLARE_UI_PLATFORM_COMMON_MEMBERS

	public:
		static Ref<ViewInstance> createViewInstance(GtkWidget* handle);
		static void registerViewInstance(GtkWidget* handle, ViewInstance* instance);
		static Ref<ViewInstance> getViewInstance(GtkWidget* handle);
		static Ref<View> getView(GtkWidget* handle);
		static void removeViewInstance(GtkWidget* handle);
		static GtkWidget* getViewHandle(ViewInstance* instance);
		static GtkWidget* getViewHandle(View* view);
		
		static Ref<WindowInstance> createWindowInstance(GtkWindow* handle);
		static void registerWindowInstance(GtkWindow* handle, WindowInstance* instance);
		static Ref<WindowInstance> getWindowInstance(GtkWindow* handle);
		static void removeWindowInstance(GtkWindow* handle);
		static GtkWindow* getWindowHandle(WindowInstance* instance);
		static GtkWindow* getWindowHandle(Window* window);

		static Ref<WindowInstance> getActiveWindowInstance();
		
		static sl_bool initializeGtk();
		static sl_uint32 getGtkMajorVersion();
		static sl_uint32 getGtkMinorVersion();
		static sl_bool isSupportedGtk(sl_uint32 major);
		static sl_bool isSupportedGtk(sl_uint32 major, sl_uint32 minor);

		static void getGdkColor(const Color& color, GdkColor* outGdkColor);
		static void getScreenLocationOfWidget(GtkWidget* widget, sl_ui_len* out_x = sl_null, sl_ui_len* out_y = sl_null);
		static void setWidgetFont(GtkWidget* widget, const Ref<Font>& font);

		static void applyEventModifiers(UIEvent* event, guint state);

		static GtkMenuShell* getMenuHandle(Menu* menu);
		static Ref<Menu> getMenu(GtkMenuShell* hMenu);
		static sl_bool isPopupMenu(Menu* menu);

	};

}

#endif

#endif
