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

/*

 Template Engine for HTML/CSS/JavaScript

 The grammar is based on
	 https://github.com/melpon/ginger
	 https://github.com/qicosmos/render

*/

#include "slib/service/ginger.h"

#include "slib/core/string_buffer.h"
#include "slib/core/parse_util.h"
#include "slib/core/file.h"
#include "slib/core/safe_static.h"

namespace slib
{

	namespace priv
	{
		namespace ginger
		{

			class BuiltIn
			{
			public:
				static Variant format(Variant& param)
				{
					if (param.isVariantList()) {
						ListElements<Variant> list(param.getVariantList());
						if (list.count) {
							if (list.count < 2) {
								return String::format(list[0].getStringView());
							}
							List<Variant> params;
							return String::formatBy(list[0].getStringView(), list.data + 1, list.count - 1);
						}
					} else {
						return String::format(param.getStringView());
					}
					return  Variant();
				}

				static Variant toInt(Variant& param)
				{
					return param.getInt32();
				}

				static Variant toUint(Variant& param)
				{
					return param.getUint32();
				}

				static Variant toInt64(Variant& param)
				{
					return param.getInt64();
				}

				static Variant toUint64(Variant& param)
				{
					return param.getUint64();
				}

				static Variant toFloat(Variant& param)
				{
					return param.getFloat();
				}

				static Variant toDouble(Variant& param)
				{
					return param.getDouble();
				}

				static Variant toString(Variant& param)
				{
					return param.toString();
				}

				static Variant toBool(Variant& param)
				{
					return (sl_bool)param;
				}

				static Variant length(Variant& param)
				{
					return param.getElementsCount();
				}

				static Variant substring(Variant& param)
				{
					VariantList params = param.getVariantList();
					return params.getValueAt(0).getString().substring(params.getValueAt(1).getInt32(0), params.getValueAt(2).getInt32(-1));
				}

				static Variant trim(Variant& param)
				{
					return param.getString().trim();
				}

				static Variant toUpper(Variant& param)
				{
					return param.getString().toUpper();
				}

				static Variant toLower(Variant& param)
				{
					return param.getString().toLower();
				}

				static Variant replaceAll(Variant& param)
				{
					VariantList params = param.getVariantList();
					StringData s1(params.getValueAt(1).getStringParam());
					StringData s2(params.getValueAt(2).getStringParam());
					return params.getValueAt(0).getString().replaceAll(s1, s2);
				}

				static Variant split(Variant& param)
				{
					VariantList params = param.getVariantList();
					StringData s(params.getValueAt(1).getStringParam());
					ListElements<String> strings(params.getValueAt(0).getString().split(s));
					VariantList ret;
					for (sl_size i = 0; i < strings.count; i++) {
						ret.add_NoLock(strings[i]);
					}
					return ret;
				}

				static Variant concat(Variant& param)
				{
					if (param.isVariantList()) {
						ListElements<Variant> list(param.getVariantList());
						if (list.count) {
							List<String> strings;
							for (sl_size i = 0; i < list.count; i++) {
								strings.add_NoLock(list[i].toString());
							}
							return String::join(strings.getData(), strings.getCount());
						}
					} else {
						return param.toString();
					}
					return  Variant();
				}

				static Variant join(Variant& param)
				{
					VariantList params = param.getVariantList();
					ListLocker<Variant> list(params.getValueAt(0).getVariantList());
					if (list.count) {
						List<String> strings;
						for (sl_size i = 0; i < list.count; i++) {
							strings.add_NoLock(list[i].toString());
						}
						return String::join(strings.getData(), strings.getCount(), params.getValueAt(1).getStringView());
					}
					return  Variant();
				}

				static Variant indexOf(Variant& param)
				{
					VariantList params = param.getVariantList();
					Variant s = params.getValueAt(0);
					if (s.isNull()) {
						return -1;
					}
					if (s.isStringType()) {
						return s.getString().indexOf(params.getValueAt(1).getString(), params.getValueAt(2).getInt32());
					}
					if (s.isVariantList()) {
						return s.getVariantList().indexOf(params.getValueAt(1).getString(), params.getValueAt(2).getInt32());
					}
					sl_int64 n = s.getElementsCount();
					if (!n) {
						return -1;
					}
					sl_int64 i = params.getValueAt(2).getInt64();
					if (i < 0) {
						i = 0;
					}
					Variant what = params.getValueAt(1).getString();
					for (; i < n; i++) {
						if (s.getElement(i) == what) {
							return i;
						}
					}
					return -1;
				}

