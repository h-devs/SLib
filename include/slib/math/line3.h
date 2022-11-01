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

#ifndef CHECKHEADER_SLIB_MATH_LINE3
#define CHECKHEADER_SLIB_MATH_LINE3

#include "matrix4.h"

namespace slib
{

	template <class T>
	class SLIB_EXPORT Line3T
	{
	public:
		Vector3T<T> point1;
		Vector3T<T> point2;

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(Line3T)

		Line3T() = default;

		template <class O>
		Line3T(const Line3T<O>& other): point1(other.point1), point2(other.point2) {}

		Line3T(const Vector3T<T>& _point1, const Vector3T<T>& _point2): point1(_point1), point2(_point2) {}

		Line3T(T x1, T y1, T z1, T x2, T y2, T z2): point1(x1, y1, z1), point2(x2, y2, z2) {}

	public:
		SLIB_CONSTEXPR Vector3T<T> getDirection() const
		{
			return point2 - point1;
		}

		T getLength2p() const noexcept
		{
			return point1.getLength2p(point2);
		}

		T getLength() const noexcept
		{
			return point1.getLength(point2);
		}

		Vector3T<T> projectPoint(const Vector3T<T>& point) const noexcept
		{
			Vector3T<T> dir = point2 - point1;
			return point1 + (point - point1).dot(dir) * dir;
		}

		T getDistanceFromPoint(const Vector3T<T>& point) const noexcept
		{
			Vector3T<T> dir = point2 - point1;
			T f = (point - point1).dot(dir);
			Vector3T<T> proj = point1 + f * dir;
			if (f < 0) {
				return point1.getLength(point);
			} else {
				if (f > getLength()) {
					return point2.getLength(point);
				} else {
					return proj.getLength(point);
				}
			}
		}

		T getDistanceFromPointOnInfiniteLine(const Vector3T<T>& point) const noexcept
		{
			Vector3T<T> dir = point2 - point1;
			T f = (point - point1).dot(dir);
			Vector3T<T> proj = point1 + f * dir;
			return proj.getLength(point);
		}

		void transform(const Matrix4T<T>& mat) noexcept
		{
			point1 = mat.transformPosition(point1);
			point2 = mat.transformPosition(point2);
		}

	public:
		template <class O>
		Line3T<T>& operator=(const Line3T<O>& other) noexcept
		{
			point1 = other.point1;
			point2 = other.point2;
			return *this;
		}

	};

	typedef Line3T<sl_real> Line3;
	typedef Line3T<float> Line3f;
	typedef Line3T<double> Line3lf;

}

#endif
