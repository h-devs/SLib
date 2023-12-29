/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/graphics/text.h"

#include "slib/graphics/font_atlas.h"
#include "slib/graphics/util.h"
#include "slib/math/calculator.h"
#include "slib/device/device.h"
#include "slib/data/xml.h"
#include "slib/core/charset.h"
#include "slib/core/string_buffer.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(TextStyle)

	TextStyle::TextStyle() noexcept:
		flagDefinedUnderline(sl_false), flagUnderline(sl_false), flagOverline(sl_false), flagLineThrough(sl_false),
		flagLink(sl_false),
		textColor(Color::Zero), backgroundColor(Color::Zero),
		lineHeight(-1), yOffset(0)
	{
	}

	Ref<TextStyle> TextStyle::duplicate() const noexcept
	{
		return new TextStyle(*this);
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(TextItem, DrawParam)

	TextItem::DrawParam::DrawParam() noexcept:
		textColor(Color::Black),
		shadowOpacity(0), shadowRadius(3), shadowColor(Color::Black), shadowOffset(0, 0),
		lineThickness(1),
		flagDrawSelection(sl_false), selectionStart(-1), selectionEnd(-1)
	{
	}

	void TextItem::DrawParam::fixSelectionRange() noexcept
	{
		if (selectionStart >= 0) {
			if (selectionEnd < 0) {
				selectionEnd = SLIB_REG_MAX;
			} else {
				if (selectionStart > selectionEnd) {
					Swap(selectionStart, selectionEnd);
				}
			}
		} else {
			selectionStart = 0;
			selectionEnd = 0;
		}
	}


	SLIB_DEFINE_OBJECT(TextItem, Object)

	TextItem::TextItem(TextItemType type) noexcept: m_type(type), m_layoutPosition(0, 0), m_layoutSize(0, 0)
	{
	}

	TextItem::~TextItem() noexcept
	{
	}

	TextItemType TextItem::getType() noexcept
	{
		return m_type;
	}

	Ref<TextStyle> TextItem::getStyle() noexcept
	{
		return m_style;
	}

	void TextItem::setStyle(const Ref<TextStyle>& style)
	{
		m_style = style;
	}

	Ref<Font> TextItem::getFont() noexcept
	{
		Ref<TextStyle> style = m_style;
		if (style.isNotNull()) {
			return style->font;
		}
		return sl_null;
	}

	Point TextItem::getLayoutPosition() noexcept
	{
		return m_layoutPosition;
	}

	void TextItem::setLayoutPosition(const Point& pt) noexcept
	{
		m_layoutPosition = pt;
	}

	Size TextItem::getLayoutSize() noexcept
	{
		return m_layoutSize;
	}

	void TextItem::setLayoutSize(const Size& size) noexcept
	{
		m_layoutSize = size;
	}

	Rectangle TextItem::getLayoutFrame() noexcept
	{
		return Rectangle(m_layoutPosition.x, m_layoutPosition.y, m_layoutPosition.x + m_layoutSize.x, m_layoutPosition.y + m_layoutSize.y);
	}

	void TextItem::draw(Canvas* canvas, sl_real x, sl_real y, const DrawParam& param)
	{
	}

	String TextItem::getPlainText()
	{
		return sl_null;
	}


	TextWordItem::TextWordItem() noexcept: TextItem(TextItemType::Word)
	{
		m_widthCached = 0;
		m_heightCached = 0;
	}

	TextWordItem::~TextWordItem() noexcept
	{
	}

	Ref<TextWordItem> TextWordItem::create(const String16& text, const Ref<TextStyle>& style) noexcept
	{
		if (style.isNotNull()) {
			Ref<TextWordItem> ret = new TextWordItem;
			if (ret.isNotNull()) {
				ret->m_text = text;
				ret->m_style = style;
				return ret;
			}
		}
		return sl_null;
	}

	const String16& TextWordItem::getText() noexcept
	{
		return m_text;
	}

	Size TextWordItem::getSize() noexcept
	{
		ObjectLocker lock(this);

		String16 text = m_text;
		Ref<Font> font = getFont();
		if (m_textCached == text && m_fontCached == font) {
			return Size(m_widthCached, m_heightCached);
		}
		if (font.isNotNull()) {
			Ref<FontAtlas> atlas = font->getSharedAtlas();
			if (atlas.isNotNull()) {
				Size size = atlas->measureText(text, sl_false);
				m_textCached = text;
				m_fontCached = font;
				m_widthCached = size.x;
				m_heightCached = size.y;
				return size;
			}
		}
		m_widthCached = 0;
		m_heightCached = 0;
		return Size::zero();
	}

	void TextWordItem::draw(Canvas* canvas, sl_real x, sl_real y, const DrawParam& param)
	{
		Ref<Font> font = getFont();
		if (font.isNull()) {
			return;
		}

		Canvas::DrawTextParam dp;
		dp.font = font;
		dp.color = param.textColor;
		if (param.shadowOpacity > 0) {
			dp.shadowOpacity = param.shadowOpacity;
			dp.shadowRadius = param.shadowRadius;
			dp.shadowColor = param.shadowColor;
			dp.shadowOffset = param.shadowOffset;
		}
		dp.x = x;
		dp.y = y;
		dp.text = m_text;
		canvas->drawText(dp);
	}

	String TextWordItem::getPlainText()
	{
		return String::from(m_text);
	}

	sl_bool TextWordItem::containsNoLatin() noexcept
	{
		if (m_flagNoLatin.flagNull) {
			sl_bool flag = sl_false;
			const String16& text = getText();
			sl_char16* s = text.getData();
			sl_size len = text.getLength();
			for (sl_size i = 0; i < len; i++) {
				sl_char16 c = s[i];
				if (c >= 128) {
					flag = sl_true;
					break;
				}
			}
			m_flagNoLatin.value = flag;
			m_flagNoLatin.flagNull = sl_false;
		}
		return m_flagNoLatin.value;
	}


	TextCharItem::TextCharItem() noexcept: TextItem(TextItemType::Char)
	{
		m_widthCached = 0;
		m_heightCached = 0;
	}

	TextCharItem::~TextCharItem() noexcept
	{
	}

	Ref<TextCharItem> TextCharItem::create(sl_char32 ch, const Ref<TextStyle>& style) noexcept
	{
		if (style.isNotNull()) {
			Ref<TextCharItem> ret = new TextCharItem;
			if (ret.isNotNull()) {
				ret->m_char = ch;
				ret->m_style = style;
				return ret;
			}
		}
		return sl_null;
	}

	Size TextCharItem::getSize() noexcept
	{
		ObjectLocker lock(this);

		Ref<Font> font = getFont();
		if (m_fontCached == font) {
			return Size(m_widthCached, m_heightCached);
		}
		if (font.isNotNull()) {
			Ref<FontAtlas> atlas = font->getSharedAtlas();
			if (atlas.isNotNull()) {
				Size size = atlas->getFontSize(m_char);
				m_fontCached = font;
				m_widthCached = size.x;
				m_heightCached = size.y;
				return size;
			}
		}
		m_widthCached = 0;
		m_heightCached = 0;
		return Size::zero();
	}

	void TextCharItem::draw(Canvas* canvas, sl_real x, sl_real y, const DrawParam& param)
	{
		Ref<Font> font = getFont();
		if (font.isNull()) {
			return;
		}

		Canvas::DrawTextParam dp;
		dp.font = font;
		dp.color = param.textColor;
		if (param.shadowOpacity > 0) {
			dp.shadowOpacity = param.shadowOpacity;
			dp.shadowRadius = param.shadowRadius;
			dp.shadowColor = param.shadowColor;
			dp.shadowOffset = param.shadowOffset;
		}
		dp.y = y;

		dp.text = StringView32(&m_char, 1);
		dp.x = x;
		canvas->drawText(dp);
	}

	String TextCharItem::getPlainText()
	{
		return String::from(&m_char, 1);
	}


	TextJoinedCharItem::TextJoinedCharItem() noexcept: TextItem(TextItemType::JoinedChar)
	{
	}

	TextJoinedCharItem::~TextJoinedCharItem() noexcept
	{
	}

	Ref<TextJoinedCharItem> TextJoinedCharItem::create(const String16& text, const Ref<TextStyle>& style) noexcept
	{
		if (style.isNotNull()) {
			Ref<TextJoinedCharItem> ret = new TextJoinedCharItem;
			if (ret.isNotNull()) {
				ret->m_text = text;
				ret->m_style = style;
				return ret;
			}
		}
		return sl_null;
	}

	void TextJoinedCharItem::setStyle(const Ref<TextStyle>& style)
	{
		TextItem::setStyle(style);
		m_joinedCharFont.setNull();
	}

	Ref<Font> TextJoinedCharItem::getFont() noexcept
	{
		Ref<TextStyle> style = m_style;
		if (style.isNotNull()) {
			Ref<Font> font = m_joinedCharFont;
			if (font.isNotNull()) {
				if (m_joinedCharFontBase == style->font) {
					return font;
				}
			}
			font = style->font;
			if (font.isNotNull()) {
				String name = style->joinedCharFamilyName;
#ifdef SLIB_PLATFORM_IS_WINDOWS
				if (name.isEmpty()) {
					name = "Segoe UI Emoji";
				}
#endif
				if (name.isNotEmpty()) {
					FontDesc desc;
					font->getDesc(desc);
					if (desc.familyName != name) {
						desc.familyName = name;
						Ref<Font> fontNew = Font::create(desc);
						m_joinedCharFont = fontNew;
						m_joinedCharFontBase = font;
						return fontNew;
					}
				}
				m_joinedCharFont = font;
				m_joinedCharFontBase = font;
				return font;
			}
		}
		return sl_null;
	}

	Size TextJoinedCharItem::getSize() noexcept
	{
		ObjectLocker lock(this);

		Ref<Font> font = getFont();
		if (m_fontCached == font) {
			return Size(m_widthCached, m_heightCached);
		}
		if (font.isNotNull()) {
			Size size = font->measureText(m_text, sl_false);
			m_fontCached = font;
			m_widthCached = size.x;
			m_heightCached = size.y;
			return size;
		}
		m_widthCached = 0;
		m_heightCached = 0;
		return Size::zero();
	}

	void TextJoinedCharItem::draw(Canvas* canvas, sl_real x, sl_real y, const DrawParam& param)
	{
		Ref<Font> font = getFont();
		if (font.isNotNull()) {
			Canvas::DrawTextParam dp;
			dp.font = font;
			dp.color = param.textColor;
			if (param.shadowOpacity > 0) {
				dp.shadowOpacity = param.shadowOpacity;
				dp.shadowRadius = param.shadowRadius;
				dp.shadowColor = param.shadowColor;
				dp.shadowOffset = param.shadowOffset;
			}
			dp.x = x;
			dp.y = y;
			dp.text = m_text;
			canvas->drawText(dp);
		}
	}

	String TextJoinedCharItem::getPlainText()
	{
		return String::from(m_text);
	}


	TextSpaceItem::TextSpaceItem() noexcept: TextItem(TextItemType::Space)
	{
	}

	TextSpaceItem::~TextSpaceItem() noexcept
	{
	}

	Ref<TextSpaceItem> TextSpaceItem::create(const Ref<TextStyle>& style) noexcept
	{
		if (style.isNotNull()) {
			Ref<TextSpaceItem> ret = new TextSpaceItem;
			if (ret.isNotNull()) {
				ret->m_style = style;
				return ret;
			}
		}
		return sl_null;
	}

	Size TextSpaceItem::getSize() noexcept
	{
		Ref<Font> font = getFont();
		if (font.isNotNull()) {
			FontMetrics fm;
			if (font->getFontMetrics(fm)) {
				sl_real h = fm.ascent + fm.descent;
				return Size(h * 0.3f, h);
			}
		}
		return Size::zero();
	}

	String TextSpaceItem::getPlainText()
	{
		SLIB_RETURN_STRING(" ")
	}


	TextTabItem::TextTabItem() noexcept: TextItem(TextItemType::Tab)
	{
	}

	TextTabItem::~TextTabItem() noexcept
	{
	}

	Ref<TextTabItem> TextTabItem::create(const Ref<TextStyle>& style) noexcept
	{
		if (style.isNotNull()) {
			Ref<TextTabItem> ret = new TextTabItem;
			if (ret.isNotNull()) {
				ret->m_style = style;
				return ret;
			}
		}
		return sl_null;
	}

	sl_real TextTabItem::getHeight() noexcept
	{
		Ref<Font> font = getFont();
		if (font.isNotNull()) {
			FontMetrics fm;
			if (font->getFontMetrics(fm)) {
				sl_real h = fm.ascent + fm.descent;
				return h;
			}
		}
		return 0;
	}

	String TextTabItem::getPlainText()
	{
		SLIB_RETURN_STRING("\t")
	}


	TextLineBreakItem::TextLineBreakItem() noexcept: TextItem(TextItemType::LineBreak)
	{
	}

	TextLineBreakItem::~TextLineBreakItem() noexcept
	{
	}

	Ref<TextLineBreakItem> TextLineBreakItem::create(const Ref<TextStyle>& style) noexcept
	{
		if (style.isNotNull()) {
			Ref<TextLineBreakItem> ret = new TextLineBreakItem;
			if (ret.isNotNull()) {
				ret->m_style = style;
				return ret;
			}
		}
		return sl_null;
	}

	sl_real TextLineBreakItem::getHeight() noexcept
	{
		Ref<Font> font = getFont();
		if (font.isNotNull()) {
			return font->getFontHeight();
		} else {
			return 0;
		}
	}

	String TextLineBreakItem::getPlainText()
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		SLIB_RETURN_STRING("\r\n")
#else
		SLIB_RETURN_STRING("\n")
#endif
	}


	TextHorizontalLineItem::TextHorizontalLineItem() noexcept : TextItem(TextItemType::HorizontalLine)
	{
	}

	TextHorizontalLineItem::~TextHorizontalLineItem() noexcept
	{
	}

	Ref<TextHorizontalLineItem> TextHorizontalLineItem::create(const Ref<TextStyle>& style) noexcept
	{
		if (style.isNotNull()) {
			Ref<TextHorizontalLineItem> ret = new TextHorizontalLineItem;
			if (ret.isNotNull()) {
				ret->m_style = style;
				return ret;
			}
		}
		return sl_null;
	}

	sl_real TextHorizontalLineItem::getHeight() noexcept
	{
		Ref<TextStyle> style = m_style;
		if (style.isNotNull()) {
			if (style->lineHeight >= 0) {
				return style->lineHeight;
			}
		}
		Ref<Font> font = getFont();
		if (font.isNotNull()) {
			return font->getFontHeight() / 2.0f;
		} else {
			return 0;
		}
	}

	String TextHorizontalLineItem::getPlainText()
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		SLIB_RETURN_STRING("\r\n")
#else
		SLIB_RETURN_STRING("\n")
