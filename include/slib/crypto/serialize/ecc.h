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

#ifndef CHECKHEADER_SLIB_CRYPTO_SERIALIZE_ECC
#define CHECKHEADER_SLIB_CRYPTO_SERIALIZE_ECC

#include "../ecc.h"

#include "../../core/serialize/primitive.h"
#include "../../core/serialize/generic.h"

namespace slib
{

	template <class OUTPUT>
	SLIB_INLINE sl_bool ECPoint::serialize(OUTPUT* output) const
	{
		return toUncompressedFormat().serialize(output);
	}

	template <class INPUT>
	SLIB_INLINE sl_bool ECPoint::deserialize(INPUT* input)
	{
		Memory mem;
		if (mem.deserialize(input)) {
			return parseUncompressedFormat(mem);
		}
		return sl_false;
	}


	template <class OUTPUT>
	SLIB_INLINE sl_bool ECPublicKey::serialize(OUTPUT* output) const
	{
		return Q.serialize(output);
	}

	template <class INPUT>
	SLIB_INLINE sl_bool ECPublicKey::deserialize(INPUT* input)
	{
		return Q.deserialize(input);
	}


	template <class OUTPUT>
	SLIB_INLINE sl_bool ECPrivateKey::serialize(OUTPUT* output) const
	{
		if (!(Q.serialize(output))) {
			return sl_false;
		}
		return d.serialize(output);
	}

	template <class INPUT>
	SLIB_INLINE sl_bool ECPrivateKey::deserialize(INPUT* input)
	{
		if (!(Q.deserialize(input))) {
			return sl_false;
		}
		return d.deserialize(input);
	}


	template <class OUTPUT>
	SLIB_INLINE sl_bool ECDSA_Signature::serialize(OUTPUT* output) const
	{
		return serialize().serialize(output);
	}

	template <class INPUT>
	SLIB_INLINE sl_bool ECDSA_Signature::deserialize(INPUT* input)
	{
		Memory mem;
		if (!(mem.deserialize(input))) {
			return sl_false;
		}
		return deserialize(mem);
	}

}

#endif
