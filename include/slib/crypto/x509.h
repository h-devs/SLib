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

#ifndef CHECKHEADER_SLIB_CRYPTO_X509
#define CHECKHEADER_SLIB_CRYPTO_X509

#include "certificate.h"

#include "../core/time.h"
#include "../core/hash_map.h"
#include "../core/flags.h"

/*

	.cer file format

	X.509 is a standard that defines the format of public key certificates.

*/

namespace slib
{

	// OpenSSL NID
	enum class X509SubjectKey
	{
		CommonName = 13,
		CountryName = 14,
		LocalityName = 15,
		StateOrProvinceName = 16,
		OrganizationName = 17,
		OrganizationalUnitName = 18,
		Title = 106,
		Surname = 100,
		SearchGuide = 859,
		Description = 107,
		StreetAddress = 660,
		BusinessCategory = 860,
		PostalAddress = 861,
		PostalCode = 661,
		PostOfficeBox = 862,
		TelephoneNumber = 864
	};

	// OpenSSL NID
	enum class X509SignatureAlgorithm
	{
		Unknown = 0,
		Sha256WithRSA = 668,
		Sha384WithRSA = 669,
		Sha512WithRSA = 670,
		Sha224WithRSA = 671,
		Sha224WithECDSA = 793,
		Sha256WithECDSA = 794,
		Sha384WithECDSA = 795,
		Sha512WithECDSA = 796
	};

	// OpenSSL NID
	enum class X509EnhancedKeyUsage
	{
		EmailProtect = 132,
		ClientAuthentication = 130,
		AnyExtendedKeyUsage = 910
	};

	// OpenSSL NID
	enum class X509AuthorityInformationAccessMethod
	{
	};

	enum class X509AuthorityInformationLocationType
	{
		Unknown = 0,
		Email = 1,
		DNS = 2,
		URI = 3
	};

	SLIB_DEFINE_FLAGS(X509KeyUsages, {
		Default = 0,
		EncipherOnly = 1,
		ControlSign = 2,
		KeyCertificateSign = 4,
		KeyAgreement = 8,
		DataEncipherment = 0x10,
		KeyEncipherment = 0x20,
		NonRepudiation = 0x40,
		DigitalSignature = 0x80,
		DecipherOnly = 0x8000
	})

	class SLIB_EXPORT X509CertificatePolicy
	{
	public:
		String identifier;
		String userNotice;
		String CPS;

	public:
		X509CertificatePolicy();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(X509CertificatePolicy)

	};

	class SLIB_EXPORT X509AuthorityInformation
	{
	public:
		X509AuthorityInformationAccessMethod method;
		X509AuthorityInformationLocationType type;
		String value;

	public:
		X509AuthorityInformation();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(X509AuthorityInformation)

	};

	class SLIB_EXPORT X509
	{
	public:
		X509() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(X509)

	public:
		// Version 1 Fields
		sl_uint32 version;
		BigInt serialNumber;
		Time validFrom;
		Time validTo;
		HashMap<X509SubjectKey, String> subject;
		HashMap<X509SubjectKey, String> issuer;
		PublicKey key;

		// Extensions
		BigInt authorityKeyId;
		BigInt subjectKeyId;
		List<X509CertificatePolicy> policies;
		List<X509AuthorityInformation> authorityInformations;
		List<X509EnhancedKeyUsage> enhancedKeyUsages;

		// Critical Extensions
		X509KeyUsages keyUsages;
		sl_bool flagEndEntity;

		// Signature
		X509SignatureAlgorithm signatureAlgorithm;
		Memory contentHash;
		Memory signature;

	public:
		sl_bool load(const void* content, sl_size size);

		sl_bool load(const Memory& memory);

		sl_bool load(const StringParam& filePath);

		sl_bool verify(const PublicKey& issuerKey);

	public:
		static sl_bool loadPublicKey(PublicKey& _out, const void* input, sl_size size) noexcept;

		static Memory savePublicKey(const PublicKey& _in) noexcept;

	};

}

#endif
