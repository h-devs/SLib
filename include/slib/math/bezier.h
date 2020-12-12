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

/*******************************
		CubicBezierCurve

P(t)	= B(3,0)*P0 + B(3,1)*P1 + B(3,2)*P2 + B(3,3)*P3
		= (1-t)^3 * P0 + 3*(1-t)^2*t * P1 + 3*(1-t)*t^2 * P2 + t^3 * P3
0 <= t <= 1

B(n,m)	= m th coefficient of nth degree Bernstein polynomial
		= C(n,m) * t^(m) * (1 - t)^(n-m)
C(n,m)	= Combinations of n things, taken m at a time
		= n! / (m! * (n-m)!)
*******************************/

#ifndef CHECKHEADER_SLIB_MATH_BEZIER
#define CHECKHEADER_SLIB_MATH_BEZIER

#include "definition.h"

#include "point.h"
#include "rectangle.h"

namespace slib
{
	
	template <class T>
	class SLIB_EXPORT CubicBezierCurveT
	{
	public:
		T x0, y0;
		T x1, y1;
		T x2, y2;
		T x3, y3;

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(CubicBezierCurveT)
		
		CubicBezierCurveT() = default;

		template <class O>
		CubicBezierCurveT(const CubicBezierCurveT<O>& other) noexcept
			:x0((T)(other.x0)), y0((T)(other.y0)),
			x1((T)(other.x1)), y1((T)(other.y1)),
			x2((T)(other.x2)), y2((T)(other.y2)),
			x3((T)(other.x3)), y3((T)(other.y3))
		{
		}

		CubicBezierCurveT(T _x0, T _y0, T _x1, T _y1, T _x2, T _y2, T _x3, T _y3) noexcept
			:x0(_x0), y0(_y0),
			x1(_x1), y1(_y1),
			x2(_x2), y2(_y2),
			x3(_x3), y3(_y3)
		{}

		CubicBezierCurveT(const PointT<T>& P0, const PointT<T>& P1, const PointT<T>& P2, const PointT<T>& P3) noexcept
			:x0(P0.x), y0(P0.y),
			x1(P1.x), y1(P1.y),
			x2(P2.x), y2(P2.y),
			x3(P3.x), y3(P3.y)
		{}

	public:
		void getPoint(T t, T& x, T& y) const noexcept
		{
			T it = 1 - t;
			T it2 = it * it;
			T it3 = it2 * it;
			T t2 = t * t;
			T t3 = t2 * t;
			T _3it2t = 3 * it2 * t;
			T _3itt2 = 3 * it * t2;
			x = it3 * x0 + _3it2t * x1 + _3itt2 * x2 + t3 * x3;
			y = it3 * y0 + _3it2t * y1 + _3itt2 * y2 + t3 * y3;
		}
	
		void getPoint(T t, PointT<T>& pt) const noexcept
		{
			getPoint(t, pt.x, pt.y);
		}
	
		PointT<T> getPoint(T t) const noexcept
		{
			PointT<T> ret;
			getPoint(t, ret.x, ret.y);
			return ret;
		}

		// returns 1 + 3 * NumberOfArcSections points
		static sl_uint32 convertArcToBezier(PointT<T> pts[13], const RectangleT<T>& rc, T startDegrees, T sweepDegrees) noexcept
		{
			T PI2 = Math::HalfPI<T>();
			T _2PI = Math::DualPI<T>();

			T width = rc.getWidth();
			T height = rc.getHeight();
			T radiusX = width / 2;
			T radiusY = height / 2;
			PointT<T> center = rc.getCenter();

			if (sweepDegrees > 360) {
				sweepDegrees = 360;
			}
			if (sweepDegrees < -360) {
				sweepDegrees = -360;
			}
			T _startAngle = Math::getRadianFromDegrees(startDegrees);
			T _sweepAngle = Math::getRadianFromDegrees(sweepDegrees);

			T startAngle = Math::convertAngleFromEllipseToCircle(_startAngle, radiusX, radiusY);
			T endAngle = Math::convertAngleFromEllipseToCircle(_startAngle + _sweepAngle, radiusX, radiusY);
			sl_bool flagNegative;
			if (sweepDegrees < 0) {
				flagNegative = sl_true;
				if (endAngle > startAngle) {
					endAngle -= _2PI;
				}
			} else {
				flagNegative = sl_false;
				if (endAngle < startAngle) {
					endAngle += _2PI;
				}
			}
			sl_int32 nPts = (sl_int32)(Math::ceil(Math::abs(endAngle - startAngle) / PI2));
			if (nPts > 4) {
				nPts = 4;
			}
			nPts *= 3;
			T s = startAngle;
			T e = startAngle;
			CubicBezierCurveT<T> c;
			for (sl_int32 i = 0; i < nPts; i += 3) {
				if (flagNegative) {
					e -= PI2;
				} else {
					e += PI2;
				}
				if (i == nPts - 3) {
					e = endAngle;
				}
				c.describeArc(center.x, center.y, radiusX, radiusY, s, e);
				if (i == 0) {
					pts[i].x = c.x0;
					pts[i].y = c.y0;
				}
				pts[i + 1].x = c.x1;
				pts[i + 1].y = c.y1;
				pts[i + 2].x = c.x2;
				pts[i + 2].y = c.y2;
				pts[i + 3].x = c.x3;
				pts[i + 3].y = c.y3;
				if (flagNegative) {
					s -= PI2;
				} else {
					s += PI2;
				}
			}
			if (nPts == 0) {
				return 0;
			} else {
				return nPts + 1;
			}
		}
	
		void describeArc(T cx, T cy, T rx, T ry, T startRadian, T endRadian) noexcept
		{
			T cos1 = Math::cos(startRadian);
			T sin1 = Math::sin(startRadian);
			T cos2 = Math::cos(endRadian);
			T sin2 = Math::sin(endRadian);
			T m = (endRadian - startRadian) / 2;
			T f = (1 - Math::cos(m)) / Math::sin(m) * 4 / 3;
			x0 = cx + rx * cos1;
			y0 = cy + ry * sin1;
			x1 = x0 - f * sin1 * rx;
			y1 = y0 + f * cos1 * ry;
			x3 = cx + rx * cos2;
			y3 = cy + ry * sin2;
			x2 = x3 + f * sin2 * rx;
			y2 = y3 - f * cos2 * ry;
		}

	public:
		template <class O>
		CubicBezierCurveT<T>& operator=(const CubicBezierCurveT<O>& other) noexcept
		{
			x0 = (T)(other.x0);
			y0 = (T)(other.y0);
			x1 = (T)(other.x1);
			y1 = (T)(other.y1);
			x2 = (T)(other.x2);
			y2 = (T)(other.y2);
			x3 = (T)(other.x3);
			y3 = (T)(other.y3);
			return *this;
		}

	};
	
	typedef CubicBezierCurveT<sl_real> CubicBezierCurve;
	typedef CubicBezierCurveT<float> CubicBezierCurvef;
	typedef CubicBezierCurveT<double> CubicBezierCurvelf;

}

#endif
