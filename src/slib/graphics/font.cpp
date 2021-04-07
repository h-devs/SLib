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

#include "slib/graphics/font.h"

#include "slib/graphics/font_atlas.h"
#include "slib/core/memory.h"
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

	Ref<Font> Font::create(String familyName, sl_real size, sl_bool flagBold, sl_bool flagItalic, sl_bool flagUnderline, sl_bool flagStrikeout)
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
		if (g_defaultFamily == fontFamily) {
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

	sl_bool Font::addFontResource(const void* data, sl_size size)
	{
		return _addFontResource(data, size, sl_null);
	}

	sl_bool Font::addFontResource(const Memory& mem)
	{
		return _addFontResource(mem.getData(), mem.getSize(), sl_null);
	}

	Ref<Referable> Font::addFontResourceRef(const void* data, sl_size size)
	{
		Ref<Referable> ref;
		if (_addFontResource(data, size, &ref)) {
			return ref;
		}
		return sl_null;
	}

	Ref<Referable> Font::addFontResourceRef(const Memory& data)
	{
		Ref<Referable> ref;
		if (_addFontResource(data.getData(), data.getSize(), &ref)) {
			return ref;
		}
		return sl_null;
	}


#if !defined(SLIB_GRAPHICS_IS_GDI)
	List<String> Font::getAllFamilyNames()
	{
		return sl_null;
	}
#endif

#if !defined(SLIB_GRAPHICS_IS_GDI)
	sl_bool Font::_addFontResource(const void* data, sl_size size, Ref<Referable>* pRef)
	{
		return sl_false;
	}
#endif

}
