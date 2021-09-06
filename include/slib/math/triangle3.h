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

#ifndef CHECKHEADER_SLIB_MATH_TRIANGLE3
#define CHECKHEADER_SLIB_MATH_TRIANGLE3

#include "plane.h"

namespace slib
{
	
	template <class T>
	class SLIB_EXPORT Triangle3T
	{
	public:
		Vector3T<T> point1;
		Vector3T<T> point2;
		Vector3T<T> point3;

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(Triangle3T)
		
		Triangle3T() = default;
		
		template <class O>
		SLIB_CONSTEXPR Triangle3T(const Triangle3T<O>& other): point1(other.point1), point2(other.point2), point3(other.point3) {}

		SLIB_CONSTEXPR Triangle3T(const Vector3T<T>& _point1, const Vector3T<T>& _point2, const Vector3T<T>& _point3): point1(_point1), point2(_point2), point3(_point3) {}

	public:
		static SLIB_CONSTEXPR Vector3T<T> getNormal(const Vector3T<T>& point1, const Vector3T<T>& point2, const Vector3T<T>& point3)
		{
			return (point1 - point2).cross(point2 - point3);
		}

		SLIB_CONSTEXPR Vector3T<T> getNormal() const
		{
			return getNormal(point1, point2, point3);
		}

		SLIB_CONSTEXPR T getSize() const
		{
			return (point1 - point2).cross(point2 - point3).getLength() / 2;
		}

		SLIB_CONSTEXPR PlaneT<T> getPlane() const
		{
			return { point1, getNormal() };
		}

		void transform(const Matrix4T<T>& mat) noexcept
		{
			point1 = mat.transformPosition(point1);
			point2 = mat.transformPosition(point2);
			point3 = mat.transformPosition(point3);
		}

		Vector3T<T> projectPoint(const Vector3T<T>& point, T* pDist = sl_null, T* pU = sl_null, T* pV = sl_null) const noexcept
		{
			const Triangle3T<T>& triangle = *this;
			Vector3T<T> P = triangle.point2 - triangle.point1;
			Vector3T<T> Q = triangle.point3 - triangle.point1;
			Vector3T<T> N = P.cross(Q);
			T LN = N.getLength();
			if (Math::isAlmostZero(LN)) {
				if (pDist) {
					*pDist = 0;
				}
				if (pU) {
					*pU = 0;
				}
				if (pV) {
					*pV = 0;
				}
				return point;
			}
			N /= LN;
			T dist = N.dot(point - triangle.point1);
			Vector3T<T> R = point + N * dist;
			if (pDist) {
				*pDist = Math::abs(dist);
			}
			Vector3T<T> K = R - triangle.point1;
			if (pU) {
				*pU = K.cross(Q).getLength() / LN;
			}
			if (pV) {
				*pV = P.cross(K).getLength() / LN;
			}
			return R;
		}

