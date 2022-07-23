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

			public:
				sl_bool load(Asn1MemoryReader& reader)
				{
					Asn1MemoryReader body;
					if (reader.readSequence(body)) {
						if (body.readObjectIdentifier(algorithm)) {
							return sl_true;
						}
					}
					return sl_false;
				}

			};

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
		Asn1MemoryReader reader(content, size);
		Asn1MemoryReader p12Body;
		if (!(reader.readSequence(p12Body))) {
			return sl_false;
		}
		sl_uint32 version;
		if (!(p12Body.readInt(version))) {
			return sl_false;
		}
		PKCS7 authSafes;
		if (!(authSafes.load(p12Body))) {
			return sl_false;
		}
		return sl_true;
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

	sl_bool PKCS7::load(Asn1MemoryReader& reader)
	{
		Asn1MemoryReader body;
		if (!(reader.readSequence(body))) {
			return sl_false;
		}
		Asn1ObjectIdentifier contentType;
		if (!(body.readObjectIdentifier(contentType))) {
			return sl_false;
		}
		// PKCS-7 OBJECT IDENTIFIER: ISO(1) Member-Body(2) US(840) RSADSI(113549) PKCS(1) 7
		if (contentType.equals("\x2A\x86\x48\x86\xF7\x0D\x01\x07\x01")) {
			// 1.2.840.113549.1.7.1 (data)
			Asn1String content;
			if (body.readOctetString(content)) {
				Asn1MemoryReader readerContent(content.data, content.length);
				Asn1MemoryReader contents;
				if (readerContent.readSequence(contents)) {
					while (load(contents));
				}
			}
		} else if (contentType.equals("\x2A\x86\x48\x86\xF7\x0D\x01\x07\x06")) {
			// 1.2.840.113549.1.7.6 (encryptedData)
			return readEncryptedData(body);
		}
		return sl_false;
	}
	
	sl_bool PKCS7::readEncryptedData(Asn1MemoryReader& reader)
	{
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

		return sl_true;
	}

}
