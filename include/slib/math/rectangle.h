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

#ifndef CHECKHEADER_SLIB_MATH_RECTANGLE
#define CHECKHEADER_SLIB_MATH_RECTANGLE

#include "point.h"
#include "size.h"
#include "matrix3.h"

#include "../core/list.h"

namespace slib
{

	template <class T, class FT = T>
	class SLIB_EXPORT RectangleT
	{
	public:
		T left;
		T top;
		T right;
		T bottom;

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(RectangleT)

		RectangleT() = default;

		template <class O, class FO>
		SLIB_CONSTEXPR RectangleT(const RectangleT<O, FO>& other) noexcept: left((T)(other.left)), top((T)(other.top)), right((T)(other.right)), bottom((T)(other.bottom)) {}

		SLIB_CONSTEXPR RectangleT(T _left, T _top, T _right, T _bottom): left(_left), top(_top), right(_right), bottom(_bottom) {}

		SLIB_CONSTEXPR RectangleT(const PointT<T, FT>& leftTop, const SizeT<T, FT>& rightBottom): left(leftTop.x), top(leftTop.y), right(rightBottom.x), bottom(rightBottom.y) {}

	public:
		static const RectangleT<T, FT>& zero() noexcept
		{
			static SLIB_ALIGN(8) T _zero[4] = { 0, 0, 0, 0 };
			return *(reinterpret_cast<RectangleT<T, FT> const*>(&_zero));
		}

		SLIB_CONSTEXPR T getWidth() const
		{
			return right - left;
		}

		SLIB_CONSTEXPR T getHeight() const
		{
			return bottom - top;
		}

		void setWidth(T width) noexcept
		{
			right = left + width;
		}

		void setHeight(T height) noexcept
		{
			bottom = top + height;
		}

		SLIB_CONSTEXPR SizeT<T, FT> getSize() const
		{
			return { right - left, bottom - top };
		}

		void setSize(T width, T height) noexcept
		{
			right = left + width;
			bottom = top + height;
		}

		void setSize(const SizeT<T, FT>& size) noexcept
		{
			right = left + size.x;
			bottom = top + size.y;
		}

		SLIB_CONSTEXPR PointT<T, FT> getLocation() const
		{
			return { left, top };
		}

		void setLocation(T _x, T _y) noexcept
		{
			T w = right - left;
			T h = bottom - top;
			left = _x;
			top = _y;
			right = _x + w;
			bottom = _y + h;
		}

		void setLocation(const PointT<T, FT>& location) noexcept
		{
			T w = right - left;
			T h = bottom - top;
			left = location.x;
			top = location.y;
			right = location.x + w;
			bottom = location.y + h;
		}

		void setLocationLeft(T _x) noexcept
		{
			T w = right - left;
			left = _x;
			right = _x + w;
		}

		void setLocationTop(T _y) noexcept
		{
			T h = bottom - top;
			top = _y;
			bottom = _y + h;
		}

		void setLocationRight(T _x) noexcept
		{
			T w = right - left;
			left = _x - w;
			right = _x;
		}

		void setLocationBottom(T _y) noexcept
		{
			T h = bottom - top;
			top = _y - h;
			bottom = _y;
		}

		void translate(T tx, T ty) noexcept
		{
			left += tx;
			top += ty;
			right += tx;
			bottom += ty;
		}

		void translate(const PointT<T, FT>& t) noexcept
		{
			left += t.x;
			top += t.y;
			right += t.x;
			bottom += t.y;
		}


		SLIB_CONSTEXPR PointT<T, FT> getLeftTop() const
		{
			return { left, top };
		}

		void setLeftTop(T _x, T _y) noexcept
		{
			left = _x;
			top = _y;
		}

		void setLeftTop(const PointT<T, FT>& pt) noexcept
		{
			left = pt.x;
			top = pt.y;
		}

		SLIB_CONSTEXPR PointT<T, FT> getLeftBottom() const
		{
			return { left, bottom };
		}

		void setLeftBottom(T _x, T _y) noexcept
		{
			left = _x;
			bottom = _y;
		}

