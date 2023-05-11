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

#ifndef CHECKHEADER_SLIB_MATH_VIEW_FRUSTUM
#define CHECKHEADER_SLIB_MATH_VIEW_FRUSTUM

#include "plane.h"
#include "sphere.h"
#include "box.h"

/*
	ViewFrustum defined on View Coordinate
 */

namespace slib
{

	template <class T>
	class SLIB_EXPORT ViewFrustumT
	{
	public:
		Matrix4T<T> MVP;

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(ViewFrustumT)

		ViewFrustumT() = default;

		template <class O>
		SLIB_CONSTEXPR ViewFrustumT(const ViewFrustumT<O>& other): MVP(other.MVP) {}

	public:
		static const ViewFrustumT<T>& fromMVP(const Matrix4T<T>& MVP) noexcept
		{
			return *((ViewFrustumT<T>*)(void*)&MVP);
		}

#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif

		void getPlanes(PlaneT<T>& near, PlaneT<T>& far, PlaneT<T>& left, PlaneT<T>& right, PlaneT<T>& top, PlaneT<T>& bottom) const noexcept
		{
			const Matrix4T<T>& m = MVP;
			left.a = m.m03 + m.m00;
			left.b = m.m13 + m.m10;
			left.c = m.m23 + m.m20;
			left.d = m.m33 + m.m30;
			right.a = m.m03 - m.m00;
			right.b = m.m13 - m.m10;
			right.c = m.m23 - m.m20;
			right.d = m.m33 - m.m30;
			bottom.a = m.m03 + m.m01;
			bottom.b = m.m13 + m.m11;
			bottom.c = m.m23 + m.m21;
			bottom.d = m.m33 + m.m31;
			top.a = m.m03 - m.m01;
			top.b = m.m13 - m.m11;
			top.c = m.m23 - m.m21;
			top.d = m.m33 - m.m31;
			near.a = m.m02;
			near.b = m.m12;
			near.c = m.m22;
			near.d = m.m32;
			far.a = m.m03 - m.m02;
			far.b = m.m13 - m.m12;
			far.c = m.m23 - m.m22;
			far.d = m.m33 - m.m32;
		}

		void getPlanes(PlaneT<T>* _out) const noexcept
		{
			getPlanes(_out[0], _out[1], _out[2], _out[3], _out[4], _out[5]);
		}

		sl_bool containsPoint(const Vector3T<T>& pt, sl_bool flagSkipNearFar = sl_true) const noexcept
		{
			PlaneT<T> planes[6];
			getPlanes(planes);
			for (sl_uint32 i = flagSkipNearFar ? 2 : 0; i < 6; i++) {
				if (planes[i].getDistanceFromPointOnNormalized(pt) < 0) {
					return sl_false;
				}
			}
			return sl_true;
		}

		sl_bool containsFacets(const Vector3T<T>* pts, sl_uint32 n, sl_bool* pFlagIntersect = sl_null, sl_bool flagSkipNearFar = sl_true) const noexcept
		{
			PlaneT<T> planes[6];
			getPlanes(planes);
			sl_bool flagIntersect = sl_false;
			for (sl_uint32 i = flagSkipNearFar ? 2 : 0; i < 6; i++) {
				sl_uint32 nIn = 0;
				for (sl_uint32 k = 0; k < n; k++) {
					if (planes[i].getDistanceFromPointOnNormalized(pts[k]) >= 0) {
						nIn++;
					}
				}
				if (nIn == 0) {
					return sl_false;
				}
				if (nIn != n) {
					flagIntersect = sl_true;
				}
			}
			if (pFlagIntersect) {
				*pFlagIntersect = flagIntersect;
			}
			return sl_true;
		}

		sl_bool containsSphere(const SphereT<T>& sphere, sl_bool* pFlagIntersect = sl_null, sl_bool flagSkipNearFar = sl_true) const noexcept
		{
			PlaneT<T> planes[6];
			getPlanes(planes);
			sl_bool flagIntersect = sl_false;
			for (sl_uint32 i = flagSkipNearFar ? 2 : 0; i < 6; i++) {
				T d = planes[i].getDistanceFromPoint(sphere.center);
				if (d < -sphere.radius) {
					return sl_false;
				}
				if (d < sphere.radius) {
					flagIntersect = sl_true;
				}
			}
			if (pFlagIntersect) {
				*pFlagIntersect = flagIntersect;
			}
			return sl_true;
		}

		sl_bool containsBox(const BoxT<T>& box, sl_bool* pFlagIntersect = sl_null, sl_bool flagSkipNearFar = sl_true) const noexcept
		{
			Vector3T<T> corners[8];
			box.getCornerPoints(corners);
			return containsFacets(corners, 8, pFlagIntersect, flagSkipNearFar);
		}

	public:
		template <class O>
		ViewFrustumT<T>& operator=(const ViewFrustumT<O>& other) noexcept
		{
			MVP = other.MVP;
			return *this;
		}

	};

	typedef ViewFrustumT<sl_real> ViewFrustum;

}

#endif
