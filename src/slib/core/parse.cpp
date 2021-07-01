/*
 *	Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

			template <class ST, class CT>
			static sl_size ApplyBackslashEscapes(const ST& s, sl_bool flagDoubleQuote, sl_bool flagAddQuote, sl_bool flagEscapeNonAscii, CT* buf) noexcept
			{
				const CT* ch = s.getData();
				sl_size len = s.getUnsafeLength();
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
					CT c = ch[i];
					CT r = 0;
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
					} else {
						if (flagEscapeNonAscii && ((sl_uint8)c < 32 || (sl_uint8)c > 126)) {
							if (sizeof(CT) == 1) {
								if (buf) {
									sl_uint8 t = (sl_uint8)c;
									buf[d++] = '\\';
									buf[d++] = 'x';
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 4) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[t & 15];
								} else {
									d += 4;
								}
							} else {
								if (buf) {
									sl_uint16 t = (sl_uint16)c;
									buf[d++] = '\\';
									buf[d++] = 'x';
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 12) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 8) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[(t >> 4) & 15];
									buf[d++] = priv::string::g_conv_radixPatternLower[t & 15];
								} else {
									d += 6;
								}
							}
							flagPrevEscaped = sl_true;
						} else {
							if (flagPrevEscaped) {
								if (SLIB_CHAR_IS_HEX(c)) {
									if (buf) {
										CT t = flagDoubleQuote ? '"' : '\'';
										buf[d++] = t;
										buf[d++] = t;
									} else {
										d += 2;
									}
								}
							}
							if (buf) {
								buf[d++] = c;
							} else {
								d++;
							}
							flagPrevEscaped = sl_false;
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

		}
	}

	String ParseUtil::applyBackslashEscapes(const StringParam& _str, sl_bool flagDoubleQuote, sl_bool flagAddQuote, sl_bool flagEscapeNonAscii) noexcept
	{
		StringData str(_str);
		sl_size n = priv::parse::ApplyBackslashEscapes<StringData, sl_char8>(str, flagDoubleQuote, flagAddQuote, flagEscapeNonAscii, sl_null);
		if (n == 0) {
			return String::getEmpty();
		}
		String ret = String::allocate(n);
		if (ret.isEmpty()) {
			return sl_null;
		}
		priv::parse::ApplyBackslashEscapes<StringData, sl_char8>(str, flagDoubleQuote, flagAddQuote, flagEscapeNonAscii, ret.getData());
		return ret;
	}

	String16 ParseUtil::applyBackslashEscapes16(const StringParam& _str, sl_bool flagDoubleQuote, sl_bool flagAddQuote, sl_bool flagEscapeNonAscii) noexcept
	{
		StringData16 str(_str);
		sl_size n = priv::parse::ApplyBackslashEscapes<StringData16, sl_char16>(str, flagDoubleQuote, flagAddQuote, flagEscapeNonAscii, sl_null);
		String16 ret = String16::allocate(n);
		if (ret.isEmpty()) {
			return sl_null;
		}
		priv::parse::ApplyBackslashEscapes<StringData16, sl_char16>(str, flagDoubleQuote, flagAddQuote, flagEscapeNonAscii, ret.getData());
		return ret;
	}

	namespace priv
	{
		namespace parse
		{

			template <class ST, class CT>
			static ST ParseBackslashEscapes(const CT* sz, sl_size n, sl_size* lengthParsed, sl_bool* outFlagError) noexcept
			{
				if (lengthParsed) {
					*lengthParsed = 0;
				}
				if (outFlagError) {
					*outFlagError = sl_true;
				}
				if (n <= 0) {
					return sl_null;
				}
				CT chEnd = 0;
				if (sz[0] == '"') {
					chEnd = '"';
				} else if (sz[0] == '\'') {
					chEnd = '\'';
				} else {
					return sl_null;
				}
				SLIB_SCOPED_BUFFER(CT, 2048, buf, n);
				if (buf == sl_null) {
					return sl_null;
				}
				sl_size len = 0;
				sl_bool flagSuccess = sl_false;
				sl_size i = 1;
				for (; i < n; i++) {
					CT ch = sz[i];
					sl_bool flagError = sl_false;
					sl_bool flagBackslash = sl_false;
					switch (ch) {
					case 0:
						break;
					case '\\':
						flagBackslash = sl_true;
						i++;
						if (i < n) {
							ch = sz[i];
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
								sl_size nh = 2;
								sl_uint32 t = ch - '0';
								while (i < n && nh > 0) {
									ch = sz[i];
									if (ch >= '0' && ch < '8') {
										t = (t << 3) | (ch - '0');
										i++;
									} else {
										break;
									}
								}
								i--;
								ch = (CT)t;
								break;
							}
							case 'x':
							{
								i++;
								sl_uint32 h = SLIB_CHAR_HEX_TO_INT(sz[i]);
								if (h < 16) {
									i++;
									sl_uint32 t = h;
									sl_size nh;
									if (sizeof(CT) == 1) {
										nh = 1;
									} else {
										nh = 3;
									}
									while (i < n && nh > 0) {
										ch = sz[i];
										h = SLIB_CHAR_HEX_TO_INT(ch);
										if (h < 16) {
											t = (t << 4) | h;
											i++;
										} else {
											break;
										}
									}
								} else {
									flagError = sl_true;
								}
								i--;
								break;
							}
							case 'u':
							{
								if (i + 4 < n) {
									i++;
									sl_uint16 t = 0;
									{
										for (int k = 0; k < 4; k++) {
											ch = sz[i];
											sl_uint16 h = SLIB_CHAR_HEX_TO_INT(ch);
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
										if (sizeof(CT) == 1) {
											if (t >= 0xD800 && t < 0xDC00) {
												if (i + 5 < n) {
													if (sz[i] == '\\' && sz[i + 1] == 'u') {
														i += 2;
														sl_uint16 t2 = 0;
														for (int k = 0; k < 4; k++) {
															ch = sz[i];
															sl_uint16 h = SLIB_CHAR_HEX_TO_INT(ch);
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
																	buf[len++] = (CT)(u[iu]);
																}
																ch = (CT)(u[nu - 1]);
															}
														}
													}
												}
											} else {
												sl_char8 u[3];
												sl_size nu = Charsets::utf16ToUtf8((sl_char16*)&t, 1, u, 3);
												if (nu > 0) {
													for (sl_size iu = 0; iu < nu - 1; iu++) {
														buf[len++] = (CT)(u[iu]);
													}
													ch = (CT)(u[nu - 1]);
												}
											}
										} else {
											ch = (CT)t;
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
								if (i + 8 < n) {
									i++;
									sl_uint32 t = 0;
									for (int k = 0; k < 4; k++) {
										ch = sz[i];
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
										if (sizeof(CT) == 1) {
											sl_char8 u[6];
											sl_char32 _t = t;
											sl_size nu = Charsets::utf32ToUtf8(&_t, 1, u, 6);
											if (nu > 0) {
												for (sl_size iu = 0; iu < nu - 1; iu++) {
													buf[len++] = (CT)(u[iu]);
												}
												ch = (CT)(u[nu - 1]);
											} else {
												flagError = sl_true;
											}
										} else {
											sl_char16 u[2];
											sl_char32 _t = t;
											sl_size nu = Charsets::utf32ToUtf16(&_t, 1, u, 2);
											if (nu > 0) {
												for (sl_size iu = 0; iu < nu - 1; iu++) {
													buf[len++] = (CT)(u[iu]);
												}
												ch = (CT)(u[nu - 1]);
											} else {
												flagError = sl_true;
											}
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
							buf[len++] = ch;
						}
					}
				}
				if (lengthParsed) {
					*lengthParsed = i;
				}
				ST ret;
				if (flagSuccess) {
					ret = ST(buf, len);
					if (outFlagError) {
						*outFlagError = sl_false;
					}
				}
				return ret;
			}

		}
	}

	String ParseUtil::parseBackslashEscapes(const StringParam& _str, sl_size* lengthParsed, sl_bool* outFlagError) noexcept
	{
		StringData str(_str);
		return priv::parse::ParseBackslashEscapes<String, sl_char8>(str.getData(), str.getUnsafeLength(), lengthParsed, outFlagError);
	}

	String16 ParseUtil::parseBackslashEscapes16(const StringParam& _str, sl_size* lengthParsed, sl_bool* outFlagError) noexcept
	{
		StringData16 str(_str);
		return priv::parse::ParseBackslashEscapes<String16, sl_char16>(str.getData(), str.getUnsafeLength(), lengthParsed, outFlagError);
	}


	namespace priv
	{
		namespace parse
		{

			template <class CT>
			static sl_size CountLineNumber(const CT* input, sl_size len, sl_size* columnLast) noexcept
			{
				sl_size line = 1;
				sl_size col = 1;
				for (sl_size i = 0; i < len; i++) {
					CT ch = input[i];
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
		
			template <class ST, class CT>
			static List<ST> SplitLines(const CT* input, sl_size len)
			{
				List<ST> ret;
				sl_size start = 0;
				for (sl_size i = 0; i < len; i++) {
					CT ch = input[i];
					if (!ch) {
						break;
					}
					if (ch == '\r') {
						ret.add_NoLock(input + start, i - start);
						if (i + 1 < len && input[i + 1] == '\n') {
							i++;
						}
						start = i + 1;
					} else if (ch == '\n') {
						ret.add_NoLock(input + start, i - start);
						start = i + 1;
					}
				}
				ret.add_NoLock(input + start, len - start);
				return ret;
			}
			
		}
	}

	sl_size ParseUtil::countLineNumber(const StringParam& _str, sl_size* columnLast) noexcept
	{
		if (_str.isNotEmpty()) {
			if (_str.is8()) {
				StringData str(_str);
				sl_size n = str.getUnsafeLength();
				return priv::parse::CountLineNumber(str.getData(), n, columnLast);
			} else {
				StringData16 str(_str);
				sl_size n = str.getUnsafeLength();
				return priv::parse::CountLineNumber(str.getData(), n, columnLast);
			}
		}
		return 0;
	}

	List<String> ParseUtil::splitLines(const String& input) noexcept
	{
		return priv::parse::SplitLines<String>(input.getData(), input.getLength());
	}

	List<String> ParseUtil::splitLines(const AtomicString& input) noexcept
	{
		return splitLines(String(input));
	}

	List<String16> ParseUtil::splitLines(const String16& input) noexcept
	{
		return priv::parse::SplitLines<String16>(input.getData(), input.getLength());
	}

	List<String16> ParseUtil::splitLines(const AtomicString16& input) noexcept
	{
		return splitLines(String16(input));
	}

	List<StringView> ParseUtil::splitLines(const StringView& input) noexcept
	{
		return priv::parse::SplitLines<StringView>(input.getData(), input.getLength());
	}

	List<StringView16> ParseUtil::splitLines(const StringView16& input) noexcept
	{
		return priv::parse::SplitLines<StringView16>(input.getData(), input.getLength());
	}

	namespace priv
	{
		namespace parse
		{

			template <class CT, class ST, class CHECKER>
			static sl_reg IndexOf2(const ST& str, sl_reg _start, const CHECKER& checker) noexcept
			{
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
				CT* sz = str.getData();
				for (sl_size i = start; i < count; i++) {
					CT ch = sz[i];
					if (!ch) {
						break;
					}
					if (checker(ch)) {
						return i;
					}
				}
				return -1;
			}

			template <class CHECKER>
			static sl_reg IndexOf(const StringParam& _str, sl_reg start, const CHECKER& checker) noexcept
			{
				if (_str.isNotNull()) {
					if (_str.is8()) {
						StringData str(_str);
						return IndexOf2<sl_char8, StringData, CHECKER>(str, start, checker);
					} else {
						StringData16 str(_str);
						return IndexOf2<sl_char16, StringData16, CHECKER>(str, start, checker);
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

		}
	}

	sl_reg ParseUtil::indexOfLine(const StringParam& input, sl_reg start) noexcept
	{
		return priv::parse::IndexOf(input, start, priv::parse::LineChecker());
	}

	sl_reg ParseUtil::indexOfNotLine(const StringParam& input, sl_reg start) noexcept
	{
		return priv::parse::IndexOf(input, start, priv::parse::NotLineChecker());
	}

	sl_reg ParseUtil::indexOfWhitespace(const StringParam& input, sl_reg start) noexcept
	{
		return priv::parse::IndexOf(input, start, priv::parse::WhitespaceChecker());
	}

	sl_reg ParseUtil::indexOfNotWhitespace(const StringParam& input, sl_reg start) noexcept
	{
		return priv::parse::IndexOf(input, start, priv::parse::NotWhitespaceChecker());
	}

	sl_reg ParseUtil::indexOfChar(const StringParam& input, const ListParam<sl_char8>& _list, sl_reg start) noexcept
	{
		ListLocker<sl_char8> list(_list);
		priv::parse::CharListChecker<sl_char8> checker;
		checker.list = list.data;
		checker.count = list.count;
		return priv::parse::IndexOf(input, start, checker);
	}

	sl_reg ParseUtil::indexOfNotChar(const StringParam& input, const ListParam<sl_char8>& _list, sl_reg start) noexcept
	{
		ListLocker<sl_char8> list(_list);
		priv::parse::NotCharListChecker<sl_char8> checker;
		checker.list = list.data;
		checker.count = list.count;
		return priv::parse::IndexOf(input, start, checker);
	}

	sl_reg ParseUtil::indexOfChar16(const StringParam& input, const ListParam<sl_char16>& _list, sl_reg start) noexcept
	{
		ListLocker<sl_char16> list(_list);
		priv::parse::CharListChecker<sl_char16> checker;
		checker.list = list.data;
		checker.count = list.count;
		return priv::parse::IndexOf(input, start, checker);
	}

	sl_reg ParseUtil::indexOfNotChar16(const StringParam& input, const ListParam<sl_char16>& _list, sl_reg start) noexcept
	{
		ListLocker<sl_char16> list(_list);
		priv::parse::NotCharListChecker<sl_char16> checker;
		checker.list = list.data;
		checker.count = list.count;
		return priv::parse::IndexOf(input, start, checker);
	}

	namespace priv
	{
		namespace parse
		{

			template <class CT, class ST, class SO>
			static sl_reg GetWord(SO& _out, const ST& str, sl_reg start)
			{
				sl_reg index = IndexOf2<CT, ST, NotWhitespaceChecker>(str, start, NotWhitespaceChecker());
				if (index >= 0) {
					sl_reg index2 = IndexOf2<CT, ST, WhitespaceChecker>(str, index, WhitespaceChecker());
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

			template <class CT, class ST, class SO>
			static List<SO> GetWords(const ST& str, sl_reg start)
			{
				List<SO> ret;
				sl_reg index = start;
				SO s;
				for (;;) {
					index = GetWord<CT, ST, SO>(s, str, index);
					if (index < 0) {
						break;
					}
					ret.add_NoLock(Move(s));
				}
				return ret;
			}

		}
	}

	sl_reg ParseUtil::getWord(String& _out, const StringParam& _input, sl_reg start) noexcept
	{
		StringData input(_input);
		return priv::parse::GetWord<sl_char8, StringData, String>(_out, input, start);
	}

	sl_reg ParseUtil::getWord16(String16& _out, const StringParam& _input, sl_reg start) noexcept
	{
		StringData16 input(_input);
		return priv::parse::GetWord<sl_char16, StringData16, String16>(_out, input, start);
	}

	List<String> ParseUtil::getWords(const StringParam& _input, sl_reg start) noexcept
	{
		StringData input(_input);
		return priv::parse::GetWords<sl_char8, StringData, String>(input, start);
	}

	List<String16> ParseUtil::getWords16(const StringParam& _input, sl_reg start) noexcept
	{
		StringData16 input(_input);
		return priv::parse::GetWords<sl_char16, StringData16, String16>(input, start);
	}

}
