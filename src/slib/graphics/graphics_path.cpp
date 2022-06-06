/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/graphics/path.h"

#include "slib/math/bezier.h"

namespace slib
{

	namespace priv
	{
		namespace graphics_path
		{

			SLIB_INLINE static sl_bool IsCubicControl2AndEnd(GraphicsPathPoint* ptControl2, sl_size n)
			{
				if (n >= 2) {
					return ptControl2[0].type == GraphicsPathPoint::CubicTo && ptControl2[1].type == GraphicsPathPoint::CubicTo;
				}
				return sl_false;
			}

			static sl_real GetBounds_GetCubicPeak(sl_real q1, sl_real q2, sl_real q3, sl_real q4, sl_real epsillon)
			{
				// for a peak to exist above 0, the cubic segment must have at least one of its control off-points above 0
				while (q2 > 0 || q3 > 0) {
					// determine which half contains the maximum and split
					if (q1 + q2 > q3 + q4) {
						// first half
						q4 = q4 + q3;
						q3 = q3 + q2;
						q2 = q2 + q1;
						q4 = q4 + q3;
						q3 = q3 + q2;
						q4 = (q4 + q3) / 8;
						q3 = q3 / 4;
						q2 = q2 / 2;
					} else {
						// second half
						q1 = q1 + q2;
						q2 = q2 + q3;
						q3 = q3 + q4;
						q1 = q1 + q2;
						q2 = q2 + q3;
						q1 = (q1 + q2) / 8;
						q2 = q2 / 4;
						q3 = q3 / 2;
					}
					// check whether either end reached the maximum */
					if (Math::abs(q1 - q2) <= epsillon && q1 >= q3) {
						return q1;
					}
					if (Math::abs(q3 - q4) <= epsillon && q2 <= q4) {
						return q4;
					}
				}
				return 0;
			}

			static void GetBounds_ProcessCubic(sl_real p1, sl_real p2, sl_real p3, sl_real p4, sl_real epsillon, sl_real& min, sl_real& max)
			{
				if (p2 > max || p3 > max) {
					max += GetBounds_GetCubicPeak(p1 - max, p2 - max, p3 - max, p4 - max, epsillon);
				}
				// now flip the signs to update the minimum
				if (p2 < min || p3 < min) {
					min -= GetBounds_GetCubicPeak(min - p1, min - p2, min - p3, min - p4, epsillon);
				}
			}

			static Rectangle GetBounds(GraphicsPathPoint* pts, sl_size n)
			{
				if (!n) {
					return Rectangle::zero();
				}

				Rectangle cbox;
				Rectangle bbox;

				sl_size i;
				for (i = 0; i < n; i++) {
					GraphicsPathPoint& pt = pts[i];
					if (i) {
						cbox.mergePoint(pt);
						if (pt.type == GraphicsPathPoint::LineTo) {
							bbox.mergePoint(pt);
						}
					} else {
						cbox.setFromPoint(pt);
						bbox.setFromPoint(pt);
					}
				}

#define BBOX_CHECK_X(pt, bbox) (pt.x < bbox.left || pt.x > bbox.right)
#define BBOX_CHECK_Y(pt, bbox) (pt.y < bbox.top || pt.y > bbox.bottom)

				if (cbox.left < bbox.left || cbox.right > bbox.right || cbox.top < bbox.top || cbox.bottom > bbox.bottom) {
					sl_real epsilon = Math::max(cbox.getWidth(), cbox.getHeight()) / 1000.0f;
					if (epsilon < SLIB_EPSILON) {
						epsilon = SLIB_EPSILON;
					}
					Point last = *pts;
					for (i = 0; i < n; i++) {
						GraphicsPathPoint& pt = pts[i];
						switch (pt.type) {
							case GraphicsPathPoint::MoveTo:
								bbox.mergePoint(pt);
								last = pt;
								break;
							case GraphicsPathPoint::LineTo:
								// bbox already contains both explicit ends of the line segment
								last = pt;
								break;
							case GraphicsPathPoint::CubicTo:
								if (IsCubicControl2AndEnd(pts + i + 1, n - i - 1)) {
									Point& control1 = pts[i];
									Point& control2 = pts[i + 1];
									Point& to = pts[i + 2];
									if (BBOX_CHECK_X(control1, bbox) || BBOX_CHECK_X(control2, bbox)) {
										GetBounds_ProcessCubic(last.x, control1.x, control2.x, to.x, epsilon, bbox.left, bbox.right);
									}
									if (BBOX_CHECK_Y(control1, bbox) || BBOX_CHECK_Y(control2, bbox)) {
										GetBounds_ProcessCubic(last.y, control1.y, control2.y, to.y, epsilon, bbox.top, bbox.bottom);
									}
									last = to;
									i += 2;
								}
								break;
							default:
								break;
						}
					}
				}
				return bbox;
			}

