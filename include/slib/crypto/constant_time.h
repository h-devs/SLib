/*
*   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CRYPTO_BASE
#define CHECKHEADER_SLIB_CRYPTO_BASE

#include "definition.h"

namespace slib
{

	class SLIB_EXPORT ConstantTimeUtil
	{
	public:
		SLIB_INLINE static sl_uint8 isNegative(sl_int8 x)
		{
			return (sl_uint8)(((sl_uint32)((sl_int32)x)) >> 31);
		}

		SLIB_INLINE static sl_uint8 equals(sl_uint8 x1, sl_uint8 x2)
		{
			return (sl_uint8)((((sl_uint32)(x1 ^ x2)) - 1) >> 31);
		}

		// Returns the given value with the MSB copied to all the other bits
		SLIB_INLINE static sl_uint32 extendSignBit(sl_uint32 a)
		{
			return 0 - (a >> 31);
		}

		// Returns 0xffffffff if a == 0 and 0 otherwise
		SLIB_INLINE static sl_uint32 getZeroMask(sl_uint32 a)
		{
			return extendSignBit((~a) & (a - 1));
		}

		// mask must be 0xFFFFFFFF or 0x00000000, if (mask) swap(a, b)
		SLIB_INLINE static void swapIfMask(sl_uint32& a, sl_uint32& b, sl_uint32 mask)
		{
			sl_uint32 x = a ^ b;
			x &= mask;
			a ^= x;
			b ^= x;
		}

		// mask must be 0xFFFFFFFF or 0x00000000, c = mask ? a : b
		SLIB_INLINE static sl_uint32 selectIfMask(sl_uint32 a, sl_uint32 b, sl_uint32 mask)
		{
			return (mask & a) | ((~mask) & b);
		}

	};

}

#endif