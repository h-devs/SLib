/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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
			
			static void salsa20_wordtobyte(
				sl_uint8* output, // 64
				const sl_uint32* constants, // 4
				const sl_uint32* key, // 8
				sl_uint32 nonce0, sl_uint32 nonce1, sl_uint32 nonce2, sl_uint32 nonce3)
			{
				sl_uint32 state[16];
				MAKE_STATE(state, constants, key, nonce)
				INNER_BLOCK(state)
				SERIALIZE_OUTPUT(output, state, constants, key, nonce)
			}
			
			static void salsa20_wordtobyte(
				const sl_uint8* data, // 64
				sl_uint8* output, // 64
				const sl_uint32* constants, // 4
				const sl_uint32* key, // 8
				sl_uint32 nonce0, sl_uint32 nonce1, sl_uint32 nonce2, sl_uint32 nonce3)
			{
				sl_uint32 state[16];
				MAKE_STATE(state, constants, key, nonce)
				INNER_BLOCK(state)
				SERIALIZE_OUTPUT(output, state, constants, key, nonce, ^ *(data++))
			}
			
		}
		
	}
	
	ChaCha20_Core::ChaCha20_Core()
	{
	}
	
	ChaCha20_Core::~ChaCha20_Core()
	{
	}
	
	void ChaCha20_Core::setKey(const void* _key)
	{
		static sl_uint32 constants[4] = {
			U8TO32_LITTLE('e', 'x', 'p', 'a'),
			U8TO32_LITTLE('n', 'd', ' ', '3'),
			U8TO32_LITTLE('2', '-', 'b', 'y'),
			U8TO32_LITTLE('t', 'e', ' ', 'k')
		};
		m_constants = constants;
		const sl_uint8* key = (const sl_uint8*)_key;
		sl_uint32* k = m_key;
		for (sl_uint32 i = 0; i < 8; i++) {
			k[i] = U8TO32_LITTLE(*key, key[1], key[2], key[3]);
			key += 4;
		}
	}

	void ChaCha20_Core::setKey16(const void* _key)
	{
		static sl_uint32 constants[4] = {
			U8TO32_LITTLE('e', 'x', 'p', 'a'),
			U8TO32_LITTLE('n', 'd', ' ', '1'),
			U8TO32_LITTLE('6', '-', 'b', 'y'),
			U8TO32_LITTLE('t', 'e', ' ', 'k')
		};
		m_constants = constants;
		const sl_uint8* key = (const sl_uint8*)_key;
		sl_uint32* k = m_key;
		for (sl_uint32 i = 0; i < 4; i++) {
			k[i + 4] = k[i] = U8TO32_LITTLE(*key, key[1], key[2], key[3]);
			key += 4;
		}
	}

	void ChaCha20_Core::setKey48(const void* _key)
	{
		const sl_uint8* key = (const sl_uint8*)_key;
		{
			sl_uint32* k = m_arrConstants;
			for (sl_uint32 i = 0; i < 4; i++) {
				k[i] = U8TO32_LITTLE(*key, key[1], key[2], key[3]);
				key += 4;
			}
			m_constants = m_arrConstants;
		}
		{
			sl_uint32* k = m_key;
			for (sl_uint32 i = 0; i < 8; i++) {
				k[i] = U8TO32_LITTLE(*key, key[1], key[2], key[3]);
				key += 4;
			}
		}
	}
	
	void ChaCha20_Core::generateBlock(sl_uint32 nonce0, sl_uint32 nonce1, sl_uint32 nonce2, sl_uint32 nonce3, void* output) const
	{
		priv::chacha::salsa20_wordtobyte((sl_uint8*)output, m_constants, m_key, nonce0, nonce1, nonce2, nonce3);
	}
	
	void ChaCha20_Core::encryptBlock(sl_uint32 nonce0, sl_uint32 nonce1, sl_uint32 nonce2, sl_uint32 nonce3, const void* input, void* output) const
	{
		priv::chacha::salsa20_wordtobyte((const sl_uint8*)input, (sl_uint8*)output, m_constants, m_key, nonce0, nonce1, nonce2, nonce3);
	}
	
	void ChaCha20_Core::decryptBlock(sl_uint32 nonce0, sl_uint32 nonce1, sl_uint32 nonce2, sl_uint32 nonce3, const void* input, void* output) const
	{
		priv::chacha::salsa20_wordtobyte((const sl_uint8*)input, (sl_uint8*)output, m_constants, m_key, nonce0, nonce1, nonce2, nonce3);
	}
	
	
	ChaCha20::ChaCha20(): m_pos(0), m_flagCounter32(sl_false)
	{
	}
	
	ChaCha20::~ChaCha20()
	{
	}
	
	void ChaCha20::start(sl_uint32 nonce0, sl_uint32 nonce1, sl_uint32 nonce2, sl_uint32 nonce3)
	{
		m_nonce[0] = nonce0;
		m_nonce[1] = nonce1;
		m_nonce[2] = nonce2;
		m_nonce[3] = nonce3;
		m_pos = 0;
		m_flagCounter32 = sl_false;
	}
	
	void ChaCha20::start(const void* _iv, sl_uint64 counter)
	{
		const sl_uint8* iv = (const sl_uint8*)_iv;
		m_nonce[0] = (sl_uint32)counter;
		m_nonce[1] = (sl_uint32)(counter >> 32);
		m_nonce[2] = U8TO32_LITTLE(iv[0], iv[1], iv[2], iv[3]);
		m_nonce[3] = U8TO32_LITTLE(iv[4], iv[5], iv[6], iv[7]);
		m_pos = 0;
		m_flagCounter32 = sl_false;
	}
	
	void ChaCha20::start32(sl_uint32 nonce0, sl_uint32 nonce1, sl_uint32 nonce2, sl_uint32 nonce3)
	{
		m_nonce[0] = nonce0;
		m_nonce[1] = nonce1;
		m_nonce[2] = nonce2;
		m_nonce[3] = nonce3;
		m_pos = 0;
		m_flagCounter32 = sl_true;
	}
	
	void ChaCha20::start32(const void* _iv, sl_uint32 counter)
	{
		const sl_uint8* iv = (const sl_uint8*)_iv;
		m_nonce[0] = counter;
		m_nonce[1] = U8TO32_LITTLE(iv[0], iv[1], iv[2], iv[3]);
		m_nonce[2] = U8TO32_LITTLE(iv[4], iv[5], iv[6], iv[7]);
		m_nonce[3] = U8TO32_LITTLE(iv[8], iv[9], iv[10], iv[11]);
		m_pos = 0;
		m_flagCounter32 = sl_true;
	}
	
	void ChaCha20::encrypt(const void* _src, void* _dst, sl_size len)
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
				priv::chacha::salsa20_wordtobyte(y, m_constants, m_key, m_nonce[0], m_nonce[1], m_nonce[2], m_nonce[3]);
				m_nonce[0]++;
				if (!m_flagCounter32) {
					if (!m_nonce[0]) {
						m_nonce[1]++;
					}
				}
			}
			dst[k] = src[k] ^ y[pos];
			pos = (pos + 1) & 0x3F;
		}
		m_pos = pos;
	}
	
	void ChaCha20::decrypt(const void* src, void* dst, sl_size len)
	{
		encrypt(src, dst, len);
	}
	
	sl_bool ChaCha20::is32BitCounter()
	{
		return m_flagCounter32;
	}
	
	void ChaCha20::set32BitCounter(sl_bool flag)
	{
		m_flagCounter32 = flag;
	}
	
	
	ChaCha20_Poly1305::ChaCha20_Poly1305()
	{
	}
	
	ChaCha20_Poly1305::~ChaCha20_Poly1305()
	{
	}
	
	void ChaCha20_Poly1305::setKey(const void* key)
	{
		m_cipher.setKey(key);
	}
	
	void ChaCha20_Poly1305::start(sl_uint32 senderId, const void* _iv)
	{
		const sl_uint8* iv = (const sl_uint8*)_iv;
		sl_uint32 n0 = U8TO32_LITTLE(iv[0], iv[1], iv[2], iv[3]);
		sl_uint32 n1 = U8TO32_LITTLE(iv[4], iv[5], iv[6], iv[7]);
		m_cipher.start32(1, senderId, n0, n1);
		sl_uint8 block0[64];
		m_cipher.generateBlock(0, senderId, n0, n1, block0);
		m_auth.start(block0); // use first 32 bytes
		m_lenAAD = 0;
		m_lenInput = 0;
	}
	
	void ChaCha20_Poly1305::putAAD(const void* data, sl_size len)
	{
		m_auth.update(data, len);
		m_lenAAD += len;
	}
	
	void ChaCha20_Poly1305::finishAAD()
	{
		sl_uint32 n = (sl_uint32)(m_lenAAD & 15);
		if (n) {
			static const sl_uint8 zeros[16] = {0};
			m_auth.update(zeros, 16 - n);
		}
	}

	void ChaCha20_Poly1305::encrypt(const void* src, void* dst, sl_size len)
	{
		if (!len) {
			return;
		}
		m_cipher.encrypt(src, dst, len);
		m_auth.update(dst, len);
		m_lenInput += len;
	}
	
	void ChaCha20_Poly1305::decrypt(const void* src, void* dst, sl_size len)
	{
		if (!len) {
			return;
		}
		m_auth.update(src, len);
		m_cipher.encrypt(src, dst, len);
		m_lenInput += len;
	}
	
	void ChaCha20_Poly1305::check(const void* src, sl_size len)
	{
		m_auth.update(src, len);
		m_lenInput += len;
	}
	
	void ChaCha20_Poly1305::finish(void* outputTag)
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
	
	sl_bool ChaCha20_Poly1305::finishAndCheckTag(const void* _tag)
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
	
	void ChaCha20_Poly1305::encrypt(sl_uint32 senderId, const void* iv, const void* AAD, sl_size lenAAD, const void* src, void* dst, sl_size len, void* outputTag)
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
	
	sl_bool ChaCha20_Poly1305::decrypt(sl_uint32 senderId, const void* iv, const void* AAD, sl_size lenAAD, const void* src, void* dst, sl_size len, const void* tag)
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

	sl_bool ChaCha20_Poly1305::check(sl_uint32 senderId, const void* iv, const void* AAD, sl_size lenAAD, const void* src, sl_size len, const void* tag)
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
	
}
