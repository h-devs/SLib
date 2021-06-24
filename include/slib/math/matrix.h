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

#ifndef CHECKHEADER_SLIB_MATH_MATRIX
#define CHECKHEADER_SLIB_MATH_MATRIX

#include "vector.h"

namespace slib
{
	
	template <sl_uint32 ROWS, sl_uint32 COLS, class T>
	class SLIB_EXPORT MatrixT
	{
	public:
		T m[ROWS][COLS];
	
	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(MatrixT)
		
		MatrixT() = default;

		template <class O>
		MatrixT(const MatrixT<ROWS, COLS, O>& other) noexcept
		{
			for (sl_uint32 row = 0; row < ROWS; row++) {
				for (sl_uint32 col = 0; col < COLS; col++) {
					m[row][col] = (T)(other.m[row][col]);
				}
			}
		}
	
		template <class O>
		MatrixT(const O* arr) noexcept
		{
			for (sl_uint32 row = 0; row < ROWS; row++) {
				for (sl_uint32 col = 0; col < COLS; col++) {
					m[row][col] = (T)(*arr);
					arr++;
				}
			}
		}

		template <class O>
		MatrixT(const VectorT<COLS, O>* rows) noexcept
		{
			for (sl_uint32 row = 0; row < ROWS; row++) {
				for (sl_uint32 col = 0; col < COLS; col++) {
					m[row][col] = rows->m[col];
				}
				rows++;
			}
		}

	public:
		static const MatrixT& zero() noexcept
		{
			static SLIB_ALIGN(8) T _zero[ROWS*COLS] = { 0 };
			return *(reinterpret_cast<MatrixT const*>(&_zero));
		}

		static const MatrixT& fromArray(const T* arr) noexcept
		{
			return *(reinterpret_cast<MatrixT const*>(arr));
		}

		static MatrixT& fromArray(T* arr) noexcept
		{
			return *(reinterpret_cast<MatrixT*>(arr));
		}

		const VectorT<COLS, T>& getRow(sl_uint32 index) const noexcept
		{
			return (reinterpret_cast<VectorT<COLS, T> const*>(this))[index];
		}

		VectorT<COLS, T>& getRow(sl_uint32 index) noexcept
		{
			return (reinterpret_cast<VectorT<COLS, T>*>(this))[index];
		}

		void setRow(sl_uint32 index, const VectorT<COLS, T>& v) noexcept
		{
			(reinterpret_cast<VectorT<COLS, T>*>(this))[index] = v;
		}
	
		VectorT<ROWS, T> getColumn(sl_uint32 index) const noexcept
		{
			VectorT<ROWS, T> ret;
			for (sl_uint32 row = 0; row < ROWS; row++) {
				ret.m[row] = m[row][index];
			}
			return ret;
		}

		void setColumn(sl_uint32 index, const VectorT<ROWS, T>& v) noexcept
		{
			for (sl_uint32 row = 0; row < ROWS; row++) {
				m[row][index] = v.m[row];
			}
		}
	
		void add(const MatrixT& other) noexcept
		{
			for (sl_uint32 row = 0; row < ROWS; row++) {
				for (sl_uint32 col = 0; col < COLS; col++) {
					m[row][col] += other.m[row][col];
				}
			}
		}

		void subtract(const MatrixT& other) noexcept
		{
			for (sl_uint32 row = 0; row < ROWS; row++) {
				for (sl_uint32 col = 0; col < COLS; col++) {
					m[row][col] -= other.m[row][col];
				}
			}
		}

		void multiply(T value) noexcept
		{
			for (sl_uint32 row = 0; row < ROWS; row++) {
				for (sl_uint32 col = 0; col < COLS; col++) {
					m[row][col] *= value;
				}
			}
		}

		void divide(T value) noexcept
		{
			for (sl_uint32 row = 0; row < ROWS; row++) {
				for (sl_uint32 col = 0; col < COLS; col++) {
					m[row][col] /= value;
				}
			}
		}

		void divideReverse(T value) noexcept
		{
			for (sl_uint32 row = 0; row < ROWS; row++) {
				for (sl_uint32 col = 0; col < COLS; col++) {
					m[row][col] = value / m[row][col];
				}
			}
		}