				static Variant lastIndexOf(Variant& param)
				{
					VariantList params = param.getVariantList();
					Variant s = params.getValueAt(0);
					if (s.isNull()) {
						return -1;
					}
					if (s.isStringType()) {
						return s.getString().lastIndexOf(params.getValueAt(1).getString(), params.getValueAt(2).getInt32(-1));
					}
					if (s.isVariantList()) {
						return s.getVariantList().lastIndexOf(params.getValueAt(1).getString(), params.getValueAt(2).getInt32(-1));
					}
					sl_int64 n = s.getElementsCount();
					if (!n) {
						return -1;
					}
					sl_int64 i = params.getValueAt(2).getInt64(-1);
					if (i < 0) {
						i = n - 1;
					}
					Variant what = params.getValueAt(1).getString();
					for (; i >= 0; i--) {
						if (s.getElement(i) == what) {
							return i;
						}
					}
					return -1;
				}

			public:
				BuiltIn()
				{
					if (m_builtins.isEmpty()) {
#define REGISTER_BUILTIN_FUNCTION(NAME) \
						{ \
							SLIB_STATIC_STRING(s, #NAME) \
							m_builtins.put_NoLock(s, Function<Variant(Variant&)>(&BuiltIn::NAME)); \
						}
						REGISTER_BUILTIN_FUNCTION(format)
						REGISTER_BUILTIN_FUNCTION(toInt)
						REGISTER_BUILTIN_FUNCTION(toUint)
						REGISTER_BUILTIN_FUNCTION(toInt64)
						REGISTER_BUILTIN_FUNCTION(toUint64)
						REGISTER_BUILTIN_FUNCTION(toFloat)
						REGISTER_BUILTIN_FUNCTION(toDouble)
						REGISTER_BUILTIN_FUNCTION(toString)
						REGISTER_BUILTIN_FUNCTION(toBool)
						REGISTER_BUILTIN_FUNCTION(length)
						REGISTER_BUILTIN_FUNCTION(substring)
						REGISTER_BUILTIN_FUNCTION(trim)
						REGISTER_BUILTIN_FUNCTION(toUpper)
						REGISTER_BUILTIN_FUNCTION(toLower)
						REGISTER_BUILTIN_FUNCTION(replaceAll)
						REGISTER_BUILTIN_FUNCTION(split)
						REGISTER_BUILTIN_FUNCTION(concat)
						REGISTER_BUILTIN_FUNCTION(join)
						REGISTER_BUILTIN_FUNCTION(indexOf)
						REGISTER_BUILTIN_FUNCTION(lastIndexOf)
					}
				}

			public:
				Variant getBuiltIn(const String& name)
				{
					return m_builtins.getValue_NoLock(name);
				}

			private:
				CHashMap<String, Variant> m_builtins;

			};

			SLIB_SAFE_STATIC_GETTER(BuiltIn, GetBuiltIn)

			class Renderer
			{
			private:
				typedef sl_char8 Char;
				Char* m_input;
				Char* m_current;
				Char* m_end;

				Variant m_data;
				CHashMap<String, Variant> m_locals;

				StringBuffer m_output;
				sl_bool m_flagError;
				sl_bool m_flagEnded;

			public:
				Renderer(const StringView& strTemplate, const Variant& data): m_data(data)
				{
					m_input = strTemplate.getData();
					m_end = m_input + strTemplate.getLength();
					m_current = m_input;

					m_flagError = sl_false;
					m_flagEnded = sl_false;
				}

			public:
				void run()
				{
					processBlock(sl_false);
				}

				String getOutput()
				{
					return m_output.merge();
				}

			private:
				void setError(const StringView& error)
				{
					m_flagError = sl_true;
					m_flagEnded = sl_true;
				}

				SLIB_INLINE void writeStatic(sl_bool flagSkip, const Char* buf, sl_size n)
				{
					if (flagSkip) {
						return;
					}
					m_output.addStatic(buf, n);
				}

				SLIB_INLINE void write(sl_bool flagSkip, const String& str)
				{
					if (flagSkip) {
						return;
					}
					m_output.add(str);
				}

