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

#include "definition.h"

#include "hmac.h"

/*

	PKCS #5: PBKDF (Password-Based Key Derivation Function)
 
 		https://tools.ietf.org/html/rfc2898

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
			const void* password, sl_uint32 lenPassword,
			const void* salt, sl_uint32 lenSalt,
			sl_uint32 nIteration,
			void* outDK, sl_uint32 lenDK);

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
			const void* password, sl_uint32 lenPassword,
			const void* salt, sl_uint32 lenSalt,
			sl_uint32 nIteration,			
			void* outDK, sl_size lenDK);

	};

	template <class HASH>
	class SLIB_EXPORT PBKDF2_HMAC : public PBKDF2< HMAC<HASH> >
	{
	};

	typedef PBKDF2_HMAC<SHA256> PBKDF2_HMAC_SHA256;

}

#include "detail/pbkdf.inc"

#endif
