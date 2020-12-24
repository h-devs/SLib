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

#ifndef CHECKHEADER_SLIB_MATH_VECTOR4
#define CHECKHEADER_SLIB_MATH_VECTOR4

#include "definition.h"

#include "vector3.h"

namespace slib
{
	
	template <class T, class FT>
	class SLIB_EXPORT VectorT<4, T, FT>
	{
	public:
		union {
			struct {
				T x;
				T y;
				T z;
				T w;
			};
			T m[4];
		};

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(VectorT)
		
		VectorT() noexcept = default;
	
		template <class O, class FO>
		constexpr VectorT(const VectorT<4, O, FO>& other) noexcept : x((T)(other.x)), y((T)(other.y)), z((T)(other.z)), w((T)(other.w)) {}
	
		constexpr VectorT(T _x, T _y, T _z, T _w) noexcept : x(_x), y(_y), z(_z), w(_w) {}

		constexpr VectorT(const VectorT<3, T, FT>& xyz, T _w) noexcept : x(xyz.x), y(xyz.y), z(xyz.z), w(_w) {}

		template <class O>
		VectorT(const O* arr) noexcept
		{
			x = (T)(arr[0]);
			y = (T)(arr[1]);
			z = (T)(arr[2]);
			w = (T)(arr[3]);
		}
		
	public:
		static const VectorT& zero() noexcept
		{
			static SLIB_ALIGN(8) T _zero[4] = { 0 };
			return *(reinterpret_cast<VectorT const*>(&_zero));
		}

		static const VectorT& fromArray(const T arr[4]) noexcept
		{
			return *(reinterpret_cast<VectorT const*>(arr));
		}
	
		static VectorT& fromArray(T arr[4]) noexcept
		{
			return *(reinterpret_cast<VectorT*>(arr));
		}
	
	public:
		const VectorT<3, T, FT>& xyz() const noexcept
		{
			return *(reinterpret_cast<VectorT<3, T, FT> const*>(this));
		}

		VectorT<3, T, FT>& xyz() noexcept
		{
			return *(reinterpret_cast<VectorT<3, T, FT>*>(this));
		}

		static VectorT fromLocation(const VectorT<3, T, FT>& v) noexcept
		{
			return {v.x, v.y, v.z, 1};
		}

		static VectorT fromDirection(const VectorT<3, T, FT>& v) noexcept
		{
			return {v.x, v.y, v.z, 0};
		}

		T dot(const VectorT& other) const noexcept
		{
			return x * other.x + y * other.y + z * other.z + w * other.w;
		}

		T getLength2p() const noexcept
		{
			return x * x + y * y + z * z + w * w;
		}

		FT getLength() const noexcept
		{
			return Math::sqrt((FT)(x * x + y * y + z * z + w * w));
		}

		T getLength2p(const VectorT& other) const noexcept
		{
			T dx = x - other.x;
			T dy = y - other.y;
			T dz = z - other.z;
			T dw = w - other.w;
			return dx * dx + dy * dy + dz * dz + dw * dw;
		}

		FT getLength(const VectorT& other) const noexcept
		{
			T dx = x - other.x;
			T dy = y - other.y;
			T dz = z - other.z;
			T dw = w - other.w;
			return Math::sqrt((FT)(dx * dx + dy * dy + dz * dz + dw * dw));
		}

		void normalize() noexcept
		{
			T l = x * x + y * y + z * z + w * w;
			if (l > 0) {
				FT d = Math::sqrt((FT)l);
				x = (T)((FT)x / d);
				y = (T)((FT)y / d);
				z = (T)((FT)z / d);
				w = (T)((FT)w / d);
			}
		}

		VectorT getNormalized() noexcept
		{
			T l = x * x + y * y + z * z + w * w;
			if (l > 0) {
				FT d = Math::sqrt((FT)l);
				T _x = (T)((FT)x / d);
				T _y = (T)((FT)y / d);
				T _z = (T)((FT)z / d);
				T _w = (T)((FT)w / d);
				return {_x, _y, _z, _w};
			}
			return *this;
		}