			static Rectangle GetControlBounds(GraphicsPathPoint* pts, sl_size n)
			{
				if (!n) {
					return Rectangle::zero();
				}
				Rectangle cbox;
				for (sl_size i = 0; i < n; i++) {
					GraphicsPathPoint& pt = pts[i];
					if (i) {
						cbox.mergePoint(pt);
					} else {
						cbox.setFromPoint(pt);
					}
				}
				return cbox;
			}

			class ContainsPoint
			{
			public:
				sl_real x;
				sl_real y;
				sl_real tolerance2;
				sl_real epsilon;

				sl_bool flagOnEdge = sl_false;
				sl_int32 winding = 0;

				Point first;
				Point current = { 0, 0 };
				sl_bool hasCurPoint = sl_false;

			public:
				SLIB_INLINE sl_int32 getCompareResult(sl_real c)
				{
					if (c > epsilon) {
						return 1;
					} else if (c < -epsilon) {
						return -1;
					} else {
						return 0;
					}
				}

				sl_int32 compareEdgeForYAgainstX(const Point& p1, const Point& p2, sl_real y, sl_real x)
				{
					sl_real adx = p2.x - p1.x;
					sl_real dx = x - p1.x;
					if (Math::abs(adx) < epsilon) {
						return getCompareResult(-dx);
					}
					if ((adx > 0 && dx < 0) || (adx < 0 && dx >= 0)) {
						return getCompareResult(adx);
					}
					sl_real dy = y - p1.y;
					sl_real ady = p2.y - p1.y;
					sl_real L = dy * adx;
					sl_real R = dx * ady;
					return getCompareResult(L - R);
				}

				void addEdge(const Point* p1, const Point* p2)
				{
					if (flagOnEdge) {
						return;
					}
					// count the number of edge crossing to -infinite
					sl_int32 dir = 1;
					if (p2->y < p1->y) {
						const Point* tmp = p1;
						p1 = p2;
						p2 = tmp;
						dir = -1;
					}
					// First check whether the query is on an edge
					if (
						(p1->x == x && p1->y == y) ||
						(p2->x == x && p2->y == y) ||
						(
							!(
								p2->y < y || p1->y > y ||
								(p1->x > x && p2->x > x) ||
								(p1->x < x && p2->x < x)
							) &&
							!compareEdgeForYAgainstX(*p1, *p2, y, x)
						)
					) {
						flagOnEdge = sl_true;
						return;
					}
					// edge is entirely above or below, note the shortening rule
					if (p2->y <= y || p1->y > y) {
						return;
					}
					// edge lies wholly to the right
					if (p1->x >= x && p2->x >= x) {
						return;
					}
					if ((p1->x <= x && p2->x <= x) || compareEdgeForYAgainstX(*p1, *p2, y, x) < 0) {
						winding += dir;
					}
				}

				void moveTo(const Point& pt)
				{
					if (hasCurPoint) {
						addEdge(&current, &first);
					}
					first = pt;
					current = pt;
					hasCurPoint = sl_true;
				}

				void lineTo(const Point& pt)
				{
					if (hasCurPoint) {
						addEdge(&current, &pt);
					}
					current = pt;
					hasCurPoint = sl_true;
				}

