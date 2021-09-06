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

#ifndef CHECKHEADER_SLIB_MATH_INT128
#define CHECKHEADER_SLIB_MATH_INT128

#include "definition.h"

#include "../core/common_members.h"
#include "../core/math.h"

#define SLIB_UINT128_MAX_LOG10I 38

namespace slib
{
	
	class SLIB_EXPORT Uint128
	{
	public:
#ifdef SLIB_ARCH_IS_LITTLE_ENDIAN
		sl_uint64 low;
		sl_uint64 high;
#else
		sl_uint64 high;
		sl_uint64 low;
#endif
	
	public:
		SLIB_CONSTEXPR Uint128(): high(0), low(0) {}

		SLIB_CONSTEXPR Uint128(const Uint128& other): high(other.high), low(other.low) {}

		SLIB_CONSTEXPR Uint128(sl_uint64 _high, sl_uint64 _low): high(_high), low(_low) {}

		SLIB_CONSTEXPR Uint128(sl_uint64 num): high(0), low(num) {}

	private:
		static const sl_uint64 _zero[2];
	
	public:
		static const Uint128& zero() noexcept
		{
			return *((Uint128*)((void*)_zero));
		}

		SLIB_CONSTEXPR sl_bool isZero() const
		{
			return high == 0 && low == 0;
		}

		SLIB_CONSTEXPR sl_bool isNotZero() const
		{
			return high != 0 || low != 0;
		}

		void setZero() noexcept
		{
			high = 0;
			low = 0;
		}
	
		sl_uint32 getMostSignificantBits() const noexcept;

		sl_uint32 getLeastSignificantBits() const noexcept;

		// 16 bytes
		void getBytesBE(void* buf) noexcept;

		// 16 bytes
		void setBytesBE(const void* buf) noexcept;

		// 16 bytes
		void getBytesLE(void* buf) noexcept;

		// 16 bytes
		void setBytesLE(const void* buf) noexcept;

	public:
		static Uint128 mul64(sl_uint64 a, sl_uint64 b) noexcept;

		static sl_bool div(const Uint128& a, const Uint128& b, Uint128* quotient = sl_null, Uint128* remainder = sl_null) noexcept;

		static sl_bool div64(const Uint128& a, sl_uint64 b, Uint128* quotient = sl_null, sl_uint64* remainder = sl_null) noexcept;

		static sl_bool div32(const Uint128& a, sl_uint32 b, Uint128* quotient = sl_null, sl_uint32* remainder = sl_null) noexcept;

		Uint128 div(const Uint128& other) const noexcept;

		Uint128 div(sl_uint64 num) const noexcept;

		Uint128 div(sl_uint32 num) const noexcept;

		Uint128 mod(const Uint128& other) const noexcept;

		sl_uint64 mod(sl_uint64 num) const noexcept;

		sl_uint32 mod(sl_uint32 num) const noexcept;

		void shiftRight() noexcept;

		void shiftLeft() noexcept;

		void makeNegative() noexcept;

		void makeBitwiseNot() noexcept;

		static const Uint128& pow10_32() noexcept;

		static Uint128 pow10(sl_uint32 exponent) noexcept;

		sl_uint32 log10() const noexcept;
	
	public:
		Uint128& operator=(const Uint128& other) noexcept
		{
			low = other.low;
			high = other.high;
			return *this;
		}

		Uint128& operator=(sl_uint64 num) noexcept
		{
			high = 0;
			low = num;
			return *this;
		}

		SLIB_CONSTEXPR operator sl_uint64() const
		{
			return low;
		}


		Uint128 operator+(const Uint128& other) const noexcept;

		Uint128 operator+(sl_uint64 num) const noexcept;

		friend Uint128 operator+(sl_uint64 num, const Uint128& v) noexcept;

		Uint128& operator+=(const Uint128& other) noexcept;

		Uint128& operator+=(sl_uint32 num) noexcept;
	
		Uint128& operator++() noexcept;

		Uint128 operator++(int) noexcept;


		Uint128 operator-(const Uint128& other) const noexcept;

		Uint128 operator-(sl_uint64 num) const noexcept;

		friend Uint128 operator-(sl_uint64 num, const Uint128& v) noexcept;
	
		Uint128& operator-=(const Uint128& other) noexcept;

		Uint128& operator-=(sl_uint64 num) noexcept;

