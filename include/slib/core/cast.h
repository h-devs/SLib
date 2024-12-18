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

#ifndef CHECKHEADER_SLIB_CORE_CAST
#define CHECKHEADER_SLIB_CORE_CAST

#include "cpp_helper.h"
#include "macro_arg.h"

#if defined(SLIB_COMPILER_IS_VC)
#	define SLIB_HAS_FEATURE_IS_CONVERTIBLE_TO
#elif defined(SLIB_COMPILER_IS_GCC)
#	if defined(__has_feature)
#		if __has_feature(is_convertible_to)
#			define SLIB_HAS_FEATURE_IS_CONVERTIBLE_TO
#		endif
#	endif
#endif

#if defined(SLIB_HAS_FEATURE_IS_CONVERTIBLE_TO)
#	define SLIB_IS_CONVERTIBLE(FROM, TO) __is_convertible_to(FROM, TO)
#else
#	define SLIB_IS_CONVERTIBLE(FROM, TO) slib::IsConvertible<FROM, TO>::value
#endif

#define SLIB_TRY_CONVERT_TYPE(FROM, TO) { static_assert(SLIB_IS_CONVERTIBLE(FROM, TO), "Cannot convert from '" #FROM "' to '" #TO "'"); }

#define SLIB_DEFINE_CAST_REF_FUNCTIONS(TEMPLATE, RET, ARG) \
	template <TEMPLATE> \
	static const RET& cast(const ARG& other) noexcept \
	{ \
		return *(reinterpret_cast<RET const*>(&other)); \
	} \
	template <TEMPLATE> \
	static RET& cast(ARG& other) noexcept \
	{ \
		return *(reinterpret_cast<RET*>(&other)); \
	} \
	template <TEMPLATE> \
	static RET&& cast(ARG&& other) noexcept \
	{ \
		return static_cast<RET&&>(*(reinterpret_cast<RET*>(&other))); \
	}

namespace slib
{

	template <class T, class V>
	SLIB_INLINE T& ForcedCast(const V& v)
	{
		return *((T*)((void*)&v));
	}

	template <class T1, class T2>
	class Cast
	{
	public:
		template <class T>
		T2 operator()(T&& v) const noexcept
		{
			return T2(Forward<T>(v));
		}
	};

	template <class T>
	class Cast<T, T>
	{
	public:
		T const& operator()(const T& v) const noexcept
		{
			return v;
		}
	};

	namespace priv
	{
		template <class T> T&& DeclaredValue() noexcept;
	}

	template <class FROM, class TO>
	class IsConvertibleHelper
	{
	private:
		template<class T> static void _test_implicit(T);

		template<class OTHER_FROM, class OTHER_TO, class OTHER_T = decltype(_test_implicit<OTHER_TO>(priv::DeclaredValue<OTHER_FROM>()))>
		static ConstValue<bool, true> _test(int);

		template <class OTHER_FROM, class OTHER_TO>
		static ConstValue<bool, false> _test(...);

	public:
		typedef decltype(_test<FROM, TO>(0)) type;

	};

	template <typename FROM, typename TO>
	struct IsConvertible : public IsConvertibleHelper<FROM, TO>::type {};

}

#endif
