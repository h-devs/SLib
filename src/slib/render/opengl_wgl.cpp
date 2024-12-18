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

#include "slib/render/opengl_wgl.h"

#if defined(SLIB_RENDER_SUPPORT_OPENGL_WGL)

#include "slib/render/engine.h"
#include "slib/render/opengl.h"
#include "slib/core/time_counter.h"
#include "slib/core/thread.h"
#include "slib/ui/platform.h"

#pragma comment(lib, "opengl32.lib")

namespace slib
{

	namespace
	{
		class RendererImpl : public Renderer
		{
		public:
			sl_bool m_flagRequestRender = sl_true;
			HGLRC m_context = sl_null;
			HWND m_hWindow = sl_null;
			HDC m_hDC = sl_null;

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
			static Ref<RendererImpl> create(void* windowHandle, const RendererParam& param)
			{
				HWND hWnd = (HWND)windowHandle;
				if (!hWnd) {
					return sl_null;
				}
				HDC hDC = GetDC(hWnd);
				if (hDC) {
					PIXELFORMATDESCRIPTOR pfd;
					Base::zeroMemory(&pfd, sizeof(pfd));
					pfd.nSize = sizeof(pfd);
					pfd.nVersion = 1;
					pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
					pfd.iPixelType = PFD_TYPE_RGBA;
					pfd.cColorBits = param.nRedBits + param.nGreenBits + param.nBlueBits;
					pfd.cAlphaBits = param.nAlphaBits;
					pfd.cAccumBits = param.nAccumBits;
					pfd.cDepthBits = param.nDepthBits;
					pfd.cStencilBits = param.nStencilBits;
					pfd.iLayerType = PFD_MAIN_PLANE;

					int iPixelFormat = ChoosePixelFormat(hDC, &pfd);
					if (iPixelFormat) {
						if (SetPixelFormat(hDC, iPixelFormat, &pfd)) {
							HGLRC context = wglCreateContext(hDC);
							if (context) {
								Ref<RendererImpl> ret = new RendererImpl();
								if (ret.isNotNull()) {
									ret->initWithParam(param);
									ret->m_hWindow = hWnd;
									ret->m_hDC = hDC;
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
								wglDeleteContext(context);
							}
						}
					}
					ReleaseDC(hWnd, hDC);
				}
				return sl_null;
			}

			void release()
			{
				ObjectLocker lock(this);
				Ref<Thread> thread = Move(m_threadRender);
				HGLRC context = m_context;
				m_context = sl_null;
				HWND hWindow = m_hWindow;
				m_hWindow = sl_null;
				HDC hDC = m_hDC;
				m_hDC = sl_null;
				lock.unlock();
				if (thread.isNotNull()) {
					thread->finishAndWait();
				}
				if (context) {
					wglDeleteContext(context);
				}
				if (hDC) {
					ReleaseDC(hWindow, hDC);
				}
			}

			void run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}

				wglMakeCurrent(m_hDC, m_context);

				GL::loadEntries();

				Ref<RenderEngine> engine = GL::createEngine();
				if (engine.isNull()) {
					return;
				}

				TimeCounter timer;
				while (thread->isNotStopping()) {
					Ref<RendererImpl> thiz = this;
					runStep(engine.get());
					if (thread->isNotStopping()) {
						sl_uint64 t = timer.getElapsedMilliseconds();
						if (t < 10) {
							Thread::sleep(10 - (sl_uint32)(t));
						}
						timer.reset();
					} else {
						break;
					}
				}
				wglMakeCurrent(NULL, NULL);
			}

			void runStep(RenderEngine* engine)
			{
				if (!(UIPlatform::isWindowVisible(m_hWindow))) {
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
					RECT rect;
					GetClientRect(m_hWindow, &rect);
					if (rect.right && rect.bottom) {
						engine->setViewport(0, 0, rect.right, rect.bottom);
						handleFrame(engine);
						if (m_hDC) {
							SwapBuffers(m_hDC);
						}
					}
				}
			}

			void requestRender()
			{
				m_flagRequestRender = sl_true;
			}

		};
	}

	Ref<Renderer> WGL::createRenderer(void* windowHandle, const RendererParam& param)
	{
		return RendererImpl::create(windowHandle, param);
	}

}

#else

namespace slib
{

	Ref<Renderer> WGL::createRenderer(void* windowHandle, const RendererParam& param)
	{
		return sl_null;
	}

}

#endif
