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

#ifndef CHECKHEADER_SLIB_CORE_JSON_GENERIC
#define CHECKHEADER_SLIB_CORE_JSON_GENERIC

#include "core.h"

namespace slib
{

	namespace priv
	{
		namespace json
		{

			template <class T, sl_bool isClass = __is_class(T), sl_bool isEnum = __is_enum(T)>
			class DoJsonHelper
			{
			};

			template <class T>
			class DoJsonHelper<T, sl_true, sl_false>
			{
			public:
				static void fromJson(const Json& json, T& _out)
				{
					if (json.isUndefined()) {
						return;
					}
					_out.setJson(json);
				}

				static void toJson(Json& json, const T& _in)
				{
					json = _in.toJson();
				}
			};

			template <class T>
			class DoJsonHelper<T, sl_false, sl_true>
			{
			public:
				static void fromJson(const Json& json, T& _out)
				{
					_out = (T)(json.getInt64((sl_int64)_out));
				}

				static void toJson(Json& json, const T& _in)
				{
					json.setInt64((sl_int64)_in);
				}
			};

		}
	}

	template <class T>
	static void FromJson(const Json& json, T& _out)
	{
		priv::json::DoJsonHelper<T>::fromJson(json, _out);
	}

	template <class T>
	static void ToJson(Json& json, const T& _in)
	{
		priv::json::DoJsonHelper<T>::toJson(json, _in);
	}

}

#endif
