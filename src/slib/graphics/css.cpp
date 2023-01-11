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
				CascadingStyleSheetHelper * sheet;
				CHAR* current;
				CHAR* end;

			public:
				static sl_bool run(CascadingStyleSheet* sheet, CHAR* data, sl_size len)
				{
					StylesParser parser;
					parser.sheet = (CascadingStyleSheetHelper*)sheet;
					parser.current = data;
					parser.end = data + len;
					return parser.run();
				}

				sl_bool run()
				{
					skipWhitespaces();
					return parseStatements(sheet->m_statements);
				}

				sl_bool parseStatements(CascadingStyleStatements& statements)
				{
					while (current < end) {
						CHAR ch = *current;
						if (ch == '}') {
							break;
						}
						if (ch == '@') {
							current++;
							if (!(parseAtRule(statements))) {
								return sl_false;
							}
						} else {
							if (!(parseRule(statements))) {
								return sl_false;
							}
						}
						skipWhitespaces();
					}
					return sl_true;
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
						if (!(parseStatements(at.statements))) {
							return sl_false;
						}
						if (current >= end) {
							return sl_false;
						}
						if (*current != '}') {
							return sl_false;
						}
						current++;
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
					HashMap< String, Ref<CascadingStyleValue> > props;
					for (;;) {
						skipWhitespaces();
						String name = parseIdentifier();
						if (name.isNull()) {
							break;
						}
						skipWhitespaces();
						if (current >= end) {
							return sl_false;
						}
						if (*current != ':') {
							break;
						}
						skipWhitespaces();
						Ref<CascadingStyleValue> value = parseValue();
						if (value.isNull()) {
							break;
						}
						if (current >= end) {
							return sl_false;
						}
						if (*current == '!') {
							current++;
							skipComment();
							String label = parseIdentifier();
							if (label == StringView::literal("important")) {
								value->setImportant();
							}
							skipWhitespaces();
							if (current >= end) {
								return sl_false;
							}
						}
						if (!(props.add_NoLock(Move(name), Move(value)))) {
							return sl_false;
						}
						if (*current == ';') {
							current++;
						} else {
							break;
						}
					}
					if (current >= end) {
						return sl_false;
					}
					if (*current != '}') {
						return sl_false;
					}
					current++;
					CascadingStyleRule rule;
					rule.selector = Move(selector);
					rule.properties = props;
					if (!(addRule(statements, Move(rule)))) {
						return sl_false;
					}
					{
						ListElements< Ref<CascadingStyleSelector> > items(group);
						for (sl_size i = 0; i < items.count; i++) {
							rule.selector = Move(items[i]);
							rule.properties = props;
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
					Ref<CascadingStyleSelector> next = parseCombindSelector();
					if (next.isNotNull()) {
						ret->combinator = combinator;
						ret->next = Move(next);
					} else {
						if (combinator != CascadingStyleCombinator::Descendant) {
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
										return sl_null;
									}
									String name = parseIdentifier();
									if (name.isNull()) {
										if (current == start) {
											return sl_null;
										}
										return ret;
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
								lenOutput += Charsets::getUtfn(code, _out);
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
					if (!(parseIdentifier(current, ret.getData(), len))) {
						return sl_null;
					}
					return String::from(ret);
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
								lenOutput += Charsets::getUtfn(code, _out);
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
						return parseIdentifier();
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
						if (ch == '(') {
							current++;
							if (!(skipValueRegion(')'))) {
								return sl_false;
							}
							current++;
						} else if (ch == '\\') {
							current++;
							if (current >= end) {
								return sl_false;
							}
							current++;
						} else if (ch == '"' || ch == '\'') {
							current++;
							sl_size n;
							if (!(parseStringValue(current, ch, sl_null, n))) {
								return sl_false;
							}
						} else if (ch == '/') {
							if (current + 1 < end && *(current + 1) == '*') {
								skipComment();
							} else {
								current++;
							}
						} else {
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
			return output.addStatic(" !important;");
		} else {
			return output.addStatic(";");
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
		if (!(output.add(m_name))) {
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
				if (!(output.add(namespaceName))) {
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
			if (!(output.add(elementName))) {
				return sl_false;
			}
		}
		if (id.isNotNull()) {

			if (!(output.add(id))) {
				return sl_false;
			}
		}
		return sl_true;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CascadingStyleStatements)

	CascadingStyleStatements::CascadingStyleStatements()
	{
	}


	CascadingStyleSheet::CascadingStyleSheet()
	{
	}

	CascadingStyleSheet::~CascadingStyleSheet()
	{
	}

	sl_bool CascadingStyleSheet::addStyles(const StringParam& _styles)
	{
		if (_styles.isEmpty()) {
			return sl_true;
		}
		if (_styles.is8BitsStringType()) {
			StringData styles(_styles);
			return StylesParser<sl_char8>::run(this, styles.getData(), styles.getLength());
		} else if (_styles.is16BitsStringType()) {
			StringData16 styles(_styles);
			return StylesParser<sl_char16>::run(this, styles.getData(), styles.getLength());
		} else {
			StringData32 styles(_styles);
			return StylesParser<sl_char32>::run(this, styles.getData(), styles.getLength());
		}
	}

}
