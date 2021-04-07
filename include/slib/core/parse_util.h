/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_PARSE_UTIL
#define CHECKHEADER_SLIB_CORE_PARSE_UTIL

#include "string.h"
#include "parse.h"

namespace slib
{

	class ParseUtil
	{
	public:
		static String applyBackslashEscapes(const StringParam& str, sl_bool flagDoubleQuote = sl_true, sl_bool flagAddQuote = sl_true, sl_bool flagEscapeNonAscii = sl_false) noexcept;

		static String16 applyBackslashEscapes16(const StringParam& str, sl_bool flagDoubleQuote = sl_true, sl_bool flagAddQuote = sl_true, sl_bool flagEscapeNonAscii = sl_false) noexcept;

		
		static String parseBackslashEscapes(const StringParam& input, sl_size* lengthParsed = sl_null, sl_bool* flagError = sl_null) noexcept;

		static String16 parseBackslashEscapes16(const StringParam& input, sl_size* lengthParsed = sl_null, sl_bool* flagError = sl_null) noexcept;

		
		static sl_size countLineNumber(const StringParam& input, sl_size* column = sl_null) noexcept;


		static sl_reg indexOfLine(const StringParam& input, sl_reg start = 0) noexcept;

		static sl_reg indexOfNotLine(const StringParam& input, sl_reg start = 0) noexcept;

		static sl_reg indexOfWhitespace(const StringParam& input, sl_reg start = 0) noexcept;

		static sl_reg indexOfNotWhitespace(const StringParam& input, sl_reg start = 0) noexcept;

		static sl_reg indexOfChar(const StringParam& input, const ListParam<sl_char8>& list, sl_reg start = 0) noexcept;

		static sl_reg indexOfNotChar(const StringParam& input, const ListParam<sl_char8>& list, sl_reg start = 0) noexcept;

		static sl_reg indexOfChar16(const StringParam& input, const ListParam<sl_char16>& list, sl_reg start = 0) noexcept;

		static sl_reg indexOfNotChar16(const StringParam& input, const ListParam<sl_char16>& list, sl_reg start = 0) noexcept;


		static sl_reg getWord(String& _out, const StringParam& input, sl_reg start = 0) noexcept;
		
		static sl_reg getWord16(String16& _out, const StringParam& input, sl_reg start = 0) noexcept;
		
		static List<String> getWords(const StringParam& input, sl_reg start = 0) noexcept;

		static List<String16> getWords16(const StringParam& input, sl_reg start = 0) noexcept;

	};

}

#endif
