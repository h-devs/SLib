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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_GTK)

#include "ui_core_common.h"

#include "slib/ui/platform.h"
#include "slib/ui/app.h"
#include "slib/ui/screen.h"
#include "slib/graphics/image.h"

namespace slib
{

	namespace priv
	{
		namespace ui_core
		{

			GtkApplication* g_app = sl_null;

			pthread_t g_threadMain = 0;
			sl_bool g_flagRunningAppLoop = sl_false;

			class ScreenImpl : public Screen
			{
			public:
				UIRect m_region;
				
			public:
				ScreenImpl()
				{
					GdkScreen* screen = gdk_screen_get_default();
					if (screen) {
						m_region.left = 0;
						m_region.top = 0;
						m_region.right = gdk_screen_get_width(screen);
						m_region.bottom = gdk_screen_get_height(screen);
					} else {
						m_region.setZero();
					}
				}

			public:
				UIRect getRegion() override
				{
					return m_region;
				}
				
			};

			static gboolean DispatchUrgentlyCallback(gpointer user_data)
			{
				Callable<void()>* callable = reinterpret_cast<Callable<void()>*>(user_data);
				callable->invoke();
				return sl_false;
			}

			static void DispatchUrgentlyDestroy(gpointer user_data)
			{
				Callable<void()>* callable = reinterpret_cast<Callable<void()>*>(user_data);
				callable->decreaseReference();
			}

			static void EventHandler(GdkEvent* event, gpointer data)
			{
				UIDispatcher::processCallbacks();
				gtk_main_do_event(event);
			}

			static void OnActiveApp(GtkApplication* app, gpointer user_data)
			{
				g_application_hold((GApplication*)app);
				UIApp::dispatchStartToApp();
				gdk_event_handler_set(EventHandler, sl_null, sl_null);
			}

			static void OnPixbufDestroyNotify(guchar* pixels, gpointer data)
			{
				Image* image = (Image*)data;
				if (image) {
					image->decreaseReference();
				}
			}

		}
	}

	using namespace priv::ui_core;

	Ref<Screen> UI::getPrimaryScreen()
	{
		return new ScreenImpl;
	}

	Ref<Screen> UI::getFocusedScreen()
	{
		return UI::getPrimaryScreen();
	}

	List< Ref<Screen> > UI::getScreens()
	{
		return List< Ref<Screen> >::createFromElement(getPrimaryScreen());
	}

	sl_bool UI::isUiThread()
	{
		return g_threadMain == pthread_self();
	}

	void UI::dispatchToUiThread(const Function<void()>& callback, sl_uint32 delayMillis)
	{
		if (callback.isNull()) {
			return;
		}
		if (delayMillis || !(UI::isUiThread())) {
			dispatchToUiThreadUrgently([callback]() {
				dispatchToUiThread(callback);
			}, delayMillis);
			return;
		}
		UIDispatcher::addCallback(callback);
		GdkEvent event;
		Base::zeroMemory(&event, sizeof(event));
		event.type = GDK_NOTHING;
		gdk_event_put(&event);
	}

	void UI::dispatchToUiThreadUrgently(const Function<void()>& callback, sl_uint32 delayMillis)
	{
		if (callback.isNull()) {
			return;
		}
		Callable<void()>* callable = callback.ref.get();
		callable->increaseReference();
		if (delayMillis) {
			g_timeout_add_full(G_PRIORITY_DEFAULT, delayMillis, DispatchUrgentlyCallback, callable, DispatchUrgentlyDestroy);
		} else {
			g_idle_add_full(G_PRIORITY_DEFAULT, DispatchUrgentlyCallback, callable, DispatchUrgentlyDestroy);
		}
	}

	void UI::openUrl(const StringParam& _url)
	{
		if (!(UI::isUiThread())) {
			String url = _url.toString();
			UI::dispatchToUiThread([url]() {
				UI::openUrl(url);
			});
			return;
		}
		StringCstr url(_url);
		GError* error = NULL;
		gtk_show_uri(NULL, url.getData(), GDK_CURRENT_TIME, &error);
	}

	sl_bool UIPlatform::initializeGtk()
	{
		g_thread_init(NULL);
		gdk_threads_init();
		if (gtk_init_check(sl_null, sl_null)) {
			return sl_true;
		}
		return sl_false;
	}

	sl_uint32 UIPlatform::getGtkMajorVersion()
	{
		auto func = gtk::getApi_gtk_get_major_version();
		if (func) {
			return (sl_uint32)(func());
		}
		if (gtk::getApi_gtk_init_check()) {
			return 2;
		}
		return 0;
	}

