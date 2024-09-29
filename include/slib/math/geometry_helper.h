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

#ifndef CHECKHEADER_SLIB_MATH_GEOMETRY_HELPER
#define CHECKHEADER_SLIB_MATH_GEOMETRY_HELPER

#include "math.h"
#include "triangle.h"
#include "line_segment.h"
#include "line.h"

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
		static List< TriangleT<T> > splitPolygonToTriangles(const PointT<T>* points, sl_size nPoints)
		{
			if (nPoints < 3) {
				return sl_null;
			}
			List< TriangleT<T> > ret;
			if (nPoints == 3) {
				TriangleT<T> t(points[0], points[1], points[2]);
				if (!ret.add_NoLock(t)) {
					return sl_null;
				}
				return ret;
			}
			if (!(_splitPolygonToTriangles(ret, points, nPoints))) {
				return sl_null;
			}
			return ret;
		}

		template <class T>
		static List< TriangleT<T> > splitPolylineToTriangles(const PointT<T>* points, sl_size nPoints, T borderWidth, sl_bool flagClose = sl_true)
		{
			if (nPoints < 2) {
				return sl_null;
			}
			List< TriangleT<T> > ret;
			if (!(_splitPolylineToTriangles(ret, points, nPoints, borderWidth / (T)2, flagClose))) {
				return sl_null;
			}
			return ret;
		}

		template <class T>
		static void splitLineToTriangles(TriangleT<T> _out[2], const PointT<T>& point1, const PointT<T>& point2, T width)
		{
			Vector2T<T> dir = point2 - point1;
			Vector2T<T> normal = Vector2T<T>(-dir.y, dir.x).getNormalized() * (width / (T)2);
			PointT<T> pt1 = point1 + normal;
			PointT<T> pt2 = point1 - normal;
			PointT<T> pt3 = point2 - normal;
			PointT<T> pt4 = point2 + normal;
			_out[0].point1 = pt2;
			_out[0].point2 = pt3;
			_out[0].point3 = pt1;
			_out[1].point1 = pt1;
			_out[1].point2 = pt3;
			_out[1].point3 = pt4;
		}

		template <class T>
		static void splitQuadrangleToTriangles(TriangleT<double> _out[2], const PointT<T>& topLeft, const PointT<T>& topRight, const PointT<T>& bottomRight, const PointT<T>& bottomLeft)
		{
			_out[0].point1 = bottomLeft;
			_out[0].point2 = bottomRight;
			_out[0].point3 = topLeft;
			_out[1].point1 = topLeft;
			_out[1].point2 = bottomRight;
			_out[1].point3 = topRight;
		}

		template <class T>
		static void splitRectangleToTriangles(TriangleT<T> _out[2], const PointT<T>& point1, const PointT<T>& point2)
		{
			splitQuadrangleToTriangles(_out, point1, PointT<T>(point2.x, point1.y), point2, PointT<T>(point1.x, point2.y));
		}

		template <class T>
		static void splitRectangleBorderToTriangles(TriangleT<T> _out[8], const PointT<T>& point1, const PointT<T>& point2, T borderWidth)
		{
			T w = borderWidth / (T)2;
			T x1 = point1.x;
			T y1 = point1.y;
			T x2 = point2.x;
			T y2 = point2.y;
			PointT<T> TL1(x1 - w, y1 - w);
			PointT<T> TL2(x1 + w, y1 + w);
			PointT<T> TR1(x2 + w, y1 - w);
			PointT<T> TR2(x2 - w, y1 + w);
			PointT<T> BR1(x2 + w, y2 + w);
			PointT<T> BR2(x2 - w, y2 - w);
			PointT<T> BL1(x1 - w, y2 + w);
			PointT<T> BL2(x1 + w, y2 - w);
			splitQuadrangleToTriangles(_out, TL1, TL2, TR2, TR1);
			splitQuadrangleToTriangles(_out + 2, TR1, TR2, BR2, BR1);
			splitQuadrangleToTriangles(_out + 4, BR1, BR2, BL2, BL1);
			splitQuadrangleToTriangles(_out + 6, BR1, BR2, TL2, TL1);
		}

		template <class T>
		static List< TriangleT<T> > splitPieToTriangles(T centerX, T centerY, T width, T height, T startRadian, T sweepRadian, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			List< TriangleT<T> > ret;
			T _n = (width + height) * sweepRadian / ((T)4 * chopLength);
			sl_uint32 n;
			if (_n > (T)maxChops) {
				n = maxChops;
			} else {
				n = (sl_uint32)_n;
				if (n < 4) {
					n = 4;
				}
			}
			width /= (T)2;
			height /= (T)2;
			PointT<T> center(centerX, centerY);
			PointT<T> last;
			for (sl_uint32 i = 0; i <= n; i++) {
				T angle = startRadian + sweepRadian * (T)i / (T)n;
				T ux = Math::cos(angle);
				T uy = Math::sin(angle);
				PointT<T> pt;
				pt.x = centerX + ux * width;
				pt.y = centerY + uy * height;
				if (i) {
					if (!(ret.add_NoLock(TriangleT<T>(last, pt, center)))) {
						return sl_null;
					}
				}
				last = pt;
			}
			return ret;
		}

		template <class T>
		static List< TriangleT<T> > splitEllipseToTriangles(T centerX, T centerY, T width, T height, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			return splitPieToTriangles(centerX, centerY, width, height, (T)0, (T)SLIB_PI_DUAL_LONG, chopLength, maxChops);
		}

		template <class T>
		static List< TriangleT<T> > splitArcToTriangles(T centerX, T centerY, T width, T height, T borderWidth, T startRadian, T sweepRadian, T chopLength = (T)1, sl_uint32 maxChops = 1024, sl_bool flagClose = sl_false)
		{
			List< TriangleT<T> > ret;
			T _n = (width + height) * sweepRadian / ((T)4 * chopLength);
			sl_uint32 n;
			if (_n > (T)maxChops) {
				n = maxChops;
			} else {
				n = (sl_uint32)_n;
				if (n < 4) {
					n = 4;
				}
			}
			T bw = borderWidth / (T)2;
			width /= (T)2;
			height /= (T)2;
			T w1 = width - bw;
			T w2 = width + bw;
			T h1 = height - bw;
			T h2 = height + bw;
			PointT<T> first1, first2;
			PointT<T> last1, last2;
			for (sl_uint32 i = 0; i <= n; i++) {
				T angle = startRadian + sweepRadian * (T)i / (T)n;
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
				} else {
					first1 = pt1;
					first2 = pt2;
				}
				last1 = pt1;
				last2 = pt2;
			}
			if (flagClose) {
				if (!(ret.add_NoLock(TriangleT<T>(last2, first2, last1)))) {
					return sl_null;
				}
				if (!(ret.add_NoLock(TriangleT<T>(last1, first2, first1)))) {
					return sl_null;
				}
			}
			return ret;
		}

		template <class T>
		static List< TriangleT<T> > splitEllipseBorderToTriangles(T centerX, T centerY, T width, T height, T borderWidth, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			return splitArcToTriangles(centerX, centerY, width, height, borderWidth, (T)0, (T)SLIB_PI_DUAL_LONG, chopLength, maxChops);
		}

		template <class T>
		static List< TriangleT<T> > splitChordToTriangles(T centerX, T centerY, T width, T height, T startRadian, T sweepRadian, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			List< TriangleT<T> > ret;
			T _n = (width + height) * sweepRadian / ((T)4 * chopLength);
			sl_uint32 n;
			if (_n > (T)maxChops) {
				n = maxChops;
			} else {
				n = (sl_uint32)_n;
				if (n < 4) {
					n = 4;
				}
			}
			width /= (T)2;
			height /= (T)2;
			PointT<T> first, last;
			for (sl_uint32 i = 0; i <= n; i++) {
				T angle = startRadian + sweepRadian * (T)i / (T)n;
				T ux = Math::cos(angle);
				T uy = Math::sin(angle);
				PointT<T> pt;
				pt.x = centerX + ux * width;
				pt.y = centerY + uy * height;
				if (i) {
					if (!(ret.add_NoLock(TriangleT<T>(last, pt, first)))) {
						return sl_null;
					}
				} else {
					first = pt;
				}
				last = pt;
			}
			return ret;
		}

	private:
		template <class T>
		static sl_bool _splitPolygonToTriangles(List< TriangleT<T> >& ret, const PointT<T>* points, sl_size nPoints)
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
				if (!(ret.add_NoLock(triangle))) {
					return sl_false;
				}
			}
			return sl_true;
		}

		template <class T>
		static sl_bool _splitPolylineToTriangles(List< TriangleT<T> >& ret, const PointT<T>* points, sl_size nPoints, T dist, sl_bool flagClosed)
		{
			List< PointT<T> > sealingPoly;
			List<PointT<T>> procTriangle;

			PointT<T> lastPoint = points[0];
			{
				for (sl_size i = 0; i < nPoints; i++) {
					const PointT<T>& pt = points[i];
					if (!lastPoint.equals(pt)) {
						procTriangle.add(lastPoint);
						lastPoint = pt;
					}
				}
			}
			if (!lastPoint.equals(procTriangle.getLastValue())) {
				procTriangle.add(lastPoint);
			}
			nPoints = procTriangle.getCount();
			if (procTriangle[0].equals(procTriangle.getLastValue())) {
				procTriangle.removeAt(nPoints - 1);
				nPoints--;
			}

			for (sl_size i = 0; i < nPoints; i++) {
				sl_size iPt = i % nPoints;
				PointT<T> point1, point2, point3;
				PointT<T> inPt;
				PointT<T> iTmp, oTmp;
				point1 = procTriangle[(iPt + nPoints - 1) % nPoints];
				point2 = procTriangle[iPt];
				point3 = procTriangle[(iPt + 1) % nPoints];
				Vector2T<T> v1 = point1 - point2;
				Vector2T<T> v2 = point3 - point2;

				if ((!flagClosed && i == 0) || (!flagClosed && i == nPoints - 1)) {
					LineT<T> normL;
					Vector2T<T> v = i == 0 ? v2 : v1;
					normL.setFromPointAndNormal(point2, v);
					_getLinePointAwayFromOne(normL, point2, dist, iTmp, oTmp);
				} else {
					// make sure point 1,2,3, can make convex part of polygon.
					sl_bool isConvex = _isPointInPolygon(point2, &procTriangle[0], nPoints, iPt);
					T angle = v1.getAbsAngleBetween(v2);
					T pseudo_dist = Math::min(v1.getLength(), v2.getLength());

					LineT<T> inNorL1, outNorL1;
					if (!_getNormalLineFromLineSegment(point1, point2, pseudo_dist, isConvex, inNorL1, outNorL1)) {
						return sl_false;
					}
					LineT<T> inNorL2, outNorL2;
					if (!_getNormalLineFromLineSegment(point3, point2, pseudo_dist, isConvex, inNorL2, outNorL2)) {
						return sl_false;
					}

					if (!inNorL1.intersect(inNorL2, &inPt)) {
						return sl_false;
					}

					LineT<T> middleLine;
					middleLine.setFromPointAndDirection(point2, inPt - point2);

					_getLinePointAwayFromOne(middleLine, point2, dist / Math::sin(angle / 2), iTmp, oTmp);
				}

				if (sealingPoly.isEmpty()) {
					if (!sealingPoly.add_NoLock(iTmp)) {
						return sl_false;
					}
					if (!sealingPoly.add_NoLock(oTmp)) {
						return sl_false;
					}
				} else {
					LineSegmentT<T> l1(point1, point2);
					LineSegmentT<T> critLine(iTmp, sealingPoly.getLastValue());
					sl_bool crit = l1.intersect(critLine);
					if (crit) {
						if (!sealingPoly.add_NoLock(iTmp)) {
							return sl_false;
						}
						if (!sealingPoly.add_NoLock(oTmp)) {
							return sl_false;
						}
					} else {
						if (!sealingPoly.add_NoLock(oTmp)) {
							return sl_false;
						}
						if (!sealingPoly.add_NoLock(iTmp)) {
							return sl_false;
						}
					}
				}
			}
			for (sl_size i = 0; i < sealingPoly.getCount(); i++) {
				if (!flagClosed && i > sealingPoly.getCount() - 3) {
					break;
				}
				if (!(ret.add_NoLock(Triangle(sealingPoly[i], sealingPoly[(i + 1) % sealingPoly.getCount()], sealingPoly[(i + 2) % sealingPoly.getCount()])))) {
					return sl_false;
				}
			}
			return sl_true;
		}

		template <class T>
		static sl_bool _getNormalLineFromLineSegment(const PointT<T>& pt1, const PointT<T>& pt2, T dist, const sl_bool isConvex, LineT<T>& _inLine, LineT<T>& _outLine)
		{
			LineT<T> l1;
			l1.setFromPointAndDirection(pt2, pt1 - pt2);

			PointT<T> startPt1, startPt2, innerPt, outerPt;
			_getLinePointAwayFromOne(l1, pt2, dist, startPt1, startPt2);

			Vector2T<T> tmp1(startPt1 - pt2);
			tmp1.normalize();
			Vector2T<T> tmp2(pt1 - pt2);
			tmp2.normalize();
			Vector2T<T> criterionVec = tmp1 - tmp2;
			sl_bool criterion = criterionVec.getLength2p() < 0.0001;

			if (!isConvex) {
				innerPt = criterion ? startPt1 : startPt2;
				outerPt = criterion ? startPt2 : startPt1;
			} else {
				innerPt = criterion ? startPt2 : startPt1;
				outerPt = criterion ? startPt1 : startPt2;
			}

			_inLine.setFromPointAndDirection(innerPt, l1.getNormal());
			_outLine.setFromPointAndDirection(outerPt, l1.getNormal());
			return sl_true;
		}

		template <class T>
		static void _getLinePointAwayFromOne(const LineT<T>& line, const PointT<T>& point, T dist, PointT<T>& _out1, PointT<T>& _out2)
		{
			Vector2T<T> dir = line.getDirection().getNormalized() * dist;
			_out1 = point + dir;
			_out2 = point - dir;
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