				void skipWhitespace(sl_bool flagCanSetEnded = sl_true)
				{
					while (m_current < m_end) {
						Char ch = *m_current;
						if (SLIB_CHAR_IS_WHITE_SPACE(ch)) {
							m_current++;
						} else {
							return;
						}
					}
					if (flagCanSetEnded) {
						m_flagEnded = sl_true;
					}
				}

				void eatMatched(const Char* sz)
				{
					while (m_current <= m_end) {
						Char s = *(sz++);
						if (!s) {
							return;
						}
						if (m_current >= m_end  || *m_current != s) {
							setError("Invalid pattern");
							return;
						}
						m_current++;
					}
					m_flagEnded = sl_true;
				}

				StringView readVariableName()
				{
					if (m_current < m_end) {
						Char ch = *m_current;
						if (!SLIB_CHAR_IS_DIGIT(ch)) {
							Char* begin = m_current;
							for (;;) {
								if (!SLIB_CHAR_IS_C_NAME(ch)) {
									break;
								}
								m_current++;
								if (m_current >= m_end) {
									m_flagEnded = sl_true;
									break;
								}
								ch = *m_current;
							}
							if (begin < m_current) {
								return StringView(begin, m_current - begin);
							}
						}
					}
					setError("Invalid name");
					return sl_null;
				}

				String readLocalName()
				{
					StringView name = readVariableName();
					if (name.isEmpty()) {
						return sl_null;
					}
					String strName = String::fromStatic(name.getData(), name.getLength());
					if (m_locals.find_NoLock(strName)) {
						setError("Duplicated local name");
						return sl_null;
					}
					return strName;
				}

				StringView readCommand(sl_bool flagCanSetEnded = sl_true)
				{
					Char* begin = m_current;
					while (m_current < m_end) {
						Char ch = *m_current;
						switch (ch) {
							case ' ':
							case '\t':
							case '\r':
							case '\n':
							case '{':
							case '}':
								return StringView(begin, m_current - begin);
							default:
								break;
						}
						m_current++;
					}
					if (flagCanSetEnded) {
						m_flagEnded = sl_true;
					}
					return sl_null;
				}

				enum OperatorPrecedence
				{
					PrecedenceUnary,
					PrecedenceMultiplyDivide,
					PrecedenceAddSub,
					PrecedenceShift,
					PrecedenceCompare,
					PrecedenceEquals,
					PrecedenceBitwiseAnd,
					PrecedenceBitwiseXor,
					PrecedenceBitwiseOr,
					PrecedenceLogicalAnd,
					PrecedenceLogicalOr,
					PrecedenceTernaryConditional,
					PrecedenceMax
				};

