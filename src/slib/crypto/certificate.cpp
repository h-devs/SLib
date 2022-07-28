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

#include "slib/crypto/certificate.h"
#include "slib/crypto/x509.h"
#include "slib/crypto/pkcs12.h"

#include "slib/core/file.h"
#include "slib/crypto/asn1.h"
#include "slib/crypto/sha1.h"
#include "slib/crypto/sha2.h"
#include "slib/crypto/des.h"
#include "slib/crypto/rc2.h"

#define OID_ISO_US "\x2A\x86\x48" // ISO(1) Member-Body(2) US(840)
#define OID_RSADSI OID_ISO_US "\x86\xF7\x0D" // 113549
#define OID_PKCS OID_RSADSI "\x01"
#define OID_PKCS1 OID_PKCS "\x01"
#define OID_PKCS1_RSA OID_PKCS1 "\x01"
#define OID_PKCS1_SHA256_WITH_RSA OID_PKCS1 "\x0B" // 11
#define OID_PKCS1_SHA384_WITH_RSA OID_PKCS1 "\x0C" // 12
#define OID_PKCS1_SHA512_WITH_RSA OID_PKCS1 "\x0D" // 13
#define OID_PKCS1_SHA224_WITH_RSA OID_PKCS1 "\x0E" // 14
#define OID_PKCS7 OID_PKCS "\x07"
#define OID_PKCS7_DATA OID_PKCS7 "\x01"
#define OID_PKCS7_ENCRYPTED_DATA OID_PKCS7 "\x06"
#define OID_PKCS9 OID_PKCS "\x09"
#define OID_PKCS9_CERTIFICATE_TYPES OID_PKCS9 "\x16" // 22
#define OID_PKCS9_X509_CERTIFICATE OID_PKCS9_CERTIFICATE_TYPES "\x01"
#define OID_PKCS12 OID_PKCS "\x0C" // 12
#define OID_PKCS12_VERSION1 OID_PKCS12 "\x0A" // 10
#define OID_PKCS12_BAG_IDS OID_PKCS12_VERSION1 "\x01"
#define OID_PKCS12_KEY_BAG OID_PKCS12_BAG_IDS "\x01"
#define OID_PKCS12_PKCS8_SHROUNDED_KEY_BAG OID_PKCS12_BAG_IDS "\x02"
#define OID_PKCS12_CERTIFICATE_BAG OID_PKCS12_BAG_IDS "\x03"
#define OID_PKCS12_PBE_IDS OID_PKCS12 "\x01"
#define OID_PKCS12_PBE_SHA1_RC4_128 OID_PKCS12_PBE_IDS "\x01"
#define OID_PKCS12_PBE_SHA1_RC4_40 OID_PKCS12_PBE_IDS "\x02"
#define OID_PKCS12_PBE_SHA1_3DES OID_PKCS12_PBE_IDS "\x03"
#define OID_PKCS12_PBE_SHA1_2DES OID_PKCS12_PBE_IDS "\x04"
#define OID_PKCS12_PBE_SHA1_RC2_128 OID_PKCS12_PBE_IDS "\x05"
#define OID_PKCS12_PBE_SHA1_RC2_40 OID_PKCS12_PBE_IDS "\x06"
#define OID_X9_62 OID_ISO_US "\xCE\x3D" // 10045
#define OID_X9_62_FIELD_TYPE OID_X9_62 "\x01"
#define OID_X9_62_PRIME_FIELD OID_X9_62_FIELD_TYPE "\x01"
#define OID_X9_62_PUBLIC_KEY OID_X9_62 "\x02"
#define OID_X9_62_EC_PUBLIC_KEY OID_X9_62_PUBLIC_KEY "\x01"
#define OID_X9_62_SIGNATURE_TYPE OID_X9_62 "\x04"
#define OID_ECDSA_WITH_SPECIFIED OID_X9_62_SIGNATURE_TYPE "\x03"
#define OID_ECDSA_WITH_SHA224 OID_ECDSA_WITH_SPECIFIED "\x01"
#define OID_ECDSA_WITH_SHA256 OID_ECDSA_WITH_SPECIFIED "\x02"
#define OID_ECDSA_WITH_SHA384 OID_ECDSA_WITH_SPECIFIED "\x03"
#define OID_ECDSA_WITH_SHA512 OID_ECDSA_WITH_SPECIFIED "\x04"
#define OID_X500 "\x55" // 2, 5
#define OID_X509 OID_X500 "\x04"
#define OID_X509_COMMON_NAME OID_X509 "\x03"
#define OID_X509_COUNTRY_NAME OID_X509 "\x06"
#define OID_X509_LOCALITY_NAME OID_X509 "\x07"
#define OID_X509_STATE_OR_PROVINCE_NAME OID_X509 "\x08"
#define OID_X509_ORGANIZATION_NAME OID_X509 "\x0A" // 10
#define OID_X509_ORGANIZATION_UNIT_NAME OID_X509 "\x0B" // 11
#define OID_X509_TITLE OID_X509 "\x0C" // 12
#define OID_X509_SURNAME OID_X509 "\x04"
#define OID_X509_SEARCH_GUIDE OID_X509 "\x0E" // 13
#define OID_X509_DESCRIPTION OID_X509 "\x0D" // 14
#define OID_X509_STREET_ADDRESS OID_X509 "\x09"
#define OID_X509_BUSINESS_CATEGORY OID_X509 "\x0F" // 15
#define OID_X509_POSTAL_ADDRESS OID_X509 "\x10" // 16
#define OID_X509_POSTAL_CODE OID_X509 "\x11" // 17
#define OID_X509_POSTAL_OFFICE_BOX OID_X509 "\x12" // 18
#define OID_X509_TELEPHONE_NUMBER OID_X509 "\x14" // 20
#define OID_IDENTIFIED_ORGANIZATION "\x2B" // ISO(1), 3
#define OID_CERTICOM_ARC OID_IDENTIFIED_ORGANIZATION "\x84" // 132
#define OID_SECG_ELLIPTIC_CURVE OID_CERTICOM_ARC "\x00"
#define OID_secp112r1 OID_SECG_ELLIPTIC_CURVE "\x06"
#define OID_secp112r2 OID_SECG_ELLIPTIC_CURVE "\x07"
#define OID_secp128r1 OID_SECG_ELLIPTIC_CURVE "\x1C" // 28
#define OID_secp128r2 OID_SECG_ELLIPTIC_CURVE "\x1D" // 29
#define OID_secp160k1 OID_SECG_ELLIPTIC_CURVE "\x09"
#define OID_secp160r1 OID_SECG_ELLIPTIC_CURVE "\x08"
#define OID_secp160r2 OID_SECG_ELLIPTIC_CURVE "\x1E" // 30
#define OID_secp192k1 OID_SECG_ELLIPTIC_CURVE "\x1F" // 31
#define OID_secp224k1 OID_SECG_ELLIPTIC_CURVE "\x20" // 32
#define OID_secp256k1 OID_SECG_ELLIPTIC_CURVE "\x0A" // 10
#define OID_secp384r1 OID_SECG_ELLIPTIC_CURVE "\x22" // 34
#define OID_secp521r1 OID_SECG_ELLIPTIC_CURVE "\x23" // 35

