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

#ifndef CHECKHEADER_SLIB_CRYPTO_HMAC
#define CHECKHEADER_SLIB_CRYPTO_HMAC

#include "sha2.h"

/*
	HMAC: Keyed-Hashing for Message Authentication (RFC-2104)
*/

namespace slib
{
	
	template <class HASH>
	class SLIB_EXPORT HMAC
	{
	public:
		enum {
			HashSize = HASH::HashSize
		};

	public:
		void start(const void* _key, sl_size lenKey)
		{
			sl_size i;
			const sl_uint8* key = (const sl_uint8*)_key;
			sl_uint8 keyLocal[HASH::BlockSize];
			if (lenKey > HASH::BlockSize) {
				HASH::hash(key, lenKey, keyLocal);
				for (i = HASH::HashSize; i < HASH::BlockSize; i++) {
					keyLocal[i] = 0;
				}
				key = keyLocal;
			} else if (lenKey < HASH::BlockSize) {
				for (i = 0; i < lenKey; i++) {
					keyLocal[i] = key[i];
				}
				for (; i < HASH::BlockSize; i++) {
					keyLocal[i] = 0;
				}
				key = keyLocal;
			}

			// hash(o_key_pad | hash(i_key_pad | message)), i_key_pad = key xor [0x36 * BlockSize], o_key_pad = key xor [0x5c * BlockSize]
			m_hash.start();
			for (i = 0; i < HASH::BlockSize; i++) {
				m_keyPad[i] = key[i] ^ 0x36;
			}
			m_hash.update(m_keyPad, HASH::BlockSize);
			for (i = 0; i < HASH::BlockSize; i++) {
				m_keyPad[i] = key[i] ^ 0x5c;
			}
		}

		void update(const void* input, sl_size n)
		{
			m_hash.update(input, n);
		}

		// output: length of HASH size
		void finish(void* output)
		{
			m_hash.finish(output);
			m_hash.start();
			m_hash.update(m_keyPad, HASH::BlockSize);
			m_hash.update(output, HASH::HashSize);
			m_hash.finish(output);
		}

		// output: length of HASH size
		static void execute(const void* key, sl_size lenKey, const void* message, sl_size lenMessage, void* output)
		{
			HMAC<HASH> hmac;
			hmac.start(key, lenKey);
			hmac.update(message, lenMessage);
			hmac.finish(output);
		}

	private:
		HASH m_hash;
		sl_uint8 m_keyPad[HASH::BlockSize];

	};
	
	typedef HMAC<SHA256> HMAC_SHA256;

}

#endif
