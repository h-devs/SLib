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

#ifndef CHECKHEADER_SLIB_MATH_VECTOR
#define CHECKHEADER_SLIB_MATH_VECTOR

#include "definition.h"

#include "../core/math.h"
#include "../core/default_members.h"
#include "../core/interpolation.h"

namespace slib
{

	template <sl_uint32 N, class T, class FT = T>
	class SLIB_EXPORT VectorT
	{
	public:
		T m[N];

	public:
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(VectorT)

		VectorT() = default;

		template <class O, class FO>
		VectorT(const VectorT<N, O, FO>& other) noexcept
		{
			for (sl_uint32 i = 0; i < N; i++) {
				m[i] = (T)(other.m[i]);
			}
		}

		template <class O>
		VectorT(const O* arr) noexcept
		{
			for (sl_uint32 i = 0; i < N; i++) {
				m[i] = (T)(arr[i]);
			}
		}

	public:
		static const VectorT& zero() noexcept
		{
			static SLIB_ALIGN(8) T _zero[N] = {0};
			return *(reinterpret_cast<VectorT*>(&_zero));
		}

		static const VectorT& fromArray(const T* arr) noexcept
		{
			return *(reinterpret_cast<VectorT const*>(arr));
		}

		static VectorT& fromArray(T* arr) noexcept
		{
			return *(reinterpret_cast<VectorT*>(arr));
		}

	public:
		T dot(const VectorT& other) const noexcept
		{
			T ret = 0;
			for (sl_uint32 i = 0; i < N; i++) {
				ret += m[i] * other.m[i];
			}
			return ret;
		}

		T getLength2p() const noexcept
		{
			T ret = 0;
			for (sl_uint32 i = 0; i < N; i++) {
				ret += m[i] * m[i];
			}
			return ret;
		}

		FT getLength() const noexcept
		{
			return Math::sqrt((FT)(getLength2p()));
		}

		T getLength2p(const VectorT& other) const noexcept
		{
			T ret = 0;
			for (sl_uint32 i = 0; i < N; i++) {
				ret += m[i] * other.m[i];
			}
			return ret;
		}

		FT getLength(const VectorT& other) const noexcept
		{
			return Math::sqrt((FT)(getLength2p(other)));
		}

		void normalize() noexcept
		{
			T l2 = getLength2p();
			if (l2 > 0) {
				FT l = (FT)(Math::sqrt((FT)l2));
				for (sl_uint32 i = 0; i < N; i++) {
					m[i] = (T)(((FT)(m[i])) / l);
				}
			}
		}

		VectorT getNormalized() noexcept
		{
			VectorT ret;
			T l2 = getLength2p();
			if (l2 > 0) {
				FT l = (FT)(Math::sqrt((FT)l2));
				for (sl_uint32 i = 0; i < N; i++) {
					ret.m[i] = (T)(((FT)(m[i])) / l);
				}
			}
			return ret;
		}

		FT getCosBetween(const VectorT& other) const noexcept
		{
			return (FT)(dot(other)) / Math::sqrt((FT)(getLength2p() * other.getLength2p()));
		}

		FT getAngleBetween(const VectorT& other) const noexcept
		{
			return Math::arccos(getCosBetween(other));
		}

		VectorT lerp(const VectorT& target, float factor) const noexcept
		{
			VectorT ret;
			for (sl_uint32 i = 0; i < N; i++) {
				ret.m[i] = (T)SLIB_LERP(m[i], target.m[i], factor);
			}
			return ret;
		}

		VectorT divideReverse(T f) const noexcept
		{
			VectorT<N, T, FT> ret;
			for (sl_uint32 i = 0; i < N; i++) {
				ret.m[i] = f / m[i];
			}
			return ret;
		}

		sl_bool equals(const VectorT& other) const noexcept
		{
			for (sl_uint32 i = 0; i < N; i++) {
				if (m[i] != other.m[i]) {
					return sl_false;
				}
			}
			return sl_true;
		}