namespace slib
{

	namespace priv
	{
		namespace cert
		{

			class X509Algorithm
			{
			public:
				Asn1ObjectIdentifier algorithm;
				Asn1Element parameter;

			public:
				sl_bool load(const Asn1Element& element)
				{
					Asn1MemoryReader body;
					if (element.getSequence(body)) {
						if (body.readObjectIdentifier(algorithm)) {
							if (body.readElement(parameter)) {
								return sl_true;
							}
						}
					}
					return sl_false;
				}

			};

			class X509Signature
			{
			public:
				X509Algorithm algorithm;
				Asn1String digest;

			public:
				sl_bool load(const Asn1Element& element)
				{
					Asn1MemoryReader body;
					if (element.getSequence(body)) {
						if (body.readObject(algorithm)) {
							return body.readOctetString(digest);
						}
					}
					return sl_false;
				}

			};

			class PKCS7
			{
			public:
				Asn1ObjectIdentifier type;
				Asn1Element content;

			public:
				sl_bool load(const Asn1Element& element)
				{
					Asn1MemoryReader body;
					if (!(element.getSequence(body))) {
						return sl_false;
					}
					if (!(body.readObjectIdentifier(type))) {
						return sl_false;
					}
					body.readElement(content);
					return sl_true;
				}

				sl_bool getData(Asn1String& _out)
				{
					return content.getOctetString(_out);
				}

			};

			static List<PKCS7> PKCS12_UnpackAuthSafes(PKCS7& authSafes)
			{
				if (!(authSafes.type.equals(OID_PKCS7_DATA))) {
					return sl_null;
				}
				Asn1String data;
				if (!(authSafes.getData(data))) {
					return sl_null;
				}
				Asn1MemoryReader reader(data.data, data.length);
				Asn1MemoryReader body;
				if (!(reader.readSequence(body))) {
					return sl_null;
				}
				List<PKCS7> ret;
				PKCS7 item;
				while (body.readObject(item)) {
					ret.add_NoLock(Move(item));
				}
				return ret;
			}

			class PKCS12_Bag
			{
			public:
				Asn1ObjectIdentifier type;
				Asn1Element content;

			public:
				sl_bool load(const Asn1Element& element)
				{
					Asn1MemoryReader body;
					if (!(element.getSequence(body))) {
						return sl_false;
					}
					if (!(body.readObjectIdentifier(type))) {
						return sl_false;
					}
					if (!(body.readOctetString(content))) {
						return sl_false;
					}
					return sl_true;
				}

			};

			class PKCS12_SafeBag
			{
			public:
				Asn1ObjectIdentifier type;
				Asn1Element content;
				
