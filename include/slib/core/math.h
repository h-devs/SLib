/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_MATH
#define CHECKHEADER_SLIB_CORE_MATH

#include "definition.h"

#define SLIB_PI					3.141592653589793f
#define SLIB_PI_LONG			3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679
#define SLIB_PI_DUAL			6.283185307179586f
#define SLIB_PI_DUAL_LONG		6.283185307179586476925286766559
#define SLIB_PI_HALF			1.570796326795f
#define SLIB_PI_HALF_LONG		1.5707963267948966192313216916398
#define SLIB_PI_QUARTER			0.7853981633975f
#define SLIB_PI_QUARTER_LONG	0.7853981633974483096156608458199
#define SLIB_EPSILON			1.192092896e-10f
#define SLIB_EPSILON_LONG		1.192092896e-20

#define SLIB_POW10_0 1
#define SLIB_POW10_1 10
#define SLIB_POW10_2 100
#define SLIB_POW10_3 1000
#define SLIB_POW10_4 10000
#define SLIB_POW10_5 100000
#define SLIB_POW10_6 1000000
#define SLIB_POW10_7 10000000
#define SLIB_POW10_8 100000000
#define SLIB_POW10_9 1000000000
#define SLIB_POW10_10 SLIB_UINT64(10000000000)
#define SLIB_POW10_11 SLIB_UINT64(100000000000)
#define SLIB_POW10_12 SLIB_UINT64(1000000000000)
#define SLIB_POW10_13 SLIB_UINT64(10000000000000)
#define SLIB_POW10_14 SLIB_UINT64(100000000000000)
#define SLIB_POW10_15 SLIB_UINT64(1000000000000000)
#define SLIB_POW10_16 SLIB_UINT64(10000000000000000)
#define SLIB_POW10_17 SLIB_UINT64(100000000000000000)
#define SLIB_POW10_18 SLIB_UINT64(1000000000000000000)
#define SLIB_POW10_19 SLIB_UINT64(10000000000000000000)

#undef max
#undef min

namespace slib
{
	
	template <class T>
	class MathContants;
	
	template <>
	class MathContants<float>
	{
	public:
		constexpr static float PI = SLIB_PI;
		constexpr static float PI_DUAL = SLIB_PI_DUAL;
		constexpr static float PI_HALF = SLIB_PI_HALF;
		constexpr static float PI_QUARTER = SLIB_PI_QUARTER;
		constexpr static float EPSILON = SLIB_EPSILON;
	};
	
	template <>
	class MathContants<double>
	{
	public:
		constexpr static double PI = SLIB_PI_LONG;
		constexpr static double PI_DUAL = SLIB_PI_DUAL_LONG;
		constexpr static double PI_HALF = SLIB_PI_HALF_LONG;
		constexpr static double PI_QUARTER = SLIB_PI_QUARTER_LONG;
		constexpr static double EPSILON = SLIB_EPSILON_LONG;
	};
	
	class Math
	{
	public:
		template <typename T>
		constexpr static T max(T a, T b) noexcept
		{
			return (a > b) ? a : b;
		}

		template <typename T>
		constexpr static T min(T a, T b) noexcept
		{
			return (a < b) ? a : b;
		}

		template <typename T>
		constexpr static T abs(T v) noexcept
		{
			return (v > 0) ? (v) : (-v);
		}

		constexpr static sl_int32 sign(float v) noexcept
		{
			return (v >= 0) ? 1 : -1;
		}

		constexpr static sl_int32 sign(double v) noexcept
		{
			return (v >= 0) ? 1 : -1;
		}
	
		static float pow(float x, float y) noexcept;

		static double pow(double x, double y) noexcept;

		static sl_uint64 pow10i(sl_uint32 exponent) noexcept;

		template <class T>
		static void pow10iT(T& _out, sl_uint32 exponent) noexcept
		{
			if (exponent < 20) {
				_out = pow10i(exponent);
				return;
			}
			_out = pow10i(exponent & 15);
			sl_uint64 p16 = pow10i(16);
			_out *= p16;
			exponent >>= 5;
			if (!exponent) {
				return;
			}
			T p = p16;
			do {
				p = p * p;
				_out *= p;
				exponent >>= 1;
			} while (exponent);
		}

		constexpr static float square(float x) noexcept
		{
			return x * x;
		}

		constexpr static double square(double x) noexcept
		{
			return x * x;
		}

		static float sqrt(float f) noexcept;

		static double sqrt(double f) noexcept;

		static float cbrt(float f) noexcept;

		static double cbrt(double f) noexcept;
	

		static float sin(float f) noexcept;

		static double sin(double f) noexcept;

		static float cos(float f) noexcept;

		static double cos(double f) noexcept;

		static float tan(float f) noexcept;

		static double tan(double f) noexcept;

		static float cot(float f) noexcept;

		static double cot(double f) noexcept;
	
		static float arcsin(float f) noexcept;

		static double arcsin(double f) noexcept;

		static float arccos(float f) noexcept;

		static double arccos(double f) noexcept;

		static float arctan(float f) noexcept;

		static double arctan(double f) noexcept;

