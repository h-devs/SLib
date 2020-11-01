/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/render/d3d.h"

#include "slib/core/thread.h"
#include "slib/core/platform_windows.h"

#include "slib/render/dl_windows_d3d.h"

#if D3D_VERSION_MAJOR < 11
#define m_context m_device
#endif

namespace slib
{
	namespace priv
	{
		namespace D3D_NAMESPACE
		{

			class EngineImpl : public RenderEngine
			{
			public:
				ID3DDeviceContext* m_context;
#if D3D_VERSION_MAJOR >= 10
				ID3DRenderTargetView* m_pRenderTarget;
#endif

			public:
				EngineImpl()
				{
					m_context = sl_null;
#if D3D_VERSION_MAJOR >= 10
					m_pRenderTarget = sl_null;
#endif
				}

				~EngineImpl()
				{

				}

			public:
				RenderEngineType getEngineType() override
				{
					return RenderEngineType::D3D_TYPE;
				}

				Ref<RenderProgramInstance> _createProgramInstance(RenderProgram* program) override
				{
					return sl_null;
				}

				Ref<VertexBufferInstance> _createVertexBufferInstance(VertexBuffer* buffer) override
				{
					return sl_null;
				}

				Ref<IndexBufferInstance> _createIndexBufferInstance(IndexBuffer* buffer) override
				{
					return sl_null;
				}

				Ref<TextureInstance> _createTextureInstance(Texture* texture) override
				{
					return sl_null;
				}

				sl_bool _beginScene() override
				{
					ID3DDeviceContext* context = m_context;
					if (!context) {
						return sl_false;
					}
#if D3D_VERSION_MAJOR >= 10
					return sl_true;
#else
					HRESULT hr = context->BeginScene();
					return SUCCEEDED(hr);
#endif
				}

				void _endScene() override
				{
#if D3D_VERSION_MAJOR < 10
					ID3DDeviceContext* context = m_context;
					if (!context) {
						return;
					}
					context->EndScene();
#endif
				}

				void _setViewport(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height) override
				{
				}

				void _clear(const RenderClearParam& param) override
				{
#if D3D_VERSION_MAJOR >= 10
					ID3DDeviceContext* context = m_context;
					if (!context) {
						return;
					}
					ID3DRenderTargetView* pRenderTarget = m_pRenderTarget;
					if (!pRenderTarget) {
						return;
					}
					if (param.flagColor) {
						float c[4];
						c[0] = param.color.getRedF();
						c[1] = param.color.getGreenF();
						c[2] = param.color.getBlueF();
						c[3] = param.color.getAlphaF();
						context->ClearRenderTargetView(pRenderTarget, c);
					}
#else
					DWORD flags = 0;
					if (param.flagColor) {
						flags |= D3DCLEAR_TARGET;
					}
					if (param.flagDepth) {
						flags |= D3DCLEAR_ZBUFFER;
					}
					if (param.flagStencil) {
						flags |= D3DCLEAR_STENCIL;
					}
					m_context->Clear(0, sl_null,
						flags,
						D3DCOLOR_ARGB(param.color.a, param.color.r, param.color.g, param.color.b),
						param.depth,
						(DWORD)(param.stencil)
					);
#endif
				}

				void _setDepthTest(sl_bool flagEnableDepthTest) override
				{
				}

				void _setDepthWriteEnabled(sl_bool flagEnableDepthWrite) override
				{
				}

				void _setDepthFunction(RenderFunctionOperation op) override
				{
				}

				void _setCullFace(sl_bool flagEnableCull, sl_bool flagCullCCW) override
				{
				}

				void _setBlending(sl_bool flagEnableBlending, const RenderBlendingParam& param) override
				{
				}

				sl_bool _beginProgram(RenderProgram* program, RenderProgramInstance* instance, RenderProgramState** ppState) override
				{
					return sl_false;
				}

				void _endProgram() override
				{
				}

				void _resetCurrentBuffers() override
				{
				}

				void _drawPrimitive(EnginePrimitive* primitive) override
				{
				}

				void _applyTexture(Texture* texture, TextureInstance* instance, sl_reg sampler) override
				{
				}

				void _setLineWidth(sl_real width) override
				{
				}

			};

			class RendererImpl : public Renderer
			{
			public:
				sl_bool m_flagRequestRender;