		void setLeftBottom(const PointT<T, FT>& pt) noexcept
		{
			left = pt.x;
			bottom = pt.y;
		}

		SLIB_CONSTEXPR PointT<T, FT> getRightTop() const
		{
			return { right, top };
		}

		void setRightTop(T _x, T _y) noexcept
		{
			right = _x;
			top = _y;
		}

		void setRightTop(const PointT<T, FT>& pt) noexcept
		{
			right = pt.x;
			top = pt.y;
		}

		SLIB_CONSTEXPR PointT<T, FT> getRightBottom() const
		{
			return { right, bottom };
		}

		void setRightBottom(T _x, T _y) noexcept
		{
			right = _x;
			bottom = _y;
		}

		void setRightBottom(const PointT<T, FT>& pt) noexcept
		{
			right = pt.x;
			bottom = pt.y;
		}

		SLIB_CONSTEXPR PointT<T, FT> getCenter() const
		{
			return { (left + right) / 2, (top + bottom) / 2 };
		}

		void setCenter(T _x, T _y) noexcept
		{
			T w = (right - left) / 2;
			T h = (bottom - top) / 2;
			left = _x - w;
			top = _y - h;
			right = _x + w;
			bottom = _y + h;
		}

		void setCenter(const PointT<T, FT>& pt) noexcept
		{
			T w = (right - left) / 2;
			T h = (bottom - top) / 2;
			left = pt.x - w;
			top = pt.y - h;
			right = pt.x + w;
			bottom = pt.y + h;
		}

		void setZero() noexcept
		{
			left = 0;
			top = 0;
			right = 0;
			bottom = 0;
		}

		SLIB_CONSTEXPR sl_bool containsPoint(T x, T y) const
		{
			return x >= left && x <= right && y >= top && y <= bottom;
		}

		SLIB_CONSTEXPR sl_bool containsPoint(const PointT<T, FT>& pt) const
		{
			return pt.x >= left && pt.x <= right && pt.y >= top && pt.y <= bottom;
		}

		SLIB_CONSTEXPR sl_bool containsRectangle(const RectangleT<T, FT>& other) const
		{
			return left <= other.left && right >= other.right && top <= other.top && bottom >= other.bottom;
		}

		sl_bool intersectRectangle(const RectangleT<T, FT>& other, RectangleT<T, FT>* outIntersect = sl_null) const noexcept
		{
			if (outIntersect) {
				T _left = Math::max(left, other.left);
				T _right = Math::min(right, other.right);
				T _top = Math::max(top, other.top);
				T _bottom = Math::min(bottom, other.bottom);
				outIntersect->left = _left;
				outIntersect->right = _right;
				outIntersect->top = _top;
				outIntersect->bottom = _bottom;
				return _left <= _right && _top <= _bottom;
			} else {
				return left <= other.right && right >= other.left && top <= other.bottom && bottom >= other.top;
			}
		}

		void setFromPoint(T x, T y) noexcept
		{
			left = right = x;
			top = bottom = y;
		}

		void setFromPoint(const PointT<T, FT>& pt) noexcept
		{
			left = right = pt.x;
			top = bottom = pt.y;
		}

		void mergePoint(T x, T y) noexcept
		{
			if (left > x) {
				left = x;
			}
			if (right < x) {
				right = x;
			}
			if (top > y) {
				top = y;
			}
			if (bottom < y) {
				bottom = y;
			}
		}

		void mergePoint(const PointT<T, FT>& pt) noexcept
		{
			if (left > pt.x) {
				left = pt.x;
			}
			if (right < pt.x) {
				right = pt.x;
			}
			if (top > pt.y) {
				top = pt.y;
			}
			if (bottom < pt.y) {
				bottom = pt.y;
			}
		}

		void mergePoints(const PointT<T, FT>* points, sl_size count) noexcept
		{
			for (sl_size i = 0; i < count; i++) {
				const PointT<T, FT>& v = points[i];
				if (left > v.x) {
					left = v.x;
				}
				if (right < v.x) {
					right = v.x;
				}
				if (top > v.y) {
					top = v.y;
				}
				if (bottom < v.y) {
					bottom = v.y;
				}
			}
		}

