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

#include "slib/render/opengl_glx.h"

#if defined(SLIB_RENDER_SUPPORT_OPENGL_GLX)

#include "slib/render/engine.h"
#include "slib/render/opengl.h"
#include "slib/core/time_counter.h"
#include "slib/core/thread.h"


#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
#	include "slib/dl/linux/gl.h"
#	include "slib/dl/linux/x11.h"
#else
#	include "gl/GLX/glx.h"
#endif

namespace slib
{

	namespace
	{
		class RendererImpl : public Renderer
		{
		public:
			Display* m_display = sl_null;
			Window m_window  = 0;
			GLXContext m_context = sl_null;
			sl_bool m_flagRequestRender = sl_true;
			Ref<Thread> m_threadRender;

		public:
			RendererImpl()
			{
			}

			~RendererImpl()
			{
				release();
			}

		public:
			static Ref<RendererImpl> create(Display* display, Window window, const RendererParam& param)
			{
				if (!display) {
					return sl_null;
				}
				if (!window) { // None
					return sl_null;
				}
				GLint attrs[] = {
					GLX_RGBA,
					GLX_RED_SIZE, param.nRedBits,
					GLX_GREEN_SIZE, param.nGreenBits,
					GLX_BLUE_SIZE, param.nBlueBits,
					GLX_ALPHA_SIZE, param.nAlphaBits,
					GLX_DEPTH_SIZE, param.nDepthBits,
					GLX_STENCIL_SIZE, param.nStencilBits,
					GLX_DOUBLEBUFFER,
					0
				};
				XVisualInfo* xvinfo = glXChooseVisual(display, 0, attrs);
				if (!xvinfo) {
					return sl_null;
				}
				GLXContext context = glXCreateContext(display, xvinfo, NULL, GL_TRUE);
				if (!context) {
					return sl_null;
				}
				Ref<RendererImpl> ret = new RendererImpl();
				if (ret.isNotNull()) {
					ret->initWithParam(param);
					ret->m_display = display;
					ret->m_window = window;
					ret->m_context = context;
					Ref<Thread> thread = Thread::create(SLIB_FUNCTION_MEMBER(ret.get(), run));
					if (thread.isNotNull()) {
						ret->m_threadRender = Move(thread);
						if (ret->m_threadRender->start()) {
							return ret;
						}
					}
					return sl_null;
				}
				glXDestroyContext(display, context);
				return sl_null;
			}

			void release()
			{
				ObjectLocker lock(this);
				Ref<Thread> thread = Move(m_threadRender);
				Display* display = m_display;
				m_display = sl_null;
				Window window = m_window;
				m_window  = 0;
				GLXContext context = m_context;
				m_context = sl_null;
				lock.unlock();
				if (thread.isNotNull()) {
					thread->finishAndWait();
				}
				if (context) {
					glXDestroyContext(display, context);
				}
			}

			void run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}

				glXMakeCurrent(m_display, m_window, m_context);

				Ref<RenderEngine> engine = GL::createEngine();
				if (engine.isNull()) {
					return;
				}

				TimeCounter timer;

				while (thread->isNotStopping()) {
					runStep(engine.get());
					if (thread->isNotStopping()) {
						sl_uint64 t = timer.getElapsedMilliseconds();
						if (t < 10) {
							thread->wait(10 - (sl_uint32)(t));
						}
						timer.reset();
					} else {
						break;
					}
				}

				glXMakeCurrent(m_display, 0, NULL);
			}

			void runStep(RenderEngine* engine)
			{
				XWindowAttributes attrs;
				if (!(XGetWindowAttributes(m_display, m_window, &attrs))) {
					return;
				}
				if (attrs.map_state != IsViewable) {
					return;
				}
				sl_bool flagUpdate = sl_false;
				if (isRenderingContinuously()) {
					flagUpdate = sl_true;
				} else {
					if (m_flagRequestRender) {
						flagUpdate = sl_true;
					}
				}
				m_flagRequestRender = sl_false;
				if (flagUpdate) {
					if (attrs.width != 0 && attrs.height != 0) {
						engine->setViewport(0, 0, attrs.width, attrs.height);
						handleFrame(engine);
						glXSwapBuffers(m_display, m_window);
					}
				}
			}

			void requestRender()
			{
				m_flagRequestRender = sl_true;
			}
		};
	}

	Ref<Renderer> GLX::createRenderer(void* xDisplay, unsigned long xWindow, const RendererParam& param)
	{
		return RendererImpl::create((Display*)xDisplay, xWindow, param);
	}

}

#else

namespace slib
{

	Ref<Renderer> GLX::createRenderer(void* xDisplay, unsigned long xWindow, const RendererParam& param)
	{
		return sl_null;
	}

}

#endif
