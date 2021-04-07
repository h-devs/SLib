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

#include "slib/render/definition.h"

#if defined(SLIB_RENDER_SUPPORT_D3D)

#include "slib/render/d3d.h"
#include "slib/render/dl_windows_d3d.h"

namespace slib
{

	namespace priv
	{

		namespace d3d8
		{
			Ref<Renderer> CreateRenderer(void* windowHandle, const RendererParam& param);
			Ref<Renderer> CreateRenderer(IDirect3DDevice8* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure);
		}

		namespace d3d9
		{
			Ref<Renderer> CreateRenderer(void* windowHandle, const RendererParam& param);
			Ref<Renderer> CreateRenderer(IDirect3DDevice9* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure);
			Memory CompileShader(const StringParam& source, const StringParam& target);
		}

		namespace d3d10
		{
			Ref<Renderer> CreateRenderer(void* windowHandle, const RendererParam& param);
			Ref<Renderer> CreateRenderer(ID3D10Device* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure);
			Memory CompileShader(const StringParam& source, const StringParam& target);
		}

		namespace d3d10_1
		{
			Ref<Renderer> CreateRenderer(void* windowHandle, const RendererParam& param);
			Ref<Renderer> CreateRenderer(ID3D10Device1* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure);
		}

		namespace d3d11
		{
			Ref<Renderer> CreateRenderer(void* windowHandle, const RendererParam& param);
			Ref<Renderer> CreateRenderer(ID3D11Device* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure);
		}

	}

	Ref<Renderer> Direct3D::createRenderer(RenderEngineType version, void* windowHandle, const RendererParam& param)
	{
		if (!windowHandle) {
			return sl_null;
		}
		if (version == RenderEngineType::D3D) {
			Ref<Renderer> renderer = priv::d3d11::CreateRenderer(windowHandle, param);
			if (renderer.isNotNull()) {
				return renderer;
			}
			renderer = priv::d3d10_1::CreateRenderer(windowHandle, param);
			if (renderer.isNotNull()) {
				return renderer;
			}
			renderer = priv::d3d10::CreateRenderer(windowHandle, param);
			if (renderer.isNotNull()) {
				return renderer;
			}
			return priv::d3d9::CreateRenderer(windowHandle, param);
		} else {
			if (version >= RenderEngineType::D3D11) {
				return priv::d3d11::CreateRenderer(windowHandle, param);
			}
			if (version >= RenderEngineType::D3D10_1) {
				return priv::d3d10_1::CreateRenderer(windowHandle, param);
			}
			if (version >= RenderEngineType::D3D10) {
				return priv::d3d10::CreateRenderer(windowHandle, param);
			}
			if (version >= RenderEngineType::D3D9) {
				return priv::d3d9::CreateRenderer(windowHandle, param);
			}
			return priv::d3d8::CreateRenderer(windowHandle, param);
		}
	}

	Ref<Renderer> Direct3D::createRenderer(IDirect3DDevice8* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure)
	{
		return priv::d3d8::CreateRenderer(device, param, windowHandle, flagFreeOnFailure);
	}

	Ref<Renderer> Direct3D::createRenderer(IDirect3DDevice9* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure)
	{
		return priv::d3d9::CreateRenderer(device, param, windowHandle, flagFreeOnFailure);
	}

	Ref<Renderer> Direct3D::createRenderer(ID3D10Device* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure)
	{
		return priv::d3d10::CreateRenderer(device, param, windowHandle, flagFreeOnFailure);
	}

	Ref<Renderer> Direct3D::createRenderer(ID3D10Device1* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure)
	{
		return priv::d3d10_1::CreateRenderer(device, param, windowHandle, flagFreeOnFailure);
	}

	Ref<Renderer> Direct3D::createRenderer(ID3D11Device* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure)
	{
		return priv::d3d11::CreateRenderer(device, param, windowHandle, flagFreeOnFailure);
	}

	Memory Direct3D::compileShader(const StringParam& source, const StringParam& target)
	{
		if (slib::d3d_compiler::getApi_D3DCompile() || slib::d3dx10::getApi_D3DX10CompileFromMemory()) {
			return priv::d3d10::CompileShader(source, target);
		} else {
			return priv::d3d9::CompileShader(source, target);
		}
	}

}

#else

namespace slib
{

	Ref<Renderer> Direct3D::createRenderer(RenderEngineType version, void* windowHandle, const RendererParam& param)
	{
		return sl_null;
	}

	Memory Direct3D::compileShader(const StringParam& source, const StringParam& target)
	{
		return sl_null;
	}

	Memory Direct3D::assembleShader(const StringParam& source)
	{
		return sl_null;
	}

}

#endif
