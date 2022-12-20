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

		void describeArc(T cx, T cy, T rx, T ry, T startRadian, T endRadian, T rotation) noexcept
		{
			T cos1 = Math::cos(startRadian);
			T sin1 = Math::sin(startRadian);
			T cos2 = Math::cos(endRadian);
			T sin2 = Math::sin(endRadian);
			T cos_r = Math::cos(rotation);
			T sin_r = Math::sin(rotation);
			T m = (endRadian - startRadian) / 2;
			T f = (1 - Math::cos(m)) / Math::sin(m) * 4 / 3;
			x0 = cx + rx * cos1 * cos_r - ry * sin1 * sin_r;
			y0 = cy + ry * sin1 * cos_r + rx * cos1 * sin_r;
			x1 = x0 - f * sin1 * rx * cos_r - f * cos1 * ry * sin_r;
			y1 = y0 + f * cos1 * ry * cos_r - f * sin1 * rx * sin_r;
			x3 = cx + rx * cos2 * cos_r - ry * sin2 * sin_r;
			y3 = cy + ry * sin2 * cos_r + rx * cos2 * sin_r;
			x2 = x3 + f * sin2 * rx * cos_r + f * cos2 * ry * sin_r;
			y2 = y3 - f * cos2 * ry * cos_r + f * sin2 * rx * sin_r;
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

		static sl_uint32 convertArcToBezier(PointT<T> pts[13], T x1, T y1, T x2, T y2, T rx, T ry, sl_bool flagLargeArc, sl_bool flagSweep)
		{
			T PI2 = Math::HalfPI<T>();
			T _2PI = Math::DualPI<T>();
			T EPSILON = Math::Epsilon<T>();

			PointT<T> p1(x1, y1), p2(x2, y2);
			PointT<T> q2 = p2 - p1;
			T radiusCheck = (q2.x * q2.x) / (rx * rx) + (q2.y * q2.y) / (ry * ry);
			if (radiusCheck > T(4)) {
				rx *= Math::sqrt(radiusCheck);
				ry *= Math::sqrt(radiusCheck);
			}
			q2.x /= rx;
			q2.y /= ry;
			T h = q2.getLength() / T(2);
			T d = Math::sqrt(T(1) - h * h);
			PointT<T> center;
			if (!flagLargeArc != !flagSweep) {
				center = (q2 + PointT<T>(-q2.y, q2.x) * (d / h)) / T(2);
			} else {
				center = (q2 + PointT<T>(q2.y, -q2.x) * (d / h)) / T(2);
			}
			center.x = p1.x + center.x * rx;
			center.y = p1.y + center.y * ry;
			T sign1 = (p1.y < center.y) ? T(-1) : T(1);
			T sign2 = (p2.y < center.y) ? T(-1) : T(1);
			T startAngle = sign1 * Math::arccos(((p1.x - center.x) / rx) > 0 ? ((p1.x - center.x) / rx) - EPSILON : ((p1.x - center.x) / rx) + EPSILON);			//  -pi ~ pi
			T endAngle = sign2 * Math::arccos(((p2.x - center.x) / rx) > 0 ? ((p2.x - center.x) / rx) - EPSILON : ((p2.x - center.x) / rx) + EPSILON);				//	-pi ~ pi
			T sign;
			if (flagSweep) {
				sign = T(1);
				if (endAngle < startAngle && !(endAngle < 0 && startAngle < 0)) {
					endAngle += _2PI;
				}
			} else {
				sign = T(-1);
				if (endAngle > startAngle) {
					endAngle -= _2PI;
				}
			}
			sl_int32 nPts = (sl_int32)(Math::ceil(Math::abs(endAngle - startAngle) / PI2));
			if (nPts > 4) {
				nPts = 4;
			}
			nPts *= 3;
			T s = startAngle, e = startAngle;
			CubicBezierCurveT<T> c;
			for (sl_int32 i = 0; i < nPts; i += 3) {
				e += sign * PI2;
				if (i == nPts - 3) {
					e = endAngle;
				}
				c.describeArc(center.x, center.y, rx, ry, s, e);
				if (!i) {
					pts[i].x = c.x0;
					pts[i].y = c.y0;
				}
				pts[i + 1].x = c.x1;
				pts[i + 1].y = c.y1;
				pts[i + 2].x = c.x2;
				pts[i + 2].y = c.y2;
				pts[i + 3].x = c.x3;
				pts[i + 3].y = c.y3;
				s += sign * PI2;
			}
			if (!nPts) {
				return 0;
			} else {
				return nPts + 1;
			}
		}

		static sl_uint32 convertArcToBezier(PointT<T> pts[13], T x1, T y1, T x2, T y2, T rx, T ry, T rotation, sl_bool flagLargeArc, sl_bool flagSweep)
		{
			T PI2 = Math::HalfPI<T>();
			T _2PI = Math::DualPI<T>();

			// Calculate the middle point between the current and the final points
			T dx2 = (x1 - x2) / 2;
			T dy2 = (y1 - y2) / 2;
			T cos_r = Math::cos(rotation);
			T sin_r = Math::sin(rotation);

			// Calculate (x3, y3)
			T x3 =  cos_r * dx2 + sin_r * dy2;
			T y3 = -sin_r * dx2 + cos_r * dy2;

			// Ensure radii are large enough
			T prx = rx * rx;
			T pry = ry * ry;
			T px = x3 * x3;
			T py = y3 * y3;

			// Check that radii are large enough
			T radiusCheck = px / prx + py / pry;
			if (radiusCheck > 1.0) {
				rx *= Math::sqrt(radiusCheck);
				ry *= Math::sqrt(radiusCheck);
			}

			// Calculate (cx1, cy1)
			T sign = (!flagLargeArc == !flagSweep) ? (T)-1.0 : (T)1.0;
			T sq = (prx * pry - prx * py - pry * px) / (prx * py + pry * px);
			T coef = sign * Math::sqrt((sq < 0) ? 0 : sq);
			T cx1 = coef *  ((rx * y3) / ry);
			T cy1 = coef * -((ry * x3) / rx);

			// Calculate (cx, cy) from (cx1, cy1)
			T sx2 = (x1 + x2) / 2;
			T sy2 = (y1 + y2) / 2;
			T cx = sx2 + (cos_r * cx1 - sin_r * cy1);
			T cy = sy2 + (sin_r * cx1 + cos_r * cy1);

			// Calculate the startAngle (angle1) and the sweep_angle (dangle)
			T ux = (x3 - cx1) / rx;
			T uy = (y3 - cy1) / ry;
			T vx = (-x3 - cx1) / rx;
			T vy = (-y3 - cy1) / ry;
			T p, n;

			// Calculate the angle start
			n = Math::sqrt(ux * ux + uy * uy);
			p = ux; // (1 * ux) + (0 * uy)
			sign = (uy < 0) ? (T)-1.0 : (T)1.0;
			T v = p / n;
			if (v < -1) {
				v = -1;
			} else if (v > 1) {
				v = 1;
			}
			T startAngle = sign * Math::arccos(v);

			// Calculate the sweep angle
			n = Math::sqrt((ux * ux + uy * uy) * (vx * vx + vy * vy));
			p = ux * vx + uy * vy;
			sign = (ux * vy - uy * vx < 0) ? T(-1) : T(1);
			v = p / n;
			if (v < T(-1)) {
				v = T(-1);
			} else if (v > T(1)) {
				v = T(1);
			}
			T sweepAngle = sign * Math::arccos(v);
			if (!flagSweep && sweepAngle > 0) {
				sweepAngle -= _2PI;
			} else if (flagSweep && sweepAngle < 0) {
				sweepAngle += _2PI;
			}
			T endAngle = startAngle + sweepAngle;

			sl_bool flagNegative;
			if (sweepAngle < 0) {
				flagNegative = sl_true;
				if (endAngle > startAngle) {
					endAngle -= PI2;
				}
			} else {
				flagNegative = sl_false;
				if (endAngle < startAngle) {
					endAngle += PI2;
				}
			}
			sl_int32 nPts = (sl_int32)(Math::ceil(Math::abs(sweepAngle) / PI2));
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
				c.describeArc(cx, cy, rx, ry, s, e, rotation);
				if (!i) {
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
			if (!nPts) {
				return 0;
			} else {
				return nPts + 1;
			}
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
