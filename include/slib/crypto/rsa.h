/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CRYPTO_RSA
#define CHECKHEADER_SLIB_CRYPTO_RSA

#include "hash.h"

#include "../math/bigint.h"
#include "../math/math.h"
#include "../crypto/mgf.h"

namespace slib
{

	class SLIB_EXPORT RSAPublicKey
	{
	public:
		BigInt N; // modulus
		BigInt E; // public exponent

	public:
		RSAPublicKey();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(RSAPublicKey)

	public:
		sl_bool isDefined() const noexcept;

		sl_uint32 getLength() const noexcept;

	};

	class SLIB_EXPORT RSAPrivateKey : public RSAPublicKey
	{
	public:
		BigInt D; // private exponent
		BigInt P; // prime 1
		BigInt Q; // prime 2
		BigInt DP; // exponent1, D mod (P-1)
		BigInt DQ; // exponent2, D mod (Q-1)
		BigInt IQ; // Q^-1 mod P

		// Use N and D only for decryption
		sl_bool flagUseOnlyD;

	public:
		RSAPrivateKey();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(RSAPrivateKey)

	public:
		sl_bool isDefined() const noexcept;

		void generate(sl_uint32 nBits) noexcept;

		sl_bool generateFromPrimes(sl_uint32 nBits) noexcept;

	};

	class SLIB_EXPORT RSA
	{
	public:
		static BigInt executePublic(const RSAPublicKey& key, const BigInt& input);

		static BigInt executePrivate(const RSAPrivateKey& key, const BigInt& input);

		static sl_bool executePublic(const RSAPublicKey& key, const void* input, sl_size sizeInput, void* output);

		static sl_bool executePrivate(const RSAPrivateKey& key, const void* input, sl_size sizeInput, void* output);

		static sl_bool execute(const RSAPublicKey* keyPublic, const RSAPrivateKey* keyPrivate, const void* input, sl_size sizeInput, void* output);

		/*
			PKCS#1 v1.5 Random Padding
		*/
		static sl_bool encrypt_pkcs1_v15(const RSAPublicKey* keyPublic, const RSAPrivateKey* keyPrivate, const void* input, sl_size sizeInput, void* output);

		static Memory encrypt_pkcs1_v15(const RSAPublicKey* keyPublic, const RSAPrivateKey* keyPrivate, const void* input, sl_size sizeInput);

		// returns output size
		static sl_uint32 decrypt_pkcs1_v15(const RSAPublicKey* keyPublic, const RSAPrivateKey* keyPrivate, const void* input, sl_size sizeInput, void* output, sl_bool* pFlagSign = sl_null);

		static Memory decrypt_pkcs1_v15(const RSAPublicKey* keyPublic, const RSAPrivateKey* keyPrivate, const void* input, sl_size sizeInput, sl_bool* pFlagSign = sl_null);

		static sl_bool encryptPublic_pkcs1_v15(const RSAPublicKey& key, const void* input, sl_size sizeInput, void* output);

		static Memory encryptPublic_pkcs1_v15(const RSAPublicKey& key, const void* input, sl_size sizeInput);

		static sl_bool encryptPrivate_pkcs1_v15(const RSAPrivateKey& key, const void* input, sl_size sizeInput, void* output);

		static Memory encryptPrivate_pkcs1_v15(const RSAPrivateKey& key, const void* input, sl_size sizeInput);

		// returns output size
		static sl_uint32 decryptPublic_pkcs1_v15(const RSAPublicKey& key, const void* input, sl_size sizeInput, void* output, sl_bool* pFlagSign = sl_null);

		static Memory decryptPublic_pkcs1_v15(const RSAPublicKey& key, const void* input, sl_size sizeInput, sl_bool* pFlagSign = sl_null);

		// returns output size
		static sl_uint32 decryptPrivate_pkcs1_v15(const RSAPrivateKey& key, const void* input, sl_size sizeInput, void* output, sl_bool* pFlagSign = sl_null);

		static Memory decryptPrivate_pkcs1_v15(const RSAPrivateKey& key, const void* input, sl_size sizeInput, sl_bool* pFlagSign = sl_null);

