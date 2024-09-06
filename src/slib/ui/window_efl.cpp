/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#if defined(SLIB_UI_IS_EFL)

#include "window.h"

#include "slib/ui/mobile_app.h"
#include "slib/core/safe_static.h"
#include "slib/platform.h"

#include "view_efl.h"

#include <app.h>
#include <Elementary.h>
#include <efl_extension.h>

namespace slib
{

	namespace
	{
		static volatile sl_int32 g_nCountActiveWindows = 0;

		class EFL_WindowInstance;
		SLIB_GLOBAL_ZERO_INITIALIZED(Ref<EFL_WindowInstance>, g_windowMain)

		class EFL_WindowInstance : public WindowInstance
		{
		public:
			Evas_Object* m_handle;
			AtomicRef<ViewInstance> m_viewContent;

		public:
			EFL_WindowInstance()
			{
				m_handle = sl_null;
			}

			~EFL_WindowInstance()
			{
				_release();
			}

		public:
			static Ref<EFL_WindowInstance> create(Evas_Object* window)
			{
				if (window) {
					Ref<EFL_WindowInstance> ret = new EFL_WindowInstance();
					if (ret.isNotNull()) {
						ret->m_handle = window;
						Ref<ViewInstance> content = UIPlatform::createViewInstance(EFL_ViewType::Window, window, sl_false);
						if (content.isNotNull()) {
							content->setWindowContent(sl_true);
							ret->m_viewContent = content;
						}

						Base::interlockedIncrement32(&g_nCountActiveWindows);
						evas_object_smart_callback_add(window, "delete,request", _ui_win_delete_request_cb, sl_null);
						evas_object_smart_callback_add(window, "wm,rotation,changed", _ui_win_rotate_cb, NULL);
						eext_object_event_callback_add(window, EEXT_CALLBACK_BACK, _ui_win_back_cb, sl_null);

						evas_object_show(window);

						if (g_windowMain.isNull()) {
							g_windowMain = ret;
						}

						UIPlatform::registerWindowInstance(window, ret.get());

						return ret;
					}
				}
				return sl_null;
			}

			static void _ui_win_delete_request_cb(void* data, Evas_Object* win, void* event_info)
			{
				Ref<WindowInstance> instance = UIPlatform::getWindowInstance(win);
				if (instance.isNotNull()) {
					(static_cast<EFL_WindowInstance*>(instance.get()))->m_handle = sl_null;
				}
				UIPlatform::removeWindowInstance(win);
				sl_int32 n = Base::interlockedDecrement32(&g_nCountActiveWindows);
				if (n <= 0) {
					ui_app_exit();
				}
			}

			static void _ui_win_rotate_cb(void* data, Evas_Object* win, void* event_info)
			{
				Ref<WindowInstance> instance = UIPlatform::getWindowInstance(win);
				if (instance.isNotNull()) {
					UISize size = UI::getScreenSize();
					instance->onResize(size.x, size.y);
				}
			}

