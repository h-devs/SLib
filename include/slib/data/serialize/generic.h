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

#ifndef CHECKHEADER_SLIB_DATA_SERIALIZE_GENERIC
#define CHECKHEADER_SLIB_DATA_SERIALIZE_GENERIC

#include "../definition.h"

namespace slib
{

	namespace priv
	{
		namespace serialize_helper
		{

			template <class T, sl_bool isClass = __is_class(T), sl_bool isEnum = __is_enum(T)>
			class DoSerializeHelper
			{
			};

			template <class T>
			class DoSerializeHelper<T, sl_true, sl_false>
			{
			public:
				template <class OUTPUT>
				static sl_bool serialize(OUTPUT* output, const T& _in)
				{
					return _in.serialize(output);
				}

				template <class INPUT>
				static sl_bool deserialize(INPUT* input, T& _out)
				{
					return _out.deserialize(input);
				}

			};

			template <class T>
			class DoSerializeHelper<T, sl_false, sl_true>
			{
			public:
				template <class OUTPUT>
				static sl_bool serialize(OUTPUT* output, const T& _in)
				{
					return CVLI::serialize(output, (sl_int64)_in);
				}

				template <class INPUT>
				static sl_bool deserialize(INPUT* input, T& _out)
				{
					sl_uint64 v;
					if (CVLI::deserialize(input, v)) {
						_out = (T)v;
						return sl_true;
					}
					return sl_false;
				}

			};

		}
	}

	template <class OUTPUT, class T>
	static sl_bool Serialize(OUTPUT* output, const T& _in)
	{
		return priv::serialize_helper::DoSerializeHelper<T>::serialize(output, _in);
	}

	template <class INPUT, class T>
	static sl_bool Deserialize(INPUT* input, T& _out)
	{
		return priv::serialize_helper::DoSerializeHelper<T>::deserialize(input, _out);
	}

}

#endif
