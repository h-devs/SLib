/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/crypto/x509.h"

namespace slib
{

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


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(X509)

	X509::X509() noexcept
	{
	}

}
