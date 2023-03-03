/*
 *   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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
#include "slib/core/stringx.h"

namespace slib
{

	namespace {

		class CascadingStyleSheetHelper : public CascadingStyleSheet
		{
		public:
			using CascadingStyleSheet::m_statements;
			using CascadingStyleSheet::m_lastRuleOrder;
		};

		template <class CHAR>
		static void SkipCommentInner(CHAR*& input, CHAR* end)
		{
			input += 2;
			for (;;) {
				if (input + 2 > end) {
					input = end;
					return;
				}
				if (*input == '*' && *(input + 1) == '/') {
					input += 2;
					break;
				} else {
					input++;
				}
			}
		}

		template <class CHAR>
		SLIB_INLINE static void SkipComment(CHAR*& input, CHAR* end)
		{
			if (input + 2 > end) {
				return;
			}
			if (*input == '/' && *(input + 1) == '*') {
				SkipCommentInner(input, end);
			}
		}

		template <class CHAR>
		static void SkipWhitespaces(CHAR*& input, CHAR* end)
		{
			while (input < end) {
				CHAR ch = *input;
				if (SLIB_CHAR_IS_WHITE_SPACE(ch)) {
					input++;
				} else if (ch == '/') {
					if (input + 1 < end && *(input + 1) == '*') {
						SkipCommentInner(input, end);
					} else {
						return;
					}
				} else {
					return;
				}
			}
		}

		template <class CHAR>
		static void SkipLine(CHAR*& input, CHAR* end)
		{
			while (input < end) {
				CHAR ch = *(input++);
				if (ch == '\r') {
					if (input < end) {
						ch = input[1];
						if (ch == '\n') {
							input++;
						}
					}
					return;
				} else if (ch == '\n') {
					return;
				}
			}
		}

		template <class CHAR>
		static sl_bool ParseHexValue(CHAR*& input, CHAR* end, sl_uint32& value)
		{
			sl_uint32 h = (sl_uint32)(*input);
			h = SLIB_CHAR_HEX_TO_INT(h);
			if (h < 16) {
				value = h;
				input++;
				sl_uint32 n = 1;
				while (input < end && n < 6) {
					h = (sl_uint32)(*input);
					h = SLIB_CHAR_HEX_TO_INT(h);
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

		template <class CHAR>
		static sl_bool ParseUnquotedStringValue(CHAR*& input, CHAR* end, sl_bool flagUrl, CHAR* _out, sl_size& lenOutput)
		{
			lenOutput = 0;
			CHAR* start = input;
			while (input < end) {
				CHAR ch = *input;
				if (SLIB_CHAR_IS_ALNUM(ch) || (ch & 0x80) || ch == '-' || ch == '_' || ch == '@' || ch == '%') {
					input++;
				} else if (ch == '\\') {
					if (input >= end) {
						return sl_false;
					}
					sl_uint32 code;
					if (ParseHexValue(input, end, code)) {
						lenOutput += Charsets::getUtfn(code, _out ? _out + lenOutput : sl_null);
						continue;
					} else {
						ch = *(input++);
					}
				} else {
					if (flagUrl) {
						if (ch == '.' || ch == '/' || ch == '#' || ch == ':') {
							input++;
						} else {
							break;
						}
					} else {
						break;
					}
				}
				if (_out) {
					_out[lenOutput] = ch;
				}
				lenOutput++;
			}
			return lenOutput > 0;
		}

		template <class CHAR>
		static sl_bool ParseUnquotedStringValue(String* _out, CHAR*& input, CHAR* end, sl_bool flagUrl)
		{
			if (input >= end) {
				return sl_false;
			}
			sl_size len;
			if (_out) {
				CHAR* s = input;
				if (!(ParseUnquotedStringValue(s, end, flagUrl, (CHAR*)sl_null, len))) {
					return sl_false;
				}
				typename StringTypeFromCharType<CHAR>::Type ret = StringTypeFromCharType<CHAR>::Type::allocate(len);
				if (ret.isNull()) {
					return sl_false;
				}
				ParseUnquotedStringValue(input, end, flagUrl, ret.getData(), len);
				*_out = String::from(ret);
				return sl_true;
			} else {
				return ParseUnquotedStringValue(input, end, flagUrl, (CHAR*)sl_null, len);
			}
		}

		template <class CHAR>
		SLIB_INLINE static String ParseUnquotedStringValue(CHAR*& input, CHAR* end, sl_bool flagUrl)
		{
			String ret;
			if (ParseUnquotedStringValue(&ret, input, end, flagUrl)) {
				return ret;
			}
			return sl_null;
		}

		template <class CHAR>
		static sl_bool ParseStringValue(CHAR*& input, CHAR* end, CHAR chOpen, CHAR* _out, sl_size& lenOutput)
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
					if (ParseHexValue(input, end, code)) {
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

		template <class CHAR>
		static sl_bool ParseStringValue(String* _out, CHAR*& input, CHAR* end)
		{
			CHAR chOpen = *input;
			if (chOpen != '"' && chOpen != '\'') {
				return ParseUnquotedStringValue(_out, input, end, sl_true);
			}
			input++;
			sl_size len;
			if (_out) {
				CHAR* s = input;
				if (!(ParseStringValue(s, end, chOpen, (CHAR*)sl_null, len))) {
					return sl_false;
				}
				typename StringTypeFromCharType<CHAR>::Type ret = StringTypeFromCharType<CHAR>::Type::allocate(len);
				if (ret.isNull()) {
					return sl_null;
				}
				if (!(ParseStringValue(input, end, chOpen, ret.getData(), len))) {
					return sl_null;
				}
				*_out = String::from(ret);
				return sl_true;
			} else {
				return ParseStringValue(input, end, chOpen, (CHAR*)sl_null, len);
			}
		}

		template <class CHAR>
		SLIB_INLINE static String ParseStringValue(CHAR*& input, CHAR* end)
		{
			String ret;
			if (ParseStringValue(&ret, input, end)) {
				return ret;
			}
			return sl_null;
		}

		template <class CHAR>
		static sl_bool ParseIdentifier(CHAR*& input, CHAR* end, CHAR* _out, sl_size& lenOutput)
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
					if (ParseHexValue(input, end, code)) {
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

		template <class CHAR>
		static String ParseIdentifier(CHAR*& input, CHAR* end)
		{
			if (input >= end) {
				return sl_null;
			}
			CHAR* s = input;
			sl_size len;
			if (!(ParseIdentifier(s, end, (CHAR*)sl_null, len))) {
				return sl_null;
			}
			typename StringTypeFromCharType<CHAR>::Type ret = StringTypeFromCharType<CHAR>::Type::allocate(len);
			if (ret.isNull()) {
				return sl_null;
			}
			ParseIdentifier(input, end, ret.getData(), len);
			return String::from(ret);
		}

		template <class CHAR>
		class StylesParser
		{
		public:
			CascadingStyleSheetHelper* sheet;
			CHAR* current;
			CHAR* end;
			sl_bool flagIgnoreErrors;
			
		public:
			static void run(CascadingStyleSheet* sheet, CHAR* data, sl_size len, CascadingStyleSheet::ParseParam& param)
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
				SkipWhitespaces(current, end);
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
								SkipLine(current, end);
							} else {
								return sl_false;
							}
						}
					} else {
						if (!(parseRule(statements))) {
							if (flagIgnoreErrors) {
								SkipLine(current, end);
							} else {
								return sl_false;
							}
						}
					}
					SkipWhitespaces(current, end);
				}
				return sl_true;
			}

			sl_bool parseAtRule(CascadingStyleStatements& statements)
			{
				CascadingStyleAtRule at;
				at.identifier = ParseIdentifier(current, end);
				if (at.identifier.isNull()) {
					return sl_false;
				}
				at.rule = parseValueRegion(0);
				if (at.rule.isNull()) {
					return sl_false;
				}
				if (*current == '{') {
					Shared<CascadingStyleStatements> statements = Shared<CascadingStyleStatements>::create();
					if (statements.isNull()) {
						return sl_false;
					}
					current++;
					SkipWhitespaces(current, end);
					CHAR* start = current;
					if (flagIgnoreErrors) {
						sl_bool flagDecl = sl_false;
						if (parseStatements(*statements, '}')) {
							flagDecl = statements->rules.isEmpty() && at.statements->atRules.isEmpty();
						} else {
							flagDecl = sl_true;
						}
						if (flagDecl) {
							current = start;
							if (!(parseDeclarations(at.declarations, '}'))) {
								return sl_false;
							}
						} else {
							at.statements = Move(statements);
						}
					} else {
						if (parseStatements(*statements, '}')) {
							at.statements = Move(statements);
						} else {
							current = start;
							if (!(parseDeclarations(at.declarations, '}'))) {
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
				Ref<CascadingStyleSelector> selector = parseCombindSelector(sl_null, CascadingStyleCombinator::None);
				if (selector.isNull()) {
					return sl_false;
				}
				if (current >= end) {
					return sl_false;
				}
				List< Ref<CascadingStyleSelector> > group;
				while (*current == ',') {
					current++;
					SkipWhitespaces(current, end);
					Ref<CascadingStyleSelector> item = parseCombindSelector(sl_null, CascadingStyleCombinator::None);
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
				SkipWhitespaces(current, end);
				CascadingStyleDeclarations declarations;
				if (!(parseDeclarations(declarations, '}'))) {
					return sl_false;
				}
				CascadingStyleRule rule;
				rule.selector = Move(selector);
				rule.declarations = declarations;
				rule.order = ++(sheet->m_lastRuleOrder);
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
				do {
					CascadingStyleSelector* selector = rule.selector.get();
					if (selector->id.isNotNull()) {
						statements.rulesById.add_NoLock(selector->id, rule);
						break;
					}
					if (selector->classNames.getCount() >= 1) {
						statements.rulesByClass.add_NoLock(selector->classNames.getValueAt_NoLock(0), rule);
						break;
					}
					if (selector->flagUniversal || selector->elementName.isNull()) {
						statements.rulesUniversal.add_NoLock(rule);
					} else {
						statements.rulesByTag.add_NoLock(selector->elementName, rule);
					}
				} while (0);
				return statements.rules.add_NoLock(Move(rule));
			}

			static CascadingStyleDeclarations parseDeclarations(CHAR* data, sl_size len)
			{
				StylesParser parser;
				parser.sheet = sl_null;
				parser.current = data;
				parser.end = data + len;
				parser.flagIgnoreErrors = sl_false;
				CascadingStyleDeclarations decls;
				parser.parseDeclarations(decls, 0);
				return decls;
			}

			Ref<CascadingStyleValue> parseDeclaration(String& name)
			{
				name = ParseIdentifier(current, end);
				if (name.isNull()) {
					return sl_null;
				}
				SkipWhitespaces(current, end);
				if (current >= end) {
					return sl_null;
				}
				if (*current != ':') {
					return sl_null;
				}
				current++;
				SkipWhitespaces(current, end);
				Ref<CascadingStyleValue> ret = parseValue();
				if (ret.isNull()) {
					return sl_null;
				}
				if (current < end) {
					if (*current == '!') {
						current++;
						SkipComment(current, end);
						String label = ParseIdentifier(current, end);
						if (label == StringView::literal("important")) {
							ret->setImportant();
						}
					}
				}
				return ret;
			}

			// Passes ending character
			sl_bool parseDeclarations(CascadingStyleDeclarations& declarations, CHAR chEnd)
			{
				while (current < end) {
					CHAR ch = *current;
					if (ch == ';') {
						current++;
					} else if (ch == chEnd) {
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
								SkipLine(current, end);
							} else {
								return sl_false;
							}
						}
					}
					SkipWhitespaces(current, end);
				}
				if (!chEnd) {
					return sl_true;
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
				SkipWhitespaces(current, end);
				if (current >= end) {
					return sl_null;
				}
				if (*current != '(') {
					return sl_null;
				}
				current++;
				SkipWhitespaces(current, end);
				String name = ParseIdentifier(current, end);
				if (name.isNull()) {
					return sl_null;
				}
				SkipWhitespaces(current, end);
				if (current >= end) {
					return sl_null;
				}
				String defaultValue;
				CHAR ch = *current;
				if (ch == ',') {
					current++;
					SkipWhitespaces(current, end);
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

			Ref<CascadingStyleSelector> parseCombindSelector(CascadingStyleSelector* before, CascadingStyleCombinator combinator)
			{
				Ref<CascadingStyleSelector> first = parseBasicSelector(before, combinator);
				if (first.isNull()) {
					return sl_null;
				}
				SkipWhitespaces(current, end);
				if (current >= end) {
					return first;
				}
				combinator = CascadingStyleCombinator::Descendant;
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
					SkipWhitespaces(current, end);
					if (current >= end) {
						return sl_null;
					}
				}
				CHAR* start = current;
				Ref<CascadingStyleSelector> last = parseCombindSelector(first.get(), combinator);
				if (last.isNotNull()) {
					return last;
				}
				if (combinator != CascadingStyleCombinator::Descendant) {
					return sl_null;
				}
				if (start != current) {
					return sl_null;
				}
				return first;
			}

			Ref<CascadingStyleSelector> parseBasicSelector(CascadingStyleSelector* before, CascadingStyleCombinator combinator)
			{
				Ref<CascadingStyleSelector> ret = new CascadingStyleSelector;
				if (ret.isNull()) {
					return sl_null;
				}
				ret->before = before;
				ret->combinator = combinator;
				CHAR* start = current;
				for (;;) {
					switch (*current) {
						case '.':
							{
								current++;
								String name = ParseIdentifier(current, end);
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
								String name = ParseIdentifier(current, end);
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
									String name = ParseIdentifier(current, end);
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
								String name = ParseIdentifier(current, end);
								if (name.isNull()) {
									name = ParseUnquotedStringValue(current, end, sl_false);
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
					SkipComment(current, end);
				}
				return ret;
			}

			sl_bool parseSelector_AttributeMatch(CascadingStyleSelector* selector)
			{
				SkipWhitespaces(current, end);
				if (current >= end) {
					return sl_false;
				}
				CascadingStyleAttributeMatch match;
				match.name = ParseIdentifier(current, end);
				if (match.name.isNull()) {
					return sl_false;
				}
				SkipWhitespaces(current, end);
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
						match.type = CascadingStyleMatchType::LocalePrefix;
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
					SkipWhitespaces(current, end);
					if (current >= end) {
						return sl_false;
					}
					if (*current != '=') {
						return sl_false;
					}
					current++;
				}
				if (match.type != CascadingStyleMatchType::Exist) {
					SkipWhitespaces(current, end);
					if (current >= end) {
						return sl_false;
					}
				}
				if (match.type != CascadingStyleMatchType::Exist) {
					match.value = ParseStringValue(current, end);
					if (match.value.isNull()) {
						return sl_false;
					}
					SkipWhitespaces(current, end);
					if (current >= end) {
						return sl_false;
					}
					if (*current == 'i' || *current == 'I') {
						match.flagIgnoreCase = sl_true;
						current++;
						SkipWhitespaces(current, end);
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
							if (!(ParseStringValue(current, end, ch, (CHAR*)sl_null, n))) {
								return sl_false;
							}
							break;
						case '/':
							if (current + 1 < end && *(current + 1) == '*') {
								SkipComment(current, end);
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
				return !chEnd;
			}

			String parseValueRegion(CHAR chEnd)
			{
				SkipWhitespaces(current, end);
				CHAR* start = current;
				if (!(skipValueRegion(chEnd))) {
					return sl_null;
				}
				if (current == start) {
					return String::getEmpty();
				}
				CHAR* last = current - 1;
				while (last > start) {
					CHAR ch = *last;
					if (SLIB_CHAR_IS_WHITE_SPACE(ch)) {
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
				if (!(ParseIdentifier(current, end, (CHAR*)sl_null, n))) {
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

	String CascadingStyleValue::toString()
	{
		StringBuffer buf;
		if (toString(buf)) {
			return buf.merge();
		} else {
			return sl_null;
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

	sl_bool CascadingStyleSelector::matchElement(const Ref<XmlElement>& element)
	{
		if (pseudoClasses.isNotNull() || pseudoElement.isNotNull()) {
			return sl_false;
		}
		if (flagNamespace) {
			if (namespaceName.isNotNull()) {
				if (element->getNamespace() != namespaceName) {
					return sl_false;
				}
			}
		}
		if (!flagUniversal) {
			if (elementName.isNotNull()) {
				if (element->getLocalName() != elementName) {
					return sl_false;
				}
			}
		}
		if (id.isNotNull()) {
			SLIB_STATIC_STRING(_id, "id")
			if (element->getAttribute(_id) != id) {
				return sl_false;
			}
		}
		if (classNames.isNotNull()) {
			SLIB_STATIC_STRING(_class, "class")
			String classValue = element->getAttribute(_class);
			ListElements<String> items(classNames);
			for (sl_size i = 0; i < items.count; i++) {
				if (Stringx::indexOfWholeWord(classValue, items[i]) < 0) {
					return sl_false;
				}
			}
		}
		if (attributes.isNotNull()) {
			ListElements<CascadingStyleAttributeMatch> items(attributes);
			for (sl_size i = 0; i < items.count; i++) {
				CascadingStyleAttributeMatch& attr = items[i];
				String value = element->getAttribute(attr.name);
				switch (attr.type) {
					case CascadingStyleMatchType::Exist:
						if (value.isNull()) {
							return sl_false;
						}
						break;
					case CascadingStyleMatchType::Equal:
						if (attr.flagIgnoreCase) {
							if (!(value.equals_IgnoreCase(attr.value))) {
								return sl_false;
							}
						} else {
							if (!(value.equals(attr.value))) {
								return sl_false;
							}
						}
						break;
					case CascadingStyleMatchType::Contains_Word:
						if (attr.flagIgnoreCase) {
							if (Stringx::indexOfWholeWord_IgnoreCase(value, attr.value) < 0) {
								return sl_false;
							}
						} else {
							if (Stringx::indexOfWholeWord(value, attr.value) < 0) {
								return sl_false;
							}
						}
						break;
					case CascadingStyleMatchType::LocalePrefix:
						if (attr.flagIgnoreCase) {
							if (!(value.startsWith_IgnoreCase(attr.value))) {
								return sl_false;
							}
						} else {
							if (!(value.startsWith(attr.value))) {
								return sl_false;
							}
						}
						{
							sl_size n = attr.value.getLength();
							if (value.getLength() > n) {
								if (value.getAt(n) != '-') {
									return sl_false;
								}
							}
						}
						break;
					case CascadingStyleMatchType::Start:
						if (attr.flagIgnoreCase) {
							if (!(value.startsWith_IgnoreCase(attr.value))) {
								return sl_false;
							}
						} else {
							if (!(value.startsWith(attr.value))) {
								return sl_false;
							}
						}
						break;
					case CascadingStyleMatchType::End:
						if (attr.flagIgnoreCase) {
							if (!(value.endsWith_IgnoreCase(attr.value))) {
								return sl_false;
							}
						} else {
							if (!(value.endsWith(attr.value))) {
								return sl_false;
							}
						}
						break;
					case CascadingStyleMatchType::Contain:
						if (attr.flagIgnoreCase) {
							if (!(value.contains_IgnoreCase(attr.value))) {
								return sl_false;
							}
						} else {
							if (!(value.contains(attr.value))) {
								return sl_false;
							}
						}
						break;
					default:
						return sl_false;
				}
			}
		}
		if (before.isNotNull()) {
			switch (combinator) {
				case CascadingStyleCombinator::Descendant:
					{
						Ref<XmlNodeGroup> e = element->getParent();
						while (e.isNotNull()) {
							if (!(e->isElementNode())) {
								break;
							}
							if (before->matchElement(Ref<XmlElement>::from(e))) {
								return sl_true;
							}
							e = e->getParent();
						}
						return sl_false;
					}
				case CascadingStyleCombinator::Child:
					{
						Ref<XmlNodeGroup> parent = element->getParent();
						if (parent.isNotNull()) {
							if (parent->isElementNode()) {
								return before->matchElement(Ref<XmlElement>::from(parent));
							}
						}
						return sl_false;
					}
				case CascadingStyleCombinator::Sibling:
					{
						Ref<XmlNodeGroup> parent = element->getParent();
						if (parent.isNotNull()) {
							sl_size n = parent->getChildCount();
							for (sl_size i = 0; i < n; i++) {
								Ref<XmlElement> item = parent->getChildElement(i);
								if (item.isNotNull()) {
									if (item == element) {
										break;
									}
									if (before->matchElement(item)) {
										return sl_true;
									}
								}
							}
						}
						return sl_false;
					}
				case CascadingStyleCombinator::Adjacent:
					{
						Ref<XmlNodeGroup> parent = element->getParent();
						if (parent.isNotNull()) {
							sl_size n = parent->getChildCount();
							Ref<XmlElement> itemBefore;
							for (sl_size i = 0; i < n; i++) {
								Ref<XmlElement> item = parent->getChildElement(i);
								if (item.isNotNull()) {
									if (item == element) {
										if (itemBefore.isNotNull()) {
											return before->matchElement(itemBefore);
										} else {
											return sl_false;
										}
									}
									itemBefore = Move(item);
								}
							}
						}
						return sl_false;
					}
				default:
					return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool CascadingStyleSelector::toString(StringBuffer& output)
	{
		if (before.isNotNull() && combinator != CascadingStyleCombinator::None) {
			if (!(before->toString(output))) {
				return sl_false;
			}
			switch (combinator) {
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
		}
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
					case CascadingStyleMatchType::LocalePrefix:
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
		return sl_true;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CascadingStyleRule)

	CascadingStyleRule::CascadingStyleRule()
	{
	}

	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CascadingStyleAtRule)

	CascadingStyleAtRule::CascadingStyleAtRule()
	{
	}


	CascadingStyleStatements::CascadingStyleStatements()
	{
	}

	CascadingStyleStatements::~CascadingStyleStatements()
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
		if (!(CascadingStyleSheet::writeDeclarationsString(output, declarations, tabLevel))) {
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
			if (!(CascadingStyleSheet::writeDeclarationsString(output, declarations, tabLevel))) {
				return sl_false;
			}
			if (!(WriteTabs(output, tabLevel))) {
				return sl_false;
			}
			if (!(output.addStatic("}"))) {
				return sl_false;
			}
		} else if (statements.isNotNull()) {
			if (!(output.addStatic(" {\r\n"))) {
				return sl_false;
			}
			if (!(statements->toString(output, tabLevel + 1))) {
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


	CascadingStyleSheet::CascadingStyleSheet(): m_lastRuleOrder(0)
	{
	}

	CascadingStyleSheet::~CascadingStyleSheet()
	{
	}
	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(CascadingStyleSheet, ParseParam)

	CascadingStyleSheet::ParseParam::ParseParam(): flagIgnoreErrors(sl_true), flagError(sl_false)
	{
	}

	void CascadingStyleSheet::addStyles(const StringParam& _styles, ParseParam& param)
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
		ParseParam param;
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

	List<CascadingStyleDeclarations> CascadingStyleSheet::getElementDeclarations(const Ref<XmlElement>& element)
	{
		if (m_statements.rules.isEmpty()) {
			return sl_null;
		}
		CMap<sl_uint32, CascadingStyleDeclarations> decls;
		if (m_statements.rulesById.isNotEmpty()) {
			SLIB_STATIC_STRING(_id, "id")
			String id = element->getAttribute(_id);
			if (id.isNotNull()) {
				MapNode<String, CascadingStyleRule>* start;
				MapNode<String, CascadingStyleRule>* end;
				if (m_statements.rulesById.getEqualRange(id, &start, &end)) {
					for (;;) {
						CascadingStyleRule& rule = start->value;
						if (rule.selector->matchElement(element)) {
							decls.emplace_NoLock(rule.order, rule.declarations);
						}
						if (start == end) {
							break;
						} else {
							start = start->getNext();
						}
					}
				}
			}
		}
		if (m_statements.rulesByClass.isNotEmpty()) {
			SLIB_STATIC_STRING(_class, "class")
			String classes = element->getAttribute(_class);
			if (classes.isNotNull()) {
				classes = classes.trim();
				if (classes.isNotEmpty()) {
					sl_reg index = 0;
					for (;;) {
						String name;
						index = Stringx::indexOfWhitespace(classes, index);
						if (index < 0) {
							name = classes;
						} else {
							name = classes.substring(0, index);
						}
						MapNode<String, CascadingStyleRule>* start;
						MapNode<String, CascadingStyleRule>* end;
						if (m_statements.rulesByClass.getEqualRange(name, &start, &end)) {
							for (;;) {
								CascadingStyleRule& rule = start->value;
								if (rule.selector->matchElement(element)) {
									decls.emplace_NoLock(rule.order, rule.declarations);
								}
								if (start == end) {
									break;
								} else {
									start = start->getNext();
								}
							}
						}
						if (index < 0) {
							break;
						}
						index = Stringx::indexOfNotWhitespace(classes, index + 1);
						if (index < 0) {
							break;
						}
						classes = classes.substring(index);
					}
				}
			}
		}
		if (m_statements.rulesByTag.isNotEmpty()) {
			String tagName = element->getLocalName();
			MapNode<String, CascadingStyleRule>* start;
			MapNode<String, CascadingStyleRule>* end;
			if (m_statements.rulesByTag.getEqualRange(tagName, &start, &end)) {
				for (;;) {
					CascadingStyleRule& rule = start->value;
					if (rule.selector->matchElement(element)) {
						decls.emplace_NoLock(rule.order, rule.declarations);
					}
					if (start == end) {
						break;
					} else {
						start = start->getNext();
					}
				}
			}
		}
		ListElements<CascadingStyleRule> rules(m_statements.rulesUniversal);
		for (sl_size i = 0; i < rules.count; i++) {
			CascadingStyleRule& rule(rules[i]);
			if (rule.selector->matchElement(element)) {
				decls.emplace_NoLock(rule.order, rule.declarations);
			}
		}
		return decls.getAllValues_NoLock();
	}

	List<CascadingStyleDeclarations> CascadingStyleSheet::getElementDeclarations(const Ref<XmlElement>& element, const StringParam& styles)
	{
		List<CascadingStyleDeclarations> decls = getElementDeclarations(element);
		CascadingStyleDeclarations add = parseDeclarations(styles);
		if (add.isNotNull()) {
			decls.add_NoLock(Move(add));
		}
		return decls;
	}

	CascadingStyleDeclarations CascadingStyleSheet::parseDeclarations(const StringParam& _input)
	{
		if (_input.isEmpty()) {
			return sl_null;
		}
		if (_input.is8BitsStringType()) {
			StringData input(_input);
			return StylesParser<sl_char8>::parseDeclarations(input.getData(), input.getLength());
		} else if (_input.is16BitsStringType()) {
			StringData16 input(_input);
			return StylesParser<sl_char16>::parseDeclarations(input.getData(), input.getLength());
		} else {
			StringData32 input(_input);
			return StylesParser<sl_char32>::parseDeclarations(input.getData(), input.getLength());
		}
	}

	void CascadingStyleSheet::mergeDeclarations(CascadingStyleDeclarations& to, const CascadingStyleDeclarations& from)
	{
		auto node = from.getFirstNode();
		while (node) {
			String& key = node->key;
			if (!(node->value->isImportant())) {
				Ref<CascadingStyleValue> orig = to.getValue_NoLock(key);
				if (orig.isNotNull()) {
					if (orig->isImportant()) {
						continue;
					}
				}
			}
			to.put_NoLock(key, node->value);
			node = node->getNext();
		}
	}

	CascadingStyleDeclarations CascadingStyleSheet::mergeDeclarations(const List<CascadingStyleDeclarations>& list)
	{
		CascadingStyleDeclarations decls;
		ListElements<CascadingStyleDeclarations> items(list);
		for (sl_size i = 0; i < items.count; i++) {
			mergeDeclarations(decls, items[i]);
		}
		return decls;
	}

	String CascadingStyleSheet::getDeclarationValue(const CascadingStyleDeclarations& decls, const String& key)
	{
		Ref<CascadingStyleValue> value = decls.getValue_NoLock(key);
		if (value.isNotNull()) {
			CascadingStyleValueType type = value->getType();
			if (type == CascadingStyleValueType::Normal) {
				return ((CascadingStyleNormalValue*)(value.get()))->getValue();
			} else if (type == CascadingStyleValueType::Variable) {
				String name = ((CascadingStyleVariableValue*)(value.get()))->getName();
				for (sl_uint32 i = 0; i < 64; i++) {
					value = decls.getValue_NoLock(name);
					CascadingStyleValueType type = value->getType();
					if (type == CascadingStyleValueType::Normal) {
						return ((CascadingStyleNormalValue*)(value.get()))->getValue();
					} else if (type == CascadingStyleValueType::Variable) {
						name = ((CascadingStyleVariableValue*)(value.get()))->getName();
					} else {
						return sl_null;
					}
				}
			}
		}
		return sl_null;
	}

	namespace {
		static Ref<CascadingStyleValue> GetDeclarationValue(const List<CascadingStyleDeclarations>& decls, const String& key)
		{
			ListElements<CascadingStyleDeclarations> items(decls);
			sl_bool flagImportant = sl_false;
			Ref<CascadingStyleValue> ret;
			for (sl_size i = 0; i < items.count; i++) {
				Ref<CascadingStyleValue> value = items[i].getValue_NoLock(key);
				if (value.isNotNull()) {
					if (value->isImportant()) {
						flagImportant = sl_true;
						ret = Move(value);
					} else {
						if (!flagImportant) {
							ret = Move(value);
						}
					}
				}
			}
			return ret;
		}
	}

	String CascadingStyleSheet::getDeclarationValue(const List<CascadingStyleDeclarations>& decls, const String& key)
	{
		Ref<CascadingStyleValue> value = GetDeclarationValue(decls, key);
		if (value.isNotNull()) {
			CascadingStyleValueType type = value->getType();
			if (type == CascadingStyleValueType::Normal) {
				return ((CascadingStyleNormalValue*)(value.get()))->getValue();
			} else if (type == CascadingStyleValueType::Variable) {
				String name = ((CascadingStyleVariableValue*)(value.get()))->getName();
				for (sl_uint32 i = 0; i < 64; i++) {
					value = GetDeclarationValue(decls, name);
					CascadingStyleValueType type = value->getType();
					if (type == CascadingStyleValueType::Normal) {
						return ((CascadingStyleNormalValue*)(value.get()))->getValue();
					} else if (type == CascadingStyleValueType::Variable) {
						name = ((CascadingStyleVariableValue*)(value.get()))->getName();
					} else {
						return sl_null;
					}
				}
			}
		}
		return sl_null;
	}

	sl_bool CascadingStyleSheet::writeDeclarationsString(StringBuffer& output, const CascadingStyleDeclarations& decls, sl_uint32 tabLevel)
	{
		auto node = decls.getFirstNode();
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

	namespace {
		template <class CHAR>
		SLIB_INLINE static sl_reg ParseStringValue(String* _out, const CHAR* input, sl_size posBegin, sl_size posEnd)
		{
			if (posBegin >= posEnd) {
				return SLIB_PARSE_ERROR;
			}
			CHAR* s = (CHAR*)(input + posBegin);
			CHAR* end = (CHAR*)(input + posEnd);
			CHAR* p = s;
			if (ParseStringValue(_out, p, end)) {
				return p - s;
			}
			return SLIB_PARSE_ERROR;
		}
	}

	sl_reg CascadingStyleSheet::parseStringValue(String* _out, const sl_char8* str, sl_size posBegin, sl_size posEnd)
	{
		return ParseStringValue(_out, str, posBegin, posEnd);
	}

	sl_reg CascadingStyleSheet::parseStringValue(String* _out, const sl_char16* str, sl_size posBegin, sl_size posEnd)
	{
		return ParseStringValue(_out, str, posBegin, posEnd);
	}
	
	sl_reg CascadingStyleSheet::parseStringValue(String* _out, const sl_char32* str, sl_size posBegin, sl_size posEnd)
	{
		return ParseStringValue(_out, str, posBegin, posEnd);
	}

}