				void processSpline(const CubicBezierCurve& curve, const Point& P1, sl_real t1, const Point& P2, sl_real t2)
				{
					if (P2.getLength2p(P1) <= tolerance2) {
						lineTo(P2);
						return;
					}
					sl_real tc = (t1 + t2) / 2.0f;
					Point Pc = curve.getPoint(tc);
					processSpline(curve, P1, t1, Pc, tc);
					processSpline(curve, Pc, tc, P2, t2);
				}

				void curveTo(const Point& b, const Point& c, const Point& d)
				{
					sl_real top, bottom, left;
					// first reject based on bbox
					bottom = top = current.y;
					if (b.y < top) top = b.y;
					if (b.y > bottom) bottom = b.y;
					if (c.y < top) top = c.y;
					if (c.y > bottom) bottom = c.y;
					if (d.y < top) top = d.y;
					if (d.y > bottom) bottom = d.y;
					if (bottom < y || top > y) {
						current = d;
						return;
					}
					left = current.x;
					if (b.x < left) left = b.x;
					if (c.x < left) left = c.x;
					if (d.x < left) left = d.x;
					if (left > x) {
						current = d;
						return;
					}
					const Point& a = current;
					// If both tangents are zero, this is just a straight line
					if (Math::isAlmostZero(a.x - b.x) && Math::isAlmostZero(a.y - b.y) && Math::isAlmostZero(c.x - d.x) && Math::isAlmostZero(c.y - d.y)) {
						return;
					}
					CubicBezierCurve curve(a, b, c, d);
					processSpline(curve, a, 0.0f, d, 1.0f);
				}

				sl_bool run(GraphicsPathPoint* pts, sl_size n, FillMode fillMode)
				{
					for (sl_size i = 0; i < n; i++) {
						GraphicsPathPoint& pt = pts[i];
						switch (pt.type) {
							case GraphicsPathPoint::MoveTo:
								moveTo(pt);
								break;
							case GraphicsPathPoint::LineTo:
								lineTo(pt);
								break;
							case GraphicsPathPoint::CubicTo:
								if (IsCubicControl2AndEnd(pts + i + 1, n - i - 1)) {
									curveTo(pts[i], pts[i + 1], pts[i + 2]);
									i += 2;
								}
								break;
							default:
								break;
						}
						if (pts[i].flagClose) {
							if (hasCurPoint) {
								addEdge(&current, &first);
								hasCurPoint = sl_false;
							}
						}
					}
					if (flagOnEdge) {
						return sl_true;
					}
					if (fillMode == FillMode::Winding) {
						return winding != 0;
					} else {
						return (sl_bool)(winding & 1);
					}
					return sl_false;
				}

				static sl_bool run(GraphicsPathPoint* pts, sl_size n, FillMode mode, sl_real x, sl_real y)
				{
					if (!n) {
						return sl_false;
					}
					Rectangle cbox = GetControlBounds(pts, n);
					ContainsPoint context;
					context.x = x;
					context.y = y;
					sl_real t = Math::max(cbox.getWidth(), cbox.getHeight()) / 1000.0f;
					context.epsilon = t / 2.0f;
					context.tolerance2 = t * t;
					if (context.epsilon < SLIB_EPSILON || context.tolerance2 < SLIB_EPSILON) {
						return sl_false;
					}
					return context.run(pts, n, mode);
				}

			};

		}
	}

	using namespace priv::graphics_path;

	SLIB_DEFINE_ROOT_OBJECT(GraphicsPath)

	GraphicsPath::GraphicsPath()
	{
		m_fillMode = FillMode::Winding;
	}

	GraphicsPath::~GraphicsPath()
	{
	}

	Ref<GraphicsPath> GraphicsPath::create()
	{
		return new GraphicsPath;
	}

	sl_size GraphicsPath::getPointsCount()
	{
		return m_points.getCount();
	}

	GraphicsPathPoint* GraphicsPath::getPoints()
	{
		return m_points.getData();
	}

