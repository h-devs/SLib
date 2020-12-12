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

#include "matrix.h"
#include "vector2.h"

#define SLIB_MATH_MATRIX_DETERMINANT2(m00,m01,m10,m11) ((m00)*(m11)-(m01)*(m10))

namespace slib
{
	
	template <class T>
	class SLIB_EXPORT MatrixT<2, 2, T>
	{
	public:
		union {
			struct {
				T m00; T m01;
				T m10; T m11;
			};
			T m[2][2];
		};
	
	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(MatrixT)
		
		MatrixT() noexcept = default;

		template <class O>
		constexpr MatrixT(const MatrixT<2, 2, O>& other) noexcept
		{
			m00 = (T)(other.m00); m01 = (T)(other.m01);
			m10 = (T)(other.m10); m11 = (T)(other.m11);
		}
	
		template <class O>
		MatrixT(const O* arr) noexcept
		{
			m00 = (T)(arr[0]); m01 = (T)(arr[1]);
			m10 = (T)(arr[2]); m11 = (T)(arr[3]);
		}

		template <class O>
		MatrixT(const VectorT<2, O>* rows) noexcept
		{
			m00 = (T)(rows[0].x); m01 = (T)(rows[0].y);
			m10 = (T)(rows[1].x); m11 = (T)(rows[1].y);
		}

		constexpr MatrixT(
			T _m00, T _m01,
			T _m10, T _m11
		) noexcept
		{
			m00 = _m00; m01 = _m01;
			m10 = _m10; m11 = _m11;
		}

	public:
		static const MatrixT& zero() noexcept
		{
			static SLIB_ALIGN(8) T _zero[4] = { 0 };
			return *(reinterpret_cast<MatrixT const*>(&_zero));
		}
	
		static const MatrixT& one() noexcept
		{
			static SLIB_ALIGN(8) T _one[4] = {
				1, 1,
				1, 1
			};
			return *(reinterpret_cast<MatrixT const*>(&_one));
		}
	
		static const MatrixT& identity() noexcept
		{
			static SLIB_ALIGN(8) T _identity[4] = {
				1, 0,
				0, 1
			};
			return *(reinterpret_cast<MatrixT const*>(&_identity));
		}

		static const MatrixT& fromArray(const T arr[4]) noexcept
		{
			return *(reinterpret_cast<MatrixT<2, 2, T> const*>(arr));
		}

		static MatrixT& fromArray(T arr[4]) noexcept
		{
			return *(reinterpret_cast<MatrixT<2, 2, T>*>(arr));
		}
	
		const VectorT<2, T>& getRow0() const noexcept
		{
			return *(reinterpret_cast<VectorT<2, T> const*>(&m00));
		}

		VectorT<2, T>& getRow0() noexcept
		{
			return *(reinterpret_cast<VectorT<2, T>*>(&m00));
		}

		void setRow0(const VectorT<2, T>& v) noexcept
		{
			m00 = v.x;
			m01 = v.y;
		}

		const VectorT<2, T>& getRow1() const noexcept
		{
			return *(reinterpret_cast<VectorT<2, T> const*>(&m10));
		}

		VectorT<2, T>& getRow1() noexcept
		{
			return *(reinterpret_cast<VectorT<2, T>*>(&m10));
		}

		void setRow1(const VectorT<2, T>& v) noexcept
		{
			m10 = v.x;
			m11 = v.y;
		}

		const VectorT<2, T>& getRow(sl_uint32 index) const noexcept
		{
			return (reinterpret_cast<VectorT<2, T> const*>(this))[index];
		}

		VectorT<2, T>& getRow(sl_uint32 index) noexcept
		{
			return (reinterpret_cast<VectorT<2, T>*>(this))[index];
		}

		void setRow(sl_uint32 index, const VectorT<2, T>& v) noexcept
		{
			(reinterpret_cast<VectorT<2, T>*>(this))[index] = v;
		}
	
		VectorT<2, T> getColumn0() const noexcept
		{
			return {m00, m10};
		}

		void setColumn0(const VectorT<2, T>& v) noexcept
		{
			m00 = v.x;
			m10 = v.y;
		}

		VectorT<2, T> getColumn1() const noexcept
		{
			return {m01, m11};
		}

		void setColumn1(const VectorT<2, T>& v) noexcept
		{
			m01 = v.x;
			m11 = v.y;
		}
	
		VectorT<2, T> getColumn(sl_uint32 index) const noexcept
		{
			const T* t = &m00 + index;
			return {t[0], t[2]};
		}

		void setColumn(sl_uint32 index, const VectorT<2, T>& v) noexcept
		{
			T* t = &m00 + index;
			t[0] = v.x;
			t[2] = v.y;
		}
	
