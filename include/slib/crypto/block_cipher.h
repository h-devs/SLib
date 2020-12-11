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

#ifndef CHECKHEADER_SLIB_CRYPTO_BLOCKCIPHER
#define CHECKHEADER_SLIB_CRYPTO_BLOCKCIPHER

#include "definition.h"

#include "../core/memory.h"
#include "../core/base.h"
#include "../core/mio.h"
#include "../core/math.h"

namespace slib
{

/*
	Defines padding method described in PKCS#5, PKCS#7.

	Padding is added at the end of the message as following.

	01
	02 02
	03 03 03
	04 04 04 04
	05 05 05 05 05

	only applicable up to 256-bytes
*/
	class SLIB_EXPORT BlockCipherPadding_PKCS7
	{
	public:
		static void addPadding(void* buf, sl_size padding);

		static sl_uint32 removePadding(const void* buf, sl_uint32 blockSize);

	};

/*
	Electronic codebook (ECB)
*/
	template <class CLASS, class PADDING>
	class SLIB_EXPORT BlockCipher_ECB
	{
	public:
		// Output Size = (size / block + 1) * block (< size + block)
		static sl_size encrypt(const CLASS* crypto, const void* _src, sl_size size, void* _dst)
		{
			const char* src = (const char*)(_src);
			char* dst = (char*)(_dst);
			sl_size n = size / CLASS::BlockSize;
			for (sl_size i = 0; i < n; i++) {
				crypto->encryptBlock(src, dst);
				src += CLASS::BlockSize;
				dst += CLASS::BlockSize;
			}
			char last[CLASS::BlockSize];
			sl_size p = n * CLASS::BlockSize;
			sl_uint32 m = (sl_uint32)(size - p);
			Base::copyMemory(last, src, m);
			PADDING::addPadding(last + m, CLASS::BlockSize - m);
			crypto->encryptBlock(last, dst);
			return p + CLASS::BlockSize;
		}

		static Memory encrypt(const CLASS* crypto, const void* src, sl_size size)
		{
			Memory mem = Memory::create(size + CLASS::BlockSize);
			if (mem.isNotNull()) {
				sl_size n = encrypt(crypto, src, size, mem.getData());
				if (n) {
					return mem.sub(0, n);
				}
			}
			return sl_null;
		}

		// destination buffer size must equals to or greater than size
		static sl_size decrypt(const CLASS* crypto, const void* _src, sl_size size, void* _dst)
		{
			const char* src = (const char*)(_src);
			char* dst = (char*)(_dst);
			if (size % CLASS::BlockSize != 0) {
				return 0;
			}
			sl_size n = size / CLASS::BlockSize;
			for (sl_size i = 0; i < n; i++) {
				crypto->decryptBlock(src, dst);
				src += CLASS::BlockSize;
				dst += CLASS::BlockSize;
			}
			sl_uint32 padding = PADDING::removePadding(dst - CLASS::BlockSize, CLASS::BlockSize);
			if (padding > 0) {
				return size - padding;
			} else {
				return 0;
			}
		}


		static Memory decrypt(const CLASS* crypto, const void* src, sl_size size)
		{
			Memory mem = Memory::create(size);
			if (mem.isNotNull()) {
				sl_size n = decrypt(crypto, src, size, mem.getData());
				if (n) {
					return mem.sub(0, n);
				}
			}
			return sl_null;
		}
		
	};


/*
	Cipher-block chaining (CBC)
*/
	template <class CLASS, class PADDING>
	class SLIB_EXPORT BlockCipher_CBC
	{
	public:
		// Output Size = (size / block + 1) * block (< size + block)
		static sl_size encrypt(const CLASS* crypto, const void* _iv, const void* _src, sl_size size, void* _dst)
		{
			const char* src = (const char*)(_src);
			char* dst = (char*)(_dst);
			const char* iv = (const char*)_iv;
			sl_size n = size / CLASS::BlockSize;
			char msg[256];
			for (sl_size i = 0; i < n; i++) {
				for (sl_uint32 k = 0; k < CLASS::BlockSize; k++) {
					msg[k] = src[k] ^ iv[k];
				}
				crypto->encryptBlock(msg, dst);
				iv = dst;
				src += CLASS::BlockSize;
				dst += CLASS::BlockSize;
			}
			sl_size p = n * CLASS::BlockSize;
			{
				sl_uint32 m = (sl_uint32)(size - p);
				for (sl_uint32 k = 0; k < m; k++) {
					msg[k] = src[k] ^ iv[k];
				}
				PADDING::addPadding(msg + m, CLASS::BlockSize - m);
				for (sl_uint32 k = m; k < CLASS::BlockSize; k++) {
					msg[k] = msg[k] ^ iv[k];
				}
				crypto->encryptBlock(msg, dst);
			}
			return p + CLASS::BlockSize;
		}

