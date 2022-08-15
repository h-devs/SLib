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

#ifndef CHECKHEADER_SLIB_CRYPTO_BLOWFISH
#define CHECKHEADER_SLIB_CRYPTO_BLOWFISH

#include "block_cipher.h"

/*
	BlowFish

	User Key Size - 32 bits to 448 bits
	Block Size - 64 bits (8 bytes)
*/

namespace slib
{
	
	class String;

	class SLIB_EXPORT Blowfish : public BlockCipher<Blowfish>
	{
	public:
		enum {
			BlockSize = 8
		};
		
	public:
		Blowfish();

		~Blowfish();

	public:
		sl_bool setKey(const void* key, sl_size lenKey /* 4 to 56 bytes */);

		void setKey_SHA256(const String& key);
		
		void encrypt(sl_uint32& d0, sl_uint32& d1) const;
		
		void decrypt(sl_uint32& d0, sl_uint32& d1) const;

		// 64 bit (8 byte) block
		void encryptBlock(const void* src, void* dst) const;

		// 64 bit (8 byte) block
		void decryptBlock(const void* src, void* dst) const;

	private:
		/* The subkeys used by the blowfish cipher */
		sl_uint32 m_P[18];
		sl_uint32 m_S[4][256];

	};
	
}

#endif
