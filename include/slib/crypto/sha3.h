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

#ifndef CHECKHEADER_SLIB_CRYPTO_SHA3
#define CHECKHEADER_SLIB_CRYPTO_SHA3

#include "hash.h"

/*
	SHA3 - Secure Hash Algorithm

	Output:
		SHA3_224 - 224bits (28 bytes)
		SHA3_256 - 256bits (32 bytes)
		SHA3_384 - 384bits (48 bytes)
		SHA3_512 - 512bits (64 bytes)
*/

namespace slib
{

	namespace priv
	{
		namespace sha3
		{
			// bit-interleaved internal representation.
			// This stores a 64 bit quantity in two 32 bit words: one word contains odd bits, the other even. This means 64-bit rotations are cheaper to compute.
			struct BitInterleaved64
			{
				sl_uint32 odd, even;
			};

			class SLIB_EXPORT SHA3Base
			{
			public:
				SHA3Base() noexcept;

				~SHA3Base();

			public:
				void start() noexcept;

				void update(const void* input, sl_size n) noexcept;

				void finish(void* output) noexcept;

			protected:

				void _updateBlock(const sl_uint8* input) noexcept;

			protected:
				BitInterleaved64 A[5][5]; // State Blocks for Keccak-f[1600]
				sl_uint8 rdata[144];
				sl_uint32 rdata_len;
				sl_uint32 rate, nhash; // in bytes
			};

		}
	}

	class SLIB_EXPORT SHA3_224 : public priv::sha3::SHA3Base, public CryptoHash<SHA3_224>
	{
	public:
		enum {
			HashSize = 28,
			BlockSize = 144
		};

	public:
		SHA3_224() noexcept;

		~SHA3_224();

	};

	class SLIB_EXPORT SHA3_256 : public priv::sha3::SHA3Base, public CryptoHash<SHA3_256>
	{
	public:
		enum {
			HashSize = 32,
			BlockSize = 136
		};

	public:
		SHA3_256() noexcept;

		~SHA3_256();

	};

	class SLIB_EXPORT SHA3_384 : public priv::sha3::SHA3Base, public CryptoHash<SHA3_384>
	{
	public:
		enum {
			HashSize = 48,
			BlockSize = 104
		};

	public:
		SHA3_384() noexcept;

		~SHA3_384();

	};

	class SLIB_EXPORT SHA3_512 : public priv::sha3::SHA3Base, public CryptoHash<SHA3_512>
	{
	public:
		enum {
			HashSize = 64,
			BlockSize = 72
		};

	public:
		SHA3_512() noexcept;

		~SHA3_512();

	};

}

#endif
