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
#include "slib/core/scoped.h"
#include "slib/core/log.h"
#include "slib/graphics/image.h"

#include "slib/render/dl_windows_d3d.h"

#if D3D_VERSION_MAJOR < 11
#define m_context m_device
#endif

#if D3D_VERSION_MAJOR >= 10
#define VERTEX_SHADER_TARGET "vs_4_0"
#define PIXEL_SHADER_TARGET "ps_4_0"
#else
#define VERTEX_SHADER_TARGET "vs_3_0"
#define PIXEL_SHADER_TARGET "ps_3_0"
#endif

namespace slib
{
	namespace priv
	{
		namespace D3D_NAMESPACE
		{

			class RendererImpl : public Renderer
			{
			public:
				RendererParam m_param;
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
							if (param.nDepthBits || param.nStencilBits) {
								d3dpp.EnableAutoDepthStencil = TRUE;
								if (param.nStencilBits) {
									if (param.nDepthBits == 15 && param.nStencilBits == 1) {
										d3dpp.AutoDepthStencilFormat = D3DFMT_D15S1;
									} else if (param.nDepthBits == 24 && param.nStencilBits == 4) {
										d3dpp.AutoDepthStencilFormat = D3DFMT_D24X4S4;
									} else {
										d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
									}
								} else {
									if (param.nDepthBits == 32) {
										d3dpp.AutoDepthStencilFormat = D3DFMT_D32;
									} else if (param.nDepthBits == 24) {
										d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;
									} else if (param.nDepthBits == 16) {
										d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
									}
								}
							}

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
									ret->m_param = param;

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

				void run();

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
							IDXGISwapChain* swap = m_pSwapChain;
							if (swap) {
								swap->Present(0, 0);
							}
#else
							ID3DDevice* device = m_device;
							if (device) {
								m_device->Present(sl_null, sl_null, sl_null, sl_null);
							}
#endif
						}
					}
				}

				void requestRender()
				{
					m_flagRequestRender = sl_true;
				}

			};

#if D3D_VERSION_MAJOR >= 10
			static void PrepareObjectFlags(const RenderObjectFlags& flags, D3D_(USAGE)& usage, UINT& cpuAccessFlags)
			{
				if (flags & RenderObjectFlags::CpuAccessWrite) {
					cpuAccessFlags |= D3D_(CPU_ACCESS_WRITE);
					usage = D3D_(USAGE_DYNAMIC);
				}
				if (flags & RenderObjectFlags::CpuAccessRead) {
					cpuAccessFlags |= D3D_(CPU_ACCESS_READ);
					usage = D3D_(USAGE_STAGING);
				}
			}
