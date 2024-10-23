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

#if defined(SLIB_UI_IS_GTK)

#include "window.h"

#include "slib/ui/screen.h"
#include "slib/ui/core.h"
#include "slib/graphics/image.h"

#include "view_gtk.h"

namespace slib
{

	namespace
	{
		static void SetWindowSize(GtkWindow* handle, sl_bool flagResizable, sl_ui_len width, sl_ui_len height)
		{
			if (flagResizable) {
				gtk_window_resize(handle, (gint)width, (gint)height);
			} else {
				gtk_window_set_default_size(handle, (gint)width, (gint)height);
			}
		}

		class GTK_WindowInstance : public WindowInstance
		{
		public:
			GtkWindow* m_handle;
			GtkWidget* m_widgetMenu;
			GtkWidget* m_widgetContent;
			GtkWidget* m_widgetContentBox;
			AtomicRef<ViewInstance> m_viewContent;

			sl_bool m_flagResizable;

			sl_bool m_flagClosed;
			sl_bool m_flagMinimized;
			sl_bool m_flagMaximized;

			sl_bool m_flagFirstResize;
			UIPoint m_location;
			UISize m_size;

		public:
			GTK_WindowInstance()
			{
				m_handle = sl_null;
				m_widgetMenu = sl_null;
				m_widgetContent = sl_null;
				m_widgetContentBox = sl_null;

				m_flagResizable = sl_true;

				m_flagClosed = sl_true;
				m_flagMinimized = sl_false;
				m_flagMaximized = sl_false;

				m_flagFirstResize = sl_true;
			}

			~GTK_WindowInstance()
			{
				_release();
			}

		public:
			static Ref<GTK_WindowInstance> create(GtkWindow* handle)
			{
				if (handle) {
					Ref<GTK_WindowInstance> ret = new GTK_WindowInstance();
					if (ret.isNotNull()) {
						ret->_init(handle);
						return ret;
					}
				}
				return sl_null;
			}

			void _init(GtkWindow* handle)
			{
				g_object_ref_sink(handle);

				m_handle = handle;
				m_flagClosed = sl_false;

				UIPlatform::registerWindowInstance(handle, this);

				g_signal_connect(handle, "destroy", G_CALLBACK(_callback_destroy_cb), NULL);
				g_signal_connect(handle, "delete-event", G_CALLBACK(_callback_close_cb), NULL);
				g_signal_connect(handle, "window-state-event", G_CALLBACK(_callback_window_state_cb), NULL);
				g_signal_connect(handle, "configure-event", G_CALLBACK(_callback_configure_event_cb), NULL);
				g_signal_connect(handle, "notify::is-active", G_CALLBACK(_callback_notify_is_active_cb), NULL);

			}

			static void _release_handle(GtkWindow* handle)
			{
				gtk_widget_destroy((GtkWidget*)handle);
				g_object_unref(handle);
			}

			void _release()
			{
				GtkWindow* handle = m_handle;
				if (handle) {
					m_handle = sl_null;
					if (m_flagClosed) {
						g_object_unref(handle);
					} else {
						if (UI::isUiThread()) {
							_release_handle(handle);
						} else {
							UI::dispatchToUiThread(Function<void()>::bind(&_release_handle, handle));
						}
					}
					UIPlatform::removeWindowInstance(handle);
				}
				m_viewContent.setNull();
				m_flagClosed = sl_true;
			}

