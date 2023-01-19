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

#include "slib/graphics/css.h"

#include "slib/core/string_buffer.h"

namespace slib
{

	namespace priv
	{
		namespace css
		{

			class CascadingStyleSheetHelper : public CascadingStyleSheet
			{
			public:
				using CascadingStyleSheet::m_statements;
			};

			template <class CHAR>
			class StylesParser
			{
			public:
				CascadingStyleSheetHelper* sheet;
				CHAR* current;
				CHAR* end;
				sl_bool flagIgnoreErrors;
				
			public:
				static void run(CascadingStyleSheet* sheet, CHAR* data, sl_size len, CascadingStylesParam& param)
				{
					StylesParser parser;
					parser.sheet = (CascadingStyleSheetHelper*)sheet;
					parser.current = data;
					parser.end = data + len;
					parser.flagIgnoreErrors = param.flagIgnoreErrors;
					param.flagError = !(parser.run());
				}

				sl_bool run()
				{
					skipWhitespaces();
					return parseStatements(sheet->m_statements, 0);
				}

				// Passes ending character
				sl_bool parseStatements(CascadingStyleStatements& statements, CHAR chEnd)
				{
					while (current < end) {
						CHAR ch = *current;
						if (chEnd) {
							if (ch == chEnd) {
								current++;
								break;
							}
						}
						if (ch == '@') {
							current++;
							if (!(parseAtRule(statements))) {
								if (flagIgnoreErrors) {
									skipLine();
								} else {
									return sl_false;
								}
							}
						} else {
							if (!(parseRule(statements))) {
								if (flagIgnoreErrors) {
									skipLine();
								} else {
									return sl_false;
								}
							}
						}
						skipWhitespaces();
					}
					return sl_true;
				}

				sl_bool parseAtRule(CascadingStyleStatements& statements)
				{
					CascadingStyleAtRule at;
					at.identifier = parseIdentifier();
					if (at.identifier.isNull()) {
						return sl_false;
					}
					at.rule = parseValueRegion(0);
					if (at.rule.isNull()) {
						return sl_false;
					}
					if (*current == '{') {
						current++;
						skipWhitespaces();
						CHAR* start = current;
						if (flagIgnoreErrors) {
							sl_bool flagDecl = sl_false;
							if (parseStatements(at.statements, '}')) {
								flagDecl = at.statements.rules.isNull() && at.statements.atRules.isNull();
							} else {
								flagDecl = sl_true;
							}
							if (flagDecl) {
								current = start;
								if (!(parseDeclarations(at.declarations))) {
									return sl_false;
								}
							}
						} else {
							if (!(parseStatements(at.statements, '}'))) {
								current = start;
								if (!(parseDeclarations(at.declarations))) {
									return sl_false;
								}
							}
						}
					} else if (*current == ';') {
						current++;
					} else {
						return sl_false;
					}
					return addAtRule(statements, Move(at));
				}

				sl_bool addAtRule(CascadingStyleStatements& statements, CascadingStyleAtRule&& rule)
				{
					return statements.atRules.add_NoLock(Move(rule));
				}

				sl_bool parseRule(CascadingStyleStatements& statements)
				{
					Ref<CascadingStyleSelector> selector = parseCombindSelector();
					if (selector.isNull()) {
						return sl_false;
					}
					if (current >= end) {
						return sl_false;
					}
					List< Ref<CascadingStyleSelector> > group;
					while (*current == ',') {
						current++;
						skipWhitespaces();
						Ref<CascadingStyleSelector> item = parseCombindSelector();
						if (item.isNull()) {
							return sl_false;
						}
						if (!(group.add_NoLock(Move(item)))) {
							return sl_false;
						}
						if (current >= end) {
							return sl_false;
						}
					}
					if (*current != '{') {
						return sl_false;
					}
					current++;
					skipWhitespaces();
					CascadingStyleDeclarations declarations;
					if (!(parseDeclarations(declarations))) {
						return sl_false;
					}
					CascadingStyleRule rule;
					rule.selector = Move(selector);
					rule.declarations = declarations;
					if (!(addRule(statements, Move(rule)))) {
						return sl_false;
					}
					{
						ListElements< Ref<CascadingStyleSelector> > items(group);
						for (sl_size i = 0; i < items.count; i++) {
							rule.selector = Move(items[i]);
							rule.declarations = declarations;
							if (!(addRule(statements, Move(rule)))) {
								return sl_false;
							}
						}
					}
					return sl_true;
				}

				sl_bool addRule(CascadingStyleStatements& statements, CascadingStyleRule&& rule)
				{
					return statements.rules.add_NoLock(Move(rule));
				}