#endif

			class RenderInputLayoutImpl : public RenderInputLayout
			{
			public:
				ID3DInputLayout* layout;
				List<sl_uint32> strides;

			public:
				RenderInputLayoutImpl()
				{
					layout = sl_null;
				}

				~RenderInputLayoutImpl()
				{
					if (layout) {
						layout->Release();
					}
				}

			public:
				static Ref<RenderInputLayoutImpl> create(ID3DDevice* device, const Memory& codeVertexShader, const RenderInputLayoutParam& param)
				{
					ListElements<RenderInputLayoutItem> inputs(param.items);
					ID3DInputLayout* layout = sl_null;
#if D3D_VERSION_MAJOR >= 10
					if (codeVertexShader.isNull()) {
						return sl_null;
					}
					List<D3D_(INPUT_ELEMENT_DESC)> descs;
					for (sl_size i = 0; i < inputs.count; i++) {
						RenderInputLayoutItem& item = inputs[i];
						D3D_(INPUT_ELEMENT_DESC) desc = { 0 };
						switch (item.semanticName) {
						case RenderInputSemanticName::Position:
							desc.SemanticName = "POSITION";
							break;
						case RenderInputSemanticName::BlendWeight:
							desc.SemanticName = "BLENDWEIGHT";
							break;
						case RenderInputSemanticName::BlendIndices:
							desc.SemanticName = "BLENDINDICES";
							break;
						case RenderInputSemanticName::Normal:
							desc.SemanticName = "NORMAL";
							break;
						case RenderInputSemanticName::PSize:
							desc.SemanticName = "PSIZE";
							break;
						case RenderInputSemanticName::TexCoord:
							desc.SemanticName = "TEXCOORD";
							break;
						case RenderInputSemanticName::Tangent:
							desc.SemanticName = "TANGENT";
							break;
						case RenderInputSemanticName::BiNormal:
							desc.SemanticName = "BINORMAL";
							break;
						case RenderInputSemanticName::TessFactor:
							desc.SemanticName = "TESSFACTOR";
							break;
						case RenderInputSemanticName::PositionT:
							desc.SemanticName = "POSITIONT";
							break;
						case RenderInputSemanticName::Color:
							desc.SemanticName = "COLOR";
							break;
						case RenderInputSemanticName::Fog:
							desc.SemanticName = "FOG";
							break;
						case RenderInputSemanticName::Depth:
							desc.SemanticName = "DEPTH";
							break;
						default:
							desc.SemanticName = NULL;
							break;
						}
						if (desc.SemanticName) {
							desc.SemanticIndex = (UINT)(item.semanticIndex);
							switch (item.type) {
							case RenderInputType::Float:
								desc.Format = DXGI_FORMAT_R32_FLOAT;
								break;
							case RenderInputType::Float2:
								desc.Format = DXGI_FORMAT_R32G32_FLOAT;
								break;
							case RenderInputType::Float3:
								desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
								break;
							case RenderInputType::Float4:
								desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
								break;
							case RenderInputType::UByte4:
								desc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
								break;
							case RenderInputType::Short2:
								desc.Format = DXGI_FORMAT_R16G16_SINT;
								break;
							case RenderInputType::Short4:
								desc.Format = DXGI_FORMAT_R16G16B16A16_SINT;
								break;
							default:
								desc.Format = DXGI_FORMAT_UNKNOWN;
								break;
							}
							if (desc.Format != DXGI_FORMAT_UNKNOWN) {
								desc.InputSlot = (UINT)(item.slot);
								desc.AlignedByteOffset = (UINT)(item.offset);
								desc.InputSlotClass = D3D_(INPUT_PER_VERTEX_DATA);
								desc.InstanceDataStepRate = 0;
								descs.add_NoLock(desc);
							}
						}
					}
					if (descs.isEmpty()) {
						return sl_null;
					}
					device->CreateInputLayout(descs.getData(), (UINT)(descs.getCount()), codeVertexShader.getData(), (SIZE_T)(codeVertexShader.getSize()), &layout);
#else
					List<D3DVERTEXELEMENT9> elements;
					for (sl_size i = 0; i < inputs.count; i++) {
						RenderInputLayoutItem& item = inputs[i];
						D3DVERTEXELEMENT9 element = { 0 };
						sl_bool flagValidUsage = sl_true;
						switch (item.semanticName) {
						case RenderInputSemanticName::Position:
							element.Usage = D3DDECLUSAGE_POSITION;
							break;
						case RenderInputSemanticName::BlendWeight:
							element.Usage = D3DDECLUSAGE_BLENDWEIGHT;
							break;
						case RenderInputSemanticName::BlendIndices:
							element.Usage = D3DDECLUSAGE_BLENDINDICES;
							break;
						case RenderInputSemanticName::Normal:
							element.Usage = D3DDECLUSAGE_NORMAL;
							break;
						case RenderInputSemanticName::PSize:
							element.Usage = D3DDECLUSAGE_PSIZE;
							break;
						case RenderInputSemanticName::TexCoord:
							element.Usage = D3DDECLUSAGE_TEXCOORD;
							break;
						case RenderInputSemanticName::Tangent:
							element.Usage = D3DDECLUSAGE_TANGENT;
							break;
						case RenderInputSemanticName::BiNormal:
							element.Usage = D3DDECLUSAGE_BINORMAL;
							break;
						case RenderInputSemanticName::TessFactor:
							element.Usage = D3DDECLUSAGE_TESSFACTOR;
							break;
						case RenderInputSemanticName::PositionT:
							element.Usage = D3DDECLUSAGE_POSITIONT;
							break;
						case RenderInputSemanticName::Color:
							element.Usage = D3DDECLUSAGE_COLOR;
							break;
						case RenderInputSemanticName::Fog:
							element.Usage = D3DDECLUSAGE_FOG;
							break;
						case RenderInputSemanticName::Depth:
							element.Usage = D3DDECLUSAGE_DEPTH;
							break;
						default:
							flagValidUsage = sl_false;
							break;
						}
						if (flagValidUsage) {
							element.UsageIndex = (BYTE)(element.UsageIndex);
							switch (item.type) {
							case RenderInputType::Float:
								element.Type = D3DDECLTYPE_FLOAT1;
								break;
							case RenderInputType::Float2:
								element.Type = D3DDECLTYPE_FLOAT2;
								break;
							case RenderInputType::Float3:
								element.Type = D3DDECLTYPE_FLOAT3;
								break;
							case RenderInputType::Float4:
								element.Type = D3DDECLTYPE_FLOAT4;
								break;
							case RenderInputType::UByte4:
								element.Type = D3DDECLTYPE_UBYTE4;
								break;
							case RenderInputType::Short2:
								element.Type = D3DDECLTYPE_SHORT2;
								break;
							case RenderInputType::Short4:
								element.Type = D3DDECLTYPE_SHORT4;
								break;
							default:
								element.Type = D3DDECLTYPE_UNUSED;
								break;
							}
							if (element.Type != D3DDECLTYPE_UNUSED) {
								element.Method = D3DDECLMETHOD_DEFAULT;
								element.Offset = (WORD)(item.offset);
								element.Stream = (WORD)(item.slot);
								elements.add_NoLock(element);
							}
						}
					}
					if (elements.isEmpty()) {
						return sl_null;
					}
					D3DVERTEXELEMENT9 element = D3DDECL_END();
					elements.add_NoLock(element);
					device->CreateVertexDeclaration(elements.getData(), &layout);
#endif
					if (!layout) {
						return sl_null;
					}
					Ref<RenderInputLayoutImpl> ret = new RenderInputLayoutImpl;
					if (ret.isNotNull()) {
						ret->layout = layout;
						ret->strides = param.strides.toList();
						return ret;
					}
					return sl_null;
				}

			};

			static Memory CompileShader(const String& str, const char* target)
			{
				if (str.isEmpty()) {
					return sl_null;
				}
#if D3D_VERSION_MAJOR >= 10
#if D3D_VERSION_MAJOR >= 11
				auto func = slib::d3d_compiler::getApi_D3DCompile();
#else
				auto func = slib::d3dx10::getApi_D3DX10CompileFromMemory();
#endif
				if (!func) {
					return sl_null;
				}
				ID3D10Blob* blob = sl_null;
#ifdef SLIB_DEBUG
				ID3D10Blob* error = sl_null;
#if D3D_VERSION_MAJOR >= 11
				HRESULT hr = func(str.getData(), (SIZE_T)(str.getLength()), NULL, NULL, NULL, "main", target, D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, &blob, &error);
#else
				HRESULT hr = func(str.getData(), (SIZE_T)(str.getLength()), NULL, NULL, NULL, "main", target, D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, NULL, &blob, &error, NULL);
#endif
#else
#if D3D_VERSION_MAJOR >= 11
				func(str.getData(), (SIZE_T)(str.getLength()), NULL, NULL, NULL, "main", target, D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, &blob, NULL);
#else
				func(str.getData(), (SIZE_T)(str.getLength()), NULL, NULL, NULL, "main", target, D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, NULL, &blob, NULL, NULL);
#endif
#endif
				if (blob) {
					Memory ret = Memory::create(blob->GetBufferPointer(), (sl_size)(blob->GetBufferSize()));
					blob->Release();
					return ret;
				}
#else
				auto func = slib::d3dx9::getApi_D3DXCompileShader();
				if (!func) {
					return sl_null;
				}
				ID3DXBuffer* shader = sl_null;
#ifdef SLIB_DEBUG
				ID3DXBuffer* error = sl_null;
				HRESULT hr = func(str.getData(), (UINT)(str.getLength()), NULL, NULL, "main", target, 0, &shader, &error, NULL);
#else
				func(str.getData(), (UINT)(str.getLength()), NULL, NULL, "main", target, 0, &shader, NULL, NULL);
#endif
				if (shader) {
					Memory ret = Memory::create(shader->GetBufferPointer(), (sl_size)(shader->GetBufferSize()));
					shader->Release();
					return ret;
				}
#endif
#ifdef SLIB_DEBUG
				else {
					if (error) {
						SLIB_LOG_DEBUG("D3DCompileError", "hr=%d, %s", (sl_reg)hr, StringView((char*)(error->GetBufferPointer()), error->GetBufferSize()));
						error->Release();
					} else {
						SLIB_LOG_DEBUG("D3DCompileError", "hr=%d", (sl_reg)hr);
					}
				}
#endif
				return sl_null;
			}

			class RenderProgramInstanceImpl : public RenderProgramInstance
			{
			public:
				ID3DDevice * device;
				ID3DDeviceContext* context;

				ID3DVertexShader* vertexShader;
				ID3DPixelShader* pixelShader;

#if D3D_VERSION_MAJOR >= 10
				Memory codeVertexShader;
				ID3DBuffer* constantBuffer[2];
				Memory memConstantBuffer[2];
				sl_uint8* pConstantBuffer[2];
				sl_uint32 sizeConstantBuffer[2];
				sl_bool flagUpdatedConstantBuffer[2];
#endif

				Ref<RenderProgramState> state;

			public:
				RenderProgramInstanceImpl()
				{
					device = sl_null;
					context = sl_null;
					vertexShader = sl_null;
					pixelShader = sl_null;
#if D3D_VERSION_MAJOR >= 10
					constantBuffer[0] = sl_null;
					pConstantBuffer[0] = sl_null;
					sizeConstantBuffer[0] = 0;
					constantBuffer[1] = sl_null;
					pConstantBuffer[1] = sl_null;
					sizeConstantBuffer[1] = 0;
#endif
				}

				~RenderProgramInstanceImpl()
				{
					if (vertexShader) {
						vertexShader->Release();
					}
					if (pixelShader) {
						pixelShader->Release();
					}
#if D3D_VERSION_MAJOR >= 10
					if (constantBuffer[0]) {
						constantBuffer[0]->Release();
					}
					if (constantBuffer[1]) {
						constantBuffer[1]->Release();
					}
#endif
				}

			public:
				static Ref<RenderProgramInstanceImpl> create(ID3DDevice* device, ID3DDeviceContext* context, RenderEngine* engine, RenderProgram* program)
				{

					Memory codeVertex = program->getHLSLCompiledVertexShader(engine);
					if (codeVertex.isNull()) {
						codeVertex = CompileShader(program->getHLSLVertexShader(engine), VERTEX_SHADER_TARGET);
						if (codeVertex.isNull()) {
							return sl_null;
						}
					}
					Memory codePixel = program->getHLSLCompiledPixelShader(engine);
					if (codePixel.isNull()) {
						codePixel = CompileShader(program->getHLSLPixelShader(engine), PIXEL_SHADER_TARGET);
						if (codePixel.isNull()) {
							return sl_null;
						}
					}
					ID3DVertexShader* vs = sl_null;
#if D3D_VERSION_MAJOR >= 11
					device->CreateVertexShader(codeVertex.getData(), (SIZE_T)(codeVertex.getSize()), NULL, &vs);
#elif D3D_VERSION_MAJOR >= 10
					device->CreateVertexShader(codeVertex.getData(), (SIZE_T)(codeVertex.getSize()), &vs);
#else
					device->CreateVertexShader((DWORD*)(codeVertex.getData()), &vs);
#endif
					if (vs) {
						ID3DPixelShader* ps = sl_null;
#if D3D_VERSION_MAJOR >= 11
						device->CreatePixelShader(codePixel.getData(), (SIZE_T)(codePixel.getSize()), NULL, &ps);
#elif D3D_VERSION_MAJOR >= 10
						device->CreatePixelShader(codePixel.getData(), (SIZE_T)(codePixel.getSize()), &ps);
#else
						device->CreatePixelShader((DWORD*)(codePixel.getData()), &ps);
#endif
						if (ps) {
#if D3D_VERSION_MAJOR >= 10
							Memory memConstantBuffer[2];
							ID3DBuffer* constantBuffer[2];
							if (createConstantBuffers(device, program, constantBuffer, memConstantBuffer)) {
#endif
								Ref<RenderProgramState> state = program->onCreate(engine);
								if (state.isNotNull()) {
									Ref<RenderProgramInstanceImpl> ret = new RenderProgramInstanceImpl();
									if (ret.isNotNull()) {
										ret->device = device;
										ret->context = context;
										ret->vertexShader = vs;
										ret->pixelShader = ps;
#if D3D_VERSION_MAJOR >= 10
										ret->constantBuffer[0] = constantBuffer[0];
										ret->pConstantBuffer[0] = (sl_uint8*)(memConstantBuffer[0].getData());
										ret->sizeConstantBuffer[0] = (sl_uint32)(memConstantBuffer[0].getSize());
										ret->memConstantBuffer[0] = Move(memConstantBuffer[0]);
										ret->constantBuffer[1] = constantBuffer[1];
										ret->pConstantBuffer[1] = (sl_uint8*)(memConstantBuffer[1].getData());
										ret->sizeConstantBuffer[1] = (sl_uint32)(memConstantBuffer[1].getSize());
										ret->memConstantBuffer[1] = Move(memConstantBuffer[1]);
#endif
										state->setProgramInstance(ret.get());
										if (program->onInit(engine, ret.get(), state.get())) {
											ret->state = state;
#if D3D_VERSION_MAJOR >= 10
											ret->codeVertexShader = codeVertex;
#endif
											ret->link(engine, program);
											return ret;
										}
										return sl_null;
									}
								}
#if D3D_VERSION_MAJOR >= 10
								constantBuffer[0]->Release();
								constantBuffer[1]->Release();
							}
#endif
							ps->Release();
						}
						vs->Release();
					}
					return sl_null;
				}

				Ref<RenderInputLayout> createInputLayout(const RenderInputLayoutParam& param) override
				{
#if D3D_VERSION_MAJOR >= 10
					return Ref<RenderInputLayout>::from(RenderInputLayoutImpl::create(device, codeVertexShader, param));
#else
					return Ref<RenderInputLayout>::from(RenderInputLayoutImpl::create(device, sl_null, param));
#endif
				}

				sl_bool getUniformLocation(const char* name, RenderUniformLocation* outLocation) override
				{
					return sl_false;
				}

				void setUniform(const RenderUniformLocation& location, RenderUniformType type, const void* _data, sl_uint32 nItems) override
				{
#if D3D_VERSION_MAJOR >= 10
					sl_uint32 indexConstant;
					if (location.shader == RenderShaderType::Vertex) {
						indexConstant = 0;
					} else if (location.shader == RenderShaderType::Pixel) {
						indexConstant = 1;
					} else {
						return;
					}
					sl_uint32 loc = (sl_uint32)(location.location << 4);
					if (loc >= sizeConstantBuffer[indexConstant]) {
						return;
					}
#endif
					const void* data = _data;
					float temp[16];
					Memory memTemp;
					if (type == RenderUniformType::Matrix3) {
						type = RenderUniformType::Float;
						if (nItems == 1) {
							data = temp;
							Matrix3& m = *((Matrix3*)_data);
							temp[0] = m.m00;
							temp[1] = m.m01;
							temp[2] = m.m02;
							temp[3] = 0;
							temp[4] = m.m10;
							temp[5] = m.m11;
							temp[6] = m.m12;
							temp[7] = 0;
							temp[8] = m.m20;
							temp[9] = m.m21;
							temp[10] = m.m22;
							temp[11] = 0;
							nItems = 12;
						} else {
							sl_uint32 n = nItems;
							nItems = n * 12;
							memTemp = Memory::create(nItems << 2);
							if (memTemp.isNull()) {
								return;
							}
							float* t = (float*)(memTemp.getData());
							data = t;
							Matrix3* m = (Matrix3*)_data;
							for (sl_uint32 i = 0; i < n; i++) {
								t[0] = m->m00;
								t[1] = m->m01;
								t[2] = m->m02;
								t[3] = 0;
								t[4] = m->m10;
								t[5] = m->m11;
								t[6] = m->m12;
								t[7] = 0;
								t[8] = m->m20;
								t[9] = m->m21;
								t[10] = m->m22;
								t[11] = 0;
								t += 12;
								m++;
							}
						}
					} else if (type == RenderUniformType::Float3 || type == RenderUniformType::Int3) {
						if (type == RenderUniformType::Float3) {
							type = RenderUniformType::Float;
						} else {
							type = RenderUniformType::Int;
						}
						float* s = (float*)_data;
						if (nItems == 1) {
							data = temp;
							temp[0] = s[0];
							temp[1] = s[1];
							temp[2] = s[2];
							temp[3] = 0;
							nItems = 4;
						} else {
							sl_uint32 n = nItems;
							nItems = n * 4;
							memTemp = Memory::create(nItems << 2);
							if (memTemp.isNull()) {
								return;
							}
							float* t = (float*)(memTemp.getData());
							data = t;
							for (sl_uint32 i = 0; i < n; i++) {
								t[0] = s[0];
								t[1] = s[1];
								t[2] = s[2];
								t[3] = 0;
								t += 4;
								s += 3;
							}
						}
					}
#if D3D_VERSION_MAJOR >= 10
					else {
						switch (type) {
						case RenderUniformType::Float:
							break;
						case RenderUniformType::Float2:
							nItems *= 2;
							break;
						case RenderUniformType::Float4:
							nItems *= 4;
							break;
						case RenderUniformType::Matrix4:
							nItems *= 16;
							break;
						case RenderUniformType::Int:
							break;
						case RenderUniformType::Int2:
							nItems *= 2;
							break;
						case RenderUniformType::Int4:
							nItems *= 4;
							break;
						case RenderUniformType::Sampler:
							break;
						default:
							return;
						}
					}
					sl_uint32 size = nItems << 2;
					sl_uint32 limit = sizeConstantBuffer[indexConstant] - loc;
					if (size > limit) {
						size = limit;
					}
					if (!size) {
						return;
					}
					Base::copyMemory(pConstantBuffer[indexConstant] + loc, data, size);
					flagUpdatedConstantBuffer[indexConstant] = sl_true;
#else
					switch (type) {
					case RenderUniformType::Float:
						break;
					case RenderUniformType::Float2:
						type = RenderUniformType::Float;
						nItems *= 2;
						break;
					case RenderUniformType::Float4:
						type = RenderUniformType::Float;
						nItems *= 4;
						break;
					case RenderUniformType::Matrix4:
						type = RenderUniformType::Float;
						nItems *= 16;
						break;
					case RenderUniformType::Int:
						break;
					case RenderUniformType::Int2:
						type = RenderUniformType::Int;
						nItems *= 2;
						break;
					case RenderUniformType::Int4:
						type = RenderUniformType::Int;
						nItems *= 4;
						break;
					case RenderUniformType::Sampler:
						type = RenderUniformType::Int;
						break;
					default:
						return;
					}
					sl_uint n4 = nItems >> 2;
					if (n4) {
						_setUniform(device, location, type, data, n4);
					}
					nItems = nItems & 3;
					if (nItems) {
						sl_uint8 t[16] = { 0 };
						Base::copyMemory(t, data, nItems << 2);
						RenderUniformLocation l = location;
						l.location += n4;
						_setUniform(device, l, type, t, 1);
					}
#endif
				}

#if D3D_VERSION_MAJOR >= 10
				static sl_bool createConstantBuffers(ID3DDevice* device, RenderProgram* program, ID3DBuffer* buffer[2], Memory memConstantBuffer[2])
				{
					sl_uint32 size[2];
					size[0] = program->getVertexShaderConstantBufferSize();
					if (!(size[0])) {
						return sl_null;
					}
					size[1] = program->getPixelShaderConstantBufferSize();
					if (!(size[1])) {
						return sl_null;
					}
					sl_uint32 i = 0;
					for (; i < 2; i++) {
						Memory mem = Memory::create(size[i]);
						if (mem.isNull()) {
							break;
						}
						Base::zeroMemory(mem.getData(), mem.getSize());

						D3D_(BUFFER_DESC) desc = { 0 };
						desc.BindFlags = D3D_(BIND_VERTEX_BUFFER);
						desc.ByteWidth = size[i];

						D3D_(SUBRESOURCE_DATA) data = { 0 };
						data.pSysMem = mem.getData();

						ID3DBuffer* buf = sl_null;
						device->CreateBuffer(&desc, &data, &buf);
						if (!buf) {
							break;
						}
						memConstantBuffer[i] = Move(mem);
						buffer[i] = buf;
					}
					if (i < 2) {
						buffer[0]->Release();
						buffer[0] = sl_null;
						return sl_false;
					}
					return sl_true;
				}

				void applyConstantBuffer(ID3DDeviceContext* context)
				{
					if (flagUpdatedConstantBuffer[0]) {
						context->UpdateSubresource(constantBuffer[0], 0, NULL, pConstantBuffer[0], 0, 0);
						flagUpdatedConstantBuffer[0] = sl_false;
					}
					context->VSSetConstantBuffers(0, 1, constantBuffer);
					if (flagUpdatedConstantBuffer[1]) {
						context->UpdateSubresource(constantBuffer[1], 0, NULL, pConstantBuffer[1], 0, 0);
						flagUpdatedConstantBuffer[1] = sl_false;
					}
					context->PSSetConstantBuffers(0, 1, constantBuffer + 1);
				}
#else
				static void _setUniform(ID3DDevice* dev, const RenderUniformLocation& location, RenderUniformType type, const void* data, sl_uint32 countVector4)
				{
					if (location.shader == RenderShaderType::Vertex) {
						if (type == RenderUniformType::Float) {
							dev->SetVertexShaderConstantF((UINT)location.location, (float*)data, (UINT)countVector4);
						} else if (type == RenderUniformType::Int) {
							dev->SetVertexShaderConstantI((UINT)location.location, (int*)data, (UINT)countVector4);
						}
					} else if (location.shader == RenderShaderType::Pixel) {
						if (type == RenderUniformType::Float) {
							dev->SetPixelShaderConstantF((UINT)location.location, (float*)data, (UINT)countVector4);
						} else if (type == RenderUniformType::Int) {
							dev->SetPixelShaderConstantI((UINT)location.location, (int*)data, (UINT)countVector4);
						}
					}
				}
#endif

			};

			class VertexBufferInstanceImpl : public VertexBufferInstance
			{
			public:
				ID3DDevice* device;
				ID3DDeviceContext* context;
				ID3DVertexBuffer* pVB;

			public:
				VertexBufferInstanceImpl()
				{
					device = sl_null;
					context = sl_null;
					pVB = sl_null;
				}

				~VertexBufferInstanceImpl()
				{
					if (pVB) {
						pVB->Release();
					}
				}

			public:
				static Ref<VertexBufferInstanceImpl> create(ID3DDevice* device, ID3DDeviceContext* context, RenderEngine* engine, VertexBuffer* buffer)
				{
					UINT size = (UINT)(buffer->getSize());
					if (!size) {
						return sl_null;
					}
					Memory content = buffer->getSource();
					if (content.getSize() < size) {
						return sl_null;
					}
					ID3DVertexBuffer* vb = sl_null;
#if D3D_VERSION_MAJOR >= 10
					D3D_(BUFFER_DESC) desc = { 0 };
					desc.BindFlags = D3D_(BIND_VERTEX_BUFFER);
					desc.ByteWidth = size;
					PrepareObjectFlags(buffer->getFlags(), desc.Usage, desc.CPUAccessFlags);
					D3D_(SUBRESOURCE_DATA) data = { 0 };
					data.pSysMem = content.getData();
					device->CreateBuffer(&desc, &data, &vb);
#else
					device->CreateVertexBuffer(size, 0, 0, D3DPOOL_MANAGED, &vb, NULL);
#endif
					if (vb) {
#if D3D_VERSION_MAJOR < 10
						void* data = sl_null;
						vb->Lock(0, size, &data, 0);
						if (data) {
							Base::copyMemory(data, content.getData(), size);
							vb->Unlock();
#endif
							Ref<VertexBufferInstanceImpl> ret = new VertexBufferInstanceImpl;
							if (ret.isNotNull()) {
								ret->device = device;
								ret->context = context;
								ret->pVB = vb;
								ret->link(engine, buffer);
								return ret;
							}
#if D3D_VERSION_MAJOR < 10
						}
#endif
						vb->Release();
					}
					return sl_null;
				}

				void onUpdate(RenderBaseObject* object) override
				{
					UINT offset = (UINT)m_updatedOffset;
					UINT size = (UINT)m_updatedSize;
					if (!size) {
						return;
					}
					VertexBuffer* buffer = (VertexBuffer*)object;
					Memory content = buffer->getSource();
					if (content.getSize() < offset + size) {
						return;
					}
					void* data = sl_null;
#if D3D_VERSION_MAJOR >= 11
					D3D_(MAPPED_SUBRESOURCE) res = { 0 };
					context->Map(pVB, 0, D3D_(MAP_WRITE), 0, &res);
					data = res.pData;
#elif D3D_VERSION_MAJOR >= 10
					pVB->Map(D3D_(MAP_WRITE), 0, &data);
#else
					pVB->Lock(offset, size, &data, 0);
#endif
					if (data) {
						Base::copyMemory(data, (sl_uint8*)(content.getData()) + offset, size);
#if D3D_VERSION_MAJOR >= 11
						context->Unmap(pVB, 0);
#elif D3D_VERSION_MAJOR >= 10
						pVB->Unmap();
#else
						pVB->Unlock();
#endif
					}
				}

				sl_bool canUpdate() override
				{
					return sl_true;
				}

			};

			class IndexBufferInstanceImpl : public IndexBufferInstance
			{
			public:
				ID3DDevice* device;
				ID3DDeviceContext* context;
				ID3DIndexBuffer * pIB;

			public:
				IndexBufferInstanceImpl()
				{
					device = sl_null;
					context = sl_null;
					pIB = sl_null;
				}

				~IndexBufferInstanceImpl()
				{
					if (pIB) {
						pIB->Release();
					}
				}

			public:
				static Ref<IndexBufferInstanceImpl> create(ID3DDevice* device, ID3DDeviceContext* context, RenderEngine* engine, IndexBuffer* buffer)
				{
					UINT size = (UINT)(buffer->getSize());
					if (!size) {
						return sl_null;
					}
					Memory content = buffer->getSource();
					if (content.getSize() < size) {
						return sl_null;
					}
					ID3DIndexBuffer* ib = sl_null;
#if D3D_VERSION_MAJOR >= 10
					D3D_(BUFFER_DESC) desc = { 0 };
					desc.BindFlags = D3D_(BIND_INDEX_BUFFER);
					desc.ByteWidth = size;
					PrepareObjectFlags(buffer->getFlags(), desc.Usage, desc.CPUAccessFlags);
					D3D_(SUBRESOURCE_DATA) data = { 0 };
					data.pSysMem = content.getData();
					device->CreateBuffer(&desc, &data, &ib);
#else
					device->CreateIndexBuffer(size, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ib, NULL);
#endif
					if (ib) {
#if D3D_VERSION_MAJOR < 10
						void* data = sl_null;
						ib->Lock(0, size, &data, 0);
						if (data) {
							Base::copyMemory(data, content.getData(), size);
							ib->Unlock();
#endif
							Ref<IndexBufferInstanceImpl> ret = new IndexBufferInstanceImpl;
							if (ret.isNotNull()) {
								ret->device = device;
								ret->context = context;
								ret->pIB = ib;
								ret->link(engine, buffer);
								return ret;
							}
#if D3D_VERSION_MAJOR < 10
						}
#endif
						ib->Release();
					}
					return sl_null;
				}

				void onUpdate(RenderBaseObject* object) override
				{
					UINT offset = (UINT)m_updatedOffset;
					UINT size = (UINT)m_updatedSize;
					if (!size) {
						return;
					}
					IndexBuffer* buffer = (IndexBuffer*)object;
					Memory content = buffer->getSource();
					if (content.getSize() < offset + size) {
						return;
					}
					void* data = sl_null;
#if D3D_VERSION_MAJOR >= 11
					D3D_(MAPPED_SUBRESOURCE) res = { 0 };
					context->Map(pIB, 0, D3D_(MAP_WRITE), 0, &res);
					data = res.pData;
#elif D3D_VERSION_MAJOR >= 10
					pIB->Map(D3D_(MAP_WRITE), 0, &data);
#else
					pIB->Lock(offset, size, &data, 0);
#endif
					if (data) {
						Base::copyMemory(data, (sl_uint8*)(content.getData()) + offset, size);
#if D3D_VERSION_MAJOR >= 11
						context->Unmap(pIB, 0);
#elif D3D_VERSION_MAJOR >= 10
						pIB->Unmap();
#else
						pIB->Unlock();
#endif
					}
				}

				sl_bool canUpdate() override
				{
					return sl_true;
				}

			};

			class TextureInstanceImpl : public TextureInstance
			{
			public:
				ID3DDevice* device;
				ID3DDeviceContext* context;
				ID3DTexture2D* pTexture;

			public:
				TextureInstanceImpl()
				{
					device = sl_null;
					context = sl_null;
					pTexture = sl_null;
				}

				~TextureInstanceImpl()
				{
					if (pTexture) {
						pTexture->Release();
					}
				}

			public:
				static Ref<TextureInstanceImpl> create(ID3DDevice* device, ID3DDeviceContext* context, RenderEngine* engine, Texture* texture)
				{
					Ref<Bitmap> source = texture->getSource();
					if (source.isNull()) {
						return sl_null;
					}
					sl_uint32 width = source->getWidth();
					if (!width) {
						return sl_null;
					}
					sl_uint32 height = source->getHeight();
					if (!height) {
						return sl_null;
					}
					ID3DTexture2D* pTexture = sl_null;
#if D3D_VERSION_MAJOR >= 10
					Ref<Image> image = source->toImage();
					if (image.isNull()) {
						return sl_null;
					}
					D3D_(TEXTURE2D_DESC) desc = { 0 };
					desc.Width = (UINT)width;
					desc.Height = (UINT)height;
					desc.MipLevels = 1;
					desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					desc.SampleDesc.Count = 1;
					desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
					PrepareObjectFlags(texture->getFlags(), desc.Usage, desc.CPUAccessFlags);
					D3D_(SUBRESOURCE_DATA) data = { 0 };
					data.pSysMem = image->getColors();
					data.SysMemPitch = (UINT)(image->getStride() << 2);
					device->CreateTexture2D(&desc, &data, &pTexture);
#else
					device->CreateTexture((UINT)width, (UINT)height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, NULL);
#endif
					if (pTexture) {
#if D3D_VERSION_MAJOR < 10
						D3DLOCKED_RECT lr;
						HRESULT hr = pTexture->LockRect(0, &lr, NULL, 0);
						if (hr == D3D_OK) {
							BitmapData bd;
							bd.format = BitmapFormat::ARGB;
							bd.data = lr.pBits;
							bd.pitch = (sl_int32)(lr.Pitch);
							bd.width = width;
							bd.height = height;
							source->readPixels(0, 0, bd);
							pTexture->UnlockRect(0);
#endif
							Ref<TextureInstanceImpl> ret = new TextureInstanceImpl();
							if (ret.isNotNull()) {
								ret->device = device;
								ret->context = context;
								ret->pTexture = pTexture;
								ret->link(engine, texture);
								return ret;
							}
#if D3D_VERSION_MAJOR < 10
						}
#endif
						pTexture->Release();
					}
					return sl_null;
				}

				void onUpdate(RenderBaseObject* object) override
				{
					Texture* texture = (Texture*)object;
					Ref<Bitmap> source = texture->getSource();
					if (source.isNull()) {
						return;
					}
#if D3D_VERSION_MAJOR >= 11
					D3D_(MAPPED_SUBRESOURCE) res = { 0 };
					context->Map(pTexture, 0, D3D_(MAP_WRITE), 0, &res);
					void* data = res.pData;
					sl_int32 pitch = (sl_int32)(res.RowPitch);
#elif D3D_VERSION_MAJOR >= 10
					D3D_(MAPPED_TEXTURE2D) mt = { 0 };
					pTexture->Map(0, D3D_(MAP_WRITE), 0, &mt);
					void* data = mt.pData;
					sl_int32 pitch = (sl_int32)(mt.RowPitch);
#else
					D3DLOCKED_RECT lr;
					RECT rc;
					rc.left = (LONG)(m_updatedRegion.left);
					rc.top = (LONG)(m_updatedRegion.top);
					rc.right = (LONG)(m_updatedRegion.right);
					rc.bottom = (LONG)(m_updatedRegion.bottom);
					pTexture->LockRect(0, &lr, &rc, 0);
					void* data = lr.pBits;
					sl_int32 pitch = (sl_int32)(lr.Pitch);
#endif
					if (data) {
						BitmapData bd;
						bd.format = BitmapFormat::ARGB;
						bd.data = data;
						bd.pitch = pitch;
						bd.width = m_updatedRegion.getWidth();
						bd.height = m_updatedRegion.getHeight();
						source->readPixels(m_updatedRegion.left, m_updatedRegion.top, bd);
#if D3D_VERSION_MAJOR >= 11
						context->Unmap(pTexture, 0);
#elif D3D_VERSION_MAJOR >= 10
#else
						pTexture->UnlockRect(0);
#endif
					}
				}

				sl_bool canUpdate() override
				{
					return sl_true;
				}

			};

			class EngineImpl : public RenderEngine
			{
			public:
				RendererImpl * m_renderer;

				Ref<RenderProgram> m_currentProgram;
				Ref<RenderProgramInstanceImpl> m_currentProgramInstance;
				Ref<RenderInputLayoutImpl> m_currentInputLayout;
				sl_uint32 m_currentVertexStride;
				Ref<VertexBufferInstanceImpl> m_currentVertexBufferInstance;
				Ref<IndexBufferInstanceImpl> m_currentIndexBufferInstance;
				Ref<RenderProgram> m_currentProgramRendering;
				Ref<RenderProgramInstanceImpl> m_currentProgramInstanceRendering;

#if D3D_VERSION_MAJOR >= 10
				ID3DDepthStencilState* m_stateDepthStencil;
				D3D_(DEPTH_STENCIL_DESC) m_stateDescDepthStencil;
				sl_bool m_flagUpdatedDepthStencilState;
				UINT m_stencilRef;
				sl_bool m_flagUpdatedDepthStencilStateStencilRef;

				ID3DRasterizerState* m_stateRasterizer;
				D3D_(RASTERIZER_DESC) m_stateDescRasterizer;
				sl_bool m_flagUpdatedRasterizerState;

				ID3DBlendState* m_stateBlend;
				D3D_(BLEND_DESC) m_stateDescBlend;
				sl_bool m_flagUpdatedBlendState;
				float m_blendFactor[4];
				sl_bool m_flagUpdatedBlendStateBlendFactor;
#endif

			public:
				EngineImpl()
				{
					m_renderer = sl_null;

					m_currentVertexStride = 0;

#if D3D_VERSION_MAJOR >= 10
					m_stateDepthStencil = sl_null;
					m_stateDescDepthStencil.DepthEnable = TRUE;
					m_stateDescDepthStencil.DepthWriteMask = D3D_(DEPTH_WRITE_MASK_ALL);
					m_stateDescDepthStencil.DepthFunc = D3D_(COMPARISON_LESS);
					m_stateDescDepthStencil.StencilEnable = FALSE;
					m_stateDescDepthStencil.StencilReadMask = D3D_(DEFAULT_STENCIL_READ_MASK);
					m_stateDescDepthStencil.StencilWriteMask = D3D_(DEFAULT_STENCIL_WRITE_MASK);
					m_stateDescDepthStencil.FrontFace.StencilFailOp = D3D_(STENCIL_OP_KEEP);
					m_stateDescDepthStencil.FrontFace.StencilDepthFailOp = D3D_(STENCIL_OP_KEEP);
					m_stateDescDepthStencil.FrontFace.StencilPassOp = D3D_(STENCIL_OP_KEEP);
					m_stateDescDepthStencil.FrontFace.StencilFunc = D3D_(COMPARISON_ALWAYS);
					m_stateDescDepthStencil.BackFace.StencilFailOp = D3D_(STENCIL_OP_KEEP);
					m_stateDescDepthStencil.BackFace.StencilDepthFailOp = D3D_(STENCIL_OP_KEEP);
					m_stateDescDepthStencil.BackFace.StencilPassOp = D3D_(STENCIL_OP_KEEP);
					m_stateDescDepthStencil.BackFace.StencilFunc = D3D_(COMPARISON_ALWAYS);
					m_flagUpdatedDepthStencilState = sl_false;
					m_stencilRef = 0;
					m_flagUpdatedDepthStencilStateStencilRef = sl_false;

					m_stateRasterizer = sl_null;
					m_stateDescRasterizer.FillMode = D3D_(FILL_SOLID);
					m_stateDescRasterizer.CullMode = D3D_(CULL_BACK);
					m_stateDescRasterizer.FrontCounterClockwise = FALSE;
					m_stateDescRasterizer.DepthBias = 0;
					m_stateDescRasterizer.DepthBiasClamp = 0.0f;
					m_stateDescRasterizer.SlopeScaledDepthBias = 0.0f;
					m_stateDescRasterizer.DepthClipEnable = TRUE;
					m_stateDescRasterizer.ScissorEnable = FALSE;
					m_stateDescRasterizer.MultisampleEnable = FALSE;
					m_stateDescRasterizer.AntialiasedLineEnable = FALSE;
					m_flagUpdatedRasterizerState = sl_false;

					m_stateBlend = sl_null;
					Base::zeroMemory(&m_stateDescBlend, sizeof(m_stateDescBlend));
#if D3D_VERSION_MAJOR >= 11
					{
						for (sl_uint k = 0; k < 8; k++) {
							m_stateDescBlend.RenderTarget[k].DestBlend = D3D_(BLEND_ZERO);
							m_stateDescBlend.RenderTarget[k].BlendOp = D3D_(BLEND_OP_ADD);
							m_stateDescBlend.RenderTarget[k].SrcBlendAlpha = D3D_(BLEND_ONE);
							m_stateDescBlend.RenderTarget[k].DestBlendAlpha = D3D_(BLEND_ZERO);
							m_stateDescBlend.RenderTarget[k].BlendOpAlpha = D3D_(BLEND_OP_ADD);
							m_stateDescBlend.RenderTarget[k].RenderTargetWriteMask = D3D_(COLOR_WRITE_ENABLE_ALL);
						}
					}
#else
					m_stateDescBlend.SrcBlend = D3D_(BLEND_ONE);
					m_stateDescBlend.DestBlend = D3D_(BLEND_ZERO);
					m_stateDescBlend.BlendOp = D3D_(BLEND_OP_ADD);
					m_stateDescBlend.SrcBlendAlpha = D3D_(BLEND_ONE);
					m_stateDescBlend.DestBlendAlpha = D3D_(BLEND_ZERO);
					m_stateDescBlend.BlendOpAlpha = D3D_(BLEND_OP_ADD);
					{
						for (sl_uint k = 0; k < 8; k++) {
							m_stateDescBlend.RenderTargetWriteMask[k] = D3D_(COLOR_WRITE_ENABLE_ALL);
						}
					}
#endif
					m_flagUpdatedBlendState = sl_false;
					m_blendFactor[0] = 0;
					m_blendFactor[1] = 0;
					m_blendFactor[2] = 0;
					m_blendFactor[3] = 0;
					m_flagUpdatedBlendStateBlendFactor = sl_false;
#endif
				}

				~EngineImpl()
				{
#if D3D_VERSION_MAJOR >= 10
					if (m_stateDepthStencil) {
						m_stateDepthStencil->Release();
					}
					if (m_stateRasterizer) {
						m_stateRasterizer->Release();
					}
#endif
				}

			public:
				SLIB_INLINE RendererParam* getRendererParam()
				{
					RendererImpl* renderer = m_renderer;
					if (renderer) {
						return &(renderer->m_param);
					}
					return sl_null;
				}

				SLIB_INLINE ID3DDevice* getDevice()
				{
					RendererImpl* renderer = m_renderer;
					if (renderer) {
						return renderer->m_device;
					}
					return sl_null;
				}

				SLIB_INLINE ID3DDeviceContext* getContext()
				{
					RendererImpl* renderer = m_renderer;
					if (renderer) {
						return renderer->m_context;
					}
					return sl_null;
				}

#if D3D_VERSION_MAJOR >= 10
				SLIB_INLINE ID3DRenderTargetView* getRenderTarget()
				{
					RendererImpl* renderer = m_renderer;
					if (renderer) {
						return renderer->m_pRenderTarget;
					}
					return sl_null;
				}
#endif

			public:
				RenderEngineType getEngineType() override
				{
					return RenderEngineType::D3D_TYPE;
				}

				Ref<RenderProgramInstance> _createProgramInstance(RenderProgram* program) override
				{
					ID3DDevice* device = getDevice();
					if (!device) {
						return sl_null;
					}
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return sl_null;
					}
					return Ref<RenderProgramInstance>::from(RenderProgramInstanceImpl::create(device, context, this, program));
				}

				Ref<VertexBufferInstance> _createVertexBufferInstance(VertexBuffer* buffer) override
				{
					ID3DDevice* device = getDevice();
					if (!device) {
						return sl_null;
					}
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return sl_null;
					}
					return Ref<VertexBufferInstance>::from(VertexBufferInstanceImpl::create(device, context, this, buffer));
				}

				Ref<IndexBufferInstance> _createIndexBufferInstance(IndexBuffer* buffer) override
				{
					ID3DDevice* device = getDevice();
					if (!device) {
						return sl_null;
					}
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return sl_null;
					}
					return Ref<IndexBufferInstance>::from(IndexBufferInstanceImpl::create(device, context, this, buffer));
				}

				Ref<TextureInstance> _createTextureInstance(Texture* texture) override
				{
					ID3DDevice* device = getDevice();
					if (!device) {
						return sl_null;
					}
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return sl_null;
					}
					return Ref<TextureInstance>::from(TextureInstanceImpl::create(device, context, this, texture));
				}

				sl_bool _beginScene() override
				{
					ID3DDeviceContext* context = getContext();
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
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return;
					}
					context->EndScene();
