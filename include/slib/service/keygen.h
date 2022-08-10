/*
*   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_SERVICE_KEYGEN
#define CHECKHEADER_SLIB_SERVICE_KEYGEN

#include "definition.h"

#include "../core/string.h"

namespace slib
{

	class Keygen
	{
	public:
		static void generateKey(String& privateKey, String& publicKey);

		static String getPublicKey(const StringView& privateKey);

		static String getMachineCode();

		static String getRequestCode(const StringView& machineCode, const StringView& publicKey, const StringView& extraInfo);

		static String getRequestCode(const StringView& publicKey, const StringView& extraInfo);

		static String getRequestCode();

		static String getExtraFromRequestCode(const StringView& publicKey, const StringView& requestCode);

		static String generateAuthorizationCode(const StringView& privateKey, const StringView& requestCode, const StringView& extraInfo);

		static String generateAuthorizationCode(const StringView& privateKey, const StringView& requestCode);

		static sl_bool verifyAuthorizationCode(const StringView& publicKey, const StringView& requestCode, const StringView& authCode);

		static String getExtraFromAuthorizationCode(const StringView& publicKey, const StringView& authCode);

	};

}

#endif