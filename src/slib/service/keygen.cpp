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

#include "slib/service/keygen.h"

//#define KEYGEN_USE_OPENSSL

#include "slib/crypto/sha2.h"
#include "slib/crypto/chacha.h"
#include "slib/crypto/ecc.h"
#include "slib/crypto/base64.h"

#if defined(SLIB_PLATFORM_IS_ANDROID)
#include "slib/device/device.h"
#else
#include "slib/storage/disk.h"
#endif

#ifdef KEYGEN_USE_OPENSSL
#include <slib/crypto/openssl.h>
#endif

namespace slib
{

	namespace priv
	{
		namespace keygen
		{
			static const char g_pattern[33] = "0123456789ABCDFGHJKLMNPQRTUVWXYZ";

			static const sl_uint8 g_inverse_pattern[128] = {
				/*00*/ 255, 255, 255, 255, 255, 255, 255, 255,
				/*08*/ 255, 255, 255, 255, 255, 255, 255, 255,
				/*10*/ 255, 255, 255, 255, 255, 255, 255, 255,
				/*18*/ 255, 255, 255, 255, 255, 255, 255, 255,
				/*20*/ 255, 255, 255, 255, 255, 255, 255, 255,
				/*28*/ 255, 255, 255, 255, 255, 255, 255, 255,
				/*30*/ 0,   1,   2,   3,   4,   5,   6,   7,
				/*38*/ 8,   9,   255, 255, 255, 255, 255, 255,
				/*40*/ 255, 10,  11,  12,  13,  2,   14,  15,
				/*48*/ 16,  1,   17,  18,  19,  20,  21,  0,
				/*50*/ 22,  23,  24,  5,   25,  26,  27,  28,
				/*58*/ 29,  30,  31,  255, 255, 255, 255, 255,
				/*60*/ 255, 10,  11,  12,  13,  2,   14,  15,
				/*68*/ 16,  1,   17,  18,  19,  20,  21,  0,
				/*70*/ 22,  23,  24,  5,   25,  26,  27,  28,
				/*78*/ 29,  30,  31,  255, 255, 255, 255, 255
			};

			static String EncodePublicKey(ECPublicKey& key)
			{
				sl_uint8 buf[64];
				key.Q.x.getBytesBE(buf, 32);
				key.Q.y.getBytesBE(buf + 32, 32);
				return Base64::encodeUrl(buf, 64);
			}

			static sl_bool DecodePublicKey(const StringParam& strKey, ECPublicKey& key)
			{
				Memory mem = Base64::decode(strKey);
				sl_size n = mem.getSize();
				if (n != 64) {
					return sl_false;
				}
				sl_uint8* buf = (sl_uint8*)(mem.getData());
				key.Q.x = BigInt::fromBytesBE(buf, 32);
				key.Q.y = BigInt::fromBytesBE(buf + 32, 32);
				return sl_true;
			}

			static String EncodePrivateKey(const ECPrivateKey& key)
			{
				sl_uint8 buf[96];
				key.Q.x.getBytesBE(buf, 32);
				key.Q.y.getBytesBE(buf + 32, 32);
				key.d.getBytesBE(buf + 64, 32);
				return Base64::encodeUrl(buf, 96);
			}

			static sl_bool DecodePrivateKey(const StringParam& strKey, ECPrivateKey& key)
			{
				Memory mem = Base64::decode(strKey);
				sl_size n = mem.getSize();
				if (n != 96) {
					return sl_false;
				}
				sl_uint8* buf = (sl_uint8*)(mem.getData());
				key.Q.x = BigInt::fromBytesBE(buf, 32);
				key.Q.y = BigInt::fromBytesBE(buf + 32, 32);
				key.d = BigInt::fromBytesBE(buf + 64, 32);
				return sl_true;
			}