		static float arctan2(float y, float x) noexcept;

		static double arctan2(double y, double x) noexcept;


		static float log(float f) noexcept;

		static double log(double f) noexcept;

		static float log2(float f) noexcept;

		static double log2(double f) noexcept;

		static float log10(float f) noexcept;

		static double log10(double f) noexcept;

		static sl_uint32 log10i(sl_uint32 n) noexcept;

		static sl_uint32 log10i(sl_uint64 n) noexcept;

		template <class T>
		static sl_uint32 log10iT(const T& v) noexcept
		{
			if (v <= SLIB_UINT64_MAX) {
				return log10i((sl_uint64)v);
			}
			T p = SLIB_POW10_16;
			T p2 = p * p;
			sl_uint32 n = 16;
			while (p2 > p && v >= p2) {
				p = p2;
				p2 = p * p;
				n *= 2;
			}
			return n + log10iT(v / p);
		}
	
		static float exp(float f) noexcept;

		static double exp(double f) noexcept;
	

		static float ceil(float f) noexcept;

		static double ceil(double f) noexcept;
	
		static float floor(float f) noexcept;

		static double floor(double f) noexcept;

		static float round(float f) noexcept;

		static double round(double f) noexcept;
	

		// check "Not a Number"  such as (sqrt(-1.0))
		static sl_bool isNaN(float f) noexcept;

		// check "Not a Number"  such as (sqrt(-1.0))
		static sl_bool isNaN(double f) noexcept;

		// check "Infinite"  such as (1.0 / 0.0)
		static sl_bool isInfinite(float f) noexcept;

		// check "Infinite"  such as (1.0 / 0.0)
		static sl_bool isInfinite(double f) noexcept;

		static sl_bool isPositiveInfinite(float f) noexcept;

		static sl_bool isPositiveInfinite(double f) noexcept;

		static sl_bool isNegativeInfinite(float f) noexcept;

		static sl_bool isNegativeInfinite(double f) noexcept;

		static void getNaN(float& f) noexcept;

		static void getNaN(double& f) noexcept;

		static void getPositiveInfinite(float& f) noexcept;

		static void getPositiveInfinite(double& f) noexcept;

		static void getNegativeInfinite(float& f) noexcept;

		static void getNegativeInfinite(double& f) noexcept;


		constexpr static float saturate(float f) noexcept
		{
			return (f<0.0f) ? 0.0f : ((f>1.0f) ? 1.0f : f);
		}

		constexpr static double saturate(double f) noexcept
		{
			return (f<0.0) ? 0.0 : ((f>1.0) ? 1.0 : f);
		}

		template <typename T>
		constexpr static T clamp(T v, T vMin, T vMax) noexcept
		{
			return (v<vMin) ? vMin : ((v>vMax) ? vMax : v);
		}

		constexpr static sl_int32 clamp0_255(sl_int32 v) noexcept
		{
			// ((-v) >> 31) & v <=> arithmetic sign shift, clamp to >=0
			return (((0xFF - (((-v) >> 31) & v)) >> 31) | (((-v) >> 31) & v)) & 0xFF; // clamp to < 256
		}

		constexpr static sl_int32 clamp0_65535(sl_int32 v) noexcept
		{
			// ((-v) >> 31) & v <=> arithmetic sign shift, clamp to >=0
			return (((0xFFFF - (((-v) >> 31) & v)) >> 31) | (((-v) >> 31) & v)) & 0xFFFF; // clamp to < 65536
		}
	
		constexpr static sl_bool isAlmostZero(float f) noexcept
		{
			return (f > -SLIB_EPSILON && f < SLIB_EPSILON);
		}

		constexpr static sl_bool isAlmostZero(double f) noexcept
		{
			return (f > -SLIB_EPSILON_LONG && f < SLIB_EPSILON_LONG);
		}

		constexpr static sl_bool isAlmostZero(sl_int32 v) noexcept
		{
			return v == 0;
		}

		constexpr static sl_bool isAlmostZero(sl_uint32 v) noexcept
		{
			return v == 0;
		}

		constexpr static sl_bool isAlmostZero(sl_int64 v) noexcept
		{
			return v == 0;
		}

		constexpr static sl_bool isAlmostZero(sl_uint64 v) noexcept
		{
			return v == 0;
		}
	
		constexpr static sl_bool isLessThanEpsilon(float f) noexcept
		{
			return (f < SLIB_EPSILON);
		}

		constexpr static sl_bool isLessThanEpsilon(double f) noexcept
		{
			return (f < SLIB_EPSILON_LONG);
		}

		constexpr static sl_bool isLessThanEpsilon(sl_int32 v) noexcept
		{
			return (v <= 0);
		}

		constexpr static sl_bool isLessThanEpsilon(sl_uint32 v) noexcept
		{
			return (v <= 0);
		}

		constexpr static sl_bool isLessThanEpsilon(sl_int64 v) noexcept
		{
			return (v <= 0);
		}

		constexpr static sl_bool isLessThanEpsilon(sl_uint64 v) noexcept
		{
			return (v <= 0);
		}
	

