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

#ifndef CHECKHEADER_SLIB_MATH_PLANE
#define CHECKHEADER_SLIB_MATH_PLANE

#include "line3.h"

/*
	ax + by + cz + d = 0
 */

namespace slib
{

	template <class T>
	class SLIB_EXPORT PlaneT
	{
	public:
		T a;
		T b;
		T c;
		T d;

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(PlaneT)
		
		PlaneT() = default;

		template <class O>
		SLIB_CONSTEXPR PlaneT(const PlaneT<O>& other): a((T)(other.a)), b((T)(other.b)), c((T)(other.c)), d((T)(other.d)) {}

		SLIB_CONSTEXPR PlaneT(T _a, T _b, T _c, T _d): a(_a), b(_b), c(_c), d(_d) {}

		PlaneT(const Vector3T<T>& point, const Vector3T<T>& normal) noexcept
		{
			setFromPointAndNormal(point, normal);
		}

	public:
		SLIB_CONSTEXPR Vector3T<T> getNormal() const
		{
			return { a, b, c };
		}

		Vector3T<T> projectOriginOnNormalized() const noexcept
		{
			return { -a * d, -b * d, -c * d };
		}

		Vector3T<T> projectOrigin() const noexcept
		{
			T L = a * a + b * b + c * c;
			if (L > 0) {
				return { -a * d / L, -b * d / L, -c * d / L };
			} else {
				return { 0, 0, 0 };
			}
		}

		T getDistanceFromPointOnNormalized(const Vector3T<T>& pos) const noexcept
		{
			return a * pos.x + b * pos.y + c * pos.z + d;
		}

		T getDistanceFromPoint(const Vector3T<T>& pos) const noexcept
		{
			T L = a * a + b * b + c * c;
			if (L > 0) {
				L = Math::sqrt(L);
				return (a * pos.x + b * pos.y + c * pos.z + d) / L;
			} else {
				return d;
			}
		}

		Vector3T<T> projectPointOnNormalized(const Vector3T<T>& pos) const noexcept
		{
			T D = a * pos.x + b * pos.y + c * pos.z + d;
			return { pos.x - D * a, pos.y - D * b, pos.z - D * c };
		}

		Vector3T<T> projectPoint(const Vector3T<T>& pos) const noexcept
		{
			T L = a * a + b * b + c * c;
			if (L > 0) {
				T D = a * pos.x + b * pos.y + c * pos.z + d;
				return { pos.x - D * a / L, pos.y - D * b / L, pos.z - D * c / L };
			} else {
				return pos;
			}
		}

		void setFromPointAndNormal(const Vector3T<T>& point, const Vector3T<T>& normal) noexcept
		{
			a = normal.x;
			b = normal.y;
			c = normal.z;
			d = -point.dot(normal);
		}

		void normalize() noexcept
		{
			T l = Math::sqrt(a * a + b * b + c * c);
			if (l > 0) {
				a /= l;
				b /= l;
				c /= l;
				d /= l;
			}
		}

		void transform(const Matrix4T<T>& mat) noexcept
		{
			T _a = a * mat.m00 + b * mat.m10 + c * mat.m20;
			T _b = a * mat.m01 + b * mat.m11 + c * mat.m21;
			T _c = a * mat.m02 + b * mat.m12 + c * mat.m22;
			T L = a * a + b * b + c * c;
			if (L > 0) {
				T k = d / L;
				d = (k * _a + mat.m30) * _a + (k * _b + mat.m31) * _b + (k * _c + mat.m32) * _c;
				a = _a;
				b = _b;
				c = _c;
			} else {
				d = 0;
			}
		}