		VectorT<COLS, T> multiplyLeft(const VectorT<ROWS, T>& v) const noexcept
		{
			VectorT<COLS, T> ret;
			for (sl_uint32 col = 0; col < COLS; col++) {
				T s = 0;
				for (sl_uint32 row = 0; row < ROWS; row++) {
					s += v.m[row] * m[row][col];
				}
				ret.m[col] = s;
			}
			return ret;
		}

		VectorT<ROWS, T> multiplyRight(const VectorT<COLS, T>& v) const noexcept
		{
			VectorT<ROWS, T> ret;
			for (sl_uint32 row = 0; row < ROWS; row++) {
				T s = 0;
				for (sl_uint32 col = 0; col < COLS; col++) {
					s += m[row][col] * v.m[col];
				}
				ret.m[row] = s;
			}
			return ret;
		}

		template <sl_uint32 ROWS2>
		void multiply(const MatrixT<ROWS, ROWS2, T>& m1, const MatrixT<ROWS2, COLS, T>& m2) noexcept;
		
		MatrixT lerp(const MatrixT& target, float factor) const noexcept
		{
			MatrixT ret;
			for (sl_uint32 row = 0; row < ROWS; row++) {
				for (sl_uint32 col = 0; col < COLS; col++) {
					ret.m[row][col] = SLIB_LERP(m[row][col], target.m[row][col], factor);
				}
			}
			return ret;
		}
	
		sl_bool equals(const MatrixT& other) const noexcept
		{
			for (sl_uint32 row = 0; row < ROWS; row++) {
				for (sl_uint32 col = 0; col < COLS; col++) {
					if (m[row][col] != other.m[row][col]) {
						return sl_false;
					}
				}
			}
			return sl_true;
		}

		sl_bool isAlmostEqual(const MatrixT& other) const noexcept
		{
			for (sl_uint32 row = 0; row < ROWS; row++) {
				for (sl_uint32 col = 0; col < COLS; col++) {
					if (!(Math::isAlmostZero(m[row][col] - other.m[row][col]))) {
						return sl_false;
					}
				}
			}
			return sl_true;
		}

	public:
		template <class O>
		MatrixT& operator=(const MatrixT<ROWS, COLS, O>& other) noexcept
		{
			for (sl_uint32 row = 0; row < ROWS; row++) {
				for (sl_uint32 col = 0; col < COLS; col++) {
					m[row][col] = (T)(other.m[row][col]);
				}
			}
		}

	};

	namespace priv
	{
		namespace matrix
		{

			template <sl_uint32 ROWS, sl_uint32 COLS, sl_uint32 ROWS2, class T>
			static void Multiply(MatrixT<ROWS, COLS, T>& m, const MatrixT<ROWS, ROWS2, T>& m1, const MatrixT<ROWS2, COLS, T>& m2)
			{
				for (sl_uint32 row = 0; row < ROWS; row++) {
					for (sl_uint32 col = 0; col < COLS; col++) {
						T s = 0;
						for (sl_uint32 k = 0; k < ROWS2; k++) {
							s += m1.m[row][k] * m2.m[k][col];
						}
						m.m[row][col] = s;
					}
				}
			}

		}
	}

	template <sl_uint32 ROWS, sl_uint32 COLS, class T>
	template <sl_uint32 ROWS2>
	SLIB_INLINE void MatrixT<ROWS, COLS, T>::multiply(const MatrixT<ROWS, ROWS2, T>& m1, const MatrixT<ROWS2, COLS, T>& m2) noexcept
	{
		priv::matrix::Multiply(*this, m1, m2);
	}

	template <sl_uint32 ROWS, sl_uint32 COLS, class T>
	SLIB_INLINE MatrixT<ROWS, COLS, T> operator+(const MatrixT<ROWS, COLS, T>& m1, const MatrixT<ROWS, COLS, T>& m2) noexcept
	{
		MatrixT<ROWS, COLS, T> ret(m1);
		ret.add(m2);
		return ret;
	}

	template <sl_uint32 ROWS, sl_uint32 COLS, class T>
	SLIB_INLINE MatrixT<ROWS, COLS, T>& operator+=(MatrixT<ROWS, COLS, T>& m1, const MatrixT<ROWS, COLS, T>& m2) noexcept
	{
		m1.add(m2);
		return m1;
	}