		constexpr static float getRadianFromDegrees(float f) noexcept
		{
			return (f * SLIB_PI / 180.0f);
		}

		constexpr static double getRadianFromDegrees(double f) noexcept
		{
			return (f * SLIB_PI_LONG / 180.0);
		}

		constexpr static float getDegreesFromRadian(float f) noexcept
		{
			return (f * 180.0f / SLIB_PI);
		}

		constexpr static double getDegreesFromRadian(double f) noexcept
		{
			return (f * 180.0 / SLIB_PI_LONG);
		}

		template <class T>
		constexpr static T PI() noexcept
		{
			return MathContants<T>::PI;
		}

		template <class T>
		constexpr static T DualPI() noexcept
		{
			return MathContants<T>::PI_DUAL;
		}

		template <class T>
		constexpr static T HalfPI() noexcept
		{
			return MathContants<T>::PI_HALF;
		}

		template <class T>
		constexpr static T QuarterPI() noexcept
		{
			return MathContants<T>::PI_QUARTER;
		}

		template <class T>
		constexpr static T Epsilon() noexcept
		{
			return MathContants<T>::EPSILON;
		}


		// normalize degree to [0, 360]
		static float normalizeDegree(float f) noexcept;

		// normalize degree to [0, 360]
		static double normalizeDegree(double f) noexcept;

		// normalize degree to [-180, 180]
		static float normalizeDegreeDistance(float f) noexcept;

		// normalize degree to [-180, 180]
		static double normalizeDegreeDistance(double f) noexcept;
	

		static float convertAngleFromEllipseToCircle(float f, float radiusX, float radiusY) noexcept;

		static double convertAngleFromEllipseToCircle(double f, double radiusX, double radiusY) noexcept;
	

		// random value between 0~1
		static double random() noexcept;

		static sl_uint32 randomInt() noexcept;
		
		static double randomByTime() noexcept;
		
		static sl_uint32 randomIntByTime() noexcept;
        
        static void srand(sl_uint32 seed) noexcept;

		static void randomMemory(void* mem, sl_size size) noexcept;


		static sl_uint32 roundUpToPowerOfTwo(sl_uint32 num) noexcept;

		static sl_uint32 roundUpToPowerOfTwo32(sl_uint32 num) noexcept;
		
		static sl_uint64 roundUpToPowerOfTwo(sl_uint64 num) noexcept;
		
		static sl_uint64 roundUpToPowerOfTwo64(sl_uint64 num) noexcept;

		
		constexpr static sl_uint32 rotateLeft(sl_uint32 x, sl_uint32 n) noexcept
		{
			return (x << n) | (x >> (32 - n));
		}
		
		constexpr static sl_uint32 rotateLeft32(sl_uint32 x, sl_uint32 n) noexcept
		{
			return (x << n) | (x >> (32 - n));
		}

		constexpr static sl_uint64 rotateLeft(sl_uint64 x, sl_uint32 n) noexcept
		{
			return (x << n) | (x >> (64 - n));
		}
		
		constexpr static sl_uint64 rotateLeft64(sl_uint64 x, sl_uint32 n) noexcept
		{
			return (x << n) | (x >> (64 - n));
		}

		constexpr static sl_uint32 rotateRight(sl_uint32 x, sl_uint32 n) noexcept
		{
			return (x >> n) | (x << (32 - n));
		}
		
		constexpr static sl_uint32 rotateRight32(sl_uint32 x, sl_uint32 n) noexcept
		{
			return (x >> n) | (x << (32 - n));
		}

		constexpr static sl_uint64 rotateRight(sl_uint64 x, sl_uint32 n) noexcept
		{
			return (x >> n) | (x << (64 - n));
		}
		
		constexpr static sl_uint64 rotateRight64(sl_uint64 x, sl_uint32 n) noexcept
		{
			return (x >> n) | (x << (64 - n));
		}


		static sl_uint32 getMostSignificantBits(sl_uint32 n) noexcept;
		
		static sl_uint32 getMostSignificantBits32(sl_uint32 n) noexcept;

		static sl_uint32 getMostSignificantBits(sl_uint64 n) noexcept;
		
		static sl_uint32 getMostSignificantBits64(sl_uint64 n) noexcept;

		static sl_uint32 getLeastSignificantBits(sl_uint32 n) noexcept;
		
		static sl_uint32 getLeastSignificantBits32(sl_uint32 n) noexcept;

		static sl_uint32 getLeastSignificantBits(sl_uint64 n) noexcept;
		
		static sl_uint32 getLeastSignificantBits64(sl_uint64 n) noexcept;


		static void mul32(sl_uint32 a, sl_uint32 b, sl_uint32& o_high, sl_uint32& o_low) noexcept;

		static void mul64(sl_uint64 a, sl_uint64 b, sl_uint64& o_high, sl_uint64& o_low) noexcept;
		
		static sl_bool div128_64(sl_uint64& high, sl_uint64& low, sl_uint64 divisor, sl_uint64& remainder) noexcept;

		static sl_bool div128_32(sl_uint64& high, sl_uint64& low, sl_uint32 divisor, sl_uint32& remainder) noexcept;

	};

}

#endif