			static void _ui_win_back_cb(void* data, Evas_Object* win, void* event_info)
			{
				if (!(MobileApp::Current::invokePressBack()) {
					elm_win_lower(win);
				}
			}

			static Ref<WindowInstance> create(Window* window)
			{
				Evas_Object* win = elm_win_util_standard_add("", "");
				if (win) {

					List<ScreenOrientation> orientations = MobileApp::getAvailableScreenOrientations();
					if (orientations.isNotNull()) {
						if (elm_win_wm_rotation_supported_get(win)) {
							elm_win_wm_rotation_available_rotations_set(win, (int*)(orientations.getData()), (unsigned int)(orientations.getCount()));
						}
					}

					if (!(window->isFullScreen())) {
						UIRect rect = MakeWindowFrame(window);
						/*
						* Following move&resize code has no effect because Tizen policy fills the window in the screen.
						* Just left for the further update.
						*/
						evas_object_move(win, (Evas_Coord)(rect.left), (Evas_Coord)(rect.top));
						evas_object_resize(win, (Evas_Coord)(rect.getWidth()), (Evas_Coord)(rect.getHeight()));
					}

					Ref<EFL_WindowInstance> ret = create(win);
					if (ret.isNotNull()) {
						elm_win_autodel_set(win, EINA_TRUE);
						return ret;
					}

					evas_object_del(win);

				}
				return sl_null;
			}

			static void _release_handle(Evas_Object* handle)
			{
				elm_win_lower(handle);
				evas_object_del(handle);
			}

			void _release()
			{
				Evas_Object* handle = m_handle;
				if (handle) {
					UIPlatform::removeWindowInstance(handle);
					if (UI::isUiThread()) {
						_release_handle(handle);
					} else {
						UI::dispatchToUiThread(Function<void()>::bind(&_release_handle, handle));
					}
				}
				m_viewContent.setNull();
				m_handle = sl_null;
			}

		public:
			void* getHandle() override
			{
				return (void*)m_handle;
			}

			void close() override
			{
				Evas_Object* handle = m_handle;
				if (handle) {
					if (!(UI::isUiThread())) {
						UI::dispatchToUiThread(SLIB_FUNCTION_WEAKREF(this, close));
						return;
					}
					UIPlatform::removeWindowInstance(handle);
					elm_win_lower(handle);
					evas_object_del(handle);
				}
				m_handle = sl_null;
				m_viewContent.setNull();
			}

			sl_bool isClosed() override
			{
				return m_handle == sl_null;
			}

			void setParentHandle(void* parent) override
			{
			}

			Ref<ViewInstance> getContentView() override
			{
				return m_viewContent;
			}

			sl_bool getFrame(UIRect& _out) override
			{
				_out = UI::getScreenBounds();
				return sl_true;
			}

			void setFrame(const UIRect& _frame) override
			{
			}

			void activate() override
			{
				Evas_Object* handle = m_handle;
				if (handle) {
					elm_win_raise(handle);
				}
			}

			void setVisible(sl_bool flag) override
			{
				Evas_Object* handle = m_handle;
				if (handle) {
					if (flag) {
						evas_object_show(handle);
					} else {
						evas_object_hide(handle);
					}
				}
			}

			void doPostCreate() override
			{
				UISize sizeClient = getClientSize();
				onResize(sizeClient.x, sizeClient.y);
			}

		};
	}

	Ref<WindowInstance> Window::createWindowInstance()
	{
		return EFL_WindowInstance::create(this);
	}


	Ref<WindowInstance> UIPlatform::createWindowInstance(Evas_Object* handle)
	{
		Ref<WindowInstance> ret = UIPlatform::_getWindowInstance(handle);
		if (ret.isNotNull()) {
			return ret;
		}
		return EFL_WindowInstance::create(handle);
	}

	void UIPlatform::registerWindowInstance(Evas_Object* handle, WindowInstance* instance)
	{
		UIPlatform::_registerWindowInstance(handle, instance);
	}

	Ref<WindowInstance> UIPlatform::getWindowInstance(Evas_Object* handle)
	{
		return UIPlatform::_getWindowInstance(handle);
	}

	void UIPlatform::removeWindowInstance(Evas_Object* handle)
	{
		UIPlatform::_removeWindowInstance(handle);
	}

	Evas_Object* UIPlatform::getWindowHandle(WindowInstance* _instance)
	{
		EFL_WindowInstance* instance = (EFL_WindowInstance*)_instance;
		if (instance) {
			return instance->m_handle;
		} else {
			return sl_null;
		}
	}

	Evas_Object* UIPlatform::getWindowHandle(Window* window)
	{
		if (window) {
			Ref<WindowInstance> _instance = window->getWindowInstance();
			if (_instance.isNotNull()) {
				EFL_WindowInstance* instance = (EFL_WindowInstance*)(_instance.get());
				return instance->m_handle;
			}
		}
		return sl_null;
	}

	Evas_Object* UIPlatform::getMainWindow()
	{
		if (g_windowMain.isNotNull()) {
			return g_windowMain->m_handle;
		}
		return sl_null;
	}

}

#endif