		void mergePoints(const Array< PointT<T, FT> >& points) noexcept
		{
			mergePoints(points.getData(), points.getCount());
		}

		void mergePoints(const List< PointT<T, FT> >& points) noexcept
		{
			ListLocker< PointT<T, FT> > list(points);
			mergePoints(list.data, list.count);
		}

		void setFromPoints(const PointT<T, FT>* points, sl_size count) noexcept
		{
			if (count > 0) {
				setFromPoint(points[0]);
				if (count > 1) {
					mergePoints(points + 1, count - 1);
				}
			} else {
				setZero();
			}
		}

		void setFromPoints(const Array< PointT<T, FT> >& points) noexcept
		{
			setFromPoints(points.getData(), points.getCount());
		}

		void setFromPoints(const List< PointT<T, FT> >& points) noexcept
		{
			ListLocker< PointT<T, FT> > list(points);
			setFromPoints(list.data, list.count);
		}

		void setFromPoints(const PointT<T, FT>& pt1, const PointT<T, FT>& pt2) noexcept
		{
			setFromPoint(pt1);
			mergePoint(pt2);
		}

		void mergeRectangle(const RectangleT<T, FT>& rect) noexcept
		{
			if (left > rect.left) {
				left = rect.left;
			}
			if (right < rect.right) {
				right = rect.right;
			}
			if (top > rect.top) {
				top = rect.top;
			}
			if (bottom < rect.bottom) {
				bottom = rect.bottom;
			}
		}

		// 4 points
		void getCornerPoints(PointT<T, FT>* _out) const noexcept
		{
			_out[0].x = left; _out[0].y = top;
			_out[1].x = right; _out[1].y = top;
			_out[2].x = left; _out[2].y = bottom;
			_out[3].x = right; _out[3].y = bottom;
		}

		void transform(const Matrix3T<FT>& mat) noexcept
		{
			Vector2T<T, FT> pts[4];
			getCornerPoints(pts);
			for (int i = 0; i < 4; i++) {
				pts[i] = mat.transformPosition(pts[i]);
			}
			setFromPoints(pts, 4);
		}

		SLIB_CONSTEXPR sl_bool equals(const RectangleT<T, FT>& other) const
		{
			return left == other.left && top == other.top && right == other.right && bottom == other.bottom;
		}

		SLIB_CONSTEXPR sl_bool isAlmostEqual(const RectangleT<T, FT>& other) const
		{
			return Math::isAlmostZero(left - other.left) &&
				Math::isAlmostZero(top - other.top) &&
				Math::isAlmostZero(right - other.right) &&
				Math::isAlmostZero(bottom - other.bottom);
		}

		SLIB_CONSTEXPR sl_bool isValidSize() const
		{
			return right > left && bottom > top;
		}

		sl_bool fixSizeError() noexcept
		{
			sl_bool flagFixed = sl_false;
			if (right < left) {
				flagFixed = sl_true;
				right = left;
			}
			if (bottom < top) {
				flagFixed = sl_true;
				bottom = top;
			}
			return flagFixed;
		}

		SLIB_CONSTEXPR RectangleT<T, FT> lerp(const RectangleT<T, FT>& target, float factor) const
		{
			return { (T)SLIB_LERP(left, target.left, factor), (T)SLIB_LERP(top, target.top, factor), (T)SLIB_LERP(right, target.right, factor), (T)SLIB_LERP(bottom, target.bottom, factor) };
		}

	public:
		template <class O, class FO>
		RectangleT<T, FT>& operator=(const RectangleT<O, FO>& other) noexcept
		{
			left = (T)(other.left);
			top = (T)(other.top);
			right = (T)(other.right);
			bottom = (T)(other.bottom);
			return *this;
		}

	};

	typedef RectangleT<sl_real> Rectangle;
	typedef RectangleT<float> Rectanglef;
	typedef RectangleT<double> Rectanglelf;
	typedef RectangleT<sl_int32, float> Rectanglei;
	typedef RectangleT<sl_int64, double> Rectangleli;

}

#endif
