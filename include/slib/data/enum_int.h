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

#ifndef CHECKHEADER_SLIB_DATA_ENUM_INT
#define CHECKHEADER_SLIB_DATA_ENUM_INT

#include "definition.h"

#include "serialize/primitive.h"

namespace slib
{

	template <class ENUM, class INT>
	class EnumInt
	{
	public:
		ENUM value;

	public:
		EnumInt() noexcept {}

		SLIB_CONSTEXPR EnumInt(ENUM _value): value(_value) {}

		SLIB_CONSTEXPR EnumInt(INT _value) : value((ENUM)_value) {}

		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(EnumInt)

	public:
		EnumInt & operator=(ENUM _value)
		{
			value = _value;
			return *this;
		}

		EnumInt & operator=(INT _value)
		{
			value = (ENUM)_value;
			return *this;
		}

		SLIB_CONSTEXPR operator ENUM() const
		{
			return (ENUM)value;
		}

		SLIB_CONSTEXPR operator INT() const
		{
			return (INT)value;
		}

	};

}

#endif