		static Memory encrypt(const CLASS* crypto, const void* iv, const void* src, sl_size size)
		{
			Memory mem = Memory::create(size + CLASS::BlockSize);
			if (mem.isNotNull()) {
				sl_size n = encrypt(crypto, iv, src, size, mem.getData());
				if (n) {
					return mem.sub(0, n);
				}
			}
			return sl_null;
		}

		// Output Size = (size / block + 2) * block (< size + block*2)
		static sl_size encrypt(const CLASS* crypto, const void* src, sl_size size, void* dst)
		{
			Math::randomMemory(dst, CLASS::BlockSize);
			return encrypt(crypto, dst, src, size, ((char*)dst) + (int)(CLASS::BlockSize)) + CLASS::BlockSize;
		}

		static Memory encrypt(const CLASS* crypto, const void* src, sl_size size)
		{
			Memory mem = Memory::create(size + CLASS::BlockSize * 2);
			if (mem.isNotNull()) {
				sl_size n = encrypt(crypto, src, size, mem.getData());
				if (n) {
					return mem.sub(0, n);
				}
			}
			return sl_null;
		}

		// destination buffer size must equals to or greater than size
		static sl_size decrypt(const CLASS* crypto, const void* _iv, const void* _src, sl_size size, void* _dst)
		{
			const char* src = (const char*)(_src);
			char* dst = (char*)(_dst);
			const char* iv = (const char*)_iv;
			if (size % CLASS::BlockSize != 0) {
				return 0;
			}
			sl_size n = size / CLASS::BlockSize;
			for (sl_size i = 0; i < n; i++) {
				crypto->decryptBlock(src, dst);
				for (sl_uint32 k = 0; k < CLASS::BlockSize; k++) {
					dst[k] ^= iv[k];
				}
				iv = src;
				src += CLASS::BlockSize;
				dst += CLASS::BlockSize;
			}
			sl_uint32 padding = PADDING::removePadding(dst - CLASS::BlockSize, CLASS::BlockSize);
			if (padding > 0) {
				return size - padding;
			} else {
				return 0;
			}
		}

		static Memory decrypt(const CLASS* crypto, const void* iv, const void* src, sl_size size)
		{
			Memory mem = Memory::create(size);
			if (mem.isNotNull()) {
				sl_size n = decrypt(crypto, iv, src, size, mem.getData());
				if (n) {
					return mem.sub(0, n);
				}
			}
			return sl_null;
		}

