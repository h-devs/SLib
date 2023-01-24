/*
 *	Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a copy
 *	of this software and associated documentation files (the "Software"), to deal
 *	in the Software without restriction, including without limitation the rights
 *	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *	copies of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be included in
 *	all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *	THE SOFTWARE.
 */

#include "slib/core/stringx.h"

#include "slib/core/list.h"
#include "slib/core/scoped_buffer.h"
#include "slib/math/math.h"

namespace slib
{

	namespace priv
	{
		namespace stringx
		{

			template <class CHAR>
			static sl_size ApplyBackslashEscapes(const CHAR* str, sl_size len, sl_bool flagDoubleQuote, sl_bool flagAddQuote, sl_bool flagEscapeNonAscii, CHAR* buf) noexcept
			{
				sl_size d;
				if (flagAddQuote) {
					d = 1;
					if (buf) {
						buf[0] = flagDoubleQuote ? '"' : '\'';
					}
				} else {
					d = 0;
				}
				sl_bool flagPrevEscaped = sl_false;
				for (sl_size i = 0; i < len; i++) {
					typename UnsignedType<CHAR>::Type c = str[i];
					typename UnsignedType<CHAR>::Type r = 0;
					switch (c) {
						case '\\':
							r = c;
							break;
						case '"':
							if (flagDoubleQuote) {
								r = c;
							}
							break;
						case '\'':
							if (!flagDoubleQuote) {
								r = c;
							}
							break;
						case 0:
							if (len & SLIB_SIZE_TEST_SIGN_BIT) {
								break;
							}
							r = '0';
							break;
						case '\n':
							r = 'n';
							break;
						case '\r':
							r = 'r';
							break;
						case '\b':
							r = 'b';
							break;
						case '\f':
							r = 'f';
							break;
						case '\a':
							r = 'a';
							break;
						case '\v':
							r = 'v';
							break;
					}
					if (r) {
						if (buf) {
							buf[d++] = '\\';
							buf[d++] = r;
						} else {
							d += 2;
						}
						flagPrevEscaped = sl_false;
					} else {
						sl_uint32 t = (sl_uint32)c;
						if (flagEscapeNonAscii && (t < 32 || t > 126)) {
							if (sizeof(CHAR) >= 4 && (t >> 16)) {
								if (buf) {
									buf[d++] = '\\';
									buf[d++] = 'x';
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 28) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 24) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 20) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 16) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 12) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 8) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 4) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[t & 15];
								} else {
									d += 10;
								}
							} else if (sizeof(CHAR) >= 2 && (t >> 8)) {
								if (buf) {
									buf[d++] = '\\';
									buf[d++] = 'x';
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 12) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 8) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 4) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[t & 15];
								} else {
									d += 6;
								}
							} else {
								if (buf) {
									buf[d++] = '\\';
									buf[d++] = 'x';
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 4) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[t & 15];
								} else {
									d += 4;
								}
							}
							flagPrevEscaped = sl_true;
						} else {
							if (flagPrevEscaped && SLIB_CHAR_IS_HEX(c)) {
								if (buf) {
									buf[d++] = '\\';
									buf[d++] = 'x';
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 4) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[t & 15];
								} else {
									d += 4;
								}
							} else {
								if (buf) {
									buf[d++] = c;
								} else {
									d++;
								}
								flagPrevEscaped = sl_false;
							}
						}
					}
				}
				if (flagAddQuote) {
					if (buf) {
						buf[d++] = flagDoubleQuote ? '"' : '\'';
					} else {
						d++;
					}
				}
				return d;
			}

			template <class VIEW>
			static typename VIEW::StringType ApplyBackslashEscapes(const VIEW& str, sl_bool flagDoubleQuote, sl_bool flagAddQuote, sl_bool flagEscapeNonAscii)
			{
				if (str.isNull()) {
					return sl_null;
				}
				typename VIEW::Char* data = str.getUnsafeData();
				sl_size len = str.getUnsafeLength();
				sl_size n = ApplyBackslashEscapes(data, len, flagDoubleQuote, flagAddQuote, flagEscapeNonAscii, (typename VIEW::Char*)sl_null);
				if (!n) {
					return VIEW::StringType::getEmpty();
				}
				typename VIEW::StringType ret = VIEW::StringType::allocate(n);
				if (ret.isNull()) {
					return sl_null;
				}
				ApplyBackslashEscapes(data, len, flagDoubleQuote, flagAddQuote, flagEscapeNonAscii, ret.getData());
				return ret;
			}

			template <class CHAR>
			static sl_bool ParseHexValue(const CHAR* src, sl_size len, sl_size& pos, sl_uint32& value)
			{
				sl_uint32 h = src[pos];
				h = SLIB_CHAR_HEX_TO_INT(h);
				if (h < 16) {
					value = h;
					pos++;
					while (pos < len) {
						h = src[pos];
						h = SLIB_CHAR_HEX_TO_INT(h);
						if (h < 16) {
							value = (value << 4) | h;
							pos++;
						} else {
							break;
						}
					}
					return sl_true;
				} else {
					return sl_false;
				}
			}

			template <class CHAR>
			static sl_bool ParseHexValue_FixedLength(const CHAR* src, sl_size countDigits, sl_size& pos, sl_uint32& value)
			{
				for (sl_size i = 0; i < countDigits; i++) {
					sl_uint32 h = src[pos];
					h = SLIB_CHAR_HEX_TO_INT(h);
					if (h < 16) {
						value = (value << 4) | h;
					} else {
						return sl_false;
					}
					pos++;
				}
				return sl_true;
			}

			template <class CHAR>
			static void ParseOctetValue(const CHAR* src, sl_size len, sl_size& pos, sl_uint32& value)
			{
				while (pos < len) {
					CHAR ch = src[pos];
					if (ch >= '0' && ch < '8') {
						value = (value << 3) | (ch - '0');
						pos++;
					} else {
						break;
					}
				}
			}

			template <class CHAR>
			SLIB_INLINE static void PutChar(CHAR* buf, sl_size& pos, CHAR ch)
			{
				if (buf) {
					buf[pos++] = ch;
				} else {
					pos++;
				}
			}

			template <class CHAR>
			static sl_size ParseBackslashEscapes(const CHAR* src, sl_size lengthSrc, sl_size* lengthParsed, sl_bool* outFlagError, CHAR* buf) noexcept
			{
				if (lengthParsed) {
					*lengthParsed = 0;
				}
				if (outFlagError) {
					*outFlagError = sl_true;
				}
				CHAR ch = *src;
				CHAR chEnd = 0;
				if (ch == '"') {
					chEnd = '"';
				} else if (ch == '\'') {
					chEnd = '\'';
				} else {
					return 0;
				}
				sl_size lengthOutput = 0;
				sl_bool flagSuccess = sl_false;
				sl_size i = 1;
				while (i < lengthSrc) {
					ch = src[i++];
					if (!ch) {
						break;
					}
					if (ch == '\\') {
						if (i >= lengthSrc) {
							break;
						}
						sl_bool flagError = sl_false;
						ch = src[i++];
						switch (ch) {
							case '\\':
							case '"':
							case '\'':
							case '/':
								PutChar(buf, lengthOutput, ch);
								break;
							case 'n':
								PutChar(buf, lengthOutput, (CHAR)'\n');
								break;
							case 'r':
								PutChar(buf, lengthOutput, (CHAR)'\r');
								break;
							case 't':
								PutChar(buf, lengthOutput, (CHAR)'\t');
								break;
							case 'b':
								PutChar(buf, lengthOutput, (CHAR)'\b');
								break;
							case 'f':
								PutChar(buf, lengthOutput, (CHAR)'\f');
								break;
							case 'a':
								PutChar(buf, lengthOutput, (CHAR)'\a');
								break;
							case '0': case '1': case '2': case '3':
							case '4': case '5': case '6': case '7':
								{
									sl_uint32 t = ch - '0';
									ParseOctetValue<CHAR>(src, lengthSrc, i, t);
									PutChar(buf, lengthOutput, (CHAR)t);
									break;
								}
							case 'x':
								if (i < lengthSrc) {
									sl_uint32 t;
									if (ParseHexValue<CHAR>(src, lengthSrc, i, t)) {
										PutChar(buf, lengthOutput, (CHAR)t);
									} else {
										flagError = sl_true;
									}
								} else {
									flagError = sl_true;
								}
								break;
							case 'u':
								if (i + 4 <= lengthSrc) {
									sl_uint32 code = 0;
									if (ParseHexValue_FixedLength(src, 4, i, code)) {
										if (sizeof(CHAR) == 2) {
											PutChar(buf, lengthOutput, (CHAR)code);
										} else {
											sl_bool flagValid;
											if (SLIB_CHAR_IS_SURROGATE(code) && i + 6 <= lengthSrc) {
												flagValid = sl_false;
												if (src[i] == '\\' && src[i + 1] == 'u') {
													i += 2;
													sl_uint32 code2 = 0;
													if (ParseHexValue_FixedLength(src, 4, i, code2)) {
														if (SLIB_CHAR_IS_SURROGATE(code2)) {
															code = Charsets::getUnicodeFromSurrogateCharacters(code, code2);
															if (code) {
																flagValid = sl_true;
															}
														} else {
															code = code2;
															flagValid = sl_true;
														}
													}
												}
											} else {
												flagValid = sl_true;
											}
											if (flagValid) {
												sl_size n = Charsets::getUtfn(code, buf ? buf + lengthOutput : sl_null);
												if (n) {
													lengthOutput += n;
												} else {
													flagError = sl_true;
												}
											} else {
												flagError = sl_true;
											}
										}
									} else {
										flagError = sl_true;
									}
								} else {
									flagError = sl_true;
								}
								break;
							case 'U':
								if (i + 8 <= lengthSrc) {
									sl_uint32 code = 0;
									if (ParseHexValue_FixedLength(src, 8, i, code)) {
										sl_size n = Charsets::getUtfn(code, buf ? buf + lengthOutput : sl_null);
										if (n) {
											lengthOutput += n;
										} else {
											flagError = sl_true;
										}
									} else {
										flagError = sl_true;
									}
								} else {
									flagError = sl_true;
								}
								break;
							default:
								flagError = sl_true;
								break;
						}
						if (flagError) {
							break;
						}
					} else {
						if (ch == chEnd) {
							flagSuccess = sl_true;
							break;
						} else {
							PutChar(buf, lengthOutput, ch);
						}
					}
				}
				if (lengthParsed) {
					*lengthParsed = i;
				}
				if (flagSuccess) {
					if (outFlagError) {
						*outFlagError = sl_false;
					}
				}
				return lengthOutput;
			}

			template <class VIEW>
			static typename VIEW::StringType ParseBackslashEscapes(const VIEW& str, sl_size* lengthParsed, sl_bool* outFlagError) noexcept
			{
				if (str.isNull()) {
					return sl_null;
				}
				typename VIEW::Char* data = str.getUnsafeData();
				sl_size len = str.getUnsafeLength();
				sl_size n = ParseBackslashEscapes(data, len, lengthParsed, outFlagError, (typename VIEW::Char*)sl_null);
				if (!n) {
					return VIEW::StringType::getEmpty();
				}
				typename VIEW::StringType ret = VIEW::StringType::allocate(n);
				if (ret.isNull()) {
					return sl_null;
				}
				ParseBackslashEscapes(data, len, lengthParsed, outFlagError, ret.getData());
				return ret;
			}

			template <class CHAR>
			static sl_size CountLineNumber(const CHAR* input, sl_size len, sl_size* columnLast) noexcept
			{
				sl_size line = 1;
				sl_size col = 1;
				for (sl_size i = 0; i < len; i++) {
					CHAR ch = input[i];
					if (!ch) {
						break;
					}
					if (ch == '\r') {
						line++;
						col = 0;
						if (i + 1 < len && input[i + 1] == '\n') {
							i++;
						}
					} else if (ch == '\n') {
						line++;
						col = 0;
					}
					col++;
				}
				if (columnLast) {
					*columnLast = col;
				}
				return line;
			}

			template <class VIEW>
			SLIB_INLINE static sl_size CountLineNumber(const VIEW& str, sl_size* columnLast) noexcept
			{
				if (str.isNull()) {
					return 0;
				}
				return CountLineNumber(str.getUnsafeData(), str.getUnsafeLength(), columnLast);
			}

			template <class STRING>
			static List<STRING> SplitLinesSub(const STRING& str, typename STRING::Char const* data, sl_size len) noexcept
			{
				List<STRING> ret;
				sl_size start = 0;
				for (sl_size i = 0; i < len; i++) {
					typename STRING::Char ch = data[i];
					if (!ch) {
						break;
					}
					if (ch == '\r') {
						ret.add_NoLock(str.substring(start, i));
						if (i + 1 < len && str[i + 1] == '\n') {
							i++;
						}
						start = i + 1;
					} else if (ch == '\n') {
						ret.add_NoLock(str.substring(start, i));
						start = i + 1;
					}
				}
				ret.add_NoLock(str.substring(start));
				return ret;
			}

			template <class STRING>
			SLIB_INLINE static List<STRING> SplitLines(const STRING& str) noexcept
			{
				if (str.isNull()) {
					return sl_null;
				}
				sl_size len;
				typename STRING::Char const* data = str.getData(len);
				return SplitLinesSub(str, data, len);
			}

			template <class VIEW>
			SLIB_INLINE static List<VIEW> SplitLinesView(const VIEW& str) noexcept
			{
				if (str.isNull()) {
					return sl_null;
				}
				return SplitLinesSub(str, str.getUnsafeData(), str.getUnsafeLength());
			}

			template <class VIEW, class CHECKER>
			static sl_reg IndexOf(const VIEW& str, sl_reg _start, const CHECKER& checker) noexcept
			{
				if (str.isNull()) {
					return -1;
				}
				sl_reg _count = str.getUnsafeLength();
				if (_count < 0) {
					sl_size start;
					if (_start < 0) {
						start = 0;
					} else {
						start = _start;
					}
					typename VIEW::Char* data = str.getUnsafeData();
					for (sl_size i = start; ; i++) {
						typename VIEW::Char ch = data[i];
						if (!ch) {
							break;
						}
						if (checker(ch)) {
							return i;
						}
					}
				} else {
					sl_size count = _count;
					sl_size start;
					if (_start < 0) {
						start = 0;
					} else {
						start = _start;
						if (start >= count) {
							return -1;
						}
					}
					typename VIEW::Char* data = str.getUnsafeData();
					for (sl_size i = start; i < count; i++) {
						typename VIEW::Char ch = data[i];
						if (checker(ch)) {
							return i;
						}
					}
				}
				return -1;
			}

			class LineChecker
			{
			public:
				template <class T>
				sl_bool operator()(const T& ch) const noexcept
				{
					return ch == '\r' || ch == '\n';
				}
			};

			class NotLineChecker
			{
			public:
				template <class T>
				sl_bool operator()(const T& ch) const
				{
					return ch != '\r' && ch != '\n';
				}
			};

			class WhitespaceChecker
			{
			public:
				template <class T>
				sl_bool operator()(const T& ch) const noexcept
				{
					return SLIB_CHAR_IS_WHITE_SPACE(ch);
				}
			};

			class NotWhitespaceChecker
			{
			public:
				template <class T>
				sl_bool operator()(const T& ch) const noexcept
				{
					return !SLIB_CHAR_IS_WHITE_SPACE(ch);
				}
			};

			template <class C>
			class CharListChecker
			{
			public:
				C * list;
				sl_size count;

			public:
				template <class T>
				sl_bool operator()(const T& ch) const noexcept
				{
					for (sl_size i = 0; i < count; i++) {
						if (ch == list[i]) {
							return sl_true;
						}
					}
					return sl_false;
				}
			};

			template <class C>
			class NotCharListChecker
			{
			public:
				C * list;
				sl_size count;

			public:
				template <class T>
				sl_bool operator()(const T& ch) const noexcept
				{
					for (sl_size i = 0; i < count; i++) {
						if (ch == list[i]) {
							return sl_false;
						}
					}
					return sl_true;
				}
			};

			template <class VIEW>
			static sl_reg GetWord(typename VIEW::StringType& _out, const VIEW& str, sl_reg start) noexcept
			{
				sl_reg index = IndexOf(str, start, NotWhitespaceChecker());
				if (index >= 0) {
					sl_reg index2 = IndexOf(str, index, WhitespaceChecker());
					if (index2 >= 0) {
						_out = str.substring(index, index2);
						return index2;
					} else {
						_out = str.substring(index);
						return str.getLength();
					}
				}
				return -1;
			}

			template <class VIEW>
			static List<typename VIEW::StringType> GetWords(const VIEW& str, sl_reg start)
			{
				List<typename VIEW::StringType> ret;
				sl_reg index = start;
				typename VIEW::StringType s;
				for (;;) {
					index = GetWord(s, str, index);
					if (index < 0) {
						break;
					}
					ret.add_NoLock(Move(s));
				}
				return ret;
			}

			template <class VIEW>
			static sl_reg IndexOfWholeWord(const VIEW& str, const VIEW& word, sl_reg _start) noexcept
			{
				if (str.isNull()) {
					return -1;
				}
				sl_size nSrc = str.getLength();
				sl_size nWhat = word.getLength();
				sl_size start;
				if (_start < 0) {
					start = 0;
				} else {
					start = _start;
					if (start + nWhat > nSrc) {
						return -1;
					}
				}
				if (!nWhat) {
					return start;
				}
				typename VIEW::Char* what = word.getUnsafeData();
				typename VIEW::Char* src = str.getUnsafeData();
				for (sl_size i = start; i < nSrc; i++) {
					typename VIEW::Char ch = src[i];
					if (SLIB_CHAR_IS_WHITE_SPACE(ch)) {
						if (VIEW(src + start, i - start) == what) {
							return start;
						}
						i++;
						while (i < nSrc) {
							ch = src[i];
							if (!SLIB_CHAR_IS_WHITE_SPACE(ch)) {
								break;
							}
							i++;
						}
						start = i;
					}
				}
				if (start < nSrc) {
					if (VIEW(src + start, nSrc - start) == what) {
						return start;
					}
				}
				return -1;
			}

			template <class VIEW>
			static sl_reg IndexOfWholeWord_IgnoreCase(const VIEW& str, const VIEW& word, sl_reg _start) noexcept
			{
				if (str.isNull()) {
					return -1;
				}
				sl_size nSrc = str.getLength();
				sl_size nWhat = word.getLength();
				sl_size start;
				if (_start < 0) {
					start = 0;
				} else {
					start = _start;
					if (start + nWhat > nSrc) {
						return -1;
					}
				}
				if (!nWhat) {
					return start;
				}
				typename VIEW::Char* what = word.getUnsafeData();
				typename VIEW::Char* src = str.getUnsafeData();
				for (sl_size i = start; i < nSrc; i++) {
					typename VIEW::Char ch = src[i];
					if (SLIB_CHAR_IS_WHITE_SPACE(ch)) {
						if (VIEW(src + start, i - start).equals_IgnoreCase(what)) {
							return start;
						}
						i++;
						while (i < nSrc) {
							ch = src[i];
							if (!SLIB_CHAR_IS_WHITE_SPACE(ch)) {
								break;
							}
							i++;
						}
						start = i;
					}
				}
				if (start < nSrc) {
					if (VIEW(src + start, nSrc - start).equals_IgnoreCase(what)) {
						return start;
					}
				}
				return -1;
			}

			template <class VIEW>
			sl_bool ParseUint32Range(const VIEW& str, sl_uint32* _from, sl_uint32* _to)
			{
				sl_uint32 from;
				sl_uint32 to;
				sl_reg index = str.indexOf('-');
				if (index > 0) {
					if (str.substring(0, index).parseUint32(10, &from)) {
						if (str.substring(index + 1).parseUint32(10, &to)) {
							if (to >= from) {
								if (_from) {
									*_from = from;
								}
								if (_to) {
									*_to = to;
								}
								return sl_true;
							}
						}
					}
				} else {
					if (str.substring(0, index).parseUint32(10, &from)) {
						if (_from) {
							*_from = from;
						}
						if (_to) {
							*_to = from;
						}
						return sl_true;
					}
				}
				return sl_false;
			}

		}
	}

	using namespace priv::stringx;

