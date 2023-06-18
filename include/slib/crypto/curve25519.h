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

#ifndef CHECKHEADER_SLIB_CRYPTO_CURVE25519
#define CHECKHEADER_SLIB_CRYPTO_CURVE25519

#include "definition.h"

#include "../core/bytes.h"

namespace slib
{

	class SLIB_EXPORT X25519
	{
	public:
		// `privateKey`: 32 Bytes
		static Bytes<32> getPublicKey(const void* privateKey);

		// `privateKey`, `publicKey`: 32 bytes
		static Bytes<32> getSharedKey(const void* privateKey, const void* publicKey);

	};

	class SLIB_EXPORT Ed25519
	{
	public:
		// `privateKey`: 32 Bytes
		static Bytes<32> getPublicKey(const void* privateKey);

		// `privateKey`, `publicKey`: 32 Bytes, `outSignature`: 64 Bytes
		static void sign(const void* privateKey, const void* publicKey, const void* message, sl_size messageSize, void* outSignature);

		// `publicKey`: 32 Bytes, `signature`: 64 Bytes
		static sl_bool verify(const void* publicKey, void* message, sl_size messageSize, const void* signature);

	};

}

#endif

