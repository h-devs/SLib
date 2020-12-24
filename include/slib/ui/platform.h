/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_PLATFORM
#define CHECKHEADER_SLIB_UI_PLATFORM

#include "definition.h"

#include "../core/ref.h"
#include "../core/function.h"
#include "../graphics/platform.h"

#include "event.h"

#if defined(SLIB_UI_IS_WIN32)
#	include "../core/platform_windows.h"
#endif
#if defined(SLIB_UI_IS_MACOS) || defined(SLIB_UI_IS_IOS)
#	include "../core/platform_apple.h"
#	include <CoreText/CoreText.h>
#endif
#if defined(SLIB_UI_IS_ANDROID)
#	include "../core/platform_android.h"
#endif
#if defined(SLIB_UI_IS_GTK)
#	include "gtk/gtk.h"
#	include "../core/dl_linux_glib.h"
#	include "dl_linux_gtk.h"
#	include "dl_linux_gdk.h"
#endif
#if defined(SLIB_UI_IS_EFL)
enum class EFL_ViewType
{
	Generic = 0,
	Grid = 1,
	OpenGL = 2,
	Window = 3
};
#	include <Evas.h>
#endif

namespace slib
{

	class Screen;
	class ViewInstance;
	class View;
	class WindowInstance;
	class Window;
	class Cursor;
	class Menu;
	class MenuItem;
	class PushNotificationMessage;
	class UIEvent;
	
	class SLIB_EXPORT UIPlatform
	{
	public:
		static void runLoop(sl_uint32 level);
		static void quitLoop();
		static void runApp();
		static void quitApp();
		
#if defined(SLIB_UI_IS_WIN32)
		static Ref<ViewInstance> createViewInstance(HWND hWnd, sl_bool flagDestroyOnRelease = sl_true);
		static void registerViewInstance(HWND hWnd, ViewInstance* instance);
		static Ref<ViewInstance> getViewInstance(HWND hWnd);
		static void removeViewInstance(HWND hWnd);
		static HWND getViewHandle(ViewInstance* instance);
		static HWND getViewHandle(View* view);
		
		static Ref<WindowInstance> createWindowInstance(HWND hWnd, sl_bool flagDestroyOnRelease = sl_true);
		static void registerWindowInstance(HWND hWnd, WindowInstance* instance);
		static Ref<WindowInstance> getWindowInstance(HWND hWnd);
		static void removeWindowInstance(HWND hWnd);
		static HWND getWindowHandle(WindowInstance* instance);
		static HWND getWindowHandle(Window* window);
		
		static Ref<Cursor> createCursor(HCURSOR hCursor, sl_bool flagDestroyOnRelease = sl_true);
		static HCURSOR getCursorHandle(const Ref<Cursor>& cursor);
		
		static HMENU getMenuHandle(const Ref<Menu>& menu);
		static Ref<Menu> getMenu(HMENU hMenu);

		static void applyEventModifiers(UIEvent* ev);

#elif defined(SLIB_UI_IS_MACOS)
#	if defined(__OBJC__)
		static Ref<ViewInstance> createViewInstance(NSView* handle);
		static void registerViewInstance(NSView* handle, ViewInstance* instance);
		static Ref<ViewInstance> getViewInstance(NSView* handle);
		static void removeViewInstance(NSView* handle);
		static NSView* getViewHandle(ViewInstance* instance);
		static NSView* getViewHandle(View* view);
		static sl_bool measureNativeWidgetFittingSize(View* view, UISize& _out);
		static sl_bool measureNativeWidgetFittingSize(ViewInstance* instance, UISize& _out);

		static Ref<WindowInstance> createWindowInstance(NSWindow* handle);
		static void registerWindowInstance(NSWindow* handle, WindowInstance* instance);
		static Ref<WindowInstance> getWindowInstance(NSWindow* handle);
		static void removeWindowInstance(NSWindow* handle);
		static NSWindow* getWindowHandle(WindowInstance* instance);
		static NSWindow* getWindowHandle(Window* window);
		static NSWindow* getMainWindow();
		static NSWindow* getKeyWindow();
		
		static Ref<Screen> createScreen(NSScreen* handle);
		static NSScreen* getScreenHandle(Screen* screen);
		static float getDpiForScreen(NSScreen* handle);
		
		static Ref<Cursor> createCursor(NSCursor* handle);
		static NSCursor* getCursorHandle(const Ref<Cursor>& cursor);
		
		static NSMenu* getMenuHandle(const Ref<Menu>& menu);
		static NSMenuItem* getMenuItemHandle(const Ref<MenuItem>& menu);
		
		static NSString* getKeyEquivalent(const KeycodeAndModifiers& km, NSUInteger& outMask);
		static void applyEventModifiers(UIEvent* ev, NSEvent* event);
		
		static void registerDidFinishLaunchingCallback(const Function<void(NSNotification*)>& callback);
#	endif
		
#elif defined(SLIB_UI_IS_IOS)
#	if defined(__OBJC__)
		static Ref<ViewInstance> createViewInstance(UIView* handle);
		static void registerViewInstance(UIView* handle, ViewInstance* instance);
		static Ref<ViewInstance> getViewInstance(UIView* handle);
		static void removeViewInstance(UIView* handle);
		static UIView* getViewHandle(ViewInstance* instance);
		static UIView* getViewHandle(View* view);
		
