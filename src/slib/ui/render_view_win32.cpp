/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#if defined(SLIB_UI_IS_WIN32)

#include "slib/ui/render_view.h"

#include "slib/render/opengl.h"
#include "slib/render/d3d.h"

#include "view_win32.h"

namespace slib
{

	namespace {

		class RenderViewInstance : public Win32_ViewInstance, public IRenderViewInstance
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
				release();
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

			sl_bool isRenderEnabled(RenderView* view) override
			{
				return m_renderer.isNotNull();
			}

			void disableRendering(RenderView* view) override
			{
				release();
			}

			sl_bool isDrawingEnabled(View* view) override
			{
				return m_renderer.isNull();
			}

			void setRenderer(const Ref<Renderer>& renderer, RedrawMode redrawMode)
			{
				m_renderer = renderer;
				if (renderer.isNotNull()) {
					renderer->setRenderingContinuously(redrawMode == RedrawMode::Continuously);
				}
			}

			LRESULT processWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
			{
				if (msg == WM_PAINT) {
					Ref<Renderer> renderer = m_renderer;
					if (renderer.isNotNull()) {
						PAINTSTRUCT ps;
						BeginPaint(m_handle, &ps);
						EndPaint(m_handle, &ps);
						renderer->requestRender();
						return 0;
					}
				} else if (msg == WM_ERASEBKGND) {
					return TRUE;
				}
				return Win32_ViewInstance::processWindowMessage(msg, wParam, lParam);
			}

			void onFrame(RenderEngine* engine)
			{
				Ref<View> _view = getView();
				if (RenderView* view = CastInstance<RenderView>(_view.get())) {
					if (m_pLastEngine != engine) {
						view->invokeCreateEngine(engine);
					}
					view->handleFrame(engine);
					m_pLastEngine = engine;
				}
			}

			void release()
			{
				ObjectLocker lock(this);
				Ref<Renderer> renderer = m_renderer;
				if (renderer.isNotNull()) {
					renderer->release();
					m_renderer.setNull();
				}
			}

		};

		SLIB_DEFINE_OBJECT(RenderViewInstance, Win32_ViewInstance)

	}

	Ref<ViewInstance> RenderView::createNativeWidget(ViewInstance* parent)
	{
		Win32_UI_Shared* shared = Win32_UI_Shared::get();
		if (!shared) {
			return sl_null;
		}
		Ref<RenderViewInstance> ret = Win32_ViewInstance::create<RenderViewInstance>(this, parent, (LPCWSTR)((LONG_PTR)(shared->wndClassForView)), sl_null, 0, 0);
		if (ret.isNotNull()) {
			RenderEngineType engineType = getPreferredEngineType();
			if (SLIB_RENDER_CHECK_ENGINE_TYPE(engineType, GL)) {
				if (SLIB_RENDER_CHECK_ENGINE_TYPE(engineType, OpenGL_ES)) {
					EGL::loadEntries();
					GLES::loadEntries();
					if (!(EGL::isAvailable() && GLES::isAvailable())) {
						engineType = RenderEngineType::OpenGL;
					}
				}
				if (SLIB_RENDER_CHECK_ENGINE_TYPE(engineType, OpenGL_ES)) {
					RendererParam rp;
					rp.onFrame = SLIB_FUNCTION_WEAKREF(ret, onFrame);
					Ref<Renderer> renderer = EGL::createRenderer((void*)(ret->getHandle()), rp);
					if (renderer.isNotNull()) {
						ret->setRenderer(renderer, m_redrawMode);
						return ret;
					}
				} else {
					RendererParam rp;
					rp.onFrame = SLIB_FUNCTION_WEAKREF(ret, onFrame);
					Ref<Renderer> renderer = WGL::createRenderer((void*)(ret->getHandle()), rp);
					if (renderer.isNotNull()) {
						ret->setRenderer(renderer, m_redrawMode);
						return ret;
					}
				}
			} else if (SLIB_RENDER_CHECK_ENGINE_TYPE(engineType, D3D)) {
				RendererParam rp;
				rp.onFrame = SLIB_FUNCTION_WEAKREF(ret, onFrame);
				Ref<Renderer> renderer = Direct3D::createRenderer(engineType, (void*)(ret->getHandle()), rp);
				if (renderer.isNotNull()) {
					ret->setRenderer(renderer, m_redrawMode);
					return ret;
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