		// destination buffer size must equals to or greater than size
		static sl_size decrypt(const CLASS* crypto, const void* src, sl_size size, void* dst)
		{
			if (size < CLASS::BlockSize) {
				return 0;
			}
			return decrypt(crypto, src, ((char*)src) + (int)(CLASS::BlockSize), size - CLASS::BlockSize, dst);
		}
		static Memory decrypt(const CLASS* crypto, const void* src, sl_size size)
		{
			Memory mem = Memory::create(size);
			if (mem.isNotNull()) {
				sl_size n = decrypt(crypto, src, size, mem.getData());
				if (n) {
					return mem.sub(0, n);
				}
			}
			return sl_null;
		}

	};

/*
	Counter Mode (CTR)
*/
	template <class CLASS>
	class SLIB_EXPORT BlockCipher_CTR
	{
	public:
		static sl_size encrypt(const CLASS* crypto, const void* _input, sl_size _size, void* _output, void* _counter, sl_uint32 offset)
		{
			if (_size == 0) {
				return 0;
			}
			sl_uint8 mask[CLASS::BlockSize];
			
			if (offset > CLASS::BlockSize) {
				return 0;
			}
			
			sl_uint8* counter = (sl_uint8*)(_counter);
			const sl_uint8* input = (const sl_uint8*)_input;
			sl_uint8* output = (sl_uint8*)_output;
			sl_size size = _size;
			sl_size i, n;
			
			if (offset) {
				crypto->encryptBlock(counter, mask);
				n = CLASS::BlockSize - offset;
				if (size > n) {
					for (i = 0; i < n; i++) {
						output[i] = input[i] ^ mask[i + offset];
					}
					size -= n;
					input += n;
					output += n;
					MIO::increaseBE(counter, CLASS::BlockSize);
				} else {
					for (i = 0; i < size; i++) {
						output[i] = input[i] ^ mask[i + offset];
					}
					return size;
				}
			}
			while (size > 0) {
				crypto->encryptBlock(counter, mask);
				n = SLIB_MIN(CLASS::BlockSize, size);
				for (i = 0; i < n; i++) {
					output[i] = input[i] ^ mask[i];
				}
				size -= n;
				input += n;
				output += n;
				MIO::increaseBE(counter, CLASS::BlockSize);
			}
			return _size;
		}

		static sl_size encrypt(const CLASS* crypto, const void* iv, sl_uint64 counter, sl_uint32 offset, const void* input, sl_size size, void* output)
		{
			if (size == 0) {
				return 0;
			}
			if (CLASS::BlockSize < 16) {
				return 0;
			}
			sl_uint8 IV[CLASS::BlockSize];
			Base::copyMemory(IV, iv, CLASS::BlockSize - 8);
			MIO::writeUint64BE(IV + CLASS::BlockSize - 8, counter);
			return encrypt(crypto, input, size, output, IV, offset);
		}

		static sl_size encrypt(const CLASS* crypto, const void* iv, sl_uint64 pos, const void* input, sl_size size, void* output)
		{
			if (CLASS::BlockSize < 16) {
				return 0;
			}
			return encrypt(crypto, iv, pos / CLASS::BlockSize, (sl_uint32)(pos % CLASS::BlockSize), input, size, output);
		}

	};
	
	template <class CLASS>
	class SLIB_EXPORT BlockCipher
	{
	public:
		void encryptBlocks(const void* _src, void* _dst, sl_size size) const
		{
			const sl_uint8* src = (const sl_uint8*)_src;
			sl_uint8* dst = (sl_uint8*)_dst;
			sl_size nBlocks = size / CLASS::BlockSize;
			for (sl_size i = 0; i < nBlocks; i++) {
				((CLASS*)this)->encryptBlock(src, dst);
				src += CLASS::BlockSize;
				dst += CLASS::BlockSize;
			}
		}
		
		void decryptBlocks(const void* _src, void* _dst, sl_size size) const
		{
			const sl_uint8* src = (const sl_uint8*)_src;
			sl_uint8* dst = (sl_uint8*)_dst;
			sl_size nBlocks = size / CLASS::BlockSize;
			for (sl_size i = 0; i < nBlocks; i++) {
				((CLASS*)this)->decryptBlock(src, dst);
				src += CLASS::BlockSize;
				dst += CLASS::BlockSize;
			}
		}
		
		sl_size encrypt_ECB_PKCS7Padding(const void* src, sl_size size, void* dst) const
		{
			return BlockCipher_ECB<CLASS, BlockCipherPadding_PKCS7>::encrypt((CLASS*)this, src, size, dst);
		}
		
		sl_size decrypt_ECB_PKCS7Padding(const void* src, sl_size size, void* dst) const
		{
			return BlockCipher_ECB<CLASS, BlockCipherPadding_PKCS7>::decrypt((CLASS*)this, src, size, dst);
		}
		
		Memory encrypt_ECB_PKCS7Padding(const void* src, sl_size size) const
		{
			return BlockCipher_ECB<CLASS, BlockCipherPadding_PKCS7>::encrypt((CLASS*)this, src, size);
		}
		
