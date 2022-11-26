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
						CHAR ch = *current;
						if (ch == '.') {
							if (ret->className.isNotNull()) {
								return sl_null;
							}
							current++;
							String name = parseIdentifier();
							if (name.isNull()) {
								return sl_null;
							}
							ret->className = Move(name);
						} else if (ch == '#') {
							if (ret->id.isNotNull()) {
								return sl_null;
							}
							current++;
							String name = parseIdentifier();
							if (name.isNull()) {
								return sl_null;
							}
							ret->id = Move(name);
						} else {
							if (ret->elementName.isNotNull()) {
								return sl_null;
							}
							String name = parseIdentifier();
							if (name.isNull()) {
								return sl_null;
							}
							ret->elementName = Move(name);
						}
						if (current >= end) {
							break;
						}
					}
					return ret;
				}

				String parseIdentifier()
				{
					CHAR* start = current;
					while (current < end) {
						CHAR ch = *current;
						if (SLIB_CHAR_IS_ALNUM(ch) || ch == '-' || ch == '_') {
							current++;
						} else if (ch == '\\') {
							return parseEscapedIdentifier(start, current - start);
						} else {
							break;
						}
					}
					if (start < current) {
						return String::from(start, current - start);
					} else {
						return sl_null;
					}
				}

				sl_bool processEscapedIdentifierPrefix(StringBuffer& buf, CHAR* prefix, sl_size lenPrefix);

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

			};

			template <class CHAR>
			sl_bool StylesParser<CHAR>::processEscapedIdentifierPrefix(StringBuffer& buf, CHAR* prefix, sl_size lenPrefix)
			{
				if (!lenPrefix) {
					return sl_true;
				}
				String str = String::from(prefix, lenPrefix);
				if (str.isNull()) {
					return sl_false;
				}
				return buf.add(Move(str));
			}

			template <>
			sl_bool StylesParser<sl_char8>::processEscapedIdentifierPrefix(StringBuffer& buf, sl_char8* prefix, sl_size lenPrefix)
			{
				if (!lenPrefix) {
					return sl_true;
				}
				return buf.addStatic(prefix, lenPrefix);
			}

		}
	}

	using namespace priv::css;

	CascadingStyleSelector::CascadingStyleSelector()
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