			static String EncodeCode(const void* _buf, sl_size nBuf)
			{
				sl_uint8* buf = (sl_uint8*)_buf;
				sl_size n = (nBuf * 8 + 4) / 5;
				sl_size m = (n - 1) >> 2;
				String ret = String::allocate(n + m);
				if (ret.isNotNull()) {
					char* str = ret.getData();
					sl_size p = 0;
					for (sl_size i = 0; i < n; i++) {
						sl_size k1 = p >> 3;
						sl_uint32 k2 = (sl_uint32)(p & 7);
						sl_uint8 c;
						if (k1 < nBuf) {
							c = buf[k1];
							if (k2) {
								c <<= k2;
							}
							c >>= 3;
							k1++;
							if (k2 > 3 && k1 < nBuf) {
								sl_uint8 c2 = buf[k1];
								c |= c2 >> (11 - k2);
							}
						} else {
							c = 0;
						}
						*str = g_pattern[c];
						str++;
						p += 5;
						if ((i & 3) == 3) {
							*str = '-';
							str++;
						}
					}
					return ret;
				}
				return sl_null;
			}

			static Memory DecodeCode(const StringParam& _code)
			{
				StringData code(_code);
				sl_size lenCode = code.getLength();
				if (!lenCode) {
					return sl_null;
				}
				sl_size nBuf = (lenCode * 5 + 7) >> 3;
				SLIB_SCOPED_BUFFER(sl_uint8, 256, buf, nBuf)
				if (!buf) {
					return sl_null;
				}
				Base::zeroMemory(buf, nBuf);
				char* str = code.getData();
				sl_size p = 0;
				for (sl_size i = 0; i < lenCode; i++) {
					sl_uint8 c = str[i];
					if (c < 128) {
						c = g_inverse_pattern[c];
						if (c < 32) {
							sl_size k1 = p >> 3;
							sl_uint32 k2 = (sl_uint32)(p & 7);
							if (k2 < 3) {
								buf[k1] |= c << (3 - k2);
							} else if (k2 == 3) {
								buf[k1] |= c;
							} else {
								buf[k1] |= c >> (k2 - 3);
								buf[k1 + 1] |= c << (11 - k2);
							}
							p += 5;
						}
					}
				}
				return Memory::create(buf, p >> 3);
			}

		}
	}

	using namespace priv::keygen;

	void Keygen::generateKey(String& privateKey, String& publicKey)
	{
		ECPrivateKey key;
#ifdef KEYGEN_USE_OPENSSL
		OpenSSL::generate_ECKey_secp256k1(key);
#else
		key.generate(EllipticCurve::secp256k1());
#endif
		publicKey = EncodePublicKey(key);
		privateKey = EncodePrivateKey(key);
	}

	String Keygen::getPublicKey(const StringParam& privateKey)
	{
		ECPrivateKey key;
		if (DecodePrivateKey(privateKey, key)) {
			return EncodePublicKey(key);
		}
		return sl_null;
	}

	String Keygen::getMachineCode()
	{
#if defined(SLIB_PLATFORM_IS_ANDROID)
		return Device::getIMEI();
#else
		return Disk::getSerialNumber(0);
#endif
	}

	String Keygen::getRequestCode(const StringParam& machineCode, const StringParam& _publicKey, const StringParam& _extraInfo)
	{
		StringData publicKey(_publicKey);
		StringData extraInfo(_extraInfo);
		sl_size lenExtra = extraInfo.getLength();
		sl_size nBuf = 10 + lenExtra;
		SLIB_SCOPED_BUFFER(sl_uint8, 256, buf, nBuf)
		if (!buf) {
			return sl_null;
		}
		{
			Base::zeroMemory(buf, 32);
			sl_uint8 hash[32];
			SHA256::hash(String::concat(extraInfo, publicKey, machineCode), hash);
			for (sl_uint32 i = 0; i < 32; i++) {
				buf[i % 10] ^= hash[i];
			}
		}
		if (lenExtra) {
			ChaCha20 c;
			sl_uint8 k[32];
			SHA256::hash(publicKey, k);
			c.setKey(k);
			c.encrypt(extraInfo.getData(), buf + 10, lenExtra);
		}
		return EncodeCode(buf, nBuf);
	}

	String Keygen::getRequestCode(const StringParam& publicKey, const StringParam& extraInfo)
	{
		return getRequestCode(getMachineCode(), publicKey, extraInfo);
	}
	
	String Keygen::getRequestCode()
	{
		return getRequestCode(sl_null, sl_null);
	}