			static Ref<WindowInstance> create(Window* window)
			{
				GtkWindow* handle = (GtkWindow*)(gtk_window_new(GTK_WINDOW_TOPLEVEL));
				if (!handle) {
					return sl_null;
				}

				Ref<WindowInstance> parent;
				GtkWindow* hParent = (GtkWindow*)(window->getParentHandle(parent));
				if (hParent) {
					gtk_window_set_transient_for(handle, hParent);
				}

				if (window->isBorderless() || window->isFullScreen() || !(window->isTitleBarVisible())) {
					gtk_window_set_decorated(handle, sl_false);
				}
				if (window->isDialog()) {
					gtk_window_set_type_hint(handle, GDK_WINDOW_TYPE_HINT_DIALOG);
				}
				if (window->isModal()) {
					gtk_window_set_modal(handle, sl_true);
				}
				if (window->isFullScreen()) {
					gtk_window_fullscreen(handle);
				}
				if (!(window->isCloseButtonEnabled())) {
					gtk_window_set_deletable(handle, 0);
				}
				sl_real alpha = window->getAlpha();
				if (alpha < 0.9999f) {
					if (alpha < 0) {
						alpha = 0;
					}
					gtk_window_set_opacity(handle, alpha);
				}

				StringCstr title = window->getTitle();
				gtk_window_set_title(handle, title.getData());

				Ref<Drawable> icon = window->getIcon();
				if (icon.isNotNull()) {
					GdkPixbuf* pixbuf = UIPlatform::createPixbuf(icon->toImage());
					if (pixbuf) {
						gtk_window_set_icon(handle, pixbuf);
					}
				}

				Ref<GTK_WindowInstance> ret = create(handle);
				if (ret.isNull()) {
					g_object_ref_sink(handle);
					g_object_unref(handle);
					return sl_null;
				}

				UIRect frameWindow = MakeWindowFrame(window);
				UISize size = frameWindow.getSize();
				if (size.x < 1) {
					size.x = 1;
				}
				if (size.y < 1) {
					size.y = 1;
				}
				ret->m_size = size;
				if (window->isResizable()) {
					ret->m_flagResizable = sl_true;
					SetWindowSize(handle, sl_true, size.x, size.y);
				} else {
					ret->m_flagResizable = sl_false;
					gtk_window_set_resizable(handle, sl_false);
					SetWindowSize(handle, sl_false, size.x, size.y);
				}
				ret->m_location = frameWindow.getLocation();
				gtk_window_move(handle, (gint)(frameWindow.left), (gint)(frameWindow.top));

				if (UIPlatform::isSupportedGtk(3)) {
					UIPlatform::setWidgetBackgroundColor((GtkWidget*)handle, window->getBackgroundColor());
				}

				GtkWidget* contentBox = gtk_event_box_new();
				if(contentBox){
					gtk_widget_show(contentBox);
					GtkWidget* contentWidget = gtk_fixed_new();
					if (contentWidget) {
						gtk_container_add((GtkContainer*)contentBox, contentWidget);
						gtk_widget_set_can_focus(contentWidget, 1);
						gtk_widget_show(contentWidget);
						Ref<PlatformViewInstance> content = PlatformViewInstance::create<PlatformViewInstance>(contentWidget);
						if (content.isNotNull()) {
							content->setWindowContent(sl_true);
							content->installEventsWithDrawing();
							ret->m_viewContent = Move(content);
							ret->m_widgetContent = contentWidget;
							ret->m_widgetContentBox = contentBox;
							if (!(UIPlatform::isSupportedGtk(3))) {
								UIPlatform::setWidgetBackgroundColor(contentWidget, window->getBackgroundColor());
							}
						}
						g_signal_connect(handle, "key-press-event", G_CALLBACK(_callback_key_event), contentWidget);
						g_signal_connect(handle, "key-release-event", G_CALLBACK(_callback_key_event), contentWidget);
					}
				}

				GtkMenuShell* hMenu = sl_null;
				Ref<Menu> menu = window->getMenu();
				if (!(UIPlatform::isPopupMenu(menu.get()))) {
					hMenu = UIPlatform::getMenuHandle(menu.get());
				}
				if (hMenu) {
					GtkWidget* box = gtk_vbox_new(0, 0);
					if(box){
						gtk_widget_show(box);
						gtk_box_pack_start((GtkBox*)box, (GtkWidget*)hMenu, 0, 0, 0);
						gtk_box_pack_start((GtkBox*)box, contentBox, 1, 1, 0);
						gtk_widget_set_size_request(box, 1, 1);
						gtk_container_add((GtkContainer*)handle, box);
						ret->m_widgetMenu = (GtkWidget*)hMenu;
					}
				} else {
					gtk_widget_set_size_request(contentBox, 1, 1);
					gtk_container_add((GtkContainer*)handle, contentBox);
				}

				ret->setSizeRange(window->getMinimumSize(), window->getMaximumSize(), window->getMinimumAspectRatio(), window->getMaximumAspectRatio());

				return ret;
			}

		public:
			void* getHandle() override
			{
				return (void*)m_handle;
			}

