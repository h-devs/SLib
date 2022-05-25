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

#include "slib/graphics/font.h"
#include "slib/graphics/truetype.h"

#include "slib/graphics/font_atlas.h"
#include "slib/core/memory.h"
#include "slib/core/mio.h"
#include "slib/core/locale.h"
#include "slib/core/safe_static.h"

namespace slib
{

	namespace priv
	{
		namespace font
		{
			
			SLIB_GLOBAL_ZERO_INITIALIZED(SpinLock, g_lockDefaultFont)
			SLIB_GLOBAL_ZERO_INITIALIZED(Ref<Font>, g_defaultFont)
			
			SLIB_GLOBAL_ZERO_INITIALIZED(AtomicString, g_defaultFamily)
			sl_real g_defaultSize = 12;

			static String GetSystemDefaultFontFamily()
			{
#ifdef SLIB_PLATFORM_IS_WIN32
				SLIB_STATIC_STRING(tohoma, "Tahoma")
				SLIB_STATIC_STRING(cambria, "Cambria")
				auto fonts = Font::getAllFamilyNames();
				if (fonts.contains(tohoma)) {
					return tohoma;
				}
				if (fonts.contains(cambria)) {
					return cambria;
				}
				SLIB_STATIC_STRING(s, "System")
				return s;
#else
				SLIB_STATIC_STRING(s, "Arial")
				return s;
#endif
			}

		}
	}

