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

#include "slib/crypto/openssl.h"

#include "slib/crypto/sha2.h"
#include "slib/core/file.h"
#include "slib/core/time_zone.h"
#include "slib/core/handle_container.h"
#include "slib/core/scoped_buffer.h"

#include "openssl/aes.h"
#include "openssl/rand.h"
#include "openssl/ec.h"
#include "openssl/x509v3.h"
#include "openssl/asn1.h"
#include "openssl/pkcs12.h"
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
			void InitThread();
#else
			static void InitThread()
			{
			}
#endif

			class SLIB_EXPORT X509_Handle
			{
			public:
				SLIB_DEFINE_HANDLE_CONTAINER_TEMPLATE_MEMBERS(X509_Handle, ::X509*, handle, sl_null, X509_free)
			};

			class SLIB_EXPORT Stack_X509_Handle
			{
			public:
				SLIB_DEFINE_HANDLE_CONTAINER_TEMPLATE_MEMBERS(Stack_X509_Handle, stack_st_X509*, handle, sl_null, sk_X509_free)
			};

			class SLIB_EXPORT PKCS12_Handle
			{
			public:
				SLIB_DEFINE_HANDLE_CONTAINER_TEMPLATE_MEMBERS(PKCS12_Handle, ::PKCS12*, handle, sl_null, PKCS12_free)
			};

			class SLIB_EXPORT EVP_PKEY_Handle
			{
			public:
				SLIB_DEFINE_HANDLE_CONTAINER_TEMPLATE_MEMBERS(EVP_PKEY_Handle, EVP_PKEY*, handle, sl_null, EVP_PKEY_free)
			};

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
			
			static ECDSA_Signature Do_sign_ECDSA(const EllipticCurve& curve, const ECPrivateKey& key, const void* hash, sl_size size)
			{
				InitThread();
				ECDSA_Signature ret;
				EC_GROUP* group = Get_EC_GROUP_from_EllipticCurve(curve);
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
			
			static sl_bool Do_verify_ECDSA(const EllipticCurve& curve, const ECPublicKey& key, const void* hash, sl_size size, const ECDSA_Signature& _sig)
			{
				InitThread();
				sl_bool flagVerified = sl_false;
				EC_GROUP* group = Get_EC_GROUP_from_EllipticCurve(curve);
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
					ASN1_STRING_set(ret, str.getData(), (int)(str.getLength()));
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
					ASN1_OCTET_STRING_set(ret, (sl_uint8*)mem.getData(), (int)(mem.getSize()));
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
			
			static ::X509* Load_X509(const void* content, sl_size size)
			{
				if (content && size) {
					const sl_uint8* buf = (sl_uint8*)content;
					return d2i_X509_AUX(sl_null, &buf, (long)size);
				}
				return sl_null;
			}

			static sl_bool Get_X509(X509& _out, ::X509* handle)
			{
				if (!handle) {
					return sl_false;
				}

				// Version 1 Fields
				_out.version = (sl_uint32)(X509_get_version(handle));
				_out.serialNumber = Get_BigInt_from_ASN1_INTEGER(X509_get0_serialNumber(handle));
				_out.subject = Get_Map_from_X509_NAME<X509SubjectKey>(X509_get_subject_name(handle));
				_out.issuer = Get_Map_from_X509_NAME<X509SubjectKey>(X509_get_issuer_name(handle));
				_out.validFrom = Get_Time_from_ASN1_TIME(X509_get0_notBefore(handle));
				_out.validTo = Get_Time_from_ASN1_TIME(X509_get0_notAfter(handle));

				// Extensions
				BASIC_CONSTRAINTS* basicConstraints = (BASIC_CONSTRAINTS*)(X509_get_ext_d2i(handle, NID_basic_constraints, sl_null, sl_null));
				if (basicConstraints) {
					_out.flagEndEntity = !(basicConstraints->ca);
					BASIC_CONSTRAINTS_free(basicConstraints);
				}
				EXTENDED_KEY_USAGE* extendedKeyUsage = (EXTENDED_KEY_USAGE*)(X509_get_ext_d2i(handle, NID_ext_key_usage, sl_null, sl_null));
				if (extendedKeyUsage) {
					int n = sk_ASN1_OBJECT_num(extendedKeyUsage);
					for (int i = 0; i < n; i++) {
						int nid = OBJ_obj2nid(sk_ASN1_OBJECT_value(extendedKeyUsage, i));
						if (nid != NID_undef) {
							_out.enhancedKeyUsages.add_NoLock((X509EnhancedKeyUsage)nid);
						}
					}
					sk_ASN1_OBJECT_pop_free(extendedKeyUsage, ASN1_OBJECT_free);
				}
				ASN1_BIT_STRING* keyUsage = (ASN1_BIT_STRING*)(X509_get_ext_d2i(handle, NID_key_usage, sl_null, sl_null));
				if (keyUsage) {
					sl_uint32 flags;
					if (keyUsage->length > 0) {
						flags = keyUsage->data[0];
						if (keyUsage->length > 1) {
							flags |= ((sl_uint32)(keyUsage->data[1])) << 8;
						}
					} else {
						flags = 0;
					}
					_out.keyUsages.value = flags;
					ASN1_BIT_STRING_free(keyUsage);
				}
				ASN1_OCTET_STRING* subjectKeyId = (ASN1_INTEGER*)(X509_get_ext_d2i(handle, NID_subject_key_identifier, sl_null, sl_null));
				if (subjectKeyId) {
					_out.subjectKeyId = Get_BigInt_from_ASN1_OCTET_STRING(subjectKeyId);
					ASN1_OCTET_STRING_free(subjectKeyId);
				}
				AUTHORITY_KEYID* authorityKeyId = (AUTHORITY_KEYID*)(X509_get_ext_d2i(handle, NID_authority_key_identifier, sl_null, sl_null));
				if (authorityKeyId) {
					_out.authorityKeyId = Get_BigInt_from_ASN1_OCTET_STRING(authorityKeyId->keyid);
					AUTHORITY_KEYID_free(authorityKeyId);
				}
				CERTIFICATEPOLICIES* certificatePolicies = (CERTIFICATEPOLICIES*)(X509_get_ext_d2i(handle, NID_certificate_policies, sl_null, sl_null));
				if (certificatePolicies) {
					int nPolicies = sk_POLICYINFO_num(certificatePolicies);
					for (int iPolicy = 0; iPolicy < nPolicies; iPolicy++) {
						POLICYINFO* policyInfo = sk_POLICYINFO_value(certificatePolicies, iPolicy);
						X509CertificatePolicy policy;
						policy.identifier = Get_String_from_ASN1_OBJECT(policyInfo->policyid);
						int nQualifiers = sk_POLICYQUALINFO_num(policyInfo->qualifiers);
						for (int iQualifier = 0; iQualifier < nQualifiers; iQualifier++) {
							POLICYQUALINFO* qualifierInfo = sk_POLICYQUALINFO_value(policyInfo->qualifiers, iQualifier);
							int nid = OBJ_obj2nid(qualifierInfo->pqualid);
							if (nid == NID_id_qt_cps) {
								policy.CPS = Get_String_from_ASN1_STRING(qualifierInfo->d.cpsuri);
							} else if (nid == NID_id_qt_unotice) {
								USERNOTICE* notice = qualifierInfo->d.usernotice;
								if (notice) {
									policy.userNotice = Get_String_from_ASN1_STRING(notice->exptext);
								}
							}
						}
						_out.policies.add_NoLock(Move(policy));
					}
					sk_POLICYINFO_pop_free(certificatePolicies, POLICYINFO_free);
				}
				AUTHORITY_INFO_ACCESS* infoAccess = (AUTHORITY_INFO_ACCESS*)(X509_get_ext_d2i(handle, NID_info_access, sl_null, sl_null));
				if (infoAccess) {
					int n = sk_ACCESS_DESCRIPTION_num(infoAccess);
					for (int i = 0; i < n; i++) {
						ACCESS_DESCRIPTION* desc = sk_ACCESS_DESCRIPTION_value(infoAccess, i);
						int nid = OBJ_obj2nid(desc->method);
						if (nid != NID_undef) {
							X509AuthorityInformation info;
							info.method = (X509AuthorityInformationAccessMethod)nid;
							switch (desc->location->type) {
								case GEN_EMAIL:
									info.type = X509AuthorityInformationLocationType::Email;
									info.value = Get_String_from_ASN1_STRING(desc->location->d.rfc822Name);
									break;
								case GEN_DNS:
									info.type = X509AuthorityInformationLocationType::DNS;
									info.value = Get_String_from_ASN1_STRING(desc->location->d.dNSName);
									break;
								case GEN_URI:
									info.type = X509AuthorityInformationLocationType::URI;
									info.value = Get_String_from_ASN1_STRING(desc->location->d.uniformResourceIdentifier);
									break;
								default:
									info.type = X509AuthorityInformationLocationType::Unknown;
									break;
							}
							if (info.type != X509AuthorityInformationLocationType::Unknown) {
								_out.authorityInformations.add_NoLock(Move(info));
							}
						}
					}
					AUTHORITY_INFO_ACCESS_free(infoAccess);
				}
				return Get_PublicKey_from_EVP_PKEY(_out.key, X509_get0_pubkey(handle));
			}

			static ::X509* Get_X509_Handle(const X509& _in)
			{
				::X509* handle = X509_new();
				if (!handle) {
					return sl_null;
				}
				X509_Handle hX509(handle);

				X509_set_version(handle, (long)(_in.version));

				ASN1_INTEGER* sn = Get_ASN1_INTEGER_from_BigInt(_in.serialNumber);
				if (sn) {
					X509_set_serialNumber(handle, sn);
					ASN1_INTEGER_free(sn);
				} else {
					return sl_null;
				}

				X509_NAME* subject = Get_X509_NAME_from_HashMap<X509SubjectKey>(_in.subject);
				if (subject) {
					X509_set_subject_name(handle, subject);
					X509_NAME_free(subject);
				} else {
					return sl_null;
				}

				X509_NAME* issuer = Get_X509_NAME_from_HashMap<X509SubjectKey>(_in.issuer);
				if (issuer) {
					X509_set_issuer_name(handle, issuer);
					X509_NAME_free(issuer);
				} else {
					return sl_null;
				}

				Set_Time_to_ASN1_TIME(X509_getm_notBefore(handle), _in.validFrom);
				Set_Time_to_ASN1_TIME(X509_getm_notAfter(handle), _in.validTo);

				EVP_PKEY* pkey = Get_EVP_PKEY_from_PublicKey(_in.key);
				if (pkey) {
					X509_set_pubkey(handle, pkey);
					EVP_PKEY_free(pkey);
				} else {
					return sl_null;
				}

				BASIC_CONSTRAINTS *basicConstraints = BASIC_CONSTRAINTS_new();
				if (basicConstraints) {
					basicConstraints->ca = !(_in.flagEndEntity);
					X509_add1_ext_i2d(handle, NID_basic_constraints, basicConstraints, 1, 0);
					BASIC_CONSTRAINTS_free(basicConstraints);
				} else {
					return sl_null;
				}

				ListElements<X509EnhancedKeyUsage> enhancedKeyUsages(_in.enhancedKeyUsages);
				if (enhancedKeyUsages.count) {
					EXTENDED_KEY_USAGE* hExtendedKeyUsage = EXTENDED_KEY_USAGE_new();
					if (hExtendedKeyUsage) {
						for (sl_size i = 0; i < enhancedKeyUsages.count; i++) {
							ASN1_OBJECT* obj = OBJ_nid2obj((int)(enhancedKeyUsages[i]));
							if (obj) {
								sk_ASN1_OBJECT_push(hExtendedKeyUsage, obj);
							}
						}
						X509_add1_ext_i2d(handle, NID_ext_key_usage, hExtendedKeyUsage, 0, 0);
						EXTENDED_KEY_USAGE_free(hExtendedKeyUsage);
					} else {
						return sl_null;
					}
				}

				if (_in.keyUsages.value) {
					ASN1_BIT_STRING* keyUsage = ASN1_BIT_STRING_new();
					if (keyUsage) {
						if (_in.keyUsages.value & 0xff00) {
							sl_uint8 value[2];
							value[0] = (sl_uint8)(_in.keyUsages.value);
							value[1] = (sl_uint8)(_in.keyUsages.value >> 8);
							ASN1_BIT_STRING_set(keyUsage, value, 2);
						} else {
							sl_uint8 value = (sl_uint8)(_in.keyUsages.value);
							ASN1_BIT_STRING_set(keyUsage, &value, 1);
						}
						X509_add1_ext_i2d(handle, NID_key_usage, keyUsage, 1, 0);
						ASN1_BIT_STRING_free(keyUsage);
					} else {
						return sl_null;
					}
				}

				if (_in.subjectKeyId.isNotNull()) {
					ASN1_OCTET_STRING* subjectKeyId = Get_ASN1_OCTET_STRING_from_BigInt(_in.subjectKeyId);
					if (subjectKeyId) {
						X509_add1_ext_i2d(handle, NID_subject_key_identifier, subjectKeyId, 0, 0);
						ASN1_OCTET_STRING_free(subjectKeyId);
					} else {
						return sl_null;
					}
				}

				if (_in.authorityKeyId.isNotNull()) {
					AUTHORITY_KEYID* authorityKeyId = AUTHORITY_KEYID_new();
					if (authorityKeyId) {
						authorityKeyId->keyid = Get_ASN1_OCTET_STRING_from_BigInt(_in.authorityKeyId);
						X509_add1_ext_i2d(handle, NID_authority_key_identifier, authorityKeyId, 0, 0);
						AUTHORITY_KEYID_free(authorityKeyId);
					} else {
						return sl_null;
					}
				}

				ListElements<X509CertificatePolicy> policies(_in.policies);
				if (policies.count) {
					CERTIFICATEPOLICIES* hPolicies = CERTIFICATEPOLICIES_new();
					if (hPolicies) {
						for (sl_size i = 0; i < policies.count; i++) {
							X509CertificatePolicy& policy = policies[i];
							POLICYINFO* policyInfo = POLICYINFO_new();
							if (policyInfo) {
								StringCstr str(policy.identifier);
								policyInfo->policyid = OBJ_txt2obj(policy.identifier.getData(), 0);
								policyInfo->qualifiers = sk_POLICYQUALINFO_new_null();
								if (policyInfo->qualifiers) {
									if (policy.CPS.isNotNull()) {
										POLICYQUALINFO* qualiferInfo = POLICYQUALINFO_new();
										if (qualiferInfo) {
											qualiferInfo->d.cpsuri = (ASN1_IA5STRING*)(Get_ASN1_STRING_from_String(policy.CPS));
											qualiferInfo->pqualid = OBJ_nid2obj(NID_id_qt_cps);
											sk_POLICYQUALINFO_push(policyInfo->qualifiers, qualiferInfo);
										}
									}
									if (policy.userNotice.isNotNull()) {
										POLICYQUALINFO* qualiferInfo = POLICYQUALINFO_new();
										if (qualiferInfo) {
											qualiferInfo->d.usernotice = USERNOTICE_new();
											if (qualiferInfo->d.usernotice) {
												qualiferInfo->d.usernotice->exptext = (ASN1_IA5STRING*)(Get_ASN1_STRING_from_String(policy.userNotice));
												qualiferInfo->pqualid = OBJ_nid2obj(NID_id_qt_cps);
												sk_POLICYQUALINFO_push(policyInfo->qualifiers, qualiferInfo);
											}
										}
									}
								}
							}
							sk_POLICYINFO_push(hPolicies, policyInfo);
						}
						X509_add1_ext_i2d(handle, NID_certificate_policies, hPolicies, 0, 0);
						CERTIFICATEPOLICIES_free(hPolicies);
					} else {
						return sl_null;
					}
				}

				ListElements<X509AuthorityInformation> authorityInformations(_in.authorityInformations);
				if (authorityInformations.count) {
					AUTHORITY_INFO_ACCESS* hAuthorityInfo = AUTHORITY_INFO_ACCESS_new();
					if (hAuthorityInfo) {
						for (sl_size i = 0; i < authorityInformations.count; i++) {
							X509AuthorityInformation& info = authorityInformations[i];
							ACCESS_DESCRIPTION* desc = ACCESS_DESCRIPTION_new();
							if (desc) {
								desc->method = OBJ_nid2obj((int)info.method);
								switch (info.type) {
									case X509AuthorityInformationLocationType::Email:
										desc->location->type = GEN_EMAIL;
										desc->location->d.rfc822Name = Get_ASN1_STRING_from_String(info.value);
										break;
									case X509AuthorityInformationLocationType::DNS:
										desc->location->type = GEN_DNS;
										desc->location->d.dNSName = Get_ASN1_STRING_from_String(info.value);
										break;
									case X509AuthorityInformationLocationType::URI:
										desc->location->type = GEN_URI;
										desc->location->d.uniformResourceIdentifier = Get_ASN1_STRING_from_String(info.value);
										break;
									default:
										break;
								}
								sk_ACCESS_DESCRIPTION_push(hAuthorityInfo, desc);
							}
						}
						X509_add1_ext_i2d(handle, NID_info_access, hAuthorityInfo, 0, 0);
						AUTHORITY_INFO_ACCESS_free(hAuthorityInfo);
					} else {
						return sl_null;
					}
				}

				return hX509.release();
			}

			static Memory Get_Memory_from_X509(::X509* handle)
			{
				sl_int32 size = (sl_int32)(i2d_X509_AUX(handle, sl_null));
				if (size > 0) {
					Memory ret = Memory::create(size);
					if (ret.isNotNull()) {
						unsigned char* buf = (unsigned char*)(ret.getData());
						if (i2d_X509_AUX(handle, &buf) == size) {
							return ret;
						}
					}
				}
				return sl_null;
			}

			static Memory Sign_X509(const X509& cert, const PrivateKey& issuerKey, const EVP_MD* md)
			{
				InitThread();
				EVP_PKEY_Handle key(Get_EVP_PKEY_from_PrivateKey(issuerKey));
				if (key.isNone()) {
					return sl_null;
				}
				X509_Handle handle(Get_X509_Handle(cert));
				if (handle.isNone()) {
					return sl_null;
				}
				if (X509_sign(handle.get(), key.get(), md)) {
					return Get_Memory_from_X509(handle.get());
				}
				return sl_null;
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
		InitThread();
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
		InitThread();
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
		InitThread();
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
		InitThread();
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

	sl_bool OpenSSL::generate_ECKey(const EllipticCurve& curve, ECPrivateKey& _output)
	{
		InitThread();
		sl_bool flagSuccess = sl_false;
		EC_GROUP* group = Get_EC_GROUP_from_EllipticCurve(curve);
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
	
	sl_bool OpenSSL::check_ECKey(const EllipticCurve& curve, const ECPublicKey& key)
	{
		InitThread();
		sl_bool flagSuccess = sl_false;
		EC_GROUP* group = Get_EC_GROUP_from_EllipticCurve(curve);
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

	ECDSA_Signature OpenSSL::sign_ECDSA(const EllipticCurve& curve, const ECPrivateKey& key, const BigInt& z)
	{
		Memory mem = z.getBytesBE();
		if (mem.isNull()) {
			return ECDSA_Signature();
		}
		return Do_sign_ECDSA(curve, key, mem.getData(), mem.getSize());
	}
	
	ECDSA_Signature OpenSSL::sign_ECDSA(const EllipticCurve& curve, const ECPrivateKey& key, const void* hash, sl_size size)
	{
		return Do_sign_ECDSA(curve, key, hash, size);
	}

	ECDSA_Signature OpenSSL::sign_ECDSA_SHA256(const EllipticCurve& curve, const ECPrivateKey& key, const void* data, sl_size size)
	{
		sl_uint8 hash[SHA256::HashSize];
		SHA256::hash(data, size, hash);
		return Do_sign_ECDSA(curve, key, hash, sizeof(hash));
	}

	ECDSA_Signature OpenSSL::sign_ECDSA_SHA384(const EllipticCurve& curve, const ECPrivateKey& key, const void* data, sl_size size)
	{
		sl_uint8 hash[SHA384::HashSize];
		SHA384::hash(data, size, hash);
		return Do_sign_ECDSA(curve, key, hash, sizeof(hash));
	}

	ECDSA_Signature OpenSSL::sign_ECDSA_SHA512(const EllipticCurve& curve, const ECPrivateKey& key, const void* data, sl_size size)
	{
		sl_uint8 hash[SHA512::HashSize];
		SHA512::hash(data, size, hash);
		return Do_sign_ECDSA(curve, key, hash, sizeof(hash));
	}

	sl_bool OpenSSL::verify_ECDSA(const EllipticCurve& curve, const ECPublicKey& key, const BigInt& z, const ECDSA_Signature& signature)
	{
		Memory mem = z.getBytesBE();
		if (mem.isNull()) {
			return sl_false;
		}
		return Do_verify_ECDSA(curve, key, mem.getData(), mem.getSize(), signature);
	}
	
	sl_bool OpenSSL::verify_ECDSA(const EllipticCurve& curve, const ECPublicKey& key, const void* hash, sl_size size, const ECDSA_Signature& signature)
	{
		return Do_verify_ECDSA(curve, key, hash, size, signature);
	}
	
	sl_bool OpenSSL::verify_ECDSA_SHA256(const EllipticCurve& curve, const ECPublicKey& key, const void* data, sl_size size, const ECDSA_Signature& signature)
	{
		sl_uint8 hash[SHA256::HashSize];
		SHA256::hash(data, size, hash);
		return Do_verify_ECDSA(curve, key, hash, sizeof(hash), signature);
	}

	sl_bool OpenSSL::verify_ECDSA_SHA384(const EllipticCurve& curve, const ECPublicKey& key, const void* data, sl_size size, const ECDSA_Signature& signature)
	{
		sl_uint8 hash[SHA384::HashSize];
		SHA384::hash(data, size, hash);
		return Do_verify_ECDSA(curve, key, hash, sizeof(hash), signature);
	}

	sl_bool OpenSSL::verify_ECDSA_SHA512(const EllipticCurve& curve, const ECPublicKey& key, const void* data, sl_size size, const ECDSA_Signature& signature)
	{
		sl_uint8 hash[SHA512::HashSize];
		SHA512::hash(data, size, hash);
		return Do_verify_ECDSA(curve, key, hash, sizeof(hash), signature);
	}

	BigInt OpenSSL::getSharedKey_ECDH(const EllipticCurve& curve, const ECPrivateKey& keyLocal, const ECPublicKey& keyRemote)
	{
		InitThread();
		BigInt ret;
		EC_GROUP* group = Get_EC_GROUP_from_EllipticCurve(curve);
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
		InitThread();
		::X509* handle = Load_X509(content, size);
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

	sl_bool OpenSSL::loadX509(X509& _out, const StringParam& filePath)
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


	sl_bool OpenSSL::loadPKCS12(PKCS12& _out, const void* content, sl_size size, const StringParam& _password)
	{
		if (!content) {
			return sl_false;
		}
		if (!size) {
			return sl_false;
		}
		InitThread();

		PKCS12_Handle p12(d2i_PKCS12(sl_null, (const unsigned char**)&content, (long)size));
		if (p12.isNone()) {
			return sl_false;
		}

		StringCstr password(_password);
		EVP_PKEY_Handle key;
		Stack_X509_Handle certificates;
		if (!(PKCS12_parse(p12.get(), password.getData(), &(key.handle), sl_null, &(certificates.handle)))) {
			return sl_false;
		}
		if (key.isNotNone()) {
			if (!(Get_PrivateKey_from_EVP_PKEY(_out.key, key.get()))) {
				return sl_false;
			}
		}
		if (certificates.isNotNone()) {
			int n = sk_X509_num(certificates.get());
			for (int i = 0; i < n; i++) {
				::X509* x509 = sk_X509_value(certificates.get(), i);
				Memory mem = Get_Memory_from_X509(x509);
				if (mem.isNull()) {
					return sl_false;
				}
				_out.certificates.add_NoLock(Move(mem));
			}
		}
		return sl_true;
	}

	sl_bool OpenSSL::loadPKCS12(PKCS12& _out, const Memory& memory, const StringParam& password)
	{
		return loadPKCS12(_out, memory.getData(), memory.getSize(), password);
	}

	sl_bool OpenSSL::loadPKCS12(PKCS12& _out, const StringParam& filePath, const StringParam& password)
	{
		Memory mem = File::readAllBytes(filePath);
		return loadPKCS12(_out, mem.getData(), mem.getSize(), password);
	}

	Memory OpenSSL::savePKCS12(const PKCS12& p12, const StringParam& _password)
	{
		StringCstr password(_password);

		EVP_PKEY_Handle key(Get_EVP_PKEY_from_PrivateKey(p12.key));
		if (key.isNone()) {
			return sl_null;
		}
		ListElements<Memory> certDataList(p12.certificates);
		if (!(certDataList.count)) {
			return sl_null;
		}
		Stack_X509_Handle certificates(sk_X509_new_null());
		if (certificates.isNone()) {
			return sl_null;
		}
		X509_Handle mainCertificate;
		List<X509_Handle> listX509;
		for (sl_size i = 0; i < certDataList.count; i++) {
			Memory& certData = certDataList[i];
			const sl_uint8* data = (const sl_uint8*)(certData.getData());
			sl_size size = certData.getSize();
			::X509* x509 = d2i_X509(sl_null, &data, (long)size);
			if (x509) {
				if (X509_check_private_key(x509, key.get())) {
					mainCertificate.handle = x509;
				} else {
					sk_X509_push(certificates.get(), x509);
					listX509.add_NoLock(x509);
				}
			}
		}
		if (mainCertificate.isNone()) {
			return sl_null;
		}

		const char* name;
		StringCstr _name(p12.friendlyName);
		if (p12.friendlyName.isNotNull()) {
			name = _name.getData();
		} else {
			name = sl_null;
		}
		PKCS12_Handle handle(PKCS12_create(password.getData(), name, key.get(), mainCertificate.get(), certificates.get(), 0, 0, 0, 0, 0));
		if (handle.isNotNone()) {
			int size = i2d_PKCS12(handle.get(), sl_null);
			if (size > 0) {
				Memory ret = Memory::create((sl_size)size);
				if (ret.isNotNull()) {
					unsigned char* buf = (unsigned char*)(ret.getData());
					if (i2d_PKCS12(handle, &buf) == size) {
						return ret;
					}
				}
			}
		}
		return sl_null;
	}

}
