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

#ifndef CHECKHEADER_SLIB_MATH_VECTOR3
#define CHECKHEADER_SLIB_MATH_VECTOR3

#include "vector2.h"

namespace slib
{

	template <class T, class FT>
	class SLIB_EXPORT VectorT<3, T, FT>
	{
	public:
		union {
			struct {
				T x;
				T y;
				T z;
			};
			T m[3];
		};

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(VectorT)

		VectorT() = default;

		template <class O, class FO>
		SLIB_CONSTEXPR VectorT(const VectorT<3, O, FO>& other): x((T)(other.x)), y((T)(other.y)), z((T)(other.z)) {}

		SLIB_CONSTEXPR VectorT(T _x, T _y, T _z): x(_x), y(_y), z(_z) {}

		template <class O>
		VectorT(const O* arr) noexcept
		{
			x = (T)(arr[0]);
			y = (T)(arr[1]);
			z = (T)(arr[2]);
		}

	public:
		static const VectorT& zero() noexcept
		{
			static SLIB_ALIGN(8) T _zero[3] = { 0 };
			return *(reinterpret_cast<VectorT const*>(&_zero));
		}

		static const VectorT& cast(const T arr[3]) noexcept
		{
			return *(reinterpret_cast<VectorT const*>(arr));
		}

		static VectorT& cast(T arr[3]) noexcept
		{
			return *(reinterpret_cast<VectorT*>(arr));
		}

		static VectorT fromLocation(const VectorT<2, T, FT>& v) noexcept
		{
			return {v.x, v.y, 1};
		}

		static VectorT fromDirection(const VectorT<2, T, FT>& v) noexcept
		{
			return {v.x, v.y, 0};
		}

		SLIB_CONSTEXPR T dot(const VectorT& other) const
		{
			return x * other.x + y * other.y + z * other.z;
		}

		SLIB_CONSTEXPR VectorT cross(const VectorT& other) const
		{
			return { y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x };
		}

		T getLength2p() const noexcept
		{
			return x * x + y * y + z * z;
		}

		FT getLength() const noexcept
		{
			return Math::sqrt((FT)(x * x + y * y + z * z));
		}

		T getLength2p(const VectorT& other) const noexcept
		{
			T dx = x - other.x;
			T dy = y - other.y;
			T dz = z - other.z;
			return dx * dx + dy * dy + dz * dz;
		}

		FT getLength(const VectorT& other) const noexcept
		{
			T dx = x - other.x;
			T dy = y - other.y;
			T dz = z - other.z;
			return Math::sqrt((FT)(dx * dx + dy * dy + dz * dz));
		}

		void normalize() noexcept
		{
			T l = x * x + y * y + z * z;
			if (l > 0) {
				FT d = Math::sqrt((FT)l);
				x = (T)((FT)x / d);
				y = (T)((FT)y / d);
				z = (T)((FT)z / d);
			}
		}

		VectorT getNormalized() noexcept
		{
			T l = x * x + y * y + z * z;
			if (l > 0) {
				FT d = Math::sqrt((FT)l);
				T _x = (T)((FT)x / d);
				T _y = (T)((FT)y / d);
				T _z = (T)((FT)z / d);
				return {_x, _y, _z};
			}
			return *this;
		}

		FT getCosBetween(const VectorT& other) const noexcept
		{
			return dot(other) / Math::sqrt((FT)(getLength2p() * other.getLength2p()));
		}

		FT getAngleBetween(const VectorT& other) const noexcept
		{
			return Math::arccos(getCosBetween(other));
		}

		SLIB_CONSTEXPR VectorT lerp(const VectorT& target, float factor) const
		{
			return {(T)SLIB_LERP(x, target.x, factor), (T)SLIB_LERP(y, target.y, factor), (T)SLIB_LERP(z, target.z, factor)};
		}

		SLIB_CONSTEXPR VectorT divideReverse(T f) const
		{
			return {f / x, f / y, f / z};
		}

		SLIB_CONSTEXPR sl_bool equals(const VectorT& other) const
		{
			return x == other.x && y == other.y && z == other.z;
		}

		SLIB_CONSTEXPR sl_bool isAlmostEqual(const VectorT& other) const
		{
			return Math::isAlmostZero(x - other.x) && Math::isAlmostZero(y - other.y) && Math::isAlmostZero(z - other.z);
		}

	public:
		template <class O, class FO>
		VectorT& operator=(const VectorT<3, O, FO>& other) noexcept
		{
			x = (T)(other.x);
			y = (T)(other.y);
			z = (T)(other.z);
			return *this;
		}

		SLIB_CONSTEXPR VectorT operator+(const VectorT& other) const
		{
			return {x + other.x, y + other.y, z + other.z};
		}

		VectorT& operator+=(const VectorT& other) noexcept
		{
			x += other.x;
			y += other.y;
			z += other.z;
			return *this;
		}

		SLIB_CONSTEXPR VectorT operator-(const VectorT& other) const
		{
			return {x - other.x, y - other.y, z - other.z};
		}

		VectorT& operator-=(const VectorT& other) noexcept
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}

		SLIB_CONSTEXPR VectorT operator*(T f) const
		{
			return {x * f, y * f, z * f};
		}

		VectorT& operator*=(T f) noexcept
		{
			x *= f;
			y *= f;
			z *= f;
			return *this;
		}

		SLIB_CONSTEXPR VectorT operator*(const VectorT& other) const
		{
			return {x * other.x, y * other.y, z * other.z};
		}

		VectorT& operator*=(const VectorT& other) noexcept
		{
			x *= other.x;
			y *= other.y;
			z *= other.z;
			return *this;
		}

		SLIB_CONSTEXPR VectorT operator/(T f) const
		{
			return {x / f, y / f, z / f};
		}

		VectorT& operator/=(T f) noexcept
		{
			x /= f;
			y /= f;
			z /= f;
			return *this;
		}

		SLIB_CONSTEXPR VectorT operator/(const VectorT& other) const
		{
			return {x / other.x, y / other.y, z / other.z};
		}

		VectorT& operator/=(const VectorT& other) noexcept
		{
			x /= other.x;
			y /= other.y;
			z /= other.z;
			return *this;
		}

		SLIB_CONSTEXPR VectorT operator-() const noexcept
		{
			return {-x, -y, -z};
		}

	};

	template <class T, class FT = T>
	using Vector3T = VectorT<3, T, FT>;

	typedef Vector3T<sl_real> Vector3;
	typedef Vector3T<float> Float3;
	typedef Vector3T<double> Double3;

}

#endif
