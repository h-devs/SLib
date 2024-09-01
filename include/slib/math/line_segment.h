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

#ifndef CHECKHEADER_SLIB_MATH_LINE_SEGMENT
#define CHECKHEADER_SLIB_MATH_LINE_SEGMENT

#include "point.h"
#include "matrix3.h"

namespace slib
{

	template <class T>
	class SLIB_EXPORT LineSegmentT
	{
	public:
		PointT<T> point1;
		PointT<T> point2;

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(LineSegmentT)

		LineSegmentT() = default;

		template <class O>
		LineSegmentT(const LineSegmentT<O>& other): point1(other.point1), point2(other.point2) {}

		LineSegmentT(const PointT<T>& _point1, const PointT<T>& _point2): point1(_point1), point2(_point2)
		{
			point1 = _point1;
			point2 = _point2;
		}

		LineSegmentT(T x1, T y1, T x2, T y2): point1(x1, y1), point2(x2, y2) {}

	public:
		SLIB_CONSTEXPR Vector2T<T> getDirection() const
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

		void transform(const Matrix3T<T>& mat) noexcept
		{
			point1 = mat.transformPosition(point1);
			point2 = mat.transformPosition(point2);
		}

		PointT<T> projectPoint(const PointT<T>& point) const noexcept
		{
			Vector2T<T> dir = point2 - point1;
			return point1 + (point - point1).dot(dir) * dir;
		}

		T getDistanceFromPoint(const PointT<T>& point) const noexcept
		{
			Vector2T<T> dir = point2 - point1;
			T f = (point - point1).dot(dir);
			Vector2T<T> proj = point1 + f * dir;
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

		T getDistanceFromPointOnInfiniteLine(const PointT<T>& point) const noexcept
		{
			Vector2T<T> dir = point2 - point1;
			T f = (point - point1).dot(dir);
			Vector2T<T> proj = point1 + f * dir;
			return proj.getLength(point);
		}

		sl_bool intersect(const LineSegmentT& other, PointT<T>* _out = sl_null) const
		{
			Vector2T<T> d1 = point1 - point2;
			Vector2T<T> d2 = other.point1 - other.point2;
			T divider = d1.x * d2.y - d1.y * d2.x;
			if (Math::isAlmostZero(divider)) {
				return sl_false;
			}
			Vector2T<T> v = point1 - other.point1;
			T t = (v.x * d2.y - v.y * d2.x) / divider;
			T u = (v.x * d1.y - v.y * d1.x) / divider;
			if ((0 <= t && t <= (T)1) && (0 <= u && u <= (T)1)) {
				if (_out) {
					_out->x = point1.x - t * d1.x;
					_out->y = point1.y - t * d1.y;
				}
				return sl_true;
			}
			return sl_false;
		}

	public:
		template <class O>
		LineSegmentT<T>& operator=(const LineSegmentT<O>& other) noexcept
		{
			point1 = other.point1;
			point2 = other.point2;
			return *this;
		}

	};

	typedef LineSegmentT<sl_real> LineSegment;

}

#endif