	template <sl_uint32 ROWS, sl_uint32 COLS, class T>
	SLIB_INLINE MatrixT<ROWS, COLS, T> operator-(const MatrixT<ROWS, COLS, T>& m1, const MatrixT<ROWS, COLS, T>& m2) noexcept
	{
		MatrixT<ROWS, COLS, T> ret(m1);
		ret.subtract(m2);
		return ret;
	}

	template <sl_uint32 ROWS, sl_uint32 COLS, class T>
	SLIB_INLINE MatrixT<ROWS, COLS, T>& operator-=(MatrixT<ROWS, COLS, T>& m1, const MatrixT<ROWS, COLS, T>& m2) noexcept
	{
		m1.subtract(m2);
		return m1;
	}

	template <sl_uint32 ROWS, sl_uint32 COLS, class T>
	SLIB_INLINE MatrixT<ROWS, COLS, T> operator-(const MatrixT<ROWS, COLS, T>& m) noexcept
	{
		MatrixT<ROWS, COLS, T> ret = {0};
		ret.subtract(m);
		return ret;
	}

	template <sl_uint32 ROWS, sl_uint32 COLS, class T>
	SLIB_INLINE MatrixT<ROWS, COLS, T> operator*(const MatrixT<ROWS, COLS, T>& m, T value) noexcept
	{
		MatrixT<ROWS, COLS, T> ret(m);
		ret.multiply(value);
		return ret;
	}
	
	template <sl_uint32 ROWS, sl_uint32 COLS, class T>
	SLIB_INLINE MatrixT<ROWS, COLS, T> operator*(T value, const MatrixT<ROWS, COLS, T>& m) noexcept
	{
		MatrixT<ROWS, COLS, T> ret(m);
		ret.multiply(value);
		return ret;
	}
	
	template <sl_uint32 ROWS, sl_uint32 COLS, class T, class O>
	SLIB_INLINE MatrixT<ROWS, COLS, T>& operator*=(MatrixT<ROWS, COLS, T>& m, const O& value) noexcept
	{
		m.multiply(value);
		return m;
	}

	template <sl_uint32 ROWS, sl_uint32 COLS, class T>
	SLIB_INLINE VectorT<ROWS, T> operator*(const MatrixT<ROWS, COLS, T>& m, const VectorT<COLS, T>& v) noexcept
	{
		return m.multiplyRight(v);
	}
	
	template <sl_uint32 ROWS, sl_uint32 COLS, class T>
	SLIB_INLINE VectorT<COLS, T> operator*(const VectorT<ROWS, T>& v, const MatrixT<ROWS, COLS, T>& m) noexcept
	{
		return m.multiplyLeft(v);
	}
	
	template <sl_uint32 ROWS, sl_uint32 COLS, sl_uint32 ROWS2, class T>
	SLIB_INLINE MatrixT<ROWS, COLS, T> operator*(const MatrixT<ROWS, ROWS2, T>& m1, const MatrixT<ROWS2, COLS, T>& m2) noexcept
	{
		MatrixT<ROWS, COLS, T> ret;
		ret.multiply(m1, m2);
		return ret;
	}

	template <sl_uint32 ROWS, sl_uint32 COLS, class T>
	SLIB_INLINE MatrixT<ROWS, COLS, T> operator/(const MatrixT<ROWS, COLS, T>& m, T value) noexcept
	{
		MatrixT<ROWS, COLS, T> ret(m);
		ret.divide(value);
		return ret;
	}

	template <sl_uint32 ROWS, sl_uint32 COLS, class T>
	SLIB_INLINE MatrixT<ROWS, COLS, T> operator/(T value, const MatrixT<ROWS, COLS, T>& m) noexcept
	{
		MatrixT<ROWS, COLS, T> ret(m);
		ret.divideReverse(value);
		return ret;
	}

	template <sl_uint32 ROWS, sl_uint32 COLS, class T>
	SLIB_INLINE MatrixT<ROWS, COLS, T>& operator/=(MatrixT<ROWS, COLS, T>& m, T value) noexcept
	{
		m.divide(value);
		return m;
	}


	template <sl_uint32 ROWS, sl_uint32 COLS>
	using Matrix = MatrixT<ROWS, COLS, sl_real>;

	template <sl_uint32 ROWS, sl_uint32 COLS>
	using Matrixf = MatrixT<ROWS, COLS, float>;

	template <sl_uint32 ROWS, sl_uint32 COLS>
	using Matrixlf = MatrixT<ROWS, COLS, double>;

}

#endif