		// return sl_true when the plane intersects to the line segment
		sl_bool intersectLine(
			const Line3T<T>& line,
			Vector3T<T>* outIntersectPoint = sl_null,
			sl_bool* pFlagParallel = sl_null,
			sl_bool* pFlagExtendPoint1 = sl_null,
			sl_bool* pFlagExtendPoint2 = sl_null) const noexcept
		{
			// distances from line end points
			T len = line.getLength();
			T d1 = getDistanceFromPointOnNormalized(line.point1);
			if (Math::isAlmostZero(len)) {
				if (pFlagParallel) {
					*pFlagParallel = sl_false;
				}
				if (pFlagExtendPoint1) {
					*pFlagExtendPoint1 = sl_false;
				}
				if (pFlagExtendPoint2) {
					*pFlagExtendPoint2 = sl_false;
				}
				if (Math::isAlmostZero(d1)) {
					return sl_true;
				} else {
					return sl_false;
				}
			}
			T d2 = getDistanceFromPointOnNormalized(line.point2);
			T dd = d1 - d2;
			if (Math::isAlmostZero(dd)) {
				if (pFlagParallel) {
					*pFlagParallel = sl_true;
				}
				if (pFlagExtendPoint1) {
					*pFlagExtendPoint1 = sl_false;
				}
				if (pFlagExtendPoint2) {
					*pFlagExtendPoint2 = sl_false;
				}
				if (Math::isAlmostZero(d1)) {
					return sl_true;
				} else {
					return sl_false;
				}
			}
			if (pFlagParallel) {
				*pFlagParallel = sl_false;
			}
			T ratioInter = d1 / dd;
			if (outIntersectPoint) {
				*outIntersectPoint = line.point1 + line.getDirection() * ratioInter;
			}
			if (d1 * d2 <= 0) {
				if (pFlagExtendPoint1) {
					*pFlagExtendPoint1 = sl_false;
				}
				if (pFlagExtendPoint2) {
					*pFlagExtendPoint2 = sl_false;
				}
				return sl_true;
			} else {
				if (ratioInter > 0) {
					if (pFlagExtendPoint1) {
						*pFlagExtendPoint1 = sl_false;
					}
					if (pFlagExtendPoint2) {
						*pFlagExtendPoint2 = sl_true;
					}
					return sl_false;
				} else {
					if (pFlagExtendPoint1) {
						*pFlagExtendPoint1 = sl_true;
					}
					if (pFlagExtendPoint2) {
						*pFlagExtendPoint2 = sl_false;
					}
					return sl_false;
				}
			}
		}

		sl_bool intersectPlane(const PlaneT<T>& plane, Line3T<T>* outIntersectLine = sl_null, sl_bool* pFlagParallel = sl_null) const noexcept
		{
			PlaneT<T> plane1 = *this;
			PlaneT<T> plane2 = plane;
			const Vector3T<T>& N1 = plane1.getNormal();
			const Vector3T<T>& N2 = plane2.getNormal();
			T D1 = plane1.d;
			T D2 = plane2.d;
			Vector3T<T> vStart;
			Vector3T<T> vDirection = N1.cross(N2);
			if (Math::isAlmostZero(vDirection.x)) {
				if (Math::isAlmostZero(vDirection.y)) {
					if (Math::isAlmostZero(vDirection.z)) {
						if (pFlagParallel) {
							*pFlagParallel = sl_true;
						}
						plane1.normalize();
						plane2.normalize();
						if (Math::isAlmostZero(D1 - D2)) {
							return sl_true;
						} else {
							return sl_false;
						}
					} else {
						vStart.z = 0;
						T D = N1.x * N2.y - N1.y * N2.x;
						vStart.x = (-D1 * N2.y + D2 * N1.y) / D;
						vStart.y = (-N1.x*D2 + N2.x*D1) / D;
					}
				} else {
					vStart.y = 0;
					T D = N1.x * N2.z - N1.z * N2.x;
					vStart.x = (-D1 * N2.z + D2 * N1.z) / D;
					vStart.z = (-N1.x*D2 + N2.x*D1) / D;
				}
			} else {
				vStart.x = 0;
				T D = N1.y * N2.z - N1.z * N2.y;
				vStart.y = (-D1 * N2.z + D2 * N1.z) / D;
				vStart.z = (-N1.y*D2 + N2.y*D1) / D;
			}
			if (pFlagParallel) {
				*pFlagParallel = sl_false;
			}
			if (outIntersectLine) {
				outIntersectLine->point1 = vStart;
				outIntersectLine->point2 = vStart + vDirection;
			}
			return sl_true;
		}

	public:
		template <class O>
		PlaneT<T>& operator=(const PlaneT<O>& other) noexcept
		{
			a = (T)(other.a);
			b = (T)(other.b);
			c = (T)(other.c);
			d = (T)(other.d);
			return *this;
		}
	
	};
	
	typedef PlaneT<sl_real> Plane;
	typedef PlaneT<float> Planef;
	typedef PlaneT<double> Planelf;

}

#endif
