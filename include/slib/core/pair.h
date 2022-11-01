/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_PAIR
#define CHECKHEADER_SLIB_CORE_PAIR

#include "default_members.h"
#include "compare.h"
#include "hash.h"
#include "cpp_helper.h"

namespace slib
{

	template <class FIRST_T, class SECOND_T>
	class SLIB_EXPORT Pair
	{
	public:
		FIRST_T first;
		SECOND_T second;

	public:
		Pair() noexcept {}

		template <class FIRST, class SECOND>
		Pair(FIRST&& _first, SECOND&& _second) noexcept: first(Forward<FIRST>(_first)), second(Forward<SECOND>(_second)) {}

		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(Pair)

	public:
		sl_compare_result compare(const Pair& other) const
		{
			sl_compare_result ret = Compare<FIRST_T>()(first, other.first);
			if (ret) {
				return ret;
			}
			return Compare<SECOND_T>()(second, other.second);
		}

		sl_bool equals(const Pair& other) const
		{
			return Equals<FIRST_T>()(first, other.first) && Equals<SECOND_T>()(second, other.second);
		}

		sl_size getHashCode() const
		{
#ifdef SLIB_ARCH_IS_64BIT
			return SLIB_MAKE_QWORD4(Rehash64To32(Hash<FIRST_T>()(first)), Rehash64To32(Hash<SECOND_T>()(second)));
#else
			return Rehash64ToSize(SLIB_MAKE_QWORD4(Hash<FIRST_T>()(first), Hash<SECOND_T>()(second)));
#endif
		}

	public:
		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS

	};

}

#endif