	String Keygen::getExtraFromRequestCode(const StringParam& _publicKey, const StringParam& requestCode)
	{
		StringData publicKey(_publicKey);
		Memory mem = DecodeCode(requestCode);
		sl_size n = mem.getSize();
		if (n > 10) {
			ChaCha20 c;
			sl_uint8 k[32];
			SHA256::hash(publicKey, k);
			c.setKey(k);
			sl_uint8* buf = (sl_uint8*)(mem.getData());
			c.decrypt(buf + 10, buf + 10, n - 10);
			return String::fromUtf8(buf + 10, n - 10);
		}
		return sl_null;
	}

	String Keygen::generateAuthorizationCode(const StringParam& strPrivateKey, const StringParam& requestCode, const StringParam& _extraInfo)
	{
		StringData extraInfo(_extraInfo);
		ECPrivateKey key;
		if (!(DecodePrivateKey(strPrivateKey, key))) {
			return sl_null;
		}
		String s = String::concat(extraInfo, requestCode);
#ifdef KEYGEN_USE_OPENSSL
		ECDSA_Signature sig = OpenSSL::sign_ECDSA_SHA256_secp256k1(key, s.getData(), s.getLength());
#else
		ECDSA_Signature sig = ECDSA::sign_SHA256(EllipticCurve::secp256k1(), key, s.getData(), s.getLength());
#endif
		if (sig.r.isNull() || sig.s.isNull()) {
			return sl_null;
		}
		sl_size lenExtra = extraInfo.getLength();
		sl_size nBuf = 64 + lenExtra;
		SLIB_SCOPED_BUFFER(sl_uint8, 512, buf, nBuf)
		if (!nBuf) {
			return sl_null;
		}
		Base::zeroMemory(buf, nBuf);
		sig.r.getBytesBE(buf, 32);
		sig.s.getBytesBE(buf + 32, 32);
		if (lenExtra) {
			ChaCha20 c;
			sl_uint8 k[32];
			SHA256::hash(EncodePublicKey(key), k);
			c.setKey(k);
			c.encrypt(extraInfo.getData(), buf + 64, lenExtra);
		}
		return EncodeCode(buf, nBuf);
	}

	String Keygen::generateAuthorizationCode(const StringParam& privateKey, const StringParam& requestCode)
	{
		return generateAuthorizationCode(privateKey, requestCode, sl_null);
	}

	sl_bool Keygen::verifyAuthorizationCode(const StringParam& strPublicKey, const StringParam& requestCode, const StringParam& authCode)
	{
		ECPublicKey key;
		if (!(DecodePublicKey(strPublicKey, key))) {
			return sl_false;
		}
		Memory mem = DecodeCode(authCode);
		sl_size n = mem.getSize();
		if (n < 64) {
			return sl_false;
		}
		sl_uint8* buf = (sl_uint8*)(mem.getData());
		String extraInfo;
		if (n > 64) {
			ChaCha20 c;
			sl_uint8 k[32];
			SHA256::hash(strPublicKey, k);
			c.setKey(k);
			c.decrypt(buf + 64, buf + 64, n - 64);
			extraInfo = String::fromUtf8(buf + 64, n - 64);
		}
		ECDSA_Signature sig;
		sig.r = BigInt::fromBytesBE(buf, 32);
		sig.s = BigInt::fromBytesBE(buf + 32, 32);
		String s = String::concat(extraInfo, requestCode);
#ifdef KEYGEN_USE_OPENSSL
		return OpenSSL::verify_ECDSA_SHA256_secp256k1(key, s.getData(), s.getLength(), sig);
#else
		return ECDSA::verify_SHA256(EllipticCurve::secp256k1(), key, s.getData(), s.getLength(), sig);
#endif
	}

	String Keygen::getExtraFromAuthorizationCode(const StringParam& publicKey, const StringParam& authCode)
	{
		Memory mem = DecodeCode(authCode);
		sl_size n = mem.getSize();
		if (n > 64) {
			ChaCha20 c;
			sl_uint8 k[32];
			SHA256::hash(publicKey, k);
			c.setKey(k);
			sl_uint8* buf = (sl_uint8*)(mem.getData());
			c.decrypt(buf + 64, buf + 64, n - 64);
			return String::fromUtf8(buf + 64, n - 64);
		}
		return sl_null;
	}

}
