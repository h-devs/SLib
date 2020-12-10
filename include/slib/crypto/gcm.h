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

#ifndef CHECKHEADER_SLIB_CRYPTO_GCM
#define CHECKHEADER_SLIB_CRYPTO_GCM

#include "definition.h"

#include "../math/int128.h"

/*
	GCM - Galois/Counter Mode

	https://en.wikipedia.org/wiki/Galois/Counter_Mode
	http://csrc.nist.gov/publications/nistpubs/800-38D/SP-800-38D.pdf
	http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/proposedmodes/gcm/gcm-revised-spec.pdf

GCM is constructed from an approved symmetric key block cipher with a block size of 128 bits,
such as the Advanced Encryption Standard (AES) algorithm

*/

namespace slib
{
	
	class SLIB_EXPORT GCM_Table
	{
	public:
		Uint128 M[16]; // Shoup's, 4-bit table
	
	public:
		void generateTable(const void* H /* 16 bytes */);

		void multiplyH(const void* X /* 16 bytes */, void* O /* 16 bytes */) const;

		void multiplyData(void* X /* 16 bytes, inout */, const void* D, sl_size lenD) const;
	
		void multiplyLength(void* X /* 16 bytes, inout */, sl_size len1, sl_size len2) const;
	
		void calculateGHash(const void* A, sl_size lenA, const void* C, sl_size lenC, void* O /* 16 bytes */) const;
	
		// lenIV shoud be at least 12
		void calculateCIV(const void* IV, sl_size lenIV, void* CIV /* 16 bytes */) const;

	};
	
	class SLIB_EXPORT GCM_Base : public GCM_Table
	{
	public:
		sl_uint8 CIV[16];
		sl_uint8 GCTR0[16];
		sl_uint8 GHASH_X[16];

	public:
		void increaseCIV();

		void putBlock(const void* src, sl_uint32 n = 16 /* n <= 16 */);

		void put(const void* src, sl_size len);

		sl_bool finish(sl_size lenA, sl_size lenC, void* tag /* out */, sl_size lenTag = 16 /* 4 <= lenTag <= 16 */);

		sl_bool finishAndCheckTag(sl_size lenA, sl_size lenC, const void* tag, sl_size lenTag = 16 /* 4 <= lenTag <= 16 */);

	};
	
	template <class CLASS>
	class SLIB_EXPORT GCM : public GCM_Base
	{
	public:
		GCM()
		{
			setCipher(sl_null);
		}

		GCM(const CLASS* cipher)
		{
			setCipher(cipher);
		}

		~GCM()
		{
		}

	public:
		sl_bool setCipher(const CLASS* cipher)
		{
			if (cipher) {
				if (CLASS::BlockSize != 16) {
					return sl_false;
				}
				sl_uint8 H[16] = { 0 };
				cipher->encryptBlock(H, H);
				generateTable(H);
			}
			m_cipher = cipher;
			return sl_true;
		}

		sl_bool start(const void* IV, sl_size lenIV)
		{
			if (!m_cipher) {
				return sl_false;
			}
			calculateCIV(IV, lenIV, CIV);
			m_cipher->encryptBlock(CIV, GCTR0);
			Base::zeroMemory(GHASH_X, 16);
			return sl_true;
		}

		void encryptBlock(const void* src, void* dst /* out */, sl_uint32 n = 16 /* n <= 16 */)
		{
			sl_uint8 GCTR[16];
			sl_size k;
			const sl_uint8* P = (const sl_uint8*)src;
			sl_uint8* C = (sl_uint8*)dst;
			increaseCIV();
			m_cipher->encryptBlock(CIV, GCTR);
			for (k = 0; k < n; k++) {
				*C = *P ^ GCTR[k];
				GHASH_X[k] ^= *C;
				C++;
				P++;
			}
			multiplyH(GHASH_X, GHASH_X);
		}

		void encrypt(const void* src, void *dst /* out */, sl_size len)
		{
			sl_uint8 GCTR[16];
			sl_size i, k, n;
			const sl_uint8* P = (const sl_uint8*)src;
			sl_uint8* C = (sl_uint8*)dst;
			
			for (i = 0; i < len; i += 16) {
				increaseCIV();
				m_cipher->encryptBlock(CIV, GCTR);
				n = len - i;
				if (n > 16) {
					n = 16;
				}
				for (k = 0; k < n; k++) {
					*C = *P ^ GCTR[k];
					GHASH_X[k] ^= *C;
					C++;
					P++;
				}
				multiplyH(GHASH_X, GHASH_X);
			}
		}

		void decryptBlock(const void* src, void* dst /* out */, sl_uint32 n = 16 /* n <= 16 */)
		{
			sl_uint8 GCTR[16];
			sl_size k;
			const sl_uint8* C = (const sl_uint8*)src;
			sl_uint8* P = (sl_uint8*)dst;
			increaseCIV();
			m_cipher->encryptBlock(CIV, GCTR);
			for (k = 0; k < n; k++) {
				GHASH_X[k] ^= *C;
				*P = *C ^ GCTR[k];
				C++;
				P++;
			}
			multiplyH(GHASH_X, GHASH_X);
		}

		void decrypt(const void* src, void *dst /* out */, sl_size len)
		{
			sl_uint8 GCTR[16];
			sl_size i, k, n;
			const sl_uint8* C = (const sl_uint8*)src;
			sl_uint8* P = (sl_uint8*)dst;
			
			for (i = 0; i < len; i += 16) {
				increaseCIV();
				m_cipher->encryptBlock(CIV, GCTR);
				n = len - i;
				if (n > 16) {
					n = 16;
				}
				for (k = 0; k < n; k++) {
					GHASH_X[k] ^= *C;
					*P = *C ^ GCTR[k];
					C++;
					P++;
				}
				multiplyH(GHASH_X, GHASH_X);
			}
		}

		sl_bool encrypt(
			const void* IV, sl_size lenIV,
			const void* A, sl_size lenA,
			const void* input, void* output /* out */, sl_size len,
			void* tag /* out */, sl_size lenTag = 16 /* 4 <= lenTag <= 16 */
		)
		{
			if (!start(IV, lenIV)) {
				return sl_false;
			}
			put(A, lenA);
			encrypt(input, output, len);
			return finish(lenA, len, tag, lenTag);
		}

		sl_bool decrypt(
			const void* IV, sl_size lenIV,
			const void* A, sl_size lenA,
			const void* input, void* output /* out */, sl_size len,
			const void* tag, sl_size lenTag = 16 /* 4 <= lenTag <= 16 */
		)
		{
			if (!start(IV, lenIV)) {
				return sl_false;
			}
			put(A, lenA);
			decrypt(input, output, len);
			return finishAndCheckTag(lenA, len, tag, lenTag);
		}
	

#ifdef check
#undef check
#endif
		sl_bool check(
			const void* IV, sl_size lenIV,
			const void* A, sl_size lenA,
			const void* C, sl_size lenC,
			const void* tag, sl_size lenTag = 16 /* 4 <= lenTag <= 16 */
		)
		{
			if (!start(IV, lenIV)) {
				return sl_false;
			}
			put(A, lenA);
			put(C, lenC);
			return finishAndCheckTag(lenA, lenC, tag, lenTag);
		}

	protected:
		const CLASS* m_cipher;

	};
	
}

#endif
