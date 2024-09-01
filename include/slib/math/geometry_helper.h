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

#include "triangle.h"

#include "../core/list.h"
#include "../math.h"

namespace slib
{

	class SLIB_EXPORT GeometryHelper
	{
	public:
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

	private:
		template <class T>
		static sl_bool _splitPolygonToTriangles(List< TriangleT<T> >& ret, const PointT<T>* points, sl_size nPoints, sl_bool flagOutputCW)
		{
			List< PointT<T> > pointList;
			List< LineSegmentT<T> > lineList;
			{
				for (sl_size i = 0; i < nPoints; i++) {
					LineSegmentT<T> line;
					line.point1 = points[i];
					line.point2 = points[(i + 1) % nPoints];
					if (!(lineList.add_NoLock(line))) {
						return sl_false;
					}
					if (!(pointList.add_NoLock(points[i]))) {
						return sl_false;
					}
				}
			}

			// check if pointlist can form a right polygon.
			{
				sl_bool bPolygon = sl_true;
				ListElements< LineSegmentT<T> > lines(lineList);
				for (sl_size i = 1; i < nPoints - 2; i++) {
					if (lines[i].intersect(lines[nPoints - 1], sl_null)) {
						bPolygon = sl_false;
						break;
					}
				}
				if (!bPolygon) {
					return sl_false;
				}
			}

			// ear cutting algorithm.
			while (pointList.getCount() > 3) {

				sl_bool bCutted = sl_false;

				for (sl_size i = 0; ; i++) {

					points = pointList.getData();
					nPoints = pointList.getCount();
					if (i >= nPoints) {
						break;
					}

					sl_bool bEar1 = sl_false;
					sl_bool bEar2 = sl_true;

					// iPt0 : located just before iPt1 in list.
					sl_size iPt0 = i ? i - 1 : nPoints - 1;
					// iPt1, iPt2, iPt3 : three vertex of focused triangle.
					sl_size iPt1 = i;
					sl_size iPt2 = (i + 1) % nPoints;
					sl_size iPt3 = (i + 2) % nPoints;

					{
						Vector2T<T> v1 = points[iPt2] - points[iPt1];
						Vector2T<T> v2 = points[iPt0] - points[iPt1];
						Vector2T<T> v3 = points[iPt3] - points[iPt1];

						T angle_v1v2 = v1.getAngleBetween(v2);
						T angle_v1v3 = v1.getAngleBetween(v3);
						T angle_v3v2 = v3.getAngleBetween(v2);

						angle_v1v2 = angle_v1v2 < 0 ? Math::DualPI<T>() + angle_v1v2 : angle_v1v2;
						angle_v1v3 = angle_v1v3 < 0 ? Math::DualPI<T>() + angle_v1v3 : angle_v1v3;
						angle_v3v2 = angle_v3v2 < 0 ? Math::DualPI<T>() + angle_v3v2 : angle_v3v2;

						if (Math::abs(angle_v1v2 - angle_v1v3 - angle_v3v2) < (T)0.0001) {
							bEar1 = sl_true;
						}
					}
					{
						LineSegmentT<T> earLine;
						earLine.point1 = points[iPt1];
						earLine.point2 = points[iPt3];
						ListElements< LineSegmentT<T> > lines(lineList);
						for (sl_size i = 0; i < lines.count; i++) {
							PointT<T> intersectPoint;
							if (earLine.intersect(lines[i], &intersectPoint)) {
								if (intersectPoint.isAlmostEqual(earLine.point1) || intersectPoint.isAlmostEqual(earLine.point2)) {
									continue;
								}
								bEar2 = sl_false;
								break;
							}
						}
					}
					if (bEar1 && bEar2) {
						TriangleT<T> triangle(points[iPt1], points[iPt2], points[iPt3]);
						if (flagOutputCW) {
							triangle.makeClockwise();
						}
						if (!ret.add_NoLock(triangle)) {
							return sl_false;
						}
						pointList.removeAt_NoLock(iPt2);
						bCutted = sl_true;
						break;
					}
				}
				if (!bCutted) {
					break;
				}
			}
			{
				points = pointList.getData();
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

	};

}

#endif