#endif
				}

				void _setViewport(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height) override
				{
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return;
					}
#if D3D_VERSION_MAJOR >= 10
					D3D_(VIEWPORT) v;
#if D3D_VERSION_MAJOR >= 11
					v.TopLeftX = (FLOAT)x;
					v.TopLeftY = (FLOAT)y;
					v.Width = (FLOAT)width;
					v.Height = (FLOAT)height;
#else
					v.TopLeftX = (INT)x;
					v.TopLeftY = (INT)y;
					v.Width = (UINT)width;
					v.Height = (UINT)height;
#endif
					v.MinDepth = 0.0f;
					v.MaxDepth = 1.0f;
					context->RSSetViewports(1, &v);
#else
					D3DVIEWPORT9 v;
					v.X = (DWORD)x;
					v.Y = (DWORD)y;
					v.Width = (DWORD)width;
					v.Height = (DWORD)height;
					v.MinZ = 0.0f;
					v.MaxZ = 1.0f;
					context->SetViewport(&v);
#endif
				}

				void _clear(const RenderClearParam& param) override
				{
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return;
					}
					RendererParam* paramRenderer = getRendererParam();
					if (!paramRenderer) {
						return;
					}

#if D3D_VERSION_MAJOR >= 10
					ID3DRenderTargetView* pRenderTarget = getRenderTarget();
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
					if (param.flagDepth && (paramRenderer->nDepthBits || paramRenderer->nStencilBits)) {
						flags |= D3DCLEAR_ZBUFFER;
					}
					if (param.flagStencil && paramRenderer->nStencilBits) {
						flags |= D3DCLEAR_STENCIL;
					}
					context->Clear(0, sl_null,
						flags,
						D3DCOLOR_ARGB(param.color.a, param.color.r, param.color.g, param.color.b),
						param.depth,
						(DWORD)(param.stencil)
					);
