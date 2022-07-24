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
#include "slib/crypto/pkcs7.h"

#include "slib/core/file.h"
#include "slib/crypto/asn1.h"
#include "slib/crypto/sha1.h"
#include "slib/crypto/des.h"
#include "slib/crypto/rc4.h"

#define OID_US "\x2A\x86\x48" // ISO(1) Member-Body(2) US(840)
#define OID_RSADSI OID_US "\x86\xF7\x0D" // 113549
#define OID_PKCS OID_RSADSI "\x01"
#define OID_PKCS7 OID_PKCS "\x07"
#define OID_PKCS7_DATA OID_PKCS7 "\x01"
#define OID_PKCS7_ENCRYPTED_DATA OID_PKCS7 "\x06"
#define OID_PKCS12 OID_PKCS "\x0C"
#define OID_PKCS12_VERSION1 OID_PKCS12 "\x0A"
#define OID_PKCS12_BAG_IDS OID_PKCS12_VERSION1 "\x01"
#define OID_PKCS12_PKCS8_SHROUNDED_KEY_BAG OID_PKCS12_BAG_IDS "\x02"
#define OID_PKCS12_PBE_IDS OID_PKCS12 "\x01"
#define OID_PKCS12_PBE_SHA1_RC4_128 OID_PKCS12_PBE_IDS "\x01"
#define OID_PKCS12_PBE_SHA1_RC4_40 OID_PKCS12_PBE_IDS "\x02"
#define OID_PKCS12_PBE_SHA1_3DES OID_PKCS12_PBE_IDS "\x03"
#define OID_PKCS12_PBE_SHA1_2DES OID_PKCS12_PBE_IDS "\x04"
#define OID_PKCS12_PBE_SHA1_RC2_128 OID_PKCS12_PBE_IDS "\x05"
#define OID_PKCS12_PBE_SHA1_RC2_40 OID_PKCS12_PBE_IDS "\x06"

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

			static List<PKCS12_SafeBag> PKCS12_Unpack_PKCS7_Data(PKCS7& p7)
			{
				if (!(p7.type.equals(OID_PKCS7_DATA))) {
					return sl_null;
				}
				Asn1String data;
				if (!(p7.getData(data))) {
					return sl_null;
				}
				Asn1MemoryReader reader(data.data, data.length);
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

			class PKCS12_PBE_Param
			{
			public:
				Asn1String salt;
				sl_uint32 iteration;

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

			static Memory PKCS12_Decrypt(const void* data, sl_size size, const X509Algorithm& alg, const StringParam& _password)
			{
				if (!data || !size) {
					return sl_null;
				}
				sl_uint32 lenKey;
				sl_bool flagRC;
				if (alg.algorithm.equals(OID_PKCS12_PBE_SHA1_RC4_128)) {
					lenKey = 16;
					flagRC = sl_true;
				} else if (alg.algorithm.equals(OID_PKCS12_PBE_SHA1_RC4_40)) {
					lenKey = 5;
					flagRC = sl_true;
				} else if (alg.algorithm.equals(OID_PKCS12_PBE_SHA1_3DES)) {
					lenKey = 24;
					flagRC = sl_false;
				} else if (alg.algorithm.equals(OID_PKCS12_PBE_SHA1_2DES)) {
					lenKey = 16;
					flagRC = sl_false;
				} else if (alg.algorithm.equals(OID_PKCS12_PBE_SHA1_RC2_128)) {
					// Not Supported
					return sl_null;
				} else if (alg.algorithm.equals(OID_PKCS12_PBE_SHA1_RC2_40)) {
					// Not Supported
					return sl_null;
				}

				// Uses UTF16-BE Encoding
				StringData16 password(_password);
				sl_size lenPassword = password.getLength();
				Memory memPassword;
				sl_uint8* bufPassword = sl_null;
				sl_size sizePassword = 0;
				if (lenPassword) {
					sizePassword = lenPassword << 1;
					memPassword = Memory::create(sizePassword);
					if (memPassword.isNull()) {
						return sl_null;
					}
					bufPassword = (sl_uint8*)(memPassword.getData());
					sl_uint8* buf = bufPassword;
					sl_char16* cur = password.getData();
					sl_char16* end = cur + lenPassword;
					while (cur < end) {
						MIO::writeUint16BE(buf, *cur);
						cur++;
						buf += 2;
					}
				}
				
				PKCS12_PBE_Param param;
				if (!(param.load(alg.parameter))) {
					return sl_null;
				}

				return sl_null;
			}

			static sl_bool PKCS12_ParseBag(PKCS12& p12, PKCS12_SafeBag& bag, const StringParam& password)
			{
				if (bag.type.equals(OID_PKCS12_PKCS8_SHROUNDED_KEY_BAG)) {
					X509Signature sig;
					if (sig.load(bag.content)) {
						Memory dec = PKCS12_Decrypt(sig.digest.data, sig.digest.length, sig.algorithm, password);
						dec = dec;
					}
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

					}
				}
				return sl_true;
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

	X509::X509() noexcept: version(2), flagEndEntity(sl_true)
	{
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

	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PKCS7)

	PKCS7::PKCS7()
	{
	}

	sl_bool PKCS7::load(const Asn1Element& element)
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

	sl_bool PKCS7::getData(Asn1String& _out)
	{
		return content.getOctetString(_out);
	}

	/*
	sl_bool PKCS7::readEncryptedData(Asn1MemoryReader& reader)
	{
		if (contentType.equals(OID_PKCS7_DATA)) {
			// 1.2.840.113549.1.7.1 (data)
			Asn1String content;
			if (body.readOctetString(content)) {
				Asn1MemoryReader readerContent(content.data, content.length);
				Asn1MemoryReader contents;
				if (readerContent.readSequence(contents)) {
					while (load(contents));
				}
			}
		} else if (contentType.equals(OID_PKCS7_ENCRYPTED_DATA)) {
			// 1.2.840.113549.1.7.6 (encryptedData)
			return readEncryptedData(body);
		} else if (contentType.equals(OID_PKCS12_PKCS8_SHROUNDED_KEY_BAG)) {
			auto s = Asn1::getObjectIdentifierString(contentType.data, contentType.length);
		}
		Asn1MemoryReader body;
		if (!(reader.readSequence(body))) {
			return sl_false;
		}
		sl_uint32 version;
		if (!(body.readInt(version))) {
			return sl_false;
		}
		// EncryptedContentInfo
		Asn1MemoryReader contentInfo;
		if (!(body.readSequence(contentInfo))) {
			return sl_false;
		}
		Asn1ObjectIdentifier contentType;
		if (!(contentInfo.readObjectIdentifier(contentType))) {
			return sl_false;
		}
		// ContentEncryptionAlgorithmIdentifier
		X509Algorithm algorithm;
		if (!(algorithm.load(contentInfo))) {
			return sl_false;
		}
		// Encryption Content
		sl_uint8 contentTag;
		Asn1MemoryReader contentBody;
		contentInfo.readAnyElement(contentTag, contentBody);
		return sl_true;
	}

	*/
}
