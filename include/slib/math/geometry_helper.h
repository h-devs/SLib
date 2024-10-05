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
					if (!(Math::isAlmostZero(p2.y - p1.y))) {
						const PointT<T>& p0 = polygonPoints[(i + nPolygonPoints - 1) % nPolygonPoints];
						if (p2.y > p1.y) {
							if (Math::isLessThanEpsilon(p0.y - p1.y)) {
								nIntersect++;
							}
						} else {
							if (Math::isLessThanEpsilon(p1.y - p0.y)) {
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
		static void splitQuadrangleToTriangles(TriangleT<T> _out[2], const PointT<T>& topLeft, const PointT<T>& topRight, const PointT<T>& bottomRight, const PointT<T>& bottomLeft)
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
		static void splitLineToTriangles(TriangleT<T> _out[2], const PointT<T>& point1, const PointT<T>& point2, T width)
		{
			T hw = width / (T)2;
			Vector2T<T> dir = point2 - point1;
			T lenDir2p = dir.getLength2p();
			if (Math::isAlmostZero(lenDir2p)) {
				splitRectangleToTriangles(_out, PointT<T>(point1.x - hw, point1.y - hw), PointT<T>(point2.x + hw, point2.y + hw));
				return;
			}
			dir *= hw / Math::sqrt(lenDir2p);
			Vector2T<T> normal(-dir.y, dir.x);
			Vector2T<T> mp1 = point1 - dir;
			Vector2T<T> mp2 = point2 + dir;
			PointT<T> sp1 = mp1 + normal;
			PointT<T> sp2 = mp1 - normal;
			PointT<T> sp3 = mp2 - normal;
			PointT<T> sp4 = mp2 + normal;
			_out[0].point1 = sp2;
			_out[0].point2 = sp3;
			_out[0].point3 = sp1;
			_out[1].point1 = sp1;
			_out[1].point2 = sp3;
			_out[1].point3 = sp4;
		}

		template <class T>
		static List< TriangleT<T> > splitPolygonToTriangles(const PointT<T>* points, sl_size nPoints)
		{
			if (nPoints < 3) {
				return sl_null;
			}
			if (nPoints == 3) {
				return List< TriangleT<T> >::createFromElement(TriangleT<T>(points[0], points[1], points[2]));
			}
			// ear cutting algorithm.
			List< TriangleT<T> > ret;
			List< PointT<T> > pointList;
			sl_size iStart = 0;
			while (nPoints > 3) {
				for (sl_size i = 0; i < nPoints; i++) {
					TriangleT<T> triangle;
					sl_size iPt = (iStart + i) % nPoints;
					triangle.point1 = points[(iPt + nPoints - 1) % nPoints];
					triangle.point2 = points[iPt];
					triangle.point3 = points[(iPt + 1) % nPoints];
					sl_bool bIgnore = sl_false;
					if (triangle.point1.isAlmostEqual(triangle.point2) || triangle.point2.isAlmostEqual(triangle.point3) || triangle.point1.isAlmostEqual(triangle.point3)) {
						bIgnore = sl_true;
					}
					sl_bool bEar = sl_false;
					if (!bIgnore) {
						if (i == nPoints - 1) {
							bEar = sl_true;
						} else if (!(_isPointInPolygon(triangle.point2, points, nPoints, iPt))) {
							bEar = sl_true;
							for (sl_size k = 0; k < nPoints - 3; k++) {
								if (isPointInPolygon(points[(iPt + 2 + k) % nPoints], &(triangle.point1), 3)) {
									bEar = sl_false;
									break;
								}
							}
						}
					}
					if (bEar) {
						if (!ret.add_NoLock(triangle)) {
							return sl_null;
						}
					}
					if (bEar || bIgnore) {
						if (pointList.isNotNull()) {
							if (!(pointList.removeAt_NoLock(iPt))) {
								return sl_null;
							}
							points = pointList.getData();
						} else {
							if (iPt + 1 < nPoints) {
								if (iPt) {
									if (!(pointList.addElements_NoLock(points, iPt))) {
										return sl_null;
									}
									if (!(pointList.addElements_NoLock(points + (iPt + 1), nPoints - iPt - 1))) {
										return sl_null;
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
			if (!(ret.add_NoLock(points[0], points[1], points[2]))) {
				return sl_null;
			}
			return ret;
		}

		template <class T, sl_bool CLOSE = sl_false>
		static List< TriangleT<T> > splitPolylineToTriangles(const PointT<T>* points, sl_size nPoints, T borderWidth)
		{
			if (!nPoints) {
				return sl_null;
			}
			if (CLOSE) {
				const PointT<T>& first = *points;
				while (nPoints > 1) {
					const PointT<T>& pt = points[nPoints - 1];
					if (!(pt.isAlmostEqual(first))) {
						break;
					}
					nPoints--;
				}
			}
			T hw = borderWidth / (T)2;
			if (nPoints == 1) {
				TriangleT<T> t[2];
				const PointT<T>& pt = *points;
				splitRectangleToTriangles(t, PointT<T>(pt.x - hw, pt.y - hw), PointT<T>(pt.x + hw, pt.y + hw));
				return List< TriangleT<T> >::create(t, 2);
			}
			if (nPoints == 2) {
				TriangleT<T> t[2];
				splitLineToTriangles(t, points[0], points[1], borderWidth);
				return List< TriangleT<T> >::create(t, 2);
			}
			List< TriangleT<T> > ret;
			Vector2T<T> prevNormal;
			sl_bool flagPrevNormal = sl_false;
			PointT<T> prevPoint, prevBp1, prevBp2;
			PointT<T> firstBp1, firstBp2; // Used for CLOSE
			Vector2T<T> firstNormal; // Used for CLOSE
			for (sl_size i = 0; i < nPoints; i++) {
				const PointT<T>& pt = points[i];
				if (i) {
					Vector2T<T> dir = pt - prevPoint;
					T lenDir2p = dir.getLength2p();
					if (Math::isAlmostZero(lenDir2p)) {
						continue;
					}
					dir *= hw / Math::sqrt(lenDir2p);
					Vector2T<T> normal(-dir.y, dir.x);
					PointT<T> bp1, bp2;
					if (flagPrevNormal) {
						_getPolygonBorderPoint(prevPoint, prevNormal, normal, bp1, bp2);
						TriangleT<T> t[2];
						splitQuadrangleToTriangles(t, prevBp1, prevBp2, bp2, bp1);
						if (!(ret.addElements_NoLock(t, 2))) {
							return sl_null;
						}
					} else {
						if (CLOSE) {
							Vector2T<T> fd = (prevPoint - points[nPoints - 1]).getNormalized() * hw;
							firstNormal = Vector2T<T>(-fd.y, fd.x);
							_getPolygonBorderPoint(prevPoint, firstNormal, normal, bp1, bp2);
							firstBp1 = bp1;
							firstBp2 = bp2;
						} else {
							bp1 = prevPoint - normal;
							bp2 = prevPoint + normal;
						}
						flagPrevNormal = sl_true;
					}
					prevBp1 = bp1;
					prevBp2 = bp2;
					prevNormal = normal;
				}
				prevPoint = pt;
			}
			if (!flagPrevNormal) {
				TriangleT<T> t[2];
				splitRectangleToTriangles(t, PointT<T>(prevPoint.x - hw, prevPoint.y - hw), PointT<T>(prevPoint.x + hw, prevPoint.y + hw));
				return List< TriangleT<T> >::create(t, 2);
			}
			if (CLOSE && ret.isNotEmpty()) {
				PointT<T> bp1, bp2;
				_getPolygonBorderPoint(prevPoint, prevNormal, firstNormal, bp1, bp2);
				TriangleT<T> t[4];
				splitQuadrangleToTriangles(t, prevBp1, prevBp2, bp2, bp1);
				splitQuadrangleToTriangles(t + 2, bp1, bp2, firstBp2, firstBp1);
				if (!(ret.addElements_NoLock(t, 4))) {
					return sl_null;
				}
			} else {
				PointT<T> bp1 = prevPoint - prevNormal;
				PointT<T> bp2 = prevPoint + prevNormal;
				TriangleT<T> t[2];
				splitQuadrangleToTriangles(t, prevBp1, prevBp2, bp2, bp1);
				if (!(ret.addElements_NoLock(t, 2))) {
					return sl_null;
				}
			}
			return ret;
		}

		template <class T>
		static List< TriangleT<T> > splitPolygonBorderToTriangles(const PointT<T>* points, sl_size nPoints, T borderWidth)
		{
			return splitPolylineToTriangles<T, sl_true>(points, nPoints, borderWidth);
		}

		template <class T, sl_bool CHORD = sl_false>
		static sl_bool splitPieToTriangles(List< TriangleT<T> >& ret, T centerX, T centerY, T radiusX, T radiusY, T startRadian, T sweepRadian, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			T _n = (radiusX + radiusY) * sweepRadian / ((T)2 * chopLength);
			sl_uint32 n;
			if (_n > (T)maxChops) {
				n = maxChops;
			} else {
				n = (sl_uint32)_n;
				if (n < 4) {
					n = 4;
				}
			}
			PointT<T> center(centerX, centerY); // Used for ARC
			PointT<T> first; // Used for CHORD
			PointT<T> last;
			T fn = (T)n;
			for (sl_uint32 i = 0; i <= n; i++) {
				T angle = startRadian + sweepRadian * (T)i / fn;
				T ux = Math::cos(angle);
				T uy = Math::sin(angle);
				PointT<T> pt;
				pt.x = centerX + ux * radiusX;
				pt.y = centerY + uy * radiusY;
				if (i) {
					if (CHORD) {
						if (i >= 2) {
							if (!(ret.add_NoLock(last, pt, first))) {
								return sl_false;
							}
						}
					} else {
						if (!(ret.add_NoLock(last, pt, center))) {
							return sl_false;
						}
					}
				} else {
					if (CHORD) {
						first = pt;
					}
				}
				last = pt;
			}
			return sl_true;
		}

		template <class T, sl_bool CHORD = sl_false>
		static List< TriangleT<T> > splitPieToTriangles(T centerX, T centerY, T radiusX, T radiusY, T startRadian, T sweepRadian, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			List< TriangleT<T> > ret;
			if (splitPieToTriangles<T, CHORD>(ret, centerX, centerY, radiusX, radiusY, startRadian, sweepRadian, chopLength, maxChops)) {
				return ret;
			}
			return sl_null;
		}

		template <class T>
		static List< TriangleT<T> > splitEllipseToTriangles(T centerX, T centerY, T radiusX, T radiusY, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			return splitPieToTriangles(centerX, centerY, radiusX, radiusY, (T)0, (T)SLIB_PI_DUAL_LONG, chopLength, maxChops);
		}

		template <class T>
		static List< TriangleT<T> > splitChordToTriangles(T centerX, T centerY, T radiusX, T radiusY, T startRadian, T sweepRadian, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			return splitPieToTriangles<T, sl_true>(centerX, centerY, radiusX, radiusY, startRadian, sweepRadian, chopLength, maxChops);
		}

		template <class T, sl_bool PIE = sl_false, sl_bool CHORD = sl_false>
		static sl_bool splitArcToTriangles(List< TriangleT<T> >& ret, T centerX, T centerY, T radiusX, T radiusY, T borderWidth, T startRadian, T sweepRadian, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			T _n = (radiusX + radiusY) * sweepRadian / ((T)2 * chopLength);
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
			T w1 = radiusX - bw;
			T w2 = radiusX + bw;
			T h1 = radiusY - bw;
			T h2 = radiusY + bw;
			PointT<T> first; // Used for PIE, CHORD
			PointT<T> last1, last2;
			T fn = (T)n;
			for (sl_uint32 i = 0; i <= n; i++) {
				T angle = startRadian + sweepRadian * (T)i / fn;
				T ux = Math::cos(angle);
				T uy = Math::sin(angle);
				PointT<T> pt1, pt2;
				pt1.x = centerX + ux * w1;
				pt1.y = centerY + uy * h1;
				pt2.x = centerX + ux * w2;
				pt2.y = centerY + uy * h2;
				if (i) {
					if (!(ret.add_NoLock(last2, pt2, last1))) {
						return sl_false;
					}
					if (!(ret.add_NoLock(last1, pt2, pt1))) {
						return sl_false;
					}
					if (PIE || CHORD) {
						if (i == n) {
							PointT<T> pt;
							pt.x = centerX + ux * radiusX;
							pt.y = centerY + uy * radiusY;
							if (PIE) {
								PointT<T> center(centerX, centerY);
								TriangleT<T> t[2];
								splitLineToTriangles(t, first, center, borderWidth);
								if (!(ret.addElements_NoLock(t, 2))) {
									return sl_false;
								}
								splitLineToTriangles(t, pt, center, borderWidth);
								if (!(ret.addElements_NoLock(t, 2))) {
									return sl_false;
								}
							} else {
								TriangleT<T> t[2];
								splitLineToTriangles(t, first, pt, borderWidth);
								if (!(ret.addElements_NoLock(t, 2))) {
									return sl_false;
								}
							}
						}
					}
				} else {
					if (PIE || CHORD) {
						first.x = centerX + ux * radiusX;
						first.y = centerY + uy * radiusY;
					}
				}
				last1 = pt1;
				last2 = pt2;
			}
			return sl_true;
		}

		template <class T, sl_bool PIE = sl_false, sl_bool CHORD = sl_false>
		static List< TriangleT<T> > splitArcToTriangles(T centerX, T centerY, T radiusX, T radiusY, T borderWidth, T startRadian, T sweepRadian, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			List< TriangleT<T> > ret;
			if (splitArcToTriangles<T, PIE, CHORD>(ret, centerX, centerY, radiusX, radiusY, borderWidth, startRadian, sweepRadian, chopLength, maxChops)) {
				return ret;
			}
			return sl_null;
		}

		template <class T>
		static List< TriangleT<T> > splitEllipseBorderToTriangles(T centerX, T centerY, T radiusX, T radiusY, T borderWidth, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			return splitArcToTriangles(centerX, centerY, radiusX, radiusY, borderWidth, (T)0, (T)SLIB_PI_DUAL_LONG, chopLength, maxChops);
		}

		template <class T>
		static List< TriangleT<T> > splitPieBorderToTriangles(T centerX, T centerY, T radiusX, T radiusY, T borderWidth, T startRadian, T sweepRadian, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			return splitArcToTriangles<T, sl_true, sl_false>(centerX, centerY, radiusX, radiusY, borderWidth, startRadian, sweepRadian, chopLength, maxChops);
		}

		template <class T>
		static List< TriangleT<T> > splitChordBorderToTriangles(T centerX, T centerY, T radiusX, T radiusY, T borderWidth, T startRadian, T sweepRadian, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			return splitArcToTriangles<T, sl_false, sl_true>(centerX, centerY, radiusX, radiusY, borderWidth, startRadian, sweepRadian, chopLength, maxChops);
		}

		template <class T>
		static List< TriangleT<T> > splitRoundRectToTriangles(T centerX, T centerY, T width, T height, T radiusX, T radiusY, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			T hw = width / (T)2;
			T hh = height / (T)2;
			T iw = hw - radiusX;
			T ih = hh - radiusY;
			List< TriangleT<T> > ret;
			TriangleT<T> t[2];
			splitRectangleToTriangles(t, PointT<T>(centerX - iw, centerY - hh), PointT<T>(centerX + iw, centerY + hh));
			if (!(ret.addElements_NoLock(t, 2))) {
				return sl_null;
			}
			splitRectangleToTriangles(t, PointT<T>(centerX - hw, centerY - ih), PointT<T>(centerX - iw, centerY + ih));
			if (!(ret.addElements_NoLock(t, 2))) {
				return sl_null;
			}
			splitRectangleToTriangles(t, PointT<T>(centerX + iw, centerY - ih), PointT<T>(centerX + hw, centerY + ih));
			if (!(ret.addElements_NoLock(t, 2))) {
				return sl_null;
			}
			maxChops >>= 2;
			if (!(splitPieToTriangles<T, sl_false>(ret, centerX - iw, centerY - ih, radiusX, radiusY, (T)SLIB_PI_LONG, (T)SLIB_PI_HALF_LONG, chopLength, maxChops))) {
				return sl_null;
			}
			if (!(splitPieToTriangles<T, sl_false>(ret, centerX + iw, centerY - ih, radiusX, radiusY, (T)(SLIB_PI_LONG + SLIB_PI_HALF_LONG), (T)SLIB_PI_HALF_LONG, chopLength, maxChops))) {
				return sl_null;
			}
			if (!(splitPieToTriangles<T, sl_false>(ret, centerX - iw, centerY + ih, radiusX, radiusY, (T)SLIB_PI_HALF_LONG, (T)SLIB_PI_HALF_LONG, chopLength, maxChops))) {
				return sl_null;
			}
			if (!(splitPieToTriangles<T, sl_false>(ret, centerX + iw, centerY + ih, radiusX, radiusY, (T)0, (T)SLIB_PI_HALF_LONG, chopLength, maxChops))) {
				return sl_null;
			}
			return ret;
		}

		template <class T>
		static List< TriangleT<T> > splitRoundRectBorderToTriangles(T centerX, T centerY, T width, T height, T radiusX, T radiusY, T borderWidth, T chopLength = (T)1, sl_uint32 maxChops = 1024)
		{
			T hw = width / (T)2;
			T hh = height / (T)2;
			T iw = hw - radiusX;
			T ih = hh - radiusY;
			T bw = borderWidth / (T)2;
			List< TriangleT<T> > ret;
			TriangleT<T> t[2];
			splitRectangleToTriangles(t, PointT<T>(centerX - iw, centerY - hh - bw), PointT<T>(centerX + iw, centerY - hh + bw));
			if (!(ret.addElements_NoLock(t, 2))) {
				return sl_null;
			}
			splitRectangleToTriangles(t, PointT<T>(centerX - iw, centerY + hh - bw), PointT<T>(centerX + iw, centerY + hh + bw));
			if (!(ret.addElements_NoLock(t, 2))) {
				return sl_null;
			}
			splitRectangleToTriangles(t, PointT<T>(centerX - hw - bw, centerY - ih), PointT<T>(centerX - hw + bw, centerY + ih));
			if (!(ret.addElements_NoLock(t, 2))) {
				return sl_null;
			}
			splitRectangleToTriangles(t, PointT<T>(centerX + hw - bw, centerY - ih), PointT<T>(centerX + hw + bw, centerY + ih));
			if (!(ret.addElements_NoLock(t, 2))) {
				return sl_null;
			}
			maxChops >>= 2;
			if (!(splitArcToTriangles<T, sl_false, sl_false>(ret, centerX - iw, centerY - ih, radiusX, radiusY, borderWidth, (T)SLIB_PI_LONG, (T)SLIB_PI_HALF_LONG, chopLength, maxChops))) {
				return sl_null;
			}
			if (!(splitArcToTriangles<T, sl_false, sl_false>(ret, centerX + iw, centerY - ih, radiusX, radiusY, borderWidth, (T)(SLIB_PI_LONG + SLIB_PI_HALF_LONG), (T)SLIB_PI_HALF_LONG, chopLength, maxChops))) {
				return sl_null;
			}
			if (!(splitArcToTriangles<T, sl_false, sl_false>(ret, centerX - iw, centerY + ih, radiusX, radiusY, borderWidth, (T)SLIB_PI_HALF_LONG, (T)SLIB_PI_HALF_LONG, chopLength, maxChops))) {
				return sl_null;
			}
			if (!(splitArcToTriangles<T, sl_false, sl_false>(ret, centerX + iw, centerY + ih, radiusX, radiusY, borderWidth, (T)0, (T)SLIB_PI_HALF_LONG, chopLength, maxChops))) {
				return sl_null;
			}
			return ret;
		}

	private:
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
					if (!(Math::isAlmostZero(p2.y - p1.y))) {
						sl_size iBefore = (i + nPolygonPoints - 1) % nPolygonPoints;
						if (iBefore == iIgnoreIndex) {
							iBefore = (iBefore + nPolygonPoints - 1) % nPolygonPoints;
						}
						const PointT<T>& p0 = polygonPoints[iBefore];
						if (p2.y > p1.y) {
							if (Math::isLessThanEpsilon(p0.y - p1.y)) {
								nIntersect++;
							}
						} else {
							if (Math::isLessThanEpsilon(p1.y - p0.y)) {
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
		static void _getPolygonBorderPoint(const PointT<T>& point, const Vector2T<T>& normal1, const Vector2T<T>& normal2, PointT<T>& outBorder1, PointT<T>& outBorder2)
		{
			LineT<T> line1, line2, line3, line4;
			line1.setFromPointAndNormal(point - normal1, -normal1);
			line2.setFromPointAndNormal(point - normal2, -normal2);
			line3.setFromPointAndNormal(point + normal1, normal1);
			line4.setFromPointAndNormal(point + normal2, normal2);
			if (line1.intersect(line2, &outBorder1) && line3.intersect(line4, &outBorder2)) {
				return;
			}
			outBorder1 = point - normal1;
			outBorder2 = point + normal1;
		}

	};

}

#endif