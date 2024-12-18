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

#ifndef CHECKHEADER_SLIB_MATH_TRANSFORM2D
#define CHECKHEADER_SLIB_MATH_TRANSFORM2D

#include "matrix3.h"
#include "rectangle.h"

namespace slib
{

	template <class T>
	class SLIB_EXPORT Transform2T
	{
	public:
		static void setTranslation(Matrix3T<T>& _out, T x, T y) noexcept
		{
			_out.m00 = 1; _out.m01 = 0; _out.m02 = 0;
			_out.m10 = 0; _out.m11 = 1; _out.m12 = 0;
			_out.m20 = x; _out.m21 = y; _out.m22 = 1;
		}

		static void setTranslation(Matrix3T<T>& _out, const Vector2T<T>& v) noexcept
		{
			setTranslation(_out, v.x, v.y);
		}

		static Matrix3T<T> getTranslationMatrix(T x, T y) noexcept
		{
			return {
				1, 0, 0,
				0, 1, 0,
				x, y, 1
			};
		}

		static Matrix3T<T> getTranslationMatrix(const Vector2T<T>& v) noexcept
		{
			return getTranslationMatrix(v.x, v.y);
		}

		static void translate(Matrix3T<T>& mat, T x, T y) noexcept
		{
			mat.m20 += x;
			mat.m21 += y;
		}

		static void translate(Matrix3T<T>& mat, const Vector2T<T>& v) noexcept
		{
			translate(mat, v.x, v.y);
		}

		static void preTranslate(Matrix3T<T>& mat, T x, T y) noexcept
		{
			mat.m20 += (x * mat.m00 + y * mat.m10);
			mat.m21 += (x * mat.m01 + y * mat.m11);
		}

		static void preTranslate(Matrix3T<T>& mat, const Vector2T<T>& v) noexcept
		{
			preTranslate(mat, v.x, v.y);
		}

		static sl_bool isTranslation(const Matrix3T<T>& mat) noexcept
		{
			return Math::isAlmostZero(mat.m00 - 1) && Math::isAlmostZero(mat.m11 - 1) && Math::isAlmostZero(mat.m01) && Math::isAlmostZero(mat.m10);
		}


		static void setScaling(Matrix3T<T>& _out, T sx, T sy) noexcept
		{
			_out.m00 = sx; _out.m01 = 0; _out.m02 = 0;
			_out.m10 = 0; _out.m11 = sy; _out.m12 = 0;
			_out.m20 = 0; _out.m21 = 0; _out.m22 = 1;
		}

		static void setScaling(Matrix3T<T>& _out, const Vector2T<T>& _scale) noexcept
		{
			setScaling(_out, _scale.x, _scale.y);
		}

		static void setScaling(Matrix3T<T>& _out, T cx, T cy, T sx, T sy) noexcept
		{
			setTranslation(_out, -cx, -cy);
			scale(_out, sx, sy);
			translate(_out, cx, cy);
		}

		static void setScaling(Matrix3T<T>& _out, const Vector2T<T>& center, const Vector2T<T>& _scale) noexcept
		{
			setScaling(_out, center.x, center.y, _scale.x, _scale.y);
		}

		static Matrix3T<T> getScalingMatrix(T x, T y) noexcept
		{
			return {
				x, 0, 0,
				0, y, 0,
				0, 0, 1
			};
		}

		static Matrix3T<T> getScalingMatrix(const Vector2T<T>& v) noexcept
		{
			return getScalingMatrix(v.x, v.y);
		}

		static Matrix3T<T> getScalingMatrix(T cx, T cy, T sx, T sy) noexcept
		{
			Matrix3T<T> ret;
			setScaling(ret, cx, cy, sx, sy);
			return ret;
		}

		static Matrix3T<T> getScalingMatrix(const Vector2T<T>& center, const Vector2T<T>& _scale) noexcept
		{
			return getScalingMatrix(center.x, center.y, _scale.x, _scale.y);
		}

		static void scale(Matrix3T<T>& mat, T sx, T sy) noexcept
		{
			mat.m00 *= sx;
			mat.m10 *= sx;
			mat.m20 *= sx;
			mat.m01 *= sy;
			mat.m11 *= sy;
			mat.m21 *= sy;
		}

		static void scale(Matrix3T<T>& mat, const Vector2T<T>& _scale) noexcept
		{
			scale(mat, _scale.x, _scale.y);
		}

		static void scale(Matrix3T<T>& mat, T cx, T cy, T sx, T sy) noexcept
		{
			translate(mat, -cx, -cy);
			scale(mat, sx, sy);
			translate(mat, cx, cy);
		}

		static void scale(Matrix3T<T>& mat, const Vector2T<T>& center, const Vector2T<T>& _scale) noexcept
		{
			scale(mat, center.x, center.y, _scale.x, _scale.y);
		}

