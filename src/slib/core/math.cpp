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

#include "slib/core/math.h"

#include "slib/core/time.h"
#include "slib/core/system.h"
#include "slib/core/process.h"
#include "slib/core/thread.h"
#include "slib/core/file.h"
#include "slib/crypto/sha2.h"

#include <math.h>
#include <stdlib.h>

#if defined(SLIB_PLATFORM_IS_UWP)
#include "float.h"
#endif

#if defined(_WIN32)
#include <slib/core/dl_windows_bcrypt.h>
#include <wincrypt.h>
#endif

namespace slib
{

	float Math::pow(float x, float y) noexcept
	{
		return ::powf(x, y);
	}

	double Math::pow(double x, double y) noexcept
	{
		return ::pow(x, y);
	}

	float Math::sqrt(float f) noexcept
	{
		return ::sqrtf(f);
	}

	double Math::sqrt(double f) noexcept
	{
		return ::sqrt(f);
	}

	float Math::cbrt(float f) noexcept
	{
		return ::cbrtf(f);
	}

	double Math::cbrt(double f) noexcept
	{
		return ::cbrt(f);
	}

	float Math::sin(float f) noexcept
	{
		return ::sinf(f);
	}

	double Math::sin(double f) noexcept
	{
		return ::sin(f);
	}

	float Math::cos(float f) noexcept
	{
		return ::cosf(f);
	}

	double Math::cos(double f) noexcept
	{
		return ::cos(f);
	}

	float Math::tan(float f) noexcept
	{
		return ::tanf(f);
	}

	double Math::tan(double f) noexcept
	{
		return ::tan(f);
	}

	float Math::cot(float f) noexcept
	{
		return 1.0f / tan(f);
	}

	double Math::cot(double f) noexcept
	{
		return 1.0 / tan(f);
	}

	float Math::arccos(float f) noexcept
	{
		return ::acosf(f);
	}

	double Math::arccos(double f) noexcept
	{
		return ::acos(f);
	}

	float Math::arcsin(float f) noexcept
	{
		return ::asinf(f);
	}

	double Math::arcsin(double f) noexcept
	{
		return ::asin(f);
	}

	float Math::arctan(float f) noexcept
	{
		return ::atanf(f);
	}

	double Math::arctan(double f) noexcept
	{
		return ::atan(f);
	}

	float Math::arctan2(float y, float x) noexcept
	{
		return ::atan2f(y, x);
	}

	double Math::arctan2(double y, double x) noexcept
	{
		return ::atan2(y, x);
	}

	float Math::log(float f) noexcept
	{
		return ::logf(f);
	}

	double Math::log(double f) noexcept
	{
		return ::log(f);
	}

	float Math::log10(float f) noexcept
	{
		return ::log10f(f);
	}

	double Math::log10(double f) noexcept
	{
		return ::log10(f);
	}

	float Math::log2(float f) noexcept
	{
		float log2f = 0.69314718055994530941723212145818f;
		return ::logf(f) / log2f;
	}

	double Math::log2(double f) noexcept
	{
		double log2 = 0.69314718055994530941723212145818;
		return ::log(f) / log2;
	}

	float Math::exp(float f) noexcept
	{
		return ::expf(f);
	}

	double Math::exp(double f) noexcept
	{
		return ::exp(f);
	}

	float Math::round(float f) noexcept
	{
		return ::floorf(f + 0.5f);
	}

	double Math::round(double f) noexcept
	{
		return ::floor(f + 0.5);
	}

	float Math::floor(float f) noexcept
	{
		return ::floorf(f);
	}

	double Math::floor(double f) noexcept
	{
		return ::floor(f);
	}

	float Math::ceil(float f) noexcept
	{
		return ::ceilf(f);
	}

	double Math::ceil(double f) noexcept
	{
		return ::ceil(f);
	}

#if defined(SLIB_PLATFORM_IS_UWP)

