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

#ifndef CHECKHEADER_SLIB_MATH_QUATERNION
#define CHECKHEADER_SLIB_MATH_QUATERNION

#include "vector4.h"

namespace slib
{

	template <class T>
	class SLIB_EXPORT QuaternionT
	{
	public:
		T x;
		T y;
		T z;
		T w;

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(QuaternionT)

		QuaternionT() = default;

		template <class O>
		SLIB_CONSTEXPR QuaternionT(const QuaternionT<O>& other): x((T)(other.x)), y((T)(other.y)), z((T)(other.z)), w((T)(other.w)) {}

		SLIB_CONSTEXPR QuaternionT(T _x, T _y, T _z, T _w): x(_x), y(_y), z(_z), w(_w) {}

		SLIB_CONSTEXPR QuaternionT(const Vector4T<T>& other): x(other.x), y(other.y), z(other.z), w(other.w) {}

	public:
		static const QuaternionT<T>& identity() noexcept
		{
			static SLIB_ALIGN(8) T _identity[4] = { 0, 0, 0, 1 };
			return *(reinterpret_cast<QuaternionT<T> const*>(&_identity));
		}

		static const QuaternionT<T>& fromArray(const T arr[2]) noexcept
		{
			return *(reinterpret_cast<QuaternionT<T> const*>(arr));
		}

		static QuaternionT<T>& fromArray(T arr[2]) noexcept
		{
			return *(reinterpret_cast<QuaternionT<T>*>(arr));
		}

		const Vector4T<T>& toVector4() const noexcept
		{
			return *((Vector4T<T>*)((void*)this));
		}

		Vector4T<T>& toVector4() noexcept
		{
			return *((Vector4T<T>*)((void*)this));
		}

		T getLength2p() const noexcept
		{
			return x * x + y * y + z * z + w * w;
		}

		T getLength() const noexcept
		{
			return Math::sqrt(x * x + y * y + z * z + w * w);
		}

		void multiply(const QuaternionT<T>& other) noexcept
		{
			T ox = other.x, oy = other.y, oz = other.z, ow = other.w;
			T qx = w * ox + x * ow + y * oz - z * oy;
			T qy = w * oy + y * ow + z * ox - x * oz;
			T qz = w * oz + z * ow + x * oy - y * ox;
			T qw = w * ow + x * ox + y * oy - z * oz;
			x = qx; y = qy; z = qz; w = qw;
		}

		void divide(const QuaternionT<T>& other) noexcept
		{
			T ox = -other.x, oy = -other.y, oz = -other.z, ow = other.w;
			T qx = w * ox + x * ow + y * oz - z * oy;
			T qy = w * oy + y * ow + z * ox - x * oz;
			T qz = w * oz + z * ow + x * oy - y * ox;
			T qw = w * ow + x * ox + y * oy - z * oz;
			x = qx; y = qy; z = qz; w = qw;
		}

		void setRotation(const Vector3T<T>& vAxis, T fAngle) noexcept
		{
			T f = Math::sin(fAngle / 2) / vAxis.getLength();
			x = vAxis.x * f;
			y = vAxis.y * f;
			z = vAxis.z * f;
			w = Math::cos(fAngle / 2);
		}

		static QuaternionT<T> getRotation(const Vector3T<T>& vAxis, T fAngle) noexcept
		{
			QuaternionT<T> ret;
			ret.setRotation(vAxis, fAngle);
			return ret;
		}

		T getAngle() const noexcept
		{
			return 2 * Math::arccos(w);
		}

		Vector4T<T> getAxis() const noexcept
		{
			return { x, y, z, 0 };
		}

		void makeInverse() noexcept
		{
			x = -x;
			y = -y;
			z = -z;
		}

		QuaternionT<T> inverse() const noexcept
		{
			QuaternionT<T> ret = *this;
			ret.makeInverse();
			return ret;
		}

		SLIB_CONSTEXPR sl_bool equals(const QuaternionT& other) const
		{
			return x == other.x && y == other.y && z == other.z && w == other.w;
		}

		SLIB_CONSTEXPR sl_bool isAlmostEqual(const QuaternionT& other) const
		{
			return Math::isAlmostZero(x - other.x) && Math::isAlmostZero(y - other.y) && Math::isAlmostZero(z - other.z) && Math::isAlmostZero(w - other.w);
		}

	public:
		template <class O>
		QuaternionT<T>& operator=(const QuaternionT<O>& other) noexcept
		{
			x = (T)(other.x);
			y = (T)(other.y);
			z = (T)(other.z);
			w = (T)(other.w);
			return *this;
		}

		QuaternionT<T>& operator=(const Vector4T<T>& other) noexcept
		{
			x = other.x;
			y = other.y;
			z = other.z;
			w = other.w;
			return *this;
		}

		QuaternionT<T>& operator*=(const QuaternionT<T>& other) noexcept
		{
			multiply(other);
			return *this;
		}

		QuaternionT<T> operator*(const QuaternionT<T>& other) const noexcept
		{
			QuaternionT<T> ret = *this;
			ret.multiply(other);
			return ret;
		}

		QuaternionT<T>& operator/=(const QuaternionT<T>& other) noexcept
		{
			divide(other);
			return *this;
		}

		QuaternionT<T> operator/(QuaternionT<T>& other) const noexcept
		{
			QuaternionT<T> ret = *this;
			ret.divide(other);
			return ret;
		}

	};

	typedef QuaternionT<sl_real> Quaternion;
	typedef QuaternionT<float> Quaternionf;
	typedef QuaternionT<double> Quaternionlf;

}

#endif