			public:
				sl_bool load(const Asn1Element& element)
				{
					Asn1MemoryReader body;
					if (!(element.getSequence(body))) {
						return sl_false;
					}
					if (!(body.readObjectIdentifier(type))) {
						return sl_false;
					}
					if (!(body.readElement(content))) {
						return sl_false;
					}
					return sl_true;
				}

			};

			static List<PKCS12_SafeBag> PKCS12_UnpackSafeBags(const void* data, sl_size size)
			{
				Asn1MemoryReader reader(data, size);
				Asn1MemoryReader body;
				if (!(reader.readSequence(body))) {
					return sl_null;
				}
				List<PKCS12_SafeBag> ret;
				PKCS12_SafeBag item;
				while (body.readObject(item)) {
					ret.add_NoLock(Move(item));
				}
				return ret;
			}

			static List<PKCS12_SafeBag> PKCS12_Unpack_PKCS7_Data(PKCS7& p7)
			{
				Asn1String data;
				if (!(p7.getData(data))) {
					return sl_null;
				}
				return PKCS12_UnpackSafeBags(data.data, data.length);
			}

			class PKCS12_PBE_Param
			{
			public:
				Asn1String salt;
				sl_uint64 iteration;

			public:
				sl_bool load(const Asn1Element& element)
				{
					Asn1MemoryReader body;
					if (!(element.getSequence(body))) {
						return sl_false;
					}
					if (!(body.readOctetString(salt))) {
						return sl_false;
					}
					if (!(body.readInt(iteration))) {
						return sl_false;
					}
					return sl_true;
				}

			};

			template <class MD>
			static sl_bool PKCS12_DeriveKey(
				const void* _password, sl_size sizePassword,
				const void* _salt, sl_size sizeSalt,
				sl_uint8 id, sl_uint64 iteration,
				void* _output, sl_size sizeOutput)
			{
				sl_size lenSalt = ((sizeSalt + MD::BlockSize - 1) / MD::BlockSize) * MD::BlockSize;
				sl_size lenPassword = ((sizePassword + MD::BlockSize - 1) / MD::BlockSize) * MD::BlockSize;
				sl_size lenI = lenSalt + lenPassword;
				SLIB_SCOPED_BUFFER(sl_uint8, 256, I, lenI)
				if (!I) {
					return sl_false;
				}

				sl_uint8* salt = (sl_uint8*)_salt;
				sl_uint8* password = (sl_uint8*)_password;
				sl_uint8* output = (sl_uint8*)_output;

				sl_uint8 D[MD::BlockSize];
				sl_size i;

				for (i = 0; i < sizeof(D); i++) {
					D[i] = id;
				}
				{
					sl_uint8* p = I;
					for (i = 0; i < lenSalt; i++) {
						*p = salt[i % sizeSalt];
						p++;
					}
					for (i = 0; i < lenPassword; i++) {
						*p = password[i % sizePassword];
						p++;
					}
				}

				MD md;
				for (;;) {
					md.start();
					md.update(D, sizeof(D));
					md.update(I, lenI);
					sl_uint8 H[MD::HashSize];
					md.finish(H);
					for (i = 1; i < iteration; i++) {
						md.start();
						md.update(H, sizeof(H));
						md.finish(H);
					}
					if (sizeof(H) >= sizeOutput) {
						Base::copyMemory(output, H, sizeOutput);
						return sl_true;
					}
					Base::copyMemory(output, H, sizeof(H));
					sizeOutput -= sizeof(H);
					output += sizeof(H);
					sl_uint8 B[MD::BlockSize];
					for (i = 0; i < sizeof(B); i++) {
						B[i] = H[i % sizeof(H)];
					}
					for (i = 0; i < lenI; i += sizeof(B)) {
						sl_uint8* I1 = I + i;
						sl_uint16 c = 1;
						for (sl_uint32 k = sizeof(B); k > 0;) {
							k--;
							c += I1[k] + B[k];
							I1[k] = (sl_uint8)c;
							c >>= 8;
						}
					}
				}
			}

