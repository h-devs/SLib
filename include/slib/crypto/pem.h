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

#ifndef CHECKHEADER_SLIB_CRYPTO_PEM
#define CHECKHEADER_SLIB_CRYPTO_PEM

#include "certificate.h"

#include "../core/memory.h"
#include "../core/list.h"

/*
	Privacy-enhanced Electronic Mail
*/

namespace slib
{

	enum class PEMInstanceType
	{
		Unknown = 0,
		Certificate = 1, // CERTIFICATE (X509)
		TrustedCertificate = 2, // TRUSTED CERTIFICATE (X509)
		CertificateRequest = 3, // CERTIFICATE REQUEST (X509 REQ)
		X509Control = 4, //	X509 CRL
		PublicKey = 5, // PUBLIC KEY
		RSAPrivateKey = 6, // RSA PRIVATE KEY
		RSAPublicKey = 7, // RSA PUBLIC KEY
		DSAPrivateKey = 8, // DSA PRIVATE KEY
		DSAPublicKey = 9, // DSA PUBLIC KEY
		PKCS7 = 10, // PKCS7
		SignedPKCS7 = 11, // PKCS #7 SIGNED DATA
		EncryptedPrivateKey = 12, // ENCRYPTED PRIVATE KEY (PKCS8)
		PrivateKey = 13, // PRIVATE KEY (PKCS8)
		DHParameters = 14, // DH PARAMETERS
		DHXParameters = 15, // X9.42 DH PARAMETERS
		SSLSessionParameters = 16, // SSL SESSION PARAMETERS
		DSAParameters = 17, // DSA PARAMETERS
		ECPublicKey = 18, // ECDSA PUBLIC KEY
		ECParamters = 19, // EC PARAMETERS
		ECPrivateKey = 20, // EC PRIVATE KEY
		Paramters = 21, // PARAMETERS
		CMS = 22 // CMS
	};

	class SLIB_EXPORT PEMInstance
	{
	public:
		PEMInstance() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PEMInstance)

	public:
		PEMInstanceType type;
		Memory content;

	public:
		sl_bool getPrivateKey(PrivateKey& _out);

		sl_bool getPublicKey(PublicKey& _out);

	};

	class SLIB_EXPORT PEM
	{
	public:
		PEM() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PEM)

	public:
		List<PEMInstance> instances;

	public:
		sl_bool load(const MemoryView& mem);

		sl_bool load(const StringParam& filePath);

		Memory save();

		sl_bool save(const StringParam& filePath);

	public:
		sl_bool getPrivateKey(PrivateKey& _out);

		sl_bool addPrivateKey(const PrivateKey& privateKey);

		sl_bool getPublicKey(PublicKey& _out);

		sl_bool addPublicKey(const PublicKey& publicKey);

	};

}

#endif