#endif
				}

				void _setDepthTest(sl_bool flagEnableDepthTest) override
				{
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return;
					}
#if D3D_VERSION_MAJOR >= 10
					BOOL v = flagEnableDepthTest ? TRUE : FALSE;
					if (m_stateDescDepthStencil.DepthEnable != v) {
						m_stateDescDepthStencil.DepthEnable = v;
						m_flagUpdatedDepthStencilState = sl_true;
					}
#else
					context->SetRenderState(D3DRS_ZENABLE, flagEnableDepthTest ? D3DZB_TRUE : D3DZB_FALSE);
#endif
				}

				void _setDepthWriteEnabled(sl_bool flagEnableDepthWrite) override
				{
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return;
					}
#if D3D_VERSION_MAJOR >= 10
					D3D_(DEPTH_WRITE_MASK) v = flagEnableDepthWrite ? D3D_(DEPTH_WRITE_MASK_ALL) : D3D_(DEPTH_WRITE_MASK_ZERO);
					if (m_stateDescDepthStencil.DepthWriteMask != v) {
						m_stateDescDepthStencil.DepthWriteMask = v;
						m_flagUpdatedDepthStencilState = sl_true;
					}
#else
					context->SetRenderState(D3DRS_ZWRITEENABLE, flagEnableDepthWrite ? TRUE : FALSE);
