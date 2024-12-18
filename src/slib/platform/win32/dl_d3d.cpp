/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/dl/win32/d3d.h"


namespace slib
{

	namespace
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

		static void* GetLibrary(const char* prefix1, const char* prefix2)
		{
			for (sl_uint32 i = 99; i > 0; i--) {
				void* lib = DynamicLibrary::loadLibrary(prefix1 + String::fromUint32(i));
				if (lib) {
					return lib;
				}
				lib = DynamicLibrary::loadLibrary(prefix2 + String::fromUint32(i));
				if (lib) {
					return lib;
				}
			}
			return sl_null;
		}
	}

#define IMPLEMENT_GET_LIBRARY(...) \
	void* getLibrary() \
	{ \
		static void* library = sl_null; \
		static sl_bool flagLoaded = sl_false; \
		if (flagLoaded) { \
			return library; \
		} \
		library = GetLibrary(__VA_ARGS__); \
		flagLoaded = sl_true; \
		return library; \
	} \

#define IMPLEMENT_GET_API \
	void* getApi(const char* name) \
	{ \
		void* library = getLibrary(); \
		if (library) { \
			return DynamicLibrary::getFunctionAddress(library, name); \
		} \
		return sl_null; \
	}

	namespace d3dx9
	{

		IMPLEMENT_GET_LIBRARY("d3dx9_", "d3dx9d_")

		IMPLEMENT_GET_API

	}

	namespace d3dx10
	{

		IMPLEMENT_GET_LIBRARY("d3dx10_", "d3dx10d_")

		IMPLEMENT_GET_API

	}

	namespace d3dx11
	{

		IMPLEMENT_GET_LIBRARY("d3dx11_", "d3dx11d_")

		IMPLEMENT_GET_API

	}

	namespace d3d_compiler
	{

		IMPLEMENT_GET_LIBRARY("d3dcompiler_")

		IMPLEMENT_GET_API

	}

}