		Uint128& operator--() noexcept;

		Uint128 operator--(int) noexcept;


		Uint128 operator*(const Uint128& other) const noexcept;

		Uint128 operator*(sl_uint64 num) const noexcept;

		friend Uint128 operator*(sl_uint64 num, const Uint128& v) noexcept;

		Uint128& operator*=(const Uint128& other) noexcept;

		Uint128& operator*=(sl_uint64 num) noexcept;


		Uint128 operator/(const Uint128& other) const noexcept;

		Uint128 operator/(sl_uint64 num) const noexcept;

		Uint128 operator/(sl_uint32 num) const noexcept;

		friend Uint128 operator/(sl_uint64 num, const Uint128& v) noexcept;
	
		Uint128& operator/=(const Uint128& other) noexcept;

		Uint128& operator/=(sl_uint64 num) noexcept;

		Uint128& operator/=(sl_uint32 num) noexcept;


		Uint128 operator%(const Uint128& other) const noexcept;

		Uint128 operator%(sl_uint64 num) const noexcept;

		Uint128 operator%(sl_uint32 num) const noexcept;

		friend Uint128 operator%(sl_uint64 num, const Uint128& v) noexcept;
	
		Uint128& operator%=(const Uint128& other) noexcept;

		Uint128& operator%=(sl_uint64 num) noexcept;

		Uint128& operator%=(sl_uint32 num) noexcept;


		Uint128 operator&(const Uint128& other) const noexcept;

		Uint128 operator&(sl_uint64 num) const noexcept;

		friend Uint128 operator&(sl_uint64 num, const Uint128& v) noexcept;

		Uint128& operator&=(const Uint128& other) noexcept;

		Uint128& operator&=(sl_uint32 num) noexcept;
	

		Uint128 operator|(const Uint128& other) const noexcept;

		Uint128 operator|(sl_uint64 num) const noexcept;

		friend Uint128 operator|(sl_uint64 num, const Uint128& v) noexcept;

		Uint128& operator|=(const Uint128& other) noexcept;

		Uint128& operator|=(sl_uint32 num) noexcept;
	

		Uint128 operator^(const Uint128& other) const noexcept;

		Uint128 operator^(sl_uint64 num) const noexcept;

		friend Uint128 operator^(sl_uint64 num, const Uint128& v) noexcept;
	
		Uint128& operator^=(const Uint128& other) noexcept;

		Uint128& operator^=(sl_uint32 num) noexcept;
	

		Uint128 operator>>(sl_uint32 n) const noexcept;

		Uint128& operator>>=(sl_uint32 n) noexcept;
	

		Uint128 operator<<(sl_uint32 n) const noexcept;

		Uint128& operator<<=(sl_uint32 n) noexcept;
	

		Uint128 operator-() const noexcept;
	
		Uint128 operator~() const noexcept;
	
	public:
		static Uint128 fromString(const StringParam& str, sl_uint32 radix = 10) noexcept;

		String toString(sl_uint32 radix = 10) const noexcept;
	
		static Uint128 fromHexString(const StringParam& str) noexcept;

		String toHexString() const noexcept;

	public:
		SLIB_DECLARE_CLASS_COMPARE_HASH_MEMBERS(Uint128)
		SLIB_DECLARE_CLASS_PARSE_INT_MEMBERS(Uint128)
		SLIB_DECLARE_CLASS_JSON_SERIALIZE_MEMBERS

		sl_bool equals(sl_uint64 num) const noexcept;
		sl_compare_result compare(sl_uint64 num) const noexcept;

		friend sl_bool operator==(sl_uint64 num, const Uint128& v) noexcept;
		friend sl_bool operator!=(sl_uint64 num, const Uint128& v) noexcept;
		friend sl_bool operator>=(sl_uint64 num, const Uint128& v) noexcept;
		friend sl_bool operator<=(sl_uint64 num, const Uint128& v) noexcept;
		friend sl_bool operator>(sl_uint64 num, const Uint128& v) noexcept;
		friend sl_bool operator<(sl_uint64 num, const Uint128& v) noexcept;

	};
	
	template <>
	void Math::pow10iT<Uint128>(Uint128& _out, sl_uint32 exponent) noexcept;
	
	template <>
	sl_uint32 Math::log10iT<Uint128>(const Uint128& v) noexcept;

}

#endif

