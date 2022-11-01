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

#ifndef CHECKHEADER_SLIB_MATH_MATRIX4
#define CHECKHEADER_SLIB_MATH_MATRIX4

#include "matrix3.h"
#include "vector4.h"

#define SLIB_MATH_MATRIX_DETERMINANT4(m00,m01,m02,m03,m10,m11,m12,m13,m20,m21,m22,m23,m30,m31,m32,m33) ((m00)*SLIB_MATH_MATRIX_DETERMINANT3(m11,m12,m13,m21,m22,m23,m31,m32,m33)-(m01)*SLIB_MATH_MATRIX_DETERMINANT3(m10,m12,m13,m20,m22,m23,m30,m32,m33)+(m02)*SLIB_MATH_MATRIX_DETERMINANT3(m10,m11,m13,m20,m21,m23,m30,m31,m33)-(m03)*SLIB_MATH_MATRIX_DETERMINANT3(m10,m11,m12,m20,m21,m22,m30,m31,m32))

namespace slib
{

	template <class T>
	class SLIB_EXPORT MatrixT<4, 4, T>
	{
	public:
		union {
			struct {
				T m00; T m01; T m02; T m03;
				T m10; T m11; T m12; T m13;
				T m20; T m21; T m22; T m23;
				T m30; T m31; T m32; T m33;
			};
			T m[4][4];
		};

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(MatrixT)

		MatrixT() = default;

		template <class O>
		SLIB_CONSTEXPR MatrixT(const MatrixT<4, 4, O>& other):
			m00((T)(other.m00)), m01((T)(other.m01)), m02((T)(other.m02)), m03((T)(other.m03)),
			m10((T)(other.m10)), m11((T)(other.m11)), m12((T)(other.m12)), m13((T)(other.m13)),
			m20((T)(other.m20)), m21((T)(other.m21)), m22((T)(other.m22)), m23((T)(other.m23)),
			m30((T)(other.m30)), m31((T)(other.m31)), m32((T)(other.m32)), m33((T)(other.m33)) {}

		SLIB_CONSTEXPR MatrixT(
			T _m00, T _m01, T _m02, T _m03,
			T _m10, T _m11, T _m12, T _m13,
			T _m20, T _m21, T _m22, T _m23,
			T _m30, T _m31, T _m32, T _m33):
			m00(_m00), m01(_m01), m02(_m02), m03(_m03),
			m10(_m10), m11(_m11), m12(_m12), m13(_m13),
			m20(_m20), m21(_m21), m22(_m22), m23(_m23),
			m30(_m30), m31(_m31), m32(_m32), m33(_m33) {}

		SLIB_CONSTEXPR MatrixT(const VectorT<4, T>& row0, const VectorT<4, T>& row1, const VectorT<4, T>& row2, const VectorT<4, T>& row3):
			m00(row0.x), m01(row0.y), m02(row0.z), m03(row0.w),
			m10(row1.x), m11(row1.y), m12(row1.z), m13(row1.w),
			m20(row2.x), m21(row2.y), m22(row2.z), m23(row2.w),
			m30(row3.x), m31(row3.y), m32(row3.z), m33(row3.w) {}

		template <class O>
		MatrixT(const O* arr) noexcept
		{
			m00 = (T)(arr[0]); m01 = (T)(arr[1]); m02 = (T)(arr[2]); m03 = (T)(arr[3]);
			m10 = (T)(arr[4]); m11 = (T)(arr[5]); m12 = (T)(arr[6]); m13 = (T)(arr[7]);
			m20 = (T)(arr[8]); m21 = (T)(arr[9]); m22 = (T)(arr[10]); m23 = (T)(arr[11]);
			m30 = (T)(arr[12]); m31 = (T)(arr[13]); m32 = (T)(arr[14]); m33 = (T)(arr[15]);
		}