		FT getCosBetween(const VectorT& other) const noexcept
		{
			return (FT)(dot(other)) / Math::sqrt((FT)(getLength2p() * other.getLength2p()));
		}

		FT getAngleBetween(const VectorT& other) const noexcept
		{
			return Math::arccos(getCosBetween(other));
		}

		VectorT lerp(const VectorT& target, float factor) const noexcept
		{
			return {(T)SLIB_LERP(x, target.x, factor), (T)SLIB_LERP(y, target.y, factor), (T)SLIB_LERP(z, target.z, factor), (T)SLIB_LERP(w, target.w, factor)};
		}
	
		VectorT divideReverse(T f) const noexcept
		{
			return {f / x, f / y, f / z, f / w};
		}

		sl_bool equals(const VectorT& other) const noexcept
		{
			return x == other.x && y == other.y && z == other.z && w == other.w;
		}
	
		sl_bool isAlmostEqual(const VectorT& other) const noexcept
		{
			return Math::isAlmostZero(x - other.x) &&
				Math::isAlmostZero(y - other.y) &&
				Math::isAlmostZero(z - other.z) &&
				Math::isAlmostZero(w - other.w);
		}

	public:
		template <class O, class FO>
		VectorT& operator=(const VectorT<4, O, FO>& other) noexcept
		{
			x = (T)(other.x);
			y = (T)(other.y);
			z = (T)(other.z);
			w = (T)(other.w);
			return *this;
		}

		VectorT operator+(const VectorT& other) const noexcept
		{
			return {x + other.x, y + other.y, z + other.z, w + other.w};
		}

		VectorT& operator+=(const VectorT& other) noexcept
		{
			x += other.x;
			y += other.y;
			z += other.z;
			w += other.w;
			return *this;
		}

		VectorT operator-(const VectorT& other) const noexcept
		{
			return {x - other.x, y - other.y, z - other.z, w - other.w};
		}

		VectorT& operator-=(const VectorT& other) noexcept
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			w -= other.w;
			return *this;
		}

		VectorT operator*(T f) const noexcept
		{
			return {x * f, y * f, z * f, w * f};
		}

		VectorT& operator*=(T f) noexcept
		{
			x *= f;
			y *= f;
			z *= f;
			w *= f;
			return *this;
		}

		VectorT operator*(const VectorT& other) const noexcept
		{
			return {x * other.x, y * other.y, z * other.z, w * other.w};
		}

		VectorT& operator*=(const VectorT& other) noexcept
		{
			x *= other.x;
			y *= other.y;
			z *= other.z;
			w *= other.w;
			return *this;
		}

		VectorT operator/(T f) const noexcept
		{
			return {x / f, y / f, z / f, w / f};
		}

		VectorT& operator/=(T f) noexcept
		{
			x /= f;
			y /= f;
			z /= f;
			w /= f;
			return *this;
		}

		VectorT operator/(const VectorT& other) const noexcept
		{
			return {x / other.x, y / other.y, z / other.z, w / other.w};
		}

		VectorT& operator/=(const VectorT& other) noexcept
		{
			x /= other.x;
			y /= other.y;
			z /= other.z;
			w /= other.w;
			return *this;
		}

		VectorT operator-() const noexcept
		{
			return {-x, -y, -z, -w};
		}

		sl_bool operator==(const VectorT& other) const noexcept
		{
			return x == other.x && y == other.y && z == other.z && w == other.w;
		}

		sl_bool operator!=(const VectorT& other) const noexcept
		{
			return x != other.x || y != other.y || z != other.z || w != other.w;
		}

	};

	template <class T, class FT = T>
	using Vector4T = VectorT<4, T, FT>;
	
	typedef Vector4T<sl_real> Vector4;
	typedef Vector4T<float> Vector4f;
	typedef Vector4T<double> Vector4lf;
	typedef Vector4T<sl_int32, float> Vector4i;
	typedef Vector4T<sl_int64, double> Vector4li;

}

#endif