	sl_bool GraphicsPath::getPointAt(sl_size index, GraphicsPathPoint* _out)
	{
		return m_points.getAt_NoLock(index, _out);
	}

	GraphicsPathPoint GraphicsPath::getPointAt(sl_size index)
	{
		return m_points.getValueAt_NoLock(index);
	}

	sl_bool GraphicsPath::getFirstPoint(GraphicsPathPoint* _out)
	{
		return m_points.getFirst_NoLock(_out);
	}

	GraphicsPathPoint GraphicsPath::getFirstPoint()
	{
		return m_points.getFirstValue_NoLock();
	}

	sl_bool GraphicsPath::getLastPoint(GraphicsPathPoint* _out)
	{
		return m_points.getLast_NoLock(_out);
	}

	GraphicsPathPoint GraphicsPath::getLastPoint()
	{
		return m_points.getLastValue_NoLock();
	}

	SpinLock* GraphicsPath::getLock()
	{
		return &m_lock;
	}

	sl_bool GraphicsPath::_checkBegin()
	{
		sl_size n = m_points.getCount();
		if (!n) {
			return sl_false;
		}
		GraphicsPathPoint& pt = (m_points.getData())[n - 1];
		if (pt.flagClose) {
			return sl_false;
		}
		return sl_true;
	}

	void GraphicsPath::_addPoint(sl_real x, sl_real y, sl_uint8 type, sl_bool flagClose)
	{
		GraphicsPathPoint pt;
		pt.x = x;
		pt.y = y;
		pt.type = type;
		pt.flagClose = (sl_uint8)flagClose;
		m_points.add_NoLock(pt);
	}

	void GraphicsPath::moveTo(sl_real x, sl_real y)
	{
		do {
			sl_size n = m_points.getCount();
			if (n) {
				GraphicsPathPoint& pt = (m_points.getData())[n - 1];
				if (pt.type == GraphicsPathPoint::MoveTo) {
					pt.x = x;
					pt.y = y;
					break;
				}
			}
			_addPoint(x, y, GraphicsPathPoint::MoveTo);
		} while (0);
		Referable* po = m_platformObject.get();
		if (po) {
			_moveTo_PO(po, x, y);
		}
	}

	void GraphicsPath::moveTo(const Point& pt)
	{
		moveTo(pt.x, pt.y);
	}

	void GraphicsPath::lineTo(sl_real x, sl_real y)
	{
		if (!(_checkBegin())) {
			return;
		}
		_addPoint(x, y, GraphicsPathPoint::LineTo);
		Referable* po = m_platformObject.get();
		if (po) {
			_lineTo_PO(po, x, y);
		}
	}

	void GraphicsPath::lineTo(const Point& pt)
	{
		lineTo(pt.x, pt.y);
	}

	void GraphicsPath::conicTo(sl_real xc, sl_real yc, sl_real xe, sl_real ye)
	{
		sl_size n = m_points.getCount();
		if (!n) {
			return;
		}
		GraphicsPathPoint& last = (m_points.getData())[n - 1];
		if (last.flagClose) {
			return;
		}
		sl_real xc1 = xc - (xc - last.x) / 3.0f;
		sl_real yc1 = yc - (yc - last.y) / 3.0f;
		sl_real xc2 = xc + (xe - xc) / 3.0f;
		sl_real yc2 = yc + (ye - yc) / 3.0f;
		_addPoint(xc1, yc1, GraphicsPathPoint::CubicTo);
		_addPoint(xc2, yc2, GraphicsPathPoint::CubicTo);
		_addPoint(xe, ye, GraphicsPathPoint::CubicTo);
		Referable* po = m_platformObject.get();
		if (po) {
			_cubicTo_PO(po, xc1, yc1, xc2, yc2, xe, ye);
		}
	}

	void GraphicsPath::conicTo(const Point& ptControl, const Point& ptEnd)
	{
		conicTo(ptControl.x, ptControl.y, ptEnd.x, ptEnd.y);
	}

