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

#include "slib/crypto/openssl.h"

#include "slib/crypto/sha2.h"
#include "slib/core/file.h"
#include "slib/core/time_zone.h"
#include "slib/core/scoped_buffer.h"

#include "openssl/aes.h"
#include "openssl/rand.h"
#include "openssl/ec.h"
#include "openssl/x509v3.h"
#include "openssl/evp.h"
#include "openssl/pem.h"

#pragma comment(lib, "ws2_32.lib")

namespace slib
{
	
	namespace priv
	{
		namespace openssl
		{

#if defined(SLIB_PLATFORM_IS_WIN32)
			void initThread();
#else
			static void initThread()
			{
			}
#endif
		
			static Memory Generate_RSA_Signature(EVP_PKEY* key, const EVP_MD* md, const void* data, sl_size size)
			{
				unsigned int len = EVP_PKEY_size(key);
				if (len > 0) {
					EVP_MD_CTX* ctx = EVP_MD_CTX_new();
					if (ctx) {
						if (EVP_DigestInit(ctx, md)) {
							if (EVP_DigestUpdate(ctx, data, (size_t)size)) {
								Memory mem = Memory::create(len);
								if (mem.isNotNull()) {
									if (EVP_SignFinal(ctx, (unsigned char*)(mem.getData()), &len, key)) {
										EVP_MD_CTX_free(ctx);
										return mem;
									}
								}
							}
						}
						EVP_MD_CTX_free(ctx);
					}
				}
				return sl_null;
			}
		
			static sl_bool Verify_RSA_Signature(EVP_PKEY* key, const EVP_MD* md, const void* data, sl_size sizeData, const void* signature, sl_size sizeSignature)
			{
				EVP_MD_CTX* ctx = EVP_MD_CTX_new();
				if (ctx) {
					if (EVP_DigestInit(ctx, md)) {
						if (EVP_DigestUpdate(ctx, data, (size_t)sizeData)) {
							if (1 == EVP_VerifyFinal(ctx, (unsigned char*)(signature), (unsigned int)(sizeSignature), key)) {
								EVP_MD_CTX_free(ctx);
								return sl_true;
							}
						}
					}
					EVP_MD_CTX_free(ctx);
				}
				return sl_false;
			}
		
			static Memory Generate_ECDSA_Signature(EVP_PKEY* key, const void* hash, sl_uint32 sizeHash)
			{
				EC_KEY* ekey = EVP_PKEY_get0_EC_KEY(key);
				if (!ekey) {
					return sl_null;
				}
				Memory ret;
				ECDSA_SIG* sig = ECDSA_do_sign((unsigned char*)hash, (int)sizeHash, ekey);
				if (sig) {
					const BIGNUM* r;
					const BIGNUM* s;
					ECDSA_SIG_get0(sig, &r, &s);
					sl_uint32 nr = (sl_uint32)(BN_num_bytes(r));
					sl_uint32 ns = (sl_uint32)(BN_num_bytes(s));
					sl_uint32 m = SLIB_MAX(nr, ns);
					sl_uint32 n = m << 1;
					ret = Memory::create(n);
					if (ret.isNotNull()) {
						unsigned char* t = (unsigned char*)(ret.getData());
						Base::zeroMemory(t, n);
						BN_bn2bin(r, t + (m - nr));
						BN_bn2bin(s, t + (n - ns));
					}
					ECDSA_SIG_free(sig);
				}
				return ret;
			}
		
			static sl_bool Verify_ECDSA_Signature(EVP_PKEY* key, const void* hash, sl_uint32 sizeHash, const void* signature, sl_size sizeSignature)
			{
				if (sizeSignature & 1) {
					return sl_false;
				}
				EC_KEY* ekey = EVP_PKEY_get0_EC_KEY(key);
				if (!ekey) {
					return sl_false;
				}
				sl_bool bRet = sl_false;
				ECDSA_SIG* sig = ECDSA_SIG_new();
				if (sig) {
					unsigned char* t = (unsigned char*)signature;
					sl_uint32 m = (sl_uint32)(sizeSignature >> 1);
					BIGNUM* r = BN_bin2bn(t, (int)m, sl_null);
					BIGNUM* s = BN_bin2bn(t + m, (int)m, sl_null);
					ECDSA_SIG_set0(sig, r, s);
					bRet = ECDSA_do_verify((unsigned char*)hash, (int)sizeHash, sig, ekey) == 1;
					ECDSA_SIG_free(sig);
				}
				return bRet;
			}

			static Memory Generate_RSA_PSS_Signature(EVP_PKEY* key, const EVP_MD* md, const void* hash, sl_size sizeHash)
			{
				::RSA* rsa = EVP_PKEY_get0_RSA(key);
				if (!rsa) {
					return sl_null;
				}
				sl_uint32 sizeRsa = (sl_uint32)(RSA_size(rsa));
				SLIB_SCOPED_BUFFER(unsigned char, 1024, padded, sizeRsa)
				if (padded) {
					Base::zeroMemory(padded, sizeRsa);
					if (RSA_padding_add_PKCS1_PSS_mgf1(rsa, padded, (unsigned char*)hash, md, md, -1)) {
						Memory ret = Memory::create(sizeRsa);
						if (ret.isNotNull()) {
							if (RSA_private_encrypt((int)sizeRsa, padded, (unsigned char*)(ret.getData()), rsa, RSA_NO_PADDING)) {
								return ret;
							}
						}
					}
				}
				return sl_null;
			}
		
			static sl_bool Verify_RSA_PSS_Signature(EVP_PKEY* key, const EVP_MD* md, const void* hash, sl_size sizeHash, const void* signature, sl_size sizeSignature)
			{
				::RSA* rsa = EVP_PKEY_get0_RSA(key);
				if (!rsa) {
					return sl_false;
				}
				sl_uint32 sizeRsa = (sl_uint32)(RSA_size(rsa));
				SLIB_SCOPED_BUFFER(unsigned char, 1024, sig, sizeRsa)
				if (sig) {
					Base::zeroMemory(sig, sizeRsa);
					if (RSA_public_decrypt((int)sizeRsa, (unsigned char*)signature, sig, rsa, RSA_NO_PADDING)) {
						if (RSA_verify_PKCS1_PSS_mgf1(rsa, (unsigned char*)hash, md, md, sig, -1)) {
							return sl_true;
						}
					}
				}
				return sl_false;
			}
		
			static BigInt Get_BigInt_from_BIGNUM(const BIGNUM* bn)
			{
				if (bn) {
					sl_size size = (sl_size)(BN_num_bytes(bn));
					if (size) {
						SLIB_SCOPED_BUFFER(unsigned char, 1024, t, size)
						if (t) {
							BN_bn2bin(bn, t);
							return BigInt::fromBytesBE(t, size);
						}
					}
				}
				return sl_null;
			}
			
			static BIGNUM* Get_BIGNUM_from_BigInt(const BigInt& n)
			{
				if (n.isNotNull()) {
					sl_size size = n.getMostSignificantBytes();
					if (size) {
						SLIB_SCOPED_BUFFER(unsigned char, 1024, t, size)
						if (t) {
							n.getBytesBE(t, size);
							return BN_bin2bn(t, (int)(size), sl_null);
						}
					}
				}
				return sl_null;
			}
			
			static ECPoint Get_ECPoint_from_EC_POINT(const EC_GROUP* group, const EC_POINT* pt)
			{
				ECPoint ret;
				if (pt) {
					BIGNUM* x = BN_new();
					if (x) {
						BIGNUM* y = BN_new();
						if (y) {
							if (1 == EC_POINT_get_affine_coordinates(group, pt, x, y, sl_null)) {
								ret.x = Get_BigInt_from_BIGNUM(x);
								if (ret.x.isNotZero()) {
									ret.y = Get_BigInt_from_BIGNUM(y);
								}
							}
							BN_free(y);
						}
						BN_free(x);
					}
				}
				return ret;
			}
			