				ID3DDevice* m_device;
#if D3D_VERSION_MAJOR >= 11
				ID3DDeviceContext* m_context;
#endif
#if D3D_VERSION_MAJOR >= 10
				IDXGISwapChain* m_pSwapChain;
				ID3DRenderTargetView* m_pRenderTarget;
#endif

				HWND m_hWnd;

				AtomicRef<Thread> m_threadRender;

			public:
				RendererImpl()
				{
					m_flagRequestRender = sl_true;

					m_device = sl_null;
#if D3D_VERSION_MAJOR >= 11
					m_context = sl_null;
#endif
#if D3D_VERSION_MAJOR >= 10
					m_pSwapChain = sl_null;
					m_pRenderTarget = sl_null;
#endif
					m_hWnd = sl_null;
				}

				~RendererImpl()
				{
					release();
				}

			public:
				static Ref<RendererImpl> create(void* windowHandle, const RendererParam& param)
				{
					HWND hWnd = (HWND)windowHandle;

#if D3D_VERSION_MAJOR >= 11
					auto funcCreateD3D = slib::d3d11::getApi_D3D11CreateDevice();
					if (funcCreateD3D) {
						ID3DDevice* device;
						ID3DDeviceContext* context;
						D3D_FEATURE_LEVEL feature;
						HRESULT hr = funcCreateD3D(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &device, (int*)&feature, &context);
						if (SUCCEEDED(hr)) {
							return create(device, param, windowHandle, sl_true);
						}
					}
#elif D3D_VERSION_MAJOR >= 10
#	if D3D_VERSION_MINOR >= 1
					auto funcCreateD3D = slib::d3d10_1::getApi_D3D10CreateDevice1();
					if (funcCreateD3D) {
						ID3DDevice* device;
						HRESULT hr = funcCreateD3D(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_FEATURE_LEVEL_10_1, D3D10_1_SDK_VERSION, &device);
						if (SUCCEEDED(hr)) {
							return create(device, param, windowHandle, sl_true);
						}
					}
#	else
					auto funcCreateD3D = slib::d3d10::getApi_D3D10CreateDevice();
					if (funcCreateD3D) {
						ID3DDevice* device;
						HRESULT hr = funcCreateD3D(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &device);
						if (SUCCEEDED(hr)) {
							return create(device, param, windowHandle, sl_true);
						}
					}
#	endif
#else
					auto funcCreateD3D = slib::d3d9::getApi_Direct3DCreate9();
					if (funcCreateD3D) {
						IDirect3D9* d3d = funcCreateD3D(D3D_SDK_VERSION);
						if (d3d) {

							ID3DDevice* device = sl_null;

							D3DPRESENT_PARAMETERS d3dpp;
							Base::zeroMemory(&d3dpp, sizeof(d3dpp));
							d3dpp.Windowed = TRUE;
							d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
							d3dpp.EnableAutoDepthStencil = TRUE;
							d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

							HRESULT hr = d3d->CreateDevice(
								D3DADAPTER_DEFAULT,
								D3DDEVTYPE_HAL,
								hWnd,
								D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
								&d3dpp,
								&device);

							d3d->Release();

							if (SUCCEEDED(hr)) {
								return create(device, param, windowHandle, sl_true);
							}

						}
					}
#endif
					return sl_null;
				}

