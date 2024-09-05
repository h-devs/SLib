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

#ifndef CHECKHEADER_SLIB_SYSTEM_DYNAMIC_LIBRARY
#define CHECKHEADER_SLIB_SYSTEM_DYNAMIC_LIBRARY

#include "definition.h"

#include "../core/string.h"
#include "../core/cast.h"

namespace slib
{

	class SLIB_EXPORT DynamicLibrary
	{
	public:
		DynamicLibrary();

		DynamicLibrary(const StringParam& path);

		DynamicLibrary(const StringParam* libs, sl_size nLibs);

		template <class... ARGS>
		DynamicLibrary(const StringParam& path1, const StringParam& path2, ARGS&&... args): m_library(loadLibrary(path1, path2, Forward<ARGS>(args)...)) {}

		~DynamicLibrary();

	public:
		sl_bool isLoaded();

		sl_bool load(const StringParam& path);

		sl_bool load(const StringParam* libs, sl_size nLibs);

		template <class... ARGS>
		sl_bool load(const StringParam& path1, const StringParam& path2, ARGS&&... args)
		{
			StringParam params[] = { path1, path2, Forward<ARGS>(args)... };
			return load(params, 2 + sizeof...(args));
		}

		void free();

		void* getFunctionAddress(const StringParam& name);

	public:
		static void* loadLibrary(const StringParam& path);

		static void* loadLibrary(const StringParam* libs, sl_size nLibs);

		template <class... ARGS>
		static void* loadLibrary(const StringParam& path1, const StringParam& path2, ARGS&&... args)
		{
			StringParam params[] = { path1, path2, Forward<ARGS>(args)... };
			return loadLibrary(params, 2 + sizeof...(args));
		}

		static void freeLibrary(void* library);

		static void* getFunctionAddress(void* library, const char* name);

	private:
		void* m_library;

	};

}

#define SLIB_DEFINE_GET_LIBRARY(NAME, ...) \
	void* NAME() \
	{ \
		static void* _lib = sl_null; \
		if (!_lib) { \
			_lib = DynamicLibrary::loadLibrary(__VA_ARGS__); \
		} \
		return _lib; \
	}

#define SLIB_DEFINE_GET_FUNCTION_ADDRESS(RET, NAME, LIB, FUNC_NAME) \
	RET NAME() \
	{ \
		static RET _func = sl_null; \
		if (!_func) { \
			void* _lib = LIB; \
			if (_lib) { \
				_func = ForcedCast<RET>(DynamicLibrary::getFunctionAddress(_lib, FUNC_NAME)); \
			} \
		} \
		return _func; \
	}

#endif
