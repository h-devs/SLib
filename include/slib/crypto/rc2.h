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

#ifndef CHECKHEADER_SLIB_CRYPTO_RC2
#define CHECKHEADER_SLIB_CRYPTO_RC2

#include "block_cipher.h"

namespace slib
{

	class String;

	class SLIB_EXPORT RC2 : public BlockCipher<RC2>
	{
	public:
		enum {
			BlockSize = 8
		};

	public:
		RC2();

		~RC2();

	public:
		// 8-1024 bits (1-128 bytes)
		void setKey(const void* key, sl_uint32 lenKey, sl_int32 nBits = -1);

		void encrypt(sl_uint32& d0, sl_uint32& d1) const;
	
		void decrypt(sl_uint32& d0, sl_uint32& d1) const;
		
		// 64 bits (8 bytes) block
		void encryptBlock(const void* src, void* dst) const;

		// 64 bits (8 bytes) block
		void decryptBlock(const void* src, void* dst) const;

	private:
		sl_uint16 m_key[64];

	};
	
}

#endif
