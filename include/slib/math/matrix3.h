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

#ifndef CHECKHEADER_SLIB_MATH_MATRIX3
#define CHECKHEADER_SLIB_MATH_MATRIX3

#include "matrix2.h"
#include "vector3.h"

#define SLIB_MATH_MATRIX_DETERMINANT3(m00,m01,m02,m10,m11,m12,m20,m21,m22) ((m00)*SLIB_MATH_MATRIX_DETERMINANT2(m11,m12,m21,m22)-(m01)*SLIB_MATH_MATRIX_DETERMINANT2(m10,m12,m20,m22)+(m02)*SLIB_MATH_MATRIX_DETERMINANT2(m10,m11,m20,m21))

namespace slib
{
	
	template <class T>
	class SLIB_EXPORT MatrixT<3, 3, T>
	{
	public:
		union {
			struct {
				T m00; T m01; T m02;
				T m10; T m11; T m12;
				T m20; T m21; T m22;
			};
			T m[3][3];
		};
	
	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(MatrixT)
		
		MatrixT() = default;
	
		template <class O>
		constexpr MatrixT(const MatrixT<3, 3, O>& other):
			m00((T)(other.m00)), m01((T)(other.m01)), m02((T)(other.m02)),
			m10((T)(other.m10)), m11((T)(other.m11)), m12((T)(other.m12)),
			m20((T)(other.m20)), m21((T)(other.m21)), m22((T)(other.m22)) {}

		constexpr MatrixT(
			T _m00, T _m01, T _m02,
			T _m10, T _m11, T _m12,
			T _m20, T _m21, T _m22):
			m00(_m00), m01(_m01), m02(_m02),
			m10(_m10), m11(_m11), m12(_m12),
			m20(_m20), m21(_m21), m22(_m22) {}
	
		constexpr MatrixT(const VectorT<3, T>& row0, const VectorT<3, T>& row1, const VectorT<3, T>& row2):
			m00(row0.x), m01(row0.y), m02(row0.z),
			m10(row1.x), m11(row1.y), m12(row1.z),
			m20(row2.x), m21(row2.y), m22(row2.z) {}

		template <class O>
		MatrixT(const O* arr) noexcept
		{
			m00 = (T)(arr[0]); m01 = (T)(arr[1]); m02 = (T)(arr[2]);
			m10 = (T)(arr[3]); m11 = (T)(arr[4]); m12 = (T)(arr[5]);
			m20 = (T)(arr[6]); m21 = (T)(arr[7]); m22 = (T)(arr[8]);
		}

		template <class O>
		MatrixT(const VectorT<3, O>* rows) noexcept
		{
			m00 = (T)(rows[0].x); m01 = (T)(rows[0].y); m02 = (T)(rows[0].z);
			m10 = (T)(rows[1].x); m11 = (T)(rows[1].y); m12 = (T)(rows[1].z);
			m20 = (T)(rows[2].x); m21 = (T)(rows[2].y); m22 = (T)(rows[2].z);
		}

	public:
		static const MatrixT& zero() noexcept
		{
			static SLIB_ALIGN(8) T _zero[9] = { 0 };
			return *(reinterpret_cast<MatrixT const*>(&_zero));
		}

		static const MatrixT& one() noexcept
		{
			static SLIB_ALIGN(8) T _one[9] = {
				1, 1, 1,
				1, 1, 1,
				1, 1, 1
			};
			return *(reinterpret_cast<MatrixT const*>(&_one));
		}

		static const MatrixT& identity() noexcept
		{
			static SLIB_ALIGN(8) T _identity[9] = {
				1, 0, 0,
				0, 1, 0,
				0, 0, 1
			};
			return *(reinterpret_cast<MatrixT const*>(&_identity));
		}

		static const MatrixT& fromArray(const T arr[9]) noexcept
		{
			return *(reinterpret_cast<MatrixT const*>(arr));
		}

		static MatrixT& fromArray(T arr[9]) noexcept
		{
			return *(reinterpret_cast<MatrixT*>(arr));
		}

	public:
		const VectorT<3, T>& getRow0() const noexcept
		{
			return *(reinterpret_cast<VectorT<3, T> const*>(&m00));
		}

		VectorT<3, T>& getRow0() noexcept
		{
			return *(reinterpret_cast<VectorT<3, T>*>(&m00));
		}

		void setRow0(const VectorT<3, T>& v) noexcept
		{
			m00 = v.x;
			m01 = v.y;
			m02 = v.z;
		}

		const VectorT<3, T>& getRow1() const noexcept
		{
			return *(reinterpret_cast<VectorT<3, T> const*>(&m10));
		}

		VectorT<3, T>& getRow1() noexcept
		{
			return *(reinterpret_cast<VectorT<3, T>*>(&m10));
		}

		void setRow1(const VectorT<3, T>& v) noexcept
		{
			m10 = v.x;
			m11 = v.y;
			m12 = v.z;
		}

		const VectorT<3, T>& getRow2() const noexcept
		{
			return *(reinterpret_cast<VectorT<3, T> const*>(&m20));
		}

