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

#ifndef CHECKHEADER_SLIB_CRYPTO_PKCS12
#define CHECKHEADER_SLIB_CRYPTO_PKCS12

#include "certificate.h"

#include "../core/memory.h"
#include "../core/list.h"

/*

	.p12, .pfx file format

	PKCS #12 defines an archive file format for storing many cryptography objects as a single file.
	It is commonly used to bundle a private key with its X.509 certificate or to bundle all the members of a chain of trust.

*/

namespace slib
{

	class SLIB_EXPORT PKCS12
	{
	public:
		PKCS12() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PKCS12)

	public:
		PrivateKey key;
		List<Memory> certificates;
		String friendlyName;

	public:
		sl_bool load(const MemoryView& input, const StringParam& password);

		sl_bool load(const StringParam& filePath, const StringParam& password);

	};

}

#endif
