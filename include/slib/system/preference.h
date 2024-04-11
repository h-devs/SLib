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

#ifndef CHECKHEADER_SLIB_SYSTEM_PREFERENCE
#define CHECKHEADER_SLIB_SYSTEM_PREFERENCE

#include "definition.h"

#include "../data/json.h"

namespace slib
{

	class SLIB_EXPORT Preference
	{
	public:
		static void setValue(const StringParam& key, const Json& value);
		
		static void removeValue(const StringParam& key);

		static Json getValue(const StringParam& key);
		
		template <class T>
		static void getValue(const StringParam& key, T& _out)
		{
			FromJson(getValue(key), _out);
		}

		template <class T>
		static void getValue(const StringParam& key, T& _out, const T& _def)
		{
			FromJson(getValue(key), _out, _def);
		}


		// used for Win32 applications
		static String getApplicationKeyName();

		static void setApplicationKeyName(const String& name);

	};

}

#endif