			static EC_POINT* Get_EC_POINT_from_ECPoint(const EC_GROUP* group, const ECPoint& pt)
			{
				EC_POINT* ret = EC_POINT_new(group);
				if (ret) {
					sl_bool flagSuccess = sl_false;
					BIGNUM* x = Get_BIGNUM_from_BigInt(pt.x);
					if (x) {
						BIGNUM* y = Get_BIGNUM_from_BigInt(pt.y);
						if (y) {
							if (1 == EC_POINT_set_affine_coordinates(group, ret, x, y, sl_null)) {
								flagSuccess = sl_true;
							}
							BN_free(y);
						}
						BN_free(x);
					}
					if (flagSuccess) {
						return ret;
					}
					EC_POINT_free(ret);
				}
				return sl_null;
			}
			
			static EC_KEY* Get_EC_KEY_from_ECPublicKey(const EC_GROUP* group, const ECPublicKey& key)
			{
				if (key.Q.isO()) {
					return sl_null;
				}
				EC_KEY* ek = EC_KEY_new();
				if (ek) {
					sl_bool flagSuccess = sl_false;
					EC_KEY_set_group(ek, group);
					EC_POINT* pt = Get_EC_POINT_from_ECPoint(group, key.Q);
					if (pt) {
						EC_KEY_set_public_key(ek, pt);
						flagSuccess = sl_true;
						EC_POINT_free(pt);
					}
					if (flagSuccess) {
						return ek;
					}
					EC_KEY_free(ek);
				}
				return sl_null;
			}
			
			static EC_KEY* Get_EC_KEY_from_ECPrivateKey(const EC_GROUP* group, const ECPrivateKey& key)
			{
				EC_KEY* ek = Get_EC_KEY_from_ECPublicKey(group, key);
				if (ek) {
					sl_bool flagSuccess = sl_false;
					BIGNUM* bn = Get_BIGNUM_from_BigInt(key.d);
					if (bn) {
						EC_KEY_set_private_key(ek, bn);
						flagSuccess = sl_true;
						BN_free(bn);
					}
					if (flagSuccess) {
						return ek;
					}
					EC_KEY_free(ek);
				}
				return sl_null;
			}
			
			static ECDSA_SIG* get_ECDSA_SIG_from_ECDSA_Signature(const ECDSA_Signature& _sig)
			{
				ECDSA_SIG* sig = ECDSA_SIG_new();
				if (sig) {
					BIGNUM* r = Get_BIGNUM_from_BigInt(_sig.r);
					if (r) {
						BIGNUM* s = Get_BIGNUM_from_BigInt(_sig.s);
						if (s) {
							ECDSA_SIG_set0(sig, r, s);
							return sig;
						}
						BN_free(r);
					}
					ECDSA_SIG_free(sig);
				}
				return sl_null;
			}
			
			static ECDSA_Signature Do_sign_ECDSA(EllipticCurveId curveId, const ECPrivateKey& key, const void* hash, sl_size size)
			{
				initThread();
				ECDSA_Signature ret;
				EC_GROUP* group = EC_GROUP_new_by_curve_name((int)curveId);
				if (group) {
					EC_KEY* ek = Get_EC_KEY_from_ECPrivateKey(group, key);
					if (ek) {
						ECDSA_SIG* sig = ECDSA_do_sign((unsigned char*)hash, (int)size, ek);
						if (sig) {
							const BIGNUM* r = ECDSA_SIG_get0_r(sig);
							if (r) {
								ret.r = Get_BigInt_from_BIGNUM(r);
							}
							const BIGNUM* s = ECDSA_SIG_get0_s(sig);
							if (s) {
								ret.s = Get_BigInt_from_BIGNUM(s);
							}
							ECDSA_SIG_free(sig);
						}
						EC_KEY_free(ek);
					}
					EC_GROUP_free(group);
				}
				return ret;
			}
			
			static sl_bool Do_verify_ECDSA(EllipticCurveId curveId, const ECPublicKey& key, const void* hash, sl_size size, const ECDSA_Signature& _sig)
			{
				initThread();
				sl_bool flagVerified = sl_false;
				EC_GROUP* group = EC_GROUP_new_by_curve_name((int)curveId);
				if (group) {
					EC_KEY* ek = Get_EC_KEY_from_ECPublicKey(group, key);
					if (ek) {
						ECDSA_SIG* sig = get_ECDSA_SIG_from_ECDSA_Signature(_sig);
						if (sig) {
							if (1 == ECDSA_do_verify((unsigned char*)hash, (int)size, sig, ek)) {
								flagVerified = sl_true;
							}
							ECDSA_SIG_free(sig);
						}
						EC_KEY_free(ek);
					}
					EC_GROUP_free(group);
				}
				return flagVerified;
			}

			static String Get_String_from_ASN1_OBJECT(const ASN1_OBJECT* object)
			{
				if (object) {
					sl_int32 size = (sl_int32)(OBJ_obj2txt(sl_null, 0, object, 0));
					if (size > 0) {
						SLIB_SCOPED_BUFFER(char, 1024, buf, size + 1)
						if (buf) {
							size = OBJ_obj2txt(buf, size + 1, object, 0);
							if (size > 0) {
								return String::fromUtf8(buf, size);
							}
						}
					} else {
						if (!size) {
							return String::getEmpty();
						}
					}
				}
				return sl_null;
			}

			static String Get_String_from_ASN1_STRING(const ASN1_STRING* str)
			{
				if (str) {
					sl_uint32 size = (sl_uint32)(ASN1_STRING_length(str));
					if (size) {
						const unsigned char* data = ASN1_STRING_get0_data(str);
						if (data) {
							return String::fromUtf8(data, size);
						}
					} else {
						return String::getEmpty();
					}
				}
				return sl_null;
			}

			static ASN1_STRING* Get_ASN1_STRING_from_String(const String& str)
			{
				ASN1_STRING *ret = ASN1_STRING_new();
				if (ret) {
					ASN1_STRING_set(ret, str.getData(), str.getLength());
					return ret;
				}
				return sl_null;
			}

			static Time Get_Time_from_ASN1_TIME(const ASN1_TIME* time)
			{
				if (time) {
					tm vt;
					ASN1_TIME_to_tm(time, &vt);
					return Time(vt.tm_year + 1900, vt.tm_mon + 1, vt.tm_mday, vt.tm_hour, vt.tm_min, vt.tm_sec, 0, 0, TimeZone::UTC());
				}
				return Time::zero();
			}

			static void Set_Time_to_ASN1_TIME(ASN1_TIME* _out, const Time& time)
			{
				ASN1_TIME_set(_out, (sl_int32)(time.toUnixTime()));
			}

			static ASN1_INTEGER* Get_ASN1_INTEGER_from_BigInt(const BigInt& n)
			{
				BIGNUM* bn = Get_BIGNUM_from_BigInt(n);
				if (bn) {
					ASN1_INTEGER* ret = BN_to_ASN1_INTEGER(bn, sl_null);
					BN_free(bn);
					return ret;
				}
				return sl_null;
			}

			static BigInt Get_BigInt_from_ASN1_INTEGER(const ASN1_INTEGER* ai)
			{
				if (ai) {
					BIGNUM* bn = ASN1_INTEGER_to_BN(ai, sl_null);
					if (bn) {
						BigInt ret = Get_BigInt_from_BIGNUM(bn);
						BN_free(bn);
						return ret;
					}
				}
				return sl_null;
			}

			static BigInt Get_BigInt_from_ASN1_OCTET_STRING(const ASN1_OCTET_STRING* oct)
			{
				Memory memory = Memory::create(oct->length);
				Base::copyMemory(memory.getData(), oct->data, oct->length);
				return BigInt::fromBytesBE(memory);
			}