			static Memory PKCS12_Decrypt(const void* data, sl_size size, const X509Algorithm& alg, const StringParam& _password)
			{
				if (!data || !size) {
					return sl_null;
				}
				sl_uint32 lenKey;
				sl_uint32 lenIV;
				sl_bool flagRC2 = sl_false;
				if (alg.algorithm.equals(OID_PKCS12_PBE_SHA1_RC4_128)) {
					// Not Supported
					return sl_null;
				} else if (alg.algorithm.equals(OID_PKCS12_PBE_SHA1_RC4_40)) {
					// Not Supported
					return sl_null;
				} else if (alg.algorithm.equals(OID_PKCS12_PBE_SHA1_3DES)) {
					lenKey = 24;
					lenIV = TripleDES::BlockSize;
				} else if (alg.algorithm.equals(OID_PKCS12_PBE_SHA1_2DES)) {
					lenKey = 16;
					lenIV = TripleDES::BlockSize;
				} else if (alg.algorithm.equals(OID_PKCS12_PBE_SHA1_RC2_128)) {
					lenKey = 16;
					lenIV = RC2::BlockSize;
					flagRC2 = sl_true;
				} else if (alg.algorithm.equals(OID_PKCS12_PBE_SHA1_RC2_40)) {
					lenKey = 5;
					lenIV = RC2::BlockSize;
					flagRC2 = sl_true;
				}

				// Uses UTF16-BE Encoding with tailing 2 zero bytes
				StringData16 password(_password);
				sl_size sizePassword = (password.getLength() + 1) << 1;
				SLIB_SCOPED_BUFFER(sl_uint8, 128, bufPassword, sizePassword)
				if (bufPassword) {
					sl_uint8* cur = bufPassword;
					sl_uint8* end = cur + sizePassword - 2;
					sl_char16* src = password.getData();
					while (cur < end) {
						MIO::writeUint16BE(cur, *src);
						src++;
						cur += 2;
					}
					*(cur++) = 0;
					*(cur++) = 0;
				} else {
					return sl_null;
				}
				
				PKCS12_PBE_Param param;
				if (!(param.load(alg.parameter))) {
					return sl_null;
				}
				sl_uint8 key[32];
				if (!(PKCS12_DeriveKey<SHA1>(bufPassword, sizePassword, param.salt.data, param.salt.length, 1, param.iteration, key, lenKey))) {
					return sl_null;
				}
				sl_uint8 iv[32];
				if (!(PKCS12_DeriveKey<SHA1>(bufPassword, sizePassword, param.salt.data, param.salt.length, 2, param.iteration, iv, lenIV))) {
					return sl_null;
				}

				if (flagRC2) {
					RC2 cipher;
					cipher.setKey(key, lenKey);
					return cipher.decrypt_CBC_PKCS7Padding(iv, data, size);
				} else {
					TripleDES cipher;
					if (lenKey == 24) {
						cipher.setKey24(key);
					} else {
						cipher.setKey16(key);
					}
					return cipher.decrypt_CBC_PKCS7Padding(iv, data, size);
				}
			}

			static sl_bool GetEllipticCurve(EllipticCurve& curve, const Asn1Element& element)
			{
				if (element.tag == SLIB_ASN1_TAG_OID) {
					Asn1ObjectIdentifier& oid = *((Asn1ObjectIdentifier*)((Asn1String*)&element));
					if (oid.equals(OID_secp112r1)) {
						curve.setId(EllipticCurveId::secp112r1);
					} else if (oid.equals(OID_secp112r2)) {
						curve.setId(EllipticCurveId::secp112r2);
					} else if (oid.equals(OID_secp128r1)) {
						curve.setId(EllipticCurveId::secp128r1);
					} else if (oid.equals(OID_secp128r2)) {
						curve.setId(EllipticCurveId::secp128r2);
					} else if (oid.equals(OID_secp160k1)) {
						curve.setId(EllipticCurveId::secp160k1);
					} else if (oid.equals(OID_secp160r1)) {
						curve.setId(EllipticCurveId::secp160r1);
					} else if (oid.equals(OID_secp160r2)) {
						curve.setId(EllipticCurveId::secp160r2);
					} else if (oid.equals(OID_secp192k1)) {
						curve.setId(EllipticCurveId::secp192k1);
					} else if (oid.equals(OID_secp224k1)) {
						curve.setId(EllipticCurveId::secp224k1);
					} else if (oid.equals(OID_secp256k1)) {
						curve.setId(EllipticCurveId::secp256k1);
					} else if (oid.equals(OID_secp384r1)) {
						curve.setId(EllipticCurveId::secp384r1);
					} else if (oid.equals(OID_secp521r1)) {
						curve.setId(EllipticCurveId::secp521r1);
					} else {
						return sl_false;
					}
					return sl_true;
				} else if (element.tag == SLIB_ASN1_TAG_SEQUENCE) {
					Asn1MemoryReader reader(element.data, element.length);
					sl_uint32 version;
					if (!(reader.readInt(version))) {
						return sl_false;
					}
					Asn1MemoryReader field;
					if (reader.readSequence(field)) {
						Asn1ObjectIdentifier type;
						if (!(field.readObjectIdentifier(type))) {
							return sl_false;
						}
						if (!(type.equals(OID_X9_62_PRIME_FIELD))) {
							return sl_false;
						}
						if (!(field.readBigInt(curve.p))) {
							return sl_false;
						}
					} else {
						return sl_false;
					}
					Asn1MemoryReader ec;
					if (reader.readSequence(ec)) {
						Asn1String _a;
						if (!(ec.readOctetString(_a))) {
							return sl_false;
						}
						curve.a = BigInt::fromBytesBE(_a.data, _a.length);
						Asn1String _b;
						if (!(ec.readOctetString(_b))) {
							return sl_false;
						}
						curve.b = BigInt::fromBytesBE(_b.data, _b.length);
					} else {
						return sl_false;
					}
					Asn1String _g;
					if (reader.readOctetString(_g)) {
						if (!(curve.G.parseBinaryFormat(_g.data, _g.length))) {
							return sl_false;
						}
					} else {
						return sl_false;
					}
					if (!(reader.readBigInt(curve.n))) {
						return sl_false;
					}
					return sl_true;
				}
				return sl_false;
			}

