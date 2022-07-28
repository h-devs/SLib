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

#include "slib/crypto/rsa.h"
#include "slib/crypto/dh.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(RSAPublicKey)
	
	RSAPublicKey::RSAPublicKey()
	{
	}

	sl_bool RSAPublicKey::isDefined() const noexcept
	{
		return N.isNotNull() && E.isNotNull();
	}

	sl_uint32 RSAPublicKey::getLength() const noexcept
	{
		return (sl_uint32)(N.getMostSignificantBytes());
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(RSAPrivateKey)
	
	RSAPrivateKey::RSAPrivateKey(): flagUseOnlyD(sl_false)
	{
	}

	sl_bool RSAPrivateKey::isDefined() const noexcept
	{
		return RSAPublicKey::isDefined() && D.isNotNull();
	}

	void RSAPrivateKey::generate(sl_uint32 nBits) noexcept
	{
		sl_uint32 h = nBits >> 1;
		nBits = h << 1;
		for (;;) {
			P = BigInt::generatePrime(h);
			Q = BigInt::generatePrime(h);
			if (generateFromPrimes(nBits)) {
				return;
			}
		}
	}
	
	sl_bool RSAPrivateKey::generateFromPrimes(sl_uint32 nBits) noexcept
	{
		sl_uint32 h = nBits >> 1;
		if (h > 100) {
			if (BigInt::shiftRight((P-Q).abs(), h - 100).isZero()) {
				return sl_false;
			}
		} else {
			if (P == Q) {
				return sl_false;
			}
		}
		N = P * Q;
		BigInt P1 = P - 1;
		BigInt Q1 = Q - 1;
		BigInt L = BigInt::lcm(P1, Q1);
		if (E.isZero()) {
			if (nBits > 20) {
				E = 65537;
			} else if (nBits > 8) {
				E = 17;
			} else {
				E = 3;
			}
		}
		if (E < 3) {
			return sl_false;
		}
		if (E >= L) {
			return sl_false;
		}
		if (BigInt::gcd(E, L) != 1) {
			return sl_false;
		}
		D = BigInt::inverseMod(E, L);
		DP = BigInt::mod(D, P1);
		DQ = BigInt::mod(D, Q1);
		IQ = BigInt::inverseMod(Q, P);
		
		// Test
		BigInt a = 3;
		BigInt b = BigInt::pow_montgomery(a, E, N);
		BigInt TP = BigInt::pow_montgomery(b, DP, P);
		BigInt TQ = BigInt::pow_montgomery(b, DQ, Q);
		BigInt c = BigInt::mod_NonNegativeRemainder((TP - TQ) * IQ, P);
		c = TQ + c * Q;
		return c == 3;
	}


	BigInt RSA::executePublic(const RSAPublicKey& key, const BigInt& input)
	{
		return BigInt::pow_montgomery(input, key.E, key.N);
	}

	BigInt RSA::executePrivate(const RSAPrivateKey& key, const BigInt& input)
	{
		if (!(key.flagUseOnlyD) && key.P.isNotNull() && key.Q.isNotNull() && key.DP.isNotNull() && key.DQ.isNotNull() && key.IQ.isNotNull()) {
			BigInt TP = BigInt::pow_montgomery(input, key.DP, key.P);
			BigInt TQ = BigInt::pow_montgomery(input, key.DQ, key.Q);
			BigInt T = BigInt::mod_NonNegativeRemainder((TP - TQ) * key.IQ, key.P);
			return TQ + T * key.Q;
		} else {
			return BigInt::pow_montgomery(input, key.D, key.N);
		}
	}

	sl_bool RSA::executePublic(const RSAPublicKey& key, const void* input, sl_size sizeInput, void* output)
	{
		BigInt T = BigInt::fromBytesBE(input, sizeInput);
		if (T >= key.N) {
			return sl_false;
		}
		T = executePublic(key, T);
		if (T.isNotNull()) {
			sl_uint32 n = key.getLength();
			if (T.getMostSignificantBytes() <= n) {
				T.getBytesBE(output, n);
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool RSA::executePrivate(const RSAPrivateKey& key, const void* input, sl_size sizeInput, void* output)
	{
		BigInt T = BigInt::fromBytesBE(input, sizeInput);
		if (T >= key.N) {
			return sl_false;
		}
		T = executePrivate(key, T);
		if (T.isNotNull()) {
			sl_uint32 n = key.getLength();
			if (T.getMostSignificantBytes() <= n) {
				T.getBytesBE(output, n);
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool RSA::execute(const RSAPublicKey* keyPublic, const RSAPrivateKey* keyPrivate, const void* input, sl_size sizeInput, void* output)
	{
		if (keyPublic) {
			return RSA::executePublic(*keyPublic, input, sizeInput, output);
		} else {
			return RSA::executePrivate(*keyPrivate, input, sizeInput, output);
		}
	}

#define RSA_PKCS1_SIGN		1
#define RSA_PKCS1_CRYPT		2

	sl_bool RSA::encrypt_pkcs1_v15(const RSAPublicKey* keyPublic, const RSAPrivateKey* keyPrivate, const void* input, sl_size sizeInput, void* output)
	{
		sl_uint32 len;
		if (keyPublic) {
			len = keyPublic->getLength();
		} else {
			len = keyPrivate->getLength();
		}
		// check (len - 8 < n + 3), 8 bytes is for enough random region
		if (!sizeInput || (sizeInput & 0x80000000) || len < sizeInput + 11) {
			return sl_false;
		}
		char* p = (char*)output;
		
		Base::moveMemory(p + len - sizeInput, input, sizeInput);
		*(p++) = 0;
		sl_uint32 lenPadding = len - 3 - (sl_uint32)sizeInput;
		if (keyPublic) {
			// encrypt mode
			*(p++) = RSA_PKCS1_CRYPT;
			Math::randomMemory(p, lenPadding);
			for (sl_uint32 i = 0; i < lenPadding; i++) {
				if (*p == 0) {
					*p = (char)((Math::randomInt() % 255) + 1);
				}
				p++;
			}
		} else {
			// sign mode
			*(p++) = RSA_PKCS1_SIGN;
			for (sl_uint32 i = 0; i < lenPadding; i++) {
				*(p++) = (char)0xFF;
			}
		}
		*(p++) = 0;
		return execute(keyPublic, keyPrivate, output, len, output);
	}

	Memory RSA::encrypt_pkcs1_v15(const RSAPublicKey* keyPublic, const RSAPrivateKey* keyPrivate, const void* input, sl_size sizeInput)
	{
		sl_uint32 len;
		if (keyPublic) {
			len = keyPublic->getLength();
		} else {
			len = keyPrivate->getLength();
		}
		// check (len - 8 < n + 3), 8 bytes is for enough random region
		if (!sizeInput || (sizeInput & 0x80000000) || len < sizeInput + 11) {
			return sl_null;
		}
		Memory mem = Memory::create(len);
		if (mem.isNull()) {
			return sl_null;
		}
		if (encrypt_pkcs1_v15(keyPublic, keyPrivate, input, sizeInput, mem.getData())) {
			return mem;
		}
		return sl_null;
	}
	
	sl_uint32 RSA::decrypt_pkcs1_v15(const RSAPublicKey* keyPublic, const RSAPrivateKey* keyPrivate, const void* input, sl_size sizeInput, void* _output, sl_bool* pFlagSign)
	{
		sl_uint32 len;
		if (keyPublic) {
			len = keyPublic->getLength();
		} else {
			len = keyPrivate->getLength();
		}
		if (len < 32) {
			return sl_false;
		}
		sl_uint8* output = (sl_uint8*)_output;
		if (!(execute(keyPublic, keyPrivate, input, sizeInput, output))) {
			return 0;
		}
		if (output[0]) {
			return sl_false;
		}
		sl_uint32 type = output[1];
		if (type == RSA_PKCS1_SIGN) {
			if (pFlagSign) {
				*pFlagSign = sl_true;
			}
		} else if (type == RSA_PKCS1_CRYPT) {
			if (pFlagSign) {
				*pFlagSign = sl_false;
			}
		} else {
			return sl_false;
		}
		sl_uint32 pos;
		for (pos = 2; pos + 1 < len; pos++) {
			if (!(output[pos])) {
				break;
			}
		}
		pos++;
		Base::moveMemory(output, output + pos, len - pos);
		return len - pos;
	}

	Memory RSA::decrypt_pkcs1_v15(const RSAPublicKey* keyPublic, const RSAPrivateKey* keyPrivate, const void* input, sl_size sizeInput, sl_bool* pFlagSign)
	{
		sl_uint32 len;
		if (keyPublic) {
			len = keyPublic->getLength();
		} else {
			len = keyPrivate->getLength();
		}
		if (len < 32) {
			return sl_null;
		}
		Memory mem = Memory::create(len);
		if (mem.isNull()) {
			return sl_null;
		}
		sl_uint32 n = decrypt_pkcs1_v15(keyPublic, keyPrivate, input, sizeInput, mem.getData(), pFlagSign);
		if (n) {
			return mem.sub(0, n);
		}
		return sl_null;
	}
	
	sl_bool RSA::encryptPublic_pkcs1_v15(const RSAPublicKey& key, const void* input, sl_size sizeInput, void* output)
	{
		return encrypt_pkcs1_v15(&key, sl_null, input, sizeInput, output);
	}

	Memory RSA::encryptPublic_pkcs1_v15(const RSAPublicKey& key, const void* input, sl_size sizeInput)
	{
		return encrypt_pkcs1_v15(&key, sl_null, input, sizeInput);
	}

	sl_bool RSA::encryptPrivate_pkcs1_v15(const RSAPrivateKey& key, const void* input, sl_size sizeInput, void* output)
	{
		return encrypt_pkcs1_v15(sl_null, &key, input, sizeInput, output);
	}

	Memory RSA::encryptPrivate_pkcs1_v15(const RSAPrivateKey& key, const void* input, sl_size sizeInput)
	{
		return encrypt_pkcs1_v15(sl_null, &key, input, sizeInput);
	}
	
	sl_uint32 RSA::decryptPublic_pkcs1_v15(const RSAPublicKey& key, const void* input, sl_size sizeInput, void* output, sl_bool* pFlagSign)
	{
		return decrypt_pkcs1_v15(&key, sl_null, input, sizeInput, output, pFlagSign);
	}

	Memory RSA::decryptPublic_pkcs1_v15(const RSAPublicKey& key, const void* input, sl_size sizeInput, sl_bool* pFlagSign)
	{
		return decrypt_pkcs1_v15(&key, sl_null, input, sizeInput, pFlagSign);
	}

	sl_uint32 RSA::decryptPrivate_pkcs1_v15(const RSAPrivateKey& key, const void* input, sl_size sizeInput, void* output, sl_bool* pFlagSign)
	{
		return decrypt_pkcs1_v15(sl_null, &key, input, sizeInput, output, pFlagSign);
	}

	Memory RSA::decryptPrivate_pkcs1_v15(const RSAPrivateKey& key, const void* input, sl_size sizeInput, sl_bool* pFlagSign)
	{
		return decrypt_pkcs1_v15(sl_null, &key, input, sizeInput, pFlagSign);
	}
	

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(DHCommonKey)
		
	DHCommonKey::DHCommonKey()
	{
	}

	void DHCommonKey::generate(sl_uint32 nBits)
	{
		P = BigInt::generatePrime(nBits);
		G = BigInt::random(nBits);
	}


	BigInt DH::getSharedKey(const DHCommonKey& common, const BigInt& localPrivateKey, const BigInt& remotePublicKey)
	{
		return BigInt::pow_montgomery(remotePublicKey, localPrivateKey, common.P);
	}
	
	BigInt DH::getPublicKey(const DHCommonKey& common, const BigInt& privateKey)
	{
		return BigInt::pow_montgomery(common.G, privateKey, common.P);
	}

}