			void close() override
			{
				if (!m_flagClosed) {
					if (!(UI::isUiThread())) {
						UI::dispatchToUiThread(SLIB_FUNCTION_WEAKREF(this, close));
						return;
					}
					GtkWindow* handle = m_handle;
					if (handle) {
						UIPlatform::removeWindowInstance(handle);
						gtk_widget_destroy((GtkWidget*)handle);
					}
					m_flagClosed = sl_true;
				}
				m_viewContent.setNull();
			}

			sl_bool isClosed() override
			{
				return m_flagClosed;
			}

			void setParentHandle(void* parent) override
			{
				GtkWindow* handle = m_handle;
				if (handle && !m_flagClosed) {
					if (parent) {
						gtk_window_set_transient_for(handle, (GtkWindow*)parent);
					} else {
						gtk_window_set_transient_for(handle, sl_null);
					}
				}
			}

			Ref<ViewInstance> getContentView() override
			{
				return m_viewContent;
			}

			static void _callback_remove_child(GtkWidget *widget, gpointer data)
			{
				gtk_container_remove((GtkContainer*)data, widget);
			}

			sl_bool getFrame(UIRect& _out) override
			{
				return sl_false;
			}

			void setFrame(const UIRect& frame) override
			{
				if (!m_flagClosed) {
					GtkWindow* handle = m_handle;
					if (handle) {
						m_location = frame.getLocation();
						m_size = frame.getSize();
						sl_ui_len width = m_size.x;
						sl_ui_len height = m_size.y;
						UIEdgeInsets insets;
						if (getClientInsets(insets)) {
							width -= insets.left + insets.right;
							height -= insets.top + insets.bottom - _getMenuHeight();
						}
						SetWindowSize(handle, m_flagResizable, width, height);
						gtk_window_move(handle, (gint)(frame.left), (gint)(frame.top));
					}
				}
			}

			void setTitle(const String& _title) override
			{
				if (!m_flagClosed) {
					GtkWindow* handle = m_handle;
					if (handle) {
						StringCstr title(_title);
						gtk_window_set_title(handle, title.getData());
					}
				}
			}

			void setIcon(const Ref<Drawable>& icon) override
			{
				GtkWindow* handle = m_handle;
				if (!handle) {
					return;
				}
				if (icon.isNotNull()) {
					GdkPixbuf* pixbuf = UIPlatform::createPixbuf(icon->toImage());
					if (pixbuf) {
						gtk_window_set_icon((GtkWindow*)handle, pixbuf);
						return;
					}
				}
				gtk_window_set_icon((GtkWindow*)handle, sl_null);
			}

			void setMenu(const Ref<Menu>& menu) override
			{
				if (m_flagClosed) {
					return;
				}
				GtkWindow* handle = m_handle;
				if (!handle) {
					return;
				}
				GtkWidget* hMenu = sl_null;
				if (!(UIPlatform::isPopupMenu(menu.get()))) {
					hMenu = (GtkWidget*)(UIPlatform::getMenuHandle(menu.get()));
				}
				if (hMenu == m_widgetMenu) {
					return;
				}
				GtkWidget* contentBox = m_widgetContentBox;
				if (!contentBox) {
					return;
				}
				g_object_ref(contentBox);
				gtk_container_foreach((GtkContainer*)handle, &_callback_remove_child, handle);
				if (hMenu) {
					GtkWidget* box = gtk_vbox_new(0, 0);
					if(box){
						gtk_widget_show(box);
						gtk_box_pack_start((GtkBox*)box, hMenu, 0, 0, 0);
						gtk_box_pack_start((GtkBox*)box, contentBox, 1, 1, 0);
						gtk_container_add((GtkContainer*)handle, box);
						m_widgetMenu = hMenu;
					}
				} else {
					gtk_container_add((GtkContainer*)handle, contentBox);
				}
				g_object_unref(contentBox);
			}

			sl_bool isActive() override
			{
				if (!m_flagClosed) {
					GtkWindow* handle = m_handle;
					if (handle) {
						return gtk_window_is_active(handle);
					}
				}
				return sl_false;
			}

			void activate() override
			{
				if (!m_flagClosed) {
					GtkWindow* handle = m_handle;
					if (handle) {
						gtk_window_present(handle);
					}
				}
			}

