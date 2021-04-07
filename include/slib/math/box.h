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

#ifndef CHECKHEADER_SLIB_MATH_BOX
#define CHECKHEADER_SLIB_MATH_BOX

#include "matrix4.h"

#include "../core/list.h"

namespace slib
{
	
	template <class T>
	class SLIB_EXPORT BoxT
	{
	public:
		T x1, y1, z1;
		T x2, y2, z2;

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(BoxT)
		
		BoxT() noexcept = default;

		template <class O>
		BoxT(const BoxT<O>& other) noexcept :
			x1((T)(other.x1)), x2((T)(other.x2)),
			y1((T)(other.y1)), y2((T)(other.y2)),
			z1((T)(other.z1)), z2((T)(other.z2))
		{}

		BoxT(T _x1, T _y1, T _z1,
			 T _x2, T _y2, T _z2) noexcept
			:
			x1(_x1), y1(_y1), z1(_z1),
			x2(_x2), y2(_y2), z2(_z2)
		{}

	public:
		static const BoxT<T>& zero() noexcept
		{
			static SLIB_ALIGN(8) T _zero[6] = { 0, 0, 0, 0, 0, 0 };
			return *((BoxT*)((void*)_zero));
		}

		Vector3T<T> getStart() const noexcept
		{
			return { x1, y1, z1 };
		}

		void setStart(T x, T y, T z) noexcept
		{
			x1 = x;
			y1 = y;
			z1 = z;
		}

		void setStart(const Vector3T<T>& v) noexcept
		{
			x1 = v.x;
			y1 = v.y;
			z1 = v.z;
		}

		Vector3T<T> getEnd() const noexcept
		{
			return { x2, y2, z2 };
		}

		void setEnd(T x, T y, T z) noexcept
		{
			x2 = x;
			y2 = y;
			z2 = z;
		}

		void setEnd(const Vector3T<T>& v) noexcept
		{
			x2 = v.x;
			y2 = v.y;
			z2 = v.z;
		}

		Vector3T<T> getSize() const noexcept
		{
			return { x2 - x1, y2 - y1, z2 - z1 };
		}

		Vector3T<T> getCenter() const noexcept
		{
			return { (x2 + x1) / 2, (y2 + y1) / 2, (z2 + z1) / 2 };
		}

		void setZero() noexcept
		{
			x1 = 0; y1 = 0; z1 = 0;
			x2 = 0; y2 = 0; z2 = 0;
		}

		sl_bool containsPoint(T x, T y, T z) const noexcept
		{
			return
				x >= x1 && x <= x2 &&
				y >= y1 && y <= y2 &&
				z >= z1 && z <= z2;
		}

		sl_bool containsPoint(const Vector3T<T>& pt) const noexcept
		{
			return
				pt.x >= x1 && pt.x <= x2 &&
				pt.y >= y1 && pt.y <= y2 &&
				pt.z >= z1 && pt.z <= z2;
		}

		sl_bool containsBox(const BoxT<T>& other) const noexcept
		{
			return
				x1 <= other.x2 && x2 >= other.x1 &&
				y1 <= other.y2 && y2 >= other.y1 &&
				z1 <= other.z2 && z2 >= other.z1;
		}

		void setFromPoint(T x, T y, T z) noexcept
		{
			x1 = x2 = x;
			y1 = y2 = y;
			z1 = z2 = z;
		}

		void setFromPoint(const Vector3T<T>& pt) noexcept
		{
			setFromPoint(pt.x, pt.y, pt.z);
		}

		void mergePoint(T x, T y, T z) noexcept
		{
			if (x1 > x) {
				x1 = x;
			}
			if (x2 < x) {
				x2 = x;
			}
			if (y1 > y) {
				y1 = y;
			}
			if (y2 < y) {
				y2 = y;
			}
			if (z1 > z) {
				z1 = z;
			}
			if (z2 < z) {
				z2 = z;
			}
		}

		void mergePoint(const Vector3T<T>& pt) noexcept
		{
			mergePoint(pt.x, pt.y, pt.z);
		}

		void mergePoints(const Vector3T<T>* points, sl_size count) noexcept
		{
			for (sl_size i = 0; i < count; i++) {
				const Vector3T<T>& v = points[i];
				if (x1 > v.x) {
					x1 = v.x;
				}
				if (x2 < v.x) {
					x2 = v.x;
				}
				if (y1 > v.y) {
					y1 = v.y;
				}
				if (y2 < v.y) {
					y2 = v.y;
				}
				if (z1 > v.z) {
					z1 = v.z;
				}
				if (z2 < v.z) {
					z2 = v.z;
				}
			}
		}

		void mergePoints(const List< Vector3T<T> >& points) noexcept
		{
			ListLocker< Vector3T<T> > list(points);
			mergePoints(list.data, list.count);
		}

		void setFromPoints(const Vector3T<T>* points, sl_size count) noexcept
		{
			if (count > 0) {
				setFromPoint(points[0]);
				if (count > 1) {
					mergePoints(points + 1, count - 1);
				}
			}
		}

		void setFromPoints(const List< Vector3T<T> >& points) noexcept
		{
			ListLocker< Vector3T<T> > list(points);
			setFromPoints(list.data, list.count);
		}

		void setFromPoints(const Vector3T<T>& pt1, const Vector3T<T>& pt2) noexcept
		{
			setFromPoint(pt1);
			mergePoint(pt2);
		}

		void mergeBox(const BoxT<T>& other) noexcept
		{
			if (x1 > other.x1) {
				x1 = other.x1;
			}
			if (x2 < other.x2) {
				x2 = other.x2;
			}
			if (y1 > other.y1) {
				y1 = other.y1;
			}
			if (y2 < other.y2) {
				y2 = other.y2;
			}
			if (z1 > other.z1) {
				z1 = other.z1;
			}
			if (z2 < other.z2) {
				z2 = other.z2;
			}
		}

		// 8 points
		void getCornerPoints(Vector3T<T>* _out) const noexcept
		{
			_out[0].x = x1; _out[0].y = y1; _out[0].z = z1;
			_out[1].x = x2; _out[1].y = y1; _out[1].z = z1;
			_out[2].x = x1; _out[2].y = y2; _out[2].z = z1;
			_out[3].x = x2; _out[3].y = y2; _out[3].z = z1;
			_out[4].x = x1; _out[4].y = y1; _out[4].z = z2;
			_out[5].x = x2; _out[5].y = y1; _out[5].z = z2;
			_out[6].x = x1; _out[6].y = y2; _out[6].z = z2;
			_out[7].x = x2; _out[7].y = y2; _out[7].z = z2;
		}

	public:
		template <class O>
		BoxT<T>& operator=(const BoxT<O>& other) noexcept
		{
			x1 = (T)(other.x1); x2 = (T)(other.x2);
			y1 = (T)(other.y1); y2 = (T)(other.y2);
			z1 = (T)(other.z1); z2 = (T)(other.z2);
			return *this;
		} 

	};
	
	typedef BoxT<sl_real> Box;
	typedef BoxT<float> Boxf;
	typedef BoxT<double> Boxlf;

}


#endif
