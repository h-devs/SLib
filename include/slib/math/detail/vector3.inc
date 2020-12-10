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

#include "../../core/math.h"

namespace slib
{
	
	template <class T, class FT>
	SLIB_INLINE const Vector3T<T, FT>& Vector3T<T, FT>::zero() noexcept
	{
		static SLIB_ALIGN(8) T _zero[3] = {0, 0, 0};
		return *(reinterpret_cast<Vector3T<T, FT> const*>(&_zero));
	}
	
	template <class T, class FT>
	SLIB_INLINE const Vector3T<T, FT>& Vector3T<T, FT>::fromArray(const T arr[3]) noexcept
	{
		return *(reinterpret_cast<Vector3T<T, FT> const*>(arr));
	}
	
	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT>& Vector3T<T, FT>::fromArray(T arr[3]) noexcept
	{
		return *(reinterpret_cast<Vector3T<T, FT>*>(arr));
	}
	
	template <class T, class FT>
	template <class O, class FO>
	SLIB_INLINE Vector3T<T, FT>& Vector3T<T, FT>::operator=(const Vector3T<O, FO>& other) noexcept
	{
		x = (T)(other.x);
		y = (T)(other.y);
		z = (T)(other.z);
		return *this;
	}
	
	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> operator*(T f, const Vector3T<T, FT>& v) noexcept
	{
		return {f * v.x, f * v.y, f * v.z};
	}
	
	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> operator/(T f, const Vector3T<T, FT>& v) noexcept
	{
		return {f / v.x, f / v.y, f / v.z};
	}
	
	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> Interpolation< Vector3T<T, FT> >::interpolate(const Vector3T<T, FT>& a, const Vector3T<T, FT>& b, float factor) noexcept
	{
		return a.lerp(b, factor);
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> Vector3T<T, FT>::fromLocation(const Vector2T<T>& v) noexcept
	{
		return {v.x, v.y, 1};
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> Vector3T<T, FT>::fromDirection(const Vector2T<T>& v) noexcept
	{
		return {v.x, v.y, 0};
	}

	template <class T, class FT>
	SLIB_INLINE T Vector3T<T, FT>::dot(const Vector3T<T, FT>& other) const noexcept
	{
		return x * other.x + y * other.y + z * other.z;
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> Vector3T<T, FT>::cross(const Vector3T<T, FT>& other) const noexcept
	{
		T _x = y*other.z - z*other.y;
		T _y = z*other.x - x*other.z;
		T _z = x*other.y - y*other.x;
		return {_x, _y, _z};
	}

	template <class T, class FT>
	SLIB_INLINE T Vector3T<T, FT>::getLength2p() const noexcept
	{
		return x * x + y * y + z * z;
	}

	template <class T, class FT>
	SLIB_INLINE FT Vector3T<T, FT>::getLength() const noexcept
	{
		return Math::sqrt((FT)(x * x + y * y + z * z));
	}

	template <class T, class FT>
	SLIB_INLINE T Vector3T<T, FT>::getLength2p(const Vector3T<T, FT>& other) const noexcept
	{
		T dx = x - other.x;
		T dy = y - other.y;
		T dz = z - other.z;
		return dx * dx + dy * dy + dz * dz;
	}

	template <class T, class FT>
	SLIB_INLINE FT Vector3T<T, FT>::getLength(const Vector3T<T, FT>& other) const noexcept
	{
		T dx = x - other.x;
		T dy = y - other.y;
		T dz = z - other.z;
		return Math::sqrt((FT)(dx * dx + dy * dy + dz * dz));
	}

	template <class T, class FT>
	SLIB_INLINE void Vector3T<T, FT>::normalize() noexcept
	{
		T l = x * x + y * y + z * z;
		if (l > 0) {
			FT d = Math::sqrt((FT)l);
			x = (T)((FT)x / d);
			y = (T)((FT)y / d);
			z = (T)((FT)z / d);
		}
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> Vector3T<T, FT>::getNormalized() noexcept
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

	template <class T, class FT>
	SLIB_INLINE FT Vector3T<T, FT>::getCosBetween(const Vector3T<T, FT>& other) const noexcept
	{
		return dot(other) / Math::sqrt((FT)(getLength2p() * other.getLength2p()));
	}

	template <class T, class FT>
	SLIB_INLINE FT Vector3T<T, FT>::getAngleBetween(const Vector3T<T, FT>& other) const noexcept
	{
		return Math::arccos(getCosBetween(other));
	}

	template <class T, class FT>
	SLIB_INLINE sl_bool Vector3T<T, FT>::equals(const Vector3T<T, FT>& other) const noexcept
	{
		return x == other.x && y == other.y && z == other.z;
	}

	template <class T, class FT>
	SLIB_INLINE sl_bool Vector3T<T, FT>::isAlmostEqual(const Vector3T<T, FT>& other) const noexcept
	{
		return Math::isAlmostZero((FT)(x - other.x)) && Math::isAlmostZero((FT)(y - other.y)) && Math::isAlmostZero((FT)(z - other.z));
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> Vector3T<T, FT>::lerp(const Vector3T<T, FT>& target, float factor) const noexcept
	{
		return {(T)SLIB_LERP(x, target.x, factor), (T)SLIB_LERP(y, target.y, factor), (T)SLIB_LERP(z, target.z, factor)};
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> Vector3T<T, FT>::operator+(const Vector3T<T, FT>& other) const noexcept
	{
		return {x + other.x, y + other.y, z + other.z};
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT>& Vector3T<T, FT>::operator+=(const Vector3T<T, FT>& other) noexcept
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> Vector3T<T, FT>::operator-(const Vector3T<T, FT>& other) const noexcept
	{
		return {x - other.x, y - other.y, z - other.z};
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT>& Vector3T<T, FT>::operator-=(const Vector3T<T, FT>& other) noexcept
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> Vector3T<T, FT>::operator*(T f) const noexcept
	{
		return {x * f, y * f, z * f};
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT>& Vector3T<T, FT>::operator*=(T f) noexcept
	{
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> Vector3T<T, FT>::operator*(const Vector3T<T, FT>& other) const noexcept
	{
		return {x * other.x, y * other.y, z * other.z};
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT>& Vector3T<T, FT>::operator*=(const Vector3T<T, FT>& other) noexcept
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;
		return *this;
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> Vector3T<T, FT>::operator/(T f) const noexcept
	{
		return {x / f, y / f, z / f};
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT>& Vector3T<T, FT>::operator/=(T f) noexcept
	{
		x /= f;
		y /= f;
		z /= f;
		return *this;
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> Vector3T<T, FT>::operator/(const Vector3T<T, FT>& other) const noexcept
	{
		return {x / other.x, y / other.y, z / other.z};
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT>& Vector3T<T, FT>::operator/(const Vector3T<T, FT>& other) noexcept
	{
		x /= other.x;
		y /= other.y;
		z /= other.z;
		return *this;
	}

	template <class T, class FT>
	SLIB_INLINE Vector3T<T, FT> Vector3T<T, FT>::operator-() const noexcept
	{
		return {-x, -y, -z};
	}

	template <class T, class FT>
	SLIB_INLINE sl_bool Vector3T<T, FT>::operator==(const Vector3T<T, FT>& other) const noexcept
	{
		return x == other.x && y == other.y && z == other.z;
	}

	template <class T, class FT>
	SLIB_INLINE sl_bool Vector3T<T, FT>::operator!=(const Vector3T<T, FT>& other) const noexcept
	{
		return x != other.x || y != other.y || z != other.z;
	}

}