#endif
				}

				void _setDepthFunction(RenderFunctionOperation op) override
				{
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return;
					}
#if D3D_VERSION_MAJOR >= 10
					D3D_(COMPARISON_FUNC) v;
					switch (op) {
					case RenderFunctionOperation::Never:
						v = D3D_(COMPARISON_NEVER);
						break;
					case RenderFunctionOperation::Always:
						v = D3D_(COMPARISON_ALWAYS);
						break;
					case RenderFunctionOperation::Equal:
						v = D3D_(COMPARISON_EQUAL);
						break;
					case RenderFunctionOperation::NotEqual:
						v = D3D_(COMPARISON_NOT_EQUAL);
						break;
					case RenderFunctionOperation::Less:
						v = D3D_(COMPARISON_LESS);
						break;
					case RenderFunctionOperation::LessEqual:
						v = D3D_(COMPARISON_LESS_EQUAL);
						break;
					case RenderFunctionOperation::Greater:
						v = D3D_(COMPARISON_GREATER);
						break;
					case RenderFunctionOperation::GreaterEqual:
						v = D3D_(COMPARISON_GREATER_EQUAL);
						break;
					default:
						return;
					}
					if (m_stateDescDepthStencil.DepthFunc != v) {
						m_stateDescDepthStencil.DepthFunc = v;
						m_flagUpdatedDepthStencilState = sl_true;
					}
