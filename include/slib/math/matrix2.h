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

#ifndef CHECKHEADER_SLIB_MATH_MATRIX2
#define CHECKHEADER_SLIB_MATH_MATRIX2

#include "definition.h"

#include "vector2.h"

#include "../core/interpolation.h"

#define SLIB_MATH_MATRIX_DETERMINANT2(m00,m01,m10,m11) ((m00)*(m11)-(m01)*(m10))

namespace slib
{
	
	template <class T>
	class SLIB_EXPORT Matrix2T
	{
	public:
		T m00; T m01;
		T m10; T m11;
	
	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(Matrix2T)
		
		SLIB_INLINE Matrix2T() noexcept = default;

		template <class O>
		SLIB_INLINE constexpr Matrix2T(const Matrix2T<O>& other) noexcept
		 : m00((T)(other.m00)), m01((T)(other.m01)),
		   m10((T)(other.m10)), m11((T)(other.m11))
		{}
	
		SLIB_INLINE constexpr Matrix2T(
			T _m00, T _m01,
			T _m10, T _m11
		) noexcept
		 : m00(_m00), m01(_m01),
		   m10(_m10), m11(_m11)
		{}

	public:
		static const Matrix2T<T>& zero() noexcept;
	
		static const Matrix2T<T>& one() noexcept;
	
		static const Matrix2T<T>& identity() noexcept;

		static const Matrix2T<T>& fromArray(const T arr[4]) noexcept;

		static Matrix2T<T>& fromArray(T arr[4]) noexcept;
	
		Vector2T<T> getRow0() const noexcept;

		void setRow0(const Vector2T<T>& v) noexcept;

		Vector2T<T> getRow1() const noexcept;

		void setRow1(const Vector2T<T>& v) noexcept;
	
		Vector2T<T> getRow(sl_uint32 index) const noexcept;

		void setRow(sl_uint32 index, const Vector2T<T>& v) noexcept;

		Vector2T<T> getColumn0() const noexcept;

		void setColumn0(const Vector2T<T>& v) noexcept;

		Vector2T<T> getColumn1() const noexcept;

		void setColumn1(const Vector2T<T>& v) noexcept;
	
		Vector2T<T> getColumn(sl_uint32 index) const noexcept;

		void setColumn(sl_uint32 index, const Vector2T<T>& v) noexcept;
	
		T getElement(sl_uint32 row, sl_uint32 column) const noexcept;

		void setElement(sl_uint32 row, sl_uint32 column, const T& v) noexcept;


		void add(const Matrix2T<T>& other) noexcept;

		void subtract(const Matrix2T<T>& other) noexcept;

		void multiply(T value) noexcept;

		void divide(T value) noexcept;

		Vector2T<T> multiplyLeft(const Vector2T<T>& v) const noexcept;

		Vector2T<T> multiplyRight(const Vector2T<T>& v) const noexcept;

		void multiply(const Matrix2T<T>& m) noexcept;

		T getDeterminant() const noexcept;

		void makeInverse() noexcept;

		Matrix2T<T> inverse() const noexcept;

		void makeTranspose() noexcept;

		Matrix2T<T> transpose() const noexcept;

		void makeInverseTranspose() noexcept;

		Matrix2T<T> inverseTranspose() const noexcept;
	
		Matrix2T<T> lerp(const Matrix2T<T>& target, float factor) const noexcept;

	public:
		template <class O>
		Matrix2T<T>& operator=(const Matrix2T<O>& other) noexcept;
	
		Matrix2T<T> operator+(const Matrix2T<T>& other) const noexcept;

		Matrix2T<T>& operator+=(const Matrix2T<T>& other) noexcept;

		Matrix2T<T> operator-(const Matrix2T<T>& other) const noexcept;

		Matrix2T<T>& operator-=(const Matrix2T<T>& other) noexcept;

		Matrix2T<T> operator-() const noexcept;
	
		Matrix2T<T> operator*(T value) const noexcept;

		Matrix2T<T>& operator*=(T value) noexcept;

		Matrix2T<T> operator/(T value) const noexcept;

		Matrix2T<T>& operator/=(T value) noexcept;
	
		Vector2T<T> operator*(const Vector2T<T>& v) const noexcept;

		Matrix2T<T> operator*(const Matrix2T<T>& other) const noexcept;

		Matrix2T<T>& operator*=(const Matrix2T<T>& other) noexcept;

		sl_bool operator==(const Matrix2T<T>& other) const noexcept;

		sl_bool operator!=(const Matrix2T<T>& other) const noexcept;
	
	};
	
	typedef Matrix2T<sl_real> Matrix2;
	typedef Matrix2T<float> Matrix2f;
	typedef Matrix2T<double> Matrix2lf;
	
	template <class T>
	Matrix2T<T> operator*(T value, const Matrix2T<T>& m) noexcept;
	
	template <class T>
	Vector2T<T> operator*(const Vector2T<T>& v, const Matrix2T<T>& m) noexcept;
	
	template <class T>
	class Interpolation< Matrix2T<T> >
	{
	public:
		static Matrix2T<T> interpolate(const Matrix2T<T>& a, const Matrix2T<T>& b, float factor) noexcept;
	};

}

#include "detail/matrix2.inc"

#endif
