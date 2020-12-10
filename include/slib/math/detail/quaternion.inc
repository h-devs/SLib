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

	template <class T>
	SLIB_INLINE const QuaternionT<T>& QuaternionT<T>::identity() noexcept
	{
		static SLIB_ALIGN(8) T _identity[4] = { 0, 0, 0, 1 };
		return *(reinterpret_cast<QuaternionT<T> const*>(&_identity));
	}

	template <class T>
	SLIB_INLINE const QuaternionT<T>& QuaternionT<T>::fromArray(const T arr[2]) noexcept
	{
		return *(reinterpret_cast<QuaternionT<T> const*>(arr));
	}
	
	template <class T>
	SLIB_INLINE QuaternionT<T>& QuaternionT<T>::fromArray(T arr[2]) noexcept
	{
		return *(reinterpret_cast<QuaternionT<T>*>(arr));
	}
	
	template <class T>
	SLIB_INLINE const Vector4T<T>& QuaternionT<T>::toVector4() const noexcept
	{
		return *((Vector4T<T>*)((void*)this));
	}
	
	template <class T>
	SLIB_INLINE Vector4T<T>& QuaternionT<T>::toVector4() noexcept
	{
		return *((Vector4T<T>*)((void*)this));
	}
	
	template <class T>
	template <class O>
	SLIB_INLINE QuaternionT<T>& QuaternionT<T>::operator=(const QuaternionT<O>& other) noexcept
	{
		x = (T)(other.x);
		y = (T)(other.y);
		z = (T)(other.z);
		w = (T)(other.w);
		return *this;
	}
	
	template <class T>
	SLIB_INLINE QuaternionT<T>& QuaternionT<T>::operator=(const Vector4T<T>& other) noexcept
	{
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;
		return *this;
	}

	template <class T>
	SLIB_INLINE T QuaternionT<T>::getLength2p() const noexcept
	{
		return x * x + y * y + z * z + w * w;
	}

	template <class T>
	SLIB_INLINE T QuaternionT<T>::getLength() const noexcept
	{
		return Math::sqrt(x * x + y * y + z * z + w * w);
	}

	template <class T>
	SLIB_INLINE void QuaternionT<T>::multiply(const QuaternionT<T>& other) noexcept
	{
		T ox = other.x, oy = other.y, oz = other.z, ow = other.w;
		T qx = w * ox + x * ow + y * oz - z * oy;
		T qy = w * oy + y * ow + z * ox - x * oz;
		T qz = w * oz + z * ow + x * oy - y * ox;
		T qw = w * ow + x * ox + y * oy - z * oz;
		x = qx; y = qy; z = qz; w = qw;
	}

	template <class T>
	SLIB_INLINE void QuaternionT<T>::divide(const QuaternionT<T>& other) noexcept
	{
		T ox = -other.x, oy = -other.y, oz = -other.z, ow = other.w;
		T qx = w * ox + x * ow + y * oz - z * oy;
		T qy = w * oy + y * ow + z * ox - x * oz;
		T qz = w * oz + z * ow + x * oy - y * ox;
		T qw = w * ow + x * ox + y * oy - z * oz;
		x = qx; y = qy; z = qz; w = qw;
	}

	template <class T>
	SLIB_INLINE void QuaternionT<T>::setRotation(const Vector3T<T>& vAxis, T fAngle) noexcept
	{
		T f = Math::sin(fAngle / 2) / vAxis.getLength();
		x = vAxis.x * f;
		y = vAxis.y * f;
		z = vAxis.z * f;
		w = Math::cos(fAngle / 2);
	}

	template <class T>
	SLIB_INLINE QuaternionT<T> QuaternionT<T>::getRotation(const Vector3T<T>& vAxis, T fAngle) noexcept
	{
		QuaternionT<T> ret;
		ret.setRotation(vAxis, fAngle);
		return ret;
	}

	template <class T>
	SLIB_INLINE T QuaternionT<T>::getAngle() const noexcept
	{
		return 2 * Math::arccos(w);
	}

	template <class T>
	SLIB_INLINE Vector4T<T> QuaternionT<T>::getAxis() const noexcept
	{
		return {x, y, z, 0};
	}

	template <class T>
	SLIB_INLINE void QuaternionT<T>::makeInverse() noexcept
	{
		x = -x;
		y = -y;
		z = -z;
	}

	template <class T>
	SLIB_INLINE QuaternionT<T> QuaternionT<T>::inverse() const noexcept
	{
		QuaternionT<T> ret = *this;
		ret.makeInverse();
		return ret;
	}

	template <class T>
	SLIB_INLINE QuaternionT<T>& QuaternionT<T>::operator*=(const QuaternionT<T>& other) noexcept
	{
		multiply(other);
		return *this;
	}

	template <class T>
	SLIB_INLINE QuaternionT<T> QuaternionT<T>::operator*(const QuaternionT<T>& other) const noexcept
	{
		QuaternionT<T> ret = *this;
		ret.multiply(other);
		return ret;
	}

	template <class T>
	SLIB_INLINE QuaternionT<T>& QuaternionT<T>::operator/=(const QuaternionT<T>& other) noexcept
	{
		divide(other);
		return *this;
	}

	template <class T>
	SLIB_INLINE QuaternionT<T> QuaternionT<T>::operator/(QuaternionT<T>& other) const noexcept
	{
		QuaternionT<T> ret = *this;
		ret.divide(other);
		return ret;
	}

	template <class T>
	SLIB_INLINE sl_bool QuaternionT<T>::operator==(const QuaternionT<T>& other) const noexcept
	{
		return (x == other.x) && (y == other.y) && (z == other.z) && (w == other.w);
	}

	template <class T>
	SLIB_INLINE sl_bool QuaternionT<T>::operator!=(const QuaternionT<T>& other) const noexcept
	{
		return (x != other.x) || (y != other.y) || (z != other.z) || (w != other.w);
	}

}