	using namespace priv::font;

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FontDesc)
	
	FontDesc::FontDesc()
	{
		size = g_defaultSize;
		flagBold = sl_false;
		flagItalic = sl_false;
		flagUnderline = sl_false;
		flagStrikeout = sl_false;
	}


	SLIB_DEFINE_ROOT_OBJECT(Font)

	Font::Font()
	{
		m_flagMetricsCache = sl_false;
	}

	Font::~Font()
	{
	}

	Ref<Font> Font::create(const FontDesc &desc)
	{
		Ref<Font> ret = new Font;
		if (ret.isNotNull()) {
			if (desc.familyName.isNotEmpty()) {
				ret->m_desc.familyName = desc.familyName;
			} else {
				ret->m_desc.familyName = getDefaultFontFamily();
			}
			ret->m_desc.size = SLIB_FONT_SIZE_PRECISION_APPLY(desc.size);
			ret->m_desc.flagBold = desc.flagBold;
			ret->m_desc.flagItalic = desc.flagItalic;
			ret->m_desc.flagUnderline = desc.flagUnderline;
			ret->m_desc.flagStrikeout = desc.flagStrikeout;
			return ret;
		}
		return sl_null;
	}

	Ref<Font> Font::create(const String& familyName, sl_real size, sl_bool flagBold, sl_bool flagItalic, sl_bool flagUnderline, sl_bool flagStrikeout)
	{
		Ref<Font> ret = new Font;
		if (ret.isNotNull()) {
			if (familyName.isNotEmpty()) {
				ret->m_desc.familyName = familyName;
			} else {
				ret->m_desc.familyName = getDefaultFontFamily();
			}
			ret->m_desc.size = SLIB_FONT_SIZE_PRECISION_APPLY(size);
			ret->m_desc.flagBold = flagBold;
			ret->m_desc.flagItalic = flagItalic;
			ret->m_desc.flagUnderline = flagUnderline;
			ret->m_desc.flagStrikeout = flagStrikeout;
			return ret;
		}
		return sl_null;
	}

	Ref<Font> Font::getDefault()
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_defaultFont)) {
			return sl_null;
		}
		if (g_defaultFont.isNotNull()) {
			SpinLocker lock(&g_lockDefaultFont);
			return g_defaultFont;
		} else {
			Ref<Font> font = create(FontDesc());
			if (font.isNotNull()) {
				SpinLocker lock(&g_lockDefaultFont);
				g_defaultFont = font;
				return font;
			}
		}
		return sl_null;
	}

	void Font::setDefault(const Ref<Font>& font)
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_defaultFont)) {
			return;
		}
		if (font.isNotNull()) {
			g_defaultFamily = font->getFamilyName();
			g_defaultSize = font->getSize();
			SpinLocker lock(&g_lockDefaultFont);
			g_defaultFont = font;
		} else {
			Ref<Font> fontNew = create(FontDesc());
			if (fontNew.isNotNull()) {
				SpinLocker lock(&g_lockDefaultFont);
				g_defaultFont = fontNew;
			}
		}
	}

	sl_real Font::getDefaultFontSize()
	{
		return g_defaultSize;
	}

	void Font::setDefaultFontSize(sl_real size)
	{
		if (size < 0) {
			size = 0;
		}
		size = SLIB_FONT_SIZE_PRECISION_APPLY(size);
		if (g_defaultSize == size) {
			return;
		}
		g_defaultSize = size;

		if (SLIB_SAFE_STATIC_CHECK_FREED(g_defaultFont)) {
			return;
		}
		SpinLocker lock(&g_lockDefaultFont);
		if (g_defaultFont.isNotNull()) {
			FontDesc desc;
			g_defaultFont->getDesc(desc);
			desc.size = size;
			Ref<Font> fontNew = create(desc);
			if (fontNew.isNotNull()) {
				g_defaultFont = fontNew;
			}
		}
	}

	String Font::getDefaultFontFamily()
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_defaultFamily)) {
			return sl_null;
		}
		if (g_defaultFamily.isNotNull()) {
			return g_defaultFamily;
		} else {
			String name = GetSystemDefaultFontFamily();
			if (name.isNotEmpty()) {
				g_defaultFamily = name;
				return name;
			}
		}
		return sl_null;
	}

	void Font::setDefaultFontFamily(const String& _fontFamily)
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_defaultFamily)) {
			return;
		}
		String fontFamily = _fontFamily;
		if (fontFamily.isEmpty()) {
			fontFamily = GetSystemDefaultFontFamily();
			if (fontFamily.isEmpty()) {
				return;
			}
		}
		if (String(g_defaultFamily) == fontFamily) {
			return;
		}
		g_defaultFamily = fontFamily;

		if (SLIB_SAFE_STATIC_CHECK_FREED(g_defaultFont)) {
			return;
		}
		SpinLocker lock(&g_lockDefaultFont);
		if (g_defaultFont.isNotNull()) {
			FontDesc desc;
			g_defaultFont->getDesc(desc);
			desc.familyName = fontFamily;
			Ref<Font> fontNew = create(desc);
			if (fontNew.isNotNull()) {
				g_defaultFont = fontNew;
			}
		}
	}

	String Font::getDefaultFontFamilyForLocale(const Locale& locale)
	{
		Language lang = locale.getLanguage();
		if (lang == Language::Korean) {
			if (locale.getCountry() == Country::NorthKorea) {
				SLIB_STATIC_STRING(s1, "KP CheonRiMa")
				SLIB_STATIC_STRING(s2, "PRK P Gothic")
				SLIB_STATIC_STRING(s3, "\xec\xb2\x9c\xeb\xa6\xac\xeb\xa7\x88")
				auto fonts = Font::getAllFamilyNames();
				if (fonts.contains_NoLock(s1)) {
					return s1;
				}
				if (fonts.contains_NoLock(s2)) {
					return s2;
				}
				if (fonts.contains_NoLock(s3)) {
					return s3;
				}
			} else {
				SLIB_STATIC_STRING(s, "Dotum")
				return s;
			}
		}
		return GetSystemDefaultFontFamily();
	}

	void Font::setDefaultFontFamilyForLocale(const Locale& locale)
	{
		setDefaultFontFamily(getDefaultFontFamilyForLocale(locale));
	}

	void Font::getDesc(FontDesc& desc)
	{
		desc = m_desc;
	}

	String Font::getFamilyName()
	{
		return m_desc.familyName;
	}

	sl_real Font::getSize()
	{
		return m_desc.size;
	}

	sl_bool Font::isBold()
	{
		return m_desc.flagBold;
	}

	sl_bool Font::isItalic()
	{
		return m_desc.flagItalic;
	}

	sl_bool Font::isUnderline()
	{
		return m_desc.flagUnderline;
	}
	
	sl_bool Font::isStrikeout()
	{
		return m_desc.flagStrikeout;
	}

	sl_bool Font::getFontMetrics(FontMetrics& _out)
	{
		if (m_flagMetricsCache) {
			_out = m_metricsCache;
			return sl_true;
		}
		if (_getFontMetrics_PO(m_metricsCache)) {
			m_flagMetricsCache = sl_true;
			_out = m_metricsCache;
			return sl_true;
		}
		return 0;
	}

	sl_real Font::getFontHeight()
	{
		FontMetrics fm;
		if (getFontMetrics(fm)) {
			return fm.leading + fm.ascent + fm.descent;
		}
		return 0;
	}

	sl_real Font::getFontAscent()
	{
		FontMetrics fm;
		if (getFontMetrics(fm)) {
			return fm.ascent;
		}
		return 0;
	}

	sl_real Font::getFontDescent()
	{
		FontMetrics fm;
		if (getFontMetrics(fm)) {
			return fm.descent;
		}
		return 0;
	}

	Size Font::measureText(const StringParam& text)
	{
		return _measureText_PO(text);
	}

	Size Font::measureText(const StringParam& _text, sl_bool flagMultiLine)
	{
		if (!flagMultiLine) {
			return _measureText_PO(_text);
		}
		StringData16 text(_text);
		sl_char16* sz = text.getData();
		sl_size len = text.getLength();
		sl_size startLine = 0;
		sl_size pos = 0;
		sl_real width = 0;
		sl_real height = 0;
		while (pos <= len) {
			sl_char16 ch;
			if (pos < len) {
				ch = sz[pos];
			} else {
				ch = '\n';
			}
			if (ch == '\r' || ch == '\n') {
				if (pos > startLine) {
					Size size = _measureText_PO(String16(sz + startLine, pos - startLine));
					if (size.x > width) {
						width = size.x;
					}
					height += size.y;
				}
				if (ch == '\r' && pos + 1 < len) {
					if (sz[pos + 1] == '\n') {
						pos++;
					}
				}
				startLine = pos + 1;
			}
			pos++;
		}
		return Size(width, height);
	}

	Ref<Referable> Font::getPlatformObject()
	{
		return m_platformObject;
	}