		static Ref<WindowInstance> createWindowInstance(UIView* handle);
		static void registerWindowInstance(UIView* handle, WindowInstance* instance);
		static Ref<WindowInstance> getWindowInstance(UIView* handle);
		static void removeWindowInstance(UIView* handle);
		static UIView* getWindowHandle(WindowInstance* instance);
		static UIWindow* getMainWindow();
		static UIWindow* getKeyWindow();

		static Ref<Screen> createScreen(UIScreen* handle);
		static UIScreen* getScreenHandle(Screen* screen);
		
		static UIViewController* getCurrentViewController();
		static UIViewController* getCurrentViewController(const Ref<Window>& parentWindow);
		static UIView* findFirstResponder(UIView* rootView);
		
		static CGFloat getGlobalScaleFactor();
		static void setGlobalScaleFactor(CGFloat factor);
				
		static void registerDidFinishLaunchingCallback(const Function<void(NSDictionary*)>& callback);
		static void registerDidRegisterForRemoteNotifications(const Function<void(NSData*, NSError*)>& callback);
		static void registerDidReceiveRemoteNotificationCallback(const Function<void(NSDictionary*)>& callback);
		static void registerOpenUrlCallback(const Function<BOOL(NSURL*, NSDictionary*)>& callback);

		static sl_bool parseRemoteNotificationInfo(NSDictionary* userInfo, PushNotificationMessage& _out);
#	endif
		
#elif defined(SLIB_UI_IS_ANDROID)
		static Ref<ViewInstance> createViewInstance(jobject jhandle);
		static void registerViewInstance(jobject jhandle, ViewInstance* instance);
		static Ref<ViewInstance> getViewInstance(jobject jhandle);
		static void removeViewInstance(jobject jhandle);
		static jobject getViewHandle(ViewInstance* instance);
		static jobject getViewHandle(View* view);
		
		static Ref<WindowInstance> createWindowInstance(jobject window);
		static void registerWindowInstance(jobject window, WindowInstance* instance);
		static Ref<WindowInstance> getWindowInstance(jobject window);
		static void removeWindowInstance(jobject window);
		static jobject getWindowHandle(WindowInstance* instance);

#elif defined(SLIB_UI_IS_GTK)
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

		static Ref<Screen> createScreen(GdkScreen* handle);
		static GdkScreen* getScreenHandle(Screen* screen);

		static Ref<WindowInstance> getActiveWindowInstance();
		
		static sl_bool initializeGtk();
		static void getGdkColor(const Color& color, GdkColor* outGdkColor);
		static void getScreenLocationOfWidget(GtkWidget* widget, sl_ui_len* out_x = sl_null, sl_ui_len* out_y = sl_null);
		static void setWidgetFont(GtkWidget* widget, const Ref<Font>& font);

		static void applyEventModifiers(UIEvent* event, guint state);

		static GtkMenuShell* getMenuHandle(const Ref<Menu>& menu);
		static Ref<Menu> getMenu(GtkMenuShell* hMenu);
		static sl_bool isPopupMenu(const Ref<Menu>& menu);

#elif defined(SLIB_UI_IS_EFL)
		static Ref<ViewInstance> createViewInstance(EFL_ViewType type, Evas_Object* handle, sl_bool flagFreeOnRelease = sl_true);
		static void registerViewInstance(Evas_Object* handle, ViewInstance* instance);
		static Ref<ViewInstance> getViewInstance(Evas_Object* handle);
		static Ref<View> getView(Evas_Object* handle);
		static void removeViewInstance(Evas_Object* handle);
		static Evas_Object* getViewHandle(ViewInstance* instance);
		static Evas_Object* getViewHandle(View* view);

		static Ref<WindowInstance> createWindowInstance(Evas_Object* handle);
		static void registerWindowInstance(Evas_Object* handle, WindowInstance* instance);
		static Ref<WindowInstance> getWindowInstance(Evas_Object* handle);
		static void removeWindowInstance(Evas_Object* handle);
		static Evas_Object* getWindowHandle(WindowInstance* instance);
		static Evas_Object* getWindowHandle(Window* window);

		static Evas_Object* getMainWindow();
		
#endif
		
	private:
		static Ref<ViewInstance> _getViewInstance(const void* handle);
		static void _registerViewInstance(const void* handle, ViewInstance* instance);
		static void _removeViewInstance(const void* handle);
		static Ref<WindowInstance> _getWindowInstance(const void* handle);
		static void _registerWindowInstance(const void* handle, WindowInstance* instance);
		static void _removeWindowInstance(const void* handle);
		static List< Ref<WindowInstance> > _getAllWindowInstances();
	};

}

#if defined(SLIB_UI_IS_WIN32)

#define SLIB_UI_MESSAGE_BEGIN 0x7100
#define SLIB_UI_MESSAGE_QUIT_LOOP SLIB_UI_MESSAGE_BEGIN
#define SLIB_UI_MESSAGE_CLOSE (SLIB_UI_MESSAGE_BEGIN+1)
#define SLIB_UI_MESSAGE_DISPATCH (SLIB_UI_MESSAGE_BEGIN+2)
#define SLIB_UI_MESSAGE_DISPATCH_DELAYED (SLIB_UI_MESSAGE_BEGIN+3)
#define SLIB_UI_MESSAGE_CUSTOM_MSGBOX (SLIB_UI_MESSAGE_BEGIN+4)
#define SLIB_UI_MESSAGE_SYSTEM_TRAY_ICON (SLIB_UI_MESSAGE_BEGIN+5)

#endif

#endif
