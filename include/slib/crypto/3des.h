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

#ifndef CHECKHEADER_SLIB_CRYPTO_3DES
#define CHECKHEADER_SLIB_CRYPTO_3DES

#include "des.h"

namespace slib
{

	class SLIB_EXPORT TripleDES : public BlockCipher<TripleDES>
	{
	public:
		enum {
			BlockSize = 8
		};

	public:
		TripleDES();

		~TripleDES();

	public:
		// 3 x 64 bit (8 bytes), note that most significant bit of each byte is not used
		void setKey(const void* key1, const void* key2, const void* key3);
		void setKey(sl_uint64 key1, sl_uint64 key2, sl_uint64 key3);
		void setKey(const void* key); // 24 Bytes
		void setKey2(const void* key1, const void* key2);
		void setKey2(sl_uint64 key1, sl_uint64 key2);
		void setKey2(const void* key); // 16 Bytes

		// 16 or 24 bytes
		sl_bool setKey(const void* key, sl_size lenKey);

		sl_uint64 encrypt(sl_uint64 data) const;

		sl_uint64 decrypt(sl_uint64 data) const;

		// 64 bits (8 bytes) block
		void encryptBlock(const void* src, void* dst) const;

		// 64 bits (8 bytes) block
		void decryptBlock(const void* src, void* dst) const;

	private:
		DES m_des1;
		DES m_des2;
		DES m_des3;

	};

}

#endif