		sl_bool intersectLine(
			Line3T<T>& line,
			Vector3T<T>* outIntersectPoint = sl_null,
			T* pDist = sl_null,
			T* pU = sl_null,
			T* pV = sl_null,
			sl_bool* pFlagParallel = sl_null,
			sl_bool* pFlagExtendPoint1 = sl_null,
			sl_bool* pFlagExtendPoint2 = sl_null,
			sl_bool* pFlagExtendTriangle = sl_null) const noexcept
		{
			const Triangle3T<T>& triangle = *this;
			T L = line.getLength();
			if (Math::isAlmostZero(L)) {
				if (pFlagParallel) {
					*pFlagParallel = sl_false;
				}
				if (pFlagExtendPoint1) {
					*pFlagExtendPoint1 = sl_false;
				}
				if (pFlagExtendPoint2) {
					*pFlagExtendPoint2 = sl_false;
				}
				T dist, u, v;
				Vector3T<T> P = projectPoint(line.point1, &dist, &u, &v);
				if (Math::isAlmostZero(dist)) {
					if (outIntersectPoint) {
						*outIntersectPoint = P;
					}
					if (u >= 0 && v >= 0 && u + v <= 1) {
						if (pFlagExtendTriangle) {
							*pFlagExtendTriangle = sl_false;
						}
					} else {
						if (pFlagExtendTriangle) {
							*pFlagExtendTriangle = sl_true;
						}
					}
					return sl_true;
				} else {
					if (pFlagExtendTriangle) {
						*pFlagExtendTriangle = sl_false;
					}
					return sl_false;
				}
			}

			T dist, u, v;
			Vector3T<T> P = triangle.point2 - triangle.point1;
			Vector3T<T> Q = triangle.point3 - triangle.point1;
			Vector3T<T> D = line.getDirection() / L;
			Vector3T<T> S = line.point1;

			Vector3T<T> DQ = D.cross(Q);
			T det = DQ.dot(P);

			if (Math::isAlmostZero(det)) {
				if (pFlagParallel) {
					*pFlagParallel = sl_true;
				}
				if (pFlagExtendPoint1) {
					*pFlagExtendPoint1 = sl_false;
				}
				if (pFlagExtendPoint2) {
					*pFlagExtendPoint2 = sl_false;
				}
				T dist, u, v;
				Vector3T<T> P = projectPoint(line.point1, &dist, &u, &v);
				if (Math::isAlmostZero(dist)) {
					if (outIntersectPoint) {
						*outIntersectPoint = P;
					}
					if (u >= 0 && v >= 0 && u + v <= 1) {
						if (pFlagExtendTriangle) {
							*pFlagExtendTriangle = sl_false;
						}
					} else {
						if (pFlagExtendTriangle) {
							*pFlagExtendTriangle = sl_true;
						}
					}
					return sl_true;
				} else {
					if (pFlagExtendTriangle) {
						*pFlagExtendTriangle = sl_false;
					}
					return sl_false;
				}
			}

			if (pFlagParallel) {
				*pFlagParallel = sl_false;
			}

			Vector3T<T> vT = S - triangle.point1;
			u = vT.dot(DQ) / det;
			Vector3T<T> TP = vT.cross(P);
			v = D.dot(TP) / det;
			dist = Q.dot(TP) / det;

			if (outIntersectPoint) {
				*outIntersectPoint = line.point1 + D * dist;
			}
			if (pDist) {
				*pDist = dist;
			}
			if (pU) {
				*pU = u;
			}
			if (pV) {
				*pV = v;
			}
			sl_bool flagIntersect = sl_true;
			if (dist > 0) {
				if (dist < L) {
					if (pFlagExtendPoint1) {
						*pFlagExtendPoint1 = sl_false;
					}
					if (pFlagExtendPoint2) {
						*pFlagExtendPoint2 = sl_false;
					}
				} else {
					if (pFlagExtendPoint1) {
						*pFlagExtendPoint1 = sl_false;
					}
					if (pFlagExtendPoint2) {
						*pFlagExtendPoint2 = sl_true;
					}
					flagIntersect = sl_false;
				}
			} else {
				if (pFlagExtendPoint1) {
					*pFlagExtendPoint1 = sl_true;
				}
				if (pFlagExtendPoint2) {
					*pFlagExtendPoint2 = sl_false;
				}
				flagIntersect = sl_false;
			}
			if (u >= 0 && v >= 0 && u + v <= 1) {
				if (pFlagExtendTriangle) {
					*pFlagExtendTriangle = sl_false;
				}
			} else {
				if (pFlagExtendTriangle) {
					*pFlagExtendTriangle = sl_true;
				}
				flagIntersect = sl_false;
			}
			return flagIntersect;
		}

