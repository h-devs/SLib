/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

/*

 Adapted from the public domain code by D. Bernstein
 
 https://github.com/floodyberry/supercop/blob/master/crypto_stream/chacha20/dolbeau/ppc-altivec/chacha.c

*/
 
#include "slib/crypto/chacha.h"

#include "slib/core/base.h"
#include "slib/core/mio.h"
#include "slib/math/math.h"
#include "slib/crypto/pbkdf.h"

namespace slib
{

#define ROUNDS 20

#define ROTATE(v, c) (((v) << (c)) | ((v) >> (32 - (c))))

#define U8TO32_LITTLE(A, B, C, D) ((((sl_uint32)(sl_uint8)(A))) | (((sl_uint32)(sl_uint8)(B))<<8) | (((sl_uint32)(sl_uint8)(C))<<16) | (((sl_uint32)(sl_uint8)(D))<<24))

#define QUARTERROUND(x, a, b, c, d) \
	x[a] += x[b]; x[d] = ROTATE(x[d]^x[a], 16); \
	x[c] += x[d]; x[b] = ROTATE(x[b]^x[c], 12); \
	x[a] += x[b]; x[d] = ROTATE(x[d]^x[a], 8); \
	x[c] += x[d]; x[b] = ROTATE(x[b]^x[c], 7);

#define MAKE_STATE(state, constants, key, nonce) \
	state[0] = constants[0]; \
	state[1] = constants[1]; \
	state[2] = constants[2]; \
	state[3] = constants[3]; \
	state[4] = key[0]; \
	state[5] = key[1]; \
	state[6] = key[2]; \
	state[7] = key[3]; \
	state[8] = key[4]; \
	state[9] = key[5]; \
	state[10] = key[6]; \
	state[11] = key[7]; \
	state[12] = nonce0; \
	state[13] = nonce1; \
	state[14] = nonce2; \
	state[15] = nonce3;

#define INNER_BLOCK(x) \
	{ \
		for (sl_uint32 i = ROUNDS; i > 0; i -= 2) { \
			QUARTERROUND(x, 0, 4, 8, 12) \
			QUARTERROUND(x, 1, 5, 9, 13) \
			QUARTERROUND(x, 2, 6, 10, 14) \
			QUARTERROUND(x, 3, 7, 11, 15) \
			QUARTERROUND(x, 0, 5, 10, 15) \
			QUARTERROUND(x, 1, 6, 11, 12) \
			QUARTERROUND(x, 2, 7, 8, 13) \
			QUARTERROUND(x, 3, 4, 9, 14) \
		} \
	}

#define SERIALIZE_ELEMENT(e, t, ...) \
	{ \
		sl_uint32 v = e; \
		*(t++) = (sl_uint8)(v) __VA_ARGS__; \
		*(t++) = (sl_uint8)((v) >> 8) __VA_ARGS__; \
		*(t++) = (sl_uint8)((v) >> 16) __VA_ARGS__; \
		*(t++) = (sl_uint8)((v) >> 24) __VA_ARGS__; \
	}

#define SERIALIZE_OUTPUT(output, state, constants, key, nonce, ...) \
	{ \
		SERIALIZE_ELEMENT(state[0] + constants[0], output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[1] + constants[1], output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[2] + constants[2], output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[3] + constants[3], output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[4] + key[0], output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[5] + key[1], output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[6] + key[2], output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[7] + key[3], output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[8] + key[4], output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[9] + key[5], output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[10] + key[6], output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[11] + key[7], output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[12] + nonce0, output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[13] + nonce1, output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[14] + nonce2, output, ##__VA_ARGS__) \
		SERIALIZE_ELEMENT(state[15] + nonce3, output, ##__VA_ARGS__) \
	}

	namespace priv
	{
		namespace chacha
		{

			static const sl_uint32 g_constants[8] = {
				U8TO32_LITTLE('e', 'x', 'p', 'a'),
				U8TO32_LITTLE('n', 'd', ' ', '3'),
				U8TO32_LITTLE('2', '-', 'b', 'y'),
				U8TO32_LITTLE('t', 'e', ' ', 'k'),
				// 16 Bytes Key
				U8TO32_LITTLE('e', 'x', 'p', 'a'),
				U8TO32_LITTLE('n', 'd', ' ', '1'),
				U8TO32_LITTLE('6', '-', 'b', 'y'),
				U8TO32_LITTLE('t', 'e', ' ', 'k')
			};

			static void Salsa20WordToByte(
				sl_uint8* output, // 64
				const sl_uint32* key, // 8
				sl_uint32 indexConstants,
				sl_uint32 nonce0, sl_uint32 nonce1, sl_uint32 nonce2, sl_uint32 nonce3) noexcept
			{
				const sl_uint32* constants = g_constants + (indexConstants << 2);
				sl_uint32 state[16];
				MAKE_STATE(state, constants, key, nonce)
				INNER_BLOCK(state)
				SERIALIZE_OUTPUT(output, state, constants, key, nonce)
			}

			static void Salsa20WordToByte(
				const sl_uint8* data, // 64
				sl_uint8* output, // 64
				const sl_uint32* key, // 8
				sl_uint32 indexConstants,
				sl_uint32 nonce0, sl_uint32 nonce1, sl_uint32 nonce2, sl_uint32 nonce3) noexcept
			{
				const sl_uint32* constants = g_constants + (indexConstants << 2);
				sl_uint32 state[16];
				MAKE_STATE(state, constants, key, nonce)
				INNER_BLOCK(state)
				SERIALIZE_OUTPUT(output, state, constants, key, nonce, ^ *(data++))
			}

		}
	}

	using namespace priv::chacha;


	ChaCha20_Core::ChaCha20_Core(): m_indexConstants(0)
	{
	}

	void ChaCha20_Core::setKey(const void* _key) noexcept
	{
		m_indexConstants = 0;
		const sl_uint8* m = (const sl_uint8*)_key;
		for (sl_uint32 i = 0; i < 8; i++) {
			key[i] = U8TO32_LITTLE(*m, m[1], m[2], m[3]);
			m += 4;
		}
	}

	void ChaCha20_Core::getKey(void* _key) noexcept
	{
		sl_uint8* m = (sl_uint8*)_key;
		for (sl_uint32 i = 0; i < 8; i++) {
			MIO::writeUint32LE(m, key[i]);
			m += 4;
		}
	}

	void ChaCha20_Core::setKey16(const void* _key) noexcept
	{
		m_indexConstants = 1;
		const sl_uint8* m = (const sl_uint8*)_key;
		for (sl_uint32 i = 0; i < 4; i++) {
			key[i + 4] = key[i] = U8TO32_LITTLE(*m, m[1], m[2], m[3]);
			m += 4;
		}
	}

	void ChaCha20_Core::generateBlock(sl_uint32 nonce0, sl_uint32 nonce1, sl_uint32 nonce2, sl_uint32 nonce3, void* output) const noexcept
	{
		Salsa20WordToByte((sl_uint8*)output, key, m_indexConstants, nonce0, nonce1, nonce2, nonce3);
	}

	void ChaCha20_Core::encryptBlock(sl_uint32 nonce0, sl_uint32 nonce1, sl_uint32 nonce2, sl_uint32 nonce3, const void* input, void* output) const noexcept
	{
		Salsa20WordToByte((const sl_uint8*)input, (sl_uint8*)output, key, m_indexConstants, nonce0, nonce1, nonce2, nonce3);
	}


	void ChaCha20_IO::encrypt(sl_uint64 offset, const void* _src, void* _dst, sl_size size) const noexcept
	{
		if (!size) {
			return;
		}
		sl_uint8* src = (sl_uint8*)_src;
		sl_uint8* dst = (sl_uint8*)_dst;
		sl_uint64 block = offset >> 6;
		sl_uint64 offsetEnd = (sl_uint64)(offset + size);
		sl_uint64 blockEnd = offsetEnd >> 6;
		sl_bool flagFirstBlock = sl_true;
		char h[64];
		for (; block <= blockEnd; block++) {
			generateBlock(iv[0], iv[1], iv[2] ^ ((sl_uint32)(block >> 32)), iv[3] ^ ((sl_uint32)block), h);
			sl_uint32 n;
			if (block == blockEnd) {
				n = (sl_uint32)(offsetEnd & 63);
			}
			else {
				n = 64;
			}
			if (flagFirstBlock) {
				flagFirstBlock = sl_false;
				sl_uint32 s = offset & 63;
				n -= s;
				for (sl_uint32 i = 0; i < n; i++) {
					dst[i] = src[i] ^ h[s + i];
				}
			}
			else {
				for (sl_uint32 i = 0; i < n; i++) {
					dst[i] = src[i] ^ h[i];
				}
			}
			src += n;
			dst += n;
		}
	}


	void ChaCha20_IO::getIV(void* bytes16) const noexcept
	{
		sl_uint8* bytes = (sl_uint8*)bytes16;
		MIO::writeUint32LE(bytes, iv[0]);
		MIO::writeUint32LE(bytes + 4, iv[1]);
		MIO::writeUint32LE(bytes + 8, iv[2]);
		MIO::writeUint32LE(bytes + 12, iv[3]);
	}

	void ChaCha20_IO::setIV(const void* bytes16) noexcept
	{
		const sl_uint8* bytes = (const sl_uint8*)bytes16;
		iv[0] = MIO::readUint32LE(bytes);
		iv[1] = MIO::readUint32LE(bytes + 4);
		iv[2] = MIO::readUint32LE(bytes + 8);
		iv[3] = MIO::readUint32LE(bytes + 12);
	}


	ChaCha20::ChaCha20() noexcept : m_pos(0)
	{
	}

	ChaCha20::~ChaCha20()
	{
	}

	void ChaCha20::start(sl_uint32 nonce0, sl_uint32 nonce1, sl_uint32 nonce2, sl_uint32 nonce3) noexcept
	{
		m_nonce[0] = nonce0;
		m_nonce[1] = nonce1;
		m_nonce[2] = nonce2;
		m_nonce[3] = nonce3;
		m_pos = 0;
	}

	void ChaCha20::start(const void* _iv, sl_uint32 counter) noexcept
	{
		const sl_uint8* iv = (const sl_uint8*)_iv;
		m_nonce[0] = counter;
		m_nonce[1] = U8TO32_LITTLE(iv[0], iv[1], iv[2], iv[3]);
		m_nonce[2] = U8TO32_LITTLE(iv[4], iv[5], iv[6], iv[7]);
		m_nonce[3] = U8TO32_LITTLE(iv[8], iv[9], iv[10], iv[11]);
		m_pos = 0;
	}

	void ChaCha20::encrypt(const void* _src, void* _dst, sl_size len) noexcept
	{
		if (!len) {
			return;
		}
		const sl_uint8* src = (const sl_uint8*)_src;
		sl_uint8* dst = (sl_uint8*)_dst;
		sl_uint8* y = m_output;
		sl_uint32 pos = m_pos;
		for (sl_size k = 0; k < len; k++) {
			if (!pos) {
				Salsa20WordToByte(y, key, m_indexConstants, m_nonce[0], m_nonce[1], m_nonce[2], m_nonce[3]);
				m_nonce[0]++;
			}
			dst[k] = src[k] ^ y[pos];
			pos = (pos + 1) & 0x3F;
		}
		m_pos = pos;
	}


	ChaCha20_Poly1305::ChaCha20_Poly1305() noexcept
	{
	}

	ChaCha20_Poly1305::~ChaCha20_Poly1305()
	{
	}

	void ChaCha20_Poly1305::setKey(const void* key) noexcept
	{
		m_cipher.setKey(key);
	}

	void ChaCha20_Poly1305::start(sl_uint32 senderId, const void* _iv) noexcept
	{
		const sl_uint8* iv = (const sl_uint8*)_iv;
		sl_uint32 n0 = U8TO32_LITTLE(iv[0], iv[1], iv[2], iv[3]);
		sl_uint32 n1 = U8TO32_LITTLE(iv[4], iv[5], iv[6], iv[7]);
		m_cipher.start(1, senderId, n0, n1);
		sl_uint8 block0[64];
		m_cipher.generateBlock(0, senderId, n0, n1, block0);
		m_auth.start(block0); // use first 32 bytes
		m_lenAAD = 0;
		m_lenInput = 0;
	}

	void ChaCha20_Poly1305::start(const void* _iv) noexcept
	{
		const sl_uint8* iv = (const sl_uint8*)_iv;
		start(U8TO32_LITTLE(iv[0], iv[1], iv[2], iv[3]), iv + 4);
	}

	void ChaCha20_Poly1305::putAAD(const void* data, sl_size len) noexcept
	{
		m_auth.update(data, len);
		m_lenAAD += len;
	}

	void ChaCha20_Poly1305::finishAAD() noexcept
	{
		sl_uint32 n = (sl_uint32)(m_lenAAD & 15);
		if (n) {
			static const sl_uint8 zeros[16] = {0};
			m_auth.update(zeros, 16 - n);
		}
	}

	void ChaCha20_Poly1305::encrypt(const void* src, void* dst, sl_size len) noexcept
	{
		if (!len) {
			return;
		}
		m_cipher.encrypt(src, dst, len);
		m_auth.update(dst, len);
		m_lenInput += len;
	}

	void ChaCha20_Poly1305::decrypt(const void* src, void* dst, sl_size len) noexcept
	{
		if (!len) {
			return;
		}
		m_auth.update(src, len);
		m_cipher.encrypt(src, dst, len);
		m_lenInput += len;
	}

	void ChaCha20_Poly1305::check(const void* src, sl_size len) noexcept
	{
		m_auth.update(src, len);
		m_lenInput += len;
	}

	void ChaCha20_Poly1305::finish(void* outputTag) noexcept
	{
		sl_uint32 n = (sl_uint32)(m_lenInput & 15);
		if (n) {
			static const sl_uint8 zeros[16] = {0};
			m_auth.update(zeros, 16 - n);
		}
		sl_uint8 len[16];
		MIO::writeUint64LE(len, m_lenAAD);
		MIO::writeUint64LE(len + 8, m_lenInput);
		m_auth.update(len, 16);
		m_auth.finish(outputTag);
	}

	sl_bool ChaCha20_Poly1305::finishAndCheckTag(const void* _tag) noexcept
	{
		const sl_uint8* tag = (const sl_uint8*)_tag;
		sl_uint8 outputTag[16];
		finish(outputTag);
		sl_uint32 n = 0;
		for (sl_uint32 i = 0; i < 16; i++) {
			n |= (outputTag[i] ^ tag[i]);
		}
		return n == 0;
	}

	void ChaCha20_Poly1305::encrypt(sl_uint32 senderId, const void* iv, const void* AAD, sl_size lenAAD, const void* src, void* dst, sl_size len, void* outputTag) noexcept
	{
		start(senderId, iv);
		if (lenAAD) {
			putAAD(AAD, lenAAD);
			finishAAD();
		}
		if (len) {
			encrypt(src, dst, len);
		}
		finish(outputTag);
	}

	Memory ChaCha20_Poly1305::encrypt(const MemoryView& content) noexcept
	{
		if (!(content.size)) {
			return sl_null;
		}
		Memory ret = Memory::create(content.size + 28);
		if (ret.isNotNull()) {
			sl_uint8* buf = (sl_uint8*)(ret.getData());
			sl_uint8* iv = buf;
			Math::randomMemory(iv, 12);
			sl_uint8* data = buf + 12;
			sl_uint8* tag = data + content.size;
			encrypt(MIO::readUint32BE(iv), iv + 4, sl_null, 0, content.data, data, content.size, tag);
			return ret;
		}
		return sl_null;
	}

	sl_bool ChaCha20_Poly1305::decrypt(sl_uint32 senderId, const void* iv, const void* AAD, sl_size lenAAD, const void* src, void* dst, sl_size len, const void* tag) noexcept
	{
		start(senderId, iv);
		if (lenAAD) {
			putAAD(AAD, lenAAD);
			finishAAD();
		}
		if (len) {
			decrypt(src, dst, len);
		}
		return finishAndCheckTag(tag);
	}

	Memory ChaCha20_Poly1305::decrypt(const MemoryView& encryptedContent) noexcept
	{
		if (encryptedContent.size <= 28) {
			return sl_null;
		}
		sl_size n = encryptedContent.size - 28;
		Memory ret = Memory::create(n);
		if (ret.isNotNull()) {
			sl_uint8* buf = (sl_uint8*)(encryptedContent.data);
			sl_uint8* iv = buf;
			sl_uint8* data = buf + 12;
			sl_uint8* tag = data + n;
			if (decrypt(MIO::readUint32BE(iv), iv + 4, sl_null, 0, data, ret.getData(), n, tag)) {
				return ret;
			}
		}
		return sl_null;
	}

	sl_bool ChaCha20_Poly1305::check(sl_uint32 senderId, const void* iv, const void* AAD, sl_size lenAAD, const void* src, sl_size len, const void* tag) noexcept
	{
		start(senderId, iv);
		if (lenAAD) {
			putAAD(AAD, lenAAD);
			finishAAD();
		}
		if (len) {
			check(src, len);
		}
		return finishAndCheckTag(tag);
	}


/*
	ChaCha20_FileEncryptor Header Format

	Check Pattern = PBKDF(SHA256(password))
	Main Encryption Key = PBKDF(password) ^ Xor Pattern

	Total Size: 128 Bytes
	_________________________________________________________________
	| Offset |  Size  |                 Content                     |
	|   0    |   12   |   PBKDF Salt for Check-Pattern              |
	|   12   |   4    |   PBKDF Iteration for Check-Pattern         |
	|   16   |   32   |   Check Pattern                             |
	|   48   |   12   |   PBKDF Salt for Main Encryption Key        |
	|   60   |   4    |   PBKDF Iteration for Main Encryption Key   |
	|   64   |   16   |   IV                                        |
	|   80   |   32   |   Xor Pattern                               |
	|   112  |   16   |   Reserved                                  |
	-----------------------------------------------------------------

*/

#define CHECK_LEN_HASH_ITERATION 1001
#define FILE_ENCRYPT_ITERATION_CREATE_DEFAULT 13
#define FILE_ENCRYPT_ITERATION_OPEN_DEFAULT 20

	namespace priv
	{
		namespace chacha_file_enc
		{

			static sl_uint32 GetMainIteration(sl_uint32 code, sl_uint32 len) noexcept
			{
				sl_uint32 n = 1 << (len - 1);
				return n | (code & (n - 1));
			}

			static sl_uint32 GenerateCheckIteration(sl_uint32& code, sl_uint32 len) noexcept
			{
				code = (code & 0xFFFFFFF) | ((len - 11) << 28);
				return GetMainIteration(code, len);
			}

			static sl_uint32 GetCheckIteration(sl_uint32 code, sl_uint32& len) noexcept
			{
				len = (code >> 28) + 11;
				return GetMainIteration(code, len);
			}

			static sl_uint32 GetCheckIteration(sl_uint32 code) noexcept
			{
				sl_uint32 len;
				return GetCheckIteration(code, len);
			}

			static sl_bool CheckPassword(const void* _header, const void* password, sl_size lenPassword, sl_uint32 iterationBitCountLimit, sl_uint32& nIterationBitCount) noexcept
			{
				sl_uint8* header = (sl_uint8*)_header;
				sl_uint8 h[32];
				PBKDF2_HMAC_SHA256::generateKey(header + 48, 12, header, 12, CHECK_LEN_HASH_ITERATION, h, 4);
				sl_uint32 code = MIO::readUint32LE(header + 12) ^ MIO::readUint32LE(h);
				sl_uint32 iter = GetCheckIteration(code, nIterationBitCount);
				if (nIterationBitCount > iterationBitCountLimit) {
					return sl_false;
				}
				SHA256::hash(password, lenPassword, h);
				sl_uint8 c[32];
				PBKDF2_HMAC_SHA256::generateKey(h, 32, header, 12, iter, c, 32);
				return Base::equalsMemory(c, header + 16, 32);
			}

			static sl_bool CheckPassword(const void* header, const void* password, sl_size lenPassword, sl_uint32 iterationBitCountLimit) noexcept
			{
				sl_uint32 nIterationBitCount;
				return CheckPassword(header, password, lenPassword, iterationBitCountLimit, nIterationBitCount);
			}

			// key: 32 Bytes
			static sl_bool GetEncryptionKey(sl_uint8* key, const void* _header, const void* password, sl_size lenPassword, sl_uint32 iterationBitCountLimit, sl_uint32& iteration) noexcept
			{
				sl_uint8* header = (sl_uint8*)_header;
				sl_uint32 nIterationBitCount;
				if (!(CheckPassword(header, password, lenPassword, iterationBitCountLimit, nIterationBitCount))) {
					return sl_false;
				}
				sl_uint32 code = MIO::readUint32LE(header + 60);
				iteration = GetMainIteration(code, nIterationBitCount);
				PBKDF2_HMAC_SHA256::generateKey(password, lenPassword, header + 48, 12, iteration, key, 32);
				for (sl_uint32 i = 0; i < 32; i++) {
					key[i] ^= header[80 + i];
				}
				return sl_true;
			}

			// key: 32 Bytes
			static sl_bool GetEncryptionKey(sl_uint8* key, const void* header, const void* password, sl_size lenPassword, sl_uint32 iterationBitCountLimit) noexcept
			{
				sl_uint32 iteration;
				return GetEncryptionKey(key, header, password, lenPassword, iterationBitCountLimit, iteration);
			}

		}
	}

	using namespace priv::chacha_file_enc;

	void ChaCha20_FileEncryptor::create(void* _header, const void* password, sl_size lenPassword, sl_uint32 iterationBitCount) noexcept
	{
		sl_uint8* header = (sl_uint8*)_header;
		Math::randomMemory(header, HeaderSize);

		if (iterationBitCount < 11) {
			iterationBitCount = 11;
		}
		if (iterationBitCount > 26) {
			iterationBitCount = 26;
		}
		{
			sl_uint8 h[32];
			sl_uint32 code = MIO::readUint32LE(header + 12);
			sl_uint32 iter = GenerateCheckIteration(code, iterationBitCount);
			PBKDF2_HMAC_SHA256::generateKey(header + 48, 12, header, 12, CHECK_LEN_HASH_ITERATION, h, 4);
			MIO::writeUint32LE(header + 12, code ^ MIO::readUint32LE(h));
			SHA256::hash(password, lenPassword, h);
			PBKDF2_HMAC_SHA256::generateKey(h, 32, header, 12, iter, header + 16, 32);
		}
		{
			sl_uint32 code = MIO::readUint32LE(header + 60);
			sl_uint32 iter = GetMainIteration(code, iterationBitCount);
			sl_uint8 key[32];
			PBKDF2_HMAC_SHA256::generateKey(password, lenPassword, header + 48, 12, iter, key, 32);
			for (sl_uint32 i = 0; i < 32; i++) {
				key[i] ^= header[80 + i];
			}
			setKey(key);
			setIV(header + 64);
		}
	}

	void ChaCha20_FileEncryptor::create(void* header, const void* password, sl_size lenPassword) noexcept
	{
		create(header, password, lenPassword, FILE_ENCRYPT_ITERATION_CREATE_DEFAULT);
	}

	sl_bool ChaCha20_FileEncryptor::open(const void* _header, const void* password, sl_size lenPassword, sl_uint32 iterationBitCountLimit) noexcept
	{
		sl_uint8* header = (sl_uint8*)_header;
		sl_uint8 key[32];
		if (GetEncryptionKey(key, header, password, lenPassword, iterationBitCountLimit)) {
			setKey(key);
			setIV(header + 64);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool ChaCha20_FileEncryptor::open(const void* header, const void* password, sl_size lenPassword) noexcept
	{
		return open(header, password, lenPassword, FILE_ENCRYPT_ITERATION_OPEN_DEFAULT);
	}

	sl_bool ChaCha20_FileEncryptor::checkPassword(const void* header, const void* password, sl_size lenPassword, sl_uint32 iterationBitCountLimit) noexcept
	{
		return CheckPassword(header, password, lenPassword, iterationBitCountLimit);
	}

	sl_bool ChaCha20_FileEncryptor::checkPassword(const void* header, const void* password, sl_size lenPassword) noexcept
	{
		return checkPassword(header, password, lenPassword, FILE_ENCRYPT_ITERATION_OPEN_DEFAULT);
	}

	sl_bool ChaCha20_FileEncryptor::changePassword(void* _header, const void* oldPassword, sl_size lenOldPassword, const void* newPassword, sl_size lenNewPassword, sl_uint32 iterationBitCountLimit) noexcept
	{
		sl_uint8* header = (sl_uint8*)_header;
		sl_uint8 key[32];
		sl_uint32 iteration = 0;
		if (GetEncryptionKey(key, header, oldPassword, lenOldPassword, iterationBitCountLimit, iteration)) {
			sl_uint8 t[32];
			PBKDF2_HMAC_SHA256::generateKey(newPassword, lenNewPassword, header + 48, 12, iteration, t, 32);
			for (sl_uint32 i = 0; i < 32; i++) {
				header[80 + i] = t[i] ^ key[i];
			}
			PBKDF2_HMAC_SHA256::generateKey(header + 48, 12, header, 12, CHECK_LEN_HASH_ITERATION, t, 4);
			sl_uint32 code = MIO::readUint32LE(header + 12) ^ MIO::readUint32LE(t);
			sl_uint32 checkIteration = GetCheckIteration(code);
			SHA256::hash(newPassword, lenNewPassword, t);
			PBKDF2_HMAC_SHA256::generateKey(t, 32, header, 12, checkIteration, header + 16, 32);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool ChaCha20_FileEncryptor::changePassword(void* header, const void* oldPassword, sl_size lenOldPassword, const void* newPassword, sl_size lenNewPassword) noexcept
	{
		return changePassword(header, oldPassword, lenOldPassword, newPassword, lenNewPassword, FILE_ENCRYPT_ITERATION_OPEN_DEFAULT);
	}

}
