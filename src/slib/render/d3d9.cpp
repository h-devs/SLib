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

#include "d3d/d3d9.h"
#include "d3d/d3dx9shader.h"

#define D3D_NAMESPACE d3d9
#define D3D_TYPE D3D9
#define D3D_VERSION_MAJOR 9
#define D3D_VERSION_MINOR 0
typedef IDirect3DDevice9 ID3DDevice;
typedef IDirect3DDevice9 ID3DDeviceContext;
typedef IDirect3DTexture9 ID3DTexture2D;
typedef IDirect3DVertexBuffer9 ID3DVertexBuffer;
typedef IDirect3DIndexBuffer9 ID3DIndexBuffer;
typedef IDirect3DVertexShader9 ID3DVertexShader;
typedef IDirect3DPixelShader9 ID3DPixelShader;
typedef IDirect3DVertexDeclaration9 ID3DInputLayout;

typedef D3DVIEWPORT9 D3DVIEWPORT;

#include "d3d_impl.h"

namespace slib
{
	namespace priv
	{
		namespace d3d9
		{

			Memory CompileShader(const StringParam& _source, const StringParam& _target)
			{
				if (_source.isEmpty()) {
					return sl_null;
				}
				auto func = slib::d3dx9::getApi_D3DXCompileShader();
				if (!func) {
					return sl_null;
				}
				ID3DXBuffer* shader = sl_null;
				StringData source(_source);
				StringCstr target(_target);
#ifdef SLIB_DEBUG
				ID3DXBuffer* error = sl_null;
				HRESULT hr = func(source.getData(), (UINT)(source.getLength()), NULL, NULL, "main", target.getData(), 0, &shader, &error, NULL);
#else
				func(source.getData(), (UINT)(source.getLength()), NULL, NULL, "main", target.getData(), 0, &shader, NULL, NULL);
#endif
				if (shader) {
					Memory ret = Memory::create(shader->GetBufferPointer(), (sl_size)(shader->GetBufferSize()));
					shader->Release();
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