		Memory decrypt_ECB_PKCS7Padding(const void* src, sl_size size) const
		{
			return BlockCipher_ECB<CLASS, BlockCipherPadding_PKCS7>::decrypt((CLASS*)this, src, size);
		}
		
		Memory encrypt_ECB_PKCS7Padding(const Memory& mem) const
		{
			return BlockCipher_ECB<CLASS, BlockCipherPadding_PKCS7>::encrypt((CLASS*)this, mem.getData(), mem.getSize());
		}
		
		Memory decrypt_ECB_PKCS7Padding(const Memory& mem) const
		{
			return BlockCipher_ECB<CLASS, BlockCipherPadding_PKCS7>::decrypt((CLASS*)this, mem.getData(), mem.getSize());
		}
		
		sl_size encrypt_CBC_PKCS7Padding(const void* iv, const void* src, sl_size size, void* dst) const
		{
			return BlockCipher_CBC<CLASS, BlockCipherPadding_PKCS7>::encrypt((CLASS*)this, iv, src, size, dst);
		}
		
		sl_size decrypt_CBC_PKCS7Padding(const void* iv, const void* src, sl_size size, void* dst) const
		{
			return BlockCipher_CBC<CLASS, BlockCipherPadding_PKCS7>::decrypt((CLASS*)this, iv, src, size, dst);
		}
		
		sl_size encrypt_CBC_PKCS7Padding(const void* src, sl_size size, void* dst) const
		{
			return BlockCipher_CBC<CLASS, BlockCipherPadding_PKCS7>::encrypt((CLASS*)this, src, size, dst);
		}
		
		sl_size decrypt_CBC_PKCS7Padding(const void* src, sl_size size, void* dst) const
		{
			return BlockCipher_CBC<CLASS, BlockCipherPadding_PKCS7>::decrypt((CLASS*)this, src, size, dst);
		}
		
		Memory encrypt_CBC_PKCS7Padding(const void* iv, const void* src, sl_size size) const
		{
			return BlockCipher_CBC<CLASS, BlockCipherPadding_PKCS7>::encrypt((CLASS*)this, iv, src, size);
		}
		
		Memory decrypt_CBC_PKCS7Padding(const void* iv, const void* src, sl_size size) const
		{
			return BlockCipher_CBC<CLASS, BlockCipherPadding_PKCS7>::decrypt((CLASS*)this, iv, src, size);
		}
	
		Memory encrypt_CBC_PKCS7Padding(const void* src, sl_size size) const
		{
			return BlockCipher_CBC<CLASS, BlockCipherPadding_PKCS7>::encrypt((CLASS*)this, src, size);
		}
		
		Memory decrypt_CBC_PKCS7Padding(const void* src, sl_size size) const
		{
			return BlockCipher_CBC<CLASS, BlockCipherPadding_PKCS7>::decrypt((CLASS*)this, src, size);
		}
		
		Memory encrypt_CBC_PKCS7Padding(const Memory& mem) const
		{
			return BlockCipher_CBC<CLASS, BlockCipherPadding_PKCS7>::encrypt((CLASS*)this, mem.getData(), mem.getSize());
		}
		
		Memory decrypt_CBC_PKCS7Padding(const Memory& mem) const
		{
			return BlockCipher_CBC<CLASS, BlockCipherPadding_PKCS7>::decrypt((CLASS*)this, mem.getData(), mem.getSize());
		}
		
		sl_size encrypt_CTR(const void* input, sl_size size, void* output, void* counter, sl_uint32 offset) const
		{
			return BlockCipher_CTR<CLASS>::encrypt((CLASS*)this, input, size, output, counter, offset);
		}
		
		sl_size encrypt_CTR(const void* iv, sl_uint64 counter, sl_uint32 offset, const void* input, sl_size size, void* output) const
		{
			return BlockCipher_CTR<CLASS>::encrypt((CLASS*)this, iv, counter, offset, input, size, output);
		}
		
		sl_size encrypt_CTR(const void* iv, sl_uint64 pos, const void* input, sl_size size, void* output) const
		{
			return BlockCipher_CTR<CLASS>::encrypt((CLASS*)this, iv, pos, input, size, output);
		}
		
	};
	
}

#endif