		/*
			PKCS#1 v2.1 OAEP - Optimal Asymmetric Encryption Padding
		*/
		template <class HASH>
		static sl_bool encrypt_oaep_v21(const RSAPublicKey* keyPublic, const RSAPrivateKey* keyPrivate, const void* _input, sl_size sizeInput, void* _output, const void* label = sl_null, sl_size sizeLabel = 0)
		{
			sl_uint32 sizeRSA;
			if (keyPublic) {
				sizeRSA = keyPublic->getLength();
			} else {
				sizeRSA = keyPrivate->getLength();
			}
			if (!sizeInput || (sizeInput & 0x80000000) || sizeRSA < sizeInput + 2 * HASH::HashSize + 2) {
				return sl_false;
			}

			const sl_uint8* input = (const sl_uint8*)_input;
			sl_uint8* output = (sl_uint8*)_output;
			sl_uint8* seed = output + 1;
			sl_uint8* DB = seed + HASH::HashSize;
			sl_uint32 sizeDB = sizeRSA - HASH::HashSize - 1;
			sl_uint8* lHash = DB;
			sl_uint8* PS = lHash + HASH::HashSize; // Zero Area
			sl_uint8* M = output + (sizeRSA - sizeInput);

			Base::moveMemory(M, input, sizeInput);
			*output = 0;
			Math::randomMemory(seed, HASH::HashSize);
			HASH::hash(label, sizeLabel, lHash);

			Base::zeroMemory(PS, M - PS - 1);
			*(M - 1) = 1;
			MGF1<HASH>::applyMask(seed, HASH::HashSize, DB, sizeDB);
			MGF1<HASH>::applyMask(DB, sizeDB, seed, HASH::HashSize);

			return execute(keyPublic, keyPrivate, output, sizeRSA, output);
		}

		// returns output size
		template <class HASH>
		static sl_uint32 decrypt_oaep_v21(const RSAPublicKey* keyPublic, const RSAPrivateKey* keyPrivate, const void* input, sl_size sizeInput, void* _output, const void* label = sl_null, sl_size sizeLabel = 0)
		{
			sl_uint32 sizeRSA;
			if (keyPublic) {
				sizeRSA = keyPublic->getLength();
			} else {
				sizeRSA = keyPrivate->getLength();
			}
			if (sizeRSA < 2 * HASH::HashSize + 2) {
				return 0;
			}
			sl_uint8* output = (sl_uint8*)_output;
			if (!(execute(keyPublic, keyPrivate, input, sizeInput, output))) {
				return 0;
			}

			sl_uint8* seed = output + 1;
			sl_uint8* DB = seed + HASH::HashSize;
			sl_uint32 sizeDB = sizeRSA - HASH::HashSize - 1;
			sl_uint8* lHash = DB;

			MGF1<HASH>::applyMask(DB, sizeDB, seed, HASH::HashSize);
			MGF1<HASH>::applyMask(seed, HASH::HashSize, DB, sizeDB);

			HASH::hash(label, sizeLabel, seed);
			sl_uint8 _check = output[0];
			sl_uint32 i;
			for (i = 0; i < HASH::HashSize; i++) {
				_check |= (seed[i] - lHash[i]);
			}
			for (i = HASH::HashSize; i < sizeDB; i++) {
				if (!_check && DB[i] == 1) {
					sl_uint32 size = sizeDB - i - 1;
					Base::moveMemory(output, DB + i + 1, size);
					return size;
				}
				if (DB[i]) {
					_check = 1;
				}
			}
			return 0;
		}

		template <class HASH>
		static sl_bool encryptPublic_oaep_v21(const RSAPublicKey& key, const void* input, sl_size sizeInput, void* output, const void* label = sl_null, sl_size sizeLabel = 0)
		{
			return encrypt_oaep_v21<HASH>(&key, sl_null, input, sizeInput, output, label, sizeLabel);
		}

		template <class HASH>
		static sl_bool encryptPrivate_oaep_v21(const RSAPrivateKey& key, const void* input, sl_size sizeInput, void* output, const void* label = sl_null, sl_size sizeLabel = 0)
		{
			return encrypt_oaep_v21<HASH>(sl_null, &key, input, sizeInput, output, label, sizeLabel);
		}

		template <class HASH>
		static sl_uint32 decryptPublic_oaep_v21(const RSAPublicKey& key, const void* input, sl_size sizeInput, void* output, const void* label = sl_null, sl_size sizeLabel = 0)
		{
			return decrypt_oaep_v21<HASH>(&key, sl_null, input, sizeInput, output, label, sizeLabel);
		}

		template <class HASH>
		static sl_uint32 decryptPrivate_oaep_v21(const RSAPrivateKey& key, const void* input, sl_size sizeInput, void* output, const void* label = sl_null, sl_size sizeLabel = 0)
		{
			return decrypt_oaep_v21<HASH>(sl_null, &key, input, sizeInput, output, label, sizeLabel);
		}

	};

}

#endif

