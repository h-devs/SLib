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
		SLIB_DECLARE_CLASS_JSON_SERIALIZE_MEMBERS
		
	public:
		// infinity
		sl_bool isO() const noexcept;

		Memory toUncompressedFormat(const EllipticCurve& curve) const noexcept;
		
		Memory toUncompressedFormat(sl_size nBytesPerComponent = 0) const noexcept;

		Memory toCompressedFormat(const EllipticCurve& curve) const noexcept;

		Memory toCompressedFormat(sl_size nBytesPerComponent = 0) const noexcept;

		Memory toHybridFormat(const EllipticCurve& curve) const noexcept;

		Memory toHybridFormat(sl_size nBytesPerComponent = 0) const noexcept;

		sl_bool parseBinaryFormat(const void* buf, sl_size n, const EllipticCurve* curve = sl_null) noexcept;

		sl_bool parseBinaryFormat(const Memory& mem, const EllipticCurve* curve = sl_null) noexcept;

		sl_bool parseBinaryFormat(const EllipticCurve& curve, const void* buf, sl_size n) noexcept;

		sl_bool parseBinaryFormat(const EllipticCurve& curve, const Memory& mem) noexcept;

	};

	// OpenSSL NID
	enum class EllipticCurveId
	{
		Unknown = 0,
		secp112r1 = 704,
		secp112r2 = 705,
		secp128r1 = 706,
		secp128r2 = 707,
		secp160k1 = 708,
		secp160r1 = 709,
		secp160r2 = 710,
		secp192k1 = 711,
		secp224k1 = 712,
		secp256k1 = 714,
		secp384r1 = 715,
		secp521r1 = 716
	};
	
	// y^2 = x^3 + a*x + b
	class SLIB_EXPORT EllipticCurve
	{
	public:
		EllipticCurveId id;
		BigInt p;
		BigInt a;
		BigInt b;
		ECPoint G; // generator
		BigInt n; // order

	public:
		EllipticCurve() noexcept;
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(EllipticCurve)
		
	public:
		static const EllipticCurve& secp112r1() noexcept;
		static const EllipticCurve& secp112r2() noexcept;
		static const EllipticCurve& secp128r1() noexcept;
		static const EllipticCurve& secp128r2() noexcept;
		static const EllipticCurve& secp160k1() noexcept;
		static const EllipticCurve& secp160r1() noexcept;
		static const EllipticCurve& secp160r2() noexcept;
		static const EllipticCurve& secp192k1() noexcept;
		static const EllipticCurve& secp224k1() noexcept;
		static const EllipticCurve& secp256k1() noexcept;
		static const EllipticCurve& secp384r1() noexcept;
		static const EllipticCurve& secp521r1() noexcept;
		
	public:
		sl_bool isDefined() const noexcept;

		sl_bool setCurveId(EllipticCurveId id) noexcept;

		ECPoint addPoint(const ECPoint& p1, const ECPoint& p2) const noexcept;
		
		ECPoint doublePoint(const ECPoint& pt) const noexcept;
		
		ECPoint multiplyPoint(const ECPoint& pt, const BigInt& k) const noexcept;
		
		ECPoint multiplyG(const BigInt& k) const noexcept;
		
		BigInt getY(const BigInt& x, sl_bool yBit) const noexcept;

	};
	
	class SLIB_EXPORT ECPublicKey
	{
	public:
		ECPoint Q; // Q = dG

	public:
		ECPublicKey() noexcept;
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ECPublicKey)
		SLIB_DECLARE_CLASS_JSON_SERIALIZE_MEMBERS
		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS

	public:
		sl_bool isDefined() const noexcept;

		sl_bool equals(const ECPublicKey& other) const noexcept;

		sl_compare_result compare(const ECPublicKey& other) const noexcept;

	public:
		sl_bool checkValid(const EllipticCurve& curve) const noexcept;

	};

	class SLIB_EXPORT ECPrivateKey : public ECPublicKey
	{
	public:
		BigInt d;
		
	public:
		ECPrivateKey() noexcept;
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ECPrivateKey)
		SLIB_DECLARE_CLASS_JSON_SERIALIZE_MEMBERS

	public:
		sl_bool isDefined() const noexcept;

		sl_bool generate(const EllipticCurve& curve) noexcept;

	};

	class SLIB_EXPORT ECPublicKeyWithCurve : public ECPublicKey, public EllipticCurve
	{
	public:
		ECPublicKeyWithCurve() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ECPublicKeyWithCurve)

	public:
		sl_bool isDefined() const noexcept;
		
	};

	class SLIB_EXPORT ECPrivateKeyWithCurve : public ECPrivateKey, public EllipticCurve
	{
	public:
		ECPrivateKeyWithCurve() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ECPrivateKeyWithCurve)

	public:
		sl_bool isDefined() const noexcept;

		sl_bool generate() noexcept;

	};

	class SLIB_EXPORT ECDSA_Signature
	{
	public:
		BigInt r;
		BigInt s;
		
	public:
		ECDSA_Signature() noexcept;

		template <class R, class S>
		ECDSA_Signature(R&& _r, S&& _s) noexcept: r(Forward<R>(_r)), s(Forward<S>(_s)) {}
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ECDSA_Signature)
		SLIB_DECLARE_CLASS_JSON_SERIALIZE_MEMBERS
	
	public:
		Memory serialize() const noexcept;

		sl_bool deserialize(const void* buf, sl_size size);
		sl_bool deserialize(const Memory& mem) noexcept;

	};
	
	// Elliptic Curve Digital Signature Algorithm
	class SLIB_EXPORT ECDSA
	{
	public:
		// z < curve.n
		static ECDSA_Signature sign(const EllipticCurve& curve, const ECPrivateKey& key, const BigInt& z, BigInt* k = sl_null) noexcept;
		
		static ECDSA_Signature sign(const EllipticCurve& curve, const ECPrivateKey& key, const void* hash, sl_size sizeHash, BigInt* k = sl_null) noexcept;

		static ECDSA_Signature sign_SHA256(const EllipticCurve& curve, const ECPrivateKey& key, const void* data, sl_size size, BigInt* k = sl_null) noexcept;

		static ECDSA_Signature sign_SHA384(const EllipticCurve& curve, const ECPrivateKey& key, const void* data, sl_size size, BigInt* k = sl_null) noexcept;

		static ECDSA_Signature sign_SHA512(const EllipticCurve& curve, const ECPrivateKey& key, const void* data, sl_size size, BigInt* k = sl_null) noexcept;

		// z < curve.n
		static sl_bool verify(const EllipticCurve& curve, const ECPublicKey& key, const BigInt& z, const ECDSA_Signature& signature) noexcept;
		
		static sl_bool verify(const EllipticCurve& curve, const ECPublicKey& key, const void* hash, sl_size sizeHash, const ECDSA_Signature& signature) noexcept;

		static sl_bool verify_SHA256(const EllipticCurve& curve, const ECPublicKey& key, const void* data, sl_size size, const ECDSA_Signature& signature) noexcept;

		static sl_bool verify_SHA384(const EllipticCurve& curve, const ECPublicKey& key, const void* data, sl_size size, const ECDSA_Signature& signature) noexcept;

		static sl_bool verify_SHA512(const EllipticCurve& curve, const ECPublicKey& key, const void* data, sl_size size, const ECDSA_Signature& signature) noexcept;

	};
	
	// Elliptic Curve Diffie-Hellman
	class SLIB_EXPORT ECDH
	{
	public:
		static BigInt getSharedKey(const EllipticCurve& curve, const ECPrivateKey& keyLocal, const ECPublicKey& keyRemote) noexcept;
		
	};
	
}

#endif

