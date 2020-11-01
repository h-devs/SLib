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

#ifndef CHECKHEADER_SLIB_RENDER_D3D
#define CHECKHEADER_SLIB_RENDER_D3D

#include "definition.h"

#ifdef SLIB_RENDER_SUPPORT_D3D

#include "engine.h"

struct IDirect3DDevice9;
struct ID3D10Device;
struct ID3D10Device1;
struct ID3D11Device;

namespace slib
{

	class SLIB_EXPORT Direct3D
	{
	public:
		static Ref<Renderer> createRenderer(RenderEngineType version, void* windowHandle, const RendererParam& param);
		
		static Ref<Renderer> createRenderer(IDirect3DDevice9* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure = sl_true);

		static Ref<Renderer> createRenderer(ID3D10Device* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure = sl_true);

		static Ref<Renderer> createRenderer(ID3D10Device1* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure = sl_true);

		static Ref<Renderer> createRenderer(ID3D11Device* device, const RendererParam& param, void* windowHandle, sl_bool flagFreeOnFailure = sl_true);

	};

}

#endif

#endif
