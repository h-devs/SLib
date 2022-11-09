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
		SLIB_CONSTEXPR CubicBezierCurveT(const CubicBezierCurveT<O>& other):
		 x0((T)(other.x0)), y0((T)(other.y0)),
		 x1((T)(other.x1)), y1((T)(other.y1)),
		 x2((T)(other.x2)), y2((T)(other.y2)),
		 x3((T)(other.x3)), y3((T)(other.y3)) {}

		SLIB_CONSTEXPR CubicBezierCurveT(T _x0, T _y0, T _x1, T _y1, T _x2, T _y2, T _x3, T _y3):
		 x0(_x0), y0(_y0),
		 x1(_x1), y1(_y1),
		 x2(_x2), y2(_y2),
		 x3(_x3), y3(_y3) {}

		SLIB_CONSTEXPR CubicBezierCurveT(const PointT<T>& P0, const PointT<T>& P1, const PointT<T>& P2, const PointT<T>& P3):
		 x0(P0.x), y0(P0.y),
		 x1(P1.x), y1(P1.y),
		 x2(P2.x), y2(P2.y),
		 x3(P3.x), y3(P3.y) {}

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

		static sl_uint32 convertArcToBezier(PointT<T> pts[13], T x0, T y0, T rx, T ry, sl_bool large_arc_flag, sl_bool sweep_flag, T x2, T y2)
		{
			PointT<T> p1(x0, y0), p2(x2, y2), center;
			T pi = Math::PI<T>();
			T epsilon = Math::Epsilon<T>();

			PointT<T> q2 = p2 - p1;
			T radius_check = (q2.x / 2 * q2.x / 2) / (rx * rx) + (q2.y / 2 * q2.y / 2) / (ry * ry);
			if (radius_check > 1.0) {
				rx *= Math::sqrt(radius_check);
				ry *= Math::sqrt(radius_check);
			}
			q2.x /= rx;
			q2.y /= ry;
			T h = q2.getLength() / 2;
			
			T d = Math::sqrt(1 - h * h);
			if (large_arc_flag != sweep_flag) {
				PointT<T> n(-q2.y, q2.x);
				center = q2 / 2 + n * (d / h / 2);
				center.x = p1.x + center.x * rx;
				center.y = p1.y + center.y * ry;
			}
			else {
				PointT<T> n(q2.y, -q2.x);
				center = q2 / 2 + n * (d / h / 2);
				center.x = p1.x + center.x * rx;
				center.y = p1.y + center.y * ry;
			}

			T sign1 = T((p1.y < center.y) ? -1.0 : 1.0);
			T sign2 = T((p2.y < center.y) ? -1.0 : 1.0);
			T start_angle = sign1 * Math::arccos(((p1.x - center.x) / rx) > 0 ? ((p1.x - center.x) / rx) - epsilon : ((p1.x - center.x) / rx) + epsilon);			//  -pi ~ pi
			T end_angle = sign2 * Math::arccos(((p2.x - center.x) / rx) > 0 ? ((p2.x - center.x) / rx) - epsilon : ((p2.x - center.x) / rx) + epsilon);				//	-pi ~ pi
			T sign;
			if (sweep_flag) {
				sign = T(1.0);
				if (end_angle < start_angle && !(end_angle < 0 && start_angle < 0)) {
					end_angle += 2 * pi;
				}
			}
			else {
				sign = T(-1.0);
				if (end_angle > start_angle) {
					end_angle -= 2 * pi;
				}
			}

			sl_int32 nPts = (sl_int32)(Math::ceil(Math::abs(end_angle - start_angle) / (pi / 2)));
			if (nPts > 4) {
				nPts = 4;
			}
			nPts *= 3;

			T s = start_angle, e = start_angle;
			CubicBezierCurveT<T> c;
			for (sl_int32 i = 0; i < nPts; i += 3) {
				e += sign * (pi / 2);
				if (i == nPts - 3) {
					e = end_angle;
				}
				c.describeArc(center.x, center.y, rx, ry, s, e);
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
				s += sign * (pi / 2);
			}

			if (nPts == 0) {
				return 0;
			}
			else {
				return nPts + 1;
			}
		}

		static sl_uint32 convertArcToBezier(PointT<T> pts[13], T x0, T y0, T rx, T ry, T rotation, sl_bool large_arc_flag, sl_bool sweep_flag, T x2, T y2)
		{
			sl_uint32 n = convertArcToBezier(pts, x0, y0, rx, ry, large_arc_flag, sweep_flag, x2, y2);

			// Apply Rotation
			T cos_r = Math::cos(rotation);
			T sin_r = Math::sin(rotation);
			for (sl_uint32 i = 0; i < n; i++) {
				auto x = pts[i].x;
				auto y = pts[i].y;
				pts[i].x = cos_r * x - sin_r * y;
				pts[i].y = sin_r * x + cos_r * y;
			}

			return n;
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