				Variant getExpression(sl_bool flagSkip, int maxPrecedence = PrecedenceMax)
				{
#define EXPR_CHECK_ERROR \
	if (m_flagError) { \
		return Variant(); \
	}

#define EXPR_CHECK_ENDED \
	if (m_flagEnded) { \
		return var; \
	}

#define EXPR_SKIP_WHITESPACE_AND_CHECK_ENDED \
	skipWhitespace(); \
	if (m_current >= m_end) { \
		m_flagEnded = sl_true; \
		return var; \
	}

#define EXPR_PROCESS_BINARY_OP_NO_CHECK_ORDER(OP, OP_LEN, MAX_ORDER) \
	{ \
		m_current += OP_LEN; \
		Variant second = getExpression(flagSkip, MAX_ORDER - 1); \
		EXPR_CHECK_ERROR \
		if (!flagSkip) { \
			var = var OP second; \
		} \
		EXPR_CHECK_ENDED \
		break; \
	}

#define EXPR_PROCESS_BINARY_OP(OP, OP_LEN, MAX_ORDER) \
	if (maxPrecedence < MAX_ORDER) { \
		return var; \
	} \
	EXPR_PROCESS_BINARY_OP_NO_CHECK_ORDER(OP, OP_LEN, MAX_ORDER)

					skipWhitespace();
					if (m_flagEnded) {
						setError("Empty expression");
						return Variant();
					}
					Variant var;
					Char ch = *m_current;
					switch (ch) {
						case '-':
							{
								m_current++;
								var = getExpression(flagSkip, PrecedenceUnary);
								EXPR_CHECK_ERROR
								if (!flagSkip) {
									var = -var;
								}
								EXPR_CHECK_ENDED
								break;
							}
						case '!':
							{
								m_current++;
								var = getExpression(flagSkip, PrecedenceUnary);
								EXPR_CHECK_ERROR
								if (!flagSkip) {
									var = !var;
								}
								EXPR_CHECK_ENDED
								break;
							}
						case '~':
							{
								m_current++;
								var = getExpression(flagSkip, PrecedenceUnary);
								EXPR_CHECK_ERROR
								if (!flagSkip) {
									var = ~var;
								}
								EXPR_CHECK_ENDED
								break;
							}
						case '(':
							{
								m_current++;
								var = getExpression(flagSkip);
								EXPR_CHECK_ERROR
								if (m_flagEnded || *m_current != ')') {
									setError("Missing character: ')'");
									return Variant();
								}
								m_current++;
								EXPR_SKIP_WHITESPACE_AND_CHECK_ENDED
								break;
							}
						case '"':
						case '\'':
							{
								sl_size lenParsed = 0;
								sl_bool flagError = sl_false;
								var = ParseUtil::parseBackslashEscapes(StringView(m_current, m_end - m_current), &lenParsed, &flagError);
								if (flagError) {
									setError("Invalid string literal");
									return Variant();
								}
								m_current += lenParsed;
								EXPR_SKIP_WHITESPACE_AND_CHECK_ENDED
								break;
							}
						default:
							{
								if (SLIB_CHAR_IS_DIGIT(ch)) {
									sl_bool flagFloat = sl_false;
									{
										Char* s = m_current + 1;
										while (s < m_end) {
											ch = *(s++);
											if (ch == '.') {
												flagFloat = sl_true;
												break;
											}
											if (ch == 'x' || ch == 'X') {
												break;
											}
											if (ch == 'e' || ch == 'E') {
												flagFloat = sl_true;
												break;
											}
											if (!SLIB_CHAR_IS_DIGIT(ch)) {
												break;
											}
										}
									}
									if (flagFloat) {
										double n;
										sl_reg result = String::parseDouble(&n, m_current, 0, m_end - m_current);
										if (result < 0) {
											setError("Invalid number");
											return Variant();
										}
										m_current += result;
										if (!flagSkip) {
											var = n;
										}
									} else {
										sl_uint64 n;
										sl_reg result = String::parseUint64(0, &n, m_current, 0, m_end - m_current);
										if (result < 0) {
											setError("Invalid integer");
											return Variant();
										}
										m_current += result;
										if (!flagSkip) {
											if (n >> 32) {
												var = n;
											} else {
												var = (sl_uint32)n;
											}
										}
									}
									EXPR_SKIP_WHITESPACE_AND_CHECK_ENDED
								} else if (SLIB_CHAR_IS_C_NAME(ch)) {
									StringView name = readVariableName();
									if (name.isEmpty()) {
										return Variant();
									}
									if (!flagSkip) {
										String strName = String::fromStatic(name.getData(), name.getLength());
										if (!(m_locals.get_NoLock(strName, &var))) {
											var = m_data.getItem(strName);
											if (var.isUndefined()) {
												var = getBuiltin(strName);
											}
										}
									}
									EXPR_SKIP_WHITESPACE_AND_CHECK_ENDED
								} else {
									setError("Invalid character");
									return Variant();
								}
								break;
							}
					}
					for (;;) {
						sl_bool flagBreak = sl_false;
						switch (*m_current) {
							case '.':
								{
									m_current++;
									StringView name = readVariableName();
									if (name.isEmpty()) {
										return Variant();
									}
									if (!flagSkip) {
										var = var.getItem(String::fromStatic(name.getData(), name.getLength()));
									}
									EXPR_SKIP_WHITESPACE_AND_CHECK_ENDED
									break;
								}
							case '[':
								{
									m_current++;
									Variant index = getExpression(flagSkip);
									EXPR_CHECK_ERROR
									if (m_flagEnded || *m_current != ']') {
										setError("Missing character: ']'");
										return Variant();
									}
									m_current++;
									if (!flagSkip) {
										if (index.isInteger()) {
											var = var.getElement(index.getUint64());
										} else if (index.isStringType()) {
											var = var.getItem(index.getString());
										} else {
											var.setUndefined();
										}
									}
									EXPR_SKIP_WHITESPACE_AND_CHECK_ENDED
									break;
								}
							case '(':
								{
									m_current++;
									VariantList params;
									Variant param;
									sl_bool flagFirstParam = sl_true;
									for (;;) {
										Variant arg = getExpression(flagSkip);
										EXPR_CHECK_ERROR
										if (m_flagEnded) {
											setError("Missing character: ')' or ','");
											return Variant();
										}
										if (flagFirstParam) {
											flagFirstParam = sl_false;
											param = Move(arg);
										} else {
											if (params.isNull()) {
												params.add_NoLock(Move(param));
											}
											params.add_NoLock(Move(arg));
										}
										ch = *m_current;
										if (ch == ',') {
											m_current++;
										} else if (ch == ')') {
											m_current++;
											break;
										} else {
											setError("Missing character: ')' or ','");
											return Variant();
										}
									}
									if (!flagSkip) {
										Function<Variant(Variant&)> func = var.getVariantFunction();
										if (params.isNotNull()) {
											Variant arg(params);
											var = func(arg);
										} else {
											var = func(param);
										}
									}
									EXPR_SKIP_WHITESPACE_AND_CHECK_ENDED
									break;
								}
							default:
								flagBreak = sl_true;
								break;
						}
						if (flagBreak) {
							break;
						}
					}
					if (maxPrecedence < PrecedenceMultiplyDivide) {
						return var;
					}
					for (;;) {
						switch (*m_current) {
							case '*':
								EXPR_PROCESS_BINARY_OP_NO_CHECK_ORDER(*, 1, PrecedenceMultiplyDivide)
							case '/':
								EXPR_PROCESS_BINARY_OP_NO_CHECK_ORDER(/, 1, PrecedenceMultiplyDivide)
							case '%':
								EXPR_PROCESS_BINARY_OP_NO_CHECK_ORDER(%, 1, PrecedenceMultiplyDivide)
							case '+':
								EXPR_PROCESS_BINARY_OP(+, 1, PrecedenceAddSub)
							case '-':
								EXPR_PROCESS_BINARY_OP(-, 1, PrecedenceAddSub)
							case '=':
								if (m_current + 1 < m_end) {
									if (m_current[1] == '=') {
										EXPR_PROCESS_BINARY_OP(==, 2 , PrecedenceEquals)
									}
								}
								setError("Invalid character");
								return Variant();
							case '!':
								if (m_current + 1 < m_end) {
									if (m_current[1] == '=') {
										EXPR_PROCESS_BINARY_OP(!= , 2, PrecedenceEquals)
									}
								}
								setError("Invalid character");
								return Variant();
							case '>':
								if (m_current + 1 < m_end) {
									ch = m_current[1];
									if (ch == '=') {
										EXPR_PROCESS_BINARY_OP(>= , 2, PrecedenceCompare)
									} else if (ch == '>') {
										EXPR_PROCESS_BINARY_OP(>> , 2, PrecedenceShift)
									}
								}
								EXPR_PROCESS_BINARY_OP(> , 1, PrecedenceCompare)
							case '<':
								if (m_current + 1 < m_end) {
									ch = m_current[1];
									if (ch == '=') {
										EXPR_PROCESS_BINARY_OP(<= , 2, PrecedenceCompare)
									} else if (ch == '<') {
										EXPR_PROCESS_BINARY_OP(<< , 2, PrecedenceShift)
									}
								}
								EXPR_PROCESS_BINARY_OP(<, 1, PrecedenceCompare)
							case '&':
								if (m_current + 1 < m_end) {
									if (m_current[1] == '&') {
										EXPR_PROCESS_BINARY_OP(&& , 2, PrecedenceLogicalAnd)
									}
								}
								EXPR_PROCESS_BINARY_OP(&, 1, PrecedenceBitwiseAnd)
							case '|':
								if (m_current + 1 < m_end) {
									if (m_current[1] == '|') {
										EXPR_PROCESS_BINARY_OP(||, 2, PrecedenceLogicalOr)
									}
								}
								EXPR_PROCESS_BINARY_OP(|, 1, PrecedenceBitwiseOr)
							case '^':
								EXPR_PROCESS_BINARY_OP(^ , 1, PrecedenceBitwiseXor)
							case '?':
								{
									if (maxPrecedence < PrecedenceTernaryConditional) {
										return var;
									}
									m_current ++;
									sl_bool flagTrue = sl_false;
									if (!flagSkip) {
										flagTrue = (sl_bool)var;
									}
									Variant first = getExpression(flagSkip || !flagTrue);
									EXPR_CHECK_ERROR
									if (m_flagEnded || *m_current != ':') {
										setError("Missing character: ':'");
										return Variant();
									}
									m_current++;
									Variant second = getExpression(flagSkip || flagTrue);
									EXPR_CHECK_ERROR
									if (!flagSkip) {
										if (flagTrue) {
											var = Move(first);
										} else {
											var = Move(second);
										}
									}
									EXPR_CHECK_ENDED
									break;
								}
							default:
								return var;
						}
					}
					return var;
				}

