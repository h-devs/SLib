/*
 *	Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/core/parse.h"
#include "slib/core/parse_util.h"

#include "slib/core/list.h"
#include "slib/core/scoped_buffer.h"
#include "slib/core/math.h"

namespace slib
{

	namespace priv
	{
		namespace parse
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
				for (; i < lengthSrc; i++) {
					ch = src[i];
					sl_bool flagError = sl_false;
					sl_bool flagBackslash = sl_false;
					switch (ch) {
						case 0:
							break;
						case '\\':
							flagBackslash = sl_true;
							i++;
							if (i < lengthSrc) {
								ch = src[i];
								switch (ch) {
									case '\\':
									case '"':
									case '\'':
									case '/':
										break;
									case 'n':
										ch = '\n';
										break;
									case 'r':
										ch = '\r';
										break;
									case 't':
										ch = '\t';
										break;
									case 'b':
										ch = '\b';
										break;
									case 'f':
										ch = '\f';
										break;
									case 'a':
										ch = '\a';
										break;
									case '0': case '1': case '2': case '3':
									case '4': case '5': case '6': case '7':
										{
											i++;
											sl_uint32 t = ch - '0';
											while (i < lengthSrc) {
												ch = src[i];
												if (ch >= '0' && ch < '8') {
													t = (t << 3) | (ch - '0');
													i++;
												} else {
													break;
												}
											}
											i--;
											ch = (CHAR)t;
											break;
										}
									case 'x':
										{
											if (i + 1 < lengthSrc) {
												i++;
												sl_uint32 h = SLIB_CHAR_HEX_TO_INT(src[i]);
												if (h < 16) {
													sl_uint32 t = h;
													i++;
													while (i < lengthSrc) {
														ch = src[i];
														h = SLIB_CHAR_HEX_TO_INT(ch);
														if (h < 16) {
															t = (t << 4) | h;
															i++;
														} else {
															break;
														}
													}
													ch = (CHAR)t;
												} else {
													flagError = sl_true;
												}
												i--;
											} else {
												flagError = sl_true;
											}
											break;
										}
									case 'u':
										{
											if (i + 4 < lengthSrc) {
												i++;
												sl_uint16 t = 0;
												{
													for (int k = 0; k < 4; k++) {
														ch = src[i];
														sl_uint16 h = (sl_uint16)(SLIB_CHAR_HEX_TO_INT(ch));
														if (h < 16) {
															t = (t << 4) | h;
															i++;
														} else {
															flagError = sl_true;
															break;
														}
													}
												}
												if (!flagError) {
													if (sizeof(CHAR) == 1) {
														if (t >= 0xD800 && t < 0xDC00) {
															if (i + 5 < lengthSrc) {
																if (src[i] == '\\' && src[i + 1] == 'u') {
																	i += 2;
																	sl_uint16 t2 = 0;
																	for (int k = 0; k < 4; k++) {
																		ch = src[i];
																		sl_uint16 h = (sl_uint16)(SLIB_CHAR_HEX_TO_INT(ch));
																		if (h < 16) {
																			t2 = (t2 << 4) | h;
																			i++;
																		} else {
																			flagError = sl_true;
																			break;
																		}
																	}
																	if (!flagError) {
																		sl_char8 u[6];
																		sl_char16 a[] = { t, t2 };
																		sl_size nu = Charsets::utf16ToUtf8(a, 2, u, 6);
																		if (nu > 0) {
																			for (sl_size iu = 0; iu < nu - 1; iu++) {
																				if (buf) {
																					buf[lengthOutput++] = (CHAR)(u[iu]);
																				} else {
																					lengthOutput++;
																				}
																			}
																			ch = (CHAR)(u[nu - 1]);
																		}
																	}
																}
															}
														} else {
															sl_char8 u[3];
															sl_size nu = Charsets::utf16ToUtf8((sl_char16*)&t, 1, u, 3);
															if (nu > 0) {
																for (sl_size iu = 0; iu < nu - 1; iu++) {
																	if (buf) {
																		buf[lengthOutput++] = (CHAR)(u[iu]);
																	} else {
																		lengthOutput++;
																	}
																}
																ch = (CHAR)(u[nu - 1]);
															}
														}
													} else {
														ch = (CHAR)t;
													}
												}
												i--;
											} else {
												flagError = sl_true;
											}
											break;
										}
									case 'U':
										{
											if (i + 8 < lengthSrc) {
												i++;
												sl_uint32 t = 0;
												for (int k = 0; k < 8; k++) {
													ch = src[i];
													sl_uint32 h = SLIB_CHAR_HEX_TO_INT(ch);
													if (h < 16) {
														t = (t << 4) | h;
														i++;
													} else {
														flagError = sl_true;
														break;
													}
												}
												if (!flagError) {
													if (sizeof(CHAR) == 1) {
														sl_char8 u[6];
														sl_char32 _t = t;
														sl_size nu = Charsets::utf32ToUtf8(&_t, 1, u, 6);
														if (nu > 0) {
															for (sl_size iu = 0; iu < nu - 1; iu++) {
																if (buf) {
																	buf[lengthOutput++] = (CHAR)(u[iu]);
																} else {
																	lengthOutput++;
																}
															}
															ch = (CHAR)(u[nu - 1]);
														} else {
															flagError = sl_true;
														}
													} else if (sizeof(CHAR) == 2) {
														sl_char16 u[2];
														sl_char32 _t = t;
														sl_size nu = Charsets::utf32ToUtf16(&_t, 1, u, 2);
														if (nu > 0) {
															for (sl_size iu = 0; iu < nu - 1; iu++) {
																if (buf) {
																	buf[lengthOutput++] = (CHAR)(u[iu]);
																} else {
																	lengthOutput++;
																}
															}
															ch = (CHAR)(u[nu - 1]);
														} else {
															flagError = sl_true;
														}
													} else {
														ch = (CHAR)t;
													}
												}
												i--;
											} else {
												flagError = sl_true;
											}
											break;
										}
									default:
										flagError = sl_true;
										break;
								}
							} else {
								flagError = sl_true;
							}
							break;
						case '\r':
						case '\n':
						case '\v':
							flagError = sl_true;
							break;
					}
					if (flagError) {
						break;
					} else {
						if (ch == chEnd && !flagBackslash) {
							flagSuccess = sl_true;
							i++;
							break;
						} else {
							if (buf) {
								buf[lengthOutput++] = ch;
							} else {
								lengthOutput++;
							}
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
				sl_size count = str.getUnsafeLength();
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
					if (!ch) {
						break;
					}
					if (checker(ch)) {
						return i;
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

		}
	}

	using namespace priv::parse;

#define PRIV_PARSE_UTIL_MEMBERS(VIEW) \
	typename VIEW::StringType ParseUtil::applyBackslashEscapes(const VIEW& str, sl_bool flagDoubleQuote, sl_bool flagAddQuote, sl_bool flagEscapeNonAscii) noexcept \
	{ \
		return ApplyBackslashEscapes(str, flagDoubleQuote, flagAddQuote, flagEscapeNonAscii); \
	} \
	\
	typename VIEW::StringType ParseUtil::parseBackslashEscapes(const VIEW& str, sl_size* lengthParsed, sl_bool* outFlagError) noexcept \
	{ \
		return ParseBackslashEscapes(str, lengthParsed, outFlagError); \
	} \
	\
	sl_size ParseUtil::countLineNumber(const VIEW& str, sl_size* columnLast) noexcept \
	{ \
		return CountLineNumber(str, columnLast); \
	} \
	\
	List<VIEW> ParseUtil::splitLines(const VIEW& str) noexcept \
	{ \
		return SplitLinesView(str); \
	} \
	\
	List<typename VIEW::StringType> ParseUtil::splitLines(typename VIEW::StringType const& str) noexcept \
	{ \
		return SplitLines(str); \
	} \
	\
	sl_reg ParseUtil::indexOfLine(const VIEW& str, sl_reg start) noexcept \
	{ \
		return IndexOf(str, start, LineChecker()); \
	} \
	\
	sl_reg ParseUtil::indexOfNotLine(const VIEW& str, sl_reg start) noexcept \
	{ \
		return IndexOf(str, start, NotLineChecker()); \
	} \
	\
	sl_reg ParseUtil::indexOfWhitespace(const VIEW& str, sl_reg start) noexcept \
	{ \
		return IndexOf(str, start, WhitespaceChecker()); \
	} \
	\
	sl_reg ParseUtil::indexOfNotWhitespace(const VIEW& str, sl_reg start) noexcept \
	{ \
		return IndexOf(str, start, NotWhitespaceChecker()); \
	} \
	\
	sl_reg ParseUtil::indexOfChar(const VIEW& str, const ListParam<typename VIEW::Char>& _list, sl_reg start) noexcept \
	{ \
		ListLocker<typename VIEW::Char> list(_list); \
		CharListChecker<typename VIEW::Char> checker; \
		checker.list = list.data; \
		checker.count = list.count; \
		return IndexOf(str, start, checker); \
	} \
	\
	sl_reg ParseUtil::indexOfNotChar(const VIEW& str, const ListParam<typename VIEW::Char>& _list, sl_reg start) noexcept \
	{ \
		ListLocker<typename VIEW::Char> list(_list); \
		NotCharListChecker<typename VIEW::Char> checker; \
		checker.list = list.data; \
		checker.count = list.count; \
		return IndexOf(str, start, checker); \
	} \
	\
	sl_reg ParseUtil::getWord(typename VIEW::StringType& _out, const VIEW& str, sl_reg start) noexcept \
	{ \
		return GetWord(_out, str, start); \
	} \
	\
	List<typename VIEW::StringType> ParseUtil::getWords(const VIEW& str, sl_reg start) noexcept \
	{ \
		return GetWords(str, start); \
	}

	PRIV_PARSE_UTIL_MEMBERS(StringView)
	PRIV_PARSE_UTIL_MEMBERS(StringView16)
	PRIV_PARSE_UTIL_MEMBERS(StringView32)

}