	void GraphicsPath::cubicTo(sl_real xc1, sl_real yc1, sl_real xc2, sl_real yc2, sl_real xe, sl_real ye)
	{
		if (!(_checkBegin())) {
			return;
		}
		_addPoint(xc1, yc1, GraphicsPathPoint::CubicTo);
		_addPoint(xc2, yc2, GraphicsPathPoint::CubicTo);
		_addPoint(xe, ye, GraphicsPathPoint::CubicTo);
		Referable* po = m_platformObject.get();
		if (po) {
			_cubicTo_PO(po, xc1, yc1, xc2, yc2, xe, ye);
		}
	}

	void GraphicsPath::cubicTo(const Point& ptControl1, const Point& ptControl2, const Point& ptEnd)
	{
		cubicTo(ptControl1.x, ptControl1.y, ptControl2.x, ptControl2.y, ptEnd.x, ptEnd.y);
	}

	void GraphicsPath::closeSubpath()
	{
		sl_size n = m_points.getCount();
		if (!n) {
			return;
		}
		GraphicsPathPoint& pt = (m_points.getData())[n - 1];
		if (pt.flagClose) {
			return;
		}
		pt.flagClose = sl_true;
		Referable* po = m_platformObject.get();
		if (po) {
			_closeSubpath_PO(po);
		}
	}

	void GraphicsPath::addArc(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees, sl_bool flagMoveTo)
	{
		Rectangle rect(x, y, x + width, y + height);
		addArc(rect, startDegrees, sweepDegrees, flagMoveTo);
	}

	void GraphicsPath::addArc(const Rectangle& rect, sl_real startDegrees, sl_real sweepDegrees, sl_bool flagMoveTo)
	{
		Point pts[13];
		sl_uint32 n = CubicBezierCurve::convertArcToBezier(pts, rect, startDegrees, sweepDegrees);
		if (flagMoveTo) {
			moveTo(pts[0]);
		} else {
			lineTo(pts[0]);
		}
		for (sl_uint32 i = 1; i + 2 < n; i += 3) {
			cubicTo(pts[i], pts[i + 1], pts[i + 2]);
		}
	}

	void GraphicsPath::addRectangle(sl_real x, sl_real y, sl_real width, sl_real height)
	{
		Rectangle rect(x, y, x + width, y + height);
		addRectangle(rect);
	}

	void GraphicsPath::addRectangle(const Rectangle& rect)
	{
		moveTo(rect.left, rect.top);
		lineTo(rect.right, rect.top);
		lineTo(rect.right, rect.bottom);
		lineTo(rect.left, rect.bottom);
		closeSubpath();
	}

	void GraphicsPath::addRoundRect(sl_real x, sl_real y, sl_real w, sl_real h, sl_real rx, sl_real ry)
	{
		float rw = rx * 2;
		float rh = ry * 2;
		float xr = x + w - rw;
		float yb = y + h - rh;
		float k1 = 0.77614234f;
		float k2 = 0.22385763f;
		moveTo(xr + rw, yb + ry);
		cubicTo(xr + rw, yb + k1 * rh, xr + k1 * rw, yb + rh, xr + rx, yb + rh);
		lineTo(x + rx, yb + rh);
		cubicTo(x + k2 * rw, yb + rh, x, yb + k1 * rh, x, yb + ry);
		lineTo(x, y + ry);
		cubicTo(x, y + k2 * rh, x + k2 * rw, y, x + rx, y);
		lineTo(xr + rx, y);
		cubicTo(xr + k1 * rw, y, xr + rw, y + k2 * rh, xr + rw, y + ry);
		//lineTo(xr + rw, yb + ry);
		closeSubpath();
	}

	void GraphicsPath::addRoundRect(const Rectangle& rect, const Size& radius)
	{
		addRoundRect(rect.left, rect.top, rect.getWidth(), rect.getHeight(), radius.x, radius.y);
	}