				Ref<CascadingStyleValue> parseDeclaration(String& name)
				{
					name = parseIdentifier();
					if (name.isNull()) {
						return sl_null;
					}
					skipWhitespaces();
					if (current >= end) {
						return sl_null;
					}
					if (*current != ':') {
						return sl_null;
					}
					current++;
					skipWhitespaces();
					Ref<CascadingStyleValue> ret = parseValue();
					if (ret.isNull()) {
						return sl_null;
					}
					if (current < end) {
						if (*current == '!') {
							current++;
							skipComment();
							String label = parseIdentifier();
							if (label == StringView::literal("important")) {
								ret->setImportant();
							}
						}
					}
					return ret;
				}

				// Passes ending character(`}`)
				sl_bool parseDeclarations(CascadingStyleDeclarations& declarations)
				{
					while (current < end) {
						CHAR ch = *current;
						if (ch == ';') {
							current++;
						} else if (ch == '}') {
							current++;
							return sl_true;
						} else {
							String name;
							Ref<CascadingStyleValue> value = parseDeclaration(name);
							if (value.isNotNull()) {
								if (!(declarations.add_NoLock(Move(name), Move(value)))) {
									declarations.setNull();
									return sl_false;
								}
							} else {
								if (flagIgnoreErrors) {
									skipLine();
								} else {
									return sl_false;
								}
							}
						}
						skipWhitespaces();
					}
					declarations.setNull();
					return sl_false;
				}

				Ref<CascadingStyleValue> parseValue()
				{
					CHAR* start = current;
					Ref<CascadingStyleValue> value = parseVariableValue();
					if (value.isNotNull()) {
						return value;
					}
					current = start;
					return parseNormalValue();
				}

				Ref<CascadingStyleValue> parseVariableValue()
				{
					if (current + 3 > end) {
						return sl_null;
					}
					if (!(*current == 'v' && *(current + 1) == 'a' && *(current + 2) == 'r')) {
						return sl_null;
					}
					current += 3;
					skipWhitespaces();
					if (current >= end) {
						return sl_null;
					}
					if (*current != '(') {
						return sl_null;
					}
					current++;
					skipWhitespaces();
					String name = parseIdentifier();
					if (name.isNull()) {
						return sl_null;
					}
					skipWhitespaces();
					if (current >= end) {
						return sl_null;
					}
					String defaultValue;
					CHAR ch = *current;
					if (ch == ',') {
						current++;
						skipWhitespaces();
						String defaultValue = parseValueRegion(')');
						if (defaultValue.isNull()) {
							return sl_null;
						}
						current++;
						return new CascadingStyleVariableValue(Move(name), Move(defaultValue));
					} else if (ch == ')') {
						current++;
						return new CascadingStyleVariableValue(Move(name));
					} else {
						return sl_null;
					}
				}

				Ref<CascadingStyleValue> parseNormalValue()
				{
					String value = parseValueRegion(0);
					if (value.isNull()) {
						return sl_null;
					}
					return new CascadingStyleNormalValue(Move(value));
				}

				Ref<CascadingStyleSelector> parseCombindSelector()
				{
					Ref<CascadingStyleSelector> ret = parseBasicSelector();
					if (ret.isNull()) {
						return sl_null;
					}
					skipWhitespaces();
					if (current >= end) {
						return ret;
					}
					CascadingStyleCombinator combinator = CascadingStyleCombinator::Descendant;
					CHAR ch = *current;
					if (ch == '>') {
						combinator = CascadingStyleCombinator::Child;
					} else if (ch == '~') {
						combinator = CascadingStyleCombinator::Sibling;
					} else if (ch == '+') {
						combinator = CascadingStyleCombinator::Adjacent;
					}
					if (combinator != CascadingStyleCombinator::Descendant) {
						current++;
						skipWhitespaces();
						if (current >= end) {
							return sl_null;
						}
					}
					CHAR* start = current;
					Ref<CascadingStyleSelector> next = parseCombindSelector();
					if (next.isNotNull()) {
						ret->combinator = combinator;
						ret->next = Move(next);
					} else {
						if (combinator != CascadingStyleCombinator::Descendant) {
							return sl_null;
						}
						if (start != current) {
							return sl_null;
						}
					}
					return ret;
				}

