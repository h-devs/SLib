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

#include "d3d/d3d8.h"
#include "d3d/d3dx8core.h"

#define D3D_NAMESPACE d3d8
#define D3D_TYPE D3D8
#define D3D_VERSION_MAJOR 8
#define D3D_VERSION_MINOR 0
typedef IDirect3DDevice8 ID3DDevice;
typedef IDirect3DDevice8 ID3DDeviceContext;
typedef IDirect3DTexture8 ID3DTexture2D;
typedef IDirect3DVertexBuffer8 ID3DVertexBuffer;
typedef IDirect3DIndexBuffer8 ID3DIndexBuffer;

typedef D3DVIEWPORT8 D3DVIEWPORT;

#include "d3d_impl.h"

#endif