#if !defined(SLIB_GRAPHICS_IS_GDI) && !defined(SLIB_GRAPHICS_IS_QUARTZ)
	List<String> Font::getAllFamilyNames()
	{
		return sl_null;
	}
#endif


	SLIB_DEFINE_ROOT_OBJECT(EmbeddedFont)

	EmbeddedFont::EmbeddedFont()
	{
	}

	EmbeddedFont::~EmbeddedFont()
	{
	}

	Ref<EmbeddedFont> EmbeddedFont::load(const Memory& content)
	{
		return load(content.getData(), content.getSize());
	}

#if !defined(SLIB_GRAPHICS_IS_GDI)
	Ref<EmbeddedFont> EmbeddedFont::load(const void* content, sl_size size)
	{
		return sl_null;
	}
#endif


	List<String> Truetype::getNames(const void* _content, sl_size size, TruetypeNameId _id)
	{
		sl_uint8* content = (sl_uint8*)_content;
		struct TTF_HEADER
		{
			sl_uint8 version[2];
			sl_uint8 numTables[4];
			sl_uint8 searchRange[2];
			sl_uint8 entrySelector[2];
			sl_uint8 rangeShift[2];
		};
		struct TTF_OFFSET_TABLE
		{
			char name[4];
			sl_uint8 checksum[4];
			sl_uint8 offset[4];
			sl_uint8 length[4];
		};
		struct TTF_NAME_TABLE_HEADER
		{
			sl_uint8 format[2];
			sl_uint8 count[2];
			sl_uint8 stringOffset[2];
		};
		struct TTF_NAME_TABLE_ENTRY
		{
			sl_uint8 platformId[2];
			sl_uint8 encodingId[2];
			sl_uint8 languageId[2];
			sl_uint8 nameId[2];
			sl_uint8 length[2];
			sl_uint8 offset[2];
		};
		TTF_HEADER* header = (TTF_HEADER*)content;
		if (size < sizeof(TTF_HEADER)) {
			return sl_null;
		}
		sl_uint8* offsetTables = content + sizeof(TTF_HEADER);
		sl_uint32 numTables = MIO::readUint32BE(header->numTables);
		if (size < sizeof(TTF_HEADER) + sizeof(TTF_OFFSET_TABLE) * numTables) {
			return sl_null;
		}
		List<String> ret;
		for (sl_uint32 i = 0; i < numTables; i++) {
			TTF_OFFSET_TABLE* offsetTable = (TTF_OFFSET_TABLE*)(offsetTables + sizeof(TTF_OFFSET_TABLE) * i);
			if (Base::equalsMemory(offsetTable->name, "name", 4)) {
				sl_uint32 offset = MIO::readUint32BE(offsetTable->offset);
				if (offset + sizeof(TTF_NAME_TABLE_HEADER) <= size) {
					TTF_NAME_TABLE_HEADER* nameHeader = (TTF_NAME_TABLE_HEADER*)(content + offset);
					sl_uint8* entries = content + offset + sizeof(TTF_NAME_TABLE_HEADER);
					sl_uint16 n = MIO::readUint16BE(nameHeader->count);
					if (offset + sizeof(TTF_NAME_TABLE_HEADER) + sizeof(TTF_NAME_TABLE_ENTRY) * n <= size) {
						for (sl_uint16 i = 0; i < n; i++) {
							TTF_NAME_TABLE_ENTRY* entry = (TTF_NAME_TABLE_ENTRY*)(entries + sizeof(TTF_NAME_TABLE_ENTRY) * i);
							sl_uint16 nameId = MIO::readUint16BE(entry->nameId);
							if (nameId == (sl_uint16)_id) {
								sl_uint16 platformId = MIO::readUint16BE(entry->platformId);
								sl_uint16 encodingId = MIO::readUint16BE(entry->encodingId);
								sl_bool flagUtf16 = sl_false;
								switch (platformId) {
								case 0: // APPLE_UNICODE
								case 2: // ISO
									flagUtf16 = sl_true;
									break;
								case 1: // MACINTOSH
									break;
								case 3: // MICROSOFT
									switch (encodingId) {
									case 0: //SYMBOL
									case 1: // UNICODE
									case 7: // UCS4
										flagUtf16 = sl_true;
										break;
									case 2: // SJIS
									case 3: // PRC
									case 4: // BIG5
									case 5: // WANSUNG
									case 6: // JOHAB
									default:
										break;
									}
									break;
								default:
									break;
								}
								sl_uint32 len = (sl_uint32)(MIO::readUint16BE(entry->length));
								sl_uint32 offsetString = offset + MIO::readUint16BE(nameHeader->stringOffset) + MIO::readUint16BE(entry->offset);
								if (offsetString + len <= size) {
									if (flagUtf16) {
										ret.add_NoLock(String::fromUtf16BE(content + offsetString, len));
									} else {
										ret.add_NoLock(String::fromUtf8(content + offsetString, len));
									}
								}
							}
						}
					}
				}
			}
		}
		return ret;
	}

}
