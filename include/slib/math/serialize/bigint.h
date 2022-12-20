/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_MATH_SERIALIZE_BIGINT
#define CHECKHEADER_SLIB_MATH_SERIALIZE_BIGINT

#include "../bigint.h"

#include "../../data/serialize/primitive.h"
#include "../../data/serialize/variable_length_integer.h"
#include "../../data/serialize/generic.h"

namespace slib
{

	template <class OUTPUT>
	SLIB_INLINE sl_bool BigInt::serialize(OUTPUT* output) const
	{
		sl_uint32* e = getElements();
		sl_size n = getMostSignificantElements();
		if (!(CVLI::serialize(output, (n << 1) | (getSign() < 0 ? 1 : 0)))) {
			return sl_false;
		}
		for (sl_size i = 0; i < n; i++) {
			if (!(Serialize(output, e[i]))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	template <class INPUT>
	SLIB_INLINE sl_bool BigInt::deserialize(INPUT* input)
	{
		sl_size n;
		if (!(CVLI::deserialize(input, n))) {
			return sl_false;
		}
		sl_bool flagNegative = (sl_bool)(n & 1);
		n >>= 1;
		CBigInt* bi = CBigInt::allocate(n);
		if (!bi) {
			return sl_false;
		}
		sl_uint32* e = bi->elements;
		for (sl_size i = 0; i < n; i++) {
			if (!(Deserialize(input, e[i]))) {
				delete bi;
				return sl_false;
			}
		}
		if (flagNegative) {
			bi->sign = -1;
		}
		ref = bi;
		return sl_true;
	}

}

#endif