#else
					DWORD v;
					switch (op) {
					case RenderFunctionOperation::Never:
						v = D3DCMP_NEVER;
						break;
					case RenderFunctionOperation::Always:
						v = D3DCMP_ALWAYS;
						break;
					case RenderFunctionOperation::Equal:
						v = D3DCMP_EQUAL;
						break;
					case RenderFunctionOperation::NotEqual:
						v = D3DCMP_NOTEQUAL;
						break;
					case RenderFunctionOperation::Less:
						v = D3DCMP_LESS;
						break;
					case RenderFunctionOperation::LessEqual:
						v = D3DCMP_LESSEQUAL;
						break;
					case RenderFunctionOperation::Greater:
						v = D3DCMP_GREATER;
						break;
					case RenderFunctionOperation::GreaterEqual:
						v = D3DCMP_GREATEREQUAL;
						break;
					default:
						return;
					}
					context->SetRenderState(D3DRS_ZFUNC, v);
#endif
				}

				void _setCullFace(sl_bool flagEnableCull, sl_bool flagCullCCW) override
				{
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return;
					}
#if D3D_VERSION_MAJOR >= 10
					D3D_(CULL_MODE) v = flagEnableCull ? (flagCullCCW ? D3D_(CULL_BACK) : D3D_(CULL_FRONT)) : D3D_(CULL_NONE);
					if (m_stateDescRasterizer.CullMode != v) {
						m_stateDescRasterizer.CullMode = v;
						m_flagUpdatedRasterizerState = sl_true;
					}
#else
					context->SetRenderState(D3DRS_CULLMODE, flagEnableCull ? (flagCullCCW ? D3DCULL_CCW : D3DCULL_CW) : D3DCULL_NONE);
#endif
				}

				void _setBlending(sl_bool flagEnableBlending, const RenderBlendingParam& param) override
				{
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return;
					}
#if D3D_VERSION_MAJOR >= 10
					BOOL v = flagEnableBlending ? TRUE : FALSE;
#if D3D_VERSION_MAJOR >= 11
					if (m_stateDescBlend.RenderTarget[0].BlendEnable != v) {
						m_stateDescBlend.RenderTarget[0].BlendEnable = v;
						m_flagUpdatedBlendState = sl_true;
					}
#else
					if (m_stateDescBlend.BlendEnable[0] != v) {
						m_stateDescBlend.BlendEnable[0] = v;
						m_flagUpdatedBlendState = sl_true;
					}
