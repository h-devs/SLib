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
		// [In] `H`: 16 bytes
		void generateTable(const void* H);

		// [In] `X`: 16 bytes, [Out] `O`: 16 Bytes
		void multiplyH(const void* X, void* O) const;

		// [In, out] `X`: 16 bytes
		void multiplyData(void* X, const void* D, sl_size lenD) const;

		// [In, out] `X`: 16 bytes
		void multiplyLength(void* X, sl_size len1, sl_size len2) const;

		// [Out] `O`: 16 Bytes
		void calculateGHash(const void* A, sl_size lenA, const void* C, sl_size lenC, void* O) const;

		// `lenIV` shoud be at least 12, [Out] `CIV`: 16 Bytes
		void calculateCIV(const void* IV, sl_size lenIV, void* CIV) const;

	};

	class SLIB_EXPORT GCM_Base
	{
	public:
		GCM_Base();

	public:
		void increaseCIV();

		void put(const void* src, sl_size len);

		// [Out] `tag`, 4 <= `lenTag` <= 16
		sl_bool finish(void* tag, sl_size lenTag = 16);

		// 4 <= `lenTag` <= 16
		sl_bool finishAndCheckTag(const void* tag, sl_size lenTag = 16);

	protected:
		void _start();

		sl_bool _encrypt(const sl_uint8*& src, sl_uint8*& dst, sl_size& len);

		sl_bool _decrypt(const sl_uint8*& src, sl_uint8*& dst, sl_size& len);

		sl_bool _finish(sl_size lenTag);

	protected:
		GCM_Table m_table;
		sl_uint8 m_civ[16];
		sl_uint8 m_gctr0[16];
		sl_uint8 m_ghashx[16];
		sl_uint8 m_gctr[16];
		sl_uint32 m_posAad;
		sl_size m_sizeAad;
		sl_uint32 m_posEnc;
		sl_size m_sizeEnc;

	};

	template <class CLASS>
	class SLIB_EXPORT GCM : public GCM_Base
	{
	public:
		GCM()
		{
			m_cipher = sl_null;
		}

		GCM(const CLASS* cipher)
		{
			setCipher(cipher);
		}

	public:
		sl_bool setCipher(const CLASS* cipher)
		{
			if (CLASS::BlockSize != 16) {
				return sl_false;
			}
			m_cipher = cipher;
			sl_uint8 H[16] = { 0 };
			cipher->encryptBlock(H, H);
			m_table.generateTable(H);
			return sl_true;
		}

		sl_bool start(const void* IV, sl_size lenIV)
		{
			if (!m_cipher) {
				return sl_false;
			}
			m_table.calculateCIV(IV, lenIV, m_civ);
			m_cipher->encryptBlock(m_civ, m_gctr0);
			_start();
			return sl_true;
		}

		void encrypt(const void* src, void *dst, sl_size len)
		{
			sl_uint8* GCTR = m_gctr;
			sl_uint8* GHASH_X = m_ghashx;
			const sl_uint8* P = (const sl_uint8*)src;
			sl_uint8* C = (sl_uint8*)dst;
			if (_encrypt(P, C, len)) {
				return;
			}
			for (;;) {
				increaseCIV();
				m_cipher->encryptBlock(m_civ, GCTR);
				if (len < 16) {
					for (sl_size k = 0; k < len; k++) {
						sl_uint8 c = *(P++) ^ GCTR[k];
						GHASH_X[k] ^= c;
						*(C++) = c;
					}
					m_posEnc = (sl_uint32)len;
					return;
				} else {
					for (sl_uint32 k = 0; k < 16; k++) {
						sl_uint8 c = *(P++) ^ GCTR[k];
						GHASH_X[k] ^= c;
						*(C++) = c;
					}
					m_table.multiplyH(GHASH_X, GHASH_X);
					len -= 16;
					if (!len) {
						return;
					}
				}
			}
		}

		void decrypt(const void* src, void *dst, sl_size len)
		{
			sl_uint8* GCTR = m_gctr;
			sl_uint8* GHASH_X = m_ghashx;
			const sl_uint8* C = (const sl_uint8*)src;
			sl_uint8* P = (sl_uint8*)dst;
			if (_decrypt(C, P, len)) {
				return;
			}
			for (;;) {
				increaseCIV();
				m_cipher->encryptBlock(m_civ, GCTR);
				if (len < 16) {
					for (sl_size k = 0; k < len; k++) {
						sl_uint8 c = *(C++);
						GHASH_X[k] ^= c;
						*(P++) = c ^ GCTR[k];
					}
					m_posEnc = (sl_uint32)len;
					return;
				} else {
					for (sl_size k = 0; k < 16; k++) {
						sl_uint8 c = *(C++);
						GHASH_X[k] ^= c;
						*(P++) = c ^ GCTR[k];
					}
					m_table.multiplyH(GHASH_X, GHASH_X);
					len -= 16;
					if (!len) {
						return;
					}
				}
			}
		}

		// [Out] `tag`, 4 <= `lenTag` <= 16
		sl_bool encrypt(const void* IV, sl_size lenIV, const void* A, sl_size lenA, const void* input, void* output, sl_size len, void* tag, sl_size lenTag = 16)
		{
			if (!(start(IV, lenIV))) {
				return sl_false;
			}
			put(A, lenA);
			encrypt(input, output, len);
			return finish(tag, lenTag);
		}

		// 4 <= `lenTag` <= 16
		sl_bool decrypt(const void* IV, sl_size lenIV, const void* A, sl_size lenA, const void* input, void* output, sl_size len, const void* tag, sl_size lenTag = 16)
		{
			if (!(start(IV, lenIV))) {
				return sl_false;
			}
			put(A, lenA);
			decrypt(input, output, len);
			return finishAndCheckTag(tag, lenTag);
		}


#ifdef check
#undef check
#endif

		// 4 <= `lenTag` <= 16
		sl_bool check(const void* IV, sl_size lenIV, const void* A, sl_size lenA, const void* C, sl_size lenC, const void* tag, sl_size lenTag = 16
		)
		{
			if (!(start(IV, lenIV))) {
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
