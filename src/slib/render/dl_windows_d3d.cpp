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

#define SLIB_IMPLEMENT_DYNAMIC_LIBRARY

#include "slib/render/dl_windows_d3d.h"

#define IMPLEMENT_GET_API \
	void* getApi(const char* name) \
	{ \
		void* library = getLibrary(); \
		if (library) { \
			return DynamicLibrary::getFunctionAddress(library, name); \
		} \
		return sl_null; \
	}


namespace slib
{

	namespace priv
	{
		namespace d3d
		{

			static void* GetLibrary(const char* prefix)
			{
				for (sl_uint32 i = 99; i > 0; i--) {
					void* lib = DynamicLibrary::loadLibrary(prefix + String::fromUint32(i));
					if (lib) {
						return lib;
					}
				}
				return sl_null;
			}

		}
	}

	namespace d3dx9
	{

		void* getLibrary()
		{
			return priv::d3d::GetLibrary("d3dx9_");
		}

		IMPLEMENT_GET_API

	}

	namespace d3dx10
	{

		void* getLibrary()
		{
			return priv::d3d::GetLibrary("d3dx10_");
		}

		IMPLEMENT_GET_API

	}

	namespace d3dx11
	{

		void* getLibrary()
		{
			return priv::d3d::GetLibrary("d3dx11_");
		}

		IMPLEMENT_GET_API

	}

	namespace d3d_compiler
	{

		void* getLibrary()
		{
			return priv::d3d::GetLibrary("d3dcompiler_");
		}

		IMPLEMENT_GET_API

	}


}