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

#ifndef CHECKHEADER_SLIB_CORE_CPP_HELPER
#define CHECKHEADER_SLIB_CORE_CPP_HELPER

#include "definition.h"

namespace slib
{
	
	template <class T> struct RemoveReference { typedef T Type; };
	template <class T> struct RemoveReference<T&> { typedef T Type; };
	template <class T> struct RemoveReference<T&&> { typedef T Type; };

	template <class T> struct RemoveConst { typedef T Type; };
	template <class T> struct RemoveConst<const T> { typedef T Type; };

	template <class T> struct RemoveConstReference { typedef T Type; };
	template <class T> struct RemoveConstReference<T const&> { typedef T Type; };
	template <class T> struct RemoveConstReference<T&> { typedef T Type; };
	template <class T> struct RemoveConstReference<T&&> { typedef T Type; };

	template <class T>
	constexpr T* RemoveConstPointerVariable(const T* t)
	{
		return (T*)t;
	}

	template <class T, T v> struct ConstValue { constexpr static T value = v; };

	template <class T> struct IsLValueHelper : ConstValue<bool, false> {};
	template <class T> struct IsLValueHelper<T&> : ConstValue<bool, true> {};
	template <class T> constexpr bool IsLValue() { return IsLValueHelper<T>::value; }

	template <class T1, class T2> struct IsSameTypeHelper : ConstValue<bool, false> {};
	template <class T> struct IsSameTypeHelper<T, T> : ConstValue<bool, true> {};
	template <class T1, class T2> constexpr bool IsSameType() { return IsSameTypeHelper<T1, T2>::value; }

	template <class T>
	constexpr typename RemoveReference<T>::Type&& Move(T&& v)
	{
		return static_cast<typename RemoveReference<T>::Type&&>(v);
	}

	template <class T>
	class MoveT : public T
	{
	public:
		MoveT(T&& other) noexcept: T(Move(other)) {}

		MoveT(const MoveT& other) noexcept: T(Move(*((T*)&other))) {}

	public:
		T&& release() const noexcept
		{
			return static_cast<T&&>(*((T*)this));
		}

	};
	
	template <class T>
	constexpr T&& Forward(typename RemoveReference<T>::Type& v)
	{
		return static_cast<T&&>(v);
	}

	template <class T>
	constexpr T&& Forward(typename RemoveReference<T>::Type&& v)
	{
		static_assert(!(IsLValue<T>()), "Can't forward an rvalue as an lvalue.");
		return static_cast<T&&>(v);
	}

	template<class T, sl_size_t N>
	constexpr sl_size_t CountOfArray(const T (&)[N])
	{
		return N;
	}
	
	
	template <class IT>
	struct UnsignedType;
	template <>
	struct UnsignedType<int> { typedef unsigned int Type; };
	template <>
	struct UnsignedType<unsigned int> { typedef unsigned int Type; };
	template <>
	struct UnsignedType<short> { typedef unsigned short Type; };
	template <>
	struct UnsignedType<unsigned short> { typedef unsigned short Type; };
	template <>
	struct UnsignedType<char> { typedef unsigned char Type; };
	template <>
	struct UnsignedType<signed char> { typedef unsigned char Type; };
	template <>
	struct UnsignedType<unsigned char> { typedef unsigned char Type; };
	template <>
	struct UnsignedType<long> { typedef unsigned long Type; };
	template <>
	struct UnsignedType<unsigned long> { typedef unsigned long Type; };
	template <>
	struct UnsignedType<sl_int64> { typedef sl_uint64 Type; };
	template <>
	struct UnsignedType<sl_uint64> { typedef sl_uint64 Type; };
	template <>
	struct UnsignedType<char16_t> { typedef char16_t Type; };
	template <>
	struct UnsignedType<char32_t> { typedef char32_t Type; };

	template <class UT>
	struct SignedType;
	template <>
	struct SignedType<int> { typedef int Type; };
	template <>
	struct SignedType<unsigned int> { typedef int Type; };
	template <>
	struct SignedType<short> { typedef short Type; };
	template <>
	struct SignedType<unsigned short> { typedef short Type; };
	template <>
	struct SignedType<char> { typedef signed char Type; };
	template <>
	struct SignedType<signed char> { typedef signed char Type; };
	template <>
	struct SignedType<unsigned char> { typedef signed char Type; };
	template <>
	struct SignedType<long> { typedef long Type; };
	template <>
	struct SignedType<unsigned long> { typedef long Type; };
	template <>
	struct SignedType<sl_int64> { typedef sl_int64 Type; };
	template <>
	struct SignedType<sl_uint64> { typedef sl_int64 Type; };

}

#endif
