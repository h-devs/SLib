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

namespace slib
{

	class ParseUtil
	{
	public:
		static String applyBackslashEscapes(const StringView& str, sl_bool flagDoubleQuote = sl_true, sl_bool flagAddQuote = sl_true, sl_bool flagEscapeNonAscii = sl_false) noexcept;

		static String16 applyBackslashEscapes(const StringView16& str, sl_bool flagDoubleQuote = sl_true, sl_bool flagAddQuote = sl_true, sl_bool flagEscapeNonAscii = sl_false) noexcept;

		static String32 applyBackslashEscapes(const StringView32& str, sl_bool flagDoubleQuote = sl_true, sl_bool flagAddQuote = sl_true, sl_bool flagEscapeNonAscii = sl_false) noexcept;


		static String parseBackslashEscapes(const StringView& input, sl_size* lengthParsed = sl_null, sl_bool* flagError = sl_null) noexcept;

		static String16 parseBackslashEscapes(const StringView16& input, sl_size* lengthParsed = sl_null, sl_bool* flagError = sl_null) noexcept;

		static String32 parseBackslashEscapes(const StringView32& input, sl_size* lengthParsed = sl_null, sl_bool* flagError = sl_null) noexcept;


		static sl_size countLineNumber(const StringView& input, sl_size* column = sl_null) noexcept;

		static sl_size countLineNumber(const StringView16& input, sl_size* column = sl_null) noexcept;

		static sl_size countLineNumber(const StringView32& input, sl_size* column = sl_null) noexcept;


		static List<String> splitLines(const String& input) noexcept;

		static List<String16> splitLines(const String16& input) noexcept;

		static List<String32> splitLines(const String32& input) noexcept;

		static List<StringView> splitLines(const StringView& input) noexcept;

		static List<StringView16> splitLines(const StringView16& input) noexcept;

		static List<StringView32> splitLines(const StringView32& input) noexcept;


		static sl_reg indexOfLine(const StringView& input, sl_reg start = 0) noexcept;

		static sl_reg indexOfLine(const StringView16& input, sl_reg start = 0) noexcept;

		static sl_reg indexOfLine(const StringView32& input, sl_reg start = 0) noexcept;


		static sl_reg indexOfNotLine(const StringView& input, sl_reg start = 0) noexcept;

		static sl_reg indexOfNotLine(const StringView16& input, sl_reg start = 0) noexcept;

		static sl_reg indexOfNotLine(const StringView32& input, sl_reg start = 0) noexcept;


		static sl_reg indexOfWhitespace(const StringView& input, sl_reg start = 0) noexcept;

		static sl_reg indexOfWhitespace(const StringView16& input, sl_reg start = 0) noexcept;

		static sl_reg indexOfWhitespace(const StringView32& input, sl_reg start = 0) noexcept;


		static sl_reg indexOfNotWhitespace(const StringView& input, sl_reg start = 0) noexcept;

		static sl_reg indexOfNotWhitespace(const StringView16& input, sl_reg start = 0) noexcept;

		static sl_reg indexOfNotWhitespace(const StringView32& input, sl_reg start = 0) noexcept;


		static sl_reg indexOfChar(const StringView& input, const ListParam<sl_char8>& list, sl_reg start = 0) noexcept;

		static sl_reg indexOfChar(const StringView16& input, const ListParam<sl_char16>& list, sl_reg start = 0) noexcept;

		static sl_reg indexOfChar(const StringView32& input, const ListParam<sl_char32>& list, sl_reg start = 0) noexcept;


		static sl_reg indexOfNotChar(const StringView& input, const ListParam<sl_char8>& list, sl_reg start = 0) noexcept;

		static sl_reg indexOfNotChar(const StringView16& input, const ListParam<sl_char16>& list, sl_reg start = 0) noexcept;

		static sl_reg indexOfNotChar(const StringView32& input, const ListParam<sl_char32>& list, sl_reg start = 0) noexcept;


		static sl_reg getWord(String& _out, const StringView& input, sl_reg start = 0) noexcept;

		static sl_reg getWord(String16& _out, const StringView16& input, sl_reg start = 0) noexcept;

		static sl_reg getWord(String32& _out, const StringView32& input, sl_reg start = 0) noexcept;


		static List<String> getWords(const StringView& input, sl_reg start = 0) noexcept;

		static List<String16> getWords(const StringView16& input, sl_reg start = 0) noexcept;

		static List<String32> getWords(const StringView32& input, sl_reg start = 0) noexcept;

	};

}

#endif