			void setBackgroundColor(const Color& color) override
			{
				if (!m_flagClosed) {
					if (UIPlatform::isSupportedGtk(3)) {
						GtkWindow* handle = m_handle;
						if (handle) {
							UIPlatform::setWidgetBackgroundColor((GtkWidget*)handle, color);
						}
					} else {
						GtkWidget* content = m_widgetContent;
						if (content) {
							UIPlatform::setWidgetBackgroundColor(content, color);
						}
					}
				}
			}

			void isMinimized(sl_bool& _out) override
			{
				_out = m_flagMinimized;
			}

			void setMinimized(sl_bool flag) override
			{
				if (!m_flagClosed) {
					GtkWindow* handle = m_handle;
					if (handle) {
						if (m_flagMinimized) {
							if (!flag) {
								m_flagMinimized = sl_false;
								gtk_window_deiconify(handle);
							}
						} else {
							if (flag) {
								m_flagMinimized = sl_true;
								gtk_window_iconify(handle);
							}
						}
					}
				}
			}

			void isMaximized(sl_bool& _out) override
			{
				_out = m_flagMaximized;
			}

			void setMaximized(sl_bool flag) override
			{
				if (!m_flagClosed) {
					GtkWindow* handle = m_handle;
					if (handle) {
						if (m_flagMaximized) {
							if (!flag) {
								m_flagMaximized = sl_false;
								gtk_window_unmaximize(handle);
							}
						} else {
							if (flag) {
								m_flagMaximized = sl_true;
								gtk_window_maximize(handle);
							}
						}
					}
				}
			}

			void setVisible(sl_bool flag) override
			{
				if (!m_flagClosed) {
					GtkWindow* handle = m_handle;
					if (handle) {
						if (flag) {
							gtk_window_move(handle, m_location.x, m_location.y);
							gtk_widget_show((GtkWidget*)handle);
						} else {
							gtk_widget_hide((GtkWidget*)handle);
						}
					}
				}
			}

			void setAlwaysOnTop(sl_bool flag) override
			{
				if (!m_flagClosed) {
					GtkWindow* handle = m_handle;
					if (handle) {
						if (flag) {
							gtk_window_set_keep_above(handle, sl_true);
						} else {
							gtk_window_set_keep_above(handle, sl_false);
						}
					}
				}
			}

			void setCloseButtonEnabled(sl_bool flag) override
			{
				if (!m_flagClosed) {
					GtkWindow* handle = m_handle;
					if (handle) {
						if (flag) {
							gtk_window_set_deletable(handle, sl_true);
						} else {
							gtk_window_set_deletable(handle, sl_false);
						}
					}
				}
			}

			void setAlpha(sl_real alpha) override
			{
				if (!m_flagClosed) {
					GtkWindow* handle = m_handle;
					if (handle) {
						if (alpha < 0) {
							alpha = 0;
						}
						if (alpha >= 1.0f) {
							alpha = 1.0f;
						}
						gtk_window_set_opacity(handle, alpha);
					}
				}
			}

			sl_bool getClientInsets(UIEdgeInsets& _out) override
			{
				if (m_flagClosed) {
					return sl_false;
				}
				GtkWindow* handle = m_handle;
				if (!handle) {
					return sl_false;
				}
				GdkWindow* _handle = gtk_widget_get_window((GtkWidget*)handle);
				if (!_handle) {
					return sl_false;
				}
				GdkRectangle rect;
				gdk_window_get_frame_extents(_handle, &rect);
				gint x = 0, y = 0, width, height;
				gdk_window_get_origin(_handle, &x, &y);
				gdk_window_get_geometry(_handle, sl_null, sl_null, &width, &height, sl_null);
				_out.left = (sl_ui_len)(x - rect.x);
				_out.top = (sl_ui_len)(y + _getMenuHeight() - rect.y);
				_out.right = (sl_ui_len)(rect.x + rect.width - (x + width));
				_out.bottom = (sl_ui_len)(rect.y + rect.height - (y + height));
				return sl_true;
			}

