/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_MATH_VECTOR2
#define CHECKHEADER_SLIB_MATH_VECTOR2

#include "vector.h"

namespace slib
{

	template <class T, class FT>
	class SLIB_EXPORT VectorT<2, T, FT>
	{
	public:
		union {
			struct {
				T x;
				T y;
			};
			T m[2];
		};

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(VectorT)

		VectorT() = default;

		template <class O, class FO>
		SLIB_CONSTEXPR VectorT(const VectorT<2, O, FO>& other): x((T)(other.x)), y((T)(other.y)) {}

		SLIB_CONSTEXPR VectorT(T _x, T _y): x(_x), y(_y) {}

		template <class O>
		VectorT(const O* arr) noexcept
		{
			x = (T)(arr[0]);
			y = (T)(arr[1]);
		}

	public:
		static const VectorT& zero() noexcept
		{
			static SLIB_ALIGN(8) T _zero[2] = { 0 };
			return *(reinterpret_cast<VectorT const*>(&_zero));
		}

		static const VectorT& fromArray(const T arr[2]) noexcept
		{
			return *(reinterpret_cast<VectorT const*>(arr));
		}

		static VectorT& fromArray(T arr[2]) noexcept
		{
			return *(reinterpret_cast<VectorT*>(arr));
		}

		SLIB_CONSTEXPR T dot(const VectorT& other) const
		{
			return x * other.x + y * other.y;
		}

		SLIB_CONSTEXPR T cross(const VectorT& other) const
		{
			return x * other.y - y * other.x;
		}

		T getLength2p() const noexcept
		{
			return x * x + y * y;
		}

		FT getLength() const noexcept
		{
			return Math::sqrt((FT)(x * x + y * y));
		}

		T getLength2p(const VectorT& other) const noexcept
		{
			T dx = x - other.x;
			T dy = y - other.y;
			return dx * dx + dy * dy;
		}

		FT getLength(const VectorT& other) const noexcept
		{
			T dx = x - other.x;
			T dy = y - other.y;
			return Math::sqrt((FT)(dx * dx + dy * dy));
		}

		void normalize() noexcept
		{
			T l = x * x + y * y;
			if (l > 0) {
				FT d = Math::sqrt((FT)l);
				x = (T)((FT)x / d);
				y = (T)((FT)y / d);
			}
		}

		VectorT getNormalized() noexcept
		{
			T l = x * x + y * y;
			if (l > 0) {
				FT d = Math::sqrt((FT)l);
				return {(T)((FT)x / d), (T)((FT)y / d)};
			}
			return *this;
		}

		FT getCosBetween(const VectorT& other) const noexcept
		{
			return (FT)(dot(other)) / Math::sqrt((FT)(getLength2p() * other.getLength2p()));
		}

		FT getAbsAngleBetween(const VectorT& other) const noexcept
		{
			return Math::arccos(getCosBetween(other));
		}

		FT getAngleBetween(const VectorT& other) const noexcept
		{
			FT a = getAbsAngleBetween(other);
			if (cross(other) > 0) {
				a = -a;
			}
			return a;
		}

		SLIB_CONSTEXPR VectorT lerp(const VectorT& target, float factor) const
		{
			return {(T)SLIB_LERP(x, target.x, factor), (T)SLIB_LERP(y, target.y, factor)};
		}

		SLIB_CONSTEXPR VectorT divideReverse(T f) const
		{
			return {f / x, f / y};
		}

		SLIB_CONSTEXPR sl_bool equals(const VectorT& other) const
		{
			return x == other.x && y == other.y;
		}

		SLIB_CONSTEXPR sl_bool isAlmostEqual(const VectorT& other) const
		{
			return Math::isAlmostZero(x - other.x) && Math::isAlmostZero(y - other.y);
		}

	public:
		template <class O, class FO>
		VectorT& operator=(const VectorT<2, O, FO>& other) noexcept
		{
			x = (T)(other.x);
			y = (T)(other.y);
			return *this;
		}

		SLIB_CONSTEXPR VectorT operator+(const VectorT& other) const
		{
			return {x + other.x, y + other.y};
		}

		VectorT& operator+=(const VectorT& other) noexcept
		{
			x += other.x;
			y += other.y;
			return *this;
		}

		SLIB_CONSTEXPR VectorT operator-(const VectorT& other) const
		{
			return {x - other.x, y - other.y};
		}

		VectorT& operator-=(const VectorT& other) noexcept
		{
			x -= other.x;
			y -= other.y;
			return *this;
		}

		SLIB_CONSTEXPR VectorT operator*(T f) const
		{
			return {x * f, y * f};
		}

		VectorT& operator*=(T f) noexcept
		{
			x *= f;
			y *= f;
			return *this;
		}

		SLIB_CONSTEXPR VectorT operator*(const VectorT& other) const
		{
			return {x * other.x, y * other.y};
		}

		VectorT& operator*=(const VectorT& other) noexcept
		{
			x *= other.x;
			y *= other.y;
			return *this;
		}

		SLIB_CONSTEXPR VectorT operator/(T f) const
		{
			return {x / f, y / f};
		}

		VectorT& operator/=(T f) noexcept
		{
			x /= f;
			y /= f;
			return *this;
		}

		SLIB_CONSTEXPR VectorT operator/(const VectorT& other) const
		{
			return {x / other.x, y / other.y};
		}

		VectorT& operator/=(const VectorT& other) noexcept
		{
			x /= other.x;
			y /= other.y;
			return *this;
		}

		SLIB_CONSTEXPR VectorT operator-() const
		{
			return {-x, -y};
		}

	};

	template <class T, class FT = T>
	using Vector2T = VectorT<2, T, FT>;

	typedef Vector2T<sl_real> Vector2;
	typedef Vector2T<float> Vector2f;
	typedef Vector2T<double> Vector2lf;
	typedef Vector2T<sl_int32, float> Vector2i;
	typedef Vector2T<sl_int64, double> Vector2li;

}

#endif
