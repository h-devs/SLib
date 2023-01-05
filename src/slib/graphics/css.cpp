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

namespace slib
{

	namespace priv
	{
		namespace css
		{

			template <class CHAR>
			class StylesParser
			{
			public:
				CascadingStyleSheet* sheet;
				CHAR* current;
				CHAR* end;

			public:
				static sl_bool run(CascadingStyleSheet* sheet, CHAR* data, sl_size len)
				{
					StylesParser parser;
					parser.sheet = sheet;
					parser.current = data;
					parser.end = data + len;
					return parser.run();
				}

				sl_bool run()
				{
					skipWhitespaces();
					while (current < end) {
						if (!(parseRule())) {
							return sl_false;
						}
					}
					return sl_true;
				}

				void skipWhitespaces()
				{
					while (current < end) {
						if (!SLIB_CHAR_IS_WHITE_SPACE(*current)) {
							return;
						}
						current++;
					}
				}

				sl_bool parseRule()
				{
					Ref<CascadingStyleSelector> selector = parseSelector();
					if (selector.isNull()) {
						return sl_false;
					}

					return sl_true;
				}

				Ref<CascadingStyleSelector> parseCombindSelector()
				{
					Ref<CascadingStyleSelector> root = parseBasicSelector();
					if (root.isNull()) {
						return sl_null;
					}
					CascadingStyleSelector* parent = root.get();
					for (;;) {
						skipWhitespaces();
						if (current >= end) {
							break;
						}
						CHAR ch = *current;
						if (ch == '>') {
							root->combinator = CascadingStyleCombinator::Child;
						} else if (ch == '~') {
							root->combinator = CascadingStyleCombinator::Sibling;
						} else if (ch == '+') {
							root->combinator = CascadingStyleCombinator::Adjacent;
						}
					}
					return root;
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
									if (SLIB_CHAR_IS_DIGIT(next) || next == '-') {
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
						} else if (ch == chEnd) {
							return sl_true;
						} else {
							current++;
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
					current++;
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

	CascadingStyleSelector::CascadingStyleSelector(): flagNamespace(sl_false), flagUniversal(sl_false)
	{
	}

	CascadingStyleSelector::~CascadingStyleSelector()
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
