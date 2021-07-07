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

#ifndef CHECKHEADER_SLIB_UI_VIEW_GTK
#define CHECKHEADER_SLIB_UI_VIEW_GTK

#include "../core/definition.h"

#if defined(SLIB_UI_IS_GTK)

#include "view.h"

#include "platform.h"

#define SLIB_GTK_EVENT_MASK_DEFAULT \
	(GDK_POINTER_MOTION_MASK | GDK_BUTTON_MOTION_MASK | \
	GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | \
	GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | \
	GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | \
	GDK_SCROLL_MASK | \
	GDK_FOCUS_CHANGE_MASK)

namespace slib
{

	class GTK_ViewInstance : public ViewInstance
	{
		SLIB_DECLARE_OBJECT

	public:
		GTK_ViewInstance();

		~GTK_ViewInstance();
		
	public:
		template <class T>
		static Ref<T> create(GtkWidget* handle, sl_bool flagFreeOnFailure = sl_true)
		{
			if (handle) {
				Ref<T> ret = new T();
				if (ret.isNotNull()) {
					ret->_init(handle);
					return ret;
				}
				if (flagFreeOnFailure) {
					g_object_ref_sink(handle);
					g_object_unref(handle);
				}
			}
			return sl_null;
		}

		template <class T>
		static Ref<T> create(View* view, ViewInstance* parent, GtkWidget* handle, sl_bool flagFreeOnFailure = sl_true)
		{
			Ref<T> ret = create<T>(handle, flagFreeOnFailure);
			if (ret.isNotNull()) {
				ret->applyProperties(view, parent);
				return ret;
			}
			return sl_null;
		}

	public:
		void applyProperties(View* view, ViewInstance* parent);

		GtkWidget* getHandle();
		
		sl_bool isValid(View* view) override;

		void setFocus(View* view, sl_bool flag) override;

		void invalidate(View* view) override;

		void invalidate(View* view, const UIRect& rect) override;

		void setFrame(View* view, const UIRect& frame) override;

		void setTransform(View* view, const Matrix3& transform) override;

		void setVisible(View* view, sl_bool flag) override;

		void setEnabled(View* view, sl_bool flag) override;

		void setOpaque(View* view, sl_bool flag) override;

		void setAlpha(View* view, sl_real alpha) override;

		void setClipping(View* view, sl_bool flag) override;

		void setDrawing(View* view, sl_bool flag) override;
		
		UIPointf convertCoordinateFromScreenToView(View* view, const UIPointf& ptScreen) override;

		UIPointf convertCoordinateFromViewToScreen(View* view, const UIPointf& ptView) override;

		void addChildInstance(View* view, const Ref<ViewInstance>& instance) override;

		void removeChildInstance(View* view, const Ref<ViewInstance>& instance) override;

		void bringToFront(View* view) override;

		void setFont(View* view, const Ref<Font>& font) override;
		
	public:
		void installEventsWithDrawing();

		void installEvents();

		void installEvents(gint mask);

		static gboolean eventCallback(GtkWidget* widget, GdkEvent* event, gpointer user_data);

		static gboolean drawCallback(GtkWidget* widget, cairo_t* cr, gpointer user_data);

	public:
		// GTK 2
		virtual void onExposeEvent(GdkEventExpose* event);
		// GTK 3
		virtual void onDrawEvent(cairo_t* cr);

		virtual gboolean onMotionNotifyEvent(GdkEventMotion* event);
		
		virtual gboolean onButtonEvent(GdkEventButton* event);
		
		virtual gboolean onCrossingEvent(GdkEventCrossing* event);

		virtual gboolean onKeyEvent(GdkEventKey* event);

		virtual gboolean onScrollEvent(GdkEventScroll* event);
		
		virtual gboolean onFocusEvent(GdkEventFocus* event);

		virtual gint getEventMask();

	private:
		void _init(GtkWidget* handle);

		void _release();

		void _updateFrameAndTransform();

	protected:
		GtkWidget* m_handle;
		UIAction m_actionDrag;

		UIRect m_frame;
		UIPoint m_translation;

	};

}

#endif

#endif
