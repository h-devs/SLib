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

	SLIB_DEFINE_ROOT_OBJECT(GraphicsPath)

	GraphicsPath::GraphicsPath()
	{
		m_fillMode = FillMode::Winding;
		m_flagBegan = sl_false;
		m_pointBegin = Point::zero();
	}

	GraphicsPath::~GraphicsPath()
	{
	}

	Ref<GraphicsPath> GraphicsPath::create()
	{
		Ref<GraphicsPath> path = new GraphicsPath;
		if (path.isNotNull()) {
			if (path->_initialize_PO()) {
				return path;
			}
		}
		return sl_null;
	}

	sl_size GraphicsPath::getPointsCount()
	{
		return m_points.getCount();
	}

	GraphicsPathPoint* GraphicsPath::getPoints()
	{
		return m_points.getData();
	}

	void GraphicsPath::moveTo(sl_real x, sl_real y)
	{
		m_flagBegan = sl_false;
		m_pointBegin.x = x;
		m_pointBegin.y = y;
	}

	void GraphicsPath::moveTo(const Point& pt)
	{
		moveTo(pt.x, pt.y);
	}

	void GraphicsPath::_checkBegin() noexcept
	{
		if (!m_flagBegan) {
			GraphicsPathPoint point;
			point.pt = m_pointBegin;
			point.type = GraphicsPathPoint::Begin;
			m_points.add_NoLock(point);
			_moveTo_PO(point.pt.x, point.pt.y);
			m_flagBegan = sl_true;
		}
	}

	void GraphicsPath::lineTo(sl_real x, sl_real y)
	{
		_checkBegin();
		GraphicsPathPoint point;
		point.pt.x = x;
		point.pt.y = y;
		point.type = GraphicsPathPoint::Line;
		m_points.add_NoLock(point);
		_lineTo_PO(x, y);
	}

	void GraphicsPath::lineTo(const Point& pt)
	{
		lineTo(pt.x, pt.y);
	}

	void GraphicsPath::conicTo(sl_real xc, sl_real yc, sl_real xe, sl_real ye)
	{
		_checkBegin();
		GraphicsPathPoint last = m_points.getLastValue_NoLock();
		sl_real xc1 = xc - (xc - last.pt.x) / 3.0f;
		sl_real yc1 = yc - (yc - last.pt.y) / 3.0f;
		sl_real xc2 = xc + (xe - xc) / 3.0f;
		sl_real yc2 = yc + (ye - yc) / 3.0f;
		GraphicsPathPoint point;
		point.pt.x = xc1;
		point.pt.y = yc1;
		point.type = GraphicsPathPoint::BezierCubic;
		m_points.add_NoLock(point);
		point.pt.x = xc2;
		point.pt.y = yc2;
		point.type = GraphicsPathPoint::BezierCubic;
		m_points.add_NoLock(point);
		point.pt.x = xe;
		point.pt.y = ye;
		point.type = GraphicsPathPoint::BezierCubic;
		m_points.add_NoLock(point);
		_cubicTo_PO(xc1, yc1, xc2, yc2, xe, ye);
	}

	void GraphicsPath::conicTo(const Point& ptControl, const Point& ptEnd)
	{
		conicTo(ptControl.x, ptControl.y, ptEnd.x, ptEnd.y);
	}

	void GraphicsPath::cubicTo(sl_real xc1, sl_real yc1, sl_real xc2, sl_real yc2, sl_real xe, sl_real ye)
	{
		_checkBegin();
		GraphicsPathPoint point;
		point.pt.x = xc1;
		point.pt.y = yc1;
		point.type = GraphicsPathPoint::BezierCubic;
		m_points.add_NoLock(point);
		point.pt.x = xc2;
		point.pt.y = yc2;
		point.type = GraphicsPathPoint::BezierCubic;
		m_points.add_NoLock(point);
		point.pt.x = xe;
		point.pt.y = ye;
		point.type = GraphicsPathPoint::BezierCubic;
		m_points.add_NoLock(point);
		_cubicTo_PO(xc1, yc1, xc2, yc2, xe, ye);
	}

	void GraphicsPath::cubicTo(const Point& ptControl1, const Point& ptControl2, const Point& ptEnd)
	{
		cubicTo(ptControl1.x, ptControl1.y, ptControl2.x, ptControl2.y, ptEnd.x, ptEnd.y);
	}

	void GraphicsPath::closeSubpath()
	{
		if (m_flagBegan) {
			sl_size n = m_points.getCount();
			if (n > 0) {
				GraphicsPathPoint* list = m_points.getData();
				list[n - 1].type |= GraphicsPathPoint::FlagClose;
				m_pointBegin = list[n - 1].pt;
				m_flagBegan = sl_false;
				_closeSubpath_PO();
			}
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
		_setFillMode_PO(mode);
	}

	Rectangle GraphicsPath::getBounds()
	{
		return _getBounds_PO();
	}

	sl_bool GraphicsPath::containsPoint(sl_real x, sl_real y)
	{
		return _containsPoint_PO(x, y);
	}

	sl_bool GraphicsPath::containsPoint(const Point& pt)
	{
		return containsPoint(pt.x, pt.y);
	}

#if !(defined(SLIB_GRAPHICS_IS_GDI)) && !(defined(SLIB_GRAPHICS_IS_QUARTZ)) && !(defined(SLIB_GRAPHICS_IS_ANDROID))

	sl_bool GraphicsPath::_initialize_PO()
	{
		return sl_true;
	}

	void GraphicsPath::_moveTo_PO(sl_real x, sl_real y)
	{
	}

	void GraphicsPath::_lineTo_PO(sl_real x, sl_real y)
	{
	}

	void GraphicsPath::_cubicTo_PO(sl_real xc1, sl_real yc1, sl_real xc2, sl_real yc2, sl_real xe, sl_real ye)
	{
	}

	void GraphicsPath::_closeSubpath_PO()
	{
	}

	void GraphicsPath::_setFillMode_PO(FillMode mode)
	{
	}

	Rectangle GraphicsPath::_getBounds_PO()
	{
		return Rectangle::zero();
	}

	sl_bool GraphicsPath::_containsPoint_PO(sl_real x, sl_real y)
	{
		return sl_false;
	}

#endif

}


