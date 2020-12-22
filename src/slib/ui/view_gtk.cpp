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

#include "slib/core/definition.h"

#if defined(SLIB_UI_IS_GTK)

#include "view_gtk.h"

#include "slib/ui/core.h"
#include "slib/math/transform2d.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(GTK_ViewInstance, ViewInstance)

	GTK_ViewInstance::GTK_ViewInstance()
	{
		m_handle = sl_null;
		m_actionDrag = UIAction::MouseMove;
	}
	
	GTK_ViewInstance::~GTK_ViewInstance()
	{
		_release();
	}
	
	void GTK_ViewInstance::_init(GtkWidget* handle)
	{
		g_object_ref_sink(handle);
		m_handle = handle;
		UIPlatform::registerViewInstance(handle, this);
	}

	void GTK_ViewInstance::applyProperties(View* view, ViewInstance* _parent)
	{
		GtkWidget* handle = m_handle;
		if (!handle) {
			return;
		}
		
		if (!(view->isEnabled())) {
			gtk_widget_set_sensitive(handle, sl_false);
		}

		if (view->isUsingFont()) {
			setFont(view, view->getFont());
		}

		if (GTK_IS_FIXED(handle) || GTK_IS_DRAWING_AREA(handle)) {
			if (view->isDrawing()) {
				gtk_widget_set_app_paintable(handle, sl_true);
			}
			installEventsWithDrawing();
		} else {
			installEvents();
		}
		
		GtkWidget* parent = sl_null;
		if (_parent) {
			parent = ((GTK_ViewInstance*)_parent)->m_handle;
		}
		m_frame = view->getFrameInInstance();
		m_translation = Transform2::getTranslationFromMatrix(view->getFinalTransformInInstance());
		if (parent) {
			if (GTK_IS_FIXED(parent)) {
				sl_ui_pos x = m_frame.left + m_translation.x;
				sl_ui_pos y = m_frame.top + m_translation.y;
				gtk_fixed_put((GtkFixed*)parent, handle, x, y);
			} else if (GTK_IS_SCROLLED_WINDOW(parent)){
				gtk_scrolled_window_add_with_viewport((GtkScrolledWindow*)parent, handle);
			}
		}

		gtk_widget_set_size_request(handle, m_frame.getWidth(), m_frame.getHeight());

		if (view->isVisible()) {
			gtk_widget_show(handle);
		}
	}

	void GTK_ViewInstance::_release()
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			UIPlatform::removeViewInstance(handle);
			g_object_unref(handle);
			m_handle = sl_null;
		}
	}

	GtkWidget* GTK_ViewInstance::getHandle()
	{
		return m_handle;
	}
	
	sl_bool GTK_ViewInstance::isValid(View* view)
	{
		return sl_true;
	}
	
	void GTK_ViewInstance::setFocus(View* view, sl_bool flag)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			if (flag) {
				gtk_widget_grab_focus(handle);
			} else {
				GtkWidget* parent = gtk_widget_get_toplevel(handle);
				if (GTK_IS_WINDOW(parent)) {
					if (gtk_window_get_focus((GtkWindow*)parent) == handle) {
						gtk_window_set_focus((GtkWindow*)parent, sl_null);
					}
				}
			}
		}
	}
	
	void GTK_ViewInstance::invalidate(View* view)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			if (!(UI::isUiThread())) {
				void(GTK_ViewInstance::*func)(View*) = &GTK_ViewInstance::invalidate;
				UI::dispatchToUiThread(Function<void()>::bindWeakRef(this, func, sl_null));
				return;
			}
			gtk_widget_queue_draw(handle);
		}
	}
	
	void GTK_ViewInstance::invalidate(View* view, const UIRect& rect)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			if (!(UI::isUiThread())) {
				void(GTK_ViewInstance::*func)(View*, const UIRect&) = &GTK_ViewInstance::invalidate;
				UI::dispatchToUiThread(Function<void()>::bindWeakRef(this, func, sl_null, rect));
				return;
			}
			gtk_widget_queue_draw_area(handle, rect.left, rect.top, rect.getWidth(), rect.getHeight());
		}
	}
	
	void GTK_ViewInstance::setFrame(View* view, const UIRect& frame)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			m_frame = frame;
			_updateFrameAndTransform();
		}
	}
	
	void GTK_ViewInstance::setTransform(View* view, const Matrix3& m)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			m_translation = Transform2::getTranslationFromMatrix(m);
			_updateFrameAndTransform();
		}
	}

	void GTK_ViewInstance::_updateFrameAndTransform()
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			GtkWidget* parent = gtk_widget_get_parent(handle);
			if (parent && GTK_IS_FIXED(parent)) {
				gtk_fixed_move((GtkFixed*)parent, handle, m_frame.left + m_translation.x, m_frame.top + m_translation.y);
			}
			gtk_widget_set_size_request(handle, m_frame.getWidth(), m_frame.getHeight());
		}
	}
	
	void GTK_ViewInstance::setVisible(View* view, sl_bool flag)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			if (flag) {
				gtk_widget_show(handle);
			} else {
				gtk_widget_hide(handle);
			}
		}
	}
	
	void GTK_ViewInstance::setEnabled(View* view, sl_bool flag)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			if (flag) {
				gtk_widget_set_sensitive(handle, sl_true);
			} else {
				gtk_widget_set_sensitive(handle, sl_false);
			}
		}
	}
	
	void GTK_ViewInstance::setOpaque(View* view, sl_bool flag)
	{
	}
	
	void GTK_ViewInstance::setAlpha(View* view, sl_real alpha)
	{
	}

	void GTK_ViewInstance::setClipping(View* view, sl_bool flag)
	{
	}

	void GTK_ViewInstance::setDrawing(View* view, sl_bool flag)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			if (GTK_IS_FIXED(handle)) {
				if (flag) {
					gtk_widget_set_app_paintable(handle, sl_true);
				} else {
					gtk_widget_set_app_paintable(handle, sl_false);
				}
			}
		}
	}
	
	UIPointf GTK_ViewInstance::convertCoordinateFromScreenToView(View* view, const UIPointf& ptScreen)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			sl_ui_len x, y;
			UIPlatform::getScreenLocationOfWidget(handle, &x, &y);
			return UIPointf((sl_ui_posf)(ptScreen.x - x), (sl_ui_posf)(ptScreen.y - y));
		}
		return ptScreen;
	}
	
	UIPointf GTK_ViewInstance::convertCoordinateFromViewToScreen(View* view, const UIPointf& ptView)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			sl_ui_len x, y;
			UIPlatform::getScreenLocationOfWidget(handle, &x, &y);
			return UIPointf((sl_ui_posf)(ptView.x + x), (sl_ui_posf)(ptView.y + y));
		}
		return ptView;
	}
	
	void GTK_ViewInstance::addChildInstance(View* view, const Ref<ViewInstance>& _child)
	{
		GTK_ViewInstance* child = static_cast<GTK_ViewInstance*>(_child.get());
		if (child) {
			GtkWidget* handle = m_handle;
			if (handle) {
				GtkWidget* handleChild = child->m_handle;
				if (handleChild) {
					if (GTK_IS_FIXED(handle)) {
						UIPoint location;
						Ref<View> view = child->getView();
						if (view.isNotNull()) {
							location = view->getLocation();
						} else {
							location.x = 0;
							location.y = 0;
						}
						gtk_fixed_put((GtkFixed*)handle, handleChild, location.x, location.y);
					} else if (GTK_IS_CONTAINER(handle)) {
						gtk_container_add((GtkContainer*)handle, handleChild);
					}
				}
			}
		}
	}
	
	void GTK_ViewInstance::removeChildInstance(View* view, const Ref<ViewInstance>& _child)
	{
		GTK_ViewInstance* child = static_cast<GTK_ViewInstance*>(_child.get());
		if (child) {
			GtkWidget* handle = m_handle;
			if (handle) {
				GtkWidget* handleChild = child->m_handle;
				if (handleChild) {
					if (GTK_IS_CONTAINER(handle)) {
						gtk_container_remove((GtkContainer*)handle, handleChild);
					}
				}
			}
		}
	}
	
	void GTK_ViewInstance::bringToFront(View* view)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			GdkWindow* window = handle->window;
			if (window) {
				gdk_window_raise(window);
			}
		}
	}

	void GTK_ViewInstance::setFont(View* view, const Ref<Font>& font)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			PangoFontDescription* desc = GraphicsPlatform::getPangoFont(font);
			if (desc) {
				gtk_widget_modify_font(handle, desc);
			}
		}
	}

	void GTK_ViewInstance::installEvents()
	{
		installEvents(getEventMask());
	}

	void GTK_ViewInstance::installEventsWithDrawing()
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			g_signal_connect(handle, "expose_event", G_CALLBACK(eventCallback), handle);
			installEvents(GDK_EXPOSURE_MASK | getEventMask());
		}
	}

	void GTK_ViewInstance::installEvents(gint mask)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			GtkWidget* handleConnect = handle;
			if (isWindowContent()) {
				handleConnect = gtk_widget_get_parent(handle);
				if (!handleConnect) {
					return;
				}
			}
			g_signal_connect(handleConnect, "motion-notify-event", G_CALLBACK(eventCallback), handle);
			g_signal_connect(handleConnect, "button-press-event", G_CALLBACK(eventCallback), handle);
			g_signal_connect(handleConnect, "button-release-event", G_CALLBACK(eventCallback), handle);
			g_signal_connect(handleConnect, "enter-notify-event", G_CALLBACK(eventCallback), handle);
			g_signal_connect(handleConnect, "leave-notify-event", G_CALLBACK(eventCallback), handle);
			g_signal_connect(handleConnect, "key-press-event", G_CALLBACK(eventCallback), handle);
			g_signal_connect(handleConnect, "key-release-event", G_CALLBACK(eventCallback), handle);
			g_signal_connect(handleConnect, "scroll-event", G_CALLBACK(eventCallback), handle);
			g_signal_connect(handleConnect, "focus-in-event", G_CALLBACK(eventCallback), handle);
			gtk_widget_set_events(handleConnect, mask);
		}
	}
	
	gboolean GTK_ViewInstance::eventCallback(GtkWidget*, GdkEvent* event, gpointer user_data)
	{
		Ref<GTK_ViewInstance> instance = Ref<GTK_ViewInstance>::from(UIPlatform::getViewInstance((GtkWidget*)user_data));
		if (instance.isNotNull()) {
			switch (event->type) {
				case GDK_EXPOSE:
					instance->onExposeEvent((GdkEventExpose*)event);
					break;
				case GDK_MOTION_NOTIFY:
					return instance->onMotionNotifyEvent((GdkEventMotion*)event);
				case GDK_BUTTON_PRESS:
				case GDK_2BUTTON_PRESS:
				case GDK_BUTTON_RELEASE:
					return instance->onButtonEvent((GdkEventButton*)event);
				case GDK_ENTER_NOTIFY:
				case GDK_LEAVE_NOTIFY:
					return instance->onCrossingEvent((GdkEventCrossing*)event);
				case GDK_KEY_PRESS:
				case GDK_KEY_RELEASE:
					return instance->onKeyEvent((GdkEventKey*)event);
				case GDK_SCROLL:
					return instance->onScrollEvent((GdkEventScroll*)event);
				case GDK_FOCUS_CHANGE:
					return instance->onFocusEvent((GdkEventFocus*)event);
			}
		}
		return sl_false;
	}

	void GTK_ViewInstance::onExposeEvent(GdkEventExpose* event)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			GdkWindow* window = handle->window;
			if (window) {
				cairo_t* cairo = gdk_cairo_create(window);
				if (cairo) {
					GtkAllocation region = handle->allocation;
					Ref<Canvas> canvas = GraphicsPlatform::createCanvas(CanvasType::View, cairo, region.width, region.height);
					if (canvas.isNotNull()) {
						Rectangle rect;
						rect.left = event->area.x;
						rect.top = event->area.y;
						rect.right = rect.left + event->area.width;
						rect.bottom = rect.top + event->area.height;
						canvas->setInvalidatedRect(rect);
						onDraw(canvas.get());
					}
				}
			}
		}
	}
	
	gboolean GTK_ViewInstance::onMotionNotifyEvent(GdkEventMotion* gevent)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			Time time;
			time.setMillisecondsCount(gevent->time);
			Ref<UIEvent> event = UIEvent::createMouseEvent(m_actionDrag, gevent->x, gevent->y, time);
			if (event.isNotNull()) {
				UIPlatform::applyEventModifiers(event.get(), gevent->state);
				onMouseEvent(event.get());
				if (event->isStoppedPropagation() || event->isPreventedDefault()) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}
	
	gboolean GTK_ViewInstance::onButtonEvent(GdkEventButton* gevent)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			UIAction action;
			if (gevent->button == 1) { // Left
				if (gevent->type == GDK_BUTTON_PRESS) {
					action = UIAction::LeftButtonDown;
					m_actionDrag = UIAction::LeftButtonDrag;
				} else if (gevent->type == GDK_BUTTON_RELEASE) {
					action = UIAction::LeftButtonUp;
					m_actionDrag = UIAction::MouseMove;
				} else { // GDK_2BUTTON_PRESS
					action = UIAction::LeftButtonDoubleClick;
				}
			} else if (gevent->button == 2) { // Middle
				if (gevent->type == GDK_BUTTON_PRESS) {
					action = UIAction::MiddleButtonDown;
					m_actionDrag = UIAction::MiddleButtonDrag;
				} else if (gevent->type == GDK_BUTTON_RELEASE) {
					action = UIAction::MiddleButtonUp;
					m_actionDrag = UIAction::MouseMove;
				} else { // GDK_2BUTTON_PRESS
					action = UIAction::MiddleButtonDoubleClick;
				}
			} else if (gevent->button == 3) { // Right
				if (gevent->type == GDK_BUTTON_PRESS) {
					action = UIAction::RightButtonDown;
					m_actionDrag = UIAction::RightButtonDrag;
				} else if (gevent->type == GDK_BUTTON_RELEASE) {
					action = UIAction::RightButtonUp;
					m_actionDrag = UIAction::MouseMove;
				} else { // GDK_2BUTTON_PRESS
					action = UIAction::RightButtonDoubleClick;
				}
			} else {
				return sl_false;
			}
			Time time;
			time.setMillisecondsCount(gevent->time);
			gdouble x = gevent->x;
			gdouble y = gevent->y;
			GdkWindow* window = handle->window;
			if (window && window != gevent->window) {
				gint wx = 0, wy = 0;
				gdk_window_get_origin(window, &wx, &wy);
				x = gevent->x_root - wx;
				y = gevent->y_root - wy;
			}
			Ref<UIEvent> event = UIEvent::createMouseEvent(action, x, y, time);
			if (event.isNotNull()) {
				UIPlatform::applyEventModifiers(event.get(), gevent->state);
				onMouseEvent(event.get());
				if (event->isStoppedPropagation() || event->isPreventedDefault()) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}
	
	gboolean GTK_ViewInstance::onCrossingEvent(GdkEventCrossing* gevent)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			UIAction action;
			if (gevent->type == GDK_ENTER_NOTIFY) {
				action = UIAction::MouseEnter;
			} else { // GDK_LEAVE_NOTIFY
				action = UIAction::MouseLeave;
			}
			Time time;
			time.setMillisecondsCount(gevent->time);
			gdouble x = gevent->x;
			gdouble y = gevent->y;
			GdkWindow* window = handle->window;
			if (window && window != gevent->window) {
				gint wx = 0, wy = 0;
				gdk_window_get_origin(window, &wx, &wy);
				x = gevent->x_root - wx;
				y = gevent->y_root - wy;
			}
			Ref<UIEvent> event = UIEvent::createMouseEvent(action, x, y, time);
			if (event.isNotNull()) {
				UIPlatform::applyEventModifiers(event.get(), gevent->state);
				onMouseEvent(event.get());
				if (event->isStoppedPropagation() || event->isPreventedDefault()) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}
	
	gboolean GTK_ViewInstance::onKeyEvent(GdkEventKey* gevent)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			UIAction action;
			if (gevent->type == GDK_KEY_PRESS) {
				action = UIAction::KeyDown;
			} else { // GDK_KEY_RELEASE
				action = UIAction::KeyUp;
			}
			Keycode key = UIEvent::getKeycodeFromSystemKeycode(gevent->keyval);
			Time time;
			time.setMillisecondsCount(gevent->time);
			Ref<UIEvent> event = UIEvent::createKeyEvent(action, key, gevent->keyval, time);
			if (event.isNotNull()) {
				UIPlatform::applyEventModifiers(event.get(), gevent->state);
				ViewInstance::onKeyEvent(event.get());
				if (event->isStoppedPropagation() || event->isPreventedDefault()) {
					return sl_true;
				}
			}
			if (key == Keycode::Up || key == Keycode::Down) {
				return sl_true;
			}
		}
		return sl_false;
	}
	
	gboolean GTK_ViewInstance::onScrollEvent(GdkEventScroll* gevent)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			Time time;
			time.setMillisecondsCount(gevent->time);
			gdouble x = gevent->x;
			gdouble y = gevent->y;
			GdkWindow* window = handle->window;
			if (window && window != gevent->window) {
				gint wx = 0, wy = 0;
				gdk_window_get_origin(window, &wx, &wy);
				x = gevent->x_root - wx;
				y = gevent->y_root - wy;
			}
			gint dx = 0;
			gint dy = 0;
			gint delta = 10;
			switch (gevent->direction) {
				case GDK_SCROLL_UP:
					dy = -delta;
					break;
				case GDK_SCROLL_DOWN:
					dy = delta;
					break;
				case GDK_SCROLL_LEFT:
					dx = -delta;
					break;
				case GDK_SCROLL_RIGHT:
					dx = delta;
					break;
				default:
					return sl_false;
			}
			Ref<UIEvent> event = UIEvent::createMouseWheelEvent(x, y, dx, dy, time);
			if (event.isNotNull()) {
				UIPlatform::applyEventModifiers(event.get(), gevent->state);
				onMouseWheelEvent(event.get());
				if (event->isStoppedPropagation() || event->isPreventedDefault()) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}
	
	gboolean GTK_ViewInstance::onFocusEvent(GdkEventFocus* gevent)
	{
		GtkWidget* handle = m_handle;
		if (handle) {
			if (gevent->in) {
				onSetFocus();
			} else {
				onKillFocus();
			}
		}
		return sl_false;
	}
	
	gint GTK_ViewInstance::getEventMask()
	{
		return SLIB_GTK_EVENT_MASK_DEFAULT;
	}

	Ref<ViewInstance> View::createGenericInstance(ViewInstance* _parent)
	{
		GTK_ViewInstance* parent = static_cast<GTK_ViewInstance*>(_parent);
		
		GtkWidget* handle;
		if (m_flagCreatingChildInstances) {
			handle = gtk_fixed_new();
		} else {
			handle = gtk_drawing_area_new();
		}
		
		if (handle) {
			GTK_WIDGET_UNSET_FLAGS(handle, GTK_NO_WINDOW);
			GTK_WIDGET_SET_FLAGS(handle, GTK_CAN_FOCUS);
			Ref<GTK_ViewInstance> ret = GTK_ViewInstance::create<GTK_ViewInstance>(this, parent, handle);
			if (ret.isNotNull()) {
				return ret;
			}
			g_object_ref_sink(handle);
			g_object_unref(handle);
		}
		return sl_null;
	}


	Ref<ViewInstance> UIPlatform::createViewInstance(GtkWidget* handle)
	{
		Ref<ViewInstance> ret = UIPlatform::_getViewInstance(handle);
		if (ret.isNotNull()) {
			return ret;
		}
		return GTK_ViewInstance::create<GTK_ViewInstance>(handle);
	}
	
	void UIPlatform::registerViewInstance(GtkWidget* handle, ViewInstance* instance)
	{
		UIPlatform::_registerViewInstance(handle, instance);
	}
	
	Ref<ViewInstance> UIPlatform::getViewInstance(GtkWidget* handle)
	{
		return UIPlatform::_getViewInstance(handle);
	}
	
	Ref<View> UIPlatform::getView(GtkWidget* handle)
	{
		Ref<ViewInstance> instance = UIPlatform::_getViewInstance(handle);
		if (instance.isNotNull()) {
			return instance->getView();
		}
		return sl_null;
	}

	void UIPlatform::removeViewInstance(GtkWidget* handle)
	{
		UIPlatform::_removeViewInstance(handle);
	}
	
	GtkWidget* UIPlatform::getViewHandle(ViewInstance* _instance)
	{
		GTK_ViewInstance* instance = static_cast<GTK_ViewInstance*>(_instance);
		if (instance) {
			return instance->getHandle();
		}
		return sl_null;
	}
	
	GtkWidget* UIPlatform::getViewHandle(View* view)
	{
		if (view) {
			Ref<ViewInstance> _instance = view->getViewInstance();
			if (_instance.isNotNull()) {
				GTK_ViewInstance* instance = static_cast<GTK_ViewInstance*>(_instance.get());
				return instance->getHandle();
			}
		}
		return sl_null;
	}

	void UIPlatform::getScreenLocationOfWidget(GtkWidget* widget, sl_ui_len* out_x, sl_ui_len* out_y)
	{
		sl_ui_len x = 0;
		sl_ui_len y = 0;
		GdkWindow* window = widget->window;
		if (window) {
			gint ox = 0;
			gint oy = 0;
			gdk_window_get_origin(window, &ox, &oy);
			GtkAllocation allocation = widget->allocation;
			x = ox + allocation.x;
			y = oy + allocation.y;
		}
		if (out_x) {
			*out_x = x;
		}
		if (out_y) {
			*out_y = y;
		}
	}

	void UIPlatform::setWidgetFont(GtkWidget* handle, const Ref<Font>& font)
	{
		PangoFontDescription* desc = GraphicsPlatform::getPangoFont(font);
		if (desc) {
			gtk_widget_modify_font(handle, desc);
		}
	}

}

#endif


