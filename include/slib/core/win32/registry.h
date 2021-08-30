/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_WIN32_REGISTRY
#define CHECKHEADER_SLIB_CORE_WIN32_REGISTRY

#include "../definition.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "windows.h"

#include "../handle_container.h"
#include "../string.h"
#include "../hash_map.h"
#include "../variant_def.h"

namespace slib
{

	namespace win32
	{

		class SLIB_EXPORT Registry
		{
		public:
			SLIB_DECLARE_HANDLE_CONTAINER_MEMBERS(Registry, HKEY, handle, NULL)

		public:
			static Registry open(HKEY hKeyParent, const StringParam& subPath, REGSAM sam = KEY_ALL_ACCESS, sl_bool flagCreate = sl_false);
			
			static Registry create(HKEY hKeyParent, const StringParam& subPath, REGSAM sam = KEY_ALL_ACCESS);

		public:
			VariantMap getValues();
			
			static VariantMap getValues(HKEY hKeyParent, const StringParam& subPath);

			static VariantMap getValues(const StringParam& path);

			sl_bool getValue(const StringParam& name, Variant* out);

			static sl_bool getValue(HKEY hKeyParent, const StringParam& subPath, const StringParam& name, Variant* out);

			static sl_bool getValue(const StringParam& path, const StringParam& name, Variant* out);

			sl_size setValues(const VariantMap& values);

			static sl_size setValues(HKEY hKeyParent, const StringParam& subPath, const VariantMap& values);

			static sl_size setValues(const StringParam& path, const VariantMap& values);

			sl_bool setValue(const StringParam& name, const Variant& value);

			static sl_bool setValue(HKEY hKeyParent, const StringParam& subPath, const StringParam& name, const Variant& value);

			static sl_bool setValue(const StringParam& path, const StringParam& name, const Variant& value);

		};

	}
	
}

#endif

#endif