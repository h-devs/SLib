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

#include "d3d/d3d10.h"

#define D3D_(X) D3D10_##X
#define D3D_VERSION_MAJOR 10
#define D3D_VERSION_MINOR 0

#define D3D_NAMESPACE d3d10
#define D3D_TYPE D3D10
typedef ID3D10Device ID3DDevice;
typedef ID3D10Device ID3DDeviceContext;

typedef ID3D10RenderTargetView ID3DRenderTargetView;
typedef ID3D10DepthStencilView ID3DDepthStencilView;
typedef ID3D10Texture2D ID3DTexture2D;
typedef ID3D10ShaderResourceView ID3DShaderResourceView;
typedef ID3D10SamplerState ID3DSamplerState;
typedef ID3D10Buffer ID3DBuffer;
typedef ID3D10Buffer ID3DIndexBuffer;
typedef ID3D10Buffer ID3DVertexBuffer;
typedef ID3D10VertexShader ID3DVertexShader;
typedef ID3D10PixelShader ID3DPixelShader;
typedef ID3D10InputLayout ID3DInputLayout;
typedef ID3D10DepthStencilState ID3DDepthStencilState;
typedef ID3D10RasterizerState ID3DRasterizerState;
typedef ID3D10BlendState ID3DBlendState;

#include "d3d_impl.h"

namespace slib
{
	namespace priv
	{
		namespace d3d10
		{

			Memory CompileShader(const StringParam& _source, const StringParam& _target)
			{
				if (_source.isEmpty()) {
					return sl_null;
				}
				ID3D10Blob* blob = sl_null;
#ifdef SLIB_DEBUG
				ID3D10Blob* error = sl_null;
				HRESULT hr;
#endif
				auto func1 = slib::d3d_compiler::getApi_D3DCompile();
				if (func1) {
					StringData source(_source);
					StringCstr target(_target);
#ifdef SLIB_DEBUG
					hr = func1(source.getData(), (SIZE_T)(source.getLength()), NULL, NULL, NULL, "main", target.getData(), D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, &blob, &error);
#else
					func1(source.getData(), (SIZE_T)(source.getLength()), NULL, NULL, NULL, "main", target.getData(), D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, &blob, NULL);
#endif
				} else {
					auto func2 = slib::d3dx10::getApi_D3DX10CompileFromMemory();
					if (!func2) {
						return sl_null;
					}
					StringData source(_source);
					StringCstr target(_target);
#ifdef SLIB_DEBUG
					hr = func2(source.getData(), (SIZE_T)(source.getLength()), NULL, NULL, NULL, "main", target.getData(), D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, NULL, &blob, &error, NULL);
#else
					func2(source.getData(), (SIZE_T)(source.getLength()), NULL, NULL, NULL, "main", target.getData(), D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, NULL, &blob, NULL, NULL);
#endif
				}
				if (blob) {
					Memory ret = Memory::create(blob->GetBufferPointer(), (sl_size)(blob->GetBufferSize()));
					blob->Release();
					return ret;
				}
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

		}
	}
}

#endif
