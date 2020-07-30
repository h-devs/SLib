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

namespace slib
{
	
	template <class HASH>
	void HMAC<HASH>::start(const void* _key, sl_size lenKey)
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

	template <class HASH>
	void HMAC<HASH>::update(const void* message, sl_size lenMessage)
	{
		m_hash.update(message, lenMessage);
	}

	template <class HASH>
	void HMAC<HASH>::finish(void* output)
	{
		m_hash.finish(output);
		m_hash.start();
		m_hash.update(m_keyPad, HASH::BlockSize);
		m_hash.update(output, HASH::HashSize);
		m_hash.finish(output);
	}

	template <class HASH>
	void HMAC<HASH>::execute(const void* key, sl_size lenKey, const void* message, sl_size lenMessage, void* output)
	{
		HMAC<HASH> hmac;
		hmac.start(key, lenKey);
		hmac.update(message, lenMessage);
		hmac.finish(output);
	}

}