				void processBlock(sl_bool flagSkip)
				{
					while (m_current < m_end) {
						{
							Char* begin = m_current;
							while (m_current < m_end) {
								Char ch = *m_current;
								if (ch == '}' || ch == '$') {
									break;
								}
								m_current++;
							}
							writeStatic(flagSkip, begin, m_current - begin);
							if (m_current >= m_end) {
								break;
							}
						}
						if (m_current + 1 >= m_end) {
							writeStatic(flagSkip, m_current, 1);
							break;
						}
						Char ch = *(m_current++);
						if (ch == '}') {
							ch = *m_current;
							if (ch == '}') {
								// end of block
								m_current++;
								return;
							}
							writeStatic(flagSkip, "}", 1);
						} else if (ch == '$') {
							ch = *m_current;
							if (ch == '$') {
								// $$
								m_current++;
								writeStatic(flagSkip, "$", 1);
							} else if (ch == '#') {
								// $# comments
								m_current++;
								while (m_current < m_end) {
									Char ch = *(m_current++);
									if (ch == '\n') {
										break;
									} else if (ch == '\r') {
										if (m_current < m_end) {
											if (*m_current == '\n') {
												m_current++;
											}
										}
										break;
									}
								}
							} else if (ch == '{') {
								m_current++;
								if (m_current >= m_end) {
									break;
								}
								ch = *m_current;
								if (ch == '{') {
									// ${{
									m_current++;
									writeStatic(flagSkip, "{{", 2);
								} else {
									// ${expression}
									Variant var = getExpression(flagSkip);
									if (m_flagError) {
										return;
									}
									eatMatched("}");
									if (m_flagEnded) {
										return;
									}
									write(flagSkip, ((Variant&)var).toString());
								}
							} else if (ch == '}') {
								m_current++;
								if (m_current >= m_end) {
									break;
								}
								ch = *m_current;
								if (ch == '}') {
									// $}}
									m_current++;
									writeStatic(flagSkip, "}}", 2);
								} else {
									setError("Unexpected character");
									return;
								}
							} else {
								StringView command = readCommand();
								if (m_flagEnded) {
									return;
								}
								if (command == "for") {
									// $for x in xs {{ <block> }}
									skipWhitespace();
									String name = readLocalName();
									if (m_flagEnded) {
										return;
									}
									skipWhitespace();
									StringView strIn = readCommand();
									if (m_flagEnded) {
										setError("Missing 'in'");
										return;
									}
									if (strIn != "in") {
										setError("Unexpected string. It must be 'in'");
										return;
									}
									Variant var = getExpression(flagSkip);
									if (m_flagError) {
										return;
									}
									if (m_flagEnded) {
										setError("Missing block");
										return;
									}
									eatMatched("{{");
									if (m_flagEnded) {
										return;
									}
									if (flagSkip) {
										processBlock(sl_true);
										if (m_flagEnded) {
											return;
										}
									} else {

#define GINGER_PROCESS_FOR_IMPL(ITER, GET_VALUE) \
										Char* current = m_current; \
										ITER { \
											m_locals.put_NoLock(name, GET_VALUE); \
											m_current = current; \
											processBlock(sl_false); \
											if (m_flagEnded) { \
												return; \
											} \
											flagPassedBlock = sl_true; \
										} \
										m_locals.remove_NoLock(name);

										sl_bool flagPassedBlock = sl_false;
										VariantMap map = var.getVariantMap();
										if (map.isNotNull()) {
											if (map.isNotEmpty()) {
												GINGER_PROCESS_FOR_IMPL(for (auto& pair : map), pair.key)
											}
										} else {
											VariantList list = var.getVariantList();
											if (list.isNotNull()) {
												ListElements<Variant> items(list);
												if (items.count) {
													GINGER_PROCESS_FOR_IMPL(for (sl_size i = 0; i < items.count; i++), items[i])
												}
											} else {
												Ref<Collection> collection = var.getCollection();
												if (collection.isNotNull()) {
													sl_uint64 n = collection->getElementsCount();
													if (n) {
														GINGER_PROCESS_FOR_IMPL(for (sl_uint64 i = 0; i < n; i++), collection->getElement(i))
													}
												} else {
													PropertyIterator iterator = var.getItemIterator();
													if (iterator.isNotNull()) {
														GINGER_PROCESS_FOR_IMPL(while (iterator.moveNext()), iterator.getKey())
													} else if (var.is8BitsStringType()) {
														StringView str = var.getStringView();
														sl_size n = str.getLength();
														sl_char8* data = str.getData();
														GINGER_PROCESS_FOR_IMPL(for (sl_size i = 0; i < n; i++), data[i])
													} else if (var.is16BitsStringType()) {
														StringView16 str = var.getStringView16();
														sl_size n = str.getLength();
														sl_char16* data = str.getData();
														GINGER_PROCESS_FOR_IMPL(for (sl_size i = 0; i < n; i++), data[i])
													} else if (var.is32BitsStringType()) {
														StringView32 str = var.getStringView32();
														sl_size n = str.getLength();
														sl_char32* data = str.getData();
														GINGER_PROCESS_FOR_IMPL(for (sl_size i = 0; i < n; i++), data[i])
													}
												}
											}
										}
										if (!flagPassedBlock) {
											processBlock(sl_true);
											if (m_flagEnded) {
												return;
											}
										}
									}
								} else if (command == "if") {
									// $if x {{ <block> }}
									// $elseif y {{ <block> }}
									// $elseif z {{ <block> }}
									// $else {{ <block> }}
									Variant cond = getExpression(flagSkip);
									if (m_flagError) {
										return;
									}
									if (m_flagEnded) {
										setError("Missing block");
										return;
									}
									eatMatched("{{");
									if (m_flagEnded) {
										return;
									}
									sl_bool flagRun = sl_false;
									if (flagSkip) {
										processBlock(sl_true);
									} else {
										flagRun = (sl_bool)cond;
										processBlock(!flagRun);
									}
									if (m_flagEnded) {
										return;
									}
									for (;;) {
										Char* ptrSaved = m_current;
										skipWhitespace(sl_false);
										if (m_current < m_end) {
											if (*m_current == '$') {
												m_current++;
												command = readCommand(sl_false);
												if (m_current < m_end) {
													if (command == "elseif") {
														cond = getExpression(flagSkip || flagRun);
														if (m_flagError) {
															return;
														}
														if (m_flagEnded) {
															setError("Missing block");
															return;
														}
														eatMatched("{{");
														if (m_flagEnded) {
															return;
														}
														if (flagSkip || flagRun) {
															processBlock(sl_true);
														} else {
															flagRun = (sl_bool)cond;
															processBlock(!flagRun);
														}
														if (m_flagEnded) {
															return;
														}
														continue;
													} else if (command == "else") {
														skipWhitespace();
														if (m_flagEnded) {
															setError("Missing block");
															return;
														}
														eatMatched("{{");
														if (m_flagEnded) {
															return;
														}
														processBlock(flagSkip || flagRun);
														if (m_flagEnded) {
															return;
														}
														break;
													}
												}
											}
										}
										m_current = ptrSaved;
										break;
									}
								} else {
									setError("Unexpected command");
									return;
								}
							}
						} else {
							setError("Unexpected error");
							return;
						}
					}
					m_flagEnded = sl_true;
				}

				Variant getBuiltin(const StringView& name)
				{
					if (name == "this") {
						return m_data;
					} else if (name == "now") {
						return Time::now();
					}
					BuiltIn* builtin = GetBuiltIn();
					if (builtin) {
						return builtin->getBuiltIn(name);
					}
					return Variant();
				}

			};

			static String Render(const StringView& strTemplate, const Variant& data)
			{
				if (strTemplate.isEmpty()) {
					return sl_null;
				}
				Renderer renderer(strTemplate, data);
				renderer.run();
				return renderer.getOutput();
			}

		}
	}

	using namespace priv::ginger;
	
	String Ginger::render(const StringView& _template, const Variant& data)
	{
		return Render(_template, data);
	}

	String Ginger::renderFile(const StringParam& filePath, const Variant& data)
	{
		return render(File::readAllText(filePath).toString(), data);
	}

}
