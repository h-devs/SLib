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

#ifndef CHECKHEADER_SLIB_CORE_DYNAMIC_LIBRARY
#define CHECKHEADER_SLIB_CORE_DYNAMIC_LIBRARY

#include "definition.h"

#include "string.h"

namespace slib
{
	
	class SLIB_EXPORT DynamicLibrary
	{
	public:
		DynamicLibrary();

		DynamicLibrary(const StringParam& path);

		DynamicLibrary(const StringParam& path1, const StringParam& path2);

		~DynamicLibrary();

	public:
		sl_bool isLoaded();

		sl_bool load(const StringParam& path);

		sl_bool load(const StringParam& path1, const StringParam& path2);

		void free();

		void* getFunctionAddress(const StringParam& name);

	public:
		static void* loadLibrary(const StringParam& path);

		static void* loadLibrary(const StringParam& path1, const StringParam& path2);

		static void freeLibrary(void* library);

		static void* getFunctionAddress(void* library, const char* name);
		
	private:
		void* m_library;

	};

}

#endif