				Ref<CascadingStyleSelector> parseBasicSelector()
				{
					Ref<CascadingStyleSelector> ret = new CascadingStyleSelector;
					if (ret.isNull()) {
						return sl_null;
					}
					CHAR* start = current;
					for (;;) {
						switch (*current) {
							case '.':
								{
									current++;
									String name = parseIdentifier();
									if (name.isNull()) {
										return sl_null;
									}
									if (!(ret->classNames.add_NoLock(Move(name)))) {
										return sl_null;
									}
									break;
								}
							case '#':
								{
									current++;
									String name = parseIdentifier();
									if (name.isNull()) {
										return sl_null;
									}
									ret->id = Move(name);
									break;
								}
							case '[':
								{
									current++;
									if (!(parseSelector_AttributeMatch(ret.get()))) {
										return sl_null;
									}
									break;
								}
							case '*':
								{
									if (ret->elementName.isNotNull() || ret->flagUniversal) {
										return sl_null;
									}
									current++;
									ret->flagUniversal = sl_true;
									break;
								}
							case '|':
								{
									if (ret->flagNamespace) {
										return sl_null;
									}
									current++;
									if (ret->flagUniversal) {
										ret->flagUniversal = sl_false;
									} else {
										ret->namespaceName = Move(ret->elementName);
										if (ret->namespaceName.isNull()) {
											ret->namespaceName.setEmpty();
										}
									}
									ret->flagNamespace = sl_true;
									if (current >= end) {
										return sl_null;
									}
									if (*current == '*') {
										current++;
										ret->flagUniversal = sl_true;
									} else {
										String name = parseIdentifier();
										if (name.isNull()) {
											return sl_null;
										}
										ret->elementName = Move(name);
									}
									break;
								}
							case ':':
								{
									current++;
									if (current >= end) {
										return sl_null;
									}
									if (*current == ':') {
										current++;
										if (ret->pseudoElement.isNotNull()) {
											return sl_null;
										}
										String name = parsePseudoClass();
										if (name.isNull()) {
											return sl_null;
										}
										ret->pseudoElement = Move(name);
									} else {
										String name = parsePseudoClass();
										if (name.isNull()) {
											return sl_null;
										}
										if (!(ret->pseudoClasses.add_NoLock(Move(name)))) {
											return sl_null;
										}
									}
									break;
								}
							default:
								{
									if (ret->elementName.isNotNull() || ret->flagUniversal) {
										return ret;
									}
									String name = parseIdentifier();
									if (name.isNull()) {
										name = parseUnquotedStringValue();
										if (name.isNull()) {
											if (current == start) {
												return sl_null;
											}
											return ret;
										}
									}
									ret->elementName = Move(name);
									break;
								}
						}
						if (current >= end) {
							break;
						}
						skipComment();
					}
					return ret;
				}

				sl_bool parseSelector_AttributeMatch(CascadingStyleSelector* selector)
				{
					skipWhitespaces();
					if (current >= end) {
						return sl_false;
					}
					CascadingStyleAttributeMatch match;
					match.name = parseIdentifier();
					if (match.name.isNull()) {
						return sl_false;
					}
					skipWhitespaces();
					if (current >= end) {
						return sl_false;
					}
					match.type = CascadingStyleMatchType::Exist;
					switch (*current) {
						case '~':
							match.type = CascadingStyleMatchType::Contains_Word;
							current++;
							break;
						case '|':
							match.type = CascadingStyleMatchType::Contains_WordHyphen;
							current++;
							break;
						case '^':
							match.type = CascadingStyleMatchType::Start;
							current++;
							break;
						case '$':
							match.type = CascadingStyleMatchType::End;
							current++;
							break;
						case '*':
							match.type = CascadingStyleMatchType::Contain;
							current++;
							break;
						case '=':
							match.type = CascadingStyleMatchType::Equal;
							current++;
							break;
					}
					if (match.type > CascadingStyleMatchType::Equal) {
						skipWhitespaces();
						if (current >= end) {
							return sl_false;
						}
						if (*current != '=') {
							return sl_false;
						}
						current++;
					}
					if (match.type != CascadingStyleMatchType::Exist) {
						skipWhitespaces();
						if (current >= end) {
							return sl_false;
						}
					}
					if (match.type != CascadingStyleMatchType::Exist) {
						match.value = parseStringValue();
						if (match.value.isNull()) {
							return sl_false;
						}
						skipWhitespaces();
						if (current >= end) {
							return sl_false;
						}
						if (*current == 'i' || *current == 'I') {
							match.flagIgnoreCase = sl_true;
							current++;
							skipWhitespaces();
							if (current >= end) {
								return sl_false;
							}
						}
					}
					if (*current != ']') {
						return sl_false;
					}
					current++;
					return selector->attributes.add_NoLock(Move(match));
				}

				void skipCommentInner()
				{
					current += 2;
					for (;;) {
						if (current + 2 > end) {
							current = end;
							return;
						}
						if (*current == '*' && *(current + 1) == '/') {
							current += 2;
							break;
						} else {
							current++;
						}
					}
				}

				void skipComment()
				{
					if (current + 2 > end) {
						return;
					}
					if (*current == '/' && *(current + 1) == '*') {
						skipCommentInner();
					}
				}

				void skipWhitespaces()
				{
					while (current < end) {
						CHAR ch = *current;
						if (SLIB_CHAR_IS_WHITE_SPACE(ch)) {
							current++;
						} else if (ch == '/') {
							if (current + 1 < end && *(current + 1) == '*') {
								skipCommentInner();
							} else {
								return;
							}
						} else {
							return;
						}
					}
				}

				void skipLine()
				{
					while (current < end) {
						CHAR ch = *(current++);
						if (ch == '\r') {
							if (current < end) {
								ch = current[1];
								if (ch == '\n') {
									current++;
								}
							}
							return;
						} else if (ch == '\n') {
							return;
						}
					}
				}

