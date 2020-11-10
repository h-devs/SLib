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
				ID3DDevice* device;
				ID3DDeviceContext* context;
				ID3DInputLayout* layout;

			public:
				RenderInputLayoutImpl()
				{
					device = sl_null;
					context = sl_null;
					layout = sl_null;
				}

				~RenderInputLayoutImpl()
				{
					if (layout) {
						layout->Release();
					}
				}

			public:
				static Ref<RenderInputLayoutImpl> create(ID3DDevice* device, ID3DDeviceContext* context, const Memory& codeVertexShader, const RenderProgramStateItem* items, sl_uint32 nItems)
				{
					ID3DInputLayout* layout = sl_null;
#if D3D_VERSION_MAJOR >= 10
					if (codeVertexShader.isNull()) {
						return sl_null;
					}
					List<D3D_(INPUT_ELEMENT_DESC)> descs;
					for (sl_uint32 i = 0; i < nItems; i++) {
						const RenderProgramStateItem& item = items[i];
						if (item.kind == RenderProgramStateKind::Input) {
							D3D_(INPUT_ELEMENT_DESC) desc = { 0 };
							switch (item.input.semanticName) {
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
								desc.SemanticIndex = (UINT)(item.input.semanticIndex);
								switch (item.input.type) {
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
									desc.InputSlot = (UINT)(item.input.slot);
									desc.AlignedByteOffset = (UINT)(item.input.offset);
									desc.InputSlotClass = D3D_(INPUT_PER_VERTEX_DATA);
									desc.InstanceDataStepRate = 0;
									descs.add_NoLock(desc);
								}
							}
						}
					}
					if (descs.isEmpty()) {
						return sl_null;
					}
					device->CreateInputLayout(descs.getData(), (UINT)(descs.getCount()), codeVertexShader.getData(), (SIZE_T)(codeVertexShader.getSize()), &layout);
#else
					List<D3DVERTEXELEMENT9> elements;
					for (sl_uint32 i = 0; i < nItems; i++) {
						const RenderProgramStateItem& item = items[i];
						if (item.kind == RenderProgramStateKind::Input) {
							D3DVERTEXELEMENT9 element = { 0 };
							sl_bool flagValidUsage = sl_true;
							switch (item.input.semanticName) {
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
								switch (item.input.type) {
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
									element.Offset = (WORD)(item.input.offset);
									element.Stream = (WORD)(item.input.slot);
									elements.add_NoLock(element);
								}
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
						ret->device = device;
						ret->context = context;
						ret->layout = layout;
						return ret;
					}
					return sl_null;
				}

			public:
				void load() override
				{
#if D3D_VERSION_MAJOR >= 10
					context->IASetInputLayout(layout);
#else
					device->SetVertexDeclaration(layout);
#endif
				}

				void unload() override
				{
				}

			};

			static Memory CompileShader(const String& str, const char* target)
			{
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
#if D3D_VERSION_MAJOR >= 11
				func(str.getData(), (SIZE_T)(str.getLength()), NULL, NULL, NULL, "main", target, D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, &blob, NULL);
#else
				func(str.getData(), (SIZE_T)(str.getLength()), NULL, NULL, NULL, "main", target, D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, NULL, &blob, NULL, NULL);
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
				func(str.getData(), (UINT)(str.getLength()), NULL, NULL, "main", target, 0, &shader, NULL, NULL);
				if (shader) {
					Memory ret = Memory::create(shader->GetBufferPointer(), (sl_size)(shader->GetBufferSize()));
					shader->Release();
					return ret;
				}
#endif
				return sl_null;
			}

			class RenderProgramInstanceImpl : public RenderProgramInstance
			{
			public:
				ID3DDevice* device;
				ID3DDeviceContext* context;
				
				ID3DVertexShader* vertexShader;
				ID3DPixelShader* pixelShader;

#if D3D_VERSION_MAJOR >= 10
				Memory codeVertexShader;
#endif

				Ref<RenderProgramState> state;

			public:
				RenderProgramInstanceImpl()
				{
					device = sl_null;
					context = sl_null;
					vertexShader = sl_null;
					pixelShader = sl_null;
				}

				~RenderProgramInstanceImpl()
				{
					if (vertexShader) {
						vertexShader->Release();
					}
					if (pixelShader) {
						pixelShader->Release();
					}
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
					if (codePixel.isNotNull()) {
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
							Ref<RenderProgramState> state = program->onCreate(engine);
							if (state.isNotNull()) {
								Ref<RenderProgramInstanceImpl> ret = new RenderProgramInstanceImpl();
								if (ret.isNotNull()) {
									ret->device = device;
									ret->context = context;
									ret->vertexShader = vs;
									ret->pixelShader = ps;
									state->programInstance = ret.get();
									if (program->onInit(engine, state.get())) {
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
							ps->Release();
						}
						vs->Release();
					}
					return sl_null;
				}

				Ref<RenderInputLayout> createInputLayout(sl_uint32 stride, const RenderProgramStateItem* items, sl_uint32 nItems) override
				{
#if D3D_VERSION_MAJOR >= 10
					return Ref<RenderInputLayout>::from(RenderInputLayoutImpl::create(device, context, codeVertexShader, items, nItems));
#else
					return Ref<RenderInputLayout>::from(RenderInputLayoutImpl::create(device, context, sl_null, items, nItems));
#endif
				}

				sl_bool getUniformLocation(const char* name, RenderUniformLocation* outLocation) override
				{
					return sl_false;
				}

				void setUniform(const RenderUniformLocation& location, RenderUniformType type, const void* data, sl_uint32 nItems) override
				{
#if D3D_VERSION_MAJOR < 10
					switch (type) {
					case RenderUniformType::Float:
						break;
					case RenderUniformType::Float2:
						type = RenderUniformType::Float;
						nItems *= 2;
						break;
					case RenderUniformType::Float3:
						type = RenderUniformType::Float;
						nItems *= 3;
						break;
					case RenderUniformType::Float4:
						type = RenderUniformType::Float;
						nItems *= 4;
						break;
					case RenderUniformType::Matrix3:
						type = RenderUniformType::Float;
						nItems *= 9;
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
					case RenderUniformType::Int3:
						type = RenderUniformType::Int;
						nItems *= 3;
						break;
					case RenderUniformType::Int4:
						type = RenderUniformType::Int;
						nItems *= 4;
						break;
					case RenderUniformType::Sampler:
						type = RenderUniformType::Int;
						break;
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

#if D3D_VERSION_MAJOR < 10
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

			public:
				EngineImpl()
				{
					m_renderer = sl_null;
				}

				~EngineImpl()
				{
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
					return sl_null;
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