			static ASN1_OCTET_STRING* Get_ASN1_OCTET_STRING_from_BigInt(const BigInt& num)
			{
				ASN1_OCTET_STRING* ret = ASN1_OCTET_STRING_new();
				Memory mem = num.getBytesBE();
				if (ret) {
					ASN1_OCTET_STRING_set(ret, (sl_uint8*)mem.getData(), mem.getSize());
					return ret;
				}
				return sl_null;
			}

			template <class NID>
			static HashMap<NID, String> Get_Map_from_X509_NAME(const X509_NAME* name)
			{
				HashMap<NID, String> ret;
				if (name) {
					sl_uint32 count = (sl_uint32)(X509_NAME_entry_count(name));
					for (sl_uint32 i = 0; i < count; i++) {
						X509_NAME_ENTRY* entry = X509_NAME_get_entry(name, i);
						if (entry) {
							ASN1_OBJECT* obj = X509_NAME_ENTRY_get_object(entry);
							if (obj) {
								int nid = OBJ_obj2nid(obj);
								String value = Get_String_from_ASN1_STRING(X509_NAME_ENTRY_get_data(entry));
								ret.add_NoLock((NID)nid, Move(value));
							}
						}
					}
				}
				return ret;
			}

			template <class NID>
			static X509_NAME* Get_X509_NAME_from_HashMap(const HashMap<NID, String>& map)
			{
				X509_NAME* ret = X509_NAME_new();
				if (!ret) {
					return sl_null;
				}
				auto node = map.getFirstNode();
				while (node) {
					StringCstr value(node->value);
					if (!(X509_NAME_add_entry_by_NID(ret, (int)(node->key), MBSTRING_UTF8, (sl_uint8*)(value.getData()), (int)(value.getLength()), 0, 0))) {
						X509_NAME_free(ret);
						return sl_null;
					}
					node = node->next;
				}
				return ret;
			}

			static sl_bool Get_RSAPublicKey_from_RSA(RSAPublicKey& _out, const ::RSA* rsa)
			{
				if (rsa) {
					const BIGNUM* n = sl_null;
					const BIGNUM* e = sl_null;
					RSA_get0_key(rsa, &n, &e, sl_null);
					if (n && e) {
						_out.N = Get_BigInt_from_BIGNUM(n);
						_out.E = Get_BigInt_from_BIGNUM(e);
						return sl_true;
					}
				}
				return sl_false;
			}

			static sl_bool Get_RSAPrivateKey_from_RSA(RSAPrivateKey& _out, const ::RSA* rsa)
			{
				if (rsa) {
					const BIGNUM* n = sl_null;
					const BIGNUM* e = sl_null;
					const BIGNUM* d = sl_null;
					RSA_get0_key(rsa, &n, &e, &d);
					if (n && e && d) {
						_out.N = Get_BigInt_from_BIGNUM(n);
						_out.E = Get_BigInt_from_BIGNUM(e);
						_out.D = Get_BigInt_from_BIGNUM(d);
						_out.flagUseOnlyD = sl_true;
						return sl_true;
					}
				}
				return sl_false;
			}

			static sl_bool Get_EllipticCurve_from_EC_KEY(EllipticCurve& _out, const EC_KEY* ekey)
			{
				if (ekey) {
					const EC_GROUP* group = EC_KEY_get0_group(ekey);
					if (group) {
						if (_out.setId((EllipticCurveId)(EC_GROUP_get_curve_name(group)))) {
							return sl_true;
						}
						sl_bool flagSuccess = sl_false;
						BIGNUM* a = BN_new();
						if (a) {
							BIGNUM* b = BN_new();
							if (b) {
								BIGNUM* p = BN_new();
								if (p) {
									if (EC_GROUP_get_curve(group, p, a, b, sl_null)) {
										_out.a = Get_BigInt_from_BIGNUM(a);
										_out.b = Get_BigInt_from_BIGNUM(b);
										_out.p = Get_BigInt_from_BIGNUM(p);
										_out.G = Get_ECPoint_from_EC_POINT(group, EC_GROUP_get0_generator(group));
										_out.n = Get_BigInt_from_BIGNUM(EC_GROUP_get0_order(group));
										flagSuccess = sl_true;
									}
									BN_free(p);
								}
								BN_free(b);
							}
							BN_free(a);
						}
						return flagSuccess;
					}
				}
				return sl_false;
			}

			static ::EC_GROUP* Get_EC_GROUP_from_EllipticCurve(const EllipticCurve& curve)
			{
				if (curve.id != EllipticCurveId::Unknown) {
					EC_GROUP* group = EC_GROUP_new_by_curve_name((int)curve.id);
					if (group) {
						return group;
					}
					return sl_null;
				}

				BIGNUM* p = Get_BIGNUM_from_BigInt(curve.p);
				if (p) {
					BIGNUM* a = Get_BIGNUM_from_BigInt(curve.a);
					if (a) {
						BIGNUM* b = Get_BIGNUM_from_BigInt(curve.b);
						if (b) {
							::EC_GROUP* ec_group = EC_GROUP_new_curve_GFp(p, a, b, sl_null);
							if (ec_group) {
								BN_free(p);
								BN_free(a);
								BN_free(b);
								return ec_group;
							}
							BN_free(b);
						}
						BN_free(a);
					}
					BN_free(p);
				}
				
				return sl_null;
			}


			static sl_bool Get_ECPublicKeyWithCurve_from_EC_KEY(ECPublicKeyWithCurve& _out, const EC_KEY* ekey)
			{
				if (ekey) {
					if (Get_EllipticCurve_from_EC_KEY(_out, ekey)) {
						_out.Q = Get_ECPoint_from_EC_POINT(EC_KEY_get0_group(ekey), EC_KEY_get0_public_key(ekey));
						return !(_out.Q.isO());
					}
				}
				return sl_false;
			}

			static sl_bool Get_ECPrivateKeyWithCurve_from_EC_KEY(ECPrivateKeyWithCurve& _out, const EC_KEY* ekey)
			{
				if (ekey) {
					if (Get_EllipticCurve_from_EC_KEY(_out, ekey)) {
						_out.Q = Get_ECPoint_from_EC_POINT(EC_KEY_get0_group(ekey), EC_KEY_get0_public_key(ekey));
						_out.d = Get_BigInt_from_BIGNUM(EC_KEY_get0_private_key(ekey));
						return !(_out.Q.isO() || _out.d.isZero());
					}
				}
				return sl_false;
			}

			static sl_bool Get_PublicKey_from_EVP_PKEY(PublicKey& _out, EVP_PKEY* pkey)
			{
				if (pkey) {
					int keyType = EVP_PKEY_id(pkey);
					if (keyType == EVP_PKEY_RSA || keyType == EVP_PKEY_RSA_PSS) {
						return Get_RSAPublicKey_from_RSA(_out.rsa, EVP_PKEY_get0_RSA(pkey));
					} else if (keyType == EVP_PKEY_EC) {
						return Get_ECPublicKeyWithCurve_from_EC_KEY(_out.ecc, EVP_PKEY_get0_EC_KEY(pkey));
					}
				}
				return sl_false;
			}

			static sl_bool Get_PrivateKey_from_EVP_PKEY(PrivateKey& _out, EVP_PKEY* pkey)
			{
				if (pkey) {
					int keyType = EVP_PKEY_id(pkey);
					if (keyType == EVP_PKEY_RSA || keyType == EVP_PKEY_RSA_PSS) {
						return Get_RSAPrivateKey_from_RSA(_out.rsa, EVP_PKEY_get0_RSA(pkey));
					} else if (keyType == EVP_PKEY_EC) {
						return Get_ECPrivateKeyWithCurve_from_EC_KEY(_out.ecc, EVP_PKEY_get0_EC_KEY(pkey));
					}
				}
				return sl_false;
			}