				sl_bool parseHexValue(CHAR*& input, sl_uint32& value)
				{
					sl_uint32 h = SLIB_CHAR_HEX_TO_INT(*input);
					if (h < 16) {
						value = h;
						input++;
						sl_uint32 n = 1;
						while (input < end && n < 6) {
							h = SLIB_CHAR_HEX_TO_INT(*input);
							if (h < 16) {
								value = (value << 4) | h;
								input++;
								n++;
							} else {
								break;
							}
						}
						if (input < end) {
							CHAR ch = *input;
							if (ch == ' ' || ch == '\t' || ch == '\n') {
								input++;
							} else if (ch == '\r') {
								input++;
								if (input < end) {
									ch = *input;
									if (ch == '\n') {
										input++;
									}
								}
							}
						}
						return sl_true;
					} else {
						return sl_false;
					}
				}

				sl_bool parseIdentifier(CHAR*& input, CHAR* _out, sl_size& lenOutput)
				{
					lenOutput = 0;
					CHAR* start = input;
					while (input < end) {
						CHAR ch = *input;
						if (SLIB_CHAR_IS_ALNUM(ch) || ch == '-' || ch == '_') {
							if (start == input) {
								if (SLIB_CHAR_IS_DIGIT(ch)) {
									return sl_false;
								}
								if (ch == '-' && input + 1 < end) {
									CHAR next = *(input + 1);
									if (SLIB_CHAR_IS_DIGIT(next)) {
										return sl_false;
									}
								}
							}
							input++;
						} else if (ch == '\\') {
							input++;
							if (input >= end) {
								return sl_false;
							}
							sl_uint32 code;
							if (parseHexValue(input, code)) {
								lenOutput += Charsets::getUtfn(code, _out ? _out + lenOutput : sl_null);
								continue;
							} else {
								ch = *(input++);
							}
						} else {
							break;
						}
						if (_out) {
							_out[lenOutput] = ch;
						}
						lenOutput++;
					}
					return lenOutput > 0;
				}

				String parseIdentifier()
				{
					if (current >= end) {
						return sl_null;
					}
					CHAR* s = current;
					sl_size len;
					if (!(parseIdentifier(s, sl_null, len))) {
						return sl_null;
					}
					typename StringTypeFromCharType<CHAR>::Type ret = StringTypeFromCharType<CHAR>::Type::allocate(len);
					if (ret.isNull()) {
						return sl_null;
					}
					parseIdentifier(current, ret.getData(), len);
					return String::from(ret);
				}

				sl_bool parseUnquotedStringValue(CHAR*& input, CHAR* _out, sl_size& lenOutput)
				{
					lenOutput = 0;
					CHAR* start = input;
					while (input < end) {
						CHAR ch = *input;
						if (SLIB_CHAR_IS_ALNUM(ch) || (ch & 0x80) || ch == '-' || ch == '_' || ch == '.' || ch == '%' || ch == '@') {
							input++;
						} else if (ch == '\\') {
							if (input >= end) {
								return sl_false;
							}
							sl_uint32 code;
							if (parseHexValue(input, code)) {
								lenOutput += Charsets::getUtfn(code, _out ? _out + lenOutput : sl_null);
								continue;
							} else {
								ch = *(input++);
							}
						} else {
							break;
						}
						if (_out) {
							_out[lenOutput] = ch;
						}
						lenOutput++;
					}
					return lenOutput > 0;
				}

				String parseUnquotedStringValue()
				{
					if (current >= end) {
						return sl_null;
					}
					CHAR* s = current;
					sl_size len;
					if (!(parseUnquotedStringValue(s, sl_null, len))) {
						return sl_null;
					}
					typename StringTypeFromCharType<CHAR>::Type ret = StringTypeFromCharType<CHAR>::Type::allocate(len);
					if (ret.isNull()) {
						return sl_null;
					}
					parseUnquotedStringValue(current, ret.getData(), len);
					return String::from(ret);
				}

				sl_bool parseStringValue(CHAR*& input, CHAR chOpen, CHAR* _out, sl_size& lenOutput)
				{
					lenOutput = 0;
					while (input < end) {
						CHAR ch = *(input++);
						if (ch == chOpen) {
							return sl_true;
						} else if (ch == '\\') {
							if (input >= end) {
								return sl_false;
							}
							sl_uint32 code;
							if (parseHexValue(input, code)) {
								lenOutput += Charsets::getUtfn(code, _out ? _out + lenOutput : sl_null);
								continue;
							} else {
								ch = *(input++);
							}
						}
						if (_out) {
							_out[lenOutput] = ch;
						}
						lenOutput++;
					}
					return sl_false;
				}

