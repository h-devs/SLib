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

#ifndef CHECKHEADER_SLIB_PLATFORM_WIN32_WMI
#define CHECKHEADER_SLIB_PLATFORM_WIN32_WMI

#include "../definition.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "../../core/variant.h"

namespace slib 
{
	namespace win32 
	{
		
		class SLIB_EXPORT Wmi
		{
		public:
			static Variant getQueryResponseValue(const StringParam& query, const StringParam& fieldName);

			static VariantMap getQueryResponseRecord(const StringParam& query, const StringParam* fieldNames, sl_size nFields);

			template <class... ARGS>
			static VariantMap getQueryResponseRecord(const StringParam& query, const StringParam& fieldName, ARGS&&... args)
			{
				StringParam params[] = { fieldName, Forward<ARGS>(args)... };
				return getQueryResponseRecord(query, params, 1 + sizeof...(args));
			}

			static List<VariantMap> getQueryResponseRecords(const StringParam& query, const StringParam* fieldNames, sl_size nFields);

			template <class... ARGS>
			static List<VariantMap> getQueryResponseRecords(const StringParam& query, const StringParam& fieldName, ARGS&&... args)
			{
				StringParam params[] = { fieldName, Forward<ARGS>(args)... };
				return getQueryResponseRecords(query, params, 1 + sizeof...(args));
			}

			static Time getDateTime(const Variant& value);

		};

	}

}

#endif

#endif