			static ::RSA* Get_RSA_from_RSAPublicKey(const RSAPublicKey& key, const BigInt* D = sl_null)
			{
				::RSA* rsa = RSA_new();
				if (rsa) {
					BIGNUM* n = Get_BIGNUM_from_BigInt(key.N);
					if (n) {
						BIGNUM* e = Get_BIGNUM_from_BigInt(key.E);
						if (e) {
							sl_bool flagSuccess = sl_true;
							BIGNUM* d = sl_null;
							if (D) {
								d = Get_BIGNUM_from_BigInt(*D);
								if (!d) {
									flagSuccess = sl_false;
								}
							}
							if (flagSuccess) {
								RSA_set0_key(rsa, n, e, d);
								return rsa;
							}
							BN_free(e);
						}
						BN_free(n);
					}
					RSA_free(rsa);
				}
				return sl_null;
			}

			static ::RSA* Get_RSA_from_RSAPrivateKey(const RSAPrivateKey& key)
			{
				return Get_RSA_from_RSAPublicKey(key, &(key.D));
			}


			static EVP_PKEY* Get_EVP_PKEY_from_RSAPublicKey(const RSAPublicKey& key, const BigInt* D = sl_null)
			{
				EVP_PKEY* ret = EVP_PKEY_new();
				if (ret) {
					::RSA* rsa = Get_RSA_from_RSAPublicKey(key, D);
					if (rsa) {
						EVP_PKEY_set_alias_type(ret, EVP_PKEY_RSA);
						EVP_PKEY_set1_RSA(ret, rsa);
						RSA_free(rsa);
						return ret;
					}
					EVP_PKEY_free(ret);
				}
				return sl_null;
			}

			static EVP_PKEY* Get_EVP_PKEY_from_ECPublicKey(const ECPublicKeyWithCurve curve)
			{
				EVP_PKEY* ret = EVP_PKEY_new();
				if (ret) {
					EC_GROUP* group = Get_EC_GROUP_from_EllipticCurve(curve);
					if (group) {
						EC_KEY* ek = Get_EC_KEY_from_ECPublicKey(group, curve);
						if (ek) {
							EVP_PKEY_set1_EC_KEY(ret, ek);
							return ret;
						}
						EC_GROUP_free(group);
					}
					EVP_PKEY_free(ret);
				}
				return sl_null;
			}

			static EVP_PKEY* Get_EVP_PKEY_from_ECPrivateKey(const ECPrivateKeyWithCurve& curve)
			{
				EVP_PKEY* ret = EVP_PKEY_new();
				if (ret) {
					EC_GROUP* group = Get_EC_GROUP_from_EllipticCurve(curve);
					if (group) {
						EC_KEY* ek = Get_EC_KEY_from_ECPrivateKey(group, curve);
						if (ek) {
							EVP_PKEY_set1_EC_KEY(ret, ek);
							return ret;
						}
						EC_GROUP_free(group);
					}
					EVP_PKEY_free(ret);
				}
				return sl_null;
			}

			static EVP_PKEY* Get_EVP_PKEY_from_RSAPrivateKey(const RSAPrivateKey& key)
			{
				return Get_EVP_PKEY_from_RSAPublicKey(key, &(key.D));
			}

			
			static EVP_PKEY* Get_EVP_PKEY_from_PublicKey(const PublicKey& key)
			{
				if (key.isRSA()) {
					return Get_EVP_PKEY_from_RSAPublicKey(key.rsa);
				} else if (key.isECC()) {
					return Get_EVP_PKEY_from_ECPublicKey(key.ecc);
				}
				return sl_null;
			}

			static EVP_PKEY* Get_EVP_PKEY_from_PrivateKey(const PrivateKey& key)
			{
				if (key.isRSA()) {
					return Get_EVP_PKEY_from_RSAPrivateKey(key.rsa);
				} else if (key.isECC()) {
					return Get_EVP_PKEY_from_ECPrivateKey(key.ecc);
				}
				return sl_null;
			}
			
			static void Get_X509_Extension(X509& _out, ::X509* handle) 
			{
				BASIC_CONSTRAINTS *bs = (BASIC_CONSTRAINTS*)X509_get_ext_d2i(handle, (int)X509ExtensionKey::BasicConstraints, NULL, NULL);
				if (bs) {
					_out.basicCA = bs->ca;
					BASIC_CONSTRAINTS_free(bs);
				}

				EXTENDED_KEY_USAGE *extusage = (EXTENDED_KEY_USAGE*)X509_get_ext_d2i(handle, (int)X509ExtensionKey::ExtendedKeyUsage, NULL, NULL);
				if (extusage) {
					for (int i = 0; i < sk_ASN1_OBJECT_num(extusage); i++) {
						int nid = OBJ_obj2nid(sk_ASN1_OBJECT_value(extusage, i));
						_out.usages.add_NoLock(nid);
					}
					sk_ASN1_OBJECT_pop_free(extusage, ASN1_OBJECT_free);
				}

				ASN1_BIT_STRING *usage = (ASN1_BIT_STRING*)X509_get_ext_d2i(handle, (int)X509ExtensionKey::KeyUsage, NULL, NULL);
				if (usage) {
					_out.keyUsage = String::makeHexString(usage->data, usage->length);
					ASN1_BIT_STRING_free(usage);
				}

				ASN1_OCTET_STRING* skid = (ASN1_INTEGER*)X509_get_ext_d2i(handle, (int)X509ExtensionKey::SubjectKeyIdentifier, NULL, NULL);
				if (skid) {
					_out.subjectKeyId = Get_BigInt_from_ASN1_OCTET_STRING(skid);
					ASN1_OCTET_STRING_free(skid);
				}

				AUTHORITY_KEYID *akid = (AUTHORITY_KEYID*)X509_get_ext_d2i(handle, (int)X509ExtensionKey::AuthorityKeyIdentifier, NULL, NULL);
				if (akid) {
					_out.authKeyId = Get_BigInt_from_ASN1_OCTET_STRING(akid->keyid);
					AUTHORITY_KEYID_free(akid);
				}

				sl_int32 i;
				CERTIFICATEPOLICIES *ext_cpols = (CERTIFICATEPOLICIES*)X509_get_ext_d2i(handle, (int)X509ExtensionKey::CertificatePolicies, &i, NULL);
				if (ext_cpols) {
					POLICYINFO *pinfo;

					for (i = 0; i < sk_POLICYINFO_num(ext_cpols); i++) {
						pinfo = sk_POLICYINFO_value(ext_cpols, i);
						X509Policy policy;
						policy.identifier = Get_String_from_ASN1_OBJECT(pinfo->policyid);
						POLICYQUALINFO *qualinfo;
						for (int j = 0; i < sk_POLICYQUALINFO_num(pinfo->qualifiers); i++) {
							qualinfo = sk_POLICYQUALINFO_value(pinfo->qualifiers, i);
							PolicyQualifierID nid = (PolicyQualifierID)OBJ_obj2nid(qualinfo->pqualid);
							if (nid == PolicyQualifierID::Qualifier_CPS) {
								String str = Get_String_from_ASN1_STRING(qualinfo->d.cpsuri);
								policy.qualifiers.add(PolicyQualifierID::Qualifier_CPS, str);
							} else if (nid == PolicyQualifierID::Qualifier_User_Notice) {
								USERNOTICE *notice = qualinfo->d.usernotice;
								if (notice) {
									String str = Get_String_from_ASN1_STRING(notice->exptext);
									policy.qualifiers.add(PolicyQualifierID::Qualifier_User_Notice, str);
								}
								
							}
						}
						_out.certPolicies.add_NoLock(policy);
					}
					sk_POLICYINFO_pop_free(ext_cpols, POLICYINFO_free);
				}
				
				AUTHORITY_INFO_ACCESS *authInfo = (AUTHORITY_INFO_ACCESS*)X509_get_ext_d2i(handle, (int)X509ExtensionKey::AuthorityInfoAccess, NULL, NULL);
				if (authInfo) {
					ACCESS_DESCRIPTION *desc;
					for (i = 0; i < sk_ACCESS_DESCRIPTION_num(authInfo); i++) {
						desc = sk_ACCESS_DESCRIPTION_value(authInfo, i);
						X509AuthorityInformation info;

						info.method = OBJ_obj2nid(desc->method);
						info.generalNameType = desc->location->type;
						info.generalName = "<unsupported>";
						switch (info.generalNameType) {
						case GEN_EMAIL:
						case GEN_DNS:
						case GEN_URI:
							info.generalName = Get_String_from_ASN1_STRING(desc->location->d.ia5);
							break;
						}

						_out.authorityInfo.add_NoLock(info);
					}
					AUTHORITY_INFO_ACCESS_free(authInfo);
				}
			}
			
