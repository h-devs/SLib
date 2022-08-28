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
#include "slib/crypto/pkcs8.h"
#include "slib/crypto/pem.h"

#include "slib/core/file.h"
#include "slib/crypto/asn1.h"
#include "slib/crypto/sha1.h"
#include "slib/crypto/sha2.h"
#include "slib/crypto/des.h"
#include "slib/crypto/rc2.h"
#include "slib/crypto/base64.h"

#define PKCS12_DEFAULT_ITERATION 51200
#define PKCS12_DEFAULT_SALT_SIZE 20

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
#define OID_PKCS9_FRIENDLY_NAME OID_PKCS9 "\x14" // 20
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
#define OID_CE OID_X500 "\x1D" // 29
#define OID_SUBJECT_KEY_ID OID_CE "\x0E" // 14
#define OID_AUTHORITY_KEY_ID OID_CE "\x23" // 35
#define OID_IDENTIFIED_ORGANIZATION "\x2B" // ISO(1), 3
#define OID_CERTICOM_ARC OID_IDENTIFIED_ORGANIZATION "\x81\x04" // 132
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
				Asn1MemoryReader parameter;

			public:
				sl_bool load(Asn1MemoryReader& reader)
				{
					Asn1MemoryReader body;
					if (reader.readSequence(body)) {
						if (body.readObjectIdentifier(algorithm)) {
							parameter = body;
							return sl_true;
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
				sl_bool load(Asn1MemoryReader& reader)
				{
					Asn1MemoryReader body;
					if (reader.readSequence(body)) {
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
				Asn1MemoryReader content;

			public:
				sl_bool load(Asn1MemoryReader& reader)
				{
					Asn1MemoryReader body;
					if (reader.readSequence(body)) {
						if (!(body.readObjectIdentifier(type))) {
							return sl_false;
						}
						content = body;
						return sl_true;
					}
					return sl_false;
				}

				sl_bool getData(Asn1String& _out)
				{
					return content.readOctetString(_out, SLIB_ASN1_TAG_CONTEXT(0));
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
				if (reader.readSequence(reader)) {
					List<PKCS7> ret;
					PKCS7 item;
					while (reader.readObject(item)) {
						ret.add_NoLock(Move(item));
					}
					return ret;
				}
				return sl_null;
			}

			class PKCS12_Bag
			{
			public:
				Asn1ObjectIdentifier type;
				Asn1String content;

			public:
				sl_bool load(Asn1MemoryReader& reader)
				{
					Asn1MemoryReader body;
					if (reader.readSequence(body)) {
						if (!(body.readObjectIdentifier(type))) {
							return sl_false;
						}
						if (!(body.readOctetString(content, SLIB_ASN1_TAG_CONTEXT(0)))) {
							return sl_false;
						}
						return sl_true;
					}
					return sl_false;
				}

			};

			class PKCS12_SafeBag
			{
			public:
				Asn1ObjectIdentifier type;
				Asn1MemoryReader content;
				
			public:
				sl_bool load(Asn1MemoryReader& reader)
				{
					Asn1MemoryReader body;
					if (reader.readSequence(body)) {
						if (!(body.readObjectIdentifier(type))) {
							return sl_false;
						}
						content = body;
						return sl_true;
					}
					return sl_false;
				}

			};

			class PKCS12_PBE_Param
			{
			public:
				Asn1String salt;
				sl_uint64 iteration;

			public:
				sl_bool load(Asn1MemoryReader& reader)
				{
					Asn1MemoryReader body;
					if (reader.readSequence(body)) {
						if (!(body.readOctetString(salt))) {
							return sl_false;
						}
						if (!(body.readInt(iteration))) {
							return sl_false;
						}
						return sl_true;
					}
					return sl_false;
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

			template <class MD>
			static sl_bool PKCS12_DeriveKeyAndIV(
				const StringParam& _password,
				const void* salt, sl_size sizeSalt,
				sl_uint64 iteration,
				void* key, sl_size sizeKey,
				void* iv, sl_size sizeIV)
			{
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
					return sl_false;
				}
				if (!(PKCS12_DeriveKey<MD>(bufPassword, sizePassword, salt, sizeSalt, 1, iteration, key, sizeKey))) {
					return sl_false;
				}
				if (!(PKCS12_DeriveKey<MD>(bufPassword, sizePassword, salt, sizeSalt, 2, iteration, iv, sizeIV))) {
					return sl_false;
				}
				return sl_true;
			}

			static Memory PKCS12_Decrypt(const void* data, sl_size size, const StringParam& password, const X509Algorithm& alg)
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
				PKCS12_PBE_Param param;
				if (!(alg.parameter.peekObject(param))) {
					return sl_null;
				}
				sl_uint8 key[32];
				sl_uint8 iv[32];
				if (!(PKCS12_DeriveKeyAndIV<SHA1>(password, param.salt.data, param.salt.length, param.iteration, key, lenKey, iv, lenIV))) {
					return sl_null;
				}
				if (flagRC2) {
					RC2 cipher;
					if (!(cipher.setKey(key, lenKey))) {
						return sl_null;
					}
					return cipher.decrypt_CBC_PKCS7Padding(iv, data, size);
				} else {
					TripleDES cipher;
					if (!(cipher.setKey(key, lenKey))) {
						return sl_null;
					}
					return cipher.decrypt_CBC_PKCS7Padding(iv, data, size);
				}
			}

			template <class CIPHER, sl_uint32 KeySize, class MD>
			static Memory PKCS12_Encrypt(const void* data, sl_size sizeData, const StringParam& password, const void* salt, sl_size sizeSalt, sl_uint64 iteration)
			{
				if (!data || !sizeData) {
					return sl_null;
				}
				sl_uint8 key[KeySize];
				sl_uint8 iv[CIPHER::BlockSize];
				if (!(PKCS12_DeriveKeyAndIV<MD>(password, salt, sizeSalt, iteration, key, sizeof(key), iv, sizeof(iv)))) {
					return sl_null;
				}
				CIPHER cipher;
				if (!(cipher.setKey(key, sizeof(key)))) {
					return sl_null;
				}
				return cipher.encrypt_CBC_PKCS7Padding(iv, data, sizeData);
			}

			static sl_bool Load_RSAPrivateKey(RSAPrivateKey& _out, const void* content, sl_size size)
			{
				Asn1MemoryReader reader(content, size);
				Asn1MemoryReader body;
				if (reader.readSequence(body)) {
					sl_uint32 version;
					if (!(body.readInt(version))) {
						return sl_false;
					}
					if (!(body.readBigInt(_out.N))) {
						return sl_false;
					}
					if (!(body.readBigInt(_out.E))) {
						return sl_false;
					}
					if (!(body.readBigInt(_out.D))) {
						return sl_false;
					}
					body.readBigInt(_out.P);
					body.readBigInt(_out.Q);
					body.readBigInt(_out.DP);
					body.readBigInt(_out.DQ);
					body.readBigInt(_out.IQ);
					return sl_true;
				}
				return sl_false;
			}

			static Memory Save_RSAPrivateKey(const RSAPrivateKey& _in)
			{
				Asn1MemoryWriter writer;
				// Version
				if (!(writer.writeInt(0))) {
					return sl_null;
				}
				if (!(writer.writeBigInt(_in.N))) {
					return sl_null;
				}
				if (!(writer.writeBigInt(_in.E))) {
					return sl_null;
				}
				if (!(writer.writeBigInt(_in.D))) {
					return sl_null;
				}
				if (_in.P.isNotNull() && _in.Q.isNotNull() && _in.DP.isNotNull() && _in.DQ.isNotNull() && _in.IQ.isNotNull()) {
					if (!(writer.writeBigInt(_in.P))) {
						return sl_null;
					}
					if (!(writer.writeBigInt(_in.Q))) {
						return sl_null;
					}
					if (!(writer.writeBigInt(_in.DP))) {
						return sl_null;
					}
					if (!(writer.writeBigInt(_in.DQ))) {
						return sl_null;
					}
					if (!(writer.writeBigInt(_in.IQ))) {
						return sl_null;
					}
				}
				return Asn1::serializeElement(SLIB_ASN1_TAG_SEQUENCE, writer);
			}

			static sl_bool Load_RSAPublicKey(RSAPublicKey& _out, const void* content, sl_size size)
			{
				Asn1MemoryReader reader(content, size);
				Asn1MemoryReader body;
				if (reader.readSequence(body)) {
					if (!(body.readBigInt(_out.N))) {
						return sl_false;
					}
					if (!(body.readBigInt(_out.E))) {
						return sl_false;
					}
					return sl_true;
				}
				return sl_false;
			}

			static Memory Save_RSAPublicKey(const RSAPublicKey& _in)
			{
				Asn1MemoryWriter writer;
				if (!(writer.writeBigInt(_in.N))) {
					return sl_null;
				}
				if (!(writer.writeBigInt(_in.E))) {
					return sl_null;
				}
				return Asn1::serializeElement(SLIB_ASN1_TAG_SEQUENCE, writer);
			}

			static sl_bool Load_EllipticCurve(EllipticCurve& curve, Asn1MemoryReader& input)
			{
				Asn1ObjectIdentifier oid;
				if (input.readObjectIdentifier(oid)) {
					if (oid.equals(OID_secp112r1)) {
						curve.setCurveId(EllipticCurveId::secp112r1);
					} else if (oid.equals(OID_secp112r2)) {
						curve.setCurveId(EllipticCurveId::secp112r2);
					} else if (oid.equals(OID_secp128r1)) {
						curve.setCurveId(EllipticCurveId::secp128r1);
					} else if (oid.equals(OID_secp128r2)) {
						curve.setCurveId(EllipticCurveId::secp128r2);
					} else if (oid.equals(OID_secp160k1)) {
						curve.setCurveId(EllipticCurveId::secp160k1);
					} else if (oid.equals(OID_secp160r1)) {
						curve.setCurveId(EllipticCurveId::secp160r1);
					} else if (oid.equals(OID_secp160r2)) {
						curve.setCurveId(EllipticCurveId::secp160r2);
					} else if (oid.equals(OID_secp192k1)) {
						curve.setCurveId(EllipticCurveId::secp192k1);
					} else if (oid.equals(OID_secp224k1)) {
						curve.setCurveId(EllipticCurveId::secp224k1);
					} else if (oid.equals(OID_secp256k1)) {
						curve.setCurveId(EllipticCurveId::secp256k1);
					} else if (oid.equals(OID_secp384r1)) {
						curve.setCurveId(EllipticCurveId::secp384r1);
					} else if (oid.equals(OID_secp521r1)) {
						curve.setCurveId(EllipticCurveId::secp521r1);
					} else {
						return sl_false;
					}
					return sl_true;
				}
				Asn1MemoryReader reader;
				if (input.readSequence(reader)) {
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
						if (!(curve.G.parseBinaryFormat(curve, MemoryView(_g.data, _g.length)))) {
							return sl_false;
						}
					} else {
						return sl_false;
					}
					if (!(reader.readBigInt(curve.n))) {
						return sl_false;
					}
					if (!(reader.readInt(curve.h))) {
						return sl_false;
					}
					return sl_true;
				}
				return sl_false;
			}

			static Memory Save_EllipticCurve(const EllipticCurve& curve)
			{
				switch (curve.id) {
					case EllipticCurveId::secp112r1:
						return Asn1::serializeElement(SLIB_ASN1_TAG_OID, OID_secp112r1);
					case EllipticCurveId::secp112r2:
						return Asn1::serializeElement(SLIB_ASN1_TAG_OID, OID_secp112r2);
					case EllipticCurveId::secp128r1:
						return Asn1::serializeElement(SLIB_ASN1_TAG_OID, OID_secp128r1);
					case EllipticCurveId::secp128r2:
						return Asn1::serializeElement(SLIB_ASN1_TAG_OID, OID_secp128r2);
					case EllipticCurveId::secp160k1:
						return Asn1::serializeElement(SLIB_ASN1_TAG_OID, OID_secp160k1);
					case EllipticCurveId::secp160r1:
						return Asn1::serializeElement(SLIB_ASN1_TAG_OID, OID_secp160r1);
					case EllipticCurveId::secp160r2:
						return Asn1::serializeElement(SLIB_ASN1_TAG_OID, OID_secp160r2);
					case EllipticCurveId::secp192k1:
						return Asn1::serializeElement(SLIB_ASN1_TAG_OID, OID_secp192k1);
					case EllipticCurveId::secp224k1:
						return Asn1::serializeElement(SLIB_ASN1_TAG_OID, OID_secp224k1);
					case EllipticCurveId::secp256k1:
						return Asn1::serializeElement(SLIB_ASN1_TAG_OID, OID_secp256k1);
					case EllipticCurveId::secp384r1:
						return Asn1::serializeElement(SLIB_ASN1_TAG_OID, OID_secp384r1);
					case EllipticCurveId::secp521r1:
						return Asn1::serializeElement(SLIB_ASN1_TAG_OID, OID_secp521r1);
					default:
						break;
				}

				Asn1MemoryWriter writer;
				// Version
				if (!(writer.writeInt(1))) {
					return sl_null;
				}
				{
					Asn1MemoryWriter field;
					if (!(field.writeObjectIdentifier(OID_X9_62_PRIME_FIELD))) {
						return sl_null;
					}
					if (!(field.writeBigInt(curve.p))) {
						return sl_null;
					}
					if (!(writer.writeSequence(field))) {
						return sl_null;
					}
				}
				{
					Asn1MemoryWriter ec;
					if (!(ec.writeOctetString(curve.a.getBytesBE(sl_true)))) {
						return sl_null;
					}
					if (!(ec.writeOctetString(curve.b.getBytesBE(sl_true)))) {
						return sl_null;
					}
					if (!(writer.writeSequence(ec))) {
						return sl_null;
					}
				}
				if (!(writer.writeOctetString(curve.G.toCompressedFormat(curve)))) {
					return sl_null;
				}
				if (!(writer.writeBigInt(curve.n))) {
					return sl_null;
				}
				return Asn1::serializeElement(SLIB_ASN1_TAG_SEQUENCE, writer);
			}

			static sl_bool Load_ECPrivateKey(ECPrivateKeyWithCurve& _out, const void* content, sl_size size)
			{
				Asn1MemoryReader reader(content, size);
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
					_out.d = BigInt::fromBytesBE(key.data, key.length);
					if (_out.d.isNull()) {
						return sl_false;
					}
					Asn1MemoryReader param;
					if (body.readElement(SLIB_ASN1_TAG_CONTEXT(0), param)) {
						Load_EllipticCurve(_out, param);
					}
					Asn1String pub;
					sl_uint8 nBitsRemain;
					if (body.readBitString(pub, nBitsRemain, SLIB_ASN1_TAG_CONTEXT(1))) {
						if (nBitsRemain) {
							return sl_false;
						}
						_out.Q.parseBinaryFormat(_out, MemoryView(pub.data, pub.length));
					} else {
						_out.Q = _out.multiplyG(_out.d);
					}
					return _out.isDefined();
				}
				return sl_false;
			}

			static Memory Save_ECPrivateKey(const ECPrivateKeyWithCurve& _in)
			{
				Asn1MemoryWriter writer;
				// Version
				if (!(writer.writeInt(1))) {
					return sl_null;
				}
				if (!(writer.writeOctetString(_in.d.getBytesBE(sl_true)))) {
					return sl_null;
				}
				Memory curve = Save_EllipticCurve(_in);
				if (curve.isNull()) {
					return sl_null;
				}
				if (!(writer.writeElement(SLIB_ASN1_TAG_CONTEXT(0), curve))) {
					return sl_null;
				}
				if (!(writer.writeBitString(_in.Q.toCompressedFormat(_in), SLIB_ASN1_TAG_CONTEXT(1)))) {
					return sl_null;
				}
				return Asn1::serializeElement(SLIB_ASN1_TAG_SEQUENCE, writer);
			}

			class PKCS8_PrivateKey
			{
			public:
				X509Algorithm algorithm;
				Asn1String key;
				
			public:
				sl_bool load(Asn1MemoryReader& reader)
				{
					Asn1MemoryReader body;
					if (reader.readSequence(body)) {
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
					return sl_false;
				}

				sl_bool getPrivateKey(PrivateKey& _out)
				{
					if (algorithm.algorithm.equals(OID_PKCS1_RSA)) {
						return Load_RSAPrivateKey(_out.rsa, key.data, key.length);
					} else if (algorithm.algorithm.equals(OID_X9_62_EC_PUBLIC_KEY)) {
						Asn1MemoryReader reader(algorithm.parameter);
						Load_EllipticCurve(_out.ecc, reader);
						return Load_ECPrivateKey(_out.ecc, key.data, key.length);
					}
					return sl_false;
				}

				static Memory savePrivateKey(const PrivateKey& _in)
				{
					Asn1MemoryWriter writer;
					// Version
					if (!(writer.writeInt(0))) {
						return sl_null;
					}
					Asn1MemoryWriter algorithm;
					if (_in.isRSA()) {
						if (!(algorithm.writeObjectIdentifier(OID_PKCS1_RSA))) {
							return sl_null;
						}
						if (!(writer.writeSequence(algorithm))) {
							return sl_null;
						}
						Memory key = Save_RSAPrivateKey(_in.rsa);
						if (key.isNull()) {
							return sl_null;
						}
						if (!(writer.writeOctetString(key))) {
							return sl_null;
						}
					} else if (_in.isECC()) {
						if (!(algorithm.writeObjectIdentifier(OID_X9_62_EC_PUBLIC_KEY))) {
							return sl_null;
						}
						Memory param = Save_EllipticCurve(_in.ecc);
						if (param.isNull()) {
							return sl_null;
						}
						if (!(algorithm.writeBytes(param))) {
							return sl_null;
						}
						if (!(writer.writeSequence(algorithm))) {
							return sl_null;
						}
						Memory key = Save_ECPrivateKey(_in.ecc);
						if (key.isNull()) {
							return sl_null;
						}
						if (!(writer.writeOctetString(key))) {
							return sl_null;
						}
					} else {
						return sl_null;
					}
					return Asn1::serializeElement(SLIB_ASN1_TAG_SEQUENCE, writer);
				}

			};

			static void PKCS12_LoadBagFriendlyName(String16& _out, PKCS12_SafeBag& bag)
			{
				Asn1MemoryReader attrs;
				if (bag.content.readSet(attrs)) {
					// X.509 Attribute
					Asn1MemoryReader attr;
					while (attrs.readSequence(attr)) {
						Asn1ObjectIdentifier oid;
						if (attr.readObjectIdentifier(oid)) {
							if (oid.equals(OID_PKCS9_FRIENDLY_NAME)) {
								Asn1MemoryReader value;
								if (attr.readSet(value)) {
									value.readBmpString(_out);
								}
								return;
							}
						}
					}
				}
			}

			static Memory PKCS12_SaveBagFriendlyName(const String16& friendlyName)
			{
				if (friendlyName.isEmpty()) {
					return sl_null;
				}
				Asn1MemoryWriter attr;
				if (!(attr.writeObjectIdentifier(OID_PKCS9_FRIENDLY_NAME))) {
					return sl_null;
				}
				if (!(attr.writeBmpString(friendlyName, SLIB_ASN1_TAG_SET))) {
					return sl_null;
				}
				Asn1MemoryWriter attrs;
				if (!(attrs.writeSequence(attr))) {
					return sl_null;
				}
				return Asn1::serializeElement(SLIB_ASN1_TAG_SET, attrs);
			}

			static sl_bool PKCS12_ParseBag(PKCS12& p12, PKCS12_SafeBag& bag, const StringParam& password)
			{
				if (bag.type.equals(OID_PKCS12_KEY_BAG)) {
					PKCS8_PrivateKey p8;
					if (!(bag.content.readObject(p8, SLIB_ASN1_TAG_CONTEXT(0)))) {
						return sl_false;
					}
					if (!(p8.getPrivateKey(p12.key))) {
						return sl_false;
					}
					PKCS12_LoadBagFriendlyName(p12.friendlyName, bag);
				} else if (bag.type.equals(OID_PKCS12_PKCS8_SHROUNDED_KEY_BAG)) {
					X509Signature sig;
					if (!(bag.content.readObject(sig, SLIB_ASN1_TAG_CONTEXT(0)))) {
						return sl_false;
					}
					Memory dec = PKCS12_Decrypt(sig.digest.data, sig.digest.length, password, sig.algorithm);
					if (dec.isNull()) {
						return sl_false;
					}
					PKCS8_PrivateKey p8;
					Asn1MemoryReader reader(dec);
					if (!(reader.readObject(p8))) {
						return sl_false;
					}
					if (!(p8.getPrivateKey(p12.key))) {
						return sl_false;
					}
					PKCS12_LoadBagFriendlyName(p12.friendlyName, bag);
				} else if (bag.type.equals(OID_PKCS12_CERTIFICATE_BAG)) {
					PKCS12_Bag value;
					if (!(bag.content.readObject(value, SLIB_ASN1_TAG_CONTEXT(0)))) {
						return sl_false;
					}
					if (!(value.type.equals(OID_PKCS9_X509_CERTIFICATE))) {
						return sl_false;
					}
					if (!(value.content.length)) {
						return sl_false;
					}
					if (!(p12.certificates.add_NoLock(Memory::create(value.content.data, value.content.length)))) {
						return sl_false;
					}
					String16 friendlyName;
					PKCS12_LoadBagFriendlyName(friendlyName, bag);
					if (!(p12.friendlyNames.add_NoLock(Move(friendlyName)))) {
						return sl_false;
					}
				}
				return sl_true;
			}

			static sl_bool PKCS12_ParseBags(PKCS12& p12, const void* content, sl_size size, const StringParam& password)
			{
				Asn1MemoryReader reader(content, size);
				if (reader.readSequence(reader)) {
					PKCS12_SafeBag item;
					while (reader.readObject(item)) {
						if (!(PKCS12_ParseBag(p12, item, password))) {
							return sl_false;
						}
					}
					return sl_true;
				}
				return sl_false;
			}

			static Memory PKCS12_Decrypt_PKCS7(PKCS7& p7, const StringParam& password)
			{
				Asn1MemoryReader body;
				if (!(p7.content.readSequence(body, SLIB_ASN1_TAG_CONTEXT(0)))) {
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
				Asn1String encData;
				if (!(contentInfo.readElement(SLIB_ASN1_TAG_CONTEXT_IMPLICIT(0), encData))) {
					return sl_null;
				}
				return PKCS12_Decrypt(encData.data, encData.length, password, algorithm);
			}

			static Memory PKCS12_Encrypt_PKCS7(const MemoryView& content, const StringParam& password, sl_uint32 iteration)
			{
				Asn1MemoryWriter contentInfo;
				if (!(contentInfo.writeObjectIdentifier(OID_PKCS7_DATA))) {
					return sl_null;
				}
				sl_uint8 salt[PKCS12_DEFAULT_SALT_SIZE];
				{
					Math::randomMemory(salt, sizeof(salt));
					Asn1MemoryWriter algorithm;
					if (!(algorithm.writeObjectIdentifier(OID_PKCS12_PBE_SHA1_3DES))) {
						return sl_null;
					}
					Asn1MemoryWriter param;
					if (!(param.writeOctetString(MemoryView(salt, sizeof(salt))))) {
						return sl_null;
					}
					if (!(param.writeInt(iteration))) {
						return sl_null;
					}
					if (!(algorithm.writeSequence(param))) {
						return sl_null;
					}
					if (!(contentInfo.writeSequence(algorithm))) {
						return sl_null;
					}
				}
				Memory encData = PKCS12_Encrypt<TripleDES, 24, SHA1>(content.data, content.size, password, salt, sizeof(salt), iteration);
				if (encData.isNull()) {
					return sl_null;
				}
				if (!(contentInfo.writeElement(SLIB_ASN1_TAG_CONTEXT_IMPLICIT(0), encData))) {
					return sl_null;
				}

				Asn1MemoryWriter body;
				// version
				if (!(body.writeInt(0))) {
					return sl_null;
				}
				if (!(body.writeSequence(contentInfo))) {
					return sl_null;
				}
				return Asn1::serializeElement(SLIB_ASN1_TAG_SEQUENCE, body);
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
						Asn1String content;
						if (!(p7.getData(content))) {
							return sl_false;
						}
						if (!(PKCS12_ParseBags(p12, content.data, content.length, password))) {
							return sl_false;
						}
					} else if (p7.type.equals(OID_PKCS7_ENCRYPTED_DATA)) {
						Memory mem = PKCS12_Decrypt_PKCS7(p7, password);
						if (mem.isNull()) {
							return sl_null;
						}
						if (!(PKCS12_ParseBags(p12, mem.getData(), mem.getSize(), password))) {
							return sl_false;
						}
					}
				}
				return sl_true;
			}

			static Memory PKCS12_Save(const PKCS12& p12, const StringParam& password)
			{
				Asn1MemoryWriter bags;
				if (p12.key.isDefined()) {
					Memory key = PKCS8_PrivateKey::savePrivateKey(p12.key);
					if (key.isNull()) {
						return sl_null;
					}
					Asn1MemoryWriter bag;
					if (!(bag.writeObjectIdentifier(OID_PKCS12_KEY_BAG))) {
						return sl_null;
					}
					if (!(bag.writeElement(SLIB_ASN1_TAG_CONTEXT(0), key))) {
						return sl_null;
					}
					Memory attrs = PKCS12_SaveBagFriendlyName(p12.friendlyName);
					if (attrs.isNotNull()) {
						if (!(bag.writeBytes(attrs))) {
							return sl_null;
						}
					}
					if (!(bags.writeSequence(bag))) {
						return sl_null;
					}
				}
				ListElements<Memory> certificates(p12.certificates);
				for (sl_size i = 0; i < certificates.count; i++) {
					Asn1MemoryWriter bag;
					if (!(bag.writeObjectIdentifier(OID_PKCS9_X509_CERTIFICATE))) {
						return sl_null;
					}
					if (!(bag.writeElement(SLIB_ASN1_TAG_CONTEXT(0), SLIB_ASN1_TAG_OCTET_STRING, certificates[i]))) {
						return sl_null;
					}
					Asn1MemoryWriter safeBag;
					if (!(safeBag.writeObjectIdentifier(OID_PKCS12_CERTIFICATE_BAG))) {
						return sl_null;
					}
					if (!(safeBag.writeSequence(bag, SLIB_ASN1_TAG_CONTEXT(0)))) {
						return sl_null;
					}
					Memory attrs = PKCS12_SaveBagFriendlyName(p12.friendlyNames.getValueAt_NoLock(i));
					if (attrs.isNotNull()) {
						if (!(safeBag.writeBytes(attrs))) {
							return sl_null;
						}
					}
					if (!(bags.writeSequence(safeBag))) {
						return sl_null;
					}
				}
				if (!(bags.getWrittenSize())) {
					return sl_null;
				}
				Memory encData = Asn1::serializeElement(SLIB_ASN1_TAG_SEQUENCE, bags);
				if (encData.isNull()) {
					return sl_null;
				}
				encData = PKCS12_Encrypt_PKCS7(encData, password, PKCS12_DEFAULT_ITERATION);
				if (encData.isNull()) {
					return sl_null;
				}
				Asn1MemoryWriter encBag;
				if (!(encBag.writeObjectIdentifier(OID_PKCS7_ENCRYPTED_DATA))) {
					return sl_null;
				}
				if (!(encBag.writeElement(SLIB_ASN1_TAG_CONTEXT(0), encData))) {
					return sl_null;
				}
				Memory authSafes = Asn1::serializeElement(SLIB_ASN1_TAG_SEQUENCE, encBag);
				if (authSafes.isNull()) {
					return sl_null;
				}
				authSafes = Asn1::serializeElement(SLIB_ASN1_TAG_SEQUENCE, authSafes);
				if (authSafes.isNull()) {
					return sl_null;
				}

				Asn1MemoryWriter p7Body;
				if (!(p7Body.writeObjectIdentifier(OID_PKCS7_DATA))) {
					return sl_null;
				}
				if (!(p7Body.writeOctetString(authSafes, SLIB_ASN1_TAG_CONTEXT(0)))) {
					return sl_null;
				}

				Asn1MemoryWriter p12Body;
				// version
				if (!(p12Body.writeInt(3))) {
					return sl_null;
				}
				if (!(p12Body.writeSequence(p7Body))) {
					return sl_null;
				}
				return Asn1::serializeElement(SLIB_ASN1_TAG_SEQUENCE, p12Body);
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
				sl_bool load(Asn1MemoryReader& reader)
				{
					Asn1MemoryReader body;
					if (reader.readSequence(body)) {
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
					return sl_false;
				}

				sl_bool getPublicKey(PublicKey& _out)
				{
					if (algorithm.algorithm.equals(OID_PKCS1_RSA)) {
						return Load_RSAPublicKey(_out.rsa, key.data, key.length);
					} else if (algorithm.algorithm.equals(OID_X9_62_EC_PUBLIC_KEY)) {
						if (!(Load_EllipticCurve(_out.ecc, algorithm.parameter))) {
							return sl_false;
						}
						return _out.ecc.Q.parseBinaryFormat(_out.ecc, MemoryView(key.data, key.length));
					}
					return sl_false;
				}

				static Memory savePublicKey(const PublicKey& _in)
				{
					Asn1MemoryWriter writer;
					Asn1MemoryWriter algorithm;
					if (_in.isRSA()) {
						if (!(algorithm.writeObjectIdentifier(OID_PKCS1_RSA))) {
							return sl_null;
						}
						if (!(writer.writeSequence(algorithm))) {
							return sl_null;
						}
						Memory key = Save_RSAPublicKey(_in.rsa);
						if (key.isNull()) {
							return sl_null;
						}
						if (!(writer.writeBitString(key))) {
							return sl_null;
						}
					} else if (_in.isECC()) {
						if (!(algorithm.writeObjectIdentifier(OID_X9_62_EC_PUBLIC_KEY))) {
							return sl_null;
						}
						Memory param = Save_EllipticCurve(_in.ecc);
						if (param.isNull()) {
							return sl_null;
						}
						if (!(algorithm.writeBytes(param))) {
							return sl_null;
						}
						if (!(writer.writeSequence(algorithm))) {
							return sl_null;
						}
						Memory key = _in.ecc.Q.toCompressedFormat(_in.ecc);
						if (key.isNull()) {
							return sl_null;
						}
						if (!(writer.writeBitString(key))) {
							return sl_null;
						}
					} else {
						return sl_null;
					}
					return Asn1::serializeElement(SLIB_ASN1_TAG_SEQUENCE, writer);
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

	void PublicKey::setECC(const EllipticCurve& curve, const ECPublicKey& key) noexcept
	{
		ecc.set(curve, key);
	}

	void PublicKey::setECC(const EllipticCurve& curve, ECPublicKey&& key) noexcept
	{
		ecc.set(curve, Move(key));
	}

	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PrivateKey)

	PrivateKey::PrivateKey()
	{
	}

	sl_bool PrivateKey::isDefined() const noexcept
	{
		return rsa.isDefined() || ecc.isDefined();
	}

	sl_bool PrivateKey::isRSA() const noexcept
	{
		return rsa.isDefined();
	}

	sl_bool PrivateKey::isECC() const noexcept
	{
		return ecc.isDefined();
	}

	void PrivateKey::setECC(const EllipticCurve& curve, const ECPrivateKey& key) noexcept
	{
		ecc.set(curve, key);
	}

	void PrivateKey::setECC(const EllipticCurve& curve, ECPrivateKey&& key) noexcept
	{
		ecc.set(curve, Move(key));
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

	sl_bool X509::load(const MemoryView& input)
	{
		Asn1MemoryReader reader(input);
		if (!(reader.readSequence(reader))) {
			return sl_false;
		}
		Asn1String content;
		content.data = reader.current;
		Asn1MemoryReader body;
		if (reader.readSequence(body)) {
			content.length = (sl_uint32)(body.end - content.data);
			body.readInt(version, SLIB_ASN1_TAG_CONTEXT(0));
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
			Asn1String issuerUID;
			{
				sl_uint8 bitsRemain;
				body.readBitString(issuerUID, bitsRemain, SLIB_ASN1_TAG_CONTEXT(1));
			}
			Asn1String subjectUID;
			{
				sl_uint8 bitsRemain;
				body.readBitString(subjectUID, bitsRemain, SLIB_ASN1_TAG_CONTEXT(2));
			}
			Asn1MemoryReader extensions;
			if (body.readSequence(extensions, SLIB_ASN1_TAG_CONTEXT(3))) {
				Asn1MemoryReader extension;
				while (extensions.readSequence(extension)) {
					Asn1ObjectIdentifier oid;
					if (extension.readObjectIdentifier(oid)) {
						sl_bool flagCritical = sl_false;
						extension.readBoolean(flagCritical);
						Asn1String value;
						if (extension.readOctetString(value)) {
							if (oid.equals(OID_SUBJECT_KEY_ID)) {
								Asn1MemoryReader item(value);
								if (item.readOctetString(value)) {
									subjectKeyId = BigInt::fromBytesBE(value.data, value.length);
								}
							} else if (oid.equals(OID_AUTHORITY_KEY_ID)) {
								Asn1MemoryReader item(value);
								Asn1MemoryReader keyId;
								if (item.readSequence(keyId)) {
									if (keyId.readElement(SLIB_ASN1_TAG_CONTEXT_IMPLICIT(0), value)) {
										authorityKeyId = BigInt::fromBytesBE(value.data, value.length);
									}
								}
							}
						}
					}
				}
			}
		} else {
			return sl_false;
		}
		X509Algorithm sigAlg;
		if (reader.readObject(sigAlg)) {
			signatureAlgorithm = GetSignatureAlgorithm(sigAlg.algorithm);
			if (signatureAlgorithm == X509SignatureAlgorithm::Unknown) {
				return sl_false;
			}
		} else {
			return sl_false;
		}
		Asn1String sig;
		sl_uint8 nSigBitsRemain;
		if (reader.readBitString(sig, nSigBitsRemain)) {
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

	sl_bool X509::loadPublicKey(PublicKey& _out, const MemoryView& _in) noexcept
	{
		Asn1MemoryReader reader(_in);
		X509_PublicKey key;
		if (reader.readObject(key)) {
			return key.getPublicKey(_out);
		}
		return sl_false;
	}

	Memory X509::savePublicKey(const PublicKey& _in) noexcept
	{
		return X509_PublicKey::savePublicKey(_in);
	}


	sl_bool PKCS8::loadPrivateKey(PrivateKey& _out, const MemoryView& _in) noexcept
	{
		Asn1MemoryReader reader(_in);
		PKCS8_PrivateKey key;
		if (reader.readObject(key)) {
			return key.getPrivateKey(_out);
		}
		return sl_false;
	}

	Memory PKCS8::savePrivateKey(const PrivateKey& _in) noexcept
	{
		return PKCS8_PrivateKey::savePrivateKey(_in);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PKCS12)

	PKCS12::PKCS12() noexcept
	{
	}

	sl_bool PKCS12::load(const MemoryView& input, const StringParam& password)
	{
		return PKCS12_Load(*this, input.data, input.size, password);
	}

	sl_bool PKCS12::load(const StringParam& filePath, const StringParam& password)
	{
		return load(File::readAllBytes(filePath), password);
	}

	Memory PKCS12::save(const StringParam& password) const
	{
		return PKCS12_Save(*this, password);
	}

	sl_bool PKCS12::save(const StringParam& filePath, const StringParam& password) const
	{
		Memory content = save(password);
		if (content.isNotNull()) {
			return File::writeAllBytes(filePath, content);
		}
		return sl_false;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PEMInstance)

	PEMInstance::PEMInstance() noexcept: type(PEMInstanceType::Unknown)
	{
	}

	sl_bool PEMInstance::getPrivateKey(PrivateKey& _out)
	{
		switch (type) {
			case PEMInstanceType::PrivateKey:
				return PKCS8::loadPrivateKey(_out, content);
			case PEMInstanceType::RSAPrivateKey:
				return Load_RSAPrivateKey(_out.rsa, content.getData(), content.getSize());
			case PEMInstanceType::ECPrivateKey:
				return Load_ECPrivateKey(_out.ecc, content.getData(), content.getSize());
			default:
				break;
		}
		return sl_false;
	}

	sl_bool PEMInstance::getPublicKey(PublicKey& _out)
	{
		switch (type) {
			case PEMInstanceType::PublicKey:
				return X509::loadPublicKey(_out, content);
			case PEMInstanceType::RSAPrivateKey:
				return Load_RSAPublicKey(_out.rsa, content.getData(), content.getSize());
			case PEMInstanceType::ECPublicKey:
				return _out.ecc.Q.parseBinaryFormat(_out.ecc, content);
			case PEMInstanceType::Certificate:
				{
					X509 cert;
					if (cert.load(content)) {
						_out = cert.key;
						return sl_true;
					}
					return sl_false;
				}
			default:
				break;
		}
		return sl_false;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PEM)

	PEM::PEM() noexcept
	{
	}

	sl_bool PEM::load(const MemoryView& input)
	{
		instances.setNull();
		sl_uint8* s = (sl_uint8*)(input.data);
		sl_uint8* end = s + input.size;
		if (s + 3 <= end) {
			if (*s == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
				s += 3;
			}
		}
		for (;;) {
			while (s < end) {
				if (!SLIB_CHAR_IS_WHITE_SPACE(*s)) {
					break;
				}
				s++;
			}
			if (s >= end) {
				break;
			}
			if (s + 11 >= end) {
				return sl_false;
			}
			if (!(Base::equalsMemory(s, "-----BEGIN ", 11))) {
				return sl_false;
			}
			s += 11;
			sl_uint8* r = Base::findMemory(s, end - s, '-');
			if (!r) {
				return sl_false;
			}
			if (r + 5 >= end) {
				return sl_false;
			}
			if (!(Base::equalsMemory(r, "-----", 5))) {
				return sl_false;
			}
			StringView type((char*)s, r - s);
			PEMInstance instance;
			if (type == StringView::literal("CERTIFICATE")) {
				instance.type = PEMInstanceType::Certificate;
			} else if (type == StringView::literal("TRUSTED CERTIFICATE")) {
				instance.type = PEMInstanceType::TrustedCertificate;
			} else if (type == StringView::literal("CERTIFICATE REQUEST")) {
				instance.type = PEMInstanceType::CertificateRequest;
			} else if (type == StringView::literal("X509 CRL")) {
				instance.type = PEMInstanceType::X509Control;
			} else if (type == StringView::literal("PUBLIC KEY")) {
				instance.type = PEMInstanceType::PublicKey;
			} else if (type == StringView::literal("RSA PRIVATE KEY")) {
				instance.type = PEMInstanceType::RSAPrivateKey;
			} else if (type == StringView::literal("RSA PUBLIC KEY")) {
				instance.type = PEMInstanceType::RSAPublicKey;
			} else if (type == StringView::literal("DSA PRIVATE KEY")) {
				instance.type = PEMInstanceType::DSAPrivateKey;
			} else if (type == StringView::literal("DSA PUBLIC KEY")) {
				instance.type = PEMInstanceType::DSAPublicKey;
			} else if (type == StringView::literal("PKCS7")) {
				instance.type = PEMInstanceType::PKCS7;
			} else if (type == StringView::literal("PKCS #7 SIGNED DATA")) {
				instance.type = PEMInstanceType::SignedPKCS7;
			} else if (type == StringView::literal("ENCRYPTED PRIVATE KEY")) {
				instance.type = PEMInstanceType::EncryptedPrivateKey;
			} else if (type == StringView::literal("PRIVATE KEY")) {
				instance.type = PEMInstanceType::PrivateKey;
			} else if (type == StringView::literal("DH PARAMETERS")) {
				instance.type = PEMInstanceType::DHParameters;
			} else if (type == StringView::literal("X9.42 DH PARAMETERS")) {
				instance.type = PEMInstanceType::DHXParameters;
			} else if (type == StringView::literal("SSL SESSION PARAMETERS")) {
				instance.type = PEMInstanceType::SSLSessionParameters;
			} else if (type == StringView::literal("DSA PARAMETERS")) {
				instance.type = PEMInstanceType::DSAParameters;
			} else if (type == StringView::literal("ECDSA PUBLIC KEY")) {
				instance.type = PEMInstanceType::ECPublicKey;
			} else if (type == StringView::literal("EC PARAMETERS")) {
				instance.type = PEMInstanceType::ECParamters;
			} else if (type == StringView::literal("EC PRIVATE KEY")) {
				instance.type = PEMInstanceType::ECPrivateKey;
			} else if (type == StringView::literal("PARAMETERS")) {
				instance.type = PEMInstanceType::Paramters;
			} else if (type == StringView::literal("CMS")) {
				instance.type = PEMInstanceType::CMS;
			}
			s = r + 5;
			r = Base::findMemory(s, end - s, '-');
			if (!r) {
				return sl_false;
			}
			if (r + 9 >= end) {
				return sl_false;
			}
			if (!(Base::equalsMemory(r, "-----END ", 9))) {
				return sl_false;
			}
			instance.content = Base64::decode(StringView((char*)s, r - s));
			if (instance.content.isNull()) {
				return sl_false;
			}
			s = r + 9;
			r = Base::findMemory(s, end - s, '-');
			if (!r) {
				return sl_false;
			}
			if (r + 5 >= end) {
				return sl_false;
			}
			if (!(Base::equalsMemory(r, "-----", 5))) {
				return sl_false;
			}
			if (StringView((char*)s, r - s) != type) {
				return sl_false;
			}
			if (!(instances.add_NoLock(Move(instance)))) {
				return sl_false;
			}
			s = r + 5;
		}
		return instances.isNotNull();
	}

	sl_bool PEM::load(const StringParam& filePath)
	{
		return load(File::readAllBytes(filePath));
	}

	Memory PEM::save()
	{
		ListElements<PEMInstance> items(instances);
		if (!(items.count)) {
			return sl_null;
		}
		SerializeOutput output;
		for (sl_size i = 0; i < items.count; i++) {
			PEMInstance& instance = items[i];
			StringView type;
			switch (instance.type) {
				case PEMInstanceType::Certificate:
					type = StringView::literal("CERTIFICATE");
					break;
				case PEMInstanceType::TrustedCertificate:
					type = StringView::literal("TRUSTED CERTIFICATE");
					break;
				case PEMInstanceType::CertificateRequest:
					type = StringView::literal("CERTIFICATE REQUEST");
					break;
				case PEMInstanceType::X509Control:
					type = StringView::literal("X509 CRL");
					break;
				case PEMInstanceType::PublicKey:
					type = StringView::literal("PUBLIC KEY");
					break;
				case PEMInstanceType::RSAPrivateKey:
					type = StringView::literal("RSA PRIVATE KEY");
					break;
				case PEMInstanceType::RSAPublicKey:
					type = StringView::literal("RSA PUBLIC KEY");
					break;
				case PEMInstanceType::DSAPrivateKey:
					type = StringView::literal("DSA PRIVATE KEY");
					break;
				case PEMInstanceType::DSAPublicKey:
					type = StringView::literal("DSA PUBLIC KEY");
					break;
				case PEMInstanceType::PKCS7:
					type = StringView::literal("PKCS7");
					break;
				case PEMInstanceType::SignedPKCS7:
					type = StringView::literal("PKCS #7 SIGNED DATA");
					break;
				case PEMInstanceType::EncryptedPrivateKey:
					type = StringView::literal("ENCRYPTED PRIVATE KEY");
					break;
				case PEMInstanceType::PrivateKey:
					type = StringView::literal("PRIVATE KEY");
					break;
				case PEMInstanceType::DHParameters:
					type = StringView::literal("DH PARAMETERS");
					break;
				case PEMInstanceType::DHXParameters:
					type = StringView::literal("X9.42 DH PARAMETERS");
					break;
				case PEMInstanceType::SSLSessionParameters:
					type = StringView::literal("SSL SESSION PARAMETERS");
					break;
				case PEMInstanceType::DSAParameters:
					type = StringView::literal("DSA PARAMETERS");
					break;
				case PEMInstanceType::ECPublicKey:
					type = StringView::literal("ECDSA PUBLIC KEY");
					break;
				case PEMInstanceType::ECParamters:
					type = StringView::literal("EC PARAMETERS");
					break;
				case PEMInstanceType::ECPrivateKey:
					type = StringView::literal("EC PRIVATE KEY");
					break;
				case PEMInstanceType::Paramters:
					type = StringView::literal("PARAMETERS");
					break;
				case PEMInstanceType::CMS:
					type = StringView::literal("CMS");
					break;
				default:
					return sl_null;
			}
			if (!(output.write("-----BEGIN ", 11))) {
				return sl_null;
			}
			if (!(output.write(type.getData(), type.getLength()))) {
				return sl_null;
			}
			if (!(output.write("-----\r\n", 7))) {
				return sl_null;
			}
			String base64 = Base64::encode(instance.content);
			if (base64.isNull()) {
				return sl_null;
			}
			sl_char8* s = base64.getData();
			sl_size len = base64.getLength();
			for (sl_size i = 0; i < len; i += 64) {
				sl_size n = len - i;
				if (!(output.write(s + i, n > 64 ? 64 : n))) {
					return sl_null;
				}
				if (!(output.write("\r\n", 2))) {
					return sl_null;
				}
			}
			if (!(output.write("-----END ", 9))) {
				return sl_null;
			}
			if (!(output.write(type.getData(), type.getLength()))) {
				return sl_null;
			}
			if (!(output.write("-----\r\n", 7))) {
				return sl_null;
			}
		}
		return output.releaseToMemory();
	}

	sl_bool PEM::save(const StringParam& filePath)
	{
		Memory content = save();
		if (content.isNotNull()) {
			return File::writeAllBytes(filePath, content);
		}
		return sl_false;
	}

	sl_bool PEM::getPrivateKey(PrivateKey& _out)
	{
		ListElements<PEMInstance> items(instances);
		for (sl_size i = 0; i < items.count; i++) {
			if (items[i].getPrivateKey(_out)) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool PEM::addPrivateKey(const PrivateKey& privateKey)
	{
		PEMInstance instance;
		instance.content = PKCS8::savePrivateKey(privateKey);
		if (instance.content.isNotNull()) {
			instance.type = PEMInstanceType::PrivateKey;
			return instances.add_NoLock(Move(instance));
		}
		return sl_false;
	}

	sl_bool PEM::getPublicKey(PublicKey& _out)
	{
		ListElements<PEMInstance> items(instances);
		for (sl_size i = 0; i < items.count; i++) {
			if (items[i].getPublicKey(_out)) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool PEM::addPublicKey(const PublicKey& publicKey)
	{
		PEMInstance instance;
		instance.content = X509_PublicKey::savePublicKey(publicKey);
		if (instance.content.isNotNull()) {
			instance.type = PEMInstanceType::PublicKey;
			return instances.add_NoLock(Move(instance));
		}
		return sl_false;
	}

}