	void GraphicsPath::addEllipse(sl_real x, sl_real y, sl_real width, sl_real height)
	{
		Rectangle rect(x, y, x + width, y + height);
		addArc(rect, 0, 360, sl_true);
		closeSubpath();
	}

	void GraphicsPath::addEllipse(const Rectangle& rect)
	{
		addArc(rect, 0, 360, sl_true);
		closeSubpath();
	}

	void GraphicsPath::addPie(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees)
	{
		Rectangle rect(x, y, x + width, y + height);
		addPie(rect, startDegrees, sweepDegrees);
	}

	void GraphicsPath::addPie(const Rectangle& rect, sl_real startDegrees, sl_real sweepDegrees)
	{
		moveTo(rect.getCenter());
		addArc(rect, startDegrees, sweepDegrees, sl_false);
		closeSubpath();
	}

	FillMode GraphicsPath::getFillMode()
	{
		return m_fillMode;
	}

	void GraphicsPath::setFillMode(FillMode mode)
	{
		m_fillMode = mode;
		Referable* po = m_platformObject.get();
		if (po) {
			_setFillMode_PO(po, mode);
		}
	}

	Rectangle GraphicsPath::getBounds()
	{
		return GetBounds(m_points.getData(), m_points.getCount());
	}

	Rectangle GraphicsPath::getControlBounds()
	{
		return GetControlBounds(m_points.getData(), m_points.getCount());
	}

	sl_bool GraphicsPath::containsPoint(sl_real x, sl_real y)
	{
		return ContainsPoint::run(m_points.getData(), m_points.getCount(), m_fillMode, x, y);
	}

	sl_bool GraphicsPath::containsPoint(const Point& pt)
	{
		return containsPoint(pt.x, pt.y);
	}

	void GraphicsPath::_initPlatformObject()
	{
		if (m_platformObject.isNotNull()) {
			return;
		}
		Ref<Referable> refPo = _createPlatformObject();
		if (refPo.isNull()) {
			return;
		}
		Referable* po = refPo.get();
		sl_size n = m_points.getCount();
		GraphicsPathPoint* pts = m_points.getData();
		for (sl_size i = 0; i < n; i++) {
			GraphicsPathPoint& pt = pts[i];
			switch (pt.type) {
				case GraphicsPathPoint::MoveTo:
					_moveTo_PO(po, pt.x, pt.y);
					break;
				case GraphicsPathPoint::LineTo:
					_lineTo_PO(po, pt.x, pt.y);
					break;
				case GraphicsPathPoint::CubicTo:
					if (IsCubicControl2AndEnd(pts + i + 1, n - i - 1)) {
						_cubicTo_PO(po, pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y, pts[i + 2].x, pts[i + 2].y);
						i += 2;
					}
					break;
				default:
					break;
			}
			if (pts[i].flagClose) {
				_closeSubpath_PO(po);
			}
		}
		_setFillMode_PO(po, m_fillMode);
		if (m_platformObject.isNotNull()) {
			return;
		}
		SpinLocker lock(&m_lock);
		if (m_platformObject.isNotNull()) {
			return;
		}
		m_platformObject = Move(refPo);
	}

#if !(defined(SLIB_GRAPHICS_IS_GDI)) && !(defined(SLIB_GRAPHICS_IS_QUARTZ)) && !(defined(SLIB_GRAPHICS_IS_ANDROID))

	Ref<Referable> GraphicsPath::_createPlatformObject()
	{
		return sl_null;
	}

	void GraphicsPath::_moveTo_PO(Referable* po, sl_real x, sl_real y)
	{
	}

	void GraphicsPath::_lineTo_PO(Referable* po, sl_real x, sl_real y)
	{
	}

	void GraphicsPath::_cubicTo_PO(Referable* po, sl_real xc1, sl_real yc1, sl_real xc2, sl_real yc2, sl_real xe, sl_real ye)
	{
	}

	void GraphicsPath::_closeSubpath_PO(Referable* po)
	{
	}

	void GraphicsPath::_setFillMode_PO(Referable* po, FillMode mode)
	{
	}

#endif

}