			static sl_bool Get_X509(X509& _out, ::X509* handle)
			{
				if (!handle) {
					return sl_false;
				}
				_out.version = (sl_uint32)(X509_get_version(handle));
				_out.serialNumber = Get_BigInt_from_ASN1_INTEGER(X509_get0_serialNumber(handle));
				const X509_ALGOR* algorithm;
				X509_get0_signature(sl_null, &algorithm, handle);
				if (algorithm) {
					_out.signatureAlgorithm = Get_String_from_ASN1_OBJECT(algorithm->algorithm);
				}
				_out.subject = Get_Map_from_X509_NAME<X509SubjectKey>(X509_get_subject_name(handle));
				_out.issuer = Get_Map_from_X509_NAME<X509SubjectKey>(X509_get_issuer_name(handle));
				_out.validFrom = Get_Time_from_ASN1_TIME(X509_get0_notBefore(handle));
				_out.validTo = Get_Time_from_ASN1_TIME(X509_get0_notAfter(handle));

				Get_X509_Extension(_out, handle);
				return Get_PublicKey_from_EVP_PKEY(_out.key, X509_get0_pubkey(handle));
			}

			static void Set_X509_Extension(const X509& in, ::X509* handle)
			{
				BASIC_CONSTRAINTS *basicConstraint = BASIC_CONSTRAINTS_new();
				if (basicConstraint) {
					basicConstraint->ca = in.basicCA;
					X509_add1_ext_i2d(handle, (int)X509ExtensionKey::BasicConstraints, basicConstraint, 1, 0);
					BASIC_CONSTRAINTS_free(basicConstraint);
				}

				EXTENDED_KEY_USAGE *extendKeyUsage = EXTENDED_KEY_USAGE_new();
				if (extendKeyUsage) {
					for (auto nid : in.usages) {
						ASN1_OBJECT* obj = OBJ_nid2obj(nid);
						sk_ASN1_OBJECT_push(extendKeyUsage, obj);
					}
					X509_add1_ext_i2d(handle, (int)X509ExtensionKey::ExtendedKeyUsage, extendKeyUsage, 0, 0);
					EXTENDED_KEY_USAGE_free(extendKeyUsage);
				}

				ASN1_BIT_STRING* bitString = ASN1_BIT_STRING_new();
				if (bitString) {
					Memory mem = Memory::create(in.keyUsage.getLength() / 2);
					in.keyUsage.parseHexString(mem.getData());
					ASN1_BIT_STRING_set(bitString, (sl_uint8*)mem.getData(), mem.getSize());
					X509_add1_ext_i2d(handle, (int)X509ExtensionKey::KeyUsage, bitString, 1, 0);
					ASN1_BIT_STRING_free(bitString);
				}


				ASN1_OCTET_STRING* skid = Get_ASN1_OCTET_STRING_from_BigInt(in.subjectKeyId);
				if (skid) {
					X509_add1_ext_i2d(handle, (int)X509ExtensionKey::SubjectKeyIdentifier, skid, 0, 0);
					ASN1_OCTET_STRING_free(skid);
				}

				AUTHORITY_KEYID *akid = AUTHORITY_KEYID_new();
				if (akid) {
					akid->keyid = Get_ASN1_OCTET_STRING_from_BigInt(in.authKeyId);
					X509_add1_ext_i2d(handle, (int)X509ExtensionKey::AuthorityKeyIdentifier, akid, 0, 0);
					ASN1_OCTET_STRING_free(skid);
				}

				CERTIFICATEPOLICIES *certPolicies = CERTIFICATEPOLICIES_new();
				if (certPolicies) {
					for (auto policy : in.certPolicies) {
						POLICYINFO *pinfo = POLICYINFO_new();
						pinfo->policyid = OBJ_txt2obj(policy.identifier.getData(), 0);

						auto qualify = policy.qualifiers.getFirstNode();
						while (qualify) {
							POLICYQUALINFO* qualInfo = POLICYQUALINFO_new();
							if (qualInfo) {
								qualInfo->pqualid = OBJ_nid2obj((sl_int32)qualify->key);
								if (qualify->key == PolicyQualifierID::Qualifier_CPS) {
									qualInfo->d.cpsuri = (ASN1_IA5STRING*)Get_ASN1_STRING_from_String(qualify->value);
								} else if (qualify->key == PolicyQualifierID::Qualifier_User_Notice) {
									qualInfo->d.usernotice = USERNOTICE_new();
									if (qualInfo->d.usernotice) {
										qualInfo->d.usernotice->exptext = Get_ASN1_STRING_from_String(qualify->value);
									}
								}
								sk_POLICYQUALINFO_push(pinfo->qualifiers, qualInfo);
							}
							qualify = qualify->next;
						}

						sk_POLICYINFO_push(certPolicies, pinfo);
					}
					X509_add1_ext_i2d(handle, (int)X509ExtensionKey::CertificatePolicies, certPolicies, 0, 0);

					CERTIFICATEPOLICIES_free(certPolicies);
				}

				AUTHORITY_INFO_ACCESS *authorityInfo = AUTHORITY_INFO_ACCESS_new();
				if (authorityInfo) {
					for (auto info : in.authorityInfo) {
						ACCESS_DESCRIPTION* desc = ACCESS_DESCRIPTION_new();
						if (desc) {
							desc->method = OBJ_nid2obj(info.method);
							switch (info.generalNameType) {
							case GEN_EMAIL:
							case GEN_DNS:
							case GEN_URI:
								desc->location->type = info.generalNameType;
								desc->location->d.ia5 = Get_ASN1_STRING_from_String(info.generalName);
								break;
							}
							sk_ACCESS_DESCRIPTION_push(authorityInfo, desc);
						}
					}

					X509_add1_ext_i2d(handle, (int)X509ExtensionKey::AuthorityInfoAccess, authorityInfo, 0, 0);
					AUTHORITY_INFO_ACCESS_free(authorityInfo);
				}
			}

			static ::X509* Get_X509_Handle(const X509& _in)
			{
				::X509* handle = X509_new();
				if (!handle) {
					return sl_null;
				}
				X509_set_version(handle, 2);
				ASN1_INTEGER* sn = Get_ASN1_INTEGER_from_BigInt(_in.serialNumber);
				if (sn) {
					X509_set_serialNumber(handle, sn);
					ASN1_INTEGER_free(sn);
					X509_NAME* subject = Get_X509_NAME_from_HashMap<X509SubjectKey>(_in.subject);
					if (subject) {
						X509_set_subject_name(handle, subject);
						X509_NAME_free(subject);
						X509_NAME* issuer = Get_X509_NAME_from_HashMap<X509SubjectKey>(_in.issuer);
						if (issuer) {
							X509_set_issuer_name(handle, issuer);
							X509_NAME_free(issuer);
							Set_Time_to_ASN1_TIME(X509_getm_notBefore(handle), _in.validFrom);
							Set_Time_to_ASN1_TIME(X509_getm_notAfter(handle), _in.validTo);
							EVP_PKEY* pkey = Get_EVP_PKEY_from_PublicKey(_in.key);
							if (pkey) {
								X509_set_pubkey(handle, pkey);
								EVP_PKEY_free(pkey);
								Set_X509_Extension(_in, handle);
								return handle;
							}
						}
					}
				}

				X509_free(handle);
				return sl_null;
			}