#endif
	}


	TextAttachItem::TextAttachItem() noexcept: TextItem(TextItemType::Attach)
	{
	}

	TextAttachItem::~TextAttachItem() noexcept
	{
	}


	SLIB_DEFINE_OBJECT(TextParagraph, Object)

	TextParagraph::TextParagraph() noexcept
	{
		m_contentWidth = 0;
		m_contentHeight = 0;

		m_align = Alignment::Left;
	}

	TextParagraph::~TextParagraph() noexcept
	{
	}

	namespace {

		template <class CHAR>
		SLIB_INLINE static sl_bool CheckHTTP(const CHAR* s, sl_size len)
		{
			if (len <= 7) return sl_false;
			if (*(s++) != 'h') return sl_false;
			if (*(s++) != 't') return sl_false;
			if (*(s++) != 't') return sl_false;
			if (*(s++) != 'p') return sl_false;
			if (*(s++) != ':') return sl_false;
			if (*(s++) != '/') return sl_false;
			if (*(s++) != '/') return sl_false;
			return sl_true;
		}

		template <class CHAR>
		SLIB_INLINE static sl_bool CheckHTTPS(const CHAR* s, sl_size len)
		{
			if (len <= 8) return sl_false;
			if (*(s++) != 'h') return sl_false;
			if (*(s++) != 't') return sl_false;
			if (*(s++) != 't') return sl_false;
			if (*(s++) != 'p') return sl_false;
			if (*(s++) != 's') return sl_false;
			if (*(s++) != ':') return sl_false;
			if (*(s++) != '/') return sl_false;
			if (*(s++) != '/') return sl_false;
			return sl_true;
		}

		template <class CHAR>
		SLIB_INLINE static sl_bool CheckWWW(const CHAR* s, sl_size len)
		{
			if (len <= 4) return sl_false;
			if (*(s++) != 'w') return sl_false;
			if (*(s++) != 'w') return sl_false;
			if (*(s++) != 'w') return sl_false;
			if (*(s++) != '.') return sl_false;
			return sl_true;
		}

		template <class CHAR>
		static sl_bool CheckURL(const CHAR* s, sl_size len)
		{
			if (CheckHTTP(s, len)) {
				return sl_true;
			}
			if (CheckHTTPS(s, len)) {
				return sl_true;
			}
			if (CheckWWW(s, len)) {
				return sl_true;
			}
			return sl_false;
		}

		static sl_bool CheckURL(const String16& text, String16& url)
		{
			sl_char16* s = text.getData();
			sl_size len = text.getLength();
			if (CheckHTTP(s, len)) {
				url = text;
				return sl_true;
			}
			if (CheckHTTPS(s, len)) {
				url = text;
				return sl_true;
			}
			if (CheckWWW(s, len)) {
				sl_reg len = text.indexOf('/');
				if (len < 0) {
					len = text.getLength();
				}
				sl_reg indexDotDot = text.indexOf(SLIB_UNICODE(".."));
				if (indexDotDot >= 0 && indexDotDot < len) {
					return sl_false;
				}
				sl_char16* sz = text.getData();
				for (sl_reg i = 0; i < len; i++) {
					sl_char16 ch = sz[i];
					if (!(SLIB_CHAR_IS_ALNUM(ch) || ch == '-' || ch == '_' || ch == '.')) {
						return sl_false;
					}
				}
				url = StringView16::literal(u"http://") + text;
				return sl_true;
			}
			return sl_false;
		}

		template <class CHAR>
		static Ref<TextItem> CreateWordOrCharItem(const CHAR* str, sl_size len, const Ref<TextStyle>& style)
		{
			if (len == 1) {
				return TextCharItem::create(str[0], style);
			} else {
				String16 s = String16::from(str, len);
				sl_size n = s.getLength();
				if (!n) {
					return sl_null;
				}
				if (n == 1) {
					return TextCharItem::create(s.getAt(0), style);
				}
				return TextWordItem::create(s, style);
			}
		}

		static void AddWordItems(CList< Ref<TextItem> >& items, const String16& str, const Ref<TextStyle>& style, sl_bool flagEnabledHyperlinksInPlainText)
		{
			if (flagEnabledHyperlinksInPlainText) {
				String16 url;
				if (CheckURL(str, url)) {
					Ref<TextStyle> styleNew = style->duplicate();
					if (styleNew.isNotNull()) {
						styleNew->flagLink = sl_true;
						styleNew->href = String::from(url);
						Ref<TextWordItem> item = TextWordItem::create(str, styleNew);
						if (item.isNotNull()) {
							items.add_NoLock(Move(item));
						}
					}
					return;
				}
			}
			sl_char16* s = str.getData();
			sl_size len = str.getLength();
			sl_size start = 0;
			for (sl_size i = 0; i < len; i++) {
				sl_char16 c = s[i];
				if (c == '-') {
					Ref<TextWordItem> item = TextWordItem::create(str.substring(start, i + 1), style);
					if (item.isNotNull()) {
						items.add_NoLock(Move(item));
					}
					start = i + 1;
				}
			}
			if (start < len) {
				Ref<TextWordItem> item = TextWordItem::create(str.substring(start, len), style);
				if (item.isNotNull()) {
					items.add_NoLock(Move(item));
				}
			}
		}

		template <class CHAR>
		static void AddWordItems(CList< Ref<TextItem> >& items, const CHAR* str, sl_size len, const Ref<TextStyle>& style, sl_bool flagEnabledHyperlinksInPlainText)
		{
			if (len == 1) {
				Ref<TextCharItem> item = TextCharItem::create(str[0], style);
				if (item.isNotNull()) {
					items.add_NoLock(Move(item));
				}
			} else {
				String16 s = String16::from(str, len);
				sl_size n = s.getLength();
				if (!n) {
					return;
				}
				if (n == 1) {
					Ref<TextCharItem> item = TextCharItem::create(s.getAt(0), style);
					if (item.isNotNull()) {
						items.add_NoLock(Move(item));
					}
				} else {
					AddWordItems(items, s, style, flagEnabledHyperlinksInPlainText);
				}
			}
		}

	}

	template <class CHAR>
	void TextParagraph::_addText(const CHAR* data, sl_size len, const Ref<TextStyle>& style, sl_bool flagEnabledHyperlinksInPlainText, sl_bool flagMnemonic) noexcept
	{
		if (!len) {
			return;
		}
		ObjectLocker lock(this);
		sl_size startWord = 0;
		sl_size pos = 0;
		while (pos < len) {
			sl_size oldPos = pos;
			sl_uint32 ch;
			if (!(Charsets::getUnicode(*((sl_char32*)&ch), data, len, pos))) {
				ch = '?';
				pos++;
			}
#define BEGIN_ADD_TEXT_CASE \
				if (startWord < oldPos) { \
					AddWordItems(m_items, data + startWord, oldPos - startWord, style, flagEnabledHyperlinksInPlainText); \
				}
#define END_ADD_TEXT_CASE \
				startWord = pos; \
				break
			switch (ch) {
				case ' ':
				case 0xA0: /*nbsp*/
					BEGIN_ADD_TEXT_CASE {
						Ref<TextSpaceItem> item = TextSpaceItem::create(style);
						if (item.isNotNull()) {
							m_items.add_NoLock(Move(item));
						}
					} END_ADD_TEXT_CASE;
				case '\t':
					BEGIN_ADD_TEXT_CASE {
						Ref<TextTabItem> item = TextTabItem::create(style);
						if (item.isNotNull()) {
							m_items.add_NoLock(Move(item));
						}
					} END_ADD_TEXT_CASE;
				case '\r':
				case '\n':
					BEGIN_ADD_TEXT_CASE {
						Ref<TextLineBreakItem> item = TextLineBreakItem::create(style);
						if (item.isNotNull()) {
							m_items.add_NoLock(Move(item));
						}
						if (ch == '\r' && pos < len) {
							if (data[pos] == '\n') {
								pos++;
							}
						}
					} END_ADD_TEXT_CASE;
				default:
					{
						sl_size lenJoinedChar = Charsets::getJoinedCharLength(ch, data + pos, len - pos);
						if (lenJoinedChar && lenJoinedChar + pos > oldPos + 1) {
							BEGIN_ADD_TEXT_CASE{
								pos += lenJoinedChar;
								Ref<TextJoinedCharItem> item = TextJoinedCharItem::create(String16::from(data + oldPos, pos - oldPos), style);
								if (item.isNotNull()) {
									m_items.add_NoLock(Move(item));
								}
							} END_ADD_TEXT_CASE;
						} else if (flagMnemonic && ch == '&' && pos < len) {
							ch = data[pos];
							if (SLIB_CHAR_IS_ALNUM(ch)) {
								BEGIN_ADD_TEXT_CASE {
									Ref<TextStyle> _style = style;
									if (!(style->flagUnderline)) {
										_style = style->duplicate();
										if (_style.isNotNull()) {
											_style->flagUnderline = sl_true;
										} else {
											_style = style;
										}
									}
									Ref<TextCharItem> item = TextCharItem::create(ch, _style);
									if (item.isNotNull()) {
										m_items.add_NoLock(Move(item));
									}
									pos++;
									flagMnemonic = sl_false;
								} END_ADD_TEXT_CASE;
							} else if (ch == '&') {
								BEGIN_ADD_TEXT_CASE {
									Ref<TextCharItem> item = TextCharItem::create('&', style);
									if (item.isNotNull()) {
										m_items.add_NoLock(Move(item));
									}
									pos++;
								} END_ADD_TEXT_CASE;
							}
						}
						break;
					}
			}
		}
		if (startWord) {
			if (startWord < len) {
				AddWordItems(m_items, data + startWord, len - startWord, style, flagEnabledHyperlinksInPlainText);
			}
		} else {
			AddWordItems(m_items, data, len, style, flagEnabledHyperlinksInPlainText);
		}
	}

	void TextParagraph::addText(const StringParam& _text, const Ref<TextStyle>& style, sl_bool flagEnabledHyperlinksInPlainText, sl_bool flagMnemonic) noexcept
	{
		if (style.isNull()) {
			return;
		}
		if (_text.is32BitsStringType()) {
			StringData32 text(_text);
			_addText(text.getData(), text.getLength(), style, flagEnabledHyperlinksInPlainText, flagMnemonic);
		} else if (_text.is16BitsStringType()) {
			StringData16 text(_text);
			_addText(text.getData(), text.getLength(), style, flagEnabledHyperlinksInPlainText, flagMnemonic);
		} else {
			StringData text(_text);
			_addText(text.getData(), text.getLength(), style, flagEnabledHyperlinksInPlainText, flagMnemonic);
		}
	}

	void TextParagraph::addHyperTextNodeGroup(const Ref<XmlNodeGroup>& group, const Ref<TextStyle>& style) noexcept
	{
		if (group.isNull()) {
			return;
		}
		sl_size n = group->getChildCount();
		for (sl_size i = 0; i < n; i++) {
			Ref<XmlNode> child = group->getChild(i);
			if (child.isNotNull()) {
				XmlNodeType type = child->getType();
				if (type == XmlNodeType::Element) {
					addHyperTextElement(Ref<XmlElement>::from(child), style);
				} else if (type == XmlNodeType::WhiteSpace) {
					XmlWhiteSpace* space = (XmlWhiteSpace*)(child.get());
					addText(space->getContent(), style);
				} else if (type == XmlNodeType::Text) {
					XmlText* text = (XmlText*)(child.get());
					addText(text->getText(), style);
				}
			}
		}
	}

	namespace {
		static sl_bool ParseSize(const String& _str, const Ref<Font>& _font, float* _out)
		{
			sl_real sizeBase;
			Ref<Font> font = _font;
			if (font.isNotNull()) {
				sizeBase = font->getSize();
			} else {
				font = Font::getDefault();
				if (font.isNotNull()) {
					sizeBase = font->getSize();
				} else {
					sizeBase = Font::getDefaultFontSize();
				}
			}
			String str = _str.trim().toLower();
			sl_real f = -1;
			if (str == "medium") {
				f = Font::getDefaultFontSize();
			} else if (str == "xx-small") {
				f = Font::getDefaultFontSize() / 4;
			} else if (str == "x-small") {
				f = Font::getDefaultFontSize() / 2;
			} else if (str == "small") {
				f = Font::getDefaultFontSize() / 4 * 3;
			} else if (str == "large") {
				f = Font::getDefaultFontSize() / 2 * 3;
			} else if (str == "x-large") {
				f = Font::getDefaultFontSize() * 2;
			} else if (str == "xx-large") {
				f = Font::getDefaultFontSize() * 4;
			} else if (str == "smaller") {
				f = sizeBase / 1.5f;
			} else if (str == "initial") {
				f = Font::getDefaultFontSize();
			} else if (str == "inherit") {
				f = sizeBase;
			}
			if (f >= 0) {
				if (_out) {
					*_out = f;
				}
				return sl_true;
			}
			sl_char8* sz = str.getData();
			sl_size len = str.getLength();
			f = 0;
			sl_reg pos = Calculator::calculate(&f, sl_null, sz, 0, len);
			if (pos > 0) {
				StringView unit = StringView(sz + pos, len - pos).trim();
				if (Math::isAlmostZero(f) && unit.isEmpty()) {
				} else if (unit == ("%")) {
					f = sizeBase * f / 100;
				} else if (unit == ("cm")) {
					f = GraphicsUtil::centimeterToPixel(f);
				} else if (unit == ("mm")) {
					f = GraphicsUtil::millimeterToPixel(f);
				} else if (unit == ("in")) {
					f = GraphicsUtil::inchToPixel(f);
				} else if (unit == ("px")) {
				} else if (unit == ("pt")) {
					f = GraphicsUtil::pointToPixel(f);
				} else if (unit == ("pc")) {
					f = GraphicsUtil::picasToPixel(f);
				} else if (unit == ("em")) {
					f = sizeBase * f;
				} else if (unit == ("rem")) {
					f = Font::getDefaultFontSize() * f;
				} else if (unit == ("ch")) {
					if (font.isNotNull()) {
						f = font->getFontHeight() * f;
					}
				} else if (unit == ("ex")) {
					if (font.isNotNull()) {
						f = font->measureText("0").x * f;
					}
				} else if (unit == ("vw")) {
					f = (sl_real)(Device::getScreenWidth() * f / 100);
				} else if (unit == ("vh")) {
					f = (sl_real)(Device::getScreenHeight() * f / 100);
				} else if (unit == ("vmin")) {
					f = (sl_real)(Math::min(Device::getScreenWidth(), Device::getScreenHeight()) * f / 100);
				} else if (unit == ("vmax")) {
					f = (sl_real)(Math::max(Device::getScreenWidth(), Device::getScreenHeight()) * f / 100);
				} else {
					return sl_false;
				}
				if (_out) {
					*_out = f;
				}
				return sl_true;
			}
			return sl_false;
		}
	}

	void TextParagraph::addHyperTextElement(const Ref<XmlElement>& element, const Ref<TextStyle>& style) noexcept
	{
		if (element.isNull()) {
			return;
		}

		Ref<Font> font = style->font;

		sl_bool flagDefineTextColor = sl_false;
		Color attrTextColor;
		sl_bool flagDefineBackColor = sl_false;
		Color attrBackColor;
		sl_bool flagDefineFamilyName = sl_false;
		String attrFamilyName;
		sl_bool flagDefineJoinedCharFamilyName = sl_false;
		String attrJoinedCharFamilyName;
		sl_bool flagDefineFontSize = sl_false;
		String attrFontSize;
		sl_real attrFontSizeParsed = 0;
		sl_bool flagDefineBold = sl_false;
		sl_bool attrBold = sl_false;
		sl_bool flagDefineUnderline = sl_false;
		sl_bool attrUnderline = sl_false;
		sl_bool flagDefineOverline = sl_false;
		sl_bool attrOverline = sl_false;
		sl_bool flagDefineLineThrough = sl_false;
		sl_bool attrLineThrough = sl_false;
		sl_bool flagDefineItalic = sl_false;
		sl_bool attrItalic = sl_false;
		sl_bool flagDefineLink = sl_false;
		sl_bool flagDefineHref = sl_false;
		String attrHref;
		sl_bool flagDefineLineHeight = sl_false;
		String attrLineHeight;
		sl_real attrLineHeightParsed = 0;
		sl_bool flagDefineYOffset = sl_false;
		sl_real attrYOffset = 0;

		String name = element->getName().toLower();
		if (name == StringView::literal("a")) {
			flagDefineLink = sl_true;
		} else if (name == StringView::literal("b")) {
			flagDefineBold = sl_true;
			attrBold = sl_true;
		} else if (name == StringView::literal("i")) {
			flagDefineItalic = sl_true;
			attrItalic = sl_true;
		} else if (name == StringView::literal("u")) {
			flagDefineUnderline = sl_true;
			attrUnderline = sl_true;
		} else if (name == StringView::literal("sup")) {
			if (font.isNotNull()) {
				flagDefineYOffset = sl_true;
				attrYOffset = style->yOffset - font->getFontHeight() / 4;
				flagDefineFontSize = sl_true;
				attrFontSizeParsed = font->getSize() * 2 / 3;
			}
		} else if (name == StringView::literal("sub")) {
			if (font.isNotNull()) {
				flagDefineYOffset = sl_true;
				attrYOffset = style->yOffset + font->getFontHeight() / 4;
				flagDefineFontSize = sl_true;
				attrFontSizeParsed = font->getSize() * 2 / 3;
			}
		}

		{
			String value = element->getAttribute_IgnoreCase(StringView::literal("href"));
			if (value.isNotNull()) {
				flagDefineHref = sl_true;
				attrHref = value;
			}
		}
		{
			String value = element->getAttribute_IgnoreCase(StringView::literal("face"));
			if (value.isNotNull()) {
				flagDefineFamilyName = sl_true;
				attrFamilyName = value;
			}
		}
		{
			String value = element->getAttribute_IgnoreCase(StringView::literal("joinedCharFace"));
			if (value.isNotNull()) {
				flagDefineJoinedCharFamilyName = sl_true;
				attrJoinedCharFamilyName = value;
			}
		}
		{
			String value = element->getAttribute_IgnoreCase(StringView::literal("size"));
			if (value.isNotNull()) {
				flagDefineFontSize = sl_true;
				attrFontSize = value.trim().toLower();
			}
		}
		{
			String value = element->getAttribute_IgnoreCase(StringView::literal("color"));
			if (value.isNotNull()) {
				if (attrTextColor.parse(value)) {
					flagDefineTextColor = sl_true;
				}
			}
		}
		{
			String value = element->getAttribute_IgnoreCase(StringView::literal("bgcolor"));
			if (value.isNotNull()) {
				if (attrBackColor.parse(value)) {
					flagDefineBackColor = sl_true;
				}
			}
		}

		String attrStyle = element->getAttribute_IgnoreCase(StringView::literal("style"));
		if (attrStyle.isNotEmpty()) {
			attrStyle = attrStyle.toLower();
			sl_char8* buf = attrStyle.getData();
			sl_size len = attrStyle.getLength();
			sl_size pos = 0;
			while (pos < len) {
				sl_reg end = attrStyle.indexOf(';', pos);
				if (end < 0) {
					end = len;
				}
				sl_size d = pos;
				for (; (sl_reg)d < end; d++) {
					if (buf[d] == ':') {
						break;
					}
				}
				if (pos < d && (sl_reg)d < end - 1) {
					String name = attrStyle.substring(pos, d).trim().toLower();
					String value = attrStyle.substring(d + 1, end).trim().toLower();
					if (name == StringView::literal("background-color")) {
						if (attrBackColor.parse(value)) {
							flagDefineBackColor = sl_true;
						}
					} else if (name == StringView::literal("color")) {
						if (attrTextColor.parse(value)) {
							flagDefineTextColor = sl_true;
						}
					} else if (name == StringView::literal("line-height")) {
						flagDefineLineHeight = sl_true;
						attrLineHeight = value;
					} else if (name == StringView::literal("font-family")) {
						flagDefineFamilyName = sl_true;
						attrFamilyName = value;
					} else if (name == StringView::literal("emoji-family")) {
						flagDefineJoinedCharFamilyName = sl_true;
						attrJoinedCharFamilyName = value;
					} else if (name == StringView::literal("font-size")) {
						flagDefineFontSize = sl_true;
						attrFontSize = value;
					} else if (name == StringView::literal("font-weight")) {
						flagDefineBold = sl_true;
						attrBold = value == StringView::literal("bold");
					} else if (name == StringView::literal("font-style")) {
						flagDefineItalic = sl_true;
						attrItalic = value == StringView::literal("italic") || value == StringView::literal("oblique");
					} else if (name == StringView::literal("font")) {
						ListElements<String> elements(value.split(" "));
						sl_size indexSize = 0;
						for (; indexSize < elements.count; indexSize++) {
							String& s = elements[indexSize];
							if (s == StringView::literal("oblique") || s == StringView::literal("italic")) {
								flagDefineItalic = sl_true;
								attrBold = sl_true;
							} else if (s == StringView::literal("bold")) {
								flagDefineBold = sl_true;
								attrBold = sl_true;
							}
							sl_reg indexLineHeight = s.indexOf('/');
							if (indexLineHeight < 0) {
								if (ParseSize(s, font, &attrFontSizeParsed)) {
									attrFontSize.setNull();
									flagDefineFontSize = sl_true;
									break;
								}
							} else {
								if (ParseSize(s.substring(indexLineHeight + 1), font, &attrLineHeightParsed)) {
									attrLineHeight.setNull();
									flagDefineLineHeight = sl_true;
								}
								if (ParseSize(s.substring(0, indexLineHeight), font, &attrFontSizeParsed)) {
									attrFontSize.setNull();
									flagDefineFontSize = sl_true;
								}
								break;
							}
						}
						SLIB_STATIC_STRING(strSpace, " ")
						String face = String::join(ListLocker<String>(elements.list, indexSize + 1), strSpace);
						if (face.isNotEmpty()) {
							flagDefineFamilyName = sl_true;
							attrFamilyName = face;
						}
					} else if (name == StringView::literal("text-decoration") || name == StringView::literal("text-decoration-line")) {
						flagDefineUnderline = sl_true;
						attrUnderline = value.contains(StringView::literal("underline"));
						flagDefineOverline = sl_true;
						attrOverline = value.contains(StringView::literal("overline"));
						flagDefineLineThrough = sl_true;
						attrLineThrough = value.contains(StringView::literal("line-through"));
					}
				}
				pos = end + 1;
			}
		}

		if (flagDefineFontSize) {
			if (attrFontSize.isNotNull()) {
				if (!(ParseSize(attrFontSize, font, &attrFontSizeParsed))) {
					flagDefineFontSize = sl_false;
				}
			}
		}
		if (flagDefineLineHeight) {
			if (attrLineHeight.isNotNull()) {
				if (!(ParseSize(attrLineHeight, font, &attrLineHeightParsed))) {
					flagDefineLineHeight = sl_false;
				}
			}
		}

		Ref<TextStyle> styleNew = style;
		FontDesc fontDesc;
		sl_bool flagNewFont = sl_false;
		do {
			if (font.isNull()) {
				flagNewFont = sl_true;
				break;
			}
			font->getDesc(fontDesc);
			if (flagDefineFamilyName) {
				if (fontDesc.familyName != attrFamilyName) {
					flagNewFont = sl_true;
					break;
				}
			}
			if (flagDefineFontSize) {
				if (!(Math::isAlmostZero(fontDesc.size - attrFontSizeParsed))) {
					flagNewFont = sl_true;
					break;
				}
			}
			if (flagDefineBold) {
				if (fontDesc.flagBold != attrBold) {
					flagNewFont = sl_true;
					break;
				}
			}
			if (flagDefineItalic) {
				if (fontDesc.flagItalic != attrItalic) {
					flagNewFont = sl_true;
					break;
				}
			}
		} while (0);

		sl_bool flagNewStyle = flagNewFont;
		if (!flagNewStyle) {
			do {
				if (flagDefineJoinedCharFamilyName) {
					if (style->joinedCharFamilyName != attrJoinedCharFamilyName) {
						flagNewStyle = sl_true;
						break;
					}
				}
				if (flagDefineTextColor) {
					if (style->textColor != attrTextColor) {
						flagNewStyle = sl_true;
						break;
					}
				}
				if (flagDefineBackColor) {
					if (style->backgroundColor != attrBackColor) {
						flagNewStyle = sl_true;
						break;
					}
				}
				if (flagDefineLink) {
					if (!(style->flagLink)) {
						flagNewStyle = sl_true;
						break;
					}
				}
				if (flagDefineHref) {
					if (style->href != attrHref) {
						flagNewStyle = sl_true;
						break;
					}
				}
				if (flagDefineYOffset) {
					if (style->yOffset != attrYOffset) {
						flagNewStyle = sl_true;
						break;
					}
				}
				if (flagDefineLineHeight) {
					if (!(Math::isAlmostZero(style->lineHeight - attrLineHeightParsed))) {
						flagNewStyle = sl_true;
						break;
					}
				}
				if (flagDefineUnderline) {
					if (style->flagUnderline != attrUnderline) {
						flagNewStyle = sl_true;
						break;
					}
				}
				if (flagDefineOverline) {
					if (style->flagOverline != attrOverline) {
						flagNewStyle = sl_true;
						break;
					}
				}
				if (flagDefineLineThrough) {
					if (style->flagLineThrough != attrLineThrough) {
						flagNewStyle = sl_true;
						break;
					}
				}
			} while (0);
		}

		if (flagNewStyle) {
			styleNew = style->duplicate();
			if (styleNew.isNull()) {
				return;
			}
			if (flagNewFont) {
				if (flagDefineFamilyName) {
					fontDesc.familyName = attrFamilyName;
				}
				if (flagDefineFontSize) {
					fontDesc.size = attrFontSizeParsed;
				}
				if (flagDefineBold) {
					fontDesc.flagBold = attrBold;
				}
				if (flagDefineItalic) {
					fontDesc.flagItalic = attrItalic;
				}
				font = Font::create(fontDesc);
				styleNew->font = font;
			}
			if (flagDefineJoinedCharFamilyName) {
				styleNew->joinedCharFamilyName = attrJoinedCharFamilyName;
			}
			if (flagDefineUnderline) {
				styleNew->flagDefinedUnderline = sl_true;
				styleNew->flagUnderline = attrUnderline;
			}
			if (flagDefineOverline) {
				styleNew->flagOverline = attrOverline;
			}
			if (flagDefineLineThrough) {
				styleNew->flagLineThrough = attrLineThrough;
			}
			if (flagDefineTextColor) {
				styleNew->textColor = attrTextColor;
			}
			if (flagDefineBackColor) {
				styleNew->backgroundColor = attrBackColor;
			}
			if (flagDefineLink) {
				styleNew->flagLink = sl_true;
			}
			if (flagDefineHref) {
				styleNew->href = attrHref;
			}
			if (flagDefineYOffset) {
				styleNew->yOffset = attrYOffset;
			}
			if (flagDefineLineHeight) {
				styleNew->lineHeight = attrLineHeightParsed;
			}
		}

		if (name == StringView::literal("br")) {
			Ref<TextLineBreakItem> item = TextLineBreakItem::create(style);
			if (item.isNotNull()) {
				m_items.add_NoLock(Move(item));
			}
		} else if (name == StringView::literal("hr")) {
			Ref<TextHorizontalLineItem> item = TextHorizontalLineItem::create(style);
			if (item.isNotNull()) {
				m_items.add_NoLock(Move(item));
			}
		}
		addHyperTextNodeGroup(Ref<XmlNodeGroup>::from(element), styleNew);
	}

	void TextParagraph::addHyperText(const StringParam& text, const Ref<TextStyle>& style) noexcept
	{
		Xml::ParseParam param;
		param.flagLogError = sl_false;
		param.setCreatingOnlyElementsAndTexts();
		param.flagCreateWhiteSpaces = sl_true;
		param.flagCheckWellFormed = sl_false;
		Ref<XmlDocument> xml = Xml::parse(text, param);
		if (xml.isNotNull()) {
			addHyperTextNodeGroup(Ref<XmlNodeGroup>::from(xml), style);
		}
	}

	String TextParagraph::getPlainText()
	{
		StringBuffer buf;
		ObjectLocker lock(this);
		ListElements< Ref<TextItem> > items(m_items);
		for (sl_size i = 0; i < items.count; i++) {
			Ref<TextItem>& item = items[i];
			String text = item->getPlainText();
			if (text.isNotNull()) {
				buf.add(text);
			}
		}
		return buf.merge();
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(TextParagraph, LayoutParam)

	TextParagraph::LayoutParam::LayoutParam() noexcept:
		width(1),
		tabWidth(1), tabMargin(1),
		multiLineMode(MultiLineMode::Single),
		lineCount(0)
	{
	}

	namespace {

		class TextWordItemHelper : public TextWordItem
		{
		public:
			using TextWordItem::m_text;
		};

		class Layouter
		{
		public:
			CList< Ref<TextItem> >* m_layoutItems;
			sl_real m_layoutWidth;
			Alignment m_align;
			MultiLineMode m_multiLineMode;
			EllipsizeMode m_ellipsizeMode;
			sl_uint32 m_lineCount;
			sl_real m_tabMargin;
			sl_real m_tabWidth;

			sl_bool m_flagEnd;
			sl_real m_x;
			sl_real m_y;

			CList< Ref<TextItem> > m_lineItems;
			sl_real m_lineWidth;
			sl_real m_lineHeight;
			sl_uint32 m_lineNo;

			sl_real m_maxWidth;

		public:
			Layouter(CList< Ref<TextItem> >* layoutItems, const TextParagraph::LayoutParam& param) noexcept
			{
				m_layoutItems = layoutItems;
				m_layoutWidth = param.width;
				m_align = param.align;
				m_multiLineMode = param.multiLineMode;
				m_ellipsizeMode = param.ellipsisMode;
				m_lineCount = param.lineCount;
				m_tabWidth = param.tabWidth;
				m_tabMargin = param.tabMargin;

				m_flagEnd = sl_false;
				m_x = 0;
				m_y = 0;

				m_lineWidth = 0;
				m_lineHeight = 0;
				m_lineNo = 0;
				m_maxWidth = 0;
			}

			void endLine() noexcept
			{
				sl_size n = m_lineItems.getCount();
				if (!n) {
					return;
				}

				if (m_ellipsizeMode != EllipsizeMode::None && m_lineWidth > m_layoutWidth) {
					m_align = Alignment::Left;
				}
				sl_real x;
				if (m_align == Alignment::Left) {
					x = 0;
				} else if (m_align == Alignment::Right) {
					x = - m_lineWidth;
				} else {
					x = - m_lineWidth / 2;
				}

				sl_real bottom = m_y + m_lineHeight;

				Ref<TextItem>* p = m_lineItems.getData();
				for (sl_size i = 0; i < n; i++) {
					TextItem* item = p[i].get();
					Size size = item->getLayoutSize();
					Point pt(x, m_y + (m_lineHeight - size.y) / 2);
					item->setLayoutPosition(pt);
					x += size.x;
					TextItemType type = item->getType();
					if (type == TextItemType::Attach) {
						(static_cast<TextAttachItem*>(item))->setPosition(pt);
					}
				}

				m_lineNo++;
				if (m_ellipsizeMode != EllipsizeMode::None) {
					if ((m_lineWidth > m_layoutWidth && m_multiLineMode == MultiLineMode::Single) || (m_lineCount > 0 && m_lineNo >= m_lineCount && m_ellipsizeMode == EllipsizeMode::End)) {
						endEllipsize();
					}
				}

				p = m_lineItems.getData();
				n = m_lineItems.getCount();
				for (sl_size i = 0; i < n; i++) {
					TextItem* item = p[i].get();
					TextItemType type = item->getType();
					if (type == TextItemType::Word || type == TextItemType::Char || type == TextItemType::JoinedChar || type == TextItemType::Space || type == TextItemType::Tab || type == TextItemType::HorizontalLine) {
						m_layoutItems->add_NoLock(item);
					}
				}

				m_lineItems.removeAll_NoLock();
				if (m_lineWidth > m_maxWidth) {
					m_maxWidth = m_lineWidth;
				}
				m_x = 0;
				m_lineWidth = 0;
				m_y = bottom;
				m_lineHeight = 0;

				if (m_multiLineMode == MultiLineMode::Single) {
					m_flagEnd = sl_true;
				} else {
					if (m_lineCount > 0) {
						if (m_lineNo >= m_lineCount) {
							m_flagEnd = sl_true;
						}
					}
				}
			}

			void endEllipsize() noexcept
			{
				SLIB_STATIC_STRING16(strEllipsis, "...")
				CList< Ref<TextItem> >* listItems = &m_lineItems;
				sl_size nItems = listItems->getCount();
				Ref<TextItem>* items = listItems->getData();
				if (nItems < 1) {
					return;
				}
				Ref<TextStyle> style = items[nItems - 1]->getStyle();
				Ref<TextWordItem> itemEllipsis = TextWordItem::create(strEllipsis, style);
				if (itemEllipsis.isNull()) {
					return;
				}
				Size sizeEllipsis = itemEllipsis->getSize();
				if (m_layoutWidth < sizeEllipsis.x) {
					return;
				}
				sl_real xLimit = m_layoutWidth - sizeEllipsis.x;
				if (m_ellipsizeMode == EllipsizeMode::End) {
					for (sl_size i = 0; i < nItems; i++) {
						TextItem* item = items[i].get();
						Point pos = item->getLayoutPosition();
						if (pos.x + item->getLayoutSize().x > xLimit) {
							itemEllipsis->setLayoutPosition(pos);
							itemEllipsis->setLayoutSize(sizeEllipsis);
							if (item->getType() == TextItemType::Word) {
								String16 text = ((TextWordItem*)item)->getText();
								Ref<TextWordItem> word = TextWordItem::create(text, item->getStyle());
								if (word.isNotNull()) {
									sl_real widthLimit = xLimit - pos.x;
									sl_size n = text.getLength();
									sl_size k = n;
									Size size;
									for (; k > 0; k--) {
										((TextWordItemHelper*)(word.get()))->m_text = text.substring(0, k);
										size = word->getSize();
										if (size.x <= widthLimit) {
											break;
										}
									}
									listItems->setCount_NoLock(i);
									if (k > 0) {
										word->setLayoutPosition(item->getLayoutPosition());
										word->setLayoutSize(size);
										listItems->add_NoLock(word);
										pos.x += size.x;
										itemEllipsis->setLayoutPosition(pos);
									}
								} else {
									listItems->setCount_NoLock(i);
								}
							} else {
								listItems->setCount_NoLock(i);
							}
							listItems->add_NoLock(itemEllipsis);
							m_flagEnd = sl_true;
							return;
						}
					}
				} else if (m_ellipsizeMode == EllipsizeMode::Start) {
					for (sl_size i = 0; i < nItems; i++) {
						TextItem* item = items[nItems - 1 - i].get();
						Point pos = item->getLayoutPosition();
						pos.x = m_layoutWidth - m_lineWidth + pos.x;
						item->setLayoutPosition(pos);
						if (pos.x < sizeEllipsis.x) {
							if (i > 0) {
								itemEllipsis->setLayoutPosition(Point(items[nItems - i]->getLayoutPosition().x - sizeEllipsis.x, item->getLayoutPosition().y));
							} else {
								itemEllipsis->setLayoutPosition(Point(pos.x + item->getLayoutSize().x - sizeEllipsis.x, item->getLayoutPosition().y));
							}
							itemEllipsis->setLayoutSize(sizeEllipsis);
							if (item->getType() == TextItemType::Word) {
								String16 text = ((TextWordItem*)item)->getText();
								Ref<TextWordItem> word = TextWordItem::create(text, item->getStyle());
								if (word.isNotNull()) {
									sl_real widthWord = item->getLayoutSize().x;
									sl_real widthLimit = widthWord - (sizeEllipsis.x - pos.x);
									sl_size n = text.getLength();
									sl_size k = n;
									Size size;
									for (; k > 0; k--) {
										((TextWordItemHelper*)(word.get()))->m_text = text.substring(n - k, n);
										size = word->getSize();
										if (size.x <= widthLimit) {
											break;
										}
									}
									listItems->removeRange_NoLock(0, nItems - i);
									if (k > 0) {
										pos.x += widthWord - size.x;
										word->setLayoutPosition(pos);
										word->setLayoutSize(size);
										listItems->add_NoLock(word);
										pos.x -= sizeEllipsis.x;
										itemEllipsis->setLayoutPosition(pos);
									}
								} else {
									listItems->removeRange_NoLock(0, nItems - i);
								}
							} else {
								listItems->removeRange_NoLock(0, nItems - i);
							}
							listItems->insert_NoLock(0, itemEllipsis);
							m_flagEnd = sl_true;
							return;
						}
					}
				} else if (m_ellipsizeMode == EllipsizeMode::Middle) {
					itemEllipsis->setLayoutSize(sizeEllipsis);
					itemEllipsis->setLayoutPosition(Point(xLimit / 2, items[0]->getLayoutPosition().y));
					for (sl_size iMidStart = 0; iMidStart < nItems; iMidStart++) {
						TextItem* itemMidStart = items[iMidStart].get();
						Point pos = itemMidStart->getLayoutPosition();
						if (pos.x + itemMidStart->getLayoutSize().x > xLimit / 2) {
							Ref<TextWordItem> wordStart;
							if (itemMidStart->getType() == TextItemType::Word) {
								String16 text = ((TextWordItem*)itemMidStart)->getText();
								wordStart = TextWordItem::create(text, itemMidStart->getStyle());
								if (wordStart.isNotNull()) {
									sl_real widthLimit = xLimit / 2 - pos.x;
									sl_size n = text.getLength();
									sl_size k = n;
									Size size;
									for (; k > 0; k--) {
										((TextWordItemHelper*)(wordStart.get()))->m_text = text.substring(0, k);
										size = wordStart->getSize();
										if (size.x <= widthLimit) {
											break;
										}
									}
									if (k) {
										wordStart->setLayoutPosition(itemMidStart->getLayoutPosition());
										wordStart->setLayoutSize(size);
									} else {
										wordStart.setNull();
									}
								}
							}
							Ref<TextWordItem> wordEnd;
							sl_size iMidEnd = nItems - 1;
							while (iMidEnd >= iMidStart) {
								TextItem* itemMidEnd = items[iMidEnd].get();
								pos = itemMidEnd->getLayoutPosition();
								pos.x = m_layoutWidth - m_lineWidth + pos.x;
								itemMidEnd->setLayoutPosition(pos);
								if (pos.x < xLimit / 2 + sizeEllipsis.x) {
									if (itemMidEnd->getType() == TextItemType::Word) {
										String16 text = ((TextWordItem*)itemMidEnd)->getText();
										wordEnd = TextWordItem::create(text, itemMidEnd->getStyle());
										if (wordEnd.isNotNull()) {
											sl_real widthWord = itemMidEnd->getLayoutSize().x;
											sl_real widthLimit = widthWord - (xLimit / 2 + sizeEllipsis.x - pos.x);
											sl_size n = text.getLength();
											sl_size k = n;
											Size size;
											for (; k > 0; k--) {
												((TextWordItemHelper*)(wordEnd.get()))->m_text = text.substring(n - k, n);
												size = wordEnd->getSize();
												if (size.x <= widthLimit) {
													break;
												}
											}
											if (k > 0) {
												pos.x += widthWord - size.x;
												wordEnd->setLayoutPosition(pos);
												wordEnd->setLayoutSize(size);
											} else {
												wordEnd.setNull();
											}
										}
									}
									break;
								}
								if (iMidEnd == iMidStart) {
									break;
								} else {
									iMidEnd--;
								}
							}
							listItems->removeRange_NoLock(iMidStart, iMidEnd - iMidStart + 1);
							if (wordEnd.isNotNull()) {
								listItems->insert_NoLock(iMidStart, wordEnd);
							}
							listItems->insert_NoLock(iMidStart, itemEllipsis);
							if (wordStart.isNotNull()) {
								listItems->insert_NoLock(iMidStart, wordStart);
							}
							m_flagEnd = sl_true;
							return;
						}
					}
				}
			}

			void breakWord(TextWordItem* breakItem) noexcept
			{
				Ref<TextStyle> style = breakItem->getStyle();
				if (style.isNull()) {
					return;
				}

				Ref<Font> font = style->font;
				if (font.isNull()) {
					return;
				}

				String16 text = breakItem->getText();
				if (text.isEmpty()) {
					return;
				}

				Ref<FontAtlas> atlas = font->getSharedAtlas();
				if (atlas.isNull()) {
					return;
				}

				ObjectLocker lockAtlas(atlas.get());

				sl_char16* chars = text.getData();
				sl_size len = text.getLength();

				sl_real widthRemaining = m_layoutWidth - m_x;

				Size size = atlas->getFontSize_NoLock(chars[0]);
				sl_real x = size.x;
				sl_real height = size.y;
				sl_size startLine = 0;
				sl_size pos = 1;

				if (size.x > widthRemaining && m_x > 0) {
					endLine();
					widthRemaining = m_layoutWidth;
					if (m_flagEnd) {
						return;
					}
				}

				while (pos < len) {
					size = atlas->getFontSize_NoLock(chars[pos]);
					if (pos > startLine && x + size.x > widthRemaining) {
						Ref<TextItem> newItem = CreateWordOrCharItem(chars + startLine, pos - startLine, style);
						if (newItem.isNotNull()) {
							addLineItem(newItem.get(), Size(x, height));
						}
						startLine = pos;
						endLine();
						x = 0;
						height = 0;
						widthRemaining = m_layoutWidth;
						if (m_flagEnd) {
							return;
						}
					}
					x += size.x;
					if (size.y > height) {
						height = size.y;
					}
					pos++;
				}
				if (len > startLine) {
					Ref<TextItem> newItem = CreateWordOrCharItem(chars + startLine, len - startLine, style);
					if (newItem.isNotNull()) {
						addLineItem(newItem.get(), Size(x, height));
					}
				}
			}

			void processLineItem(TextItem* item, const Size& size) noexcept
			{
				item->setLayoutSize(size);
				TextItemType type = item->getType();
				if (m_x + size.x > m_layoutWidth) {
					sl_bool flagWrap = sl_false;
					sl_bool flagBreak = sl_false;
					if (m_multiLineMode == MultiLineMode::WordWrap) {
						flagWrap = sl_true;
					} else if (m_multiLineMode == MultiLineMode::BreakWord) {
						if (type == TextItemType::Word) {
							flagBreak = sl_true;
						} else {
							flagWrap = sl_true;
						}
					} else if (m_multiLineMode == MultiLineMode::LatinWrap) {
						if (type == TextItemType::Word && ((TextWordItem*)item)->containsNoLatin()) {
							flagBreak = sl_true;
						} else {
							flagWrap = sl_true;
						}
					}
					if (flagWrap) {
						if (m_lineItems.getCount()) {
							endLine();
							if (m_flagEnd) {
								return;
							}
							if (type == TextItemType::Word && size.x > m_layoutWidth) {
								breakWord((TextWordItem*)item);
								return;
							}
						} else {
							if (type == TextItemType::Word) {
								breakWord((TextWordItem*)item);
								return;
							}
						}
					} else if (flagBreak) {
						breakWord((TextWordItem*)item);
						return;
					}
				}
				applyLineHeight(item, size.y);
				m_lineItems.add_NoLock(item);
				m_x += size.x;
				m_lineWidth = m_x;
			}

			void addLineItem_SpaceTab(TextItem* item, const Size& size) noexcept
			{
				item->setLayoutSize(size);
				applyLineHeight(item, size.y);
				m_lineItems.add_NoLock(item);
				if (IsWrappingMultiLineMode(m_multiLineMode) && m_x + size.x > m_layoutWidth) {
					endLine();
				} else {
					m_x += size.x;
					m_lineWidth = m_x;
				}
			}

			void addLineItem(TextItem* item, const Size& size, sl_bool flagAdvancePosition = sl_true)
			{
				item->setLayoutSize(size);
				applyLineHeight(item, size.y);
				m_lineItems.add_NoLock(item);
				if (flagAdvancePosition) {
					m_x += size.x;
				}
				m_lineWidth = m_x;
			}

			void processWord(TextWordItem* item) noexcept
			{
				processLineItem(item, item->getSize());
			}

			void processChar(TextCharItem* item) noexcept
			{
				processLineItem(item, item->getSize());
			}

			void processJoinedChar(TextJoinedCharItem* item) noexcept
			{
				processLineItem(item, item->getSize());
			}

			void processSpace(TextSpaceItem* item) noexcept
			{
				addLineItem_SpaceTab(item, item->getSize());
			}

			void processTab(TextTabItem* item) noexcept
			{
				sl_real tabX = m_x + m_tabMargin;
				tabX = (Math::floor(tabX / m_tabWidth) + 1) * m_tabWidth;
				sl_real h = item->getHeight();
				addLineItem_SpaceTab(item, Size(tabX - m_x, h));
			}

			void processLineBreak(TextLineBreakItem* item) noexcept
			{
				sl_real h = item->getHeight();
				addLineItem(item, Size(h / 2, h), sl_false);
				endLine();
				item->setLayoutPosition(Point(m_x, m_y));
			}

			void processHorizontalLine(TextHorizontalLineItem* item) noexcept
			{
				endLine();
				sl_real h = item->getHeight();
				addLineItem(item, Size(h / 2, h), sl_false);
				item->setLayoutPosition(Point(m_x, m_y));
				endLine();
			}

			void processAttach(TextAttachItem* item) noexcept
			{
				processLineItem(item, item->getSize());
			}

			void applyLineHeight(TextItem* item, sl_real height)
			{
				sl_real lineHeight = height;
				Ref<TextStyle> style = item->getStyle();
				if (style.isNotNull()) {
					if (style->lineHeight >= 0) {
						lineHeight = style->lineHeight;
					}
				}
				if (lineHeight > m_lineHeight) {
					m_lineHeight = lineHeight;
				}
			}

			void layout(CList< Ref<TextItem> >* list) noexcept
			{
				sl_size n = list->getCount();
				Ref<TextItem>* items = list->getData();

				for (sl_size i = 0; i < n; i++) {

					TextItem* item = items[i].get();

					TextItemType type = item->getType();

					switch (type) {
						case TextItemType::Word:
							processWord(static_cast<TextWordItem*>(item));
							break;

						case TextItemType::Char:
							processChar(static_cast<TextCharItem*>(item));
							break;

						case TextItemType::JoinedChar:
							processJoinedChar(static_cast<TextJoinedCharItem*>(item));
							break;

						case TextItemType::Space:
							processSpace(static_cast<TextSpaceItem*>(item));
							break;

						case TextItemType::Tab:
							processTab(static_cast<TextTabItem*>(item));
							break;

						case TextItemType::LineBreak:
							processLineBreak(static_cast<TextLineBreakItem*>(item));
							break;

						case TextItemType::HorizontalLine:
							processHorizontalLine(static_cast<TextHorizontalLineItem*>(item));
							break;

						case TextItemType::Attach:
							processAttach(static_cast<TextAttachItem*>(item));
							break;
					}

					if (m_flagEnd) {
						break;
					}
				}

				m_lineWidth = m_x;
				endLine();

			}

		};
	}

	void TextParagraph::layout(const LayoutParam& param) noexcept
	{
		ObjectLocker lock(this);

		m_layoutItems.removeAll_NoLock();

		if (IsWrappingMultiLineMode(param.multiLineMode)) {
			if (param.width < SLIB_EPSILON) {
				return;
			}
		}

		Layouter layouter(&m_layoutItems, param);
		layouter.layout(&m_items);

		m_align = layouter.m_align;
		m_contentWidth = layouter.m_maxWidth;
		m_contentHeight = layouter.m_y;

	}
	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(TextParagraph, DrawParam)

	TextParagraph::DrawParam::DrawParam() noexcept
	{
	}

	void TextParagraph::draw(Canvas* canvas, sl_real left, sl_real right, sl_real y, const DrawParam& _param) noexcept
	{
		DrawParam param = _param;

		sl_real x;
		if (m_align == Alignment::Left) {
			x = left;
		} else if (m_align == Alignment::Right) {
			x = right;
		} else {
			x = (left + right) / 2;
		}

		Rectangle rc = canvas->getInvalidatedRect();
		rc.left -= x;
		rc.right -= x;
		rc.top -= y;
		rc.bottom -= y;

		ObjectLocker lock(this);

		ListElements< Ref<TextItem> > items(m_layoutItems);
		for (sl_size i = 0; i < items.count; i++) {
			TextItem* item = items[i].get();
			TextItemType type = item->getType();
			Ref<TextStyle> style = item->getStyle();
			if (style.isNotNull()) {
				if (style->textColor.isNotZero()) {
					param.textColor = style->textColor;
				} else {
					if (style->flagLink) {
						param.textColor = _param.linkColor;
						if (param.textColor.isZero()) {
							param.textColor = getDefaultLinkColor();
						}
					} else {
						param.textColor = _param.textColor;
					}
				}
				if (param.lineColor.isZero()) {
					param.lineColor = param.textColor;
				}
				if (type == TextItemType::Word || type == TextItemType::Char || type == TextItemType::JoinedChar) {
					Rectangle frame = item->getLayoutFrame();
					frame.top += style->yOffset;
					frame.bottom += style->yOffset;
					if (rc.intersectRectangle(frame)) {
						Ref<Font> font = style->font;
						if (font.isNotNull()) {
							Color backColor = style->backgroundColor;
							if (backColor.a > 0) {
								canvas->fillRectangle(Rectangle(x + frame.left, y + frame.top, x + frame.right, y + frame.bottom), backColor);
							}
							item->draw(canvas, x + frame.left, y + frame.top, param);
						}
					}
				} else if (type == TextItemType::HorizontalLine) {
					Rectangle frame = item->getLayoutFrame();
					Color backColor = style->backgroundColor;
					if (backColor.a > 0) {
						canvas->fillRectangle(Rectangle(left, y + frame.top, right, y + frame.bottom), backColor);
					}
					Ref<Pen> pen = Pen::createSolidPen(param.lineThickness, param.lineColor);
					if (pen.isNotNull()) {
						sl_real cy = frame.getCenterY();
						canvas->drawLine(left, y + cy, right, y + cy, pen);
					}
				}
				sl_bool flagUnderline = style->flagUnderline;
				if (!(style->flagDefinedUnderline) && style->flagLink) {
					flagUnderline = isDefaultLinkUnderline();
				}
				if (flagUnderline || style->flagOverline || style->flagLineThrough) {
					if (type == TextItemType::Word || type == TextItemType::Char || type == TextItemType::JoinedChar || type == TextItemType::Space || type == TextItemType::Tab) {
						Rectangle frame = item->getLayoutFrame();
						frame.top += style->yOffset;
						frame.bottom += style->yOffset;
						if (rc.intersectRectangle(frame)) {
							Ref<Font> font = style->font;
							if (font.isNotNull()) {
								Ref<Pen> pen = Pen::createSolidPen(param.lineThickness, param.lineColor);
								if (pen.isNotNull()) {
									FontMetrics fm;
									if (font->getFontMetrics(fm)) {
										if (flagUnderline) {
											sl_real yLine = y + frame.bottom - fm.descent / 2;
											canvas->drawLine(Point(x + frame.left, yLine), Point(x + frame.right, yLine), pen);
										}
										if (style->flagOverline) {
											sl_real yLine = y + frame.bottom - fm.descent - fm.ascent;
											canvas->drawLine(Point(x + frame.left, yLine), Point(x + frame.right, yLine), pen);
										}
										if (style->flagLineThrough) {
											sl_real yLine = y + frame.bottom - (fm.descent + fm.ascent) / 2;
											canvas->drawLine(Point(x + frame.left, yLine), Point(x + frame.right, yLine), pen);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	sl_real TextParagraph::getContentWidth() noexcept
	{
		return m_contentWidth;
	}

	sl_real TextParagraph::getContentHeight() noexcept
	{
		return m_contentHeight;
	}

	Ref<TextItem> TextParagraph::getTextItemAtLocation(sl_real x, sl_real y, sl_real left, sl_real right) noexcept
	{
		sl_real startX;
		if (m_align == Alignment::Left) {
			startX = left;
		} else if (m_align == Alignment::Right) {
			startX = right;
		} else {
			startX = (left + right) / 2;
		}
		x -= startX;
		ListElements< Ref<TextItem> > items(m_layoutItems);
		for (sl_size i = 0; i < items.count; i++) {
			TextItem* item = items[i].get();
			Ref<TextStyle> style = item->getStyle();
			if (style.isNotNull()) {
				Rectangle frame = item->getLayoutFrame();
				frame.top += style->yOffset;
				frame.bottom += style->yOffset;
				if (frame.containsPoint(x, y)) {
					return item;
				}
			}
		}
		return sl_null;
	}

	sl_text_pos TextParagraph::getEndPosition() noexcept
	{
		sl_text_pos n = 0;
		ObjectLocker lock(this);
		ListElements< Ref<TextItem> > items(m_items);
		for (sl_size i = 0; i < items.count; i++) {
			TextItem* item = items[i].get();
			if (item->getType() == TextItemType::Word) {
				n += (sl_text_pos)(((TextWordItem*)item)->getText().getLength());
			} else {
				n++;
			}
		}
		return n;
	}

	Alignment TextParagraph::getAlignment() noexcept
	{
		return m_align;
	}

	namespace {
		static Color g_defaultLinkColor = Color::Blue;
		static sl_bool g_defaultLinkUnderline = sl_true;
	}

	const Color& TextParagraph::getDefaultLinkColor()
	{
		return g_defaultLinkColor;
	}

	void TextParagraph::setDefaultLinkColor(const Color& color)
	{
		g_defaultLinkColor = color;
	}

	sl_bool TextParagraph::isDefaultLinkUnderline()
	{
		return g_defaultLinkUnderline;
	}

	void TextParagraph::setDefaultLinkUnderline(sl_bool flag)
	{
		g_defaultLinkUnderline = flag;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(TextBoxParam)

	TextBoxParam::TextBoxParam() noexcept:
		flagHyperText(sl_false),
		flagMnemonic(sl_false),
		width(0),
		multiLineMode(MultiLineMode::WordWrap),
		ellipsizeMode(EllipsizeMode::None),
		lineCount(0),
		align(Alignment::TopLeft),
		flagEnabledHyperlinksInPlainText(sl_false)
	{
	}


	SLIB_DEFINE_OBJECT(TextBox, Object)

	TextBox::TextBox() noexcept
	{
		m_flagHyperText = sl_false;
		m_width = 0;
		m_multiLineMode = MultiLineMode::Single;
		m_alignHorizontal = Alignment::Left;
		m_alignVertical = Alignment::Top;

		m_contentWidth = 0;
		m_contentHeight = 0;

		m_style = new TextStyle;
	}

	TextBox::~TextBox() noexcept
	{
	}

	void TextBox::update(const TextBoxParam& param) noexcept
	{
		ObjectLocker lock(this);

		Ref<Font> font = param.font;
		if (font.isNull()) {
			return;
		}

		FontDesc fd;
		font->getDesc(fd);
		if (fd.flagUnderline || fd.flagStrikeout) {
			if (fd.flagUnderline) {
				m_style->flagUnderline = sl_true;
				m_style->flagDefinedUnderline = sl_true;
			}
			if (fd.flagStrikeout) {
				m_style->flagLineThrough = sl_true;
			}
			fd.flagStrikeout = sl_false;
			fd.flagUnderline = sl_false;
			font = Font::create(fd);
		}
		sl_bool flagChangedFont = sl_false;
		if (m_font.isNotNull()) {
			FontDesc fdOld;
			m_font->getDesc(fdOld);
			if (fdOld.familyName != fd.familyName || fdOld.size != fd.size || fdOld.flagBold != fd.flagBold || fdOld.flagItalic != fd.flagItalic) {
				flagChangedFont = sl_true;
			}
		} else {
			flagChangedFont = sl_true;
		}

		m_style->font = font;

		sl_real width = param.width;
		if (width < SLIB_EPSILON) {
			width = 0;
		}
		MultiLineMode multiLineMode = param.multiLineMode;
		EllipsizeMode ellipsizeMode = param.ellipsizeMode;
		sl_uint32 lineCount = param.lineCount;

		if (multiLineMode != MultiLineMode::Single) {
			if (lineCount == 0) {
				ellipsizeMode = EllipsizeMode::None;
			} else {
				if (ellipsizeMode != EllipsizeMode::End) {
					ellipsizeMode = EllipsizeMode::None;
				}
			}
		}

		m_alignVertical = param.align & Alignment::VerticalMask;
		Alignment alignHorizontal = param.align & Alignment::HorizontalMask;
		if (width < SLIB_EPSILON) {
			if (multiLineMode != MultiLineMode::Single) {
				multiLineMode = MultiLineMode::Multiple;
			}
			ellipsizeMode = EllipsizeMode::None;
			width = 0;
		} else {
			if ((multiLineMode == MultiLineMode::Single || multiLineMode == MultiLineMode::Multiple) && ellipsizeMode == EllipsizeMode::None) {
				width = 0;
			}
		}

		sl_bool flagReLayout = sl_false;
		if (m_text != param.text || m_flagHyperText != param.flagHyperText || (param.flagHyperText && flagChangedFont) || (!(param.flagHyperText) && param.flagEnabledHyperlinksInPlainText)) {
			m_paragraph.setNull();
			m_contentWidth = 0;
			m_contentHeight = 0;
			if (param.text.isNotEmpty()) {
				m_paragraph = new TextParagraph;
				if (param.flagHyperText) {
					m_font = font;
					m_paragraph->addHyperText(param.text, m_style);
				} else {
					m_paragraph->addText(param.text, m_style, param.flagEnabledHyperlinksInPlainText, param.flagMnemonic);
				}
			}
			m_text = param.text;
			m_flagHyperText = param.flagHyperText;
			flagReLayout = sl_true;
		}
		if (param.text.isEmpty()) {
			return;
		}
		if (m_paragraph.isNotNull()) {
			if (flagChangedFont || !(Math::isAlmostZero(m_width - width)) || m_multiLineMode != multiLineMode || m_ellipsisMode != ellipsizeMode || m_lineCount != lineCount || m_alignHorizontal != alignHorizontal) {
				flagReLayout = sl_true;
			}
			if (flagReLayout) {
				TextParagraph::LayoutParam paramParagraph;
				paramParagraph.width = width;
				paramParagraph.tabWidth = font->getFontHeight() * 2;
				paramParagraph.tabMargin = paramParagraph.tabWidth / 4;
				paramParagraph.multiLineMode = multiLineMode;
				paramParagraph.ellipsisMode = ellipsizeMode;
				paramParagraph.lineCount = lineCount;
				paramParagraph.align = alignHorizontal;
				m_paragraph->layout(paramParagraph);

				m_font = font;
				m_width = width;
				m_multiLineMode = multiLineMode;
				m_ellipsisMode = ellipsizeMode;
				m_alignHorizontal = alignHorizontal;
				m_lineCount = lineCount;

				m_contentWidth = m_paragraph->getContentWidth();
				m_contentHeight = m_paragraph->getContentHeight();
			}
		}
	}
	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(TextBox, DrawParam)

	TextBox::DrawParam::DrawParam() noexcept
	{
	}

	void TextBox::draw(Canvas* canvas, const DrawParam& param) const noexcept
	{
		if (param.textColor.a == 0) {
			return;
		}
		sl_real widthDraw = param.frame.getWidth();
		if (widthDraw < SLIB_EPSILON) {
			return;
		}
		sl_real heightDraw = param.frame.getHeight();
		if (heightDraw < SLIB_EPSILON) {
			return;
		}
		ObjectLocker lock(this);
		if (m_paragraph.isNotNull()) {
			sl_real height = m_paragraph->getContentHeight();
			sl_real y;
			if (m_alignVertical == Alignment::Left) {
				y = param.frame.top;
			} else if (m_alignVertical == Alignment::Bottom) {
				y = param.frame.bottom - height;
			} else {
				y = (param.frame.top + param.frame.bottom - height) / 2;
			}
			m_paragraph->draw(canvas, param.frame.left, param.frame.right, y, param);
		}
	}

	sl_real TextBox::getContentWidth() const noexcept
	{
		return m_contentWidth;
	}

	sl_real TextBox::getContentHeight() const noexcept
	{
		return m_contentHeight;
	}

	Ref<TextItem> TextBox::getTextItemAtLocation(sl_real x, sl_real y, const Rectangle& frame) const noexcept
	{
		ObjectLocker lock(this);
		if (m_paragraph.isNotNull()) {
			sl_real height = m_paragraph->getContentHeight();
			sl_real startY;
			if (m_alignVertical == Alignment::Top) {
				startY = frame.top;
			} else if (m_alignVertical == Alignment::Bottom) {
				startY = frame.bottom - height;
			} else {
				startY = (frame.top + frame.bottom - height) / 2;
			}
			return m_paragraph->getTextItemAtLocation(x, y - startY, frame.left, frame.right);
		}
		return sl_null;
	}

	sl_size TextBox::getEndPosition() const noexcept
	{
		ObjectLocker lock(this);
		if (m_paragraph.isNotNull()) {
			return m_paragraph->getEndPosition();
		}
		return 0;
	}

	Ref<Font> TextBox::getFont() const noexcept
	{
		ObjectLocker lock(this);
		return m_font;
	}

	String TextBox::getText() const noexcept
	{
		ObjectLocker lock(this);
		return m_text;
	}

	String TextBox::getPlainText() const noexcept
	{
		ObjectLocker lock(this);
		if (m_paragraph.isNotNull()) {
			return m_paragraph->getPlainText();
		} else {
			return sl_null;
		}
	}

	MultiLineMode TextBox::getMultiLineMode() const noexcept
	{
		return m_multiLineMode;
	}

	EllipsizeMode TextBox::getEllipsizeMode() const noexcept
	{
		return m_ellipsisMode;
	}

	Alignment TextBox::getAlignment() const noexcept
	{
		return m_alignVertical | m_alignHorizontal;
	}

}
