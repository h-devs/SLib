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

#ifndef CHECKHEADER_SLIB_MATH_COMPLEX
#define CHECKHEADER_SLIB_MATH_COMPLEX

#include "definition.h"

#include "../core/math.h"
#include "../core/default_members.h"

namespace slib
{

	template <class T>
	class ComplexT
	{
	public:
		T real;
		T imag; // imaginary

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(ComplexT)

		ComplexT() = default;

		template <class O>
		SLIB_CONSTEXPR ComplexT(const ComplexT<O>& other) noexcept: real((T)(other.real)), imag((T)(other.imag)) {}

		SLIB_CONSTEXPR ComplexT(const T& _real, const T& _imag) noexcept: real(_real), imag(_imag) {}

		SLIB_CONSTEXPR ComplexT(const T& _real) noexcept: real(_real), imag(0) {}

	public:
		SLIB_CONSTEXPR ComplexT conjugate() const
		{
			return ComplexT(real, -imag);
		}

		ComplexT reciprocal() const noexcept
		{
			T t = real * real + imag * imag;
			return ComplexT(real / t, -imag / t);
		}

		T abs() const noexcept
		{
			return Math::sqrt(real * real + imag * imag);
		}

		SLIB_CONSTEXPR T abs2() const
		{
			return real * real + imag * imag;
		}

		ComplexT exp() const noexcept
		{
			T t = Math::exp(real);
			return ComplexT(Math::cos(imag) * t, Math::sin(imag) * t);
		}

	public:
		template <class O>
		ComplexT& operator=(const ComplexT<O>& other) noexcept
		{
			real = (T)(other.real);
			imag = (T)(other.imag);
			return *this;
		}

		ComplexT& operator=(const T& _real) noexcept
		{
			real = _real;
			imag = 0;
			return *this;
		}

		ComplexT operator+(const ComplexT& other) const noexcept
		{
			return ComplexT(real + other.real, imag + other.imag);
		}

		ComplexT& operator+=(const ComplexT& other) noexcept
		{
			real += other.real;
			imag += other.imag;
			return *this;
		}

		ComplexT operator-(const ComplexT& other) const noexcept
		{
			return ComplexT(real - other.real, imag - other.imag);
		}

		ComplexT& operator-=(const ComplexT& other) noexcept
		{
			real -= other.real;
			imag -= other.imag;
			return *this;
		}

		ComplexT operator*(const ComplexT& other) const noexcept
		{
			return ComplexT(real * other.real - imag * other.imag, imag * other.real + real * other.imag);
		}

		ComplexT& operator*=(const ComplexT& other) noexcept
		{
			T _real = real * other.real - imag * other.imag;
			T _imag = imag * other.real + real * other.imag;
			real = _real;
			imag = _imag;
			return *this;
		}

		ComplexT operator/(const ComplexT& other) const noexcept
		{
			T t = other.real * other.real + other.imag * other.imag;
			return ComplexT((real * other.real + imag * other.imag) / t, (imag * other.real - real * other.imag) / t);
		}

		ComplexT& operator/=(const ComplexT& other) noexcept
		{
			T t = other.real * other.real + other.imag * other.imag;
			T _real = (real * other.real + imag * other.imag) / t;
			T _imag = (imag * other.real - real * other.imag) / t;
			real = _real;
			imag = _imag;
			return *this;
		}

	};

	typedef ComplexT<sl_real> Complex;
	typedef ComplexT<float> Complexf;
	typedef ComplexT<double> Complexlf;

}

#endif