			static Memory Save_X509(::X509* handle)
			{
				sl_int32 size = (sl_int32)(i2d_X509(handle, sl_null));
				if (size > 0) {
					Memory ret = Memory::create(size);
					if (ret.isNotNull()) {
						unsigned char* buf = (unsigned char*)(ret.getData());
						if (i2d_X509(handle, &buf) == size) {
							return ret;
						}
					}
				}
				return sl_null;
			}

			static Memory Sign_X509(const X509& cert, const PrivateKey& issuerKey, const EVP_MD* md)
			{
				EVP_PKEY* key = Get_EVP_PKEY_from_PrivateKey(issuerKey);
				if (key) {
					Memory ret;
					::X509* handle = Get_X509_Handle(cert);
					if (handle) {
						if (X509_sign(handle, key, md)) {
							ret = Save_X509(handle);
						}
						X509_free(handle);
					}
					EVP_PKEY_free(key);
					return ret;
				}
				return sl_null;
			}


			static int Do_Sign_Init(EVP_MD_CTX *ctx, EVP_PKEY *pkey, const EVP_MD *md)
			{
				EVP_PKEY_CTX *pkctx = NULL;
				sl_int32 def_nid;

				/*
				* EVP_PKEY_get_default_digest_nid() returns 2 if the digest is mandatory
				* for this algorithm.
				*/
				if (EVP_PKEY_get_default_digest_nid(pkey, &def_nid) == 2
					&& def_nid == NID_undef) {
					/* The signing algorithm requires there to be no digest */
					md = NULL;
				}
				if (!EVP_DigestSignInit(ctx, &pkctx, md, NULL, pkey))
					return 0;
				
				return 1;
			}

			sl_int32 Do_X509_Sign(::X509 *x, EVP_PKEY *pkey, const EVP_MD *md)
			{
				int rv;
				EVP_MD_CTX *mctx = EVP_MD_CTX_new();

				rv = Do_Sign_Init(mctx, pkey, md);
				if (rv > 0)
					rv = X509_sign_ctx(x, mctx);
				EVP_MD_CTX_free(mctx);
				return rv > 0 ? 1 : 0;
			}

			static Memory Cert_X509(const X509& cert, const X509& issuer, const PrivateKey& issuerKey, const EVP_MD* md)
			{
				Memory ret = sl_null;
				::X509* handleCert = Get_X509_Handle(cert);
				if (!handleCert) {
					return ret;
				}
				::X509* handleIssuer = Get_X509_Handle(issuer);
				if (!handleIssuer) {
					X509_free(handleCert);
					return ret;
				}
				if (!X509_set_issuer_name(handleCert, X509_get_subject_name(handleIssuer))) {
					X509_free(handleIssuer);
					X509_free(handleCert);
					return ret;
				}
				EVP_PKEY *_issuerKey = Get_EVP_PKEY_from_PrivateKey(issuerKey);
				if (_issuerKey) {
					if (X509_check_private_key(handleIssuer, _issuerKey)) {
						X509_STORE_CTX *xsc = X509_STORE_CTX_new();
						if (xsc) {
							X509_STORE* ctx = X509_STORE_new();
							if (ctx) {
								if (X509_STORE_set_default_paths(ctx)) {
									if (X509_STORE_CTX_init(xsc, ctx, handleCert, NULL)) {
										X509_STORE_CTX_set_cert(xsc, handleCert);
										X509_STORE_CTX_set_flags(xsc, X509_V_FLAG_CHECK_SS_SIGNATURE);
										X509_verify_cert(xsc);

										X509V3_CTX ctx2;
										X509_set_version(handleCert, 2); /* version 3 certificate */
										X509V3_set_ctx(&ctx2, handleIssuer, handleCert, NULL, NULL, 0);
										if (Do_X509_Sign(handleCert, _issuerKey, md)) {
											ret = Save_X509(handleCert);
										}
									}
								}
								X509_STORE_free(ctx);
							}

							X509_STORE_CTX_free(xsc);
						}
					}
					EVP_PKEY_free(_issuerKey);
				}
				X509_free(handleIssuer);
				X509_free(handleCert);
				return ret;
			}
		}
	}
	
	using namespace priv::openssl;
	
	
	OpenSSL_AES::OpenSSL_AES()
	{
		m_keyEnc = sl_null;
		m_keyDec = sl_null;
	}
	
	OpenSSL_AES::~OpenSSL_AES()
	{
		if (m_keyEnc) {
			Base::freeMemory(m_keyEnc);
		}
		if (m_keyDec) {
			Base::freeMemory(m_keyDec);
		}
	}
	
	sl_bool OpenSSL_AES::setKey(const void* key, sl_uint32 lenKey)
	{
		if (!m_keyEnc) {
			m_keyEnc = Base::createMemory(sizeof(AES_KEY));
		}
		if (AES_set_encrypt_key((unsigned char*)key, lenKey << 3, (AES_KEY*)m_keyEnc) != 0) {
			return sl_false;
		}
		if (!m_keyDec) {
			m_keyDec = Base::createMemory(sizeof(AES_KEY));
		}
		if (AES_set_decrypt_key((unsigned char*)key, lenKey << 3, (AES_KEY*)m_keyDec) != 0) {
			return sl_false;
		}
		return sl_true;
	}
	
	void OpenSSL_AES::setKey_SHA256(const String& key)
	{
		char sig[32];
		SHA256::hash(key, sig);
		setKey(sig, 32);
	}
	
	void OpenSSL_AES::encryptBlock(const void* src, void* dst) const
	{
		AES_encrypt((unsigned char*)src, (unsigned char*)dst, (AES_KEY*)m_keyEnc);
	}
	
	void OpenSSL_AES::decryptBlock(const void* src, void* dst) const
	{
		AES_decrypt((unsigned char*)src, (unsigned char*)dst, (AES_KEY*)m_keyDec);
	}


	SLIB_DEFINE_OBJECT(OpenSSL_Key, Object)
	
	OpenSSL_Key::OpenSSL_Key()
	{
		m_key = sl_null;
	}

	OpenSSL_Key::~OpenSSL_Key()
	{
		if (m_key) {
			EVP_PKEY_free(m_key);
		}
	}

	Ref<OpenSSL_Key> OpenSSL_Key::createPublicKey(const StringParam& _pem)
	{
		BIO* bio = BIO_new(BIO_s_mem());
		if (bio) {
			StringData pem(_pem);
			BIO_write(bio, pem.getData(), (int)(pem.getLength()));
			EVP_PKEY* key = PEM_read_bio_PUBKEY(bio, sl_null, sl_null, sl_null);
			BIO_free(bio);
			if (key) {
				Ref<OpenSSL_Key> ret = new OpenSSL_Key;
				if (ret.isNotNull()) {
					ret->m_key = key;
					return ret;
				}
				EVP_PKEY_free(key);
			}
		}
		return sl_null;
	}

	Ref<OpenSSL_Key> OpenSSL_Key::createPrivateKey(const StringParam& _pem)
	{
		BIO* bio = BIO_new(BIO_s_mem());
		if (bio) {
			StringData pem(_pem);
			BIO_write(bio, pem.getData(), (int)(pem.getLength()));
			EVP_PKEY* key = PEM_read_bio_PrivateKey(bio, sl_null, sl_null, sl_null);
			BIO_free(bio);
			if (key) {
				Ref<OpenSSL_Key> ret = new OpenSSL_Key;
				if (ret.isNotNull()) {
					ret->m_key = key;
					return ret;
				}
				EVP_PKEY_free(key);
			}
		}
		return sl_null;
	}

	Memory OpenSSL_Key::sign_RSA_SHA256(const void* data, sl_size sizeData)
	{
		return Generate_RSA_Signature(m_key, EVP_sha256(), data, sizeData);
	}
	