				static Ref<RendererImpl> create(ID3DDevice* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure)
				{
					if (!device) {
						return sl_null;
					}
#if D3D_VERSION_MAJOR >= 11
					ID3DDeviceContext* context = sl_null;
					device->GetImmediateContext(&context);
					if (context) {
#endif
#if D3D_VERSION_MAJOR >= 10
						IDXGISwapChain* pSwapChain = sl_null;
						auto funcCreateDXGIFactory = dxgi::getApi_CreateDXGIFactory();
						if (funcCreateDXGIFactory) {
							IDXGIFactory* pFactory = sl_null;
							funcCreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
							if (pFactory) {
								DXGI_SWAP_CHAIN_DESC desc;
								Base::zeroMemory(&desc, sizeof(desc));
								desc.BufferCount = 1;
								desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
								desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
								desc.OutputWindow = (HWND)windowHandle;
								desc.SampleDesc.Count = 1;
								desc.Windowed = TRUE;
								pFactory->CreateSwapChain(device, &desc, &pSwapChain);
								pFactory->Release();
							}
						}
						
						if (pSwapChain) {

							ID3DRenderTargetView* pRenderTarget = sl_null;
							ID3DTexture2D* pBackBuffer = sl_null;
							pSwapChain->GetBuffer(0, __uuidof(ID3DTexture2D), (void**)&pBackBuffer);

							if (pBackBuffer) {
								device->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTarget);
								pBackBuffer->Release();
							}

							if (pRenderTarget) {
#endif
								Ref<RendererImpl> ret = new RendererImpl();

								if (ret.isNotNull()) {

									ret->m_device = device;
#if D3D_VERSION_MAJOR >= 11
									ret->m_context = context;
#endif
#if D3D_VERSION_MAJOR >= 10
									ret->m_pSwapChain = pSwapChain;
									ret->m_pRenderTarget = pRenderTarget;

									ret->m_context->OMSetRenderTargets(1, &pRenderTarget, NULL);
#endif

									ret->m_hWnd = (HWND)windowHandle;

									ret->initWithParam(param);

									ret->m_threadRender = Thread::start(SLIB_FUNCTION_MEMBER(RendererImpl, run, ret.get()));

									return ret;
								}
#if D3D_VERSION_MAJOR >= 10
								pRenderTarget->Release();
							}
							pSwapChain->Release();
						}
#endif
#if D3D_VERSION_MAJOR >= 11
						context->Release();
					}
#endif

					if (flagFreeOnFailure) {
						device->Release();
					}

					return sl_null;
				}

				void release()
				{
					ObjectLocker lock(this);

					Ref<Thread> thread = m_threadRender;
					if (thread.isNotNull()) {
						thread->finishAndWait();
						m_threadRender.setNull();
					}

#if D3D_VERSION_MAJOR >= 10
					if (m_pRenderTarget) {
						m_pRenderTarget->Release();
						m_pRenderTarget = sl_null;
					}
					if (m_pSwapChain) {
						m_pSwapChain->Release();
						m_pSwapChain = sl_null;
					}
#endif
#if D3D_VERSION_MAJOR >= 11
					if (m_context) {
						m_context->Release();
						m_context = sl_null;
					}
#endif
					if (m_device) {
						m_device->Release();
						m_device = sl_null;
					}
				}

				void run()
				{
					Ref<EngineImpl> engine = new EngineImpl;
					if (engine.isNull()) {
						return;
					}

					engine->m_context = m_context;
#if D3D_VERSION_MAJOR >= 10
					engine->m_pRenderTarget = m_pRenderTarget;
#endif

					TimeCounter timer;
					Ref<Thread> thread = Thread::getCurrent();
					while (thread.isNull() || thread->isNotStopping()) {
						Ref<RendererImpl> thiz = this;
						runStep(engine.get());
						if (thread.isNull() || thread->isNotStopping()) {
							sl_uint64 t = timer.getElapsedMilliseconds();
							if (t < 10) {
								Thread::sleep(10 - (sl_uint32)(t));
							}
							timer.reset();
						} else {
							break;
						}
					}

					engine->m_context = sl_null;
#if D3D_VERSION_MAJOR >= 10
					engine->m_pRenderTarget = sl_null;
#endif
				}

				void runStep(RenderEngine* engine)
				{
					if (m_hWnd) {
						if (!(Windows::isWindowVisible(m_hWnd))) {
							return;
						}
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
						GetClientRect(m_hWnd, &rect);
						if (rect.right != 0 && rect.bottom != 0) {
							engine->setViewport(0, 0, rect.right, rect.bottom);
							dispatchFrame(engine);
#if D3D_VERSION_MAJOR >= 10
							m_pSwapChain->Present(0, 0);
#else
							m_device->Present(sl_null, sl_null, sl_null, sl_null);
#endif
						}
					}
				}

				void requestRender()
				{
					m_flagRequestRender = sl_true;
				}

			};

			Ref<Renderer> CreateRenderer(void* windowHandle, const RendererParam& param)
			{
				return RendererImpl::create(windowHandle, param);
			}

			Ref<Renderer> CreateRenderer(ID3DDevice* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure)
			{
				return RendererImpl::create(device, param, windowHandle, flagFreeOnFailure);
			}

		}
	}
}