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

#ifndef CHECKHEADER_SLIB_CRYPTO_X509
#define CHECKHEADER_SLIB_CRYPTO_X509

#include "certificate.h"

#include "../core/time.h"
#include "../core/hash_map.h"

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

	enum class X509ExtensionKey 
	{
		AuthorityKeyIdentifier = 90,
		AuthorityInfoAccess = 177,
		CertificatePolicies = 89,
		SubjectKeyIdentifier = 82,
		BasicConstraints = 87,
		ExtendedKeyUsage = 126,
		KeyUsage = 83
	};

	enum class PolicyQualifierID
	{
		Qualifier_CPS = 164,
		Qualifier_User_Notice = 165
	};

	struct X509Policy 
	{
		String identifier;
		HashMap<PolicyQualifierID, String> qualifiers;
	};
	struct X509AuthorityInformation
	{
		String generalName;
		sl_int32 method;
		sl_int32 generalNameType;
	};

	// .cer file format
	class SLIB_EXPORT X509
	{
	public:
		X509() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(X509)

	public:
		sl_uint32 version;
		BigInt serialNumber;					// 80 bit
		String signatureAlgorithm;
		Time validFrom;
		Time validTo;
		HashMap<X509SubjectKey, String> subject;
		HashMap<X509SubjectKey, String> issuer;
		PublicKey key;

		BigInt authKeyId;						// 160 bit
		BigInt subjectKeyId;					// 160 bit
		List<sl_int32> usages;					// NID_email_protect, NID_client_auth
		String keyUsage;						// KU_DIGITAL_SIGNATURE ||
		sl_int32 basicCA;						// BASIC_CONSTRAINTS -> ca

		List<X509Policy> certPolicies;
		List<X509AuthorityInformation> authorityInfo;
		

	public:
		sl_bool load(const void* content, sl_size size);

		sl_bool load(const Memory& memory);

		sl_bool loadFile(const StringParam& filePath);

	public:
		Memory sign(const PrivateKey& issuerKey) const;

	};

}

#endif