#endif
					
					if (flagEnableBlending) {
#if D3D_VERSION_MAJOR >= 11
						_setBlendOperation(m_stateDescBlend.RenderTarget[0].BlendOp, param.operation, m_flagUpdatedBlendState);
						_setBlendOperation(m_stateDescBlend.RenderTarget[0].BlendOpAlpha, param.operationAlpha, m_flagUpdatedBlendState);
						_setBlendFactor(m_stateDescBlend.RenderTarget[0].SrcBlend, param.blendSrc, m_flagUpdatedBlendState);
						_setBlendFactor(m_stateDescBlend.RenderTarget[0].DestBlend, param.blendDst, m_flagUpdatedBlendState);
						_setBlendFactor(m_stateDescBlend.RenderTarget[0].SrcBlendAlpha, param.blendSrcAlpha, m_flagUpdatedBlendState);
						_setBlendFactor(m_stateDescBlend.RenderTarget[0].DestBlendAlpha, param.blendDstAlpha, m_flagUpdatedBlendState);
#else
						_setBlendOperation(m_stateDescBlend.BlendOp, param.operation, m_flagUpdatedBlendState);
						_setBlendOperation(m_stateDescBlend.BlendOpAlpha, param.operationAlpha, m_flagUpdatedBlendState);
						_setBlendFactor(m_stateDescBlend.SrcBlend, param.blendSrc, m_flagUpdatedBlendState);
						_setBlendFactor(m_stateDescBlend.DestBlend, param.blendDst, m_flagUpdatedBlendState);
						_setBlendFactor(m_stateDescBlend.SrcBlendAlpha, param.blendSrcAlpha, m_flagUpdatedBlendState);
						_setBlendFactor(m_stateDescBlend.DestBlendAlpha, param.blendDstAlpha, m_flagUpdatedBlendState);
#endif
						if (m_blendFactor[0] != param.blendConstant.x || m_blendFactor[1] != param.blendConstant.y || m_blendFactor[2] != param.blendConstant.z || m_blendFactor[3] != param.blendConstant.w) {
							m_blendFactor[0] = param.blendConstant.x;
							m_blendFactor[1] = param.blendConstant.y;
							m_blendFactor[2] = param.blendConstant.z;
							m_blendFactor[3] = param.blendConstant.w;
							m_flagUpdatedBlendState = sl_true;
						}
					}
#else
					context->SetRenderState(D3DRS_ALPHABLENDENABLE, flagEnableBlending ? TRUE : FALSE);
					if (flagEnableBlending) {
						_setBlendOperation(context, D3DRS_BLENDOP, param.operation);
						_setBlendOperation(context, D3DRS_BLENDOPALPHA, param.operationAlpha);
						_setBlendFactor(context, D3DRS_SRCBLEND, param.blendSrc);
						_setBlendFactor(context, D3DRS_DESTBLEND, param.blendDst);
						_setBlendFactor(context, D3DRS_SRCBLENDALPHA, param.blendSrcAlpha);
						_setBlendFactor(context, D3DRS_DESTBLENDALPHA, param.blendDstAlpha);
						Color f(param.blendConstant);
						context->SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_ARGB(f.a, f.r, f.g, f.b));
					}
#endif
				}

				sl_bool _beginProgram(RenderProgram* program, RenderProgramInstance* _instance, RenderProgramState** ppState) override
				{
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return sl_false;
					}
					RenderProgramInstanceImpl* instance = (RenderProgramInstanceImpl*)_instance;
					if (m_currentProgramInstance != instance) {
#if D3D_VERSION_MAJOR >= 11
						context->VSSetShader(instance->vertexShader, NULL, 0);
						context->PSSetShader(instance->pixelShader, NULL, 0);
#elif D3D_VERSION_MAJOR >= 10
						context->VSSetShader(instance->vertexShader);
						context->PSSetShader(instance->pixelShader);
#else
						context->SetVertexShader(instance->vertexShader);
						context->SetPixelShader(instance->pixelShader);
#endif
						m_currentProgramInstance = instance;
					}
					if (ppState) {
						*ppState = instance->state.get();
					}
					m_currentProgram = program;
					return sl_true;
				}

				void _endProgram() override
				{
				}

				void _resetCurrentBuffers() override
				{
					m_currentProgram.setNull();
					m_currentProgramInstance.setNull();
					m_currentProgramRendering.setNull();
					m_currentProgramInstanceRendering.setNull();
					m_currentVertexBufferInstance.setNull();
					m_currentIndexBufferInstance.setNull();
				}

				void _drawPrimitive(EnginePrimitive* primitive) override
				{
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return;
					}
#if D3D_VERSION_MAJOR >= 10
					switch (primitive->type) {
					case PrimitiveType::Triangle:
						context->IASetPrimitiveTopology(D3D_(PRIMITIVE_TOPOLOGY_TRIANGLELIST));
						break;
					case PrimitiveType::TriangleStrip:
						context->IASetPrimitiveTopology(D3D_(PRIMITIVE_TOPOLOGY_TRIANGLESTRIP));
						break;
					case PrimitiveType::Line:
						context->IASetPrimitiveTopology(D3D_(PRIMITIVE_TOPOLOGY_LINELIST));
						break;
					case PrimitiveType::LineStrip:
						context->IASetPrimitiveTopology(D3D_(PRIMITIVE_TOPOLOGY_LINESTRIP));
						break;
					case PrimitiveType::Point:
						context->IASetPrimitiveTopology(D3D_(PRIMITIVE_TOPOLOGY_POINTLIST));
						break;
					default:
						return;
					}
#else
					D3DPRIMITIVETYPE type;
					sl_int32 nPrimitives;
					switch (primitive->type) {
					case PrimitiveType::Triangle:
						type = D3DPT_TRIANGLELIST;
						nPrimitives = primitive->countElements / 3;
						break;
					case PrimitiveType::TriangleStrip:
						type = D3DPT_TRIANGLESTRIP;
						nPrimitives = primitive->countElements - 2;
						break;
					case PrimitiveType::TriangleFan:
						type = D3DPT_TRIANGLEFAN;
						nPrimitives = primitive->countElements - 2;
						break;
					case PrimitiveType::Line:
						type = D3DPT_LINELIST;
						nPrimitives = primitive->countElements / 2;
						break;
					case PrimitiveType::LineStrip:
						type = D3DPT_LINESTRIP;
						nPrimitives = primitive->countElements - 1;
						break;
					case PrimitiveType::Point:
						type = D3DPT_POINTLIST;
						nPrimitives = primitive->countElements;
						break;
					default:
						return;
					}
					if (nPrimitives <= 0) {
						return;
					}
#endif
					if (m_currentProgram.isNull()) {
						return;
					}
					if (m_currentProgramInstance.isNull()) {
						return;
					}

					sl_bool flagResetProgramState = sl_false;
					if (m_currentProgramInstanceRendering != m_currentProgramInstance) {
						flagResetProgramState = sl_true;
					}

					if (flagResetProgramState) {
						if (m_currentProgramInstanceRendering.isNotNull()) {
							m_currentProgramRendering->onPostRender(this, m_currentProgramInstanceRendering.get(), m_currentProgramInstanceRendering->state.get());
						}
						m_currentProgramInstanceRendering = m_currentProgramInstance;
						m_currentProgramRendering = m_currentProgram;
						m_currentProgram->onPreRender(this, m_currentProgramInstance.get(), m_currentProgramInstance->state.get());
					}

					RenderInputLayoutImpl* layout = m_currentInputLayout.get();
					if (!layout) {
						return;
					}
					sl_uint32 nVerticesMin = 0;
					VertexBufferInstanceImpl* vb = static_cast<VertexBufferInstanceImpl*>(primitive->vertexBufferInstance.get());
					if (vb) {
						vb->doUpdate(primitive->vertexBuffer.get());
						UINT stride = (UINT)(layout->strides.getValueAt_NoLock(0));
#if D3D_VERSION_MAJOR >= 10
						UINT offset = 0;
						context->IASetVertexBuffers(0, 1, &(vb->pVB), &stride, &offset);
#else
						context->SetStreamSource(0, vb->pVB, 0, stride);
#endif
					} else {
						ListElements< Ref<VertexBufferInstance> > list(primitive->vertexBufferInstances);
						if (!(list.count)) {
							return;
						}
#if D3D_VERSION_MAJOR >= 10
						SLIB_SCOPED_BUFFER(ID3DVertexBuffer*, 16, bufs, list.count)
						SLIB_SCOPED_BUFFER(UINT, 16, strides, list.count)
						SLIB_SCOPED_BUFFER(UINT, 16, offsets, list.count)
#endif
						for (sl_size i = 0; i < list.count; i++) {
							VertexBufferInstanceImpl* vb = (VertexBufferInstanceImpl*)(list[i].get());
							UINT stride = (UINT)(layout->strides.getValueAt_NoLock(i));
#if D3D_VERSION_MAJOR >= 10
							bufs[i] = vb->pVB;
							offsets[i] = 0;
							strides[i] = stride;
#else
							context->SetStreamSource((UINT)i, vb->pVB, 0, stride);
#endif
						}
#if D3D_VERSION_MAJOR >= 10
						context->IASetVertexBuffers(0, (UINT)(list.count), bufs, strides, offsets);
#endif
					}
#if D3D_VERSION_MAJOR >= 10
					m_currentProgramInstanceRendering->applyConstantBuffer(context);
