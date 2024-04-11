/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CRYPTO_DES
#define CHECKHEADER_SLIB_CRYPTO_DES

#include "block_cipher.h"

/*
 	DES - Data Encryption Standard

	User Key Size - 56 bits (7 bytes)
	Block Size - 64 bits (8 bytes)
*/

namespace slib
{

	class SLIB_EXPORT DES : public BlockCipher<DES>
	{
	public:
		enum {
			BlockSize = 8
		};

	public:
		DES();

		~DES();

	public:
		void setKey(sl_uint64 key);

		// 64 bit (8 bytes), note that most significant bit of each byte is not used
		void setKey(const void* key);

		sl_bool setKey(const void* key, sl_size lenKey);

		sl_uint64 encrypt(sl_uint64 data) const;

		sl_uint64 decrypt(sl_uint64 data) const;

		// 64 bits (8 bytes) block
		void encryptBlock(const void* src, void* dst) const;

		// 64 bits (8 bytes) block
		void decryptBlock(const void* src, void* dst) const;

	private:
		sl_uint64 m_K[16];

	};

}

#endif
