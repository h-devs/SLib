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

#ifndef CHECKHEADER_SLIB_CORE_ENDIAN
#define CHECKHEADER_SLIB_CORE_ENDIAN

#include "definition.h"

namespace slib
{

	enum class EndianType
	{
		Little = 0,
		Big = 1
	};

	class SLIB_EXPORT Endian
	{
	public:
		constexpr static EndianType Little = EndianType::Little;
		constexpr static EndianType Big = EndianType::Big;

	public:
		sl_bool checkLittleEndianRuntime() noexcept;

		sl_bool checkBigEndianRuntime() noexcept
		{
			return !(checkLittleEndianRuntime());
		}

		static EndianType get() noexcept
		{
#if defined(SLIB_ARCH_IS_LITTLE_ENDIAN)
			return EndianType::Little;
#elif defined(SLIB_ARCH_IS_BIG_ENDIAN)
			return EndianType::Big;
#else
			return Endian::checkLittleEndianRuntime() ? EndianType::Little : EndianType::Big;
#endif
		}

		static sl_bool isLE() noexcept
		{
#if defined(SLIB_ARCH_IS_LITTLE_ENDIAN)
			return sl_true;
#elif defined(SLIB_ARCH_IS_BIG_ENDIAN)
			return sl_false;
#else
			return Endian::checkLittleEndianRuntime();
#endif
		}

		static sl_bool isBE() noexcept
		{
#if defined(SLIB_ARCH_IS_LITTLE_ENDIAN)
			return sl_false;
#elif defined(SLIB_ARCH_IS_BIG_ENDIAN)
			return sl_true;
#else
			return Endian::checkBigEndianRuntime();
#endif
		}

		static sl_uint16 swap16(sl_uint16 v) noexcept
		{
			sl_uint8* b = (sl_uint8*)(&v);
			sl_uint8 t = b[0];
			b[0] = b[1];
			b[1] = t;
			return v;
		}

		static sl_uint32 swap32(sl_uint32 v) noexcept
		{
			sl_uint8* b = (sl_uint8*)(&v);
			for (int i = 0; i < 2; i++) {
				sl_uint8 t = b[i];
				b[i] = b[3-i];
				b[3-i] = t;
			}
			return v;
		}

		static sl_uint64 swap64(sl_uint64 v) noexcept
		{
			sl_uint8* b = (sl_uint8*)(&v);
			for (int i = 0; i < 4; i++) {
				sl_uint8 t = b[i];
				b[i] = b[7 - i];
				b[7 - i] = t;
			}
			return v;
		}


		static float swapFloat(float v) noexcept
		{
			sl_uint8* b = (sl_uint8*)(&v);
			for (int i = 0; i < 2; i++) {
				sl_uint8 t = b[i];
				b[i] = b[3 - i];
				b[3 - i] = t;
			}
			return v;
		}

		static double swapDouble(double v) noexcept
		{
			sl_uint8* b = (sl_uint8*)(&v);
			for (int i = 0; i < 4; i++) {
				sl_uint8 t = b[i];
				b[i] = b[3 - i];
				b[3 - i] = t;
			}
			return v;
		}


		// swap only if the system is little endian
		static sl_uint16 swap16LE(sl_uint16 v) noexcept
		{
			if (isLE()) {
				return swap16(v);
			} else {
				return v;
			}
		}

		static sl_uint32 swap32LE(sl_uint32 v) noexcept
		{
			if (isLE()) {
				return swap32(v);
			} else {
				return v;
			}
		}

		static sl_uint64 swap64LE(sl_uint64 v) noexcept
		{
			if (isLE()) {
				return swap64(v);
			} else {
				return v;
			}
		}

		static float swapFloatLE(float v) noexcept
		{
			if (isLE()) {
				return swapFloat(v);
			} else {
				return v;
			}
		}

		static double swapDoubleLE(double v) noexcept
		{
			if (isLE()) {
				return swapDouble(v);
			} else {
				return v;
			}
		}

		// swap only if the system is big endian
		static sl_uint16 swap16BE(sl_uint16 v) noexcept
		{
			if (isLE()) {
				return v;
			} else {
				return swap16(v);
			}
		}

		static sl_uint32 swap32BE(sl_uint32 v) noexcept
		{
			if (isLE()) {
				return v;
			} else {
				return swap32(v);
			}
		}

		static sl_uint64 swap64BE(sl_uint64 v) noexcept
		{
			if (isLE()) {
				return v;
			} else {
				return swap64(v);
			}
		}

		static float swapFloatBE(float v) noexcept
		{
			if (isLE()) {
				return v;
			} else {
				return swapFloat(v);
			}
		}

		static double swapDoubleBE(double v) noexcept
		{
			if (isLE()) {
				return v;
			} else {
				return swapDouble(v);
			}
		}

	};

}

#endif