		static void preScale(Matrix3T<T>& mat, T sx, T sy) noexcept
		{
			mat.m00 *= sx;
			mat.m01 *= sx;
			mat.m10 *= sy;
			mat.m11 *= sy;
		}

		static void preScale(Matrix3T<T>& mat, const Vector2T<T>& _scale) noexcept
		{
			preScale(mat, _scale.x, _scale.y);
		}

		static void preScale(Matrix3T<T>& mat, T cx, T cy, T sx, T sy) noexcept
		{
			preTranslate(mat, cx, cy);
			preScale(mat, sx, sy);
			preTranslate(mat, -cx, -cy);
		}

		static void preScale(Matrix3T<T>& mat, const Vector2T<T>& center, const Vector2T<T>& _scale) noexcept
		{
			preScale(mat, center.x, center.y, _scale.x, _scale.y);
		}


		static void setRotation(Matrix3T<T>& _out, T radians) noexcept
		{
			T c = Math::cos(radians);
			T s = Math::sin(radians);
			_out.m00 = c; _out.m01 = s; _out.m02 = 0;
			_out.m10 = -s; _out.m11 = c; _out.m12 = 0;
			_out.m20 = 0; _out.m21 = 0; _out.m22 = 1;
		}

		static void setRotation(Matrix3T<T>& _out, T cx, T cy, T radians) noexcept
		{
			setTranslation(_out, -cx, -cy);
			rotate(_out, radians);
			translate(_out, cx, cy);
		}

		static void setRotation(Matrix3T<T>& _out, const Vector2T<T>& pt, T radians) noexcept
		{
			setRotation(_out, pt.x, pt.y, radians);
		}

		static Matrix3T<T> getRotationMatrix(T radians) noexcept
		{
			T c = Math::cos(radians);
			T s = Math::sin(radians);
			return {
				c, s, 0,
				-s, c, 0,
				0, 0, 1
			};
		}

		static Matrix3T<T> getRotationMatrix(T cx, T cy, T radians) noexcept
		{
			Matrix3T<T> ret;
			setRotation(ret, cx, cy, radians);
			return ret;
		}

		static Matrix3T<T> getRotationMatrix(const Vector2T<T>& pt, T radians) noexcept
		{
			return getRotationMatrix(pt.x, pt.y, radians);
		}

		static void rotate(Matrix3T<T>& mat, T radians) noexcept
		{
			T c = Math::cos(radians);
			T s = Math::sin(radians);
			T _m00 = mat.m00 * c - mat.m01 * s;
			T _m01 = mat.m00 * s + mat.m01 * c;
			T _m10 = mat.m10 * c - mat.m11 * s;
			T _m11 = mat.m10 * s + mat.m11 * c;
			T _m20 = mat.m20 * c - mat.m21 * s;
			T _m21 = mat.m20 * s + mat.m21 * c;
			mat.m00 = _m00;
			mat.m01 = _m01;
			mat.m10 = _m10;
			mat.m11 = _m11;
			mat.m20 = _m20;
			mat.m21 = _m21;
		}

		static void rotate(Matrix3T<T>& mat, T cx, T cy, T radians) noexcept
		{
			translate(mat, -cx, -cy);
			rotate(mat, radians);
			translate(mat, cx, cy);
		}

		static void rotate(Matrix3T<T>& mat, const Vector2T<T>& pt, T radians) noexcept
		{
			rotate(mat, pt.x, pt.y, radians);
		}

		static void preRotate(Matrix3T<T>& mat, T radians) noexcept
		{
			T c = Math::cos(radians);
			T s = Math::sin(radians);
			T _m00 = c * mat.m00 + s * mat.m10;
			T _m01 = c * mat.m01 + s * mat.m11;
			T _m10 = c * mat.m10 - s * mat.m00;
			T _m11 = c * mat.m11 - s * mat.m01;
			mat.m00 = _m00;
			mat.m01 = _m01;
			mat.m10 = _m10;
			mat.m11 = _m11;
		}

		static void preRotate(Matrix3T<T>& mat, T cx, T cy, T radians) noexcept
		{
			preTranslate(mat, cx, cy);
			preRotate(mat, radians);
			preTranslate(mat, -cx, -cy);
		}

		static void preRotate(Matrix3T<T>& mat, const Vector2T<T>& pt, T radians) noexcept
		{
			preRotate(mat, pt.x, pt.y, radians);
		}


		static void setSkewX(Matrix3T<T>& _out, T sx) noexcept
		{
			_out.m00 = 1; _out.m01 = 0; _out.m02 = 0;
			_out.m10 = sx; _out.m11 = 1; _out.m12 = 0;
			_out.m20 = 0; _out.m21 = 0; _out.m22 = 1;
		}