		VectorT<3, T>& getRow2() noexcept
		{
			return *(reinterpret_cast<VectorT<3, T>*>(&m20));
		}

		const VectorT<3, T>& getRow(sl_uint32 index) const noexcept
		{
			return (reinterpret_cast<VectorT<3, T> const*>(this))[index];
		}

		VectorT<3, T>& getRow(sl_uint32 index) noexcept
		{
			return (reinterpret_cast<VectorT<3, T>*>(this))[index];
		}

		void setRow(sl_uint32 index, const VectorT<3, T>& v) noexcept
		{
			(reinterpret_cast<VectorT<3, T>*>(this))[index] = v;
		}

		VectorT<3, T> getColumn0() const noexcept
		{
			return {m00, m10, m20};
		}

		void setColumn0(const VectorT<3, T>& v) noexcept
		{
			m00 = v.x;
			m10 = v.y;
			m20 = v.z;
		}

		VectorT<3, T> getColumn1() const noexcept
		{
			return {m01, m11, m21};
		}

		void setColumn1(const VectorT<3, T>& v) noexcept
		{
			m01 = v.x;
			m11 = v.y;
			m21 = v.z;
		}

		VectorT<3, T> getColumn2() const noexcept
		{
			return {m02, m12, m22};
		}

		void setColumn2(const VectorT<3, T>& v) noexcept
		{
			m02 = v.x;
			m12 = v.y;
			m22 = v.z;
		}
	
		VectorT<3, T> getColumn(sl_uint32 index) const noexcept
		{
			const T* t = &m00 + index;
			return {t[0], t[3], t[6]};
		}

		void setColumn(sl_uint32 index, const VectorT<3, T>& v) noexcept
		{
			T* t = &m00 + index;
			t[0] = v.x;
			t[3] = v.y;
			t[6] = v.z;
		}

		void add(const MatrixT& other) noexcept
		{
			m00 += other.m00; m01 += other.m01; m02 += other.m02;
			m10 += other.m10; m11 += other.m11; m12 += other.m12;
			m20 += other.m20; m21 += other.m21; m22 += other.m22;
		}

		void subtract(const MatrixT& other) noexcept
		{
			m00 -= other.m00; m01 -= other.m01; m02 -= other.m02;
			m10 -= other.m10; m11 -= other.m11; m12 -= other.m12;
			m20 -= other.m20; m21 -= other.m21; m22 -= other.m22;
		}

		void multiply(T value) noexcept
		{
			m00 *= value; m01 *= value; m02 *= value;
			m10 *= value; m11 *= value; m12 *= value;
			m20 *= value; m21 *= value; m22 *= value;
		}

		void divide(T value) noexcept
		{
			m00 /= value; m01 /= value; m02 /= value;
			m10 /= value; m11 /= value; m12 /= value;
			m20 /= value; m21 /= value; m22 /= value;
		}

		void divideReverse(T value) noexcept
		{
			m00 = value / m00; m01 = value / m01; m02 = value / m02;
			m10 = value / m10; m11 = value / m11; m12 = value / m12;
			m20 = value / m20; m21 = value / m21; m22 = value / m22;
		}

		VectorT<3, T> multiplyLeft(const VectorT<3, T>& v) const noexcept
		{
			T _x = v.x * m00 + v.y * m10 + v.z * m20;
			T _y = v.x * m01 + v.y * m11 + v.z * m21;
			T _z = v.x * m02 + v.y * m12 + v.z * m22;
			return {_x, _y, _z};
		}

		VectorT<3, T> multiplyRight(const VectorT<3, T>& v) const noexcept
		{
			T _x = m00 * v.x + m01 * v.y + m02 * v.z;
			T _y = m10 * v.x + m11 * v.y + m12 * v.z;
			T _z = m20 * v.x + m21 * v.y + m22 * v.z;
			return {_x, _y, _z};
		}

		VectorT<2, T> transformPosition(T x, T y) const noexcept
		{
			T _x = x * m00 + y * m10 + m20;
			T _y = x * m01 + y * m11 + m21;
			return {_x, _y};
		}

		VectorT<2, T> transformPosition(const VectorT<2, T>& v) const noexcept
		{
			T _x = v.x * m00 + v.y * m10 + m20;
			T _y = v.x * m01 + v.y * m11 + m21;
			return {_x, _y};
		}

		VectorT<2, T> transformDirection(T x, T y) const noexcept
		{
			T _x = x * m00 + y * m10;
			T _y = x * m01 + y * m11;
			return {_x, _y};
		}

		VectorT<2, T> transformDirection(const VectorT<2, T>& v) const noexcept
		{
			T _x = v.x * m00 + v.y * m10;
			T _y = v.x * m01 + v.y * m11;
			return {_x, _y};
		}

		void multiply(const MatrixT& m1, const MatrixT& m2) noexcept
		{
			T v0, v1, v2;
			v0 = m1.m00 * m2.m00 + m1.m01 * m2.m10 + m1.m02 * m2.m20;
			v1 = m1.m00 * m2.m01 + m1.m01 * m2.m11 + m1.m02 * m2.m21;
			v2 = m1.m00 * m2.m02 + m1.m01 * m2.m12 + m1.m02 * m2.m22;
			m00 = v0; m01 = v1; m02 = v2;
			v0 = m1.m10 * m2.m00 + m1.m11 * m2.m10 + m1.m12 * m2.m20;
			v1 = m1.m10 * m2.m01 + m1.m11 * m2.m11 + m1.m12 * m2.m21;
			v2 = m1.m10 * m2.m02 + m1.m11 * m2.m12 + m1.m12 * m2.m22;
			m10 = v0; m11 = v1; m12 = v2;
			v0 = m1.m20 * m2.m00 + m1.m21 * m2.m10 + m1.m22 * m2.m20;
			v1 = m1.m20 * m2.m01 + m1.m21 * m2.m11 + m1.m22 * m2.m21;
			v2 = m1.m20 * m2.m02 + m1.m21 * m2.m12 + m1.m22 * m2.m22;
			m20 = v0; m21 = v1; m22 = v2;
		}

		void multiply(const MatrixT& m) noexcept
		{
			multiply(*this, m);
		}

		template <sl_uint32 ROWS2>
		void multiply(const MatrixT<3, ROWS2, T>& m1, const MatrixT<ROWS2, 3, T>& m2) noexcept
		{
			priv::matrix::Multiply(*this, m1, m2);
		}

		constexpr T getDeterminant() const
		{
			return SLIB_MATH_MATRIX_DETERMINANT3(m00, m01, m02,
												m10, m11, m12,
												m20, m21, m22);
		}

		void makeInverse() noexcept
		{
			T A00 = SLIB_MATH_MATRIX_DETERMINANT2(m11, m12, m21, m22);
			T A01 = -SLIB_MATH_MATRIX_DETERMINANT2(m10, m12, m20, m22);
			T A02 = SLIB_MATH_MATRIX_DETERMINANT2(m10, m11, m20, m21);
			T A10 = -SLIB_MATH_MATRIX_DETERMINANT2(m01, m02, m21, m22);
			T A11 = SLIB_MATH_MATRIX_DETERMINANT2(m00, m02, m20, m22);
			T A12 = -SLIB_MATH_MATRIX_DETERMINANT2(m00, m01, m20, m21);
			T A20 = SLIB_MATH_MATRIX_DETERMINANT2(m01, m02, m11, m12);
			T A21 = -SLIB_MATH_MATRIX_DETERMINANT2(m00, m02, m10, m12);
			T A22 = SLIB_MATH_MATRIX_DETERMINANT2(m00, m01, m10, m11);
			T D = 1 / (m00*A00 + m01*A01 + m02*A02);
			m00 = D * A00; m01 = D * A10; m02 = D * A20;
			m10 = D * A01; m11 = D * A11; m12 = D * A21;
			m20 = D * A02; m21 = D * A12; m22 = D * A22;
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
			f = m02; m02 = m20; m20 = f;
			f = m12; m12 = m21; m21 = f;
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
				SLIB_LERP(m00, target.m00, factor), SLIB_LERP(m01, target.m01, factor), SLIB_LERP(m02, target.m02, factor),
				SLIB_LERP(m10, target.m10, factor), SLIB_LERP(m11, target.m11, factor), SLIB_LERP(m12, target.m12, factor),
				SLIB_LERP(m20, target.m20, factor), SLIB_LERP(m21, target.m21, factor), SLIB_LERP(m22, target.m22, factor)
			};
		}
	
		constexpr sl_bool equals(const MatrixT& other) const
		{
			return m00 == other.m00 && m01 == other.m01 && m02 == other.m02 &&
				m10 == other.m10 && m11 == other.m11 && m12 == other.m12 &&
				m20 == other.m20 && m21 == other.m21 && m22 == other.m22;
		}

		constexpr sl_bool isAlmostEqual(const MatrixT& other) const
		{
			return Math::isAlmostZero(m00 - other.m00) && Math::isAlmostZero(m01 - other.m01) && Math::isAlmostZero(m02 - other.m02) &&
				Math::isAlmostZero(m10 - other.m10) && Math::isAlmostZero(m11 - other.m11) && Math::isAlmostZero(m12 - other.m12) &&
				Math::isAlmostZero(m20 - other.m20) && Math::isAlmostZero(m21 - other.m21) && Math::isAlmostZero(m22 - other.m22);
		}

	public:
		template <class O>
		MatrixT& operator=(const MatrixT<3, 3, O>& other) noexcept
		{
			m00 = (T)(other.m00); m01 = (T)(other.m01); m02 = (T)(other.m02);
			m10 = (T)(other.m10); m11 = (T)(other.m11); m12 = (T)(other.m12);
			m20 = (T)(other.m20); m21 = (T)(other.m21); m22 = (T)(other.m22);
			return *this;
		}

	};
	
	template <class T>
	using Matrix3T = MatrixT<3, 3, T>;
	
	typedef Matrix3T<sl_real> Matrix3;
	typedef Matrix3T<float> Matrix3f;
	typedef Matrix3T<double> Matrix3lf;
	
}

#endif
