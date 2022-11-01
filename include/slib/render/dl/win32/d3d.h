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

#ifndef CHECKHEADER_SLIB_RENDER_DL_WIN32_D3D
#define CHECKHEADER_SLIB_RENDER_DL_WIN32_D3D

#include "../../../core/definition.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "../../../core/dl.h"

#include "../../../core/win32/windows.h"

struct IDirect3D8;
struct IDirect3D9;
struct ID3D10Device;
struct ID3D10Device1;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGIAdapter;

struct _D3DXMACRO;
struct ID3DXInclude;
struct ID3DXBuffer;
struct ID3DXConstantTable;

struct _D3D_SHADER_MACRO;
struct ID3DInclude;
struct ID3DX10ThreadPump;
struct ID3D10Blob;

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

	SLIB_IMPORT_LIBRARY_BEGIN(d3d8, "d3d8.dll")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			Direct3DCreate8,
			IDirect3D8*, WINAPI,
			UINT SDKVersion
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

	SLIB_IMPORT_LIBRARY_BEGIN(d3dx8, "d3dx8d.dll")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			D3DXAssembleShader,
			HRESULT, WINAPI,
			LPCVOID pSrcData,
			UINT SrcDataLen,
			DWORD Flags,
			ID3DXBuffer** ppConstants,
			ID3DXBuffer** ppCompiledShader,
			ID3DXBuffer** ppCompilationErrors
		)

	SLIB_IMPORT_LIBRARY_END

	namespace d3dx9
	{

		void* getLibrary();
		void* getApi(const char* name);

		SLIB_IMPORT_LIBRARY_FUNCTION(
			D3DXCompileShader,
			HRESULT, WINAPI,
			LPCSTR pSrcData,
			UINT srcDataLen,
			const _D3DXMACRO *pDefines,
			ID3DXInclude* pInclude,
			LPCSTR pFunctionName,
			LPCSTR pProfile,
			DWORD Flags,
			ID3DXBuffer **ppShader,
			ID3DXBuffer **ppErrorMsgs,
			ID3DXConstantTable **ppConstantTable
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			D3DXAssembleShader,
			HRESULT, WINAPI,
			LPCSTR pSrcData,
			UINT SrcDataLen,
			const _D3DXMACRO *pDefines,
			ID3DXInclude *pInclude,
			DWORD Flags,
			ID3DXBuffer **ppShader,
			ID3DXBuffer **ppErrorMsgs
		)

	}

	namespace d3dx10
	{

		void* getLibrary();
		void* getApi(const char* name);

		SLIB_IMPORT_LIBRARY_FUNCTION(
			D3DX10CompileFromMemory,
			HRESULT, WINAPI,
			LPCSTR pSrcData,
			SIZE_T SrcDataLen,
			LPCSTR pFileName,
			const _D3D_SHADER_MACRO *pDefines,
			ID3DInclude* pInclude,
			LPCSTR pFunctionName,
			LPCSTR pProfile,
			UINT Flags1,
			UINT Flags2,
			ID3DX10ThreadPump *pPump,
			ID3D10Blob **ppShader,
			ID3D10Blob **ppErrorMsgs,
			HRESULT *pHResult
		)

	}

	namespace d3dx11
	{

		void* getLibrary();
		void* getApi(const char* name);

	}

	namespace d3d_compiler
	{

		void* getLibrary();
		void* getApi(const char* name);

		SLIB_IMPORT_LIBRARY_FUNCTION(
			D3DCompile,
			HRESULT, WINAPI,
			LPCVOID pSrcData,
			SIZE_T SrcDataSize,
			LPCSTR pSourceName,
			const _D3D_SHADER_MACRO *pDefines,
			ID3DInclude* pInclude,
			LPCSTR pEntrypoint,
			LPCSTR pTarget,
			UINT Flags1,
			UINT Flags2,
			ID3D10Blob **ppCode,
			ID3D10Blob **ppErrorMsgs
		)

	}

}

#endif

#endif