		static void setSkewY(Matrix3T<T>& _out, T sy) noexcept
		{
			_out.m00 = 1; _out.m01 = sy; _out.m02 = 0;
			_out.m10 = 0; _out.m11 = 1; _out.m12 = 0;
			_out.m20 = 0; _out.m21 = 0; _out.m22 = 1;
		}

		static Matrix3T<T> getSkewXMatrix(T x) noexcept
		{
			return {
				1, 0, 0,
				x, 1, 0,
				0, 0, 1
			};
		}

		static Matrix3T<T> getSkewYMatrix(T y) noexcept
		{
			return {
				1, y, 0,
				0, 1, 0,
				0, 0, 1
			};
		}

		static void skewX(Matrix3T<T>& mat, T sx) noexcept
		{
			mat.m00 += mat.m01 * sx;
			mat.m10 += mat.m11 * sx;
			mat.m20 += mat.m21 * sx;
		}

		static void skewY(Matrix3T<T>& mat, T sy) noexcept
		{
			mat.m01 += mat.m00 * sy;
			mat.m11 += mat.m10 * sy;
			mat.m21 += mat.m20 * sy;
		}

		static void preSkewX(Matrix3T<T>& mat, T sx) noexcept
		{
			mat.m10 += sx * mat.m00;
			mat.m11 += sx * mat.m01;
		}

		static void preSkewY(Matrix3T<T>& mat, T sy) noexcept
		{
			mat.m00 += sy * mat.m10;
			mat.m01 += sy * mat.m11;
		}


		static T getRotationAngleFromDirToDir(const Vector2T<T>& from, const Vector2T<T>& to) noexcept
		{
			return to.getAngleBetween(from);
		}

		static void setTransformFromDirToDir(Matrix3T<T>& _out, const Vector2T<T>& from, const Vector2T<T>& to) noexcept
		{
			setRotation(_out, getRotationAngleFromDirToDir(from, to));
		}

		static Matrix3T<T> getTransformMatrixFromDirToDir(const Vector2T<T>& from, const Vector2T<T>& to) noexcept
		{
			Matrix3T<T> ret;
			setTransformFromDirToDir(ret, from, to);
			return ret;
		}

		static void setTransformFromRectToRect(Matrix3T<T>& _out, const RectangleT<T>& from, const RectangleT<T>& to) noexcept
		{
			T widthFrom = from.getWidth();
			T heightFrom = from.getHeight();
			T widthTo = to.getWidth();
			T heightTo = to.getHeight();
			if (Math::isAlmostZero(widthFrom)) {
				_out.m00 = 0;
			} else {
				_out.m00 = widthTo / widthFrom;
			}
			if (Math::isAlmostZero(heightFrom)) {
				_out.m11 = 0;
			} else {
				_out.m11 = heightTo / heightFrom;
			}
			_out.m01 = 0; _out.m02 = 0;
			_out.m10 = 0; _out.m12 = 0;
			_out.m20 = to.left - from.left * _out.m00;
			_out.m21 = to.top - from.top * _out.m11;
			_out.m22 = 1;
		}

		static Matrix3T<T> getTransformMatrixFromRectToRect(const RectangleT<T>& from, const RectangleT<T>& to) noexcept
		{
			Matrix3T<T> ret;
			setTransformFromRectToRect(ret, from, to);
			return ret;
		}

		static T getXScaleFromMatrix(const Matrix3T<T>& mat) noexcept
		{
			return Math::sqrt(mat.m00 * mat.m00 + mat.m01 * mat.m01);
		}

		static T getYScaleFromMatrix(const Matrix3T<T>& mat) noexcept
		{
			return Math::sqrt(mat.m10 * mat.m10 + mat.m11 * mat.m11);
		}

		static Vector2T<T> getScaleFromMatrix(const Matrix3T<T>& mat) noexcept
		{
			return { getXScaleFromMatrix(mat), getYScaleFromMatrix(mat) };
		}

		static T getRotationAngleFromMatrix(const Matrix3T<T>& mat) noexcept
		{
			T x = mat.m00;
			T y = mat.m01;
			T cos = x / Math::sqrt(x * x + y * y);
			T a = Math::arccos(cos);
			if (y < 0) {
				a = -a;
			}
			return a;
		}

		static T getXTranslationFromMatrix(const Matrix3T<T>& mat) noexcept
		{
			return mat.m20;
		}

		static T getYTranslationFromMatrix(const Matrix3T<T>& mat) noexcept
		{
			return mat.m20;
		}

		static Vector2T<T> getTranslationFromMatrix(const Matrix3T<T>& mat) noexcept
		{
			return { mat.m20, mat.m21 };
		}

	};

	typedef Transform2T<sl_real> Transform2;

}

#endif
