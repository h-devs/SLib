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

#ifndef CHECKHEADER_SLIB_CRYPTO_HKDF
#define CHECKHEADER_SLIB_CRYPTO_HKDF

#include "hmac.h"

/*

	HKDF (HMAC-Based Key Derivation Function)
 
 		https://tools.ietf.org/html/rfc5869

*/

namespace slib
{

	template <class HASH>
	class SLIB_EXPORT HKDF
	{
	public:
		/*
			Input:
				inputKM, lenInputKM: input keying material
				salt, lenSalt: [Optional]
				info, lenInfo: [Optional]
				lenDK: length of derived key (output) in octets (<= 255 * `HASH size`)
			Output:
				outDK: derived key
		*/
		static void generateKey(
			const void* inputKM, sl_size lenInputKM,
			const void* salt, sl_size lenSalt,
			const void* info, sl_size lenInfo,
			void* _outDK, sl_uint32 lenDK)
		{
			char* outDK = (char*)_outDK;

			char prk[HASH::HashSize]; // pseudorandom key
			HMAC<HASH>::execute(salt, lenSalt, inputKM, lenInputKM, prk);

			char t[HASH::HashSize];
			sl_uint8 ctr = 1;
			sl_size n = (lenDK + sizeof(t) - 1) / sizeof(t);
			for (sl_size i = 0; i < n; i++) {
				HMAC<HASH> hmac;
				hmac.start(prk, sizeof(prk));
				if (i) {
					hmac.update(t, sizeof(t));
				}
				hmac.update(info, lenInfo);
				hmac.update(&ctr, 1);
				hmac.finish(t);
				sl_size m = lenDK;
				if (m > sizeof(t)) {
					m = sizeof(t);
				}
				for (sl_size k = 0; k < m; k++) {
					*(outDK++) = t[k];
				}
				ctr++;
				lenDK -= sizeof(t);
			}
		}

		static void generateKey(
			const void* inputKM, sl_size lenInputKM,
			const void* salt, sl_size lenSalt,
			void* outDK, sl_uint32 lenDK)
		{
			generateKey(inputKM, lenInputKM, salt, lenSalt, sl_null, 0, outDK, lenDK);
		}

		static void generateKey(
			const void* inputKM, sl_size lenInputKM,
			void* outDK, sl_uint32 lenDK)
		{
			generateKey(inputKM, lenInputKM, sl_null, 0, sl_null, 0, outDK, lenDK);
		}

	};

	typedef HKDF<SHA256> HKDF_SHA256;

}

#endif
