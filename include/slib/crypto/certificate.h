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

#ifndef CHECKHEADER_SLIB_CRYPTO_CERTIFICATE
#define CHECKHEADER_SLIB_CRYPTO_CERTIFICATE

#include "rsa.h"
#include "ecc.h"

namespace slib
{

	class SLIB_EXPORT PublicKey
	{
	public:
		RSAPublicKey rsa;
		ECPublicKeyWithCurve ecc;

	public:
		PublicKey();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PublicKey)

	public:
		sl_bool isRSA() const noexcept;

		sl_bool isECC() const noexcept;

		void setECC(const EllipticCurve& curve, const ECPublicKey& key) noexcept;

		void setECC(const EllipticCurve& curve, ECPublicKey&& key) noexcept;

	};

	class SLIB_EXPORT PrivateKey
	{
	public:
		RSAPrivateKey rsa;
		ECPrivateKeyWithCurve ecc;

	public:
		PrivateKey();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PrivateKey)

	public:
		sl_bool isDefined() const noexcept;

		sl_bool isRSA() const noexcept;

		sl_bool isECC() const noexcept;

		void setECC(const EllipticCurve& curve, const ECPrivateKey& key) noexcept;

		void setECC(const EllipticCurve& curve, ECPrivateKey&& key) noexcept;

	};

}

#endif

