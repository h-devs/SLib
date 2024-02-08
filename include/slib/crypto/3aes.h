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

#ifndef CHECKHEADER_SLIB_CRYPTO_3AES
#define CHECKHEADER_SLIB_CRYPTO_3AES

#include "aes.h"

namespace slib
{

	class SLIB_EXPORT TripleAES : public BlockCipher<TripleAES>
	{
	public:
		enum {
			BlockSize = 16
		};

	public:
		TripleAES();

		~TripleAES();

	public:
		// 3 x 256 bit (96 bytes)
		void setKey(const void* key);
		void setKey(const void* key1, const void* key2, const void* key3);

		void encrypt(sl_uint32& d0, sl_uint32& d1, sl_uint32& d2, sl_uint32& d3) const;

		void decrypt(sl_uint32& d0, sl_uint32& d1, sl_uint32& d2, sl_uint32& d3) const;

		// 128 bits (16 bytes) block
		void encryptBlock(const void* src, void* dst) const;

		// 128 bits (16 bytes) block
		void decryptBlock(const void* src, void* dst) const;

	private:
		AES m_aes1;
		AES m_aes2;
		AES m_aes3;

	};

}

#endif