				String parseStringValue()
				{
					CHAR chOpen = *current;
					if (chOpen != '"' && chOpen != '\'') {
						return parseUnquotedStringValue();
					}
					current++;
					CHAR* s = current;
					sl_size len;
					if (!(parseStringValue(s, chOpen, sl_null, len))) {
						return sl_null;
					}
					typename StringTypeFromCharType<CHAR>::Type ret = StringTypeFromCharType<CHAR>::Type::allocate(len);
					if (ret.isNull()) {
						return sl_null;
					}
					if (!(parseStringValue(current, chOpen, ret.getData(), len))) {
						return sl_null;
					}
					return String::from(ret);
				}

				sl_bool skipValueRegion(CHAR chEnd)
				{
					while (current < end) {
						CHAR ch = *current;
						switch (ch) {
							case '(':
								current++;
								if (!(skipValueRegion(')'))) {
									return sl_false;
								}
								current++;
								break;
							case '[':
								current++;
								if (!(skipValueRegion(']'))) {
									return sl_false;
								}
								current++;
								break;
							case '\\':
								current++;
								if (current >= end) {
									return sl_false;
								}
								current++;
								break;
							case '"':
							case '\'':
								current++;
								sl_size n;
								if (!(parseStringValue(current, ch, sl_null, n))) {
									return sl_false;
								}
								break;
							case '/':
								if (current + 1 < end && *(current + 1) == '*') {
									skipComment();
								} else {
									current++;
								}
								break;
							default:
								if (chEnd) {
									if (ch == chEnd) {
										return sl_true;
									} else {
										current++;
									}
								} else {
									if (ch == ';' || ch == '{' || ch == '}' || ch == '!') {
										return sl_true;
									} else {
										current++;
									}
								}
								break;
						}
					}
					return sl_false;
				}

				String parseValueRegion(CHAR chEnd)
				{
					skipWhitespaces();
					CHAR* start = current;
					if (!(skipValueRegion(chEnd))) {
						return sl_null;
					}
					if (current == start) {
						return String::getEmpty();
					}
					CHAR* last = current - 1;
					while (last > start) {
						if (SLIB_CHAR_IS_WHITE_SPACE(*last)) {
							last--;
						} else {
							break;
						}
					}
					return String::from(start, last - start + 1);
				}

				String parsePseudoClass()
				{
					CHAR* start = current;
					sl_size n;
					if (!(parseIdentifier(current, sl_null, n))) {
						return sl_null;
					}
					if (current < end) {
						if (*current == '(') {
							current++;
							if (!(skipValueRegion(')'))) {
								return sl_null;
							}
							current++;
						}
					}
					return String::from(start, current - start);
				}

			};

			SLIB_INLINE static void WriteChar(char* output, sl_size& lenOutput, char ch)
			{
				if (output) {
					output[lenOutput] = ch;
				}
				lenOutput++;
			}

			// Returns output length
			static sl_size MakeIdentifier(char* input, sl_size lenInput, char* output, sl_bool& outFlagDiffOriginal)
			{
				sl_size lenOutput = 0;
				sl_size posInput = 0;
				sl_bool flagDiffOriginal = sl_false;
				const char* hex = "0123456789abcdef";
				while (posInput < lenInput) {
					char ch = input[posInput];
					if (SLIB_CHAR_IS_ALNUM(ch) || ch == '-' || ch == '_') {
						if (!posInput) {
							if (SLIB_CHAR_IS_DIGIT(ch)) {
								if (output) {
									output[lenOutput++] = '\\';
									output[lenOutput++] = '0';
									output[lenOutput++] = '0';
									output[lenOutput++] = '0';
									output[lenOutput++] = '0';
									output[lenOutput++] = hex[(ch >> 4) & 15];
								} else {
									lenOutput += 6;
								}
								ch = hex[ch & 15];
								flagDiffOriginal = sl_true;
							} else if (ch == '-' && lenInput >= 2) {
								char next = input[1];
								if (SLIB_CHAR_IS_DIGIT(next)) {
									WriteChar(output, lenOutput, '\\');
									flagDiffOriginal = sl_true;
								}
							}
						}
						WriteChar(output, lenOutput, ch);
						posInput++;
					} else if (ch == '\\') {
						flagDiffOriginal = sl_true;
						if (output) {
							output[lenOutput++] = '\\';
							output[lenOutput++] = '\\';
						} else {
							lenOutput += 2;
						}
						posInput++;
					} else {
						flagDiffOriginal = sl_true;
						sl_char32 code;
						if (!(Charsets::getUnicode(code, input, lenInput, posInput))) {
							code = ch;
							posInput++;
						}
						if (output) {
							output[lenOutput++] = '\\';
							output[lenOutput++] = hex[(code >> 20) & 15];
							output[lenOutput++] = hex[(code >> 16) & 15];
							output[lenOutput++] = hex[(code >> 12) & 15];
							output[lenOutput++] = hex[(code >> 8) & 15];
							output[lenOutput++] = hex[(code >> 4) & 15];
							output[lenOutput++] = hex[code & 15];
						} else {
							lenOutput += 7;
						}
					}
				}
				outFlagDiffOriginal = flagDiffOriginal;
				return lenOutput;
			}

