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

#include "slib/graphics/image.h"
#include "slib/core/time_counter.h"
#include "slib/core/thread.h"
#include "slib/core/scoped_buffer.h"
#include "slib/core/log.h"
#include "slib/ui/platform.h"
#include "slib/dl/win32/d3d.h"

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

	namespace
	{
		class RendererImpl : public Renderer
		{
		public:
			RendererParam m_param;
			sl_bool m_flagRequestRender = sl_true;

			ID3DDevice* m_device = sl_null;
#if D3D_VERSION_MAJOR >= 11
			ID3DDeviceContext* m_context = sl_null;
#endif
#if D3D_VERSION_MAJOR >= 10
			IDXGISwapChain* m_pSwapChain = sl_null;
			ID3DRenderTargetView* m_pRenderTarget = sl_null;
			ID3DDepthStencilView* m_pDepthStencil = sl_null;
#endif

			HWND m_hWnd = sl_null;

			AtomicRef<Thread> m_threadRender;

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
#	if D3D_VERSION_MAJOR >= 9
				auto funcCreateD3D = slib::d3d9::getApi_Direct3DCreate9();
				if (funcCreateD3D) {
					IDirect3D9* d3d = funcCreateD3D(D3D_SDK_VERSION);
#	else
				auto funcCreateD3D = slib::d3d8::getApi_Direct3DCreate8();
				if (funcCreateD3D) {
					IDirect3D8* d3d = funcCreateD3D(D3D_SDK_VERSION);
#	endif
					if (d3d) {
						D3DPRESENT_PARAMETERS d3dpp = {};
						preparePresentParams(d3dpp, param);
						d3dpp.hDeviceWindow = hWnd;
						ID3DDevice* device = sl_null;
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
				Ref<RendererImpl> ret = new RendererImpl();
				if (ret.isNotNull()) {
					ret->m_device = device;
					ret->m_hWnd = (HWND)windowHandle;
					ret->m_param = param;
					if (ret->createResources()) {
						ret->initWithParam(param);
						ret->m_threadRender = Thread::start(SLIB_FUNCTION_MEMBER(ret.get(), run));
						return ret;
					}
				} else {
					if (flagFreeOnFailure) {
						device->Release();
					}
				}
				return sl_null;
			}

#if D3D_VERSION_MAJOR <= 9
			static void preparePresentParams(D3DPRESENT_PARAMETERS& d3dpp, const RendererParam& param)
			{
				d3dpp.Windowed = TRUE;
				d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
				d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
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
			}

			sl_bool recreateDevice()
			{
				if (!m_device) {
					return sl_false;
				}
#	if D3D_VERSION_MAJOR >= 9
				IDirect3D9* d3d = sl_null;
#	else
				IDirect3D8* d3d = sl_null;
#	endif
				m_device->GetDirect3D(&d3d);
				if (!d3d) {
					return sl_false;
				}
				m_device->Release();
				m_device = sl_null;
				D3DPRESENT_PARAMETERS d3dpp = {};
				preparePresentParams(d3dpp, m_param);
				d3dpp.hDeviceWindow = m_hWnd;
				ID3DDevice* device = sl_null;
				HRESULT hr = d3d->CreateDevice(
					D3DADAPTER_DEFAULT,
					D3DDEVTYPE_HAL,
					m_hWnd,
					D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
					&d3dpp,
					&device);
				m_device = device;
				return SUCCEEDED(hr);
			}
#endif

			void release()
			{
				ObjectLocker lock(this);
				Ref<Thread> thread = m_threadRender;
				if (thread.isNotNull()) {
					thread->finishAndWait();
					m_threadRender.setNull();
				}
				releaseResources();
				if (m_device) {
					m_device->Release();
					m_device = sl_null;
				}
			}

			sl_bool createResources()
			{
#if D3D_VERSION_MAJOR >= 11
				m_device->GetImmediateContext(&m_context);
				if (!m_context) {
					return sl_false;
				}
#endif
#if D3D_VERSION_MAJOR >= 10
				{
					auto funcCreateDXGIFactory = dxgi::getApi_CreateDXGIFactory();
					if (!funcCreateDXGIFactory) {
						return sl_false;
					}
					IDXGIFactory* pFactory = sl_null;
					funcCreateDXGIFactory(IID_PPV_ARGS(&pFactory));
					if (!pFactory) {
						return sl_false;
					}
					DXGI_SWAP_CHAIN_DESC desc = {};
					desc.BufferCount = 1;
					desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
					desc.OutputWindow = m_hWnd;
					desc.SampleDesc.Count = 1;
					desc.Windowed = TRUE;
					pFactory->CreateSwapChain(m_device, &desc, &m_pSwapChain);
					pFactory->Release();
					if (!m_pSwapChain) {
						return sl_false;
					}
				}
				{
					ID3DTexture2D* pBackBuffer = sl_null;
					m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
					if (!pBackBuffer) {
						return sl_false;
					}
					m_device->CreateRenderTargetView(pBackBuffer, NULL, &m_pRenderTarget);
					pBackBuffer->Release();
					if (!m_pRenderTarget) {
						return sl_false;
					}
				}
				{
					const DXGI_FORMAT format = DXGI_FORMAT_D24_UNORM_S8_UINT;
					RECT rc;
					GetClientRect(m_hWnd, &rc);

					ID3DTexture2D* pTexture = sl_null;
					{
						D3D_(TEXTURE2D_DESC) desc = {};
						desc.Width = rc.right;
						desc.Height = rc.bottom;
						desc.MipLevels = 1;
						desc.ArraySize = 1;
						desc.Format = format;
						desc.SampleDesc.Count = 1;
						desc.SampleDesc.Quality = 0;
						desc.Usage = D3D_(USAGE_DEFAULT);
						desc.BindFlags = D3D_(BIND_DEPTH_STENCIL);
						desc.CPUAccessFlags = 0;
						desc.MiscFlags = 0;
						m_device->CreateTexture2D(&desc, NULL, &pTexture);
					}
					if (pTexture) {
						D3D_(DEPTH_STENCIL_VIEW_DESC) desc = {};
						desc.Format = format;
						desc.ViewDimension = D3D_(DSV_DIMENSION_TEXTURE2D);
						desc.Texture2D.MipSlice = 0;
						m_device->CreateDepthStencilView(pTexture, &desc, &m_pDepthStencil);
						pTexture->Release();
					}
					if (!m_pDepthStencil) {
						return sl_false;
					}
				}
				m_context->OMSetRenderTargets(1, &m_pRenderTarget, m_pDepthStencil);
#endif
				return sl_true;
			}

			void releaseResources()
			{
#if D3D_VERSION_MAJOR >= 10
				if (m_pRenderTarget) {
					m_pRenderTarget->Release();
					m_pRenderTarget = sl_null;
				}
				if (m_pDepthStencil) {
					m_pDepthStencil->Release();
					m_pDepthStencil = sl_null;
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
			}

			sl_bool resize()
			{
#if D3D_VERSION_MAJOR >= 10
				releaseResources();
				return createResources();
#else
				return recreateDevice();
#endif
			}

			void run();

			void runStep(RenderEngine* engine)
			{
				if (m_hWnd) {
					if (!(UIPlatform::isWindowVisible(m_hWnd))) {
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
					if (rect.right && rect.bottom) {
						engine->setViewport(0, 0, rect.right, rect.bottom);
						handleFrame(engine);
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
		static void PrepareObjectFlags(const RenderObjectFlags& flags, D3D_(USAGE)& usage, UINT& cpuAccessFlags, sl_bool& flagLockable)
		{
			if (flags & RenderObjectFlags::CpuAccessWrite) {
				cpuAccessFlags |= D3D_(CPU_ACCESS_WRITE);
				usage = D3D_(USAGE_DYNAMIC);
				flagLockable = sl_true;
			}
			if (flags & RenderObjectFlags::CpuAccessRead) {
				cpuAccessFlags |= D3D_(CPU_ACCESS_READ);
				usage = D3D_(USAGE_STAGING);
				flagLockable = sl_true;
			}
		}
#endif

#if D3D_VERSION_MAJOR >= 9
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
				if (!(inputs.count)) {
					return sl_null;
				}
				ID3DInputLayout* layout = sl_null;
#if D3D_VERSION_MAJOR >= 10
				if (codeVertexShader.isNull()) {
					return sl_null;
				}
				List<D3D_(INPUT_ELEMENT_DESC)> descs;
				for (sl_size i = 0; i < inputs.count; i++) {
					RenderInputLayoutItem& item = inputs[i];
					D3D_(INPUT_ELEMENT_DESC) desc = {};
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
					D3DVERTEXELEMENT9 element = {};
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
#endif

#if D3D_VERSION_MAJOR >= 10
		class ConstantBuffer
		{
		public:
			ID3DBuffer* handle;
			Memory mem;
			sl_uint8* data;
			sl_uint32 size;
			sl_bool flagUpdated;

		public:
			ConstantBuffer()
			{
				handle = sl_null;
				data = sl_null;
				size = 0;
				flagUpdated = sl_false;
			}

			ConstantBuffer(const ConstantBuffer&) = delete;

			ConstantBuffer(ConstantBuffer&& other)
			{
				handle = other.handle;
				mem = Move(other.mem);
				data = other.data;
				size = other.size;
				flagUpdated = other.flagUpdated;
				other.handle = sl_null;
			}

			~ConstantBuffer()
			{
				if (handle) {
					handle->Release();
				}
			}

		public:
			sl_bool init(ID3DDevice* device, sl_uint32 _size)
			{
				size = _size;
				mem = Memory::create(size);
				if (mem.isNull()) {
					return sl_false;
				}
				data = (sl_uint8*)(mem.getData());
				Base::zeroMemory(data, size);

				D3D_(BUFFER_DESC) desc = {};
				desc.BindFlags = D3D_(BIND_CONSTANT_BUFFER);
				desc.ByteWidth = size;

				D3D_(SUBRESOURCE_DATA) res = {};
				res.pSysMem = data;

				device->CreateBuffer(&desc, &res, &handle);
				if (handle) {
					return sl_true;
				}
				return sl_false;
			}

			void update(ID3DDeviceContext* context)
			{
				if (flagUpdated) {
					context->UpdateSubresource(handle, 0, NULL, data, 0, 0);
					flagUpdated = sl_false;
				}
			}

		};
#endif

		class RenderProgramInstanceImpl : public RenderProgramInstance
		{
		public:
			ID3DDevice* device;
			ID3DDeviceContext* context;

#if D3D_VERSION_MAJOR >= 9
			ID3DVertexShader* vertexShader;
			ID3DPixelShader* pixelShader;
#else
			DWORD hVertexShader;
			DWORD hPixelShader;
			List<sl_uint32> strides;
#endif

#if D3D_VERSION_MAJOR >= 10
			Memory codeVertexShader;
			List<ConstantBuffer> constantBuffersVS;
			List<ID3DBuffer*> constantBufferHandlesVS;
			List<ConstantBuffer> constantBuffersPS;
			List<ID3DBuffer*> constantBufferHandlesPS;
#endif

			Ref<RenderProgramState> state;

		public:
			RenderProgramInstanceImpl()
			{
				device = sl_null;
				context = sl_null;
#if D3D_VERSION_MAJOR >= 9
				vertexShader = sl_null;
				pixelShader = sl_null;
#else
				hVertexShader = 0;
				hPixelShader = 0;
#endif
			}

			~RenderProgramInstanceImpl()
			{
#if D3D_VERSION_MAJOR >= 9
				if (vertexShader) {
					vertexShader->Release();
				}
				if (pixelShader) {
					pixelShader->Release();
				}
#else
				Ref<RenderEngine> engine = getEngine();
				if (engine.isNotNull()) {
					if (hVertexShader) {
						device->DeleteVertexShader(hVertexShader);
					}
					if (hPixelShader) {
						device->DeletePixelShader(hPixelShader);
					}
				}
#endif
			}

		public:
			static Ref<RenderProgramInstanceImpl> create(ID3DDevice* device, ID3DDeviceContext* context, RenderEngine* engine, RenderProgram* program)
			{
#if D3D_VERSION_MAJOR >= 9
				Memory codeVertex = program->getCompiledShader(engine, RenderShaderType::HLSL_Vertex);
				if (codeVertex.isNull()) {
					codeVertex = Direct3D::compileShader(program->getShader(engine, RenderShaderType::HLSL_Vertex), VERTEX_SHADER_TARGET);
					if (codeVertex.isNull()) {
						return sl_null;
					}
				}
				Memory codePixel = program->getCompiledShader(engine, RenderShaderType::HLSL_Pixel);
				if (codePixel.isNull()) {
					codePixel = Direct3D::compileShader(program->getShader(engine, RenderShaderType::HLSL_Pixel), PIXEL_SHADER_TARGET);
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
#else
				Memory codeVertex = program->getCompiledShader(engine, RenderShaderType::Assembly_Vertex);
				Memory constantsVertex;
				if (codeVertex.isNull()) {
					codeVertex = assembleShader(program->getShader(engine, RenderShaderType::Assembly_Vertex), &constantsVertex);
					if (codeVertex.isNull()) {
						return sl_null;
					}
				}
				Memory codePixel = program->getCompiledShader(engine, RenderShaderType::Assembly_Pixel);
				if (codePixel.isNull()) {
					codePixel = assembleShader(program->getShader(engine, RenderShaderType::Assembly_Pixel), sl_null);
					if (codePixel.isNull()) {
						return sl_null;
					}
				}
				Ref<RenderProgramState> state = program->onCreate(engine);
				if (state.isNull()) {
					return sl_null;
				}
				Ref<RenderProgramInstanceImpl> ret = new RenderProgramInstanceImpl();
				if (ret.isNull()) {
					return sl_null;
				}
				state->setProgramInstance(ret.get());
				if (!(program->onInit(engine, ret.get(), state.get()))) {
					return sl_null;
				}
				RenderInputLayoutParam inputLayoutParam;
				if (!(program->getInputLayoutParam(state, inputLayoutParam))) {
					return sl_null;
				}
				List<DWORD> decl = createVertexDecl(inputLayoutParam.items);
				if (decl.isEmpty()) {
					return sl_null;
				}
				if (constantsVertex.isNotNull()) {
					decl.insertElements(0, (DWORD*)(constantsVertex.getData()), constantsVertex.getSize() >> 2);
				}
				DWORD hVertexShader = 0;
				device->CreateVertexShader(decl.getData(), (DWORD*)(codeVertex.getData()), &hVertexShader, 0);
				if (hVertexShader) {
#endif
#if D3D_VERSION_MAJOR >= 9
					ID3DPixelShader* ps = sl_null;
#if D3D_VERSION_MAJOR >= 11
					device->CreatePixelShader(codePixel.getData(), (SIZE_T)(codePixel.getSize()), NULL, &ps);
#elif D3D_VERSION_MAJOR >= 10
					device->CreatePixelShader(codePixel.getData(), (SIZE_T)(codePixel.getSize()), &ps);
#else
					device->CreatePixelShader((DWORD*)(codePixel.getData()), &ps);
#endif
					if (ps) {
#else
					DWORD hPixelShader = 0;
					device->CreatePixelShader((DWORD*)(codePixel.getData()), &hPixelShader);
					if (hPixelShader) {
#endif
#if D3D_VERSION_MAJOR >= 10
						List<ConstantBuffer> constantBuffersVS;
						List<ID3DBuffer*> constantBufferHandlesVS;
						List<ConstantBuffer> constantBuffersPS;
						List<ID3DBuffer*> constantBufferHandlesPS;
						if (createConstantBuffers(device, program, constantBuffersVS, constantBufferHandlesVS, constantBuffersPS, constantBufferHandlesPS)) {
#endif
#if D3D_VERSION_MAJOR >= 9
							Ref<RenderProgramState> state = program->onCreate(engine);
							if (state.isNotNull()) {
								Ref<RenderProgramInstanceImpl> ret = new RenderProgramInstanceImpl();
								if (ret.isNotNull()) {
#endif
									ret->device = device;
									ret->context = context;
#if D3D_VERSION_MAJOR >= 9
									ret->vertexShader = vs;
									ret->pixelShader = ps;
#else
									ret->hVertexShader = hVertexShader;
									ret->hPixelShader = hPixelShader;
									ret->strides = inputLayoutParam.strides.toList();
#endif
#if D3D_VERSION_MAJOR >= 10
									ret->constantBuffersVS = Move(constantBuffersVS);
									ret->constantBufferHandlesVS = Move(constantBufferHandlesVS);
									ret->constantBuffersPS = Move(constantBuffersPS);
									ret->constantBufferHandlesPS = Move(constantBufferHandlesPS);
#endif
#if D3D_VERSION_MAJOR >= 9
									state->setProgramInstance(ret.get());
									if (program->onInit(engine, ret.get(), state.get())) {
#endif
										ret->state = state;
#if D3D_VERSION_MAJOR >= 10
										ret->codeVertexShader = codeVertex;
#endif
										ret->link(engine, program);
										return ret;
#if D3D_VERSION_MAJOR >= 9
									}
									return sl_null;
								}
							}
#endif
#if D3D_VERSION_MAJOR >= 10
						}
#endif
#if D3D_VERSION_MAJOR >= 9
						ps->Release();
#else
						device->DeletePixelShader(hPixelShader);
#endif
					}
#if D3D_VERSION_MAJOR >= 9
					vs->Release();
#else
					device->DeleteVertexShader(hVertexShader);
#endif
				}
				return sl_null;
			}

			Ref<RenderInputLayout> createInputLayout(const RenderInputLayoutParam& param) override
			{
#if D3D_VERSION_MAJOR >= 10
				return Ref<RenderInputLayout>::cast(RenderInputLayoutImpl::create(device, codeVertexShader, param));
#elif D3D_VERSION_MAJOR >= 9
				return Ref<RenderInputLayout>::cast(RenderInputLayoutImpl::create(device, sl_null, param));
#else
				return sl_null;
#endif
			}

			sl_bool getUniformLocation(const char* name, RenderUniformLocation* outLocation) override
			{
				if (outLocation->registerNo >= 0) {
					outLocation->location = 0;
					return sl_true;
				}
				return sl_false;
			}

			void setUniform(const RenderUniformLocation& location, RenderUniformType type, const void* data, sl_uint32 nItems) override
			{
				if (location.registerNo < 0) {
					return;
				}
				switch (location.shader) {
					case RenderShaderStage::Vertex:
					case RenderShaderStage::Pixel:
						_setUniform(location, type, data, nItems);
						break;
					default:
						if (location.shader & RenderShaderStage::Vertex) {
							RenderUniformLocation l = location;
							l.shader = RenderShaderStage::Vertex;
							_setUniform(l, type, data, nItems);
						}
						if (location.shader & RenderShaderStage::Pixel) {
							RenderUniformLocation l = location;
							l.shader = RenderShaderStage::Pixel;
							_setUniform(l, type, data, nItems);
						}
						break;
				}
			}

			void _setUniform(const RenderUniformLocation& location, RenderUniformType type, const void* _data, sl_uint32 nItems)
			{
#if D3D_VERSION_MAJOR >= 10
				ConstantBuffer* buffer;
				if (location.shader == RenderShaderStage::Vertex) {
					buffer = constantBuffersVS.getPointerAt(location.bufferNo);
				} else if (location.shader == RenderShaderStage::Pixel) {
					buffer = constantBuffersPS.getPointerAt(location.bufferNo);
				} else {
					return;
				}
				if (!buffer) {
					return;
				}
				sl_uint32 loc = (sl_uint32)(location.registerNo << 4);
				if (loc >= buffer->size) {
					return;
				}
#endif
				const void* data = _data;
				float temp[16];
				float *t = temp;
				Memory memTemp;
				switch (type) {
					case RenderUniformType::Float:
						break;
					case RenderUniformType::Float2:
						type = RenderUniformType::Float;
						nItems <<= 1;
						break;
					case RenderUniformType::Float3:
					case RenderUniformType::Int3:
						if (type == RenderUniformType::Float3) {
							type = RenderUniformType::Float;
						} else {
							type = RenderUniformType::Int;
						}
						if (nItems == 1) {
							float* s = (float*)_data;
							data = t;
							t[0] = s[0]; t[1] = s[1]; t[2] = s[2]; t[3] = 0;
							nItems = 4;
						} else {
							sl_uint32 n = nItems;
							nItems = n << 2;
							memTemp = Memory::create(nItems << 2);
							if (memTemp.isNull()) {
								return;
							}
							float* s = (float*)_data;
							float* t = (float*)(memTemp.getData());
							data = t;
							for (sl_uint32 i = 0; i < n; i++) {
								t[0] = s[0]; t[1] = s[1]; t[2] = s[2]; t[3] = 0;
								t += 4;
								s += 3;
							}
						}
						break;
					case RenderUniformType::Float4:
						type = RenderUniformType::Float;
						nItems <<= 2;
						break;
					case RenderUniformType::Matrix3:
						type = RenderUniformType::Float;
						if (nItems == 1) {
							data = t;
							Matrix3& m = *((Matrix3*)_data);
							t[0] = m.m00; t[1] = m.m10; t[2] = m.m20; t[3] = 0;
							t[4] = m.m01; t[5] = m.m11; t[6] = m.m21; t[7] = 0;
							t[8] = m.m02; t[9] = m.m12; t[10] = m.m22; t[11] = 0;
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
							Matrix3* _m = (Matrix3*)_data;
							for (sl_uint32 i = 0; i < n; i++) {
								Matrix3& m = *_m;
								t[0] = m.m00; t[1] = m.m10; t[2] = m.m20; t[3] = 0;
								t[4] = m.m01; t[5] = m.m11; t[6] = m.m21; t[7] = 0;
								t[8] = m.m02; t[9] = m.m12; t[10] = m.m22; t[11] = 0;
								t += 12;
								_m++;
							}
						}
						break;
					case RenderUniformType::Matrix4:
						type = RenderUniformType::Float;
						if (nItems == 1) {
							data = t;
							Matrix4& m = *((Matrix4*)_data);
							t[0] = m.m00; t[1] = m.m10; t[2] = m.m20; t[3] = m.m30;
							t[4] = m.m01; t[5] = m.m11; t[6] = m.m21; t[7] = m.m31;
							t[8] = m.m02; t[9] = m.m12; t[10] = m.m22; t[11] = m.m32;
							t[12] = m.m03; t[13] = m.m13; t[14] = m.m23; t[15] = m.m33;
							nItems = 16;
						} else {
							sl_uint32 n = nItems;
							nItems = n << 4;
							memTemp = Memory::create(nItems << 2);
							if (memTemp.isNull()) {
								return;
							}
							float* t = (float*)(memTemp.getData());
							data = t;
							Matrix4* _m = (Matrix4*)_data;
							for (sl_uint32 i = 0; i < n; i++) {
								Matrix4& m = *_m;
								t[0] = m.m00; t[1] = m.m10; t[2] = m.m20; t[3] = m.m30;
								t[4] = m.m01; t[5] = m.m11; t[6] = m.m21; t[7] = m.m31;
								t[8] = m.m02; t[9] = m.m12; t[10] = m.m22; t[11] = m.m32;
								t[12] = m.m03; t[13] = m.m13; t[14] = m.m23; t[15] = m.m33;
								t += 16;
								_m++;
							}
						}
						break;
					case RenderUniformType::Int:
						break;
					case RenderUniformType::Int2:
						type = RenderUniformType::Int;
						nItems <<= 1;
						break;
					case RenderUniformType::Int4:
						type = RenderUniformType::Int;
						nItems <<= 2;
						break;
					case RenderUniformType::Sampler:
						return;
					default:
						return;
				}
#if D3D_VERSION_MAJOR >= 10
				sl_uint32 size = nItems << 2;
				sl_uint32 limit = buffer->size - loc;
				if (size > limit) {
					size = limit;
				}
				if (!size) {
					return;
				}
				Base::copyMemory(buffer->data + loc, data, size);
				buffer->flagUpdated = sl_true;
#else
				sl_uint n4 = nItems >> 2;
				if (n4) {
					_setUniform(device, location, type, data, n4);
				}
				nItems = nItems & 3;
				if (nItems) {
					sl_uint8 t[16] = {};
					Base::copyMemory(t, data, nItems << 2);
					RenderUniformLocation l = location;
					l.registerNo += n4;
					_setUniform(device, l, type, t, 1);
				}
#endif
			}

#if D3D_VERSION_MAJOR >= 10
			static sl_bool createConstantBuffers(ID3DDevice* device, RenderProgram* program, List<ConstantBuffer>& buffersVS, List<ID3DBuffer*>& handlesVS, List<ConstantBuffer>& buffersPS, List<ID3DBuffer*>& handlesPS)
			{
				{
					sl_uint32 n = program->getVertexShaderConstantBufferCount();
					if (!n) {
						return sl_false;
					}
					for (sl_uint32 i = 0; i < n; i++) {
						ConstantBuffer buffer;
						if (buffer.init(device, program->getVertexShaderConstantBufferSize(i))) {
							if (!(handlesVS.add_NoLock(buffer.handle))) {
								return sl_false;
							}
							if (!(buffersVS.add_NoLock(Move(buffer)))) {
								return sl_false;
							}
						} else {
							return sl_false;
						}
					}
				}
				{
					sl_uint32 n = program->getPixelShaderConstantBufferCount();
					if (!n) {
						return sl_false;
					}
					for (sl_uint32 i = 0; i < n; i++) {
						ConstantBuffer buffer;
						if (buffer.init(device, program->getPixelShaderConstantBufferSize(i))) {
							if (!(handlesPS.add_NoLock(buffer.handle))) {
								return sl_false;
							}
							if (!(buffersPS.add_NoLock(Move(buffer)))) {
								return sl_false;
							}
						} else {
							return sl_false;
						}
					}
				}
				return sl_true;
			}

			void applyConstantBuffer(ID3DDeviceContext* context)
			{
				{
					ListElements<ConstantBuffer> list(constantBuffersVS);
					for (sl_size i = 0; i < list.count; i++) {
						ConstantBuffer& buffer = list[i];
						buffer.update(context);
					}
					context->VSSetConstantBuffers(0, (UINT)(list.count), constantBufferHandlesVS.getData());
				}
				{
					ListElements<ConstantBuffer> list(constantBuffersPS);
					for (sl_size i = 0; i < list.count; i++) {
						ConstantBuffer& buffer = list[i];
						buffer.update(context);
					}
					context->PSSetConstantBuffers(0, (UINT)(list.count), constantBufferHandlesPS.getData());
				}
			}
#else
			static void _setUniform(ID3DDevice* dev, const RenderUniformLocation& location, RenderUniformType type, const void* data, sl_uint32 countVector4)
			{
				if (location.shader == RenderShaderStage::Vertex) {
#if D3D_VERSION_MAJOR >= 9
					if (type == RenderUniformType::Float) {
						dev->SetVertexShaderConstantF((UINT)(location.registerNo), (float*)data, (UINT)countVector4);
					} else if (type == RenderUniformType::Int) {
						dev->SetVertexShaderConstantI((UINT)(location.registerNo), (int*)data, (UINT)countVector4);
					}
#else
					dev->SetVertexShaderConstant((DWORD)(location.registerNo), data, (DWORD)countVector4);
#endif
				} else if (location.shader == RenderShaderStage::Pixel) {
#if D3D_VERSION_MAJOR >= 9
					if (type == RenderUniformType::Float) {
						dev->SetPixelShaderConstantF((UINT)(location.registerNo), (float*)data, (UINT)countVector4);
					} else if (type == RenderUniformType::Int) {
						dev->SetPixelShaderConstantI((UINT)(location.registerNo), (int*)data, (UINT)countVector4);
					}
#else
					dev->SetPixelShaderConstant((DWORD)(location.registerNo), data, (DWORD)countVector4);
#endif
				}
			}
#endif

#if D3D_VERSION_MAJOR < 9
			static List<DWORD> createVertexDecl(ListParam<RenderInputLayoutItem>& _items)
			{
				ListLocker<RenderInputLayoutItem> items(_items);
				List<DWORD> decl;
				sl_uint32 slot = 0xffffffff;
				for (sl_size i = 0; i < items.count; i++) {
					RenderInputLayoutItem& item = items[i];
					if (item.slot != slot) {
						decl.add_NoLock(D3DVSD_STREAM((DWORD)item.slot));
						slot = item.slot;
					}
					DWORD type = 0;
					switch (item.type) {
						case RenderInputType::Float:
							type = D3DVSDT_FLOAT1;
							break;
						case RenderInputType::Float2:
							type = D3DVSDT_FLOAT2;
							break;
						case RenderInputType::Float3:
							type = D3DVSDT_FLOAT3;
							break;
						case RenderInputType::Float4:
							type = D3DVSDT_FLOAT4;
							break;
						case RenderInputType::UByte4:
							type = D3DVSDT_UBYTE4;
							break;
						case RenderInputType::Short2:
							type = D3DVSDT_SHORT2;
							break;
						case RenderInputType::Short4:
							type = D3DVSDT_SHORT4;
							break;
						default:
							break;
					}
					decl.add_NoLock(D3DVSD_REG((DWORD)i, type));
				}
				if (decl.isEmpty()) {
					return sl_null;
				}
				decl.add_NoLock(D3DVSD_END());
				return decl;
			}

			static DWORD getRegisterMapping(RenderInputSemanticName name, sl_uint32 index)
			{
				sl_bool flagError = sl_false;
				DWORD reg = 0;
				switch (name) {
					case RenderInputSemanticName::Position:
						reg = D3DVSDE_POSITION;
						break;
					case RenderInputSemanticName::BlendWeight:
						reg = D3DVSDE_BLENDWEIGHT;
						break;
					case RenderInputSemanticName::BlendIndices:
						reg = D3DVSDE_BLENDINDICES;
						break;
					case RenderInputSemanticName::Normal:
						reg = D3DVSDE_NORMAL;
						break;
					case RenderInputSemanticName::PSize:
						reg = D3DVSDE_PSIZE;
						break;
					case RenderInputSemanticName::TexCoord:
						reg = D3DVSDE_TEXCOORD0;
						break;
					case RenderInputSemanticName::Color:
						reg = D3DVSDE_DIFFUSE;
						break;
					case RenderInputSemanticName::Tangent:
					case RenderInputSemanticName::BiNormal:
					case RenderInputSemanticName::TessFactor:
					case RenderInputSemanticName::PositionT:
					case RenderInputSemanticName::Fog:
					case RenderInputSemanticName::Depth:
						flagError = sl_true;
						break;
				}
				if (!flagError && index) {
					if (reg == D3DVSDE_POSITION) {
						if (index == 1) {
							reg = D3DVSDE_POSITION2;
						} else {
							flagError = sl_true;
						}
					} else if (reg == D3DVSDE_NORMAL) {
						if (index == 1) {
							reg = D3DVSDE_NORMAL2;
						} else {
							flagError = sl_true;
						}
					} else if (reg == D3DVSDE_DIFFUSE) {
						if (index == 1) {
							reg = D3DVSDE_SPECULAR;
						} else {
							flagError = sl_true;
						}
					} else if (reg == D3DVSDE_TEXCOORD0) {
						if (index < 8) {
							reg += (DWORD)index;
						} else {
							flagError = sl_true;
						}
					} else {
						flagError = sl_true;
					}
				}
				if (flagError) {
					return 0xffffffff;
				} else {
					return reg;
				}
			}

			static Memory assembleShader(const StringParam& _source, Memory* pOutConstantsDefinition)
			{
				if (_source.isEmpty()) {
					return sl_null;
				}
				auto func = slib::d3dx8::getApi_D3DXAssembleShader();
				if (!func) {
					return sl_null;
				}
				ID3DXBuffer* shader = sl_null;
				ID3DXBuffer* error = sl_null;
				ID3DXBuffer* constants = sl_null;
				StringData source(_source);
				HRESULT hr = func(source.getData(), (UINT)(source.getLength()), 0, &constants, &shader, &error);
				Memory ret;
				if (hr == S_OK && shader) {
					ret = Memory::create(shader->GetBufferPointer(), (sl_size)(shader->GetBufferSize()));
					if (constants && pOutConstantsDefinition) {
						*pOutConstantsDefinition = Memory::create(constants->GetBufferPointer(), (sl_size)(constants->GetBufferSize()));
					}
				}
#ifdef SLIB_DEBUG
				else {
					if (error) {
						SLIB_LOG_DEBUG("D3DAssembleError", "hr=%d, %s", (sl_reg)hr, StringView((char*)(error->GetBufferPointer()), error->GetBufferSize()));
					} else {
						SLIB_LOG_DEBUG("D3DAssembleError", "hr=%d", (sl_reg)hr);
					}
				}
#endif
				if (error) {
					error->Release();
				}
				if (constants) {
					constants->Release();
				}
				if (shader) {
					shader->Release();
				}
				return ret;
			}
#endif

		};

#if D3D_VERSION_MAJOR < 10
		static void CreateDeviceBuffer(ID3DDevice* device, UINT size, ID3DVertexBuffer** _out)
		{
#if D3D_VERSION_MAJOR >= 9
			device->CreateVertexBuffer(size, 0, 0, D3DPOOL_MANAGED, _out, NULL);
#else
			device->CreateVertexBuffer(size, 0, 0, D3DPOOL_MANAGED, _out);
#endif
		}

		static void CreateDeviceBuffer(ID3DDevice* device, UINT size, ID3DIndexBuffer** _out)
		{
#if D3D_VERSION_MAJOR >= 9
			device->CreateIndexBuffer(size, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, _out, NULL);
#else
			device->CreateIndexBuffer(size, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, _out);
#endif
		}
#endif

		template <class BUFFER, class BUFFER_INSTANCE, class DEVICE_BUFFER, sl_bool flagVertexBuffer>
		class RenderBufferInstanceImpl : public BUFFER_INSTANCE
		{
		public:
			ID3DDevice* device;
			ID3DDeviceContext* context;
			DEVICE_BUFFER* handle;
			sl_uint32 size;
			sl_bool flagLockable;

		public:
			RenderBufferInstanceImpl()
			{
				device = sl_null;
				context = sl_null;
				handle = sl_null;
				size = 0;
				flagLockable = sl_false;
			}

			~RenderBufferInstanceImpl()
			{
				if (handle) {
					handle->Release();
				}
			}

		public:
			static Ref<RenderBufferInstanceImpl> create(ID3DDevice* device, ID3DDeviceContext* context, RenderEngine* engine, BUFFER* buffer)
			{
				UINT size = (UINT)(buffer->getSize());
				if (!size) {
					return sl_null;
				}
				Memory content = buffer->getSource();
				if (content.getSize() < size) {
					return sl_null;
				}
				DEVICE_BUFFER* handle = sl_null;
#if D3D_VERSION_MAJOR >= 10
				sl_bool flagLockable = sl_false;
				D3D_(BUFFER_DESC) desc = {};
				desc.BindFlags = flagVertexBuffer ? D3D_(BIND_VERTEX_BUFFER) : D3D_(BIND_INDEX_BUFFER);
				desc.ByteWidth = size;
				PrepareObjectFlags(buffer->getFlags(), desc.Usage, desc.CPUAccessFlags, flagLockable);
				D3D_(SUBRESOURCE_DATA) data = {};
				data.pSysMem = content.getData();
				device->CreateBuffer(&desc, &data, &handle);
#else
				sl_bool flagLockable = sl_true;
				CreateDeviceBuffer(device, size, &handle);
#endif
				if (handle) {
#if D3D_VERSION_MAJOR < 10
					void* data = sl_null;
#if D3D_VERSION_MAJOR >= 9
					handle->Lock(0, size, &data, 0);
#else
					handle->Lock(0, size, (BYTE**)&data, 0);
#endif
					if (data) {
						Base::copyMemory(data, content.getData(), size);
						handle->Unlock();
#endif
						Ref<RenderBufferInstanceImpl> ret = new RenderBufferInstanceImpl;
						if (ret.isNotNull()) {
							ret->device = device;
							ret->context = context;
							ret->handle = handle;
							ret->link(engine, buffer);
							ret->size = (sl_uint32)size;
							ret->flagLockable = flagLockable;
							return ret;
						}
#if D3D_VERSION_MAJOR < 10
					}
#endif
					handle->Release();
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
				BUFFER* buffer = (BUFFER*)object;
				Memory content = buffer->getSource();
				if (content.getSize() < offset + size) {
					return;
				}
				void* data = sl_null;
#if D3D_VERSION_MAJOR >= 10
				if (!flagLockable) {
					context->UpdateSubresource(handle, 0, NULL, content.getData(), 0, 0);
					return;
				}
#if D3D_VERSION_MAJOR >= 11
				D3D_(MAPPED_SUBRESOURCE) res = {};
				context->Map(handle, 0, D3D_(MAP_WRITE), 0, &res);
				data = res.pData;
#else
				handle->Map(D3D_(MAP_WRITE), 0, &data);
#endif
#else
#if D3D_VERSION_MAJOR >= 9
				handle->Lock(offset, size, &data, 0);
#else
				handle->Lock(offset, size, (BYTE**)&data, 0);
#endif
#endif
				if (data) {
					Base::copyMemory(data, (sl_uint8*)(content.getData()) + offset, size);
#if D3D_VERSION_MAJOR >= 11
					context->Unmap(handle, 0);
#elif D3D_VERSION_MAJOR >= 10
					handle->Unmap();
#else
					handle->Unlock();
#endif
				}
			}

		};

		typedef RenderBufferInstanceImpl<VertexBuffer, VertexBufferInstance, ID3DVertexBuffer, sl_true> VertexBufferInstanceImpl;

		typedef RenderBufferInstanceImpl<IndexBuffer, IndexBufferInstance, ID3DIndexBuffer, sl_false> IndexBufferInstanceImpl;


		class TextureInstanceImpl : public TextureInstance
		{
		public:
			ID3DDevice* device;
			ID3DDeviceContext* context;
			ID3DTexture2D* handle;
#if D3D_VERSION_MAJOR >= 10
			ID3DShaderResourceView* view;
#endif
			sl_bool flagLockable;

		public:
			TextureInstanceImpl()
			{
				device = sl_null;
				context = sl_null;
				handle = sl_null;
#if D3D_VERSION_MAJOR >= 10
				view = sl_null;
#endif
				flagLockable = sl_false;
			}

			~TextureInstanceImpl()
			{
				if (handle) {
					handle->Release();
				}
#if D3D_VERSION_MAJOR >= 10
				if (view) {
					view->Release();
				}
#endif
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
				ID3DTexture2D* handle = sl_null;
#if D3D_VERSION_MAJOR >= 10
				Ref<Image> image = source->toImage();
				if (image.isNull()) {
					return sl_null;
				}
				sl_bool flagLockable = sl_false;
				D3D_(TEXTURE2D_DESC) desc = {};
				desc.Width = (UINT)width;
				desc.Height = (UINT)height;
				desc.MipLevels = 1;
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				desc.SampleDesc.Count = 1;
				desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
				desc.ArraySize = 1;
				PrepareObjectFlags(texture->getFlags(), desc.Usage, desc.CPUAccessFlags, flagLockable);
				D3D_(SUBRESOURCE_DATA) data = {};
				data.pSysMem = image->getColors();
				data.SysMemPitch = (UINT)(image->getStride() << 2);
				device->CreateTexture2D(&desc, &data, &handle);
#else
				sl_bool flagLockable = sl_true;
#if D3D_VERSION_MAJOR >= 9
				device->CreateTexture((UINT)width, (UINT)height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &handle, NULL);
#else
				device->CreateTexture((UINT)width, (UINT)height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &handle);
#endif
#endif
				if (handle) {
#if D3D_VERSION_MAJOR >= 10
					ID3DShaderResourceView* view = sl_null;
					D3D_(SHADER_RESOURCE_VIEW_DESC) view_desc = {};
					view_desc.Format = desc.Format;
					view_desc.ViewDimension = D3D_(SRV_DIMENSION_TEXTURE2D);
					view_desc.Texture2D.MipLevels = desc.MipLevels;
					device->CreateShaderResourceView(handle, &view_desc, &view);
					if (view) {
#else
					D3DLOCKED_RECT lr;
					HRESULT hr = handle->LockRect(0, &lr, NULL, 0);
					if (hr == D3D_OK) {
						BitmapData bd;
						bd.format = BitmapFormat::BGRA;
						bd.data = lr.pBits;
						bd.pitch = (sl_int32)(lr.Pitch);
						bd.width = width;
						bd.height = height;
						source->readPixels(0, 0, bd);
						handle->UnlockRect(0);
#endif
						Ref<TextureInstanceImpl> ret = new TextureInstanceImpl();
						if (ret.isNotNull()) {
							ret->device = device;
							ret->context = context;
							ret->handle = handle;
#if D3D_VERSION_MAJOR >= 10
							ret->view = view;
#endif
							ret->flagLockable = flagLockable;
							ret->link(engine, texture);
							return ret;
						}
#if D3D_VERSION_MAJOR >= 10
						view->Release();
#endif
					}
					handle->Release();
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
#if D3D_VERSION_MAJOR >= 10
				if (!flagLockable) {
					Ref<Image> image = source->toImage();
					if (image.isNull()) {
						return;
					}
					context->UpdateSubresource(handle, 0, NULL, image->getColors(), (UINT)(image->getStride() << 2), 0);
					return;
				}
#if D3D_VERSION_MAJOR >= 11
				D3D_(MAPPED_SUBRESOURCE) res = {};
				context->Map(handle, 0, D3D_(MAP_WRITE), 0, &res);
				void* data = res.pData;
				sl_int32 pitch = (sl_int32)(res.RowPitch);
#else
				D3D_(MAPPED_TEXTURE2D) mt = {};
				handle->Map(0, D3D_(MAP_WRITE), 0, &mt);
				void* data = mt.pData;
				sl_int32 pitch = (sl_int32)(mt.RowPitch);
#endif
#else
				D3DLOCKED_RECT lr;
				RECT rc;
				rc.left = (LONG)(m_updatedRegion.left);
				rc.top = (LONG)(m_updatedRegion.top);
				rc.right = (LONG)(m_updatedRegion.right);
				rc.bottom = (LONG)(m_updatedRegion.bottom);
				handle->LockRect(0, &lr, &rc, 0);
				void* data = lr.pBits;
				sl_int32 pitch = (sl_int32)(lr.Pitch);
#endif
				if (data) {
					BitmapData bd;
					bd.format = BitmapFormat::BGRA;
					bd.data = data;
					bd.pitch = pitch;
					bd.width = m_updatedRegion.getWidth();
					bd.height = m_updatedRegion.getHeight();
					source->readPixels(m_updatedRegion.left, m_updatedRegion.top, bd);
#if D3D_VERSION_MAJOR >= 11
					context->Unmap(handle, 0);
#elif D3D_VERSION_MAJOR >= 10
					handle->Unmap(0);
#else
					handle->UnlockRect(0);
#endif
				}
			}

		};

#if D3D_VERSION_MAJOR >= 10
		static D3D_(COMPARISON_FUNC) GetComparisonFunction(RenderFunctionOperation op)
		{
			switch (op) {
				case RenderFunctionOperation::Never:
					return D3D_(COMPARISON_NEVER);
				case RenderFunctionOperation::Always:
					return D3D_(COMPARISON_ALWAYS);
				case RenderFunctionOperation::Equal:
					return D3D_(COMPARISON_EQUAL);
				case RenderFunctionOperation::NotEqual:
					return D3D_(COMPARISON_NOT_EQUAL);
				case RenderFunctionOperation::Less:
					return D3D_(COMPARISON_LESS);
				case RenderFunctionOperation::LessEqual:
					return D3D_(COMPARISON_LESS_EQUAL);
				case RenderFunctionOperation::Greater:
					return D3D_(COMPARISON_GREATER);
				case RenderFunctionOperation::GreaterEqual:
					return D3D_(COMPARISON_GREATER_EQUAL);
				default:
					break;
			}
			return D3D_(COMPARISON_NEVER);
		}

		static void CreateState(ID3DDevice* device, const RenderDepthStencilParam& param, ID3DDepthStencilState** state)
		{
			D3D_(DEPTH_STENCIL_DESC) desc = {};
			desc.DepthEnable = param.flagTestDepth ? TRUE : FALSE;
			desc.DepthWriteMask = param.flagWriteDepth ? D3D_(DEPTH_WRITE_MASK_ALL) : D3D_(DEPTH_WRITE_MASK_ZERO);
			desc.DepthFunc = GetComparisonFunction(param.depthFunction);
			desc.StencilEnable = param.flagStencil ? TRUE : FALSE;
			desc.StencilReadMask = (UINT8)(param.stencilReadMask);
			desc.StencilWriteMask = (UINT8)(param.stencilWriteMask);
			desc.FrontFace.StencilFailOp = D3D_(STENCIL_OP_KEEP);
			desc.FrontFace.StencilDepthFailOp = D3D_(STENCIL_OP_KEEP);
			desc.FrontFace.StencilPassOp = D3D_(STENCIL_OP_KEEP);
			desc.FrontFace.StencilFunc = D3D_(COMPARISON_ALWAYS);
			desc.BackFace.StencilFailOp = D3D_(STENCIL_OP_KEEP);
			desc.BackFace.StencilDepthFailOp = D3D_(STENCIL_OP_KEEP);
			desc.BackFace.StencilPassOp = D3D_(STENCIL_OP_KEEP);
			desc.BackFace.StencilFunc = D3D_(COMPARISON_ALWAYS);
			device->CreateDepthStencilState(&desc, state);
		}

		static void SetState(ID3DDeviceContext* context, ID3DDepthStencilState* state, const RenderDepthStencilParam& param, sl_int32 samplerNo)
		{
			context->OMSetDepthStencilState(state, (UINT)(param.stencilRef));
		}

		static void CreateState(ID3DDevice* device, const RenderRasterizerParam& param, ID3DRasterizerState** state)
		{
			D3D_(RASTERIZER_DESC) desc = {};
			desc.FillMode = param.flagWireFrame ? D3D_(FILL_WIREFRAME) : D3D_(FILL_SOLID);
			desc.CullMode = param.flagCull ? (param.flagCullCCW ? D3D_(CULL_BACK) : D3D_(CULL_FRONT)) : D3D_(CULL_NONE);
			desc.FrontCounterClockwise = FALSE;
			desc.DepthBias = 0;
			desc.DepthBiasClamp = 0.0f;
			desc.SlopeScaledDepthBias = 0.0f;
			desc.DepthClipEnable = TRUE;
			desc.ScissorEnable = FALSE;
			desc.MultisampleEnable = param.flagMultiSample ? TRUE : FALSE;
			desc.AntialiasedLineEnable = param.flagMultiSample ? TRUE : FALSE;
			device->CreateRasterizerState(&desc, state);
		}

		static void SetState(ID3DDeviceContext* context, ID3DRasterizerState* state, const RenderRasterizerParam& param, sl_int32 samplerNo)
		{
			context->RSSetState(state);
		}

		static D3D_(BLEND_OP) GetBlendOperation( RenderBlendingOperation op)
		{
			switch (op) {
				case RenderBlendingOperation::Add:
					return D3D_(BLEND_OP_ADD);
				case RenderBlendingOperation::Subtract:
					return D3D_(BLEND_OP_SUBTRACT);
				case RenderBlendingOperation::ReverseSubtract:
					return D3D_(BLEND_OP_REV_SUBTRACT);
				default:
					break;
			}
			return D3D_(BLEND_OP_ADD);
		}

		static D3D_(BLEND) GetBlendFactor(RenderBlendingFactor f)
		{
			switch (f) {
				case RenderBlendingFactor::One:
					return D3D_(BLEND_ONE);
				case RenderBlendingFactor::Zero:
					return D3D_(BLEND_ZERO);
				case RenderBlendingFactor::SrcAlpha:
					return D3D_(BLEND_SRC_ALPHA);
				case RenderBlendingFactor::OneMinusSrcAlpha:
					return D3D_(BLEND_INV_SRC_ALPHA);
				case RenderBlendingFactor::DstAlpha:
					return D3D_(BLEND_DEST_ALPHA);
				case RenderBlendingFactor::OneMinusDstAlpha:
					return D3D_(BLEND_INV_DEST_ALPHA);
				case RenderBlendingFactor::SrcColor:
					return D3D_(BLEND_SRC_COLOR);
				case RenderBlendingFactor::OneMinusSrcColor:
					return D3D_(BLEND_INV_SRC_COLOR);
				case RenderBlendingFactor::DstColor:
					return D3D_(BLEND_DEST_COLOR);
				case RenderBlendingFactor::OneMinusDstColor:
					return D3D_(BLEND_INV_DEST_COLOR);
				case RenderBlendingFactor::SrcAlphaSaturate:
					return D3D_(BLEND_SRC_ALPHA_SAT);
				case RenderBlendingFactor::Constant:
					return D3D_(BLEND_BLEND_FACTOR);
				case RenderBlendingFactor::OneMinusConstant:
					return D3D_(BLEND_INV_BLEND_FACTOR);
				default:
					break;
			}
			return D3D_(BLEND_ONE);
		}

		void CreateState(ID3DDevice* device, const RenderBlendParam& param, ID3DBlendState** state)
		{
			D3D_(BLEND_DESC) desc = {};
#if D3D_VERSION_MAJOR >= 11
			for (sl_uint k = 0; k < 8; k++) {
				desc.RenderTarget[k].DestBlend = D3D_(BLEND_ZERO);
				desc.RenderTarget[k].BlendOp = D3D_(BLEND_OP_ADD);
				desc.RenderTarget[k].SrcBlendAlpha = D3D_(BLEND_ONE);
				desc.RenderTarget[k].DestBlendAlpha = D3D_(BLEND_ZERO);
				desc.RenderTarget[k].BlendOpAlpha = D3D_(BLEND_OP_ADD);
				desc.RenderTarget[k].RenderTargetWriteMask = D3D_(COLOR_WRITE_ENABLE_ALL);
			}
			desc.RenderTarget[0].BlendEnable = param.flagBlending ? TRUE : FALSE;
			desc.RenderTarget[0].BlendOp = GetBlendOperation(param.operation);
			desc.RenderTarget[0].BlendOpAlpha = GetBlendOperation(param.operationAlpha);
			desc.RenderTarget[0].SrcBlend = GetBlendFactor(param.blendSrc);
			desc.RenderTarget[0].DestBlend = GetBlendFactor(param.blendDst);
			desc.RenderTarget[0].SrcBlendAlpha = GetBlendFactor(param.blendSrcAlpha);
			desc.RenderTarget[0].DestBlendAlpha = GetBlendFactor(param.blendDstAlpha);
#else
			desc.BlendEnable[0] = param.flagBlending ? TRUE : FALSE;
			desc.BlendOp = GetBlendOperation(param.operation);
			desc.BlendOpAlpha = GetBlendOperation(param.operationAlpha);
			desc.SrcBlend = GetBlendFactor(param.blendSrc);
			desc.DestBlend = GetBlendFactor(param.blendDst);
			desc.SrcBlendAlpha = GetBlendFactor(param.blendSrcAlpha);
			desc.DestBlendAlpha = GetBlendFactor(param.blendDstAlpha);
			for (sl_uint k = 0; k < 8; k++) {
				desc.RenderTargetWriteMask[k] = D3D_(COLOR_WRITE_ENABLE_ALL);
			}
#endif
			device->CreateBlendState(&desc, state);
		}

		static void SetState(ID3DDeviceContext* context, ID3DBlendState* state, const RenderBlendParam& param, sl_int32 samplerNo)
		{
			FLOAT f[] = { param.blendConstant.x, param.blendConstant.y, param.blendConstant.z, param.blendConstant.w };
			context->OMSetBlendState(state, f, 0xffffffff);
		}

		static D3D_(TEXTURE_ADDRESS_MODE) GetTextureAddressMode(TextureWrapMode mode)
		{
			switch (mode) {
				case TextureWrapMode::Clamp:
					return D3D_(TEXTURE_ADDRESS_CLAMP);
				case TextureWrapMode::Mirror:
					return D3D_(TEXTURE_ADDRESS_MIRROR);
				case TextureWrapMode::Repeat:
					return D3D_(TEXTURE_ADDRESS_WRAP);
				default:
					break;
			}
			return D3D_(TEXTURE_ADDRESS_CLAMP);
		}

		static void CreateState(ID3DDevice* device, const RenderSamplerParam& param, ID3DSamplerState** state)
		{
			D3D_(SAMPLER_DESC) desc = {};
			if (param.magFilter == TextureFilterMode::Point) {
				if (param.minFilter == TextureFilterMode::Point) {
					desc.Filter = D3D_(FILTER_MIN_MAG_MIP_POINT);
				} else {
					desc.Filter = D3D_(FILTER_MIN_POINT_MAG_MIP_LINEAR);
				}
			} else {
				if (param.minFilter == TextureFilterMode::Point) {
					desc.Filter = D3D_(FILTER_MIN_LINEAR_MAG_MIP_POINT);
				} else {
					desc.Filter = D3D_(FILTER_MIN_MAG_MIP_LINEAR);
				}
			}
			desc.AddressU = GetTextureAddressMode(param.wrapX);
			desc.AddressV = GetTextureAddressMode(param.wrapY);
			desc.AddressW = D3D_(TEXTURE_ADDRESS_CLAMP);
			desc.MaxLOD = 3.402823466e+38f;
			desc.MaxAnisotropy = 16;
			desc.ComparisonFunc = D3D_(COMPARISON_NEVER);
			device->CreateSamplerState(&desc, state);
		}

		static void SetState(ID3DDeviceContext* context, ID3DSamplerState* state, const RenderSamplerParam& param, sl_int32 samplerNo)
		{
			context->PSSetSamplers((UINT)(samplerNo), 1, &state);
		}

		template <class STATE, class STATE_HANDLE>
		class StateInstanceImpl : public RenderBaseObjectInstance
		{
		public:
			STATE_HANDLE* handle;

		public:
			StateInstanceImpl()
			{
				handle = sl_null;
			}

			~StateInstanceImpl()
			{
				if (handle) {
					handle->Release();
				}
			}

		public:
			static void setState(ID3DDevice* device, ID3DDeviceContext* context, RenderEngine* engine, STATE* state, sl_int32 samplerNo = 0)
			{
				Ref<RenderBaseObjectInstance> _instance = state->getInstance(engine);
				if (_instance.isNotNull()) {
					SetState(context, ((StateInstanceImpl*)(_instance.get()))->handle, state->getParam(), samplerNo);
					return;
				}
				STATE_HANDLE* handle = sl_null;
				CreateState(device, state->getParam(), &handle);
				if (handle) {
					Ref<StateInstanceImpl> instance = new StateInstanceImpl;
					if (instance.isNotNull()) {
						instance->handle = handle;
						instance->link(engine, state);
						SetState(context, handle, state->getParam(), samplerNo);
						return;
					}
					handle->Release();
				}
			}

		};
#endif

		class EngineImpl : public RenderEngine
		{
		public:
			RendererImpl* m_renderer = sl_null;

			Ref<RenderProgram> m_currentProgram;
			Ref<RenderProgramInstanceImpl> m_currentProgramInstance;
#if D3D_VERSION_MAJOR >= 9
			Ref<RenderInputLayoutImpl> m_currentInputLayout;
#endif
			sl_uint32 m_currentVertexStride = 0;
			Ref<VertexBufferInstanceImpl> m_currentVertexBufferInstance;
			Ref<IndexBufferInstanceImpl> m_currentIndexBufferInstance;
			Ref<RenderProgram> m_currentProgramRendering;
			Ref<RenderProgramInstanceImpl> m_currentProgramInstanceRendering;

		public:
			RendererParam* getRendererParam()
			{
				RendererImpl* renderer = m_renderer;
				if (renderer) {
					return &(renderer->m_param);
				}
				return sl_null;
			}

			ID3DDevice* getDevice()
			{
				RendererImpl* renderer = m_renderer;
				if (renderer) {
					return renderer->m_device;
				}
				return sl_null;
			}

			ID3DDeviceContext* getContext()
			{
				RendererImpl* renderer = m_renderer;
				if (renderer) {
					return renderer->m_context;
				}
				return sl_null;
			}

#if D3D_VERSION_MAJOR >= 10
			IDXGISwapChain* getSwapChain()
			{
				RendererImpl* renderer = m_renderer;
				if (renderer) {
					return renderer->m_pSwapChain;
				}
				return sl_null;
			}

			ID3DRenderTargetView* getRenderTarget()
			{
				RendererImpl* renderer = m_renderer;
				if (renderer) {
					return renderer->m_pRenderTarget;
				}
				return sl_null;
			}

			ID3DDepthStencilView* getDepthStencil()
			{
				RendererImpl* renderer = m_renderer;
				if (renderer) {
					return renderer->m_pDepthStencil;
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
				return Ref<RenderProgramInstance>::cast(RenderProgramInstanceImpl::create(device, context, this, program));
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
				return Ref<VertexBufferInstance>::cast(VertexBufferInstanceImpl::create(device, context, this, buffer));
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
				return Ref<IndexBufferInstance>::cast(IndexBufferInstanceImpl::create(device, context, this, buffer));
			}

			Ref<TextureInstance> _createTextureInstance(Texture* texture, sl_int32 sampler) override
			{
				ID3DDevice* device = getDevice();
				if (!device) {
					return sl_null;
				}
				ID3DDeviceContext* context = getContext();
				if (!context) {
					return sl_null;
				}
				Ref<TextureInstanceImpl> ret = TextureInstanceImpl::create(device, context, this, texture);
				if (ret.isNotNull()) {
					_applyTexture(texture, ret.get(), sampler);
					return Ref<TextureInstance>::cast(ret);
				}
				return sl_null;
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
				D3DVIEWPORT v;
				v.X = (DWORD)x;
				v.Y = (DWORD)y;
				v.Width = (DWORD)width;
				v.Height = (DWORD)height;
				v.MinZ = 0.0f;
				v.MaxZ = 1.0f;
				context->SetViewport(&v);
#endif
			}

			void _clear(const ClearParam& param) override
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
				if (pRenderTarget) {
					if (param.flagColor) {
						float c[4];
						c[0] = param.color.getRedF();
						c[1] = param.color.getGreenF();
						c[2] = param.color.getBlueF();
						c[3] = param.color.getAlphaF();
						context->ClearRenderTargetView(pRenderTarget, c);
					}
				}
				ID3DDepthStencilView* pDepthStencil = getDepthStencil();
				if (pDepthStencil) {
					UINT flags = 0;
					if (param.flagDepth) {
						flags |= D3D10_CLEAR_DEPTH;
					}
					if (param.flagStencil) {
						flags |= D3D10_CLEAR_STENCIL;
					}
					if (flags) {
						context->ClearDepthStencilView(pDepthStencil, flags, param.depth, (UINT8)(param.stencil));
					}
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

			void _setDepthStencilState(RenderDepthStencilState* state) override
			{
				ID3DDeviceContext* context = getContext();
				if (!context) {
					return;
				}
#if D3D_VERSION_MAJOR >= 10
				ID3DDevice* device = getDevice();
				if (!device) {
					return;
				}
				StateInstanceImpl<RenderDepthStencilState, ID3DDepthStencilState>::setState(device, context, this, state);
#else
				const RenderDepthStencilParam& param = state->getParam();
				context->SetRenderState(D3DRS_ZENABLE, param.flagTestDepth ? D3DZB_TRUE : D3DZB_FALSE);
				context->SetRenderState(D3DRS_ZWRITEENABLE, param.flagWriteDepth ? TRUE : FALSE);
				DWORD v;
				switch (param.depthFunction) {
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
						v = D3DCMP_NEVER;
						break;
				}
				context->SetRenderState(D3DRS_ZFUNC, v);
				context->SetRenderState(D3DRS_STENCILENABLE, param.flagStencil ? TRUE : FALSE);
				context->SetRenderState(D3DRS_STENCILMASK, (DWORD)(param.stencilReadMask));
				context->SetRenderState(D3DRS_STENCILWRITEMASK, (DWORD)(param.stencilWriteMask));
				context->SetRenderState(D3DRS_STENCILREF, (DWORD)(param.stencilRef));
#endif
			}

			void _setRasterizerState(RenderRasterizerState* state) override
			{
				ID3DDeviceContext* context = getContext();
				if (!context) {
					return;
				}
#if D3D_VERSION_MAJOR >= 10
				ID3DDevice* device = getDevice();
				if (!device) {
					return;
				}
				StateInstanceImpl<RenderRasterizerState, ID3DRasterizerState>::setState(device, context, this, state);
#else
				const RenderRasterizerParam& param = state->getParam();
				context->SetRenderState(D3DRS_CULLMODE, param.flagCull ? (param.flagCullCCW ? D3DCULL_CCW : D3DCULL_CW) : D3DCULL_NONE);
				context->SetRenderState(D3DRS_FILLMODE, param.flagWireFrame ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
				context->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, param.flagMultiSample ? TRUE : FALSE);
#endif
			}

			void _setBlendState(RenderBlendState* state) override
			{
				ID3DDeviceContext* context = getContext();
				if (!context) {
					return;
				}
#if D3D_VERSION_MAJOR >= 10
				ID3DDevice* device = getDevice();
				if (!device) {
					return;
				}
				StateInstanceImpl<RenderBlendState, ID3DBlendState>::setState(device, context, this, state);
#else
				const RenderBlendParam& param = state->getParam();
				context->SetRenderState(D3DRS_ALPHABLENDENABLE, param.flagBlending ? TRUE : FALSE);
				if (param.flagBlending) {
					_setBlendOperation(context, D3DRS_BLENDOP, param.operation);
					_setBlendFactor(context, D3DRS_SRCBLEND, param.blendSrc);
					_setBlendFactor(context, D3DRS_DESTBLEND, param.blendDst);
#if D3D_VERSION_MAJOR >= 9
					_setBlendOperation(context, D3DRS_BLENDOPALPHA, param.operationAlpha);
					_setBlendFactor(context, D3DRS_SRCBLENDALPHA, param.blendSrcAlpha);
					_setBlendFactor(context, D3DRS_DESTBLENDALPHA, param.blendDstAlpha);
					Color f(param.blendConstant);
					context->SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_ARGB(f.a, f.r, f.g, f.b));
#endif
				}
#endif
			}

			void _setSamplerState(sl_int32 samplerNo, RenderSamplerState* state) override
			{
				ID3DDeviceContext* context = getContext();
				if (!context) {
					return;
				}
#if D3D_VERSION_MAJOR >= 10
				ID3DDevice* device = getDevice();
				if (!device) {
					return;
				}
				StateInstanceImpl<RenderSamplerState, ID3DSamplerState> ::setState(device, context, this, state, samplerNo);
#else
				const RenderSamplerParam& param = state->getParam();
				DWORD s = (DWORD)samplerNo;
#if D3D_VERSION_MAJOR >= 9
				context->SetSamplerState(s, D3DSAMP_MAGFILTER, _getTextureFilterType(param.magFilter));
				context->SetSamplerState(s, D3DSAMP_MINFILTER, _getTextureFilterType(param.minFilter));
				context->SetSamplerState(s, D3DSAMP_ADDRESSU, _getTextureWrapType(param.wrapX));
				context->SetSamplerState(s, D3DSAMP_ADDRESSV, _getTextureWrapType(param.wrapY));
#else
				context->SetTextureStageState(s, D3DTSS_MAGFILTER, _getTextureFilterType(param.magFilter));
				context->SetTextureStageState(s, D3DTSS_MINFILTER, _getTextureFilterType(param.minFilter));
				context->SetTextureStageState(s, D3DTSS_ADDRESSU, _getTextureWrapType(param.wrapX));
				context->SetTextureStageState(s, D3DTSS_ADDRESSV, _getTextureWrapType(param.wrapY));
#endif
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
#elif D3D_VERSION_MAJOR >= 9
					context->SetVertexShader(instance->vertexShader);
					context->SetPixelShader(instance->pixelShader);
#else
					context->SetVertexShader(instance->hVertexShader);
					context->SetPixelShader(instance->hPixelShader);
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
						nPrimitives = primitive->elementCount / 3;
						break;
					case PrimitiveType::TriangleStrip:
						type = D3DPT_TRIANGLESTRIP;
						nPrimitives = primitive->elementCount - 2;
						break;
					case PrimitiveType::TriangleFan:
						type = D3DPT_TRIANGLEFAN;
						nPrimitives = primitive->elementCount - 2;
						break;
					case PrimitiveType::Line:
						type = D3DPT_LINELIST;
						nPrimitives = primitive->elementCount / 2;
						break;
					case PrimitiveType::LineStrip:
						type = D3DPT_LINESTRIP;
						nPrimitives = primitive->elementCount - 1;
						break;
					case PrimitiveType::Point:
						type = D3DPT_POINTLIST;
						nPrimitives = primitive->elementCount;
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

#if D3D_VERSION_MAJOR >= 9
				RenderInputLayoutImpl* layout = m_currentInputLayout.get();
				if (!layout) {
					return;
				}
#endif
#if D3D_VERSION_MAJOR < 10
				sl_uint32 nVerticesMin = 0;
#endif
				VertexBufferInstanceImpl* vb = static_cast<VertexBufferInstanceImpl*>(primitive->vertexBufferInstance.get());
				if (vb) {
					vb->doUpdate(primitive->vertexBuffer.get());
#if D3D_VERSION_MAJOR >= 9
					UINT stride = (UINT)(layout->strides.getValueAt_NoLock(0));
#else
					UINT stride = (UINT)(m_currentProgramInstanceRendering->strides.getValueAt_NoLock(0));
#endif
#if D3D_VERSION_MAJOR >= 10
					UINT offset = 0;
					context->IASetVertexBuffers(0, 1, &(vb->handle), &stride, &offset);
#else
#if D3D_VERSION_MAJOR >= 9
					context->SetStreamSource(0, vb->handle, 0, stride);
#else
					context->SetStreamSource(0, vb->handle, stride);
#endif
					if (stride) {
						nVerticesMin = vb->size / stride;
					}
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
					if (!(bufs && strides && offsets)) {
						return;
					}
#endif
					for (sl_size i = 0; i < list.count; i++) {
						VertexBufferInstanceImpl* vb = (VertexBufferInstanceImpl*)(list[i].get());
#if D3D_VERSION_MAJOR >= 9
						UINT stride = (UINT)(layout->strides.getValueAt_NoLock(i));
#else
						UINT stride = (UINT)(m_currentProgramInstanceRendering->strides.getValueAt_NoLock(i));
#endif
#if D3D_VERSION_MAJOR >= 10
						bufs[i] = vb->handle;
						offsets[i] = 0;
						strides[i] = stride;
#else
#if D3D_VERSION_MAJOR >= 9
						context->SetStreamSource((UINT)i, vb->handle, 0, stride);
#else
						context->SetStreamSource((UINT)i, vb->handle, stride);
#endif
						if (stride) {
							sl_uint32 nVertices = vb->size / stride;
							if (!nVerticesMin || nVertices < nVerticesMin) {
								nVerticesMin = nVertices;
							}
						}
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
					context->IASetIndexBuffer(ib->handle, DXGI_FORMAT_R16_UINT, 0);
					context->DrawIndexed((UINT)(primitive->elementCount), 0, 0);
#else
#if D3D_VERSION_MAJOR >= 9
					context->SetIndices(ib->handle);
					context->DrawIndexedPrimitive(type, 0, 0, (UINT)nVerticesMin, 0, (UINT)nPrimitives);
#else
					context->SetIndices(ib->handle, 0);
					context->DrawIndexedPrimitive(type, 0, (UINT)nVerticesMin, 0, (UINT)nPrimitives);
#endif
#endif
				} else {
#if D3D_VERSION_MAJOR >= 10
					context->Draw((UINT)(primitive->elementCount), 0);
#else
					context->DrawPrimitive(type, 0, (UINT)nPrimitives);
#endif
				}
			}

			void _applyTexture(Texture* texture, TextureInstance* _instance, sl_int32 sampler) override
			{
				ID3DDeviceContext* context = getContext();
				if (!context) {
					return;
				}
				TextureInstanceImpl* instance = (TextureInstanceImpl*)_instance;
				if (!instance) {
					return;
				}
				instance->doUpdate(texture);
#if D3D_VERSION_MAJOR >= 10
				context->PSSetShaderResources((UINT)sampler, 1, &(instance->view));
#else
				context->SetTexture((DWORD)sampler, instance->handle);
#endif
			}

#if D3D_VERSION_MAJOR < 9
			sl_bool isInputLayoutAvailable() override
			{
				return sl_false;
			}
#endif

			void _setInputLayout(RenderInputLayout* _layout) override
			{
#if D3D_VERSION_MAJOR >= 9
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
#endif
			}

			void _setLineWidth(sl_real width) override
			{
			}

#if D3D_VERSION_MAJOR < 10
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
#if D3D_VERSION_MAJOR >= 9
					case RenderBlendingFactor::Constant:
						v = D3DBLEND_BLENDFACTOR;
						break;
					case RenderBlendingFactor::OneMinusConstant:
						v = D3DBLEND_INVBLENDFACTOR;
						break;
#endif
					default:
						return;
				}
				context->SetRenderState(state, v);
			}

			static D3DTEXTUREFILTERTYPE _getTextureFilterType(TextureFilterMode mode)
			{
				if (mode == TextureFilterMode::Point) {
					return D3DTEXF_POINT;
				} else {
					return D3DTEXF_LINEAR;
				}
			}

			static D3DTEXTUREADDRESS _getTextureWrapType(TextureWrapMode mode)
			{
				switch (mode) {
					case TextureWrapMode::Clamp:
						return D3DTADDRESS_CLAMP;
					case TextureWrapMode::Mirror:
						return D3DTADDRESS_MIRROR;
					case TextureWrapMode::Repeat:
						return D3DTADDRESS_WRAP;
					default:
						break;
				}
				return D3DTADDRESS_WRAP;
			}
#endif

		};

		void RendererImpl::run()
		{
			Thread* thread = Thread::getCurrent();
			if (!thread) {
				return;
			}

			Ref<EngineImpl> engine = new EngineImpl;
			if (engine.isNull()) {
				return;
			}
			engine->m_renderer = this;

			RECT rect;
			GetClientRect(m_hWnd, &rect);
			LONG lastWidth = rect.right;
			LONG lastHeight = rect.bottom;

			TimeCounter timer;
			while (thread->isNotStopping()) {
				Ref<RendererImpl> thiz = this;
				GetClientRect(m_hWnd, &rect);
				if (lastWidth != rect.right || lastHeight != rect.bottom) {
					lastWidth = rect.right;
					lastHeight = rect.bottom;
					engine = new EngineImpl;
					if (engine.isNull()) {
						return;
					}
					engine->m_renderer = this;
					if (!(resize())) {
						return;
					}
				}
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

			engine->m_renderer = sl_null;
		}
	}

	namespace priv
	{
		namespace D3D_NAMESPACE
		{

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