			class PKCS8_PrivateKey
			{
			public:
				X509Algorithm algorithm;
				Asn1String key;
				
			public:
				sl_bool load(const Asn1Element& element)
				{
					Asn1MemoryReader body;
					if (!(element.getSequence(body))) {
						return sl_false;
					}
					sl_uint32 version;
					if (!(body.readInt(version))) {
						return sl_false;
					}
					if (!(body.readObject(algorithm))) {
						return sl_false;
					}
					if (!(body.readOctetString(key))) {
						return sl_false;
					}
					return sl_true;
				}

				sl_bool getPrivateKey(PrivateKey& _out)
				{
					if (algorithm.algorithm.equals(OID_PKCS1_RSA)) {
						Asn1MemoryReader reader(key);
						Asn1MemoryReader body;
						if (reader.readSequence(body)) {
							sl_uint32 version;
							if (!(body.readInt(version))) {
								return sl_false;
							}
							if (!(body.readBigInt(_out.rsa.N))) {
								return sl_false;
							}
							if (!(body.readBigInt(_out.rsa.E))) {
								return sl_false;
							}
							if (!(body.readBigInt(_out.rsa.D))) {
								return sl_false;
							}
							body.readBigInt(_out.rsa.P);
							body.readBigInt(_out.rsa.Q);
							body.readBigInt(_out.rsa.DP);
							body.readBigInt(_out.rsa.DQ);
							body.readBigInt(_out.rsa.IQ);
							return sl_true;
						}
					} else if (algorithm.algorithm.equals(OID_X9_62_EC_PUBLIC_KEY)) {
						Asn1MemoryReader reader(key);
						Asn1MemoryReader body;
						if (reader.readSequence(body)) {
							sl_uint32 version;
							if (!(body.readInt(version))) {
								return sl_false;
							}
							Asn1String key;
							if (!(body.readOctetString(key))) {
								return sl_false;
							}
							_out.ecc.d = BigInt::fromBytesBE(key.data, key.length);
							if (_out.ecc.d.isNull()) {
								return sl_false;
							}
							Asn1Element param;
							if (!(body.readElement(param))) {
								return sl_false;
							}
							if (!(GetEllipticCurve(_out.ecc, param))) {
								return sl_false;
							}
							Asn1String pub;
							sl_uint8 nBitsRemain;
							if (body.readBitString(pub, nBitsRemain)) {
								if (nBitsRemain) {
									return sl_false;
								}
								return _out.ecc.Q.parseBinaryFormat(_out.ecc, pub.data, pub.length);
							} else {
								_out.ecc.Q = _out.ecc.multiplyG(_out.ecc.d);
							}
							return sl_true;
						}
					}
					return sl_false;
				}

			};

			static sl_bool PKCS12_ParseBag(PKCS12& p12, PKCS12_SafeBag& bag, const StringParam& password)
			{
				if (bag.type.equals(OID_PKCS12_KEY_BAG)) {
					PKCS8_PrivateKey p8;
					if (p8.load(bag.content)) {
						return p8.getPrivateKey(p12.key);
					}
				} else if (bag.type.equals(OID_PKCS12_PKCS8_SHROUNDED_KEY_BAG)) {
					X509Signature sig;
					if (sig.load(bag.content)) {
						Memory dec = PKCS12_Decrypt(sig.digest.data, sig.digest.length, sig.algorithm, password);
						if (dec.isNotNull()) {
							PKCS8_PrivateKey p8;
							Asn1MemoryReader reader(dec);
							if (reader.readObject(p8)) {
								return p8.getPrivateKey(p12.key);
							}
						}
					}
					return sl_false;
				} else if (bag.type.equals(OID_PKCS12_CERTIFICATE_BAG)) {
					PKCS12_Bag value;
					if (value.load(bag.content)) {
						if (value.type.equals(OID_PKCS9_X509_CERTIFICATE)) {
							if (value.content.length) {
								return p12.certificates.add_NoLock(Memory::create(value.content.data, value.content.length));
							}
						}
					}
					return sl_false;
				}
				return sl_true;
			}

			static sl_bool PKCS12_ParseBags(PKCS12& p12, PKCS12_SafeBag* bags, sl_size nBags, const StringParam& password)
			{
				for (sl_size i = 0; i < nBags; i++) {
					if (!(PKCS12_ParseBag(p12, bags[i], password))) {
						return sl_false;
					}
				}
				return sl_true;
			}