			static String MakeIdentifier(const String& value)
			{
				sl_bool flagDiff;
				sl_size n = MakeIdentifier(value.getData(), value.getLength(), sl_null, flagDiff);
				if (!flagDiff) {
					return value;
				}
				String ret = String::allocate(n);
				if (ret.isNotNull()) {
					MakeIdentifier(value.getData(), value.getLength(), ret.getData(), flagDiff);
				}
				return ret;
			}

			static sl_bool WriteIdentifier(StringBuffer& buf, const String& value)
			{
				String s = MakeIdentifier(value);
				if (s.isNull()) {
					return sl_false;
				}
				return buf.add(Move(s));
			}

			// Returns output length
			static sl_size MakeStringValue(char* input, sl_size lenInput, char* output, sl_bool& outFlagDiffOriginal)
			{
				sl_size lenOutput = 0;
				sl_size posInput = 0;
				sl_bool flagDiffOriginal = sl_false;
				while (posInput < lenInput) {
					char ch = input[posInput];
					if (SLIB_CHAR_IS_PRINTABLE_ASCII(ch)) {
						if (ch == '\\' || ch == '"') {
							WriteChar(output, lenOutput, '\\');
							flagDiffOriginal = sl_true;
						}
						WriteChar(output, lenOutput, ch);
						posInput++;
					} else if (ch == '\t') {
						WriteChar(output, lenOutput, ch);
						posInput++;
					} else {
						flagDiffOriginal = sl_true;
						sl_char32 code;
						if (!(Charsets::getUnicode(code, input, lenInput, posInput))) {
							code = ch;
							posInput++;
						}
						if (output) {
							output[lenOutput++] = '\\';
							const char* hex = "0123456789abcdef";
							output[lenOutput++] = hex[(code >> 20) & 15];
							output[lenOutput++] = hex[(code >> 16) & 15];
							output[lenOutput++] = hex[(code >> 12) & 15];
							output[lenOutput++] = hex[(code >> 8) & 15];
							output[lenOutput++] = hex[(code >> 4) & 15];
							output[lenOutput++] = hex[code & 15];
						} else {
							lenOutput += 7;
						}
					}
				}
				outFlagDiffOriginal = flagDiffOriginal;
				return lenOutput;
			}

			static String MakeStringValue(const String& value)
			{
				sl_bool flagDiff;
				sl_size n = MakeStringValue(value.getData(), value.getLength(), sl_null, flagDiff);
				if (!flagDiff) {
					return value;
				}
				String ret = String::allocate(n);
				if (ret.isNotNull()) {
					MakeStringValue(value.getData(), value.getLength(), ret.getData(), flagDiff);
				}
				return ret;
			}

			static sl_bool WriteStringValue(StringBuffer& buf, const String& value)
			{
				String s = MakeStringValue(value);
				if (s.isNull()) {
					return sl_false;
				}
				if (!(buf.addStatic("\""))) {
					return sl_false;
				}
				if (!(buf.add(Move(s)))) {
					return sl_false;
				}
				return buf.addStatic("\"");
			}

			static sl_bool WriteTabs(StringBuffer& buf, sl_uint32 nTabs)
			{
				for (sl_uint32 i = 0; i < nTabs; i++) {
					if (!(buf.addStatic("\t"))) {
						return sl_false;
					}
				}
				return sl_true;
			}

		}
	}

	using namespace priv::css;

	CascadingStyleValue::CascadingStyleValue(CascadingStyleValueType type): m_type(type), m_flagImportant(sl_false)
	{
	}

	CascadingStyleValue::~CascadingStyleValue()
	{
	}

	sl_bool CascadingStyleValue::toString_Suffix(StringBuffer& output)
	{
		if (m_flagImportant) {
			return output.addStatic(" !important");
		} else {
			return sl_true;
		}
	}


	CascadingStyleNormalValue::CascadingStyleNormalValue(String&& value): CascadingStyleValue(CascadingStyleValueType::Normal), m_value(Move(value))
	{
	}

	CascadingStyleNormalValue::~CascadingStyleNormalValue()
	{
	}

	sl_bool CascadingStyleNormalValue::toString(StringBuffer& output)
	{
		if (!(output.add(m_value))) {
			return sl_false;
		}
		return toString_Suffix(output);
	}


	CascadingStyleVariableValue::CascadingStyleVariableValue(String&& name, String&& defaultValue): CascadingStyleValue(CascadingStyleValueType::Variable), m_name(Move(name)), m_defaultValue(Move(defaultValue))
	{
	}

	CascadingStyleVariableValue::CascadingStyleVariableValue(String&& name): CascadingStyleValue(CascadingStyleValueType::Variable), m_name(Move(name))
	{
	}

	CascadingStyleVariableValue::~CascadingStyleVariableValue()
	{
	}

