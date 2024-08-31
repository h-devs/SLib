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
		static List< TriangleT<T> > splitPolygonToTriangles(const PointT<T>* points, sl_size nPoints)
		{
			if (nPoints < 3) {
				return sl_null;
			}
			List< TriangleT<T> > ret;
			if (nPoints == 3) {
				if (!ret.add_NoLock(points[0], points[1], points[2])) {
					return sl_null;
				}
				return ret;
			}
			if (!(_splitPolygonToTriangles(ret, points, nPoints))) {
				return sl_null;
			}
			return ret;
		}

	private:
		template <class T>
		static sl_bool _splitPolygonToTriangles(List< TriangleT<T> >& ret, const PointT<T>* _points, sl_size _nPoints)
		{
			List<Point> pointList;
			List<LineSegment> lineList;
			{
				for (sl_size i = 0; i < _nPoints; i++) {
					LineSegment line;
					line.point1 = _points[i];
					line.point2 = _points[(i + 1) % _nPoints];

					if (!(lineList.add_NoLock(line))) {
						return sl_false;
					}

					if (!(pointList.add_NoLock(_points[i]))) {
						return sl_false;
					}
				}
			}

			// check if pointlist can form a right polygon.
			{
				sl_bool isPolygon = sl_true;
				for (sl_size i = 1; i < _nPoints - 2; i++) {
					if (lineList[i].intersect(lineList[_nPoints - 1], sl_null)) {
						isPolygon = sl_false;
						break;
					}
				}
				if (!isPolygon) {
					return sl_false;
				}
			}

			// ear cutting algorithm.
			while (pointList.getCount() > 3) {

				sl_size initCount = pointList.getCount();

				for (sl_reg i = 0; i < pointList.getCount(); i++) {

					// point_1, point_2, point_3 : three vertex of focused triangle.
					// point_0 : located just before point_1 in list.

					sl_bool earCondition1 = sl_false;
					sl_bool earCondition2 = sl_true;

					sl_reg point_0 = (i - 1) < 0 ? (pointList.getCount() - 1) : (i - 1);
					sl_reg point_1 = i;
					sl_reg point_2 = (i + 1) % pointList.getCount();
					sl_reg point_3 = (i + 2) % pointList.getCount();

					{
						Vector2 v1 = pointList[point_2] - pointList[point_1];
						Vector2 v2 = pointList[point_0] - pointList[point_1];
						Vector2 v3 = pointList[point_3] - pointList[point_1];

						sl_real angle_v1v2 = v1.getAngleBetween(v2);
						sl_real angle_v1v3 = v1.getAngleBetween(v3);
						sl_real angle_v3v2 = v3.getAngleBetween(v2);

						angle_v1v2 = angle_v1v2 < 0 ? 2 * SLIB_PI + angle_v1v2 : angle_v1v2;
						angle_v1v3 = angle_v1v3 < 0 ? 2 * SLIB_PI + angle_v1v3 : angle_v1v3;
						angle_v3v2 = angle_v3v2 < 0 ? 2 * SLIB_PI + angle_v3v2 : angle_v3v2;

						if (Math::abs(angle_v1v2 - angle_v1v3 - angle_v3v2) < 0.0001f) {
							earCondition1 = sl_true;
						}
					}
					{
						LineSegment earLine;
						earLine.point1 = pointList[point_1];
						earLine.point2 = pointList[point_3];
						for (sl_size i = 0; i < lineList.getCount(); i++) {
							Point intersectPoint;
							if (earLine.intersect(lineList[i], &intersectPoint)) {
								if (intersectPoint.isAlmostEqual(earLine.point1) || intersectPoint.isAlmostEqual(earLine.point2)) {
									continue;
								}
								earCondition2 = sl_false;
								break;
							}
						}

					}
					if (earCondition1 && earCondition2) {
						if (!ret.add_NoLock(pointList[point_1], pointList[point_2], pointList[point_3])) {
							return sl_false;
						}
						Point temp;
						pointList.removeAt_NoLock(point_2);
						break;
					}
				}
				sl_size changedCount = pointList.getCount();
				if (initCount == changedCount) break;
			}
			if (!(ret.add_NoLock(pointList[0], pointList[1], pointList[2]))) {
				return sl_false;
			}
			return sl_true;
		}
	};

}

#endif