		template <class O>
		MatrixT(const VectorT<4, O>* rows) noexcept
		{
			m00 = (T)(rows[0].x); m01 = (T)(rows[0].y); m02 = (T)(rows[0].z); m03 = (T)(rows[0].w);
			m10 = (T)(rows[1].x); m11 = (T)(rows[1].y); m12 = (T)(rows[1].z); m13 = (T)(rows[1].w);
			m20 = (T)(rows[2].x); m21 = (T)(rows[2].y); m22 = (T)(rows[2].z); m23 = (T)(rows[2].w);
			m30 = (T)(rows[3].x); m31 = (T)(rows[3].y); m32 = (T)(rows[3].z); m33 = (T)(rows[3].w);
		}

	public:
		static const MatrixT& zero() noexcept
		{
			static SLIB_ALIGN(8) T _zero[16] = { 0 };
			return *(reinterpret_cast<MatrixT const*>(&_zero));
		}

		static const MatrixT& one() noexcept
		{
			static SLIB_ALIGN(8) T _one[16] = {
				1, 1, 1, 1,
				1, 1, 1, 1,
				1, 1, 1, 1,
				1, 1, 1, 1
			};
			return *(reinterpret_cast<MatrixT const*>(&_one));
		}

		static const MatrixT& identity() noexcept
		{
			static SLIB_ALIGN(8) T _identity[16] = {
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1
			};
			return *(reinterpret_cast<MatrixT const*>(&_identity));
		}

		static const MatrixT& fromArray(const T arr[16]) noexcept
		{
			return *(reinterpret_cast<MatrixT const*>(arr));
		}

		static MatrixT& fromArray(T arr[16]) noexcept
		{
			return *(reinterpret_cast<MatrixT*>(arr));
		}

		const VectorT<4, T>& getRow0() const noexcept
		{
			return *(reinterpret_cast<VectorT<4, T> const*>(&m00));
		}

		VectorT<4, T>& getRow0() noexcept
		{
			return *(reinterpret_cast<VectorT<4, T>*>(&m00));
		}

		void setRow0(const VectorT<4, T>& v) noexcept
		{
			m00 = v.x;
			m01 = v.y;
			m02 = v.z;
			m03 = v.w;
		}

		const VectorT<4, T>& getRow1() const noexcept
		{
			return *(reinterpret_cast<VectorT<4, T> const*>(&m10));
		}

		VectorT<4, T>& getRow1() noexcept
		{
			return *(reinterpret_cast<VectorT<4, T>*>(&m10));
		}

		void setRow1(const VectorT<4, T>& v) noexcept
		{
			m10 = v.x;
			m11 = v.y;
			m12 = v.z;
			m13 = v.w;
		}

		const VectorT<4, T>& getRow2() const noexcept
		{
			return *(reinterpret_cast<VectorT<4, T> const*>(&m20));
		}

		VectorT<4, T>& getRow2() noexcept
		{
			return *(reinterpret_cast<VectorT<4, T>*>(&m20));
		}

		void setRow2(const VectorT<4, T>& v) noexcept
		{
			m20 = v.x;
			m21 = v.y;
			m22 = v.z;
			m23 = v.w;
		}

		const VectorT<4, T>& getRow3() const noexcept
		{
			return *(reinterpret_cast<VectorT<4, T> const*>(&m30));
		}

		VectorT<4, T>& getRow3() noexcept
		{
			return *(reinterpret_cast<VectorT<4, T>*>(&m30));
		}

		void setRow3(const VectorT<4, T>& v) noexcept
		{
			m30 = v.x;
			m31 = v.y;
			m32 = v.z;
			m33 = v.w;
		}

		const VectorT<4, T>& getRow(sl_uint32 index) const noexcept
		{
			return (reinterpret_cast<VectorT<4, T> const*>(this))[index];
		}

		VectorT<4, T>& getRow(sl_uint32 index) noexcept
		{
			return (reinterpret_cast<VectorT<4, T>*>(this))[index];
		}

		void setRow(sl_uint32 index, const VectorT<4, T>& v) noexcept
		{
			(reinterpret_cast<VectorT<4, T>*>(this))[index] = v;
		}

		VectorT<4, T> getColumn0() const noexcept
		{
			return {m00, m10, m20, m30};
		}

		void setColumn0(const VectorT<4, T>& v) noexcept
		{
			m00 = v.x;
			m10 = v.y;
			m20 = v.z;
			m30 = v.w;
		}

		VectorT<4, T> getColumn1() const noexcept
		{
			return {m01, m11, m21, m31};
		}

		void setColumn1(const VectorT<4, T>& v) noexcept
		{
			m01 = v.x;
			m11 = v.y;
			m21 = v.z;
			m31 = v.w;
		}

		VectorT<4, T> getColumn2() const noexcept
		{
			return {m02, m12, m22, m32};
		}

		void setColumn2(const VectorT<4, T>& v) noexcept
		{
			m02 = v.x;
			m12 = v.y;
			m22 = v.z;
			m32 = v.w;
		}

		VectorT<4, T> getColumn3() const noexcept
		{
			return {m03, m13, m23, m33};
		}

		void setColumn3(const VectorT<4, T>& v) noexcept
		{
			m03 = v.x;
			m13 = v.y;
			m23 = v.z;
			m33 = v.w;
		}

		VectorT<4, T> getColumn(sl_uint32 index) const noexcept
		{
			const T* t = &m00 + index;
			return {t[0], t[4], t[8], t[12]};
		}

		void setColumn(sl_uint32 index, const VectorT<4, T>& v) noexcept
		{
			T* t = &m00 + index;
			t[0] = v.x;
			t[4] = v.y;
			t[8] = v.z;
			t[12] = v.w;
		}

		void add(const MatrixT& other) noexcept
		{
			m00 += other.m00; m01 += other.m01; m02 += other.m02; m03 += other.m03;
			m10 += other.m10; m11 += other.m11; m12 += other.m12; m13 += other.m13;
			m20 += other.m20; m21 += other.m21; m22 += other.m22; m23 += other.m23;
			m30 += other.m30; m31 += other.m31; m32 += other.m32; m33 += other.m33;
		}

		void subtract(const MatrixT& other) noexcept
		{
			m00 -= other.m00; m01 -= other.m01; m02 -= other.m02; m03 -= other.m03;
			m10 -= other.m10; m11 -= other.m11; m12 -= other.m12; m13 -= other.m13;
			m20 -= other.m20; m21 -= other.m21; m22 -= other.m22; m23 -= other.m23;
			m30 -= other.m30; m31 -= other.m31; m32 -= other.m32; m33 -= other.m33;
		}

		void multiply(T value) noexcept
		{
			m00 *= value; m01 *= value; m02 *= value; m03 *= value;
			m10 *= value; m11 *= value; m12 *= value; m13 *= value;
			m20 *= value; m21 *= value; m22 *= value; m23 *= value;
			m30 *= value; m31 *= value; m32 *= value; m33 *= value;
		}

		void divide(T value) noexcept
		{
			m00 /= value; m01 /= value; m02 /= value; m03 /= value;
			m10 /= value; m11 /= value; m12 /= value; m13 /= value;
			m20 /= value; m21 /= value; m22 /= value; m23 /= value;
			m30 /= value; m31 /= value; m32 /= value; m33 /= value;
		}

		void divideReverse(T value) noexcept
		{
			m00 = value / m00; m01 = value / m01; m02 = value / m02; m03 = value / m03;
			m10 = value / m10; m11 = value / m11; m12 = value / m12; m13 = value / m13;
			m20 = value / m20; m21 = value / m21; m22 = value / m22; m23 = value / m23;
			m30 = value / m30; m31 = value / m31; m32 = value / m32; m33 = value / m33;
		}

		VectorT<4, T> multiplyLeft(const VectorT<4, T>& v) const noexcept
		{
			T _x = v.x * m00 + v.y * m10 + v.z * m20 + v.w * m30;
			T _y = v.x * m01 + v.y * m11 + v.z * m21 + v.w * m31;
			T _z = v.x * m02 + v.y * m12 + v.z * m22 + v.w * m32;
			T _w = v.x * m03 + v.y * m13 + v.z * m23 + v.w * m33;
			return {_x, _y, _z, _w};
		}

		VectorT<4, T> multiplyRight(const VectorT<4, T>& v) const noexcept
		{
			T _x = m00 * v.x + m01 * v.y + m02 * v.z + m03 * v.w;
			T _y = m10 * v.x + m11 * v.y + m12 * v.z + m13 * v.w;
			T _z = m20 * v.x + m21 * v.y + m22 * v.z + m23 * v.w;
			T _w = m30 * v.x + m31 * v.y + m32 * v.z + m33 * v.w;
			return {_x, _y, _z, _w};
		}

		Vector3T<T> transformPosition(T x, T y, T z) const noexcept
		{
			T _x = x * m00 + y * m10 + z * m20 + m30;
			T _y = x * m01 + y * m11 + z * m21 + m31;
			T _z = x * m02 + y * m12 + z * m22 + m32;
			return {_x, _y, _z};
		}

		Vector3T<T> transformPosition(const Vector3T<T>& v) const noexcept
		{
			T _x = v.x * m00 + v.y * m10 + v.z * m20 + m30;
			T _y = v.x * m01 + v.y * m11 + v.z * m21 + m31;
			T _z = v.x * m02 + v.y * m12 + v.z * m22 + m32;
			return {_x, _y, _z};
		}

		Vector3T<T> transformDirection(T x, T y, T z) const noexcept
		{
			T _x = x * m00 + y * m10 + z * m20;
			T _y = x * m01 + y * m11 + z * m21;
			T _z = x * m02 + y * m12 + z * m22;
			return {_x, _y, _z};
		}

		Vector3T<T> transformDirection(const Vector3T<T>& v) const noexcept
		{
			T _x = v.x * m00 + v.y * m10 + v.z * m20;
			T _y = v.x * m01 + v.y * m11 + v.z * m21;
			T _z = v.x * m02 + v.y * m12 + v.z * m22;
			return {_x, _y, _z};
		}

		void multiply(const MatrixT& m1, const MatrixT& m2) noexcept
		{
			T v0, v1, v2, v3;
			v0 = m1.m00 * m2.m00 + m1.m01 * m2.m10 + m1.m02 * m2.m20 + m1.m03 * m2.m30;
			v1 = m1.m00 * m2.m01 + m1.m01 * m2.m11 + m1.m02 * m2.m21 + m1.m03 * m2.m31;
			v2 = m1.m00 * m2.m02 + m1.m01 * m2.m12 + m1.m02 * m2.m22 + m1.m03 * m2.m32;
			v3 = m1.m00 * m2.m03 + m1.m01 * m2.m13 + m1.m02 * m2.m23 + m1.m03 * m2.m33;
			m00 = v0; m01 = v1; m02 = v2; m03 = v3;
			v0 = m1.m10 * m2.m00 + m1.m11 * m2.m10 + m1.m12 * m2.m20 + m1.m13 * m2.m30;
			v1 = m1.m10 * m2.m01 + m1.m11 * m2.m11 + m1.m12 * m2.m21 + m1.m13 * m2.m31;
			v2 = m1.m10 * m2.m02 + m1.m11 * m2.m12 + m1.m12 * m2.m22 + m1.m13 * m2.m32;
			v3 = m1.m10 * m2.m03 + m1.m11 * m2.m13 + m1.m12 * m2.m23 + m1.m13 * m2.m33;
			m10 = v0; m11 = v1; m12 = v2; m13 = v3;
			v0 = m1.m20 * m2.m00 + m1.m21 * m2.m10 + m1.m22 * m2.m20 + m1.m23 * m2.m30;
			v1 = m1.m20 * m2.m01 + m1.m21 * m2.m11 + m1.m22 * m2.m21 + m1.m23 * m2.m31;
			v2 = m1.m20 * m2.m02 + m1.m21 * m2.m12 + m1.m22 * m2.m22 + m1.m23 * m2.m32;
			v3 = m1.m20 * m2.m03 + m1.m21 * m2.m13 + m1.m22 * m2.m23 + m1.m23 * m2.m33;
			m20 = v0; m21 = v1; m22 = v2; m23 = v3;
			v0 = m1.m30 * m2.m00 + m1.m31 * m2.m10 + m1.m32 * m2.m20 + m1.m33 * m2.m30;
			v1 = m1.m30 * m2.m01 + m1.m31 * m2.m11 + m1.m32 * m2.m21 + m1.m33 * m2.m31;
			v2 = m1.m30 * m2.m02 + m1.m31 * m2.m12 + m1.m32 * m2.m22 + m1.m33 * m2.m32;
			v3 = m1.m30 * m2.m03 + m1.m31 * m2.m13 + m1.m32 * m2.m23 + m1.m33 * m2.m33;
			m30 = v0; m31 = v1; m32 = v2; m33 = v3;
		}

		void multiply(const MatrixT& m) noexcept
		{
			multiply(*this, m);
		}

		template <sl_uint32 ROWS2>
		void multiply(const MatrixT<4, ROWS2, T>& m1, const MatrixT<ROWS2, 4, T>& m2) noexcept
		{
			priv::matrix::Multiply(*this, m1, m2);
		}

		T getDeterminant() const noexcept
		{
			return SLIB_MATH_MATRIX_DETERMINANT4(m00, m01, m02, m03,
												m10, m11, m12, m13,
												m20, m21, m22, m23,
												m30, m31, m32, m33);
		}

		void makeInverse() noexcept
		{
			T A00 = SLIB_MATH_MATRIX_DETERMINANT3(m11, m12, m13, m21, m22, m23, m31, m32, m33);
			T A01 = -SLIB_MATH_MATRIX_DETERMINANT3(m10, m12, m13, m20, m22, m23, m30, m32, m33);
			T A02 = SLIB_MATH_MATRIX_DETERMINANT3(m10, m11, m13, m20, m21, m23, m30, m31, m33);
			T A03 = -SLIB_MATH_MATRIX_DETERMINANT3(m10, m11, m12, m20, m21, m22, m30, m31, m32);
			T A10 = -SLIB_MATH_MATRIX_DETERMINANT3(m01, m02, m03, m21, m22, m23, m31, m32, m33);
			T A11 = SLIB_MATH_MATRIX_DETERMINANT3(m00, m02, m03, m20, m22, m23, m30, m32, m33);
			T A12 = -SLIB_MATH_MATRIX_DETERMINANT3(m00, m01, m03, m20, m21, m23, m30, m31, m33);
			T A13 = SLIB_MATH_MATRIX_DETERMINANT3(m00, m01, m02, m20, m21, m22, m30, m31, m32);
			T A20 = SLIB_MATH_MATRIX_DETERMINANT3(m01, m02, m03, m11, m12, m13, m31, m32, m33);
			T A21 = -SLIB_MATH_MATRIX_DETERMINANT3(m00, m02, m03, m10, m12, m13, m30, m32, m33);
			T A22 = SLIB_MATH_MATRIX_DETERMINANT3(m00, m01, m03, m10, m11, m13, m30, m31, m33);
			T A23 = -SLIB_MATH_MATRIX_DETERMINANT3(m00, m01, m02, m10, m11, m12, m30, m31, m32);
			T A30 = -SLIB_MATH_MATRIX_DETERMINANT3(m01, m02, m03, m11, m12, m13, m21, m22, m23);
			T A31 = SLIB_MATH_MATRIX_DETERMINANT3(m00, m02, m03, m10, m12, m13, m20, m22, m23);
			T A32 = -SLIB_MATH_MATRIX_DETERMINANT3(m00, m01, m03, m10, m11, m13, m20, m21, m23);
			T A33 = SLIB_MATH_MATRIX_DETERMINANT3(m00, m01, m02, m10, m11, m12, m20, m21, m22);
			T D = 1 / (m00*A00 + m01*A01 + m02*A02 + m03*A03);

			m00 = A00*D; m10 = A01*D; m20 = A02*D; m30 = A03*D;
			m01 = A10*D; m11 = A11*D; m21 = A12*D; m31 = A13*D;
			m02 = A20*D; m12 = A21*D; m22 = A22*D; m32 = A23*D;
			m03 = A30*D; m13 = A31*D; m23 = A32*D; m33 = A33*D;
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
			f = m03; m03 = m30; m30 = f;
			f = m12; m12 = m21; m21 = f;
			f = m13; m13 = m31; m31 = f;
			f = m23; m23 = m32; m32 = f;
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

		SLIB_CONSTEXPR MatrixT lerp(const MatrixT& target, float factor) const
		{
			return {
				SLIB_LERP(m00, target.m00, factor), SLIB_LERP(m01, target.m01, factor), SLIB_LERP(m02, target.m02, factor), SLIB_LERP(m03, target.m03, factor),
				SLIB_LERP(m10, target.m10, factor), SLIB_LERP(m11, target.m11, factor), SLIB_LERP(m12, target.m12, factor), SLIB_LERP(m13, target.m13, factor),
				SLIB_LERP(m20, target.m20, factor), SLIB_LERP(m21, target.m21, factor), SLIB_LERP(m22, target.m22, factor), SLIB_LERP(m23, target.m23, factor),
				SLIB_LERP(m30, target.m30, factor), SLIB_LERP(m31, target.m31, factor), SLIB_LERP(m32, target.m32, factor), SLIB_LERP(m33, target.m33, factor)};
		}

		SLIB_CONSTEXPR sl_bool equals(const MatrixT& other) const
		{
			return m00 == other.m00 && m01 == other.m01 && m02 == other.m02 && m03 == other.m03 &&
				m10 == other.m10 && m11 == other.m11 && m12 == other.m12 && m13 == other.m13 &&
				m20 == other.m20 && m21 == other.m21 && m22 == other.m22 && m23 == other.m23 &&
				m30 == other.m30 && m31 == other.m31 && m32 == other.m32 && m33 == other.m33;
		}

		SLIB_CONSTEXPR sl_bool isAlmostEqual(const MatrixT& other) const
		{
			return Math::isAlmostZero(m00 - other.m00) && Math::isAlmostZero(m01 - other.m01) && Math::isAlmostZero(m02 - other.m02) && Math::isAlmostZero(m03 - other.m03) &&
				Math::isAlmostZero(m10 - other.m10) && Math::isAlmostZero(m11 - other.m11) && Math::isAlmostZero(m12 - other.m12) && Math::isAlmostZero(m13 - other.m13) &&
				Math::isAlmostZero(m20 - other.m20) && Math::isAlmostZero(m21 - other.m21) && Math::isAlmostZero(m22 - other.m22) && Math::isAlmostZero(m23 - other.m23) &&
				Math::isAlmostZero(m30 - other.m30) && Math::isAlmostZero(m31 - other.m31) && Math::isAlmostZero(m32 - other.m32) && Math::isAlmostZero(m33 - other.m33);
		}

	public:
		template <class O>
		MatrixT& operator=(const MatrixT<4, 4, O>& other) noexcept
		{
			m00 = (T)(other.m00); m01 = (T)(other.m01); m02 = (T)(other.m02); m03 = (T)(other.m03);
			m10 = (T)(other.m10); m11 = (T)(other.m11); m12 = (T)(other.m12); m13 = (T)(other.m13);
			m20 = (T)(other.m20); m21 = (T)(other.m21); m22 = (T)(other.m22); m23 = (T)(other.m23);
			m30 = (T)(other.m30); m31 = (T)(other.m31); m32 = (T)(other.m32); m33 = (T)(other.m33);
			return *this;
		}

	};

	template <class T>
	using Matrix4T = MatrixT<4, 4, T>;

	typedef Matrix4T<sl_real> Matrix4;
	typedef Matrix4T<float> Matrix4f;
	typedef Matrix4T<double> Matrix4lf;

}

#endif
