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

#ifndef CHECKHEADER_SLIB_CRYPTO_ECC
#define CHECKHEADER_SLIB_CRYPTO_ECC

#include "definition.h"

#include "../math/bigint.h"
#include "../core/list.h"
#include "../core/default_members.h"

/*
 		Elliptic-curve cryptography
 
 	https://en.wikipedia.org/wiki/Elliptic-curve_cryptography
 
*/

namespace slib
{
	
	class EllipticCurve;
	
	class SLIB_EXPORT ECPoint
	{
	public:
		BigInt x;
		BigInt y;
		
	public:
		ECPoint() noexcept;
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ECPoint)
		
	public:
		sl_bool isO() const noexcept;

		Memory toUncompressedFormat(const EllipticCurve& curve) const noexcept;
		
		Memory toUncompressedFormat(sl_size nBytesPerComponent = 0) const noexcept;
		
		sl_bool parseUncompressedFormat(const void* buf, sl_size n) noexcept;
		
		String toUncompressedFormatString(const EllipticCurve& curve) const noexcept;
		
		String toUncompressedFormatString(sl_size nBytesPerComponent = 0) const noexcept;
		
		sl_bool parseUncompressedFormatString(const StringParam& str) noexcept;

	};
	
	class SLIB_EXPORT EllipticCurve
	{
	public:
		BigInt p;
		BigInt a;
		BigInt b;
		ECPoint G;
		BigInt n; // order
		sl_uint8 h; // cofactor
		
		List<ECPoint> pow2g;
		
	public:
		EllipticCurve() noexcept;
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(EllipticCurve)
		
	public:
		static const EllipticCurve& secp256k1() noexcept;
		
	public:
		ECPoint addPoint(const ECPoint& p1, const ECPoint& p2) const noexcept;
		
		ECPoint doublePoint(const ECPoint& pt) const noexcept;
		
		ECPoint multiplyPoint(const ECPoint& pt, const BigInt& k) const noexcept;
		
		ECPoint multiplyG(const BigInt& k) const noexcept;
		
	};
	
	class SLIB_EXPORT ECPublicKey
	{
	public:
		ECPoint Q; // Q = dG

	public:
		ECPublicKey() noexcept;
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ECPublicKey)
		
	public:
		sl_bool isNull() const noexcept;

		sl_bool equals(const ECPublicKey& other) const noexcept;

		sl_bool checkValid(const EllipticCurve& curve) const noexcept;

		sl_bool verifySignature(const EllipticCurve& curve, const void* hash, sl_size size, const void* signature, sl_size sizeSignature) const noexcept;

	};

	class SLIB_EXPORT ECPrivateKey : public ECPublicKey
	{
	public:
		BigInt d;
		
	public:
		ECPrivateKey();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ECPrivateKey)
		
	public:
		sl_bool generate(const EllipticCurve& curve);
		
	};

	class SLIB_EXPORT ECPublicKey_secp256k1 : public ECPublicKey
	{
	public:
		ECPublicKey_secp256k1();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ECPublicKey_secp256k1)

	public:
		sl_bool checkValid() const;

		sl_bool verifySignature(const void* hash, sl_size size, const void* signature, sl_size sizeSignature) const;

	};

	class SLIB_EXPORT ECDSA_Signature
	{
	public:
		BigInt r;
		BigInt s;
		
	public:
		ECDSA_Signature();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ECDSA_Signature)
		
	};
	
	// Elliptic Curve Digital Signature Algorithm
	class SLIB_EXPORT ECDSA
	{
	public:
		// z < curve.n
		static ECDSA_Signature sign(const EllipticCurve& curve, const ECPrivateKey& key, const BigInt& z, BigInt* k = sl_null);
		
		static ECDSA_Signature sign(const EllipticCurve& curve, const ECPrivateKey& key, const void* hash, sl_size sizeHash, BigInt* k = sl_null);

		static ECDSA_Signature sign_SHA256(const EllipticCurve& curve, const ECPrivateKey& key, const void* data, sl_size size, BigInt* k = sl_null);

		// z < curve.n
		static sl_bool verify(const EllipticCurve& curve, const ECPublicKey& key, const BigInt& z, const ECDSA_Signature& signature);
		
		static sl_bool verify(const EllipticCurve& curve, const ECPublicKey& key, const void* hash, sl_size sizeHash, const ECDSA_Signature& signature);

		static sl_bool verify_SHA256(const EllipticCurve& curve, const ECPublicKey& key, const void* data, sl_size size, const ECDSA_Signature& signature);

	};
	
	// Elliptic Curve Diffie-Hellman
	class SLIB_EXPORT ECDH
	{
	public:
		static BigInt getSharedKey(const EllipticCurve& curve, const ECPrivateKey& keyLocal, const ECPublicKey& keyRemote);
		
	};
	
}

#endif

