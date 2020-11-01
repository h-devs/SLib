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

#ifndef CHECKHEADER_SLIB_RENDER_DL_WINDOWS_D3D
#define CHECKHEADER_SLIB_RENDER_DL_WINDOWS_D3D

#include "definition.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "../core/dl.h"

#include <windows.h>

struct IDirect3D9;
struct ID3D10Device;
struct ID3D10Device1;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGIAdapter;

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(dxgi, "dxgi.dll")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			CreateDXGIFactory,
			HRESULT, WINAPI,
			REFIID riid,
			void **ppFactory
		)

	SLIB_IMPORT_LIBRARY_END

	SLIB_IMPORT_LIBRARY_BEGIN(d3d9, "d3d9.dll")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			Direct3DCreate9,
			IDirect3D9*, WINAPI,
			UINT SDKVersion
		) 

	SLIB_IMPORT_LIBRARY_END

	SLIB_IMPORT_LIBRARY_BEGIN(d3d10, "d3d10.dll")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			D3D10CreateDevice,
			HRESULT, WINAPI,
			IDXGIAdapter *pAdapter,
			int DriverType, // D3D10_DRIVER_TYPE
			HMODULE Software,
			UINT Flags,
			UINT SDKVersion,
			ID3D10Device **ppDevice
		)

	SLIB_IMPORT_LIBRARY_END

	SLIB_IMPORT_LIBRARY_BEGIN(d3d10_1, "d3d10_1.dll")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			D3D10CreateDevice1,
			HRESULT, WINAPI,
			IDXGIAdapter *pAdapter,
			int DriverType, // D3D10_DRIVER_TYPE
			HMODULE Software,
			UINT Flags,
			int HardwareLevel, // D3D10_FEATURE_LEVEL1	
			UINT SDKVersion,
			ID3D10Device1 **ppDevice
		)

	SLIB_IMPORT_LIBRARY_END

	SLIB_IMPORT_LIBRARY_BEGIN(d3d11, "d3d11.dll")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			D3D11CreateDevice,
			HRESULT, WINAPI,
			IDXGIAdapter *pAdapter,
			int DriverType, // D3D_DRIVER_TYPE
			HMODULE Software,
			UINT Flags,
			const int *pFeatureLevels, // D3D_FEATURE_LEVEL
			UINT FeatureLevels,
			UINT SDKVersion,
			ID3D11Device **ppDevice,
			int *pFeatureLevel, // D3D_FEATURE_LEVEL
			ID3D11DeviceContext **ppImmediateContext
		)

	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
