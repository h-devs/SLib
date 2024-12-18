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

#ifndef CHECKHEADER_SLIB_UI_RENDER_VIEW
#define CHECKHEADER_SLIB_UI_RENDER_VIEW

#include "view.h"

#include "../render/engine.h"
#include "../core/queue.h"

namespace slib
{

	class IRenderViewInstance;

	class SLIB_EXPORT RenderView : public ViewGroup
	{
		SLIB_DECLARE_OBJECT

	public:
		RenderView();

		~RenderView();

	protected:
		void init() override;

	public:
		RenderEngineType getPreferredEngineType();

		void setPreferredEngineType(RenderEngineType type);


		RedrawMode getRedrawMode();

		void setRedrawMode(RedrawMode mode);


		sl_bool isDispatchingEventsToRenderingThread();

		void setDispatchingEventsToRenderingThread(sl_bool flag);


		sl_bool isRenderEnabled();

		void disableRendering();

		void requestRender();


		void invalidate(UIUpdateMode mode = UIUpdateMode::Redraw) override;

		void invalidate(const UIRect& rect, UIUpdateMode mode = UIUpdateMode::Redraw) override;

		void renderViewContent(RenderEngine* engine);


		Ref<AnimationLoop> getAnimationLoop() override;

		sl_bool isDrawingThread() override;

		void dispatchToDrawingThread(const Function<void()>& callback, sl_uint32 delayMillis = 0) override;

		void runOnDrawingThread(const Function<void()>& callback) override;

		Ref<Dispatcher> getDispatcher() override;


		sl_bool isDebugTextVisible();

		void setDebugTextVisible(sl_bool flagVisible = sl_true);

		sl_bool isDebugTextVisibleOnRelease();

		void setDebugTextVisibleOnRelease(sl_bool flagVisible = sl_true);

	public:
		SLIB_DECLARE_EVENT_HANDLER(RenderView, CreateEngine, RenderEngine* engine)

		SLIB_DECLARE_EVENT_HANDLER(RenderView, Frame, RenderEngine* engine)
		virtual void handleFrame(RenderEngine* engine);

	public:
		void onAttach() override;

		void onDetach() override;

	protected:
		void onDrawBackground(Canvas* canvas) override;

	protected:
		Ref<ViewInstance> createInstance(ViewInstance* _parent) override;

		virtual Ptr<IRenderViewInstance> getRenderViewInstance();

	public:
		void dispatchDraw(Canvas* canvas) override;

		void dispatchMouseEvent(UIEvent* ev) override;

		void dispatchTouchEvent(UIEvent* ev) override;

		void dispatchMouseWheelEvent(UIEvent* ev) override;

		void dispatchKeyEvent(UIEvent* ev) override;

	private:
		void _processPostedCallbacks();

		void _processPostedCallbacksNoLock();

		void _dispatchMouseEvent(const Ref<UIEvent>& ev);

		void _dispatchTouchEvent(const Ref<UIEvent>& ev);

		void _dispatchMouseWheelEvent(const Ref<UIEvent>& ev);

		void _dispatchKeyEvent(const Ref<UIEvent>& ev);

	protected:
		RenderEngineType m_preferredEngineType;
		RedrawMode m_redrawMode;
		sl_bool m_flagDispatchEventsToRenderingThread;

		Ref<AnimationLoop> m_animationLoop;
		sl_uint64 m_lastRenderingThreadId;
		Queue< Function<void()> > m_queuePostedCallbacks;

		sl_bool m_flagDebugTextVisible;
		sl_bool m_flagDebugTextVisibleOnRelease;

		Mutex m_lockRender;

		Ref<RenderDepthStencilState> m_stateCanvasDepthStencil;
		Ref<RenderBlendState> m_stateCanvasBlend;
		Ref<RenderRasterizerState> m_stateCanvasRasterizer;
		Ref<RenderSamplerState> m_stateCanvasSampler;

	};

	class SLIB_EXPORT IRenderViewInstance
	{
	public:
		virtual void setRedrawMode(RenderView* view, RedrawMode mode) = 0;

		virtual void requestRender(RenderView* view) = 0;

		virtual sl_bool isRenderEnabled(RenderView* view);

		virtual void disableRendering(RenderView* view);

	};

}

#endif
