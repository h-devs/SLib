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

#ifndef CHECKHEADER_SLIB_CORE_STRING_TRAITS
#define CHECKHEADER_SLIB_CORE_STRING_TRAITS

#include "base.h"

namespace slib
{

	template <class CHAR_TYPE>
	class StringTraits;

	template <>
	class StringTraits<sl_char8>
	{
	public:
		static sl_size getLength(const sl_char8* sz) noexcept
		{
			return Base::getStringLength(sz);
		}

		static sl_size copy(sl_char8* dst, const sl_char8* src) noexcept
		{
			return Base::copyString(dst, src);
		}

		static sl_size copy(sl_char8* dst, const sl_char8* src, sl_size count) noexcept
		{
			return Base::copyString(dst, src, count);
		}

		static sl_bool equals(const sl_char8* s1, const sl_char8* s2) noexcept
		{
			return Base::equalsString(s1, s2);
		}

		static sl_bool equals(const sl_char8* s1, const sl_char8* s2, sl_size count) noexcept
		{
			return Base::equalsString(s1, s2, count);
		}

		static sl_compare_result compare(const sl_char8* s1, const sl_char8* s2) noexcept
		{
			return Base::compareString(s1, s2);
		}

		static sl_compare_result compare(const sl_char8* s1, const sl_char8* s2, sl_size count) noexcept
		{
			return Base::compareString(s1, s2, count);
		}

		static sl_bool equals_IgnoreCase(const sl_char8* s1, const sl_char8* s2) noexcept
		{
			return Base::equalsString_IgnoreCase(s1, s2);
		}

		static sl_bool equals_IgnoreCase(const sl_char8* s1, const sl_char8* s2, sl_size count) noexcept
		{
			return Base::equalsString_IgnoreCase(s1, s2, count);
		}

		static sl_compare_result compare_IgnoreCase(const sl_char8* s1, const sl_char8* s2) noexcept
		{
			return Base::compareString_IgnoreCase(s1, s2);
		}

		static sl_compare_result compare_IgnoreCase(const sl_char8* s1, const sl_char8* s2, sl_size count) noexcept
		{
			return Base::compareString_IgnoreCase(s1, s2, count);
		}

	};

	template <>
	class StringTraits<sl_char16>
	{
	public:
		static sl_size getLength(const sl_char16* sz) noexcept
		{
			return Base::getStringLength2(sz);
		}

		static sl_size copy(sl_char16* dst, const sl_char16* src) noexcept
		{
			return Base::copyString2(dst, src);
		}

		static sl_size copy(sl_char16* dst, const sl_char16* src, sl_size count) noexcept
		{
			return Base::copyString2(dst, src, count);
		}

		static sl_bool equals(const sl_char16* s1, const sl_char16* s2) noexcept
		{
			return Base::equalsString2(s1, s2);
		}

		static sl_bool equals(const sl_char16* s1, const sl_char16* s2, sl_size count) noexcept
		{
			return Base::equalsString2(s1, s2, count);
		}

		static sl_compare_result compare(const sl_char16* s1, const sl_char16* s2) noexcept
		{
			return Base::compareString2(s1, s2);
		}

		static sl_compare_result compare(const sl_char16* s1, const sl_char16* s2, sl_size count) noexcept
		{
			return Base::compareString2(s1, s2, count);
		}

		static sl_bool equals_IgnoreCase(const sl_char16* s1, const sl_char16* s2) noexcept
		{
			return Base::equalsString2_IgnoreCase(s1, s2);
		}

		static sl_bool equals_IgnoreCase(const sl_char16* s1, const sl_char16* s2, sl_size count) noexcept
		{
			return Base::equalsString2_IgnoreCase(s1, s2, count);
		}

		static sl_compare_result compare_IgnoreCase(const sl_char16* s1, const sl_char16* s2) noexcept
		{
			return Base::compareString2_IgnoreCase(s1, s2);
		}

		static sl_compare_result compare_IgnoreCase(const sl_char16* s1, const sl_char16* s2, sl_size count) noexcept
		{
			return Base::compareString2_IgnoreCase(s1, s2, count);
		}

	};

	template <>
	class StringTraits<sl_char32>
	{
	public:
		static sl_size getLength(const sl_char32* sz) noexcept
		{
			return Base::getStringLength4(sz);
		}

		static sl_size copy(sl_char32* dst, const sl_char32* src) noexcept
		{
			return Base::copyString4(dst, src);
		}

		static sl_size copy(sl_char32* dst, const sl_char32* src, sl_size count) noexcept
		{
			return Base::copyString4(dst, src, count);
		}

		static sl_bool equals(const sl_char32* s1, const sl_char32* s2) noexcept
		{
			return Base::equalsString4(s1, s2);
		}

		static sl_bool equals(const sl_char32* s1, const sl_char32* s2, sl_size count) noexcept
		{
			return Base::equalsString4(s1, s2, count);
		}

		static sl_compare_result compare(const sl_char32* s1, const sl_char32* s2) noexcept
		{
			return Base::compareString4(s1, s2);
		}

		static sl_compare_result compare(const sl_char32* s1, const sl_char32* s2, sl_size count) noexcept
		{
			return Base::compareString4(s1, s2, count);
		}

		static sl_bool equals_IgnoreCase(const sl_char32* s1, const sl_char32* s2) noexcept
		{
			return Base::equalsString4_IgnoreCase(s1, s2);
		}

		static sl_bool equals_IgnoreCase(const sl_char32* s1, const sl_char32* s2, sl_size count) noexcept
		{
			return Base::equalsString4_IgnoreCase(s1, s2, count);
		}

		static sl_compare_result compare_IgnoreCase(const sl_char32* s1, const sl_char32* s2) noexcept
		{
			return Base::compareString4_IgnoreCase(s1, s2);
		}

		static sl_compare_result compare_IgnoreCase(const sl_char32* s1, const sl_char32* s2, sl_size count) noexcept
		{
			return Base::compareString4_IgnoreCase(s1, s2, count);
		}

	};

}

#endif

