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

#ifndef CHECKHEADER_SLIB_DATA_INTERPOLATION
#define CHECKHEADER_SLIB_DATA_INTERPOLATION

#include "definition.h"

#define SLIB_LERP(a, b, factor) (((a) * (1 - factor)) + ((b) * factor))

namespace slib
{

	template <class TYPE, sl_bool isClass = __is_class(TYPE)>
	class DefaultInterpolation
	{
	};

	template <class TYPE>
	class DefaultInterpolation<TYPE, sl_true>
	{
	public:
		static TYPE interpolate(const TYPE& a, const TYPE& b, float factor)
		{
			return a.lerp(b, factor);
		}
	};

	template <class TYPE>
	class DefaultInterpolation<TYPE, sl_false>
	{
	public:
		static TYPE interpolate(const TYPE& a, const TYPE& b, float factor)
		{
			return (TYPE)(SLIB_LERP(a, b, factor));
		}
	};

	template <class TYPE>
	class SLIB_EXPORT Interpolation
	{
	public:
		static TYPE interpolate(const TYPE& a, const TYPE& b, float factor)
		{
			return DefaultInterpolation<TYPE>::interpolate(a, b, factor);
		}

	};

}

#endif