		sl_bool intersectPlane(
			const PlaneT<T>& plane,
			Line3T<T>* outLine = sl_null,
			sl_uint32* outLineNo1 = sl_null,
			sl_uint32* outLineNo2 = sl_null) const noexcept
		{
			const Triangle3T<T>& triangle = *this;
			Line3T<T> line1(triangle.point1, triangle.point2);
			Line3T<T> line2(triangle.point2, triangle.point3);
			Line3T<T> line3(triangle.point3, triangle.point1);
			Vector3T<T> v1, v2;
			if (plane.intersectLine(line1, &v1)) {
				if (plane.intersectLine(line2, &v2)) {
					if (outLine) {
						outLine->point1 = v1;
						outLine->point2 = v2;
					}
					if (outLineNo1) {
						*outLineNo1 = 0;
					}
					if (outLineNo2) {
						*outLineNo2 = 1;
					}
					return sl_true;
				} else if (plane.intersectLine(line3, &v2)) {
					if (outLine) {
						outLine->point1 = v2;
						outLine->point2 = v1;
					}
					if (outLineNo1) {
						*outLineNo1 = 2;
					}
					if (outLineNo2) {
						*outLineNo2 = 0;
					}
					return sl_true;
				}
			} else if (plane.intersectLine(line2, &v1)) {
				if (plane.intersectLine(line3, &v2)) {
					if (outLine) {
						outLine->point1 = v1;
						outLine->point2 = v2;
					}
					if (outLineNo1) {
						*outLineNo1 = 1;
					}
					if (outLineNo2) {
						*outLineNo2 = 2;
					}
					return sl_true;
				}
			}
			return sl_false;
		}

		sl_bool intersectTriangle(const Triangle3T<T>& triangle, Line3T<T>* outLine) const noexcept
		{
			const Triangle3T<T>& triangle1 = *this;
			const Triangle3T<T>& triangle2 = triangle;
			Line3T<T> line11(triangle1.point1, triangle1.point2);
			Line3T<T> line12(triangle1.point2, triangle1.point3);
			Line3T<T> line13(triangle1.point3, triangle1.point1);
			Line3T<T> line21(triangle2.point1, triangle2.point2);
			Line3T<T> line22(triangle2.point2, triangle2.point3);
			Line3T<T> line23(triangle2.point3, triangle2.point1);
			Vector3T<T> v1, v2;
			sl_bool flagSuccess = sl_false;
			if (triangle2.intersectLine(line11, &v1)) {
				if (triangle2.intersectLine(line12, &v2)) {
					flagSuccess = sl_true;
				} else if (triangle2.intersectLine(line13, &v2)) {
					flagSuccess = sl_true;
				} else {
					if (triangle1.intersectLine(line21, &v2)) {
						flagSuccess = sl_true;
					} else if (triangle1.intersectLine(line22, &v2)) {
						flagSuccess = sl_true;
					} else if (triangle1.intersectLine(line23, &v2)) {
						flagSuccess = sl_true;
					}
				}
			} else if (triangle2.intersectLine(line12, &v1)) {
				if (triangle2.intersectLine(line13, &v2)) {
					flagSuccess = sl_true;
				} else {
					if (triangle1.intersectLine(line21, &v2)) {
						flagSuccess = sl_true;
					} else if (triangle1.intersectLine(line22, &v2)) {
						flagSuccess = sl_true;
					} else if (triangle1.intersectLine(line23, &v2)) {
						flagSuccess = sl_true;
					}
				}
			} else if (triangle2.intersectLine(line13, &v1)) {
				if (triangle1.intersectLine(line21, &v2)) {
					flagSuccess = sl_true;
				} else if (triangle1.intersectLine(line22, &v2)) {
					flagSuccess = sl_true;
				} else if (triangle1.intersectLine(line23, &v2)) {
					flagSuccess = sl_true;
				}
			} else {
				if (triangle1.intersectLine(line21, &v1)) {
					if (triangle1.intersectLine(line22, &v2)) {
						flagSuccess = sl_true;
					} else if (triangle1.intersectLine(line23, &v2)) {
						flagSuccess = sl_true;
					}
				} else if (triangle1.intersectLine(line22, &v1)) {
					if (triangle1.intersectLine(line23, &v2)) {
						flagSuccess = sl_true;
					}
				}
			}
			if (flagSuccess) {
				if (outLine) {
					outLine->point1 = v1;
					outLine->point2 = v2;
				}
				return sl_true;
			}
			return sl_false;
		}

	public:
		template <class O>
		Triangle3T<T>& operator=(const Triangle3T<O>& other) noexcept
		{
			point1 = other.point1;
			point2 = other.point2;
			point3 = other.point3;
			return *this;
		}
	
	};
	
	typedef Triangle3T<sl_real> Triangle3;
	typedef Triangle3T<float> Triangle3f;
	typedef Triangle3T<double> Triangle3lf;

}

#endif
