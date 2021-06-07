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

#include "slib/crypto/ecc.h"

#include "slib/crypto/sha2.h"

#include "slib/core/string_buffer.h"
#include "slib/core/math.h"
#include "slib/core/safe_static.h"

#include "ecc_secp256k1.inc"

namespace slib
{
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ECPoint)
	
	ECPoint::ECPoint() noexcept
	{
	}
	
	sl_bool ECPoint::isO() const noexcept
	{
		return y.isZero();
	}
	
	Memory ECPoint::toUncompressedFormat(const EllipticCurve& curve) const noexcept
	{
		return toUncompressedFormat(curve.n.getMostSignificantBytes());
	}
	
	Memory ECPoint::toUncompressedFormat(sl_size nBytesPerComponent) const noexcept
	{
		if (isO()) {
			sl_uint8 c = 0;
			return Memory::create(&c, 1);
		}
		if (!nBytesPerComponent) {
			nBytesPerComponent = Math::max(x.getMostSignificantBytes(), y.getMostSignificantBytes());
		}
		Memory ret = Memory::create((nBytesPerComponent << 1) + 1);
		if (ret.isNotNull()) {
			sl_uint8* buf = (sl_uint8*)(ret.getData());
			buf[0] = 4;
			if (x.getBytesBE(buf + 1, nBytesPerComponent)) {
				if (y.getBytesBE(buf + 1 + nBytesPerComponent, nBytesPerComponent)) {
					return ret;
				}
			}
		}
		return sl_null;
	}
	
	sl_bool ECPoint::parseUncompressedFormat(const void* _buf, sl_size size) noexcept
	{
		const sl_uint8* buf = (const sl_uint8*)_buf;
		if (size) {
			if (buf[0] == 4) {
				size--;
				if (!(size & 1)) {
					size >>= 1;
					BigInt _x = BigInt::fromBytesBE(buf + 1, size);
					if (_x.isNotNull()) {
						BigInt _y = BigInt::fromBytesBE(buf + 1 + size, size);
						if (_y.isNotNull()) {
							x = Move(_x);
							y = Move(_y);
							return sl_true;
						}
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool ECPoint::parseUncompressedFormat(const Memory& mem) noexcept
	{
		return parseUncompressedFormat(mem.getData(), mem.getSize());
	}

	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(EllipticCurve)

	EllipticCurve::EllipticCurve() noexcept: h(1)
	{
	}
	
	ECPoint EllipticCurve::addPoint(const ECPoint& p1, const ECPoint& p2) const noexcept
	{
		if (p1.isO()) {
			return p2;
		} else if (p2.isO()) {
			return p1;
		}
		if (p1.x == p2.x) {
			if (p1.y + p2.y == p) {
				return ECPoint();
			} else {
				return doublePoint(p1);
			}
		} else {
			ECPoint ret;
			BigInt lambda = BigInt::mod_NonNegativeRemainder((p2.y - p1.y) * BigInt::inverseMod(p2.x - p1.x, p), p);
			ret.x = BigInt::mod_NonNegativeRemainder((lambda * lambda) - p1.x - p2.x, p);
			ret.y = BigInt::mod_NonNegativeRemainder((lambda * (p1.x - ret.x)) - p1.y, p);
			return ret;
		}
	}
	
	ECPoint EllipticCurve::doublePoint(const ECPoint& pt) const noexcept
	{
		if (pt.isO()) {
			return pt;
		}
		ECPoint ret;
		BigInt x2 = pt.x * pt.x;
		BigInt lambda = BigInt::mod_NonNegativeRemainder((x2 + x2 + x2 + a) * BigInt::inverseMod(pt.y + pt.y, p), p);
		ret.x = BigInt::mod_NonNegativeRemainder((lambda * lambda) - pt.x - pt.x, p);
		ret.y = BigInt::mod_NonNegativeRemainder((lambda * (pt.x - ret.x)) - pt.y, p);
		return ret;
	}
	
	ECPoint EllipticCurve::multiplyPoint(const ECPoint& pt, const BigInt& _k) const noexcept
	{
		CBigInt* k = _k.ref.get();
		if (!k) {
			return ECPoint();
		}
		if (k->isZero()) {
			return ECPoint();
		}
		if (k->equals(1)) {
			return pt;
		}
		sl_size nBits = k->getMostSignificantBits();
		ECPoint ret;
		ECPoint pt2 = pt;
		for (sl_size i = 0; i < nBits; i++) {
			if (k->getBit(i)) {
				ret = addPoint(ret, pt2);
			}
			pt2 = doublePoint(pt2);
		}
		return ret;
	}
	
	ECPoint EllipticCurve::multiplyG(const BigInt& _k) const noexcept
	{
		if (pow2g.isNull()) {
			return multiplyPoint(G, _k);
		}
		CBigInt* k = _k.ref.get();
		if (!k) {
			return ECPoint();
		}
		if (k->isZero()) {
			return ECPoint();
		}
		if (k->equals(1)) {
			return G;
		}
		sl_size nBits = k->getMostSignificantBits();
		ECPoint ret;
		ECPoint pt;
		for (sl_size i = 0; i < nBits; i++) {
			if (k->getBit(i)) {
				if (i) {
					pow2g.getAt(i - 1, &pt);
					ret = addPoint(ret, pt);
				} else {
					ret = addPoint(ret, G);
				}
			}
		}
		return ret;
	}
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ECPublicKey)
	
	ECPublicKey::ECPublicKey() noexcept
	{
	}

	sl_bool ECPublicKey::isNull() const noexcept
	{
		return Q.x.isNull();
	}

	sl_bool ECPublicKey::equals(const ECPublicKey& other) const noexcept
	{
		return Q.x.equals(other.Q.x);
	}

	sl_compare_result ECPublicKey::compare(const ECPublicKey& other) const noexcept
	{
		return Q.x.compare(other.Q.x);
	}

	sl_bool ECPublicKey::checkValid(const EllipticCurve& curve) const noexcept
	{
		if (Q.isO()) {
			return sl_false;
		}
		if (Q.x >= curve.p) {
			return sl_false;
		}
		if (Q.y >= curve.p) {
			return sl_false;
		}
		BigInt dy = BigInt::mod_NonNegativeRemainder((Q.x * Q.x * Q.x) + (curve.a * Q.x) + curve.b - (Q.y * Q.y), curve.p);
		if (dy.isNotZero()) {
			return sl_false;
		}
		ECPoint nQ = curve.multiplyPoint(Q, curve.n);
		if (!(nQ.isO())) {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool ECPublicKey::verifySignature(const EllipticCurve& curve, const void* hash, sl_size size, const void* _signature, sl_size sizeSignature) const noexcept
	{
		const sl_uint8* signature = (const sl_uint8*)_signature;
		if (sizeSignature & 1) {
			return sl_false;
		}
		sizeSignature >>= 1;
		ECDSA_Signature sig;
		sig.r = BigInt::fromBytesBE(signature, sizeSignature);
		sig.s = BigInt::fromBytesBE(signature + sizeSignature, sizeSignature);
		return ECDSA::verify(curve, *this, hash, size, sig);
	}

	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ECPrivateKey)
	
	ECPrivateKey::ECPrivateKey() noexcept
	{
	}
	
	sl_bool ECPrivateKey::generate(const EllipticCurve& curve) noexcept
	{
		if (curve.n < 3) {
			return sl_false;
		}
		BigInt n2 = curve.n - 2;
		for (;;) {
			d = BigInt::random(curve.n.getMostSignificantBits());
			if (d.isNull()) {
				return sl_false;
			}
			d = BigInt::mod_NonNegativeRemainder(d, n2) + 2;
			Q = curve.multiplyG(d);
			if (checkValid(curve)) {
				break;
			}
		}
		return sl_true;
	}

	Memory ECPrivateKey::generateSignature(const EllipticCurve& curve, const void* hash, sl_size size) const noexcept
	{
		ECDSA_Signature sig = ECDSA::sign(curve, *this, hash, size);
		sl_size n1 = sig.r.getMostSignificantBytes();
		sl_size n2 = sig.s.getMostSignificantBytes();
		sl_size n = Math::max(n1, n2);
		Memory ret = Memory::create(n << 1);
		if (ret.isNotNull()) {
			sl_uint8* signature = (sl_uint8*)(ret.getData());
			sig.r.getBytesBE(signature, n);
			sig.s.getBytesBE(signature + n, n);
			return ret;
		}
		return sl_null;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ECPublicKey_secp256k1)

	ECPublicKey_secp256k1::ECPublicKey_secp256k1() noexcept
	{
	}

	sl_bool ECPublicKey_secp256k1::checkValid() const noexcept
	{
		return ECPublicKey::checkValid(EllipticCurve::secp256k1());
	}

	sl_bool ECPublicKey_secp256k1::verifySignature(const void* hash, sl_size size, const void* signature, sl_size sizeSignature) const noexcept
	{
		return ECPublicKey::verifySignature(EllipticCurve::secp256k1(), hash, size, signature, sizeSignature);
	}

	Bytes<32> ECPublicKey_secp256k1::toId() const noexcept
	{
		Bytes<32> ret;
		Q.x.getBytesBE(ret.data, 32);
		return ret;
	}
	

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ECPrivateKey_secp256k1)

	ECPrivateKey_secp256k1::ECPrivateKey_secp256k1() noexcept
	{
	}

	Memory ECPrivateKey_secp256k1::generateSignature(const void* hash, sl_size size) const noexcept
	{
		return ECPrivateKey::generateSignature(EllipticCurve::secp256k1(), hash, size);
	}

	sl_bool ECPrivateKey_secp256k1::checkValid() const noexcept
	{
		return ECPublicKey::checkValid(EllipticCurve::secp256k1());
	}

	sl_bool ECPrivateKey_secp256k1::verifySignature(const void* hash, sl_size size, const void* signature, sl_size sizeSignature) const noexcept
	{
		return ECPublicKey::verifySignature(EllipticCurve::secp256k1(), hash, size, signature, sizeSignature);
	}

	Bytes<32> ECPrivateKey_secp256k1::toId() const noexcept
	{
		Bytes<32> ret;
		Q.x.getBytesBE(ret.data, 32);
		return ret;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ECDSA_Signature)
	
	ECDSA_Signature::ECDSA_Signature() noexcept
	{
	}
	
	namespace priv
	{
		namespace ecdsa
		{
			static BigInt MakeZ(const EllipticCurve& curve, const void* hash, sl_size hashSize) noexcept
			{
				sl_size nBits = curve.n.getMostSignificantBits();
				if (!nBits) {
					return BigInt::null();
				}
				sl_size hashBits = hashSize << 3;
				if (nBits > hashBits) {
					return BigInt::fromBytesBE(hash, hashSize);
				} else {
					sl_uint32 leftBits = (sl_uint32)(nBits & 7);
					if (leftBits) {
						sl_size nBytes = (nBits >> 3) + 1;
						return BigInt::fromBytesBE(hash, nBytes) >> (8 - leftBits);
					} else {
						return BigInt::fromBytesBE(hash, nBits >> 3);
					}
				}
			}
		}
	}
	
	ECDSA_Signature ECDSA::sign(const EllipticCurve& curve, const ECPrivateKey& key, const BigInt& z, BigInt* _k) noexcept
	{
		if (curve.G.isO()) {
			return ECDSA_Signature();
		}
		sl_size nBitsOrder = curve.n.getMostSignificantBits();
		if (nBitsOrder < 2) {
			return ECDSA_Signature();
		}
		BigInt r, s;
		for (;;) {
			sl_bool flagInputK = sl_false;
			BigInt k;
			if (_k) {
				if (_k->isNull()) {
					k = BigInt::mod_NonNegativeRemainder(BigInt::random(nBitsOrder), curve.n - 1) + 1;
				} else {
					flagInputK = sl_true;
					k = *_k;
				}
			} else {
				k = BigInt::mod_NonNegativeRemainder(BigInt::random(nBitsOrder), curve.n - 1) + 1;
			}
			ECPoint kG = curve.multiplyG(k);
			if (kG.isO()) {
				if (flagInputK) {
					return ECDSA_Signature();
				}
				continue;
			}
			r = BigInt::mod_NonNegativeRemainder(kG.x, curve.n);
			if (r.isZero()) {
				if (flagInputK) {
					return ECDSA_Signature();
				}
				continue;
			}
			BigInt k1 = BigInt::inverseMod(k, curve.n);
			s = BigInt::mod_NonNegativeRemainder(k1 * (z + r * key.d), curve.n);
			if (s.isZero()) {
				if (flagInputK) {
					return ECDSA_Signature();
				}
				continue;
			}
			if (!flagInputK) {
				if (_k) {
					*_k = k;
				}
			}
			break;
		}
		ECDSA_Signature ret;
		ret.r = r;
		ret.s = s;
		return ret;
	}
	
	ECDSA_Signature ECDSA::sign(const EllipticCurve& curve, const ECPrivateKey& key, const void* hash, sl_size size, BigInt* k) noexcept
	{
		return sign(curve, key, priv::ecdsa::MakeZ(curve, hash, size), k);
	}
	
	ECDSA_Signature ECDSA::sign_SHA256(const EllipticCurve& curve, const ECPrivateKey& key, const void* data, sl_size size, BigInt* k) noexcept
	{
		sl_uint8 hash[SHA256::HashSize];
		SHA256::hash(data, size, hash);
		return sign(curve, key, priv::ecdsa::MakeZ(curve, hash, SHA256::HashSize), k);
	}
	
	sl_bool ECDSA::verify(const EllipticCurve& curve, const ECPublicKey& key, const BigInt& z, const ECDSA_Signature& signature) noexcept
	{
		if (!(key.checkValid(curve))) {
			return sl_false;
		}
		if (signature.r.isZero()) {
			return sl_false;
		}
		if (signature.r >= curve.n) {
			return sl_false;
		}
		if (signature.s.isZero()) {
			return sl_false;
		}
		if (signature.s >= curve.n) {
			return sl_false;
		}
		BigInt s1 = BigInt::inverseMod(signature.s, curve.n);
		BigInt u1 = BigInt::mod_NonNegativeRemainder(z * s1, curve.n);
		BigInt u2 = BigInt::mod_NonNegativeRemainder(signature.r * s1, curve.n);
		ECPoint p1 = curve.multiplyG(u1);
		ECPoint p2 = curve.multiplyPoint(key.Q, u2);
		ECPoint kG = curve.addPoint(p1, p2);
		if (kG.isO()) {
			return sl_false;
		}
		return kG.x == signature.r;
	}
	
	sl_bool ECDSA::verify(const EllipticCurve& curve, const ECPublicKey& key, const void* hash, sl_size size, const ECDSA_Signature& signature) noexcept
	{
		return verify(curve, key, priv::ecdsa::MakeZ(curve, hash, size), signature);
	}
	
	sl_bool ECDSA::verify_SHA256(const EllipticCurve& curve, const ECPublicKey& key, const void* data, sl_size size, const ECDSA_Signature& signature) noexcept
	{
		sl_uint8 hash[SHA256::HashSize];
		SHA256::hash(data, size, hash);
		return verify(curve, key, priv::ecdsa::MakeZ(curve, hash, SHA256::HashSize), signature);
	}
	
	BigInt ECDH::getSharedKey(const EllipticCurve& curve, const ECPrivateKey& keyLocal, const ECPublicKey& keyRemote) noexcept
	{
		if (!(keyRemote.checkValid(curve))) {
			return BigInt::null();
		}
		ECPoint pt = curve.multiplyPoint(keyRemote.Q, keyLocal.d);
		return pt.x;
	}
	
}
