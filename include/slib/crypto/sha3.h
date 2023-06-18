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
		SHAKE128<d> - d bytes
		SHAKE256<d> - d bytes
*/

namespace slib
{

	class SLIB_EXPORT SHA3
	{
	public:
		SHA3(sl_size hashSize, sl_uint32 blockSize, sl_bool flagSHAKE = sl_false) noexcept;

		~SHA3();

	public:
		void start() noexcept;

		void update(const void* input, sl_size n) noexcept;

		void finish(void* output) noexcept;

	protected:
		void _update(const sl_uint8* input) noexcept;

		void _keccak();

		void _finish(void* output, sl_size size) noexcept;

	protected:
		sl_uint32 m_rate;
		sl_size m_hashSize; // in bytes
		sl_uint8 m_suffix;
		sl_uint64 m_state[5][5];
		sl_uint8 m_rdata[200];
		sl_uint32 m_rlen;
	};

	class SLIB_EXPORT SHA3_224 : public SHA3, public CryptoHash<SHA3_224>
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

	class SLIB_EXPORT SHA3_256 : public SHA3, public CryptoHash<SHA3_256>
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

	class SLIB_EXPORT SHA3_384 : public SHA3, public CryptoHash<SHA3_384>
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

	class SLIB_EXPORT SHA3_512 : public SHA3, public CryptoHash<SHA3_512>
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

	class SLIB_EXPORT SHAKE128 : public SHA3
	{
	public:
		SHAKE128() noexcept;

		~SHAKE128();

	public:
		void finish(void* output, sl_size size) noexcept;

		static void hash(const void* input, sl_size sizeInput, void* output, sl_size sizeOutput) noexcept;

	};

	template <sl_uint32 HASH_BITS>
	class SLIB_EXPORT SHAKE128H : public SHA3, public CryptoHash< SHAKE128H<HASH_BITS> >
	{
	public:
		enum {
			HashSize = HASH_BITS >> 3,
			BlockSize = 168
		};

	public:
		SHAKE128H() noexcept: SHA3(HashSize, BlockSize, sl_true) {}

	};

	class SLIB_EXPORT SHAKE256 : public SHA3
	{
	public:
		SHAKE256() noexcept;

		~SHAKE256();

	public:
		void finish(void* output, sl_size size) noexcept;

		static void hash(const void* input, sl_size sizeInput, void* output, sl_size sizeOutput) noexcept;

	};

	template <sl_uint32 HASH_BITS>
	class SLIB_EXPORT SHAKE256H : public SHA3, public CryptoHash< SHAKE256H<HASH_BITS> >
	{
	public:
		enum {
			HashSize = HASH_BITS >> 3,
			BlockSize = 136
		};

	public:
		SHAKE256H() noexcept: SHA3(HashSize, BlockSize, sl_true) {}

	};

}

#endif
