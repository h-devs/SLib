/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_DL
#define CHECKHEADER_SLIB_DL

#ifdef SLIB_IMPLEMENT_DYNAMIC_LIBRARY

#include "../system/dynamic_library.h"

#define SLIB_IMPORT_LIBRARY_BEGIN(NAME, ...) \
	namespace NAME { \
		void* getLibrary() { \
			static void* library = sl_null; \
			static sl_bool flagLoaded = sl_false; \
			if (flagLoaded) { \
				return library; \
			} \
			library = DynamicLibrary::loadLibrary(__VA_ARGS__); \
			flagLoaded = sl_true; \
			return library; \
		} \
		void* getApi(const char* name) { \
			void* library = getLibrary(); \
			if (library) { \
				return DynamicLibrary::getFunctionAddress(library, name); \
			} \
			return sl_null; \
		}

#define SLIB_IMPORT_LIBRARY_FUNCTION(NAME, RET_TYPE, MODIFIER, ...) \
		SLIB_IMPORT_FUNCTION_FROM_LIBRARY(getLibrary(), NAME, RET_TYPE, MODIFIER, ##__VA_ARGS__)

#define SLIB_IMPORT_LIBRARY_WRAP_FUNCTION(NAME, RET_TYPE, MODIFIER, ...) \
		SLIB_IMPORT_LIBRARY_FUNCTION(NAME, RET_TYPE, MODIFIER, ##__VA_ARGS__)

#else

#define SLIB_IMPORT_LIBRARY_BEGIN(NAME, ...) \
	namespace NAME { \
		void* getLibrary(); \
		void* getApi(const char* name);

#define SLIB_IMPORT_LIBRARY_FUNCTION(NAME, RET_TYPE, MODIFIER, ...) \
		typedef RET_TYPE (MODIFIER *DL_FUNC_TYPE_##NAME)(__VA_ARGS__); \
		DL_FUNC_TYPE_##NAME getApi_##NAME();

#define SLIB_IMPORT_LIBRARY_WRAP_FUNCTION(NAME, RET_TYPE, MODIFIER, ...) \
		SLIB_IMPORT_LIBRARY_FUNCTION(NAME, RET_TYPE, MODIFIER, ##__VA_ARGS__) \
		MODIFIER RET_TYPE wrap_##NAME(__VA_ARGS__);

#endif

#define SLIB_IMPORT_LIBRARY_END \
	}

#endif
