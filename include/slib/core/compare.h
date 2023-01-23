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

#ifndef CHECKHEADER_SLIB_CORE_COMPARE
#define CHECKHEADER_SLIB_CORE_COMPARE

#include "definition.h"

/*
* 0: They compare equal.
* <0: Either the value of the first character that does not match is lower in the compared string, or all compared characters match but the compared string is shorter.
* >0: Either the value of the first character that does not match is greater in the compared string, or all compared characters match but the compared string is longer.
*/
typedef sl_int32 sl_compare_result;

namespace slib
{

	template <class T1, class T2>
	constexpr sl_compare_result ComparePrimitiveValues(T1 a, T2 b)
	{
		return (a < b) ? (sl_compare_result)-1 : ((sl_compare_result)(a > b));
	}

	template <class T, sl_bool isClass = __is_class(T)>
	class DefaultComparator {};

	template <class T>
	class DefaultComparator<T, sl_true>
	{
	public:
		template <class T2>
		static sl_compare_result compare(const T& a, const T2& b) noexcept
		{
			return a.compare(b);
		}
	};

	template <class T>
	class DefaultComparator<T, sl_false>
	{
	public:
		template <class T2>
		static sl_compare_result compare(const T& a, const T2& b) noexcept
		{
			return (a < b) ? (sl_compare_result)-1 : ((sl_compare_result)(a > b));
		}
	};

	template <class T1, class T2 = T1>
	class Compare
	{
	public:
		sl_compare_result operator()(const T1& a, const T2& b) const noexcept
		{
			return DefaultComparator<T1>::compare(a, b);
		}
	};

	template <class T1, class T2 = T1>
	class Compare_Descending
	{
	public:
		sl_compare_result operator()(const T1& a, const T2& b) const noexcept
		{
			return -(Compare<T1, T2>()(a, b));
		}
	};

	template <class T1, class T2 = T1>
	class Compare_IgnoreCase
	{
	public:
		sl_compare_result operator()(const T1& a, const T2& b) const noexcept
		{
			return a.compare_IgnoreCase(b);
		}
	};

	template <class T1, class T2 = T1>
	class Equals
	{
	public:
		sl_bool operator()(const T1& a, const T2& b) const noexcept
		{
			return a == b;
		}
	};

	template <class T1, class T2 = T1>
	class Equals_IgnoreCase
	{
	public:
		sl_bool operator()(const T1& a, const T2& b) const noexcept
		{
			return a.equals_IgnoreCase(b);
		}
	};

}

#define PRIV_SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS(PREFIX, SUFFIX) \
	template <class OTHER> PREFIX sl_bool operator==(const OTHER& other) const SUFFIX { return equals(other); } \
	template <class OTHER> PREFIX sl_bool operator!=(const OTHER& other) const SUFFIX { return !(equals(other)); } \
	template <class OTHER> PREFIX sl_bool operator>(const OTHER& other) const SUFFIX { return compare(other) > 0; } \
	template <class OTHER> PREFIX sl_bool operator>=(const OTHER& other) const SUFFIX { return compare(other) >= 0; } \
	template <class OTHER> PREFIX sl_bool operator<(const OTHER& other) const SUFFIX { return compare(other) < 0; } \
	template <class OTHER> PREFIX sl_bool operator<=(const OTHER& other) const SUFFIX { return compare(other) <= 0; }

#define SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS PRIV_SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS(,noexcept)
#define SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS_CONSTEXPR PRIV_SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS(SLIB_CONSTEXPR,)

#endif