			void setSizeRange(const UISize& sizeMinimum, const UISize& sizeMaximum, float aspectRatioMinimum, float aspectRatioMaximum) override
			{
				if (m_flagClosed) {
					return;
				}
				GtkWindow* handle = m_handle;
				if (!handle) {
					return;
				}
				if (!m_flagResizable) {
					return;
				}

				GdkGeometry geometry;
				Base::zeroMemory(&geometry, sizeof(geometry));

				gint hints = GDK_HINT_MIN_SIZE;
				geometry.min_width = SLIB_MAX(0, sizeMinimum.x);
				geometry.min_height = SLIB_MAX(0, sizeMinimum.y);

				if (sizeMaximum.x > 0 || sizeMaximum.y > 0) {
					hints |= GDK_HINT_MAX_SIZE;
					gint w = sizeMaximum.x;
					if (w <= 0) {
						w = 1000000;
					}
					geometry.max_width = w;
					gint h = sizeMaximum.y;
					if (h <= 0) {
						h = 1000000;
					}
					geometry.max_height = h;
				}
				if (aspectRatioMinimum > 0 || aspectRatioMaximum > 0) {
					hints |= GDK_HINT_ASPECT;
					gdouble r;
					r = aspectRatioMinimum;
					if (r <= 0) {
						r = 0.00001;
					}
					geometry.min_aspect = r;
					r = aspectRatioMaximum;
					if (r <= 0) {
						r = 100000;
					}
					geometry.max_aspect = r;
				}
				gtk_window_set_geometry_hints(handle, (GtkWidget*)handle, &geometry, (GdkWindowHints)(hints));
			}

			sl_ui_len _getMenuHeight()
			{
				GtkWidget* menu = m_widgetMenu;
				if (menu) {
					GtkAllocation allocation;
					gtk_widget_get_allocation(menu, &allocation);
					gint h = allocation.height;
					if (h > 0) {
						return (sl_ui_len)h;
					}
				}
				return 0;
			}

			void _on_destroy()
			{
				m_flagClosed = sl_true;
			}

			static void _callback_destroy_cb(GtkWindow* handle, gpointer user_data)
			{
				Ref<WindowInstance> instance = UIPlatform::getWindowInstance(handle);
				if (instance.isNotNull()) {
					((GTK_WindowInstance*)(instance.get()))->_on_destroy();
				}
				UIPlatform::removeWindowInstance(handle);
			}

			static gboolean _callback_close_cb(GtkWindow* handle, GdkEvent* event, gpointer user_data)
			{
				Ref<WindowInstance> instance = UIPlatform::getWindowInstance(handle);
				if (instance.isNotNull()) {
					GTK_WindowInstance* _instance = static_cast<GTK_WindowInstance*>(instance.get());
					if (_instance->onClose()) {
						_instance->close();
					}
				}
				return sl_true; // ignore default behavior of GTK+ core
			}

			void _on_window_state(GdkEventWindowState* event)
			{
				if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED) {
					if (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) {
						m_flagMinimized = sl_true;
						onMinimize();
					} else {
						m_flagMinimized = sl_false;
						onDeminimize();
					}
				}
				if (event->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) {
					if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
						m_flagMaximized = sl_true;
						onMaximize();
					} else {
						m_flagMaximized = sl_false;
						onDemaximize();
					}
				}
			}

			static gboolean _callback_window_state_cb(GtkWindow* handle, GdkEventWindowState* event, gpointer user_data)
			{
				Ref<WindowInstance> instance = UIPlatform::getWindowInstance(handle);
				if (instance.isNotNull()) {
					((GTK_WindowInstance*)(instance.get()))->_on_window_state(event);
				}
				return sl_false;
			}

			void _on_process_configure()
			{
				GtkWindow* handle = m_handle;
				if (!handle) {
					return;
				}
				gint x, y, width, height;
				gtk_window_get_position(handle, &x, &y);
				gtk_window_get_size(handle, &width, &height);
				height -= (gint)(_getMenuHeight());

				sl_bool flagMove = !(Math::isAlmostZero(x - m_location.x) && Math::isAlmostZero(y - m_location.y));
				sl_bool flagResize = !(Math::isAlmostZero(width - m_size.x) && Math::isAlmostZero(height - m_size.y));

				if (m_flagFirstResize) {
					m_flagFirstResize = sl_false;
					flagMove = sl_false;
					flagResize = sl_true;
				}
				if (flagResize) {
					m_size.x = width;
					m_size.y = height;
					onResize((sl_ui_len)width, (sl_ui_len)height);
				}
				if (flagMove) {
					m_location.x = x;
					m_location.y = y;
					onMove(x, y);
				}
			}

			void _on_configure_event(GtkWindow* handle, GdkEventConfigure* event)
			{
				if (UIPlatform::isSupportedGtk(3)) {
					// call after animation
					UI::dispatchToUiThread(SLIB_FUNCTION_WEAKREF(this, _on_process_configure), 100);
				} else {
					_on_process_configure();
				}
			}

			static gboolean _callback_configure_event_cb(GtkWindow* handle, GdkEventConfigure* event, gpointer user_data)
			{
				Ref<WindowInstance> instance = UIPlatform::getWindowInstance(handle);
				if (instance.isNotNull()) {
					((GTK_WindowInstance*)(instance.get()))->_on_configure_event(handle, event);
				}
				return sl_false;
			}

			void _on_notify_is_active(GtkWindow* handle)
			{
				if (gtk_window_is_active(handle)) {
					onActivate();
				} else {
					onDeactivate();
				}
			}

			static void _callback_notify_is_active_cb(GtkWindow* handle, GParamSpec* pspec, gpointer user_data)
			{
				Ref<WindowInstance> instance = UIPlatform::getWindowInstance(handle);
				if (instance.isNotNull()) {
					((GTK_WindowInstance*)(instance.get()))->_on_notify_is_active(handle);
				}
			}

			static gboolean _callback_key_event(GtkWidget* widget, GdkEvent* ev, gpointer user_data)
			{
				GtkWidget* focus = gtk_window_get_focus((GtkWindow*)widget);
				if (!focus) {
					PlatformViewInstance::eventCallback(widget, ev, user_data);
				}
				return 0;
			}
		};
	}

	Ref<WindowInstance> Window::createWindowInstance()
	{
		return GTK_WindowInstance::create(this);
	}

	Ref<Window> Window::getActiveWindow()
	{
		Ref<WindowInstance> instance = UIPlatform::getActiveWindowInstance();
		if (instance.isNotNull()) {
			return instance->getWindow();
		}
		return sl_null;
	}

	sl_bool Window::_getClientInsets(UIEdgeInsets& _out)
	{
		return sl_false;
	}

	void Window::setDefaultIcon(const Ref<Drawable>& icon)
	{
		if (icon.isNotNull()) {
			GdkPixbuf* pixbuf = UIPlatform::createPixbuf(icon->toImage());
			if (pixbuf) {
				gtk_window_set_default_icon(pixbuf);
				return;
			}
		}
		gtk_window_set_default_icon(sl_null);
	}


	Ref<WindowInstance> UIPlatform::createWindowInstance(GtkWindow* handle)
	{
		Ref<WindowInstance> ret = UIPlatform::_getWindowInstance(handle);
		if (ret.isNotNull()) {
			return ret;
		}
		return GTK_WindowInstance::create(handle);
	}

	void UIPlatform::registerWindowInstance(GtkWindow* handle, WindowInstance* instance)
	{
		UIPlatform::_registerWindowInstance(handle, instance);
	}

	Ref<WindowInstance> UIPlatform::getWindowInstance(GtkWindow* handle)
	{
		return UIPlatform::_getWindowInstance(handle);
	}

	void UIPlatform::removeWindowInstance(GtkWindow* handle)
	{
		UIPlatform::_removeWindowInstance(handle);
	}

	GtkWindow* UIPlatform::getWindowHandle(WindowInstance* _instance)
	{
		GTK_WindowInstance* instance = (GTK_WindowInstance*)_instance;
		if (instance) {
			return instance->m_handle;
		} else {
			return sl_null;
		}
	}

	GtkWindow* UIPlatform::getWindowHandle(Window* window)
	{
		if (window) {
			Ref<WindowInstance> _instance = window->getWindowInstance();
			if (_instance.isNotNull()) {
				GTK_WindowInstance* instance = (GTK_WindowInstance*)(_instance.get());
				return instance->m_handle;
			}
		}
		return sl_null;
	}

	Ref<WindowInstance> UIPlatform::getActiveWindowInstance()
	{
		ListElements< Ref<WindowInstance> > instances(UIPlatform::_getAllWindowInstances());
		for (sl_size i = 0; i < instances.count; i++) {
			WindowInstance* instance = instances[i].get();
			GtkWindow* handle = UIPlatform::getWindowHandle(instance);
			if (gtk_window_is_active(handle)) {
				return instance;
			}
		}
		return sl_null;
	}

}

#endif