#define PRIV_STRINGX_MEMBERS(VIEW) \
	typename VIEW::StringType Stringx::applyBackslashEscapes(const VIEW& str, sl_bool flagDoubleQuote, sl_bool flagAddQuote, sl_bool flagEscapeNonAscii) noexcept \
	{ \
		return ApplyBackslashEscapes(str, flagDoubleQuote, flagAddQuote, flagEscapeNonAscii); \
	} \
	\
	typename VIEW::StringType Stringx::parseBackslashEscapes(const VIEW& str, sl_size* lengthParsed, sl_bool* outFlagError) noexcept \
	{ \
		return ParseBackslashEscapes(str, lengthParsed, outFlagError); \
	} \
	\
	sl_size Stringx::countLineNumber(const VIEW& str, sl_size* columnLast) noexcept \
	{ \
		return CountLineNumber(str, columnLast); \
	} \
	\
	List<VIEW> Stringx::splitLines(const VIEW& str) noexcept \
	{ \
		return SplitLinesView(str); \
	} \
	\
	List<typename VIEW::StringType> Stringx::splitLines(typename VIEW::StringType const& str) noexcept \
	{ \
		return SplitLines(str); \
	} \
	\
	sl_reg Stringx::indexOfLine(const VIEW& str, sl_reg start) noexcept \
	{ \
		return IndexOf(str, start, LineChecker()); \
	} \
	\
	sl_reg Stringx::indexOfNotLine(const VIEW& str, sl_reg start) noexcept \
	{ \
		return IndexOf(str, start, NotLineChecker()); \
	} \
	\
	sl_reg Stringx::indexOfWhitespace(const VIEW& str, sl_reg start) noexcept \
	{ \
		return IndexOf(str, start, WhitespaceChecker()); \
	} \
	\
	sl_reg Stringx::indexOfNotWhitespace(const VIEW& str, sl_reg start) noexcept \
	{ \
		return IndexOf(str, start, NotWhitespaceChecker()); \
	} \
	\
	sl_reg Stringx::indexOfChar(const VIEW& str, const ListParam<typename VIEW::Char>& _list, sl_reg start) noexcept \
	{ \
		ListLocker<typename VIEW::Char> list(_list); \
		CharListChecker<typename VIEW::Char> checker; \
		checker.list = list.data; \
		checker.count = list.count; \
		return IndexOf(str, start, checker); \
	} \
	\
	sl_reg Stringx::indexOfNotChar(const VIEW& str, const ListParam<typename VIEW::Char>& _list, sl_reg start) noexcept \
	{ \
		ListLocker<typename VIEW::Char> list(_list); \
		NotCharListChecker<typename VIEW::Char> checker; \
		checker.list = list.data; \
		checker.count = list.count; \
		return IndexOf(str, start, checker); \
	} \
	\
	sl_reg Stringx::getWord(typename VIEW::StringType& _out, const VIEW& str, sl_reg start) noexcept \
	{ \
		return GetWord(_out, str, start); \
	} \
	\
	List<typename VIEW::StringType> Stringx::getWords(const VIEW& str, sl_reg start) noexcept \
	{ \
		return GetWords(str, start); \
	} \
	\
	sl_reg Stringx::indexOfWholeWord(const VIEW& str, const VIEW& word, sl_reg start) noexcept \
	{ \
		return IndexOfWholeWord(str, word, start); \
	} \
	\
	sl_reg Stringx::indexOfWholeWord_IgnoreCase(const VIEW& str, const VIEW& word, sl_reg start) noexcept \
	{ \
		return IndexOfWholeWord_IgnoreCase(str, word, start); \
	}

	PRIV_STRINGX_MEMBERS(StringView)
	PRIV_STRINGX_MEMBERS(StringView16)
	PRIV_STRINGX_MEMBERS(StringView32)


	sl_bool Stringx::parseUint32Range(const StringParam& str, sl_uint32* from, sl_uint32* to)
	{
		if (str.isEmpty()) {
			return sl_false;
		}
		if (str.is8BitsStringType()) {
			return ParseUint32Range(StringData(str), from, to);
		} else if (str.is16BitsStringType()) {
			return ParseUint32Range(StringData16(str), from, to);
		} else {
			return ParseUint32Range(StringData32(str), from, to);
		}
		return sl_false;
	}

}