			static List<PKCS12_SafeBag> PKCS12_Unpack_PKCS7_EncryptedData(PKCS7& p7, const StringParam& password, Memory& outDecryptedData)
			{
				Asn1MemoryReader reader(p7.content);
				Asn1MemoryReader body;
				if (!(reader.readSequence(body))) {
					return sl_null;
				}
				sl_uint32 version;
				if (!(body.readInt(version))) {
					return sl_null;
				}
				Asn1MemoryReader contentInfo;
				if (!(body.readSequence(contentInfo))) {
					return sl_null;
				}
				Asn1ObjectIdentifier contentType;
				if (!(contentInfo.readObjectIdentifier(contentType))) {
					return sl_null;
				}
				X509Algorithm algorithm;
				if (!(contentInfo.readObject(algorithm))) {
					return sl_null;
				}
				Asn1Element encData;
				if (!(contentInfo.readElement(encData))) {
					return sl_null;
				}
				outDecryptedData = PKCS12_Decrypt(encData.data, encData.length, algorithm, password);
				if (outDecryptedData.isNull()) {
					return sl_null;
				}
				return PKCS12_UnpackSafeBags(outDecryptedData.getData(), outDecryptedData.getSize());
			}

			static sl_bool PKCS12_Load(PKCS12& p12, const void* content, sl_size size, const StringParam& password)
			{
				Asn1MemoryReader reader(content, size);
				Asn1MemoryReader p12Body;
				if (!(reader.readSequence(p12Body))) {
					return sl_false;
				}
				sl_uint32 version;
				if (!(p12Body.readInt(version))) {
					return sl_false;
				}
				PKCS7 p7AuthSafes;
				if (!(p12Body.readObject(p7AuthSafes))) {
					return sl_false;
				}
				ListElements<PKCS7> authSafes(PKCS12_UnpackAuthSafes(p7AuthSafes));
				if (!(authSafes.count)) {
					return sl_false;
				}
				for (sl_size i = 0; i < authSafes.count; i++) {
					PKCS7& p7 = authSafes[i];
					if (p7.type.equals(OID_PKCS7_DATA)) {
						ListElements<PKCS12_SafeBag> bags(PKCS12_Unpack_PKCS7_Data(p7));
						if (!(PKCS12_ParseBags(p12, bags.data, bags.count, password))) {
							return sl_false;
						}
					} else if (p7.type.equals(OID_PKCS7_ENCRYPTED_DATA)) {
						Memory mem;
						ListElements<PKCS12_SafeBag> bags(PKCS12_Unpack_PKCS7_EncryptedData(p7, password, mem));
						if (!(PKCS12_ParseBags(p12, bags.data, bags.count, password))) {
							return sl_false;
						}
					}
				}
				return sl_true;
			}

			static sl_bool X509_GetNameKey(const Asn1ObjectIdentifier& id, X509SubjectKey& key)
			{
				if (id.equals(OID_X509_COMMON_NAME)) {
					key = X509SubjectKey::CommonName;
					return sl_true;
				}
				if (id.equals(OID_X509_COUNTRY_NAME)) {
					key = X509SubjectKey::CountryName;
					return sl_true;
				}
				if (id.equals(OID_X509_LOCALITY_NAME)) {
					key = X509SubjectKey::LocalityName;
					return sl_true;
				}
				if (id.equals(OID_X509_STATE_OR_PROVINCE_NAME)) {
					key = X509SubjectKey::StateOrProvinceName;
					return sl_true;
				}
				if (id.equals(OID_X509_ORGANIZATION_NAME)) {
					key = X509SubjectKey::OrganizationName;
					return sl_true;
				}
				if (id.equals(OID_X509_ORGANIZATION_UNIT_NAME)) {
					key = X509SubjectKey::OrganizationalUnitName;
					return sl_true;
				}
				if (id.equals(OID_X509_TITLE)) {
					key = X509SubjectKey::Title;
					return sl_true;
				}
				if (id.equals(OID_X509_SURNAME)) {
					key = X509SubjectKey::Surname;
					return sl_true;
				}
				if (id.equals(OID_X509_SEARCH_GUIDE)) {
					key = X509SubjectKey::SearchGuide;
					return sl_true;
				}
				if (id.equals(OID_X509_DESCRIPTION)) {
					key = X509SubjectKey::Description;
					return sl_true;
				}
				if (id.equals(OID_X509_STREET_ADDRESS)) {
					key = X509SubjectKey::StreetAddress;
					return sl_true;
				}
				if (id.equals(OID_X509_BUSINESS_CATEGORY)) {
					key = X509SubjectKey::BusinessCategory;
					return sl_true;
				}
				if (id.equals(OID_X509_POSTAL_ADDRESS)) {
					key = X509SubjectKey::PostalAddress;
					return sl_true;
				}
				if (id.equals(OID_X509_POSTAL_CODE)) {
					key = X509SubjectKey::PostalCode;
					return sl_true;
				}
				if (id.equals(OID_X509_POSTAL_OFFICE_BOX)) {
					key = X509SubjectKey::PostOfficeBox;
					return sl_true;
				}
				if (id.equals(OID_X509_TELEPHONE_NUMBER)) {
					key = X509SubjectKey::TelephoneNumber;
					return sl_true;
				}
				return sl_false;
			}

			template <class KEY>
			static HashMap<KEY, String> X509_LoadName(Asn1MemoryReader& reader)
			{
				Asn1MemoryReader seq;
				if (!(reader.readSequence(seq))) {
					return sl_null;
				}
				HashMap<KEY, String> ret;
				for (;;) {
					Asn1MemoryReader set;
					if (!(seq.readSet(set))) {
						break;
					}
					Asn1MemoryReader body;
					if (set.readSequence(body)) {
						Asn1ObjectIdentifier id;
						if (body.readObjectIdentifier(id)) {
							Asn1String value;
							if (body.readUtf8String(value)) {
								KEY key;
								if (X509_GetNameKey(id, key)) {
									ret.add_NoLock(key, String::fromUtf8(value.data, value.length));
								}
							}
						}
					}
				}
				return ret;
			}

			class X509_PublicKey
			{
			public:
				X509Algorithm algorithm;
				Asn1String key;

			public:
				sl_bool load(const Asn1Element& element)
				{
					Asn1MemoryReader body;
					if (!(element.getSequence(body))) {
						return sl_false;
					}
					if (!(body.readObject(algorithm))) {
						return sl_false;
					}
					sl_uint8 nBitsRemain;
					if (!(body.readBitString(key, nBitsRemain))) {
						return sl_false;
					}
					if (nBitsRemain) {
						return sl_false;
					}
					return sl_true;
				}

				sl_bool getPublicKey(PublicKey& _out)
				{
					if (algorithm.algorithm.equals(OID_PKCS1_RSA)) {
						Asn1MemoryReader reader(key);
						Asn1MemoryReader body;
						if (reader.readSequence(body)) {
							if (!(body.readBigInt(_out.rsa.N))) {
								return sl_false;
							}
							if (!(body.readBigInt(_out.rsa.E))) {
								return sl_false;
							}
							return sl_true;
						}
					} else if (algorithm.algorithm.equals(OID_X9_62_EC_PUBLIC_KEY)) {
						if (!(GetEllipticCurve(_out.ecc, algorithm.parameter))) {
							return sl_false;
						}
						return _out.ecc.Q.parseBinaryFormat(_out.ecc, key.data, key.length);
					}
					return sl_false;
				}

			};

			static X509SignatureAlgorithm GetSignatureAlgorithm(const Asn1ObjectIdentifier& id)
			{
				if (id.equals(OID_PKCS1_SHA224_WITH_RSA)) {
					return X509SignatureAlgorithm::Sha224WithRSA;
				}
				if (id.equals(OID_PKCS1_SHA256_WITH_RSA)) {
					return X509SignatureAlgorithm::Sha256WithRSA;
				}
				if (id.equals(OID_PKCS1_SHA384_WITH_RSA)) {
					return X509SignatureAlgorithm::Sha384WithRSA;
				}
				if (id.equals(OID_PKCS1_SHA512_WITH_RSA)) {
					return X509SignatureAlgorithm::Sha512WithRSA;
				}
				if (id.equals(OID_ECDSA_WITH_SHA224)) {
					return X509SignatureAlgorithm::Sha224WithECDSA;
				}
				if (id.equals(OID_ECDSA_WITH_SHA256)) {
					return X509SignatureAlgorithm::Sha256WithECDSA;
				}
				if (id.equals(OID_ECDSA_WITH_SHA384)) {
					return X509SignatureAlgorithm::Sha384WithECDSA;
				}
				if (id.equals(OID_ECDSA_WITH_SHA512)) {
					return X509SignatureAlgorithm::Sha512WithECDSA;
				}
				return X509SignatureAlgorithm::Unknown;
			}

		}
	}

