/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CRYPTO_PBKDF
#define CHECKHEADER_SLIB_CRYPTO_PBKDF

#include "hmac.h"

/*

	PKCS #5: PBKDF (Password-Based Key Derivation Function)
 
 		https://tools.ietf.org/html/rfc8018

*/

namespace slib
{

	template <class HASH>
	class SLIB_EXPORT PBKDF1
	{
	public:
		/*
			Input:
				password, lenPassword
				salt, lenSalt: 8 bytes for PBKDF1 specification
				nIteration: iteration count
				lenDK: intended length of derived key, at most 16 for MD5 and 20 for SHA-1 (at most `HASH size`)
			Output:
				outDK: derived key
		*/
		static void generateKey(
			const void* password, sl_size lenPassword,
			const void* salt, sl_size lenSalt,
			sl_uint32 nIteration,
			void* _outDK, sl_uint32 lenDK)
		{
			char t[HASH::HashSize];
			char* outDK = (char*)_outDK;

			HASH hash;
			hash.start();
			hash.update(password, lenPassword);
			hash.update(salt, lenSalt);
			hash.finish(t);

			sl_uint32 i;
			for (i = 1; i < nIteration; i++) {
				hash.start();
				hash.update(t, HASH::HashSize);
				hash.finish(t);
			}
			for (i = 0; i < lenDK; i++) {
				outDK[i] = t[i];
			}
		}

	};

	typedef PBKDF1<SHA256> PBKDF1_SHA256;

	template <class KEYED_HASH>
	class SLIB_EXPORT PBKDF2
	{
	public:
		/*
			Input:
				password, lenPassword
				salt, lenSalt
				nIteration: iteration count
				lenDK: intended length of the derived key, at most (2^32 - 1) * `HASH size`
			Output:
				outDK: derived key
		*/
		static void generateKey(
			const void* password, sl_size lenPassword,
			const void* salt, sl_size lenSalt,
			sl_uint32 nIteration,
			void* _outDK, sl_size lenDK)
		{
			char f[KEYED_HASH::HashSize];
			char u[KEYED_HASH::HashSize];

			sl_uint8 bi[4] = {0, 0, 0, 1};
			char* outDK = (char*)_outDK;
			char* endDK = outDK + lenDK;

			KEYED_HASH h;
			sl_uint32 i = 1;
			sl_uint32 j, k;

			for (;;) {
				h.start(password, lenPassword);
				h.update(salt, lenSalt);
				h.update(bi, 4);
				h.finish(u);
				for (j = 0; j < KEYED_HASH::HashSize; j++) {
					f[j] = u[j];
				}
				for (k = 1; k < nIteration; k++) {
					h.start(password, lenPassword);
					h.update(u, KEYED_HASH::HashSize);
					h.finish(u);
					for (j = 0; j < KEYED_HASH::HashSize; j++) {
						f[j] ^= u[j];
					}
				}
				if (outDK + (sl_size)(KEYED_HASH::HashSize) >= endDK) {
					Base::copyMemory(outDK, f, endDK - outDK);
					break;
				} else {
					Base::copyMemory(outDK, f, KEYED_HASH::HashSize);
				}
				outDK += KEYED_HASH::HashSize;
				i++;
				bi[0] = (sl_uint8)(i >> 24);
				bi[1] = (sl_uint8)(i >> 16);
				bi[2] = (sl_uint8)(i >> 8);
				bi[3] = (sl_uint8)(i);
			}
		}

	};

	template <class HASH>
	using PBKDF2_HMAC = PBKDF2< HMAC<HASH> >;

	typedef PBKDF2_HMAC<SHA256> PBKDF2_HMAC_SHA256;

}

#endif