	sl_bool OpenSSL_Key::verify_RSA_SHA256(const void* data, sl_size sizeData, const void* signature, sl_size sizeSignature)
	{
		return Verify_RSA_Signature(m_key, EVP_sha256(), data, sizeData, signature, sizeSignature);
	}

	Memory OpenSSL_Key::sign_RSA_SHA384(const void* data, sl_size sizeData)
	{
		return Generate_RSA_Signature(m_key, EVP_sha384(), data, sizeData);
	}

	sl_bool OpenSSL_Key::verify_RSA_SHA384(const void* data, sl_size sizeData, const void* signature, sl_size sizeSignature)
	{
		return Verify_RSA_Signature(m_key, EVP_sha384(), data, sizeData, signature, sizeSignature);
	}

	Memory OpenSSL_Key::sign_RSA_SHA512(const void* data, sl_size sizeData)
	{
		return Generate_RSA_Signature(m_key, EVP_sha512(), data, sizeData);
	}

	sl_bool OpenSSL_Key::verify_RSA_SHA512(const void* data, sl_size sizeData, const void* signature, sl_size sizeSignature)
	{
		return Verify_RSA_Signature(m_key, EVP_sha512(), data, sizeData, signature, sizeSignature);
	}

	Memory OpenSSL_Key::sign_RSA_PSS_SHA256(const void* data, sl_size sizeData)
	{
		unsigned char h[32];
		SHA256::hash(data, sizeData, h);
		return Generate_RSA_PSS_Signature(m_key, EVP_sha256(), h, 32);
	}

	sl_bool OpenSSL_Key::verify_RSA_PSS_SHA256(const void* data, sl_size sizeData, const void* signature, sl_size sizeSignature)
	{
		unsigned char h[32];
		SHA256::hash(data, sizeData, h);
		return Verify_RSA_PSS_Signature(m_key, EVP_sha256(), h, 32, signature, sizeSignature);
	}

	Memory OpenSSL_Key::sign_RSA_PSS_SHA384(const void* data, sl_size sizeData)
	{
		unsigned char h[48];
		SHA384::hash(data, sizeData, h);
		return Generate_RSA_PSS_Signature(m_key, EVP_sha384(), h, 48);
	}

	sl_bool OpenSSL_Key::verify_RSA_PSS_SHA384(const void* data, sl_size sizeData, const void* signature, sl_size sizeSignature)
	{
		unsigned char h[48];
		SHA384::hash(data, sizeData, h);
		return Verify_RSA_PSS_Signature(m_key, EVP_sha384(), h, 48, signature, sizeSignature);
	}

	Memory OpenSSL_Key::sign_RSA_PSS_SHA512(const void* data, sl_size sizeData)
	{
		unsigned char h[64];
		SHA384::hash(data, sizeData, h);
		return Generate_RSA_PSS_Signature(m_key, EVP_sha512(), h, 64);
	}

	sl_bool OpenSSL_Key::verify_RSA_PSS_SHA512(const void* data, sl_size sizeData, const void* signature, sl_size sizeSignature)
	{
		unsigned char h[64];
		SHA384::hash(data, sizeData, h);
		return Verify_RSA_PSS_Signature(m_key, EVP_sha512(), h, 64, signature, sizeSignature);
	}

	Memory OpenSSL_Key::sign_ECDSA_SHA256(const void* data, sl_size sizeData)
	{
		unsigned char h[32];
		SHA256::hash(data, sizeData, h);
		return Generate_ECDSA_Signature(m_key, h, 32);
	}

	sl_bool OpenSSL_Key::verify_ECDSA_SHA256(const void* data, sl_size sizeData, const void* signature, sl_size sizeSignature)
	{
		unsigned char h[32];
		SHA256::hash(data, sizeData, h);
		return Verify_ECDSA_Signature(m_key, h, 32, signature, sizeSignature);
	}
	
	Memory OpenSSL_Key::sign_ECDSA_SHA384(const void* data, sl_size sizeData)
	{
		unsigned char h[48];
		SHA384::hash(data, sizeData, h);
		return Generate_ECDSA_Signature(m_key, h, 48);
	}

	sl_bool OpenSSL_Key::verify_ECDSA_SHA384(const void* data, sl_size sizeData, const void* signature, sl_size sizeSignature)
	{
		unsigned char h[48];
		SHA384::hash(data, sizeData, h);
		return Verify_ECDSA_Signature(m_key, h, 48, signature, sizeSignature);
	}

	Memory OpenSSL_Key::sign_ECDSA_SHA512(const void* data, sl_size sizeData)
	{
		unsigned char h[64];
		SHA512::hash(data, sizeData, h);
		return Generate_ECDSA_Signature(m_key, h, 64);
	}

	sl_bool OpenSSL_Key::verify_ECDSA_SHA512(const void* data, sl_size sizeData, const void* signature, sl_size sizeSignature)
	{
		unsigned char h[64];
		SHA512::hash(data, sizeData, h);
		return Verify_ECDSA_Signature(m_key, h, 64, signature, sizeSignature);
	}

	sl_bool OpenSSL::isProbablePrime(const void* num_BigEndian, sl_uint32 nBytes, sl_bool* pFlagError)
	{
		initThread();
		if (!nBytes) {
			if (pFlagError) {
				*pFlagError = sl_false;
			}
			return sl_false;
		}
		BIGNUM* num = BN_bin2bn((unsigned char*)num_BigEndian, nBytes, sl_null);
		if (num) {
			int ret = BN_is_prime_fasttest_ex(num, 0, sl_null, sl_false, sl_null);
			BN_free(num);
			if (pFlagError) {
				if (ret < 0) {
					*pFlagError = sl_true;
				} else {
					*pFlagError = sl_false;
				}
			}
			return ret == 1;
		} else {
			if (pFlagError) {
				*pFlagError = sl_true;
			}
		}
		return sl_false;
	}
	
	Memory OpenSSL::generatePrime(sl_uint32 nBits)
	{
		initThread();
		BIGNUM* prime = BN_new();
		if (BN_generate_prime_ex(prime, (int)nBits, sl_false, sl_null, sl_null, sl_null)) {
			sl_size n = (sl_size)(BN_num_bytes(prime));
			Memory ret = Memory::create(n);
			if (ret.isNotNull()) {
				BN_bn2bin(prime, (unsigned char*)(ret.getData()));
			}
			BN_free(prime);
			return ret;
		}
		return sl_null;
	}
	
	sl_bool OpenSSL::randomBytes(void* bytes, sl_uint32 nBytes, sl_bool flagPrivate)
	{
		initThread();
		if (flagPrivate) {
			if (RAND_priv_bytes((unsigned char*)bytes, (int)nBytes) == 1) {
				return sl_true;
			}
		} else {
			if (RAND_bytes((unsigned char*)bytes, (int)nBytes) == 1) {
				return sl_true;
			}
		}
		return sl_false;
	}

	void OpenSSL::generateRSA(RSAPrivateKey& key, sl_uint32 nBits)
	{
		initThread();
		sl_uint32 h = nBits >> 1;
		nBits = h << 1;
		for (;;) {
			key.P = BigInt::fromBytesBE(generatePrime(h));
			key.Q = BigInt::fromBytesBE(generatePrime(h));
			if (key.generateFromPrimes(nBits)) {
				return;
			}
		}
	}