	using namespace priv::cert;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PublicKey)

	PublicKey::PublicKey()
	{
	}

	sl_bool PublicKey::isRSA() const noexcept
	{
		return rsa.isDefined();
	}

	sl_bool PublicKey::isECC() const noexcept
	{
		return ecc.isDefined();
	}

	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PrivateKey)

	PrivateKey::PrivateKey()
	{
	}

	sl_bool PrivateKey::isRSA() const noexcept
	{
		return rsa.isDefined();
	}

	sl_bool PrivateKey::isECC() const noexcept
	{
		return ecc.isDefined();
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(X509CertificatePolicy)

	X509CertificatePolicy::X509CertificatePolicy()
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(X509AuthorityInformation)

	X509AuthorityInformation::X509AuthorityInformation()
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(X509)

	X509::X509() noexcept: version(2), flagEndEntity(sl_true), signatureAlgorithm(X509SignatureAlgorithm::Unknown)
	{
	}

	sl_bool X509::load(const void* input, sl_size size)
	{
		Asn1MemoryReader reader(input, size);
		Asn1MemoryReader root;
		if (!(reader.readSequence(root))) {
			return sl_false;
		}
		Asn1String content;
		content.data = root.current;
		Asn1MemoryReader body;
		if (root.readSequence(body)) {
			content.length = (sl_uint32)(body.end - content.data);
			body.readInt(version);
			if (!(body.readBigInt(serialNumber))) {
				return sl_false;
			}
			X509Algorithm signatureAlgorithm;
			if (!(body.readObject(signatureAlgorithm))) {
				return sl_false;
			}
			issuer = X509_LoadName<X509SubjectKey>(body);
			Asn1MemoryReader validity;
			if (body.readSequence(validity)) {
				if (!(validity.readTime(validFrom))) {
					return sl_false;
				}
				if (!(validity.readTime(validTo))) {
					return sl_false;
				}
			} else {
				return sl_false;
			}
			subject = X509_LoadName<X509SubjectKey>(body);
			X509_PublicKey pub;
			if (body.readObject(pub)) {
				if (!(pub.getPublicKey(key))) {
					return sl_false;
				}
			} else {
				return sl_false;
			}
		} else {
			return sl_false;
		}
		X509Algorithm sigAlg;
		if (root.readObject(sigAlg)) {
			signatureAlgorithm = GetSignatureAlgorithm(sigAlg.algorithm);
			if (signatureAlgorithm == X509SignatureAlgorithm::Unknown) {
				return sl_false;
			}
		} else {
			return sl_false;
		}
		Asn1String sig;
		sl_uint8 nSigBitsRemain;
		if (root.readBitString(sig, nSigBitsRemain)) {
			if (nSigBitsRemain) {
				return sl_false;
			}
			signature = Memory::create(sig.data, sig.length);
		} else {
			return sl_false;
		}
		switch (signatureAlgorithm) {
			case X509SignatureAlgorithm::Sha224WithRSA:
			case X509SignatureAlgorithm::Sha224WithECDSA:
				contentHash = SHA224::hash(content.data, content.length);
				break;
			case X509SignatureAlgorithm::Sha256WithRSA:
			case X509SignatureAlgorithm::Sha256WithECDSA:
				contentHash = SHA256::hash(content.data, content.length);
				break;
			case X509SignatureAlgorithm::Sha384WithRSA:
			case X509SignatureAlgorithm::Sha384WithECDSA:
				contentHash = SHA384::hash(content.data, content.length);
				break;
			case X509SignatureAlgorithm::Sha512WithRSA:
			case X509SignatureAlgorithm::Sha512WithECDSA:
				contentHash = SHA512::hash(content.data, content.length);
				break;
			default:
				return sl_false;
		}
		return sl_true;
	}

	sl_bool X509::load(const Memory& mem)
	{
		return load(mem.getData(), mem.getSize());
	}

	sl_bool X509::load(const StringParam& filePath)
	{
		return load(File::readAllBytes(filePath));
	}

	sl_bool X509::verify(const PublicKey& issuerKey)
	{
		switch (signatureAlgorithm) {
			case X509SignatureAlgorithm::Sha224WithRSA:
			case X509SignatureAlgorithm::Sha256WithRSA:
			case X509SignatureAlgorithm::Sha384WithRSA:
			case X509SignatureAlgorithm::Sha512WithRSA:
				if (issuerKey.rsa.isDefined()) {
					sl_bool flagSign;
					Memory dec = RSA::decryptPublic_pkcs1_v15(issuerKey.rsa, signature.getData(), signature.getSize(), &flagSign);
					Asn1MemoryReader reader(dec);
					X509Signature sig;
					if (reader.readObject(sig)) {
						sl_size nHash = contentHash.getSize();
						if (sig.digest.length == nHash) {
							return Base::equalsMemory(contentHash.getData(), sig.digest.data, nHash);
						}
					}
				}
				break;
			case X509SignatureAlgorithm::Sha224WithECDSA:
			case X509SignatureAlgorithm::Sha256WithECDSA:
			case X509SignatureAlgorithm::Sha384WithECDSA:
			case X509SignatureAlgorithm::Sha512WithECDSA:
				if (issuerKey.ecc.isDefined()) {
					Asn1MemoryReader reader(signature);
					Asn1MemoryReader body;
					if (reader.readSequence(body)) {
						ECDSA_Signature sig;
						if (!(body.readBigInt(sig.r))) {
							return sl_false;
						}
						if (!(body.readBigInt(sig.s))) {
							return sl_false;
						}
						return ECDSA::verify(issuerKey.ecc, issuerKey.ecc, contentHash.getData(), contentHash.getSize(), sig);
					}
				}
				break;
			default:
				break;
		}
		return sl_false;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PKCS12)

	PKCS12::PKCS12() noexcept
	{
	}

	sl_bool PKCS12::load(const void* content, sl_size size, const StringParam& password)
	{
		return PKCS12_Load(*this, content, size, password);
	}

	sl_bool PKCS12::load(const Memory& mem, const StringParam& password)
	{
		return load(mem.getData(), mem.getSize(), password);
	}

	sl_bool PKCS12::load(const StringParam& filePath, const StringParam& password)
	{
		return load(File::readAllBytes(filePath), password);
	}

}
