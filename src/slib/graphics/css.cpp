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

				Ref<CascadingStyleSelector> parseSelector()
				{
					Ref<CascadingStyleSelector> ret = new CascadingStyleSelector;
					if (ret.isNull()) {
						return sl_null;
					}
					for (;;) {
						switch (*current) {
							case '.':
								{
									if (ret->className.isNotNull()) {
										return sl_null;
									}
									current++;
									String name = parseIdentifier();
									if (name.isNull()) {
										return sl_null;
									}
									ret->className = Move(name);
									break;
								}
							case '#':
								{
									if (ret->id.isNotNull()) {
										return sl_null;
									}
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
									if (!(parseSelectorAttributeMatch(ret.get()))) {
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
							default:
								{
									if (ret->elementName.isNotNull() || ret->flagUniversal) {
										return sl_null;
									}
									String name = parseIdentifier();
									if (name.isNull()) {
										return sl_null;
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

				String parseIdentifier()
				{
					if (current >= end) {
						return sl_null;
					}
					List<CHAR> buf;
					CHAR* start = current;
					while (current < end) {
						CHAR ch = *current;
						if (SLIB_CHAR_IS_ALNUM(ch) || ch == '-' || ch == '_') {
							current++;
						} else if (ch == '\\') {
							if (start < current) {
								if (!(buf.addElements_NoLock(start, current - start))) {
									return sl_null;
								}
							}
							current++;
							if (current >= end) {
								return sl_null;
							}

							start = current;
						} else {
							break;
						}
					}
					if (start < current) {
						if (buf.isNull()) {
							return String::from(start, current - start);
						} else {
							if (!(buf.addElements_NoLock(start, current - start))) {
								return sl_null;
							}
						}
					}
					return String::from(buf.getData(), buf.getCount());
				}

				String parseEscapedIdentifier(CHAR* prefix, sl_size lenPrefix)
				{
					if (current >= end) {
						return sl_null;
					}
					StringBuffer buf;
					if (!(processEscapedIdentifierPrefix(buf, prefix, lenPrefix))) {
						return sl_null;
					}
					CHAR* start = current;
					while (current < end) {
						CHAR ch = *current;
						if (SLIB_CHAR_IS_ALNUM(ch) || ch == '-' || ch == '_') {
							current++;
						} else if (ch == '\\') {
							if (!(processEscapedIdentifierPrefix(buf, start, current - start))) {
								return sl_null;
							}
							if (current >= end) {
								return sl_null;
							}
							CHAR ch = *current;
							if (!SLIB_CHAR_IS_HEX(ch)) {
								return sl_null;
							}
							sl_char32 code = SLIB_CHAR_HEX_TO_INT(ch);
							current++;
							sl_uint32 n = 1;
							while (current < end && n < 6) {
								CHAR ch = *current;
								if (SLIB_CHAR_IS_HEX(ch)) {
									code = (code << 4) | SLIB_CHAR_HEX_TO_INT(ch);
									current++;
									n++;
								} else {
									break;
								}
							}
							String str = String::from(&code, 1);
							if (str.isNull()) {
								return sl_null;
							}
							if (!(buf.add(Move(str)))) {
								return sl_null;
							}
						} else {
							break;
						}
					}
					if (!(processEscapedIdentifierPrefix(buf, start, current - start))) {
						return sl_null;
					}
					return buf.merge();
				}

				sl_bool parseSelectorAttributeMatch(CascadingStyleSelector* selector)
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
							ch = *(input++);
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
						return sl_null;
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