	sl_bool OpenSSL::generate_ECKey(EllipticCurveId curveId, ECPrivateKey& _output)
	{
		initThread();
		sl_bool flagSuccess = sl_false;
		EC_GROUP* group = EC_GROUP_new_by_curve_name((int)curveId);
		if (group) {
			EC_KEY* key = EC_KEY_new();
			if (key) {
				flagSuccess = sl_true;
				EC_KEY_set_group(key, group);
				for (;;) {
					if (1 == EC_KEY_generate_key(key)) {
						const BIGNUM* p = EC_KEY_get0_private_key(key);
						_output.d = Get_BigInt_from_BIGNUM(p);
						if (_output.d.isNull()) {
							flagSuccess = sl_false;
							break;
						}
						const EC_POINT* Q = EC_KEY_get0_public_key(key);
						_output.Q = Get_ECPoint_from_EC_POINT(group, Q);
						if (_output.Q.isO()) {
							flagSuccess = sl_false;
							break;
						}
						break;
					}
				}
				EC_KEY_free(key);
			}
			EC_GROUP_free(group);
		}
		return flagSuccess;
	}
	
	sl_bool OpenSSL::check_ECKey(EllipticCurveId curveId, const ECPublicKey& key)
	{
		initThread();
		sl_bool flagSuccess = sl_false;
		EC_GROUP* group = EC_GROUP_new_by_curve_name((int)curveId);
		if (group) {
			EC_KEY* ek = Get_EC_KEY_from_ECPublicKey(group, key);
			if (ek) {
				flagSuccess = EC_KEY_check_key(ek) == 1;
				EC_KEY_free(ek);
			}
			EC_GROUP_free(group);
		}
		return flagSuccess;
	}

	ECDSA_Signature OpenSSL::sign_ECDSA(EllipticCurveId curveId, const ECPrivateKey& key, const BigInt& z)
	{
		Memory mem = z.getBytesBE();
		if (mem.isNull()) {
			return ECDSA_Signature();
		}
		return Do_sign_ECDSA(curveId, key, mem.getData(), mem.getSize());
	}
	
	ECDSA_Signature OpenSSL::sign_ECDSA(EllipticCurveId curveId, const ECPrivateKey& key, const void* hash, sl_size size)
	{
		return Do_sign_ECDSA(curveId, key, hash, size);
	}

	ECDSA_Signature OpenSSL::sign_ECDSA_SHA256(EllipticCurveId curveId, const ECPrivateKey& key, const void* data, sl_size size)
	{
		sl_uint8 hash[SHA256::HashSize];
		SHA256::hash(data, size, hash);
		return Do_sign_ECDSA(curveId, key, hash, sizeof(hash));
	}

	ECDSA_Signature OpenSSL::sign_ECDSA_SHA384(EllipticCurveId curveId, const ECPrivateKey& key, const void* data, sl_size size)
	{
		sl_uint8 hash[SHA384::HashSize];
		SHA384::hash(data, size, hash);
		return Do_sign_ECDSA(curveId, key, hash, sizeof(hash));
	}

	ECDSA_Signature OpenSSL::sign_ECDSA_SHA512(EllipticCurveId curveId, const ECPrivateKey& key, const void* data, sl_size size)
	{
		sl_uint8 hash[SHA512::HashSize];
		SHA512::hash(data, size, hash);
		return Do_sign_ECDSA(curveId, key, hash, sizeof(hash));
	}

	sl_bool OpenSSL::verify_ECDSA(EllipticCurveId curveId, const ECPublicKey& key, const BigInt& z, const ECDSA_Signature& signature)
	{
		Memory mem = z.getBytesBE();
		if (mem.isNull()) {
			return sl_false;
		}
		return Do_verify_ECDSA(curveId, key, mem.getData(), mem.getSize(), signature);
	}
	
	sl_bool OpenSSL::verify_ECDSA(EllipticCurveId curveId, const ECPublicKey& key, const void* hash, sl_size size, const ECDSA_Signature& signature)
	{
		return Do_verify_ECDSA(curveId, key, hash, size, signature);
	}
	
	sl_bool OpenSSL::verify_ECDSA_SHA256(EllipticCurveId curveId, const ECPublicKey& key, const void* data, sl_size size, const ECDSA_Signature& signature)
	{
		sl_uint8 hash[SHA256::HashSize];
		SHA256::hash(data, size, hash);
		return Do_verify_ECDSA(curveId, key, hash, sizeof(hash), signature);
	}

	sl_bool OpenSSL::verify_ECDSA_SHA384(EllipticCurveId curveId, const ECPublicKey& key, const void* data, sl_size size, const ECDSA_Signature& signature)
	{
		sl_uint8 hash[SHA384::HashSize];
		SHA384::hash(data, size, hash);
		return Do_verify_ECDSA(curveId, key, hash, sizeof(hash), signature);
	}

	sl_bool OpenSSL::verify_ECDSA_SHA512(EllipticCurveId curveId, const ECPublicKey& key, const void* data, sl_size size, const ECDSA_Signature& signature)
	{
		sl_uint8 hash[SHA512::HashSize];
		SHA512::hash(data, size, hash);
		return Do_verify_ECDSA(curveId, key, hash, sizeof(hash), signature);
	}

	BigInt OpenSSL::getSharedKey_ECDH(EllipticCurveId curveId, const ECPrivateKey& keyLocal, const ECPublicKey& keyRemote)
	{
		initThread();
		BigInt ret;
		EC_GROUP* group = EC_GROUP_new_by_curve_name((int)curveId);
		if (group) {
			BIGNUM* priv = Get_BIGNUM_from_BigInt(keyLocal.d);
			if (priv) {
				EC_POINT* pub = Get_EC_POINT_from_ECPoint(group, keyRemote.Q);
				if (pub) {
					EC_POINT* pt = EC_POINT_new(group);
					if (pt) {
						if (EC_POINT_mul(group, pt, sl_null, pub, priv, sl_null)) {
							BIGNUM* x = BN_new();
							if (x) {
								if (EC_POINT_get_affine_coordinates(group, pt, x, sl_null, sl_null)) {
									ret = Get_BigInt_from_BIGNUM(x);
								}
								BN_free(x);
							}
						}
						EC_POINT_free(pt);
					}
					EC_POINT_free(pub);
				}
				BN_free(priv);
			}
			EC_GROUP_free(group);
		}
		return ret;
	}

	sl_bool OpenSSL::loadX509(X509& _out, const void* content, sl_size size)
	{
		const sl_uint8* buf = (sl_uint8*)content;
		::X509* handle = d2i_X509(sl_null, &buf, (long)size);
		if (handle) {
			sl_bool bRet = Get_X509(_out, handle);
			X509_free(handle);
			return bRet;
		}
		return sl_false;
	}

	sl_bool OpenSSL::loadX509(X509& _out, const Memory& memory)
	{
		return loadX509(_out, memory.getData(), memory.getSize());
	}

	sl_bool OpenSSL::loadX509File(X509& _out, const StringParam& filePath)
	{
		Memory mem = File::readAllBytes(filePath);
		return loadX509(_out, mem.getData(), mem.getSize());
	}

	Memory OpenSSL::signX509_SHA256(const X509& cert, const PrivateKey& issuerKey)
	{
		return Sign_X509(cert, issuerKey, EVP_sha256());
	}

	Memory OpenSSL::signX509_SHA384(const X509& cert, const PrivateKey& issuerKey)
	{
		return Sign_X509(cert, issuerKey, EVP_sha384());
	}

	Memory OpenSSL::signX509_SHA512(const X509& cert, const PrivateKey& issuerKey)
	{
		return Sign_X509(cert, issuerKey, EVP_sha512());
	}

	Memory OpenSSL::certX509_SHA256(const X509& cert, const X509& issuer, const PrivateKey& issuerKey)
	{
		return Cert_X509(cert, issuer, issuerKey, EVP_sha256());
	}

	Memory OpenSSL::certX509_SHA384(const X509& cert, const X509& issuer, const PrivateKey& issuerKey)
	{
		return Cert_X509(cert, issuer, issuerKey, EVP_sha384());
	}

	Memory OpenSSL::certX509_SHA512(const X509& cert, const X509& issuer, const PrivateKey& issuerKey)
	{
		return Cert_X509(cert, issuer, issuerKey, EVP_sha512());
	}
}