	sl_bool Math::isNaN(float f) noexcept
	{
		sl_int32 ret = _isnan(f);
		if (ret == 0) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool Math::isNaN(double f) noexcept
	{
		sl_int32 ret = _isnan(f);
		if (ret == 0) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool Math::isInfinite(float f) noexcept
	{
		sl_int32 ret = _finite(f);
		if (ret == 0) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool Math::isInfinite(double f) noexcept
	{
		sl_int32 ret = _finite(f);
		if (ret == 0) {
			return sl_false;
		}
		return sl_true;
	}

#else

	sl_bool Math::isNaN(float f) noexcept
	{
		return isnan(f) != 0;
	}

	sl_bool Math::isNaN(double f) noexcept
	{
		return isnan(f) != 0;
	}

	sl_bool Math::isInfinite(float f) noexcept
	{
		return isinf(f) != 0;
	}

	sl_bool Math::isInfinite(double f) noexcept
	{
		return isinf(f) != 0;
	}

#endif

	
	namespace priv
	{
		namespace math
		{
			template <class T>
			SLIB_INLINE static T normalizeDegree(T v) noexcept
			{
				if (Math::isNaN(v)) {
					return 0;
				}
				sl_int32 n = (sl_int32)v;
				T f = v - n;
				if (f < 0) {
					f = 1 + f;
					n--;
				}
				n = n % 360;
				if (n < 0) {
					n += 360;
				}
				v = (T)(n) + f;
				return v;
			}
		}
	}
	
	float Math::normalizeDegree(float v) noexcept
	{
		return priv::math::normalizeDegree(v);
	}

	double Math::normalizeDegree(double v) noexcept
	{
		return priv::math::normalizeDegree(v);
	}

	float Math::normalizeDegreeDistance(float v) noexcept
	{
		return normalizeDegree(v + 180) - 180;
	}

	double Math::normalizeDegreeDistance(double v) noexcept
	{
		return normalizeDegree(v + 180) - 180;
	}

	namespace priv
	{
		namespace math
		{
			template <class T>
			SLIB_INLINE static T convertAngleFromEllipseToCircle(T angle, T radiusX, T radiusY) noexcept
			{
				T _cos = Math::cos(angle);
				if (Math::isAlmostZero(_cos) || Math::isAlmostZero(sin(angle))) {
					return angle;
				}
				T PI2 = Math::DualPI<T>();
				T stretched = Math::arctan2(Math::sin(angle) / Math::abs(radiusY), Math::cos(angle) / Math::abs(radiusX));
				T revs_off = Math::round(angle / PI2) - Math::round(stretched / PI2);
				stretched += revs_off * PI2;
				return stretched;
			}
		}
	}

	float Math::convertAngleFromEllipseToCircle(float angle, float radiusX, float radiusY) noexcept
	{
		return priv::math::convertAngleFromEllipseToCircle(angle, radiusX, radiusY);
	}

	double Math::convertAngleFromEllipseToCircle(double angle, double radiusX, double radiusY) noexcept
	{
		return priv::math::convertAngleFromEllipseToCircle(angle, radiusX, radiusY);
	}

	double Math::random() noexcept
	{
		return (randomInt() % 10000) / 10000.0;
	}
	
	sl_uint32 Math::randomInt() noexcept
	{
		return ::rand();
	}
	
	double Math::randomByTime() noexcept
	{
		return (randomIntByTime() % 10000) / 10000.0;
	}
	
	sl_uint32 Math::randomIntByTime() noexcept
	{
		sl_uint32 dw = System::getTickCount();
		::srand(dw);
		return ::rand();
	}
	
	void Math::srand(sl_uint32 seed) noexcept
	{
		::srand(seed);
	}
	
	void Math::randomMemory(void* _mem, sl_size size) noexcept
	{
		if (!size) {
			return;
		}
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
		{
			SLIB_STATIC_STRING(path, "/dev/urandom")
			Ref<File> file = File::open(path, FileMode::Read);
			if (file.isNotNull()) {
				sl_size sizeRead = file->read(_mem, size);
				if (sizeRead == size) {
					return;
				}
			}
		}
#elif defined(_WIN32)
		{
			auto funcBCryptOpenAlgorithmProvider = bcrypt::getApi_BCryptOpenAlgorithmProvider();
			auto funcBCryptCloseAlgorithmProvider = bcrypt::getApi_BCryptCloseAlgorithmProvider();
			auto funcBCryptGenRandom = bcrypt::getApi_BCryptGenRandom();
			PVOID hAlgorithm;
			if (funcBCryptOpenAlgorithmProvider(&hAlgorithm, L"RNG", L"Microsoft Primitive Provider", 0) == 0) {
				sl_bool flagSuccess = sl_true;
#if defined(SLIB_ARCH_IS_64BIT)
				if (size >> 32) {
					sl_size segment = 0x40000000;
					while (size) {
						sl_size n = size;
						if (n > segment) {
							n = segment;
						}
						if (funcBCryptGenRandom(hAlgorithm, (PUCHAR)_mem, (ULONG)n, 0) != 0) {
							flagSuccess = sl_false;
							break;
						}
						size -= n;
					}
				} else {
					if (funcBCryptGenRandom(hAlgorithm, (PUCHAR)_mem, (ULONG)size, 0) != 0) {
						flagSuccess = sl_false;
					}
				}
#else
				if (funcBCryptGenRandom(hAlgorithm, (PUCHAR)_mem, size, 0) != 0) {
					flagSuccess = sl_false;
				}
#endif
				funcBCryptCloseAlgorithmProvider(hAlgorithm, 0);
				if (flagSuccess) {
					return;
				}
			}
			sl_bool flagSuccess = sl_true;
			HCRYPTPROV hCryptProv;
			if (CryptAcquireContextW(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
#if defined(SLIB_ARCH_IS_64BIT)
				if (size >> 32) {
					sl_size segment = 0x40000000;
					while (size) {
						sl_size n = size;
						if (n > segment) {
							n = segment;
						}
						if (!CryptGenRandom(hCryptProv, (DWORD)n, (BYTE*)_mem)) {
							flagSuccess = sl_false;
							break;
						}
						size -= n;
					}
				} else {
					if (!CryptGenRandom(hCryptProv, (DWORD)size, (BYTE*)_mem)) {
						flagSuccess = sl_false;
					}
				}
#else
				if (!CryptGenRandom(hCryptProv, (DWORD)size, (BYTE*)_mem)) {
					flagSuccess = sl_false;
				}
#endif
				CryptReleaseContext(hCryptProv, 0);
				if (flagSuccess) {
					return;
				}
			}
		}
#endif

#define RANDOM_BLOCK 64
		sl_size nSections = size / RANDOM_BLOCK;
		if (size % RANDOM_BLOCK) {
			nSections++;
		}

		sl_uint32 dw = System::getTickCount() + (::rand());
		dw = (dw >> 16) ^ (dw & 0xFFFF);
		::srand((unsigned int)(dw + 1000));

		sl_uint8* mem = (sl_uint8*)_mem;
		for (sl_size i = 0; i < nSections; i++) {
			sl_size n;
			if (i == nSections - 1) {
				n = size - i * RANDOM_BLOCK;
			} else {
				n = RANDOM_BLOCK;
			}
			char hashSeed[RANDOM_BLOCK];
			// preparing referencing data
			{
				char buf[72];
				// 0:8 Time
				sl_uint64 t = Time::now().toInt();
				Base::copyMemory(buf, &t, 8);
				// 8:4 Stack Address
				sl_uint32 sa = (sl_uint32)(sl_reg)((void*)(&size));
				Base::copyMemory(buf + 8, &sa, 4);
				// 12:4 Random
				sl_uint32 ra = ::rand();
				Base::copyMemory(buf + 12, &ra, 4);

				static sl_bool flagStatics = sl_false;
				static sl_uint32 pid = 0;
				static char hashAppPath[32] = { 0 };
				if (!flagStatics) {
					pid = Process::getCurrentProcessId();
					String str = System::getApplicationPath();
					SHA256::hash(str, hashAppPath);
					flagStatics = sl_true;
				}
				// 16:4 process id
				Base::copyMemory(buf + 16, &pid, 4);
				// 20:4 thread id
				sl_uint32 tid = (sl_uint32)(Thread::getCurrentThreadId());
				Base::copyMemory(buf + 20, &tid, 4);
				// 24:4 new address
				void* pna = Base::createMemory(1);
				sl_uint32 na = (sl_uint32)(sl_reg)(pna);
				if (pna) {
					Base::freeMemory(pna);
				}
				Base::copyMemory(buf + 24, &na, 4);
				// 28:4 system ticks
				sl_uint32 tick = System::getTickCount();
				Base::copyMemory(buf + 28, &tick, 4);
				// 32:32 app path hash
				Base::copyMemory(buf + 32, hashAppPath, 32);
				// 64:8 sequence
				static sl_uint64 se = 0;
				se++;
				Base::copyMemory(buf + 64, &se, 8);
				SHA512::hash(buf, 72, hashSeed);
			}
			sl_uint32 mm = 0;
			for (sl_size i = 0; i < n; i++) {
				mm ^= rand();
				mem[i] = (char)mm ^ hashSeed[i];
				mm >>= 8;
			}
			mem += RANDOM_BLOCK;
		}
	}

	sl_uint32 Math::roundUpToPowerOfTwo(sl_uint32 num) noexcept
	{
		num--;
		num |= (num >> 1);
		num |= (num >> 2);
		num |= (num >> 4);
		num |= (num >> 8);
		num |= (num >> 16);
		return num + 1;
	}
	
	sl_uint32 Math::roundUpToPowerOfTwo32(sl_uint32 num) noexcept
	{
		num--;
		num |= (num >> 1);
		num |= (num >> 2);
		num |= (num >> 4);
		num |= (num >> 8);
		num |= (num >> 16);
		return num + 1;
	}

	sl_uint64 Math::roundUpToPowerOfTwo(sl_uint64 num) noexcept
	{
		num--;
		num |= (num >> 1);
		num |= (num >> 2);
		num |= (num >> 4);
		num |= (num >> 8);
		num |= (num >> 16);
		num |= (num >> 32);
		return num + 1;
	}
	
	sl_uint64 Math::roundUpToPowerOfTwo64(sl_uint64 num) noexcept
	{
		num--;
		num |= (num >> 1);
		num |= (num >> 2);
		num |= (num >> 4);
		num |= (num >> 8);
		num |= (num >> 16);
		num |= (num >> 32);
		return num + 1;
	}

	sl_uint32 Math::getMostSignificantBits(sl_uint32 n) noexcept
	{
		sl_uint32 ret = 0;
		while (n) {
			ret++;
			n >>= 1;
		}
		return ret;
	}
	
	sl_uint32 Math::getMostSignificantBits32(sl_uint32 n) noexcept
	{
		sl_uint32 ret = 0;
		while (n) {
			ret++;
			n >>= 1;
		}
		return ret;
	}

	sl_uint32 Math::getMostSignificantBits(sl_uint64 n) noexcept
	{
		sl_uint32 ret = 0;
		while (n) {
			ret++;
			n >>= 1;
		}
		return ret;
	}
	
	sl_uint32 Math::getMostSignificantBits64(sl_uint64 n) noexcept
	{
		sl_uint32 ret = 0;
		while (n) {
			ret++;
			n >>= 1;
		}
		return ret;
	}

	sl_uint32 Math::getLeastSignificantBits(sl_uint32 n) noexcept
	{
		if (n == 0) {
			return 0;
		}
		sl_uint32 ret = 0;
		while ((n & 1) == 0) {
			ret++;
			n >>= 1;
		}
		return ret;
	}
	
	sl_uint32 Math::getLeastSignificantBits32(sl_uint32 n) noexcept
	{
		if (n == 0) {
			return 0;
		}
		sl_uint32 ret = 0;
		while ((n & 1) == 0) {
			ret++;
			n >>= 1;
		}
		return ret;
	}

	sl_uint32 Math::getLeastSignificantBits(sl_uint64 n) noexcept
	{
		if (n == 0) {
			return 0;
		}
		sl_uint32 ret = 0;
		while ((n & 1) == 0) {
			ret++;
			n >>= 1;
		}
		return ret;
	}
	
	sl_uint32 Math::getLeastSignificantBits64(sl_uint64 n) noexcept
	{
		if (n == 0) {
			return 0;
		}
		sl_uint32 ret = 0;
		while ((n & 1) == 0) {
			ret++;
			n >>= 1;
		}
		return ret;
	}
	
}

