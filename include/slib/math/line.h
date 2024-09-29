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

#ifndef CHECKHEADER_SLIB_MATH_LINE
#define CHECKHEADER_SLIB_MATH_LINE

#include "point.h"
#include "matrix3.h"

/*
	ax + by + c = 0
*/

namespace slib
{

	template <class T>
	class SLIB_EXPORT LineT
	{
	public:
		T a;
		T b;
		T c;

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(LineT)

		LineT() = default;

		template <class O>
		SLIB_CONSTEXPR LineT(const LineT<O>& other): a((T)(other.a)), b((T)(other.b)), c((T)(other.c)) {}

		SLIB_CONSTEXPR LineT(T _a, T _b, T _c): a(_a), b(_b), c(_c) {}

		LineT(const PointT<T>& point, const Vector2T<T>& dir) noexcept
		{
			setFromPointAndDirection(point, dir);
		}

	public:
		SLIB_CONSTEXPR Vector2T<T> getDirection() const
		{
			return { b, -a };
		}

		SLIB_CONSTEXPR Vector2T<T> getNormal() const
		{
			return { a, b };
		}

		Vector2T<T> projectOriginOnNormalized() const noexcept
		{
			return { -a * c, -b * c };
		}

		Vector2T<T> projectOrigin() const noexcept
		{
			T L = a * a + b * b;
			if (L > 0) {
				return { -a * c / L, -b * c / L };
			} else {
				return { 0, 0 };
			}
		}

		T getDistanceFromPointOnNormalized(const PointT<T>& pos) const noexcept
		{
			return a * pos.x + b * pos.y + c;
		}

		T getDistanceFromPoint(const PointT<T>& pos) const noexcept
		{
			T L = a * a + b * b;
			if (L > 0) {
				L = Math::sqrt(L);
				return (a * pos.x + b * pos.y + c) / L;
			} else {
				return c;
			}
		}

		Vector2T<T> projectPointOnNormalized(const PointT<T>& pos) const noexcept
		{
			T D = a * pos.x + b * pos.y + c;
			return { pos.x - D * a, pos.y - D * b };
		}

		Vector2T<T> projectPoint(const PointT<T>& pos) const noexcept
		{
			T L = a * a + b * b;
			if (L > 0) {
				T D = a * pos.x + b * pos.y + c;
				return { pos.x - D * a / L, pos.y - D * b / L };
			} else {
				return pos;
			}
		}

		sl_bool intersect(const LineT<T>& line, PointT<T>* _out = sl_null) const
		{
			T t = a * line.b - b * line.a;
			if (Math::isAlmostZero(t)) {
				return sl_false;
			}
			if (_out) {
				_out->x = (line.c * b - line.b * c) / t;
				_out->y = (line.a * c - line.c * a) / t;
			}
			return sl_true;
		}

		void setFromPointAndDirection(const PointT<T>& point, const Vector2T<T>& dir) noexcept
		{
			a = dir.y;
			b = -dir.x;
			c = -(point.x * a + point.y * b);
		}

		void setFromPointAndNormal(const PointT<T>& point, const Vector2T<T>& normal) noexcept
		{
			a = normal.x;
			b = normal.y;
			c = -point.dot(normal);
		}

		void normalize() noexcept
		{
			T l = Math::sqrt(a * a + b * b);
			a /= l;
			b /= l;
			c /= l;
		}

		void transform(const Matrix3T<T>& mat) noexcept
		{
			T _a = a * mat.m00 + b * mat.m10;
			T _b = a * mat.m01 + b * mat.m11;
			T L = a * a + b * b;
			if (L > 0) {
				T k = c / L;
				c = (k * _a + mat.m20) * _a + (k * _b + mat.m21) * _b;
				a = _a;
				b = _b;
			} else {
				c = 0;
			}
		}

	public:
		template <class O>
		LineT<T>& operator=(const LineT<O>& other) noexcept
		{
			a = (T)(other.a);
			b = (T)(other.b);
			c = (T)(other.c);
			return *this;
		}

	};

	typedef LineT<sl_real> Line;

}

#endif