	sl_bool CascadingStyleVariableValue::toString(StringBuffer& output)
	{
		if (!(output.addStatic("var("))) {
			return sl_false;
		}
		if (!(WriteIdentifier(output, m_name))) {
			return sl_false;
		}
		if (m_defaultValue.isNotNull()) {
			if (!(output.addStatic(", "))) {
				return sl_false;
			}
			if (!(output.add(m_defaultValue))) {
				return sl_false;
			}
		}
		if (!(output.addStatic(")"))) {
			return sl_false;
		}
		return toString_Suffix(output);
	}


	CascadingStyleSelector::CascadingStyleSelector(): flagNamespace(sl_false), flagUniversal(sl_false)
	{
	}

	CascadingStyleSelector::~CascadingStyleSelector()
	{
	}

	sl_bool CascadingStyleSelector::toString(StringBuffer& output)
	{
		if (flagNamespace) {
			if (namespaceName.isNotNull()) {
				if (!(WriteIdentifier(output, namespaceName))) {
					return sl_false;
				}
				if (!(output.addStatic("|"))) {
					return sl_false;
				}
			} else {
				if (!(output.addStatic("*|"))) {
					return sl_false;
				}
			}
		}
		if (flagUniversal) {
			if (!(output.addStatic("*"))) {
				return sl_false;
			}
		} else if (elementName.isNotNull()) {
			if (!(WriteIdentifier(output, elementName))) {
				return sl_false;
			}
		}
		if (id.isNotNull()) {
			if (!(output.addStatic("#"))) {
				return sl_false;
			}
			if (!(WriteIdentifier(output, id))) {
				return sl_false;
			}
		}
		{
			ListElements<String> items(classNames);
			for (sl_size i = 0; i < items.count; i++) {
				String& name = items[i];
				if (!(output.addStatic("."))) {
					return sl_false;
				}
				if (!(WriteIdentifier(output, name))) {
					return sl_false;
				}
			}
		}
		{
			ListElements<CascadingStyleAttributeMatch> items(attributes);
			for (sl_size i = 0; i < items.count; i++) {
				CascadingStyleAttributeMatch& match = items[i];
				if (!(output.addStatic("["))) {
					return sl_false;
				}
				if (!(WriteIdentifier(output, match.name))) {
					return sl_false;
				}
				switch (match.type) {
					case CascadingStyleMatchType::Equal:
						if (!(output.addStatic("="))) {
							return sl_false;
						}
						break;
					case CascadingStyleMatchType::Contains_Word:
						if (!(output.addStatic("~="))) {
							return sl_false;
						}
						break;
					case CascadingStyleMatchType::Contains_WordHyphen:
						if (!(output.addStatic("|="))) {
							return sl_false;
						}
						break;
					case CascadingStyleMatchType::Start:
						if (!(output.addStatic("^="))) {
							return sl_false;
						}
						break;
					case CascadingStyleMatchType::End:
						if (!(output.addStatic("$="))) {
							return sl_false;
						}
						break;
					case CascadingStyleMatchType::Contain:
						if (!(output.addStatic("*="))) {
							return sl_false;
						}
						break;
					case CascadingStyleMatchType::Exist:
						break;
					default:
						return sl_false;
				}
				if (match.type != CascadingStyleMatchType::Exist) {
					if (!(WriteStringValue(output, match.value))) {
						return sl_false;
					}
				}
				if (!(output.addStatic("]"))) {
					return sl_false;
				}
			}
		}
		{
			ListElements<String> items(pseudoClasses);
			for (sl_size i = 0; i < items.count; i++) {
				String& name = items[i];
				if (!(output.addStatic(":"))) {
					return sl_false;
				}
				if (!(output.add(name))) {
					return sl_false;
				}
			}
		}
		if (pseudoElement.isNotNull()) {
			if (!(output.addStatic("::"))) {
				return sl_false;
			}
			if (!(output.add(pseudoElement))) {
				return sl_false;
			}
		}
		if (next.isNull()) {
			return sl_true;
		}
		switch (combinator) {
			case CascadingStyleCombinator::None:
				return sl_true;
			case CascadingStyleCombinator::Descendant:
				if (!(output.addStatic(" "))) {
					return sl_false;
				}
				break;
			case CascadingStyleCombinator::Child:
				if (!(output.addStatic(">"))) {
					return sl_false;
				}
				break;
			case CascadingStyleCombinator::Sibling:
				if (!(output.addStatic("~"))) {
					return sl_false;
				}
				break;
			case CascadingStyleCombinator::Adjacent:
				if (!(output.addStatic("+"))) {
					return sl_false;
				}
				break;
			default:
				return sl_false;
		}
		return next->toString(output);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CascadingStyleStatements)

	CascadingStyleStatements::CascadingStyleStatements()
	{
	}

