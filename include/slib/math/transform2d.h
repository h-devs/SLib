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

#ifndef CHECKHEADER_SLIB_MATH_TRANSFORM2D
#define CHECKHEADER_SLIB_MATH_TRANSFORM2D

#include "matrix3.h"

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
			_out.m00 = 1; _out.m01 = 0; _out.m02 = 0;
			_out.m10 = 0; _out.m11 = 1; _out.m12 = 0;
			_out.m20 = v.x; _out.m21 = v.y; _out.m22 = 1;
		}

		static Matrix3T<T> getTranslationMatrix(T x, T y) noexcept
		{
			return { 1, 0, 0,
				0, 1, 0,
				x, y, 1 };
		}

		static Matrix3T<T> getTranslationMatrix(const Vector2T<T>& v) noexcept
		{
			return { 1, 0, 0,
				0, 1, 0,
				v.x, v.y, 1 };
		}

		static void translate(Matrix3T<T>& mat, T x, T y) noexcept
		{
			mat.m20 += x;
			mat.m21 += y;
		}

		static void translate(Matrix3T<T>& mat, const Vector2T<T>& v) noexcept
		{
			mat.m20 += v.x;
			mat.m21 += v.y;
		}
		
		static void preTranslate(Matrix3T<T>& mat, T x, T y) noexcept
		{
			mat.m20 += (x * mat.m00 + y * mat.m10);
			mat.m21 += (x * mat.m01 + y * mat.m11);
		}
		
		static void preTranslate(Matrix3T<T>& mat, const Vector2T<T>& v) noexcept
		{
			mat.m20 += (v.x * mat.m00 + v.y * mat.m10);
			mat.m21 += (v.x * mat.m01 + v.y * mat.m11);
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

		static void setScaling(Matrix3T<T>& _out, const Vector2T<T>& v) noexcept
		{
			_out.m00 = v.x; _out.m01 = 0; _out.m02 = 0;
			_out.m10 = 0; _out.m11 = v.y; _out.m12 = 0;
			_out.m20 = 0; _out.m21 = 0; _out.m22 = 1;
		}

		static Matrix3T<T> getScalingMatrix(T x, T y) noexcept
		{
			return { x, 0, 0,
				0, y, 0,
				0, 0, 1 };
		}

		static Matrix3T<T> getScalingMatrix(const Vector2T<T>& v) noexcept
		{
			return { v.x, 0, 0,
				0, v.y, 0,
				0, 0, 1 };
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

		static void scale(Matrix3T<T>& mat, const Vector2T<T>& v) noexcept
		{
			scale(mat, v.x, v.y);
		}

		static void preScale(Matrix3T<T>& mat, T sx, T sy) noexcept
		{
			mat.m00 *= sx;
			mat.m01 *= sx;
			mat.m10 *= sy;
			mat.m11 *= sy;
		}

		static void preScale(Matrix3T<T>& mat, const Vector2T<T>& v) noexcept
		{
			preScale(mat, v.x, v.y);
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
			return { c, s, 0,
				-s, c, 0,
				0, 0, 1 };
		}

		static Matrix3T<T> getRotationMatrix(T cx, T cy, T radians) noexcept
		{
			Matrix3T<T> ret;
			setRotation(ret, cx, cy, radians);
			return ret;
		}

		static Matrix3T<T> getRotationMatrix(const Vector2T<T>& pt, T radians) noexcept
		{
			Matrix3T<T> ret;
			setRotation(ret, pt, radians);
			return ret;
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
		
		static Vector2T<T> getScaleFromMatrix(const Matrix3T<T>& mat) noexcept
		{
			Vector2T<T> ret;
			ret.x = Math::sqrt(mat.m00 * mat.m00 + mat.m01 * mat.m01);
			ret.y = Math::sqrt(mat.m10 * mat.m10 + mat.m11 * mat.m11);
			return ret;
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
		
		static Vector2T<T> getTranslationFromMatrix(const Matrix3T<T>& mat) noexcept
		{
			Vector2T<T> ret;
			ret.x = mat.m20;
			ret.y = mat.m21;
			return ret;
		}
		
	};
	
	typedef Transform2T<sl_real> Transform2;
	typedef Transform2T<float> Transform2f;
	typedef Transform2T<double> Transform2lf;

}

#endif
