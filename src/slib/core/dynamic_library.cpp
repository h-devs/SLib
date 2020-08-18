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

#include "slib/core/dynamic_library.h"

namespace slib
{

	DynamicLibrary::DynamicLibrary() : m_library(sl_null)
	{
	}

	DynamicLibrary::DynamicLibrary(const StringParam& path) : m_library(sl_null)
	{
		load(path);
	}

	DynamicLibrary::DynamicLibrary(const StringParam& path1, const StringParam& path2) : m_library(sl_null)
	{
		load(path1, path2);
	}

	DynamicLibrary::~DynamicLibrary()
	{
		free();
	}

	sl_bool DynamicLibrary::isLoaded()
	{
		return m_library != sl_null;
	}

	sl_bool DynamicLibrary::load(const StringParam& path)
	{
		free();
		void* library = loadLibrary(path);
		m_library = library;
		return library != sl_null;
	}

	sl_bool DynamicLibrary::load(const StringParam& path1, const StringParam& path2)
	{
		if (load(path1)) {
			return sl_true;
		}
		return load(path2);
	}

	void DynamicLibrary::free()
	{
		void* library = m_library;
		if (library) {
			freeLibrary(library);
			m_library = sl_null;
		}
	}

	void* DynamicLibrary::getFunctionAddress(const StringParam& _name)
	{
		void* library = m_library;
		if (library) {
			StringCstr name(_name);
			return getFunctionAddress(library, name.getData());
		} else {
			return sl_null;
		}		
	}

	void* DynamicLibrary::loadLibrary(const StringParam& path1, const StringParam& path2)
	{
		void* lib = loadLibrary(path1);
		if (lib) {
			return lib;
		}
		return loadLibrary(path2);
	}

}
