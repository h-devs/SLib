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

#ifndef CHECKHEADER_SLIB_MATH_DECIMAL128
#define CHECKHEADER_SLIB_MATH_DECIMAL128

#include "definition.h"

#include "../core/common_members.h"

/*
	IEEE 754-2008 128-bit decimal floating point (Decimal128):
		Supports 34 decimal digits of precision, a max value of approximately 10^6145, and min value of approximately -10^6145.

	Clamping:
		Clamping happens when a value�s exponent is too large for the destination format. This works by adding zeros to the coefficient to reduce the exponent to the largest usable value. An overflow occurs if the number of digits required is more than allowed in the destination format.

	Binary Integer Decimal (BID):
		Uses this binary encoding for the coefficient as specified in IEEE 754-2008 section 3.5.2 using method 2 "binary encoding" rather than method 1 "decimal encoding".
*/

#define SLIB_DECIMAL128_MAX_DIGITS 34

#ifdef negativeInfinity
#undef negativeInfinity
#endif

namespace slib
{

	class SLIB_EXPORT Decimal128
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
		constexpr Decimal128(): high(0), low(0) {}

		constexpr Decimal128(const Decimal128& other): high(other.high), low(other.low) {}
		
		constexpr Decimal128(sl_uint64 _high, sl_uint64 _low): high(_high), low(_low) {}

	private:
		static const sl_uint64 _zero[2];

	public:
		static const Decimal128& zero() noexcept
		{
			return *((Decimal128*)((void*)_zero));
		}

		static const Decimal128& infinity() noexcept;

		static const Decimal128& negativeInfinity() noexcept;

		static const Decimal128& NaN() noexcept;

		constexpr sl_bool isZero() const
		{
			return high == 0 && low == 0;
		}

		constexpr sl_bool isNotZero() const
		{
			return high != 0 || low != 0;
		}

		void setZero() noexcept
		{
			high = 0;
			low = 0;
		}

		constexpr sl_bool isPositive() const
		{
			return !((sl_bool)(high >> 63));
		}

		constexpr sl_bool isNegative() const
		{
			return (sl_bool)(high >> 63);
		}

		sl_bool isInfinity() const noexcept;

		void setInfinity(sl_bool flagPositive) noexcept;

		sl_bool isPositiveInfinity() const noexcept;

		void setPositiveInfinity() noexcept;

		sl_bool isNegativeInfinity() const noexcept;

		void setNegativeInfinity() noexcept;

		sl_bool isNaN() const noexcept;

		void setNaN() noexcept;

	public:
		// 16 bytes
		void getBytesBE(void* buf) noexcept;

		// 16 bytes
		void setBytesBE(const void* buf) noexcept;

		// 16 bytes
		void getBytesLE(void* buf) noexcept;

		// 16 bytes
		void setBytesLE(const void* buf) noexcept;

		static Decimal128 fromString(const StringParam& str) noexcept;

	public:
		SLIB_DECLARE_CLASS_COMMON_MEMBERS(Decimal128)

	public:
		Decimal128 & operator=(const Decimal128& other) noexcept
		{
			low = other.low;
			high = other.high;
			return *this;
		}

		Decimal128 operator+(const Decimal128& other) const noexcept;

		Decimal128& operator+=(const Decimal128& other) noexcept;

		Decimal128 operator-(const Decimal128& other) const noexcept;

		Decimal128& operator-=(const Decimal128& other) noexcept;

		Decimal128 operator*(const Decimal128& other) const noexcept;

		Decimal128& operator*=(const Decimal128& other) noexcept;

		Decimal128 operator/(const Decimal128& other) const noexcept;

		Decimal128& operator/=(const Decimal128& other) noexcept;

		Decimal128 operator-() const noexcept;

	};

}

#endif