	sl_bool CascadingStyleStatements::toString(StringBuffer& output, sl_uint32 tabLevel)
	{
		sl_bool flagFirst = sl_true;
		{
			ListElements<CascadingStyleAtRule> items(atRules);
			for (sl_size i = 0; i < items.count; i++) {
				CascadingStyleAtRule& item = items[i];
				if (flagFirst) {
					flagFirst = sl_false;
				} else {
					if (!(output.addStatic("\r\n"))) {
						return sl_false;
					}
				}
				if (!(item.toString(output, tabLevel))) {
					return sl_false;
				}
			}
		}
		{
			ListElements<CascadingStyleRule> items(rules);
			for (sl_size i = 0; i < items.count; i++) {
				CascadingStyleRule& item = items[i];
				if (flagFirst) {
					flagFirst = sl_false;
				} else {
					if (!(output.addStatic("\r\n"))) {
						return sl_false;
					}
				}
				if (!(item.toString(output, tabLevel))) {
					return sl_false;
				}
			}
		}
		return sl_true;
	}


	sl_bool CascadingStyleDeclarations::toString(StringBuffer& output, sl_uint32 tabLevel)
	{
		auto node = getFirstNode();
		while (node) {
			if (!(WriteTabs(output, tabLevel + 1))) {
				return sl_false;
			}
			if (!(WriteIdentifier(output, node->key))) {
				return sl_false;
			}
			if (!(output.addStatic(": "))) {
				return sl_false;
			}
			if (!(node->value->toString(output))) {
				return sl_false;
			}
			if (!(output.addStatic(";\r\n"))) {
				return sl_false;
			}
			node = node->next;
		}
		return sl_true;
	}


	sl_bool CascadingStyleRule::toString(StringBuffer& output, sl_uint32 tabLevel)
	{
		if (!(WriteTabs(output, tabLevel))) {
			return sl_false;
		}
		if (!(selector->toString(output))) {
			return sl_false;
		}
		if (!(output.addStatic(" {\r\n"))) {
			return sl_false;
		}
		if (!(declarations.toString(output, tabLevel))) {
			return sl_false;
		}
		if (!(WriteTabs(output, tabLevel))) {
			return sl_false;
		}
		return output.addStatic("}");
	}


	sl_bool CascadingStyleAtRule::toString(StringBuffer& output, sl_uint32 tabLevel)
	{
		if (!(WriteTabs(output, tabLevel))) {
			return sl_false;
		}
		if (!(output.addStatic("@"))) {
			return sl_false;
		}
		if (!(WriteIdentifier(output, identifier))) {
			return sl_false;
		}
		if (rule.isNotEmpty()) {
			if (!(output.addStatic(" "))) {
				return sl_false;
			}
			if (!(output.add(rule))) {
				return sl_false;
			}
		}
		if (declarations.isNotNull()) {
			if (!(output.addStatic(" {\r\n"))) {
				return sl_false;
			}
			if (!(declarations.toString(output, tabLevel))) {
				return sl_false;
			}
			if (!(WriteTabs(output, tabLevel))) {
				return sl_false;
			}
			if (!(output.addStatic("}"))) {
				return sl_false;
			}
		} else if (statements.atRules.isNotNull() || statements.rules.isNotNull()) {
			if (!(output.addStatic(" {\r\n"))) {
				return sl_false;
			}
			if (!(statements.toString(output, tabLevel + 1))) {
				return sl_false;
			}
			if (!(output.addStatic("\r\n"))) {
				return sl_false;
			}
			if (!(WriteTabs(output, tabLevel))) {
				return sl_false;
			}
			if (!(output.addStatic("}"))) {
				return sl_false;
			}
		} else {
			return output.addStatic(";");
		}
		return sl_true;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CascadingStylesParam)

	CascadingStylesParam::CascadingStylesParam(): flagIgnoreErrors(sl_true), flagError(sl_false)
	{
	}


	CascadingStyleSheet::CascadingStyleSheet()
	{
	}

	CascadingStyleSheet::~CascadingStyleSheet()
	{
	}

	void CascadingStyleSheet::addStyles(const StringParam& _styles, CascadingStylesParam& param)
	{
		if (_styles.isEmpty()) {
			return;
		}
		if (_styles.is8BitsStringType()) {
			StringData styles(_styles);
			StylesParser<sl_char8>::run(this, styles.getData(), styles.getLength(), param);
		} else if (_styles.is16BitsStringType()) {
			StringData16 styles(_styles);
			StylesParser<sl_char16>::run(this, styles.getData(), styles.getLength(), param);
		} else {
			StringData32 styles(_styles);
			StylesParser<sl_char32>::run(this, styles.getData(), styles.getLength(), param);
		}
	}

	sl_bool CascadingStyleSheet::addStyles(const StringParam& styles)
	{
		CascadingStylesParam param;
		addStyles(styles, param);
		return !(param.flagError);
	}

	sl_bool CascadingStyleSheet::toString(StringBuffer& output)
	{
		return m_statements.toString(output, 0);
	}

	String CascadingStyleSheet::toString()
	{
		StringBuffer buf;
		if (toString(buf)) {
			return buf.merge();
		} else {
			return sl_null;
		}
	}

}