		sl_bool isAlmostEqual(const VectorT& other) const noexcept
		{
			for (sl_uint32 i = 0; i < N; i++) {
				if (!(Math::isAlmostZero(m[i] - other.m[i]))) {
					return sl_false;
				}
			}
			return sl_true;
		}

	public:
		template <class O, class FO>
		VectorT& operator=(const VectorT<N, O, FO>& other) noexcept
		{
			for (sl_uint32 i = 0; i < N; i++) {
				m[i] = (T)(other.m[i]);
			}
			return *this;
		}

		VectorT operator+(const VectorT& other) const noexcept
		{
			VectorT ret;
			for (sl_uint32 i = 0; i < N; i++) {
				ret.m[i] = m[i] + other.m[i];
			}
			return ret;
		}

		VectorT& operator+=(const VectorT& other) noexcept
		{
			for (sl_uint32 i = 0; i < N; i++) {
				m[i] += other.m[i];
			}
			return *this;
		}

		VectorT operator-(const VectorT& other) const noexcept
		{
			VectorT ret;
			for (sl_uint32 i = 0; i < N; i++) {
				ret.m[i] = m[i] - other.m[i];
			}
			return ret;
		}

		VectorT& operator-=(const VectorT& other) noexcept
		{
			for (sl_uint32 i = 0; i < N; i++) {
				m[i] -= other.m[i];
			}
			return *this;
		}

		VectorT operator*(T f) const noexcept
		{
			VectorT ret;
			for (sl_uint32 i = 0; i < N; i++) {
				ret.m[i] = m[i] * f;
			}
			return ret;
		}

		VectorT& operator*=(T f) noexcept
		{
			for (sl_uint32 i = 0; i < N; i++) {
				m[i] *= f;
			}
			return *this;
		}

		VectorT operator*(const VectorT& other) const noexcept
		{
			VectorT ret;
			for (sl_uint32 i = 0; i < N; i++) {
				ret.m[i] = m[i] * other.m[i];
			}
			return ret;
		}

		VectorT& operator*=(const VectorT& other) noexcept
		{
			for (sl_uint32 i = 0; i < N; i++) {
				m[i] *= other.m[i];
			}
			return *this;
		}

		VectorT operator/(T f) const noexcept
		{
			VectorT ret;
			for (sl_uint32 i = 0; i < N; i++) {
				ret.m[i] = m[i] / f;
			}
			return ret;
		}

		VectorT& operator/=(T f) noexcept
		{
			for (sl_uint32 i = 0; i < N; i++) {
				m[i] /= f;
			}
			return *this;
		}

		VectorT operator/(const VectorT& other) const noexcept
		{
			VectorT ret;
			for (sl_uint32 i = 0; i < N; i++) {
				ret.m[i] = m[i] / other.m[i];
			}
			return ret;
		}

		VectorT& operator/=(const VectorT& other) noexcept
		{
			for (sl_uint32 i = 0; i < N; i++) {
				m[i] /= other.m[i];
			}
			return *this;
		}

		VectorT operator-() const noexcept
		{
			VectorT ret;
			for (sl_uint32 i = 0; i < N; i++) {
				ret.m[i] = -m[i];
			}
			return ret;
		}

	};


	template <sl_uint32 N, class T, class FT>
	SLIB_INLINE VectorT<N, T, FT> operator*(T f, const VectorT<N, T, FT>& v) noexcept
	{
		return v * f;
	}

	template <sl_uint32 N, class T, class FT>
	SLIB_INLINE VectorT<N, T, FT> operator/(T f, const VectorT<N, T, FT>& v) noexcept
	{
		return v.devideReverse(f);
	}


	template <sl_uint32 N>
	using Vector = VectorT<N, sl_real>;

	template <sl_uint32 N>
	using Vectorf = VectorT<N, float>;

	template <sl_uint32 N>
	using Vectorlf = VectorT<N, double>;

	template <sl_uint32 N>
	using Vectori = VectorT<N, sl_int32, float>;

	template <sl_uint32 N>
	using Vectorli = VectorT<N, sl_int64, double>;

}

#endif
