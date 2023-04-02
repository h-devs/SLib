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

#include "slib/ui/render_view.h"

#include "slib/render/opengl.h"

#include "view_gtk.h"

namespace slib
{

	namespace {

		class RenderViewHelper : public RenderView
		{
		};

		class RenderViewInstance : public GTK_ViewInstance, public IRenderViewInstance
		{
			SLIB_DECLARE_OBJECT

		public:
			AtomicRef<Renderer> m_renderer;
			RenderEngine* m_pLastEngine;

		public:
			RenderViewInstance()
			{
				m_pLastEngine = sl_null;
			}

			~RenderViewInstance()
			{
				Ref<Renderer> renderer = m_renderer;
				if (renderer.isNotNull()) {
					renderer->release();
				}
			}

		public:
			void setRedrawMode(RenderView* view, RedrawMode mode) override
			{
				Ref<Renderer> renderer = m_renderer;
				if (renderer.isNotNull()) {
					renderer->setRenderingContinuously(mode == RedrawMode::Continuously);
				}
			}

			void requestRender(RenderView* view) override
			{
				Ref<Renderer> renderer = m_renderer;
				if (renderer.isNotNull()) {
					renderer->requestRender();
				}
			}

			void setRenderer(const Ref<Renderer>& renderer, RedrawMode redrawMode)
			{
				m_renderer = renderer;
				if (renderer.isNotNull()) {
					renderer->setRenderingContinuously(redrawMode == RedrawMode::Continuously);
				}
			}

			void onExposeEvent(GdkEventExpose*) override
			{
				Ref<Renderer> renderer = m_renderer;
				if (renderer.isNotNull()) {
					renderer->requestRender();
				}
			}

			void onDrawEvent(cairo_t*) override
			{
				Ref<Renderer> renderer = m_renderer;
				if (renderer.isNotNull()) {
					renderer->requestRender();
				}
			}

			void onFrame(RenderEngine* engine)
			{
				Ref<RenderViewHelper> helper = CastRef<RenderViewHelper>(getView());
				if (helper.isNotNull()) {
					if (m_pLastEngine != engine) {
						helper->invokeCreateEngine(engine);
					}
					helper->handleFrame(engine);
					m_pLastEngine = engine;
				}
			}

		};

		SLIB_DEFINE_OBJECT(RenderViewInstance, GTK_ViewInstance)

	}

	Ref<ViewInstance> RenderView::createNativeWidget(ViewInstance* _parent)
	{
		GTK_ViewInstance* parent = static_cast<GTK_ViewInstance*>(_parent);
		GtkWidget* handle = gtk_drawing_area_new();
		if (handle) {
			gtk_widget_set_has_window(handle, 1);
			gtk_widget_set_can_focus(handle, 1);
			Ref<RenderViewInstance> ret = GTK_ViewInstance::create<RenderViewInstance>(this, parent, handle);
			if (ret.isNotNull()) {
				gtk_widget_realize(handle);
				GdkWindow* gwindow = gtk_widget_get_window(handle);
				if (gwindow) {
					void* xdisplay = sl_null; // GDK_WINDOW_XDISPLAY(gwindow);
					{
						auto funcGetDisplay = gdk::getApi_gdk_window_get_display();
						auto funcGetXDisplay = gdk::getApi_gdk_x11_display_get_xdisplay();
						if (funcGetDisplay && funcGetXDisplay) {
							xdisplay = funcGetXDisplay(funcGetDisplay(gwindow));
						} else {
							auto funcGetDrawableXDisplay = gdk::getApi_gdk_x11_drawable_get_xdisplay();
							auto funcGetDrawableImpl = gdk::getApi_gdk_x11_window_get_drawable_impl();
							if (funcGetDrawableXDisplay && funcGetDrawableImpl) {
								xdisplay = funcGetDrawableXDisplay(funcGetDrawableImpl(gwindow));
							}
						}
					}
					if (xdisplay) {
						unsigned long xwindow = 0;
						auto funcGetXWindow = gdk::getApi_gdk_x11_window_get_xid();
						if (funcGetXWindow) {
							xwindow = funcGetXWindow(gwindow);
						} else {
							xwindow = gdk_x11_drawable_get_xid(gwindow);
						}
						if (xwindow) {
							RendererParam rp;
							rp.onFrame = SLIB_FUNCTION_WEAKREF(ret, onFrame);
							Ref<Renderer> renderer = GLX::createRenderer(xdisplay, xwindow, rp);
							if (renderer.isNotNull()) {
								ret->setRenderer(renderer, m_redrawMode);
								return ret;
							}
						}
					}
				}
			}
		}
		return sl_null;
	}

	Ptr<IRenderViewInstance> RenderView::getRenderViewInstance()
	{
		return CastRef<RenderViewInstance>(getViewInstance());
	}

}

#endif