	sl_uint32 UIPlatform::getGtkMinorVersion()
	{
		auto func = gtk::getApi_gtk_get_minor_version();
		if (func) {
			return (sl_uint32)(func());
		}
		return 0;
	}

	sl_bool UIPlatform::isSupportedGtk(sl_uint32 major)
	{
		if (major >= 3) {
			auto funcMajor = gtk::getApi_gtk_get_major_version();
			if (!funcMajor) {
				return sl_false;
			}
			return (sl_uint32)(funcMajor()) >= major;
		} else {
			if (gtk::getApi_gtk_init_check()) {
				return sl_true;
			} else {
				return sl_false;
			}
		}
	}

	sl_bool UIPlatform::isSupportedGtk(sl_uint32 major, sl_uint32 minor)
	{
		if (major >= 3) {
			auto funcMajor = gtk::getApi_gtk_get_major_version();
			if (!funcMajor) {
				return sl_false;
			}
			sl_uint32 _major = (sl_uint32)(funcMajor());
			if (_major < major) {
				return sl_false;
			}
			if (_major > major) {
				return sl_true;
			}
			return (gtk::getApi_gtk_get_minor_version())() >= minor;
		} else {
			if (gtk::getApi_gtk_init_check()) {
				return sl_true;
			} else {
				return sl_false;
			}
		}
	}

	void UIPlatform::runLoop(sl_uint32 level)
	{
		gtk_main();
	}

	void UIPlatform::quitLoop()
	{
		gtk_main_quit();
	}

	void UIPlatform::runApp()
	{
		g_threadMain = pthread_self();

		GtkApplication* app = getApp();
		if (app) {
			if (!(UI::isQuitingApp())) {
				g_flagRunningAppLoop = sl_true;
				g_signal_connect(app, "activate", G_CALLBACK(OnActiveApp), sl_null);
				gio::getApi_g_application_run()((GApplication*)app, 0, sl_null);
				g_flagRunningAppLoop = sl_false;
				g_app = sl_null;
				g_object_unref(app);
			}
		} else {
			UIPlatform::initializeGtk();
			UIApp::dispatchStartToApp();
			if (!(UI::isQuitingApp())) {
				g_flagRunningAppLoop = sl_true;
				gdk_event_handler_set(EventHandler, sl_null, sl_null);
				gtk_main();
				g_flagRunningAppLoop = sl_false;
			}
		}
		UIApp::dispatchExitToApp();
	}

	void UIPlatform::quitApp()
	{
		if (g_flagRunningAppLoop) {
			if (g_app) {
				g_application_release((GApplication*)g_app);
			} else {
				gtk_main_quit();
			}
		}
	}

	GtkApplication* UIPlatform::getApp()
	{
		static sl_bool flagInit = sl_true;
		if (flagInit) {
			flagInit = sl_false;
			if (isSupportedGtk(3)) {
				StringCstr id;
				Ref<UIApp> app = UIApp::getApp();
				if (app.isNotNull()) {
					id = app->getApplicationId();
				}
				if (gtk::getApi_gtk_get_minor_version()() < 6) {
					if (id.isEmpty()) {
						id = String::join("app.id", String::fromUint64(Time::now().toInt()));
					}
				}
				g_app = gtk::getApi_gtk_application_new()(id.getData(), G_APPLICATION_FLAGS_NONE);
			}
		}
		return g_app;
	}

	void UIPlatform::getGdkColor(const Color& color, GdkColor* _out)
	{
		_out->pixel = 0;
		_out->red = (guint16)(color.r) * 257;
		_out->green = (guint16)(color.g) * 257;
		_out->blue = (guint16)(color.b) * 257;
	}

	GdkPixbuf* UIPlatform::createPixbuf(const Ref<Image>& image)
	{
		if (image.isNotNull()) {
			sl_uint32 width = image->getWidth();
			sl_uint32 height = image->getHeight();
			if (width && height) {
				sl_int32 stride = image->getStride();
				Color* colors = image->getColors();
				GdkPixbuf* ret = gdk_pixbuf_new_from_data((guchar*)(colors), GDK_COLORSPACE_RGB, sl_true, 8, (int)width, (int)height, stride << 2, NULL, image.get());
				if (ret) {
					image->increaseReference();
					return ret;
				}
			}
		}
		return sl_null;
	}

}

#endif