#endif
					if (primitive->indexBufferInstance.isNotNull()) {
						IndexBufferInstanceImpl* ib = (IndexBufferInstanceImpl*)(primitive->indexBufferInstance.get());
						ib->doUpdate(primitive->indexBuffer.get());
#if D3D_VERSION_MAJOR >= 10
						context->IASetIndexBuffer(ib->pIB, DXGI_FORMAT_R16_UINT, 0);
						context->DrawIndexed((UINT)(primitive->countElements), 0, 0);
#else
						context->SetIndices(ib->pIB);
						context->DrawIndexedPrimitive(type, 0, 0, (UINT)nVerticesMin, 0, (UINT)nPrimitives);
#endif
					} else {
#if D3D_VERSION_MAJOR >= 10
						context->Draw((UINT)(primitive->countElements), 0);
#else
						context->DrawPrimitive(type, 0, (UINT)nPrimitives);
#endif
					}
				}

				void _applyTexture(Texture* texture, TextureInstance* instance, sl_reg sampler) override
				{
				}

				void _setInputLayout(RenderInputLayout* _layout) override
				{
					ID3DDeviceContext* context = getContext();
					if (!context) {
						return;
					}
					RenderInputLayoutImpl* layout = (RenderInputLayoutImpl*)_layout;
					if (m_currentInputLayout == layout) {
						return;
					}
					if (layout) {
#if D3D_VERSION_MAJOR >= 10
						context->IASetInputLayout(layout->layout);
#else
						context->SetVertexDeclaration(layout->layout);
#endif
					}
					m_currentInputLayout = layout;
				}

				void _setLineWidth(sl_real width) override
				{
				}

#if D3D_VERSION_MAJOR >= 10
				void _updateDepthStencilState()
				{
					ID3DDevice* device = getDevice();
					ID3DDeviceContext* context = getContext();
					if (m_flagUpdatedDepthStencilState || m_flagUpdatedDepthStencilStateStencilRef) {
						if (m_flagUpdatedDepthStencilState) {
							m_flagUpdatedDepthStencilState = sl_false;
							m_flagUpdatedDepthStencilStateStencilRef = sl_false;
							ID3DDepthStencilState* state = sl_null;
							device->CreateDepthStencilState(&m_stateDescDepthStencil, &state);
							if (state) {
								context->OMSetDepthStencilState(state, m_stencilRef);
								ID3DDepthStencilState* stateOld = m_stateDepthStencil;
								m_stateDepthStencil = state;
								if (stateOld) {
									stateOld->Release();
								}
							}
						} else {
							m_flagUpdatedDepthStencilStateStencilRef = sl_false;
							context->OMSetDepthStencilState(m_stateDepthStencil, m_stencilRef);
						}
					}
				}

				void _updateRasterizerState()
				{
					ID3DDevice* device = getDevice();
					ID3DDeviceContext* context = getContext();
					if (!m_flagUpdatedRasterizerState) {
						return;
					}
					m_flagUpdatedRasterizerState = sl_false;
					ID3DRasterizerState* state = sl_null;
					device->CreateRasterizerState(&m_stateDescRasterizer, &state);
					if (state) {
						context->RSSetState(state);
						ID3DRasterizerState* stateOld = m_stateRasterizer;
						m_stateRasterizer = state;
						if (stateOld) {
							stateOld->Release();
						}
					}
				}

				void _updateBlendState()
				{
					ID3DDevice* device = getDevice();
					ID3DDeviceContext* context = getContext();
					if (m_flagUpdatedBlendState || m_flagUpdatedBlendStateBlendFactor) {
						if (m_flagUpdatedBlendState) {
							m_flagUpdatedBlendState = sl_false;
							m_flagUpdatedBlendStateBlendFactor = sl_false;
							ID3DBlendState* state = sl_null;
							device->CreateBlendState(&m_stateDescBlend, &state);
							if (state) {
								context->OMSetBlendState(state, m_blendFactor, 0xffffffff);
								ID3DBlendState* stateOld = m_stateBlend;
								m_stateBlend = state;
								if (stateOld) {
									stateOld->Release();
								}
							}
						} else {
							m_flagUpdatedBlendStateBlendFactor = sl_false;
							context->OMSetBlendState(m_stateBlend, m_blendFactor, 0xffffffff);
						}
					}
				}

				static void _setBlendOperation(D3D_(BLEND_OP)& _out, RenderBlendingOperation op, sl_bool& flagUpdate)
				{
					D3D_(BLEND_OP) v;
					switch (op) {
					case RenderBlendingOperation::Add:
						v = D3D_(BLEND_OP_ADD);
						break;
					case RenderBlendingOperation::Subtract:
						v = D3D_(BLEND_OP_SUBTRACT);
						break;
					case RenderBlendingOperation::ReverseSubtract:
						v = D3D_(BLEND_OP_REV_SUBTRACT);
						break;
					default:
						return;
					}
					if (_out != v) {
						_out = v;
						flagUpdate = sl_true;
					}
				}

				static void _setBlendFactor(D3D_(BLEND)& _out, RenderBlendingFactor f, sl_bool& flagUpdate)
				{
					D3D_(BLEND) v;
					switch (f) {
					case RenderBlendingFactor::One:
						v = D3D_(BLEND_ONE);
						break;
					case RenderBlendingFactor::Zero:
						v = D3D_(BLEND_ZERO);
						break;
					case RenderBlendingFactor::SrcAlpha:
						v = D3D_(BLEND_SRC_ALPHA);
						break;
					case RenderBlendingFactor::OneMinusSrcAlpha:
						v = D3D_(BLEND_INV_SRC_ALPHA);
						break;
					case RenderBlendingFactor::DstAlpha:
						v = D3D_(BLEND_DEST_ALPHA);
						break;
					case RenderBlendingFactor::OneMinusDstAlpha:
						v = D3D_(BLEND_INV_DEST_ALPHA);
						break;
					case RenderBlendingFactor::SrcColor:
						v = D3D_(BLEND_SRC_COLOR);
						break;
					case RenderBlendingFactor::OneMinusSrcColor:
						v = D3D_(BLEND_INV_SRC_COLOR);
						break;
					case RenderBlendingFactor::DstColor:
						v = D3D_(BLEND_DEST_COLOR);
						break;
					case RenderBlendingFactor::OneMinusDstColor:
						v = D3D_(BLEND_INV_DEST_COLOR);
						break;
					case RenderBlendingFactor::SrcAlphaSaturate:
						v = D3D_(BLEND_SRC_ALPHA_SAT);
						break;
					case RenderBlendingFactor::Constant:
						v = D3D_(BLEND_BLEND_FACTOR);
						break;
					case RenderBlendingFactor::OneMinusConstant:
						v = D3D_(BLEND_INV_BLEND_FACTOR);
						break;
					default:
						return;
					}
					if (_out != v) {
						_out = v;
						flagUpdate = sl_true;
					}
				}
#else
				static void _setBlendOperation(ID3DDeviceContext* context, D3DRENDERSTATETYPE state, RenderBlendingOperation op)
				{
					DWORD v;
					switch (op) {
					case RenderBlendingOperation::Add:
						v = D3DBLENDOP_ADD;
						break;
					case RenderBlendingOperation::Subtract:
						v = D3DBLENDOP_SUBTRACT;
						break;
					case RenderBlendingOperation::ReverseSubtract:
						v = D3DBLENDOP_REVSUBTRACT;
						break;
					default:
						return;
					}
					context->SetRenderState(state, v);
				}

				static void _setBlendFactor(ID3DDeviceContext* context, D3DRENDERSTATETYPE state, RenderBlendingFactor f)
				{
					DWORD v;
					switch (f) {
					case RenderBlendingFactor::One:
						v = D3DBLEND_ONE;
						break;
					case RenderBlendingFactor::Zero:
						v = D3DBLEND_ZERO;
						break;
					case RenderBlendingFactor::SrcAlpha:
						v = D3DBLEND_SRCALPHA;
						break;
					case RenderBlendingFactor::OneMinusSrcAlpha:
						v = D3DBLEND_INVSRCALPHA;
						break;
					case RenderBlendingFactor::DstAlpha:
						v = D3DBLEND_DESTALPHA;
						break;
					case RenderBlendingFactor::OneMinusDstAlpha:
						v = D3DBLEND_INVDESTALPHA;
						break;
					case RenderBlendingFactor::SrcColor:
						v = D3DBLEND_SRCCOLOR;
						break;
					case RenderBlendingFactor::OneMinusSrcColor:
						v = D3DBLEND_INVSRCCOLOR;
						break;
					case RenderBlendingFactor::DstColor:
						v = D3DBLEND_DESTCOLOR;
						break;
					case RenderBlendingFactor::OneMinusDstColor:
						v = D3DBLEND_INVDESTCOLOR;
						break;
					case RenderBlendingFactor::SrcAlphaSaturate:
						v = D3DBLEND_SRCALPHASAT;
						break;
					case RenderBlendingFactor::Constant:
						v = D3DBLEND_BLENDFACTOR;
						break;
					case RenderBlendingFactor::OneMinusConstant:
						v = D3DBLEND_INVBLENDFACTOR;
						break;
					default:
						return;
					}
					context->SetRenderState(state, v);
				}
#endif

			};

			void RendererImpl::run()
			{
				Ref<EngineImpl> engine = new EngineImpl;
				if (engine.isNull()) {
					return;
				}

				engine->m_renderer = this;

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

				engine->m_renderer = sl_null;
			}

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