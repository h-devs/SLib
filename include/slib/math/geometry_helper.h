/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_MATH_POLYGON_HELPER
#define CHECKHEADER_SLIB_MATH_POLYGON_HELPER

#include "math.h"
#include "triangle.h"
#include "line_segment.h"

#include "../core/list.h"

namespace slib
{

	class SLIB_EXPORT GeometryHelper
	{
	public:
		template <class T>
		static sl_bool isPointInPolygon(const PointT<T>& point, const PointT<T>* polygonPoints, sl_size nPolygonPoints)
		{
			sl_size nIntersect = 0;
			for (sl_size i = 0; i < nPolygonPoints; i++) {
				const PointT<T>& p1 = polygonPoints[i];
				const PointT<T>& p2 = polygonPoints[(i + 1) % nPolygonPoints];
				if (p1.x > point.x && Math::isAlmostZero(p1.y - point.y)) {
					const PointT<T>& p0 = polygonPoints[(i + nPolygonPoints - 1) % nPolygonPoints];
					if (!(Math::isAlmostZero(p0.y - p1.y) || Math::isAlmostZero(p2.y - p1.y))) {
						if (p0.y < p1.y) {
							if (p2.y > p1.y) {
								nIntersect++;
							}
						} else {
							if (p2.y < p1.y) {
								nIntersect++;
							}
						}
					}
				} else {
					if (!(p2.x > point.x && Math::isAlmostZero(p2.y - point.y))) {
						if (_intersectLineSegmentAndRayX(p1, p2, point)) {
							nIntersect++;
						}
					}
				}
			}
			return (nIntersect & 1) != 0;
		}

		template <class T>
		static List< TriangleT<T> > splitPolygonToTriangles(const PointT<T>* points, sl_size nPoints, sl_bool flagOutputCW = sl_false)
		{
			if (nPoints < 3) {
				return sl_null;
			}
			List< TriangleT<T> > ret;
			if (nPoints == 3) {
				TriangleT<T> t(points[0], points[1], points[2]);
				if (flagOutputCW) {
					t.makeClockwise();
				}
				if (!ret.add_NoLock(t)) {
					return sl_null;
				}
				return ret;
			}
			if (!(_splitPolygonToTriangles(ret, points, nPoints, flagOutputCW))) {
				return sl_null;
			}
			return ret;
		}

		template <class T>
		static List< TriangleT<T> > splitEllipseBorderToTriangles(T centerX, T centerY, T width, T height, T borderWidth)
		{
			List< TriangleT<T> > ret;
			T circum = (width + height) * (T)SLIB_PI_HALF_LONG;
			if (circum > 1000.0) {
				circum = 1000.0;
			}
			sl_uint32 n = (sl_uint32)(circum);
			T bw = borderWidth / (T)2;
			width /= (T)2;
			height /= (T)2;
			T w1 = width - bw;
			T w2 = width + bw;
			T h1 = height - bw;
			T h2 = height + bw;
			PointT<T> last1, last2;
			for (sl_uint32 i = 0; i <= n; i++) {
				T angle = (T)(SLIB_PI_DUAL_LONG) * (T)i / (T)n;
				T ux = Math::cos(angle);
				T uy = Math::sin(angle);
				PointT<T> pt1, pt2;
				pt1.x = centerX + ux * w1;
				pt1.y = centerY + uy * h1;
				pt2.x = centerX + ux * w2;
				pt2.y = centerY + uy * h2;
				if (i) {
					if (!(ret.add_NoLock(TriangleT<T>(last2, pt2, last1)))) {
						return sl_null;
					}
					if (!(ret.add_NoLock(TriangleT<T>(last1, pt2, pt1)))) {
						return sl_null;
					}
				}
				last1 = pt1;
				last2 = pt2;
			}
			return ret;
		}

	private:
		template <class T>
		static sl_bool _splitPolygonToTriangles(List< TriangleT<T> >& ret, const PointT<T>* points, sl_size nPoints, sl_bool flagOutputCW)
		{
			// ear cutting algorithm.
			List< PointT<T> > pointList;
			sl_size iStart = 0;
			while (nPoints > 3) {
				for (sl_size i = 0; i < nPoints; i++) {
					TriangleT<T> triangle;
					sl_size iPt = (iStart + i) % nPoints;
					triangle.point1 = points[(iPt + nPoints - 1) % nPoints];
					triangle.point2 = points[iPt];
					triangle.point3 = points[(iPt + 1) % nPoints];
					sl_bool bEar = sl_false;
					if (i == nPoints - 1) {
						bEar = sl_true;
					} else if (!(_isPointInPolygon(triangle.point2, points, nPoints, iPt))) {
						bEar = sl_true;
						for (sl_size i = 0; i < nPoints - 3; i++) {
							if (isPointInPolygon(points[(iPt + 2 + i) % nPoints], &(triangle.point1), 3)) {
								bEar = sl_false;
								break;
							}
						}
					}
					if (bEar) {
						if (flagOutputCW) {
							triangle.makeClockwise();
						}
						if (!ret.add_NoLock(triangle)) {
							return sl_false;
						}
						if (pointList.isNotNull()) {
							if (!(pointList.removeAt_NoLock(iPt))) {
								return sl_false;
							}
							points = pointList.getData();
						} else {
							if (iPt + 1 < nPoints) {
								if (iPt) {
									if (!(pointList.addElements_NoLock(points, iPt))) {
										return sl_false;
									}
									if (!(pointList.addElements_NoLock(points + (iPt + 1), nPoints - iPt - 1))) {
										return sl_false;
									}
									points = pointList.getData();
								} else {
									points++;
								}
							}
						}
						nPoints--;
						iStart = iPt;
						break;
					}
				}
			}
			{
				TriangleT<T> triangle(points[0], points[1], points[2]);
				if (flagOutputCW) {
					triangle.makeClockwise();
				}
				if (!(ret.add_NoLock(triangle))) {
					return sl_false;
				}
			}
			return sl_true;
		}

		template <class T>
		static sl_bool _intersectLineSegmentAndRayX(const PointT<T>& pt1, const PointT<T>& pt2, const PointT<T>& ptRay, PointT<T>* _out = sl_null)
		{
			T dy = pt2.y - pt1.y;
			if (Math::isAlmostZero(dy)) {
				return sl_false;
			}
			sl_bool bOnLine;
			if (dy >= 0) {
				bOnLine = ptRay.y >= pt1.y && ptRay.y <= pt2.y;
			} else {
				bOnLine = ptRay.y <= pt1.y && ptRay.y >= pt2.y;
			}
			if (bOnLine) {
				T ix = pt1.x + (ptRay.y - pt1.y) * (pt2.x - pt1.x) / dy;
				if (ix >= ptRay.x) {
					if (_out) {
						_out->x = ix;
						_out->y = ptRay.y;
					}
					return sl_true;
				}
			}
			return sl_false;
		}

		template <class T>
		static sl_bool _isPointInPolygon(const PointT<T>& point, const PointT<T>* polygonPoints, sl_size nPolygonPoints, sl_size iIgnoreIndex)
		{
			sl_size nIntersect = 0;
			for (sl_size i = 0; i < nPolygonPoints; i++) {
				if (i == iIgnoreIndex) {
					continue;
				}
				const PointT<T>& p1 = polygonPoints[i];
				sl_size iNext = (i + 1) % nPolygonPoints;
				if (iNext == iIgnoreIndex) {
					iNext = (iNext + 1) % nPolygonPoints;
				}
				const PointT<T>& p2 = polygonPoints[iNext];
				if (p1.x > point.x && Math::isAlmostZero(p1.y - point.y)) {
					sl_size iBefore = (i + nPolygonPoints - 1) % nPolygonPoints;
					if (iBefore == iIgnoreIndex) {
						iBefore = (iBefore + nPolygonPoints - 1) % nPolygonPoints;
					}
					const PointT<T>& p0 = polygonPoints[iBefore];
					if (!(Math::isAlmostZero(p0.y - p1.y) || Math::isAlmostZero(p2.y - p1.y))) {
						if (p0.y < p1.y) {
							if (p2.y > p1.y) {
								nIntersect++;
							}
						} else {
							if (p2.y < p1.y) {
								nIntersect++;
							}
						}
					}
				} else {
					if (!(p2.x > point.x && Math::isAlmostZero(p2.y - point.y))) {
						if (_intersectLineSegmentAndRayX(p1, p2, point)) {
							nIntersect++;
						}
					}
				}
			}
			return (nIntersect & 1) != 0;
		}

	};

}

#endif