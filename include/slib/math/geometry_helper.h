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

namespace slib
{

	class SLIB_EXPORT GeometryHelper
	{
	public:
		template <class T>
		static List< TriangleT<T> > splitPolygonToTriangles(const PointT<T>* points, sl_size nPoints)
		{
			sl_bool flagConvex;
			{
				List< TriangleT<T> > ret;
				if (!(_splitPolygonToTriangles(ret, points, nPoints, sl_true, sl_false, flagConvex))) {
					return sl_null;
				}
				if (flagConvex) {
					return ret;
				}
			}
			List< TriangleT<T> > ret;
			if (!(_splitPolygonToTriangles(ret, points, nPoints, sl_false, sl_true, flagConvex))) {
				return sl_null;
			}
			return ret;
		}

	private:
		template <class T>
		static sl_bool _splitPolygonToTriangles(List< TriangleT<T> >& ret, const PointT<T>* _points, sl_size _nPoints, sl_bool flagClockWise, sl_bool flagFillHollow, sl_bool& outFlagConvex)
		{
			outFlagConvex = sl_false;
			List<Point> pointList;
			{
				for (sl_size i = 0; i < _nPoints; i++) {
					if (!(pointList.add_NoLock(_points[i]))) {
						return sl_false;
					}
				}
			}
			sl_bool flagConvex = sl_true;
			sl_size indexBase = 0;
			for (;;) {
				ListElements<Point> pts(pointList);
				if (pts.count < 3) {
					break;
				}
				if (indexBase + 2 >= pts.count) {
					flagConvex = sl_false;
					if (flagFillHollow) {
						Point ptBase = *(pts.data);
						for (sl_size i = 1; i + 1 < pts.count; i++) {
							if (flagClockWise) {
								if (!(ret.add_NoLock(ptBase, pts[i + 1], pts[i]))) {
									return sl_false;
								}
							} else {
								if (!(ret.add_NoLock(ptBase, pts[i], pts[i + 1]))) {
									return sl_false;
								}
							}
						}
					}
					break;
				}
				Point ptBase = pts[indexBase];
				sl_size indexInvalid = 0;
				{
					for (sl_size i = indexBase + 1; i + 1 < pts.count; i++) {
						if (_isValidTriangle(ptBase, pts[i], pts[i + 1], flagClockWise)) {
							if (flagClockWise) {
								if (!(ret.add_NoLock(ptBase, pts[i], pts[i + 1]))) {
									return sl_false;
								}
							} else {
								if (!(ret.add_NoLock(ptBase, pts[i + 1], pts[i]))) {
									return sl_false;
								}
							}
						} else {
							indexInvalid = i;
							break;
						}
					}
				}
				if (indexInvalid == indexBase + 1) {
					indexBase = indexInvalid;
				} else {
					if (indexInvalid) {
						if (!(pointList.removeRange_NoLock(indexBase + 1, indexInvalid - indexBase - 1))) {
							return sl_false;
						}
					} else {
						if (!(pointList.removeRange_NoLock(indexBase + 1, pts.count - indexBase - 2))) {
							return sl_false;
						}
					}
					if (indexBase) {
						indexBase--;
					}
				}
			}
			outFlagConvex = flagConvex;
			return sl_true;
		}

		static sl_bool _isValidTriangle(const Point& pt1, const Point& pt2, const Point& pt3, sl_bool flagCW)
		{
			if (flagCW) {
				return Triangle::getCross(pt1, pt2, pt3) >= 0;
			} else {
				return Triangle::getCross(pt1, pt2, pt3) <= 0;
			}
		}

	};

}

#endif