		void add(const MatrixT& other) noexcept
		{
			m00 += other.m00; m01 += other.m01;
			m10 += other.m10; m11 += other.m11;
		}

		void subtract(const MatrixT& other) noexcept
		{
			m00 -= other.m00; m01 -= other.m01;
			m10 -= other.m10; m11 -= other.m11;
		}

		void multiply(T value) noexcept
		{
			m00 *= value; m01 *= value;
			m10 *= value; m11 *= value;
		}

		void divide(T value) noexcept
		{
			m00 /= value; m01 /= value;
			m10 /= value; m11 /= value;
		}

		void divideReverse(T value) noexcept
		{
			m00 = value / m00; m01 = value / m01;
			m10 = value / m10; m11 = value / m11;
		}

		VectorT<2, T> multiplyLeft(const VectorT<2, T>& v) const noexcept
		{
			T _x = v.x * m00 + v.y * m10;
			T _y = v.x * m01 + v.y * m11;
			return {_x, _y};
		}

		VectorT<2, T> multiplyRight(const VectorT<2, T>& v) const noexcept
		{
			T _x = m00 * v.x + m01 * v.y;
			T _y = m10 * v.x + m11 * v.y;
			return {_x, _y};
		}

		void multiply(const MatrixT& m1, const MatrixT& m2) noexcept
		{
			T _m00 = m1.m00 * m2.m00 + m1.m01 * m2.m10;
			T _m01 = m1.m00 * m2.m01 + m1.m01 * m2.m11;
			T _m10 = m1.m10 * m2.m00 + m1.m11 * m2.m10;
			T _m11 = m1.m10 * m2.m01 + m1.m11 * m2.m11;
			m00 = _m00;
			m01 = _m01;
			m10 = _m10;
			m11 = _m11;
		}

		void multiply(const MatrixT& m) noexcept
		{
			multiply(*this, m);
		}

		template <sl_uint32 ROWS2>
		void multiply(const MatrixT<2, ROWS2, T>& m1, const MatrixT<ROWS2, 2, T>& m2) noexcept
		{
			priv::matrix::Multiply(*this, m1, m2);
		}

		T getDeterminant() const noexcept
		{
			return SLIB_MATH_MATRIX_DETERMINANT2(m00, m01, m10, m11);
		}

		void makeInverse() noexcept
		{
			T A00 = m11;
			T A01 = -m10;
			T A10 = -m01;
			T A11 = m00;
			T D = 1 / SLIB_MATH_MATRIX_DETERMINANT2(m00, m01, m10, m11);
			m00 = D * A00; m01 = D * A10;
			m10 = D * A01; m11 = D * A11;
		}

		MatrixT inverse() const noexcept
		{
			MatrixT ret(*this);
			ret.makeInverse();
			return ret;
		}

		void makeTranspose() noexcept
		{
			T f;
			f = m01; m01 = m10; m10 = f;
		}

		MatrixT transpose() const noexcept
		{
			MatrixT ret(*this);
			ret.makeTranspose();
			return ret;
		}

		void makeInverseTranspose() noexcept
		{
			makeInverse();
			makeTranspose();
		}

		MatrixT inverseTranspose() const noexcept
		{
			MatrixT ret(*this);
			ret.makeInverse();
			ret.makeTranspose();
			return ret;
		}
	
		MatrixT lerp(const MatrixT& target, float factor) const noexcept
		{
			return {
				SLIB_LERP(m00, target.m00, factor), SLIB_LERP(m01, target.m01, factor),
				SLIB_LERP(m10, target.m10, factor), SLIB_LERP(m11, target.m11, factor)
			};
		}

		sl_bool equals(const MatrixT& other) const noexcept
		{
			return m00 == other.m00 && m01 == other.m01 &&
				m10 == other.m10 && m11 == other.m11;
		}

		sl_bool isAlmostEqual(const MatrixT& other) const noexcept
		{
			return Math::isAlmostZero(m00 - other.m00) && Math::isAlmostZero(m01 - other.m01) &&
				Math::isAlmostZero(m10 - other.m10) && Math::isAlmostZero(m11 - other.m11);
		}

	public:
		template <class O>
		MatrixT& operator=(const MatrixT<2, 2, O>& other) noexcept
		{
			m00 = (T)(other.m00); m01 = (T)(other.m01);
			m10 = (T)(other.m10); m11 = (T)(other.m11);
			return *this;
		}

	};

	template <class T>
	using Matrix2T = MatrixT<2, 2, T>;
	
	typedef Matrix2T<sl_real> Matrix2;
	typedef Matrix2T<float> Matrix2f;
	typedef Matrix2T<double> Matrix2lf;

}

#endif
