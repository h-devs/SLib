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

#ifndef CHECKHEADER_SLIB_CRYPTO_BASE64
#define CHECKHEADER_SLIB_CRYPTO_BASE64

#include "../core/string.h"

namespace slib
{
	
	class SLIB_EXPORT Base64
	{
	public:
		static String encode(const void* byte, sl_size size, sl_char8 padding = '=');

		static String encodeUrl(const void* byte, sl_size size, sl_char8 padding = 0);
		
		static String encode(const Memory& mem, sl_char8 padding = '=');
				
		static String encodeUrl(const Memory& mem, sl_char8 padding = 0);

		static String encode(const StringParam& str, sl_char8 padding = '=');

		static String encodeUrl(const StringParam& str, sl_char8 padding = 0);
		
		static sl_size getDecodeOutputSize(sl_size lenBase64);
		
		static sl_size decode(const StringParam& base64, void* output, sl_char8 padding = '=');

		static Memory decode(const StringParam& base64, sl_char8 padding = '=');
		
	};

}

#endif
