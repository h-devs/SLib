/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

	namespace
	{
		SLIB_GLOBAL_ZERO_INITIALIZED(SpinLock, g_lockDefaultFont)
		SLIB_GLOBAL_ZERO_INITIALIZED(Ref<Font>, g_defaultFont)

		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicString, g_defaultFamily)
		static sl_real g_defaultSize = 12;

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

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FontDesc)

	FontDesc::FontDesc(): size(-1)
	{
		flags = 0;
	}

	FontDesc::FontDesc(const String& _familyName, sl_real _size, sl_bool _flagBold, sl_bool _flagItalic, sl_bool _flagUnderline, sl_bool _flagStrikeout): familyName(_familyName), size(_size)
	{
		flags = 0;
		flagBold = _flagBold;
		flagItalic = _flagItalic;
		flagUnderline = _flagUnderline;
		flagStrikeout = _flagStrikeout;
	}

	FontDesc::FontDesc(const String& _familyName, sl_real _size, sl_int32 _flags): familyName(_familyName), size(_size)
	{
		flags = _flags;
	}

	void TextMetrics::setZero() noexcept
	{
		left = 0.0f;
		top = 0.0f;
		right = 0.0f;
		bottom = 0.0f;
		advanceX = 0.0f;
		advanceY = 0.0f;
	}

	void TextMetrics::setBlank(sl_real _advanceX, sl_real _advanceY)
	{
		left = 0.0f;
		top = 0.0f;
		right = 0.0f;
		bottom = 0.0f;
		advanceX = _advanceX;
		advanceY = _advanceY;
	}


	SLIB_DEFINE_ROOT_OBJECT(Font)

	Font::Font()
	{
		m_flagMetricsCache = sl_false;
	}

	Font::~Font()
	{
	}

	Ref<Font> Font::create(const FontDesc& src)
	{
		Ref<Font> ret = new Font;
		if (ret.isNotNull()) {
			FontDesc& dst = ret->m_desc;
			if (src.familyName.isNotEmpty()) {
				dst.familyName = src.familyName;
			} else {
				dst.familyName = getDefaultFontFamily();
			}
			dst.size = SLIB_FONT_SIZE_PRECISION_APPLY(src.size >= 0 ? src.size : g_defaultSize);
			if (src.flags >= 0) {
				dst.flags = src.flags;
			}
			return ret;
		}
		return sl_null;
	}

	Ref<Font> Font::create(const String& familyName, sl_real size, sl_bool flagBold, sl_bool flagItalic, sl_bool flagUnderline, sl_bool flagStrikeout)
	{
		return create(FontDesc(familyName, size, flagBold, flagItalic, flagUnderline, flagStrikeout));
	}

	Ref<Font> Font::create(const String& familyName, sl_real size, const Ref<Font>& original)
	{
		if (original.isNull()) {
			return create(familyName, size, getDefault());
		}
		if ((familyName.isEmpty() || familyName == original->getFamilyName()) && (size < 0.0f || size == original->getSize())) {
			return original;
		}
		Ref<Font> ret = new Font;
		if (ret.isNotNull()) {
			FontDesc& dst = ret->m_desc;
			original->getDesc(dst);
			if (familyName.isNotEmpty()) {
				dst.familyName = familyName;
			}
			if (size >= 0.0f) {
				dst.size = size;
			}
		}
		return ret;
	}

	Ref<Font> Font::create(const String& familyName, const Ref<Font>& original)
	{
		if (original.isNull()) {
			return create(familyName, getDefault());
		}
		if (familyName.isEmpty() || familyName == original->getFamilyName()) {
			return original;
		}
		Ref<Font> ret = new Font;
		if (ret.isNotNull()) {
			FontDesc& dst = ret->m_desc;
			original->getDesc(dst);
			dst.familyName = familyName;
		}
		return ret;
	}

	Ref<Font> Font::create(sl_real size, const Ref<Font>& original)
	{
		if (original.isNull()) {
			return create(size, getDefault());
		}
		if (size < 0.0f || size == original->getSize()) {
			return original;
		}
		Ref<Font> ret = new Font;
		if (ret.isNotNull()) {
			FontDesc& dst = ret->m_desc;
			original->getDesc(dst);
			dst.size = size;
		}
		return ret;
	}

	Ref<Font> Font::create(const FontDesc& desc, const Ref<Font>& original)
	{
		if (original.isNull()) {
			return create(desc, getDefault());
		}
		if ((desc.familyName.isEmpty() || desc.familyName == original->getFamilyName()) && (desc.size < 0.0f || desc.size == original->getSize()) && (desc.flags < 0 || desc.flags == original->getFlags())) {
			return original;
		}
		Ref<Font> ret = new Font;
		if (ret.isNotNull()) {
			FontDesc& dst = ret->m_desc;
			original->getDesc(dst);
			if (desc.familyName.isNotEmpty()) {
				dst.familyName = desc.familyName;
			}
			if (desc.size >= 0.0f) {
				dst.size = desc.size;
			}
			if (desc.flags >= 0) {
				dst.flags = desc.flags;
			}
		}
		return ret;
	}

	Ref<Font> Font::createBold(const Ref<Font>& original)
	{
		if (original.isNull()) {
			return createBold(getDefault());
		}
		if (original->isBold()) {
			return original;
		}
		Ref<Font> ret = new Font;
		if (ret.isNotNull()) {
			FontDesc& dst = ret->m_desc;
			original->getDesc(dst);
			dst.flagBold = sl_true;
		}
		return ret;
	}

	Ref<Font> Font::createItalic(const Ref<Font>& original)
	{
		if (original.isNull()) {
			return createItalic(getDefault());
		}
		if (original->isItalic()) {
			return original;
		}
		Ref<Font> ret = new Font;
		if (ret.isNotNull()) {
			FontDesc& dst = ret->m_desc;
			original->getDesc(dst);
			dst.flagItalic = sl_true;
		}
		return ret;
	}

	Ref<Font> Font::createUnderline(const Ref<Font>& original)
	{
		if (original.isNull()) {
			return createUnderline(getDefault());
		}
		if (original->isUnderline()) {
			return original;
		}
		Ref<Font> ret = new Font;
		if (ret.isNotNull()) {
			FontDesc& dst = ret->m_desc;
			original->getDesc(dst);
			dst.flagUnderline = sl_true;
		}
		return ret;
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
			if (locale.getCountry() == Country::KP) {
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

	sl_uint32 Font::getFlags()
	{
		return m_desc.flags;
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
		return sl_false;
	}

	sl_real Font::getFontHeight()
	{
		FontMetrics fm;
		if (getFontMetrics(fm)) {
			return fm.leading + fm.ascent + fm.descent;
		}
		return 0.0f;
	}

	sl_real Font::getFontAscent()
	{
		FontMetrics fm;
		if (getFontMetrics(fm)) {
			return fm.ascent;
		}
		return 0.0f;
	}

	sl_real Font::getFontDescent()
	{
		FontMetrics fm;
		if (getFontMetrics(fm)) {
			return fm.descent;
		}
		return 0.0f;
	}

	sl_bool Font::measureChar(sl_char32 ch, TextMetrics& _out)
	{
		String str = String::create(&ch, 1);
		if (str.isNull()) {
			return sl_false;
		}
		return measureText(str, _out);
	}

	sl_bool Font::measureText(const StringParam& text, TextMetrics& _out)
	{
		_out.advanceX = 0.0f;
		_out.advanceY = 0.0f;
		if (_measureText_PO(text, _out)) {
			if (_out.advanceX == 0.0f) {
				_out.advanceX = _out.getWidth();
			}
			if (_out.advanceY == 0.0f) {
				_out.advanceY = getFontHeight();
			}
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Font::measureText(const StringParam& _text, sl_bool flagMultiLine, TextMetrics& _out)
	{
		if (!flagMultiLine) {
			return measureText(_text, _out);
		}

		StringData16 text(_text);
		sl_size len = text.getLength();
		if (!len) {
			return sl_false;
		}
		sl_char16* sz = text.getData();

		sl_real fontHeight = getFontHeight();
		sl_real lineHeight = 0.0f;

		_out.setZero();
		sl_bool flagInitOut = sl_true;

		sl_size startLine = 0;
		sl_size pos = 0;
		while (pos <= len) {
			sl_char16 ch;
			if (pos < len) {
				ch = sz[pos];
			} else {
				ch = '\n';
			}
			if (ch == '\r' || ch == '\n') {
				if (pos > startLine) {
					TextMetrics lm;
					if (measureText(StringView16(sz + startLine, pos - startLine), lm)) {
						if (pos == len) {
							_out.advanceX = lm.advanceX;
						}
						if (lm.advanceY > lineHeight) {
							lineHeight = lm.advanceY;
						}
						lm.top += _out.advanceY;
						lm.bottom += _out.advanceY;
						if (flagInitOut) {
							(Rectangle&)_out = lm;
							flagInitOut = sl_false;
						} else {
							_out.mergeRectangle(lm);
						}
					}
				}
				if (ch == '\r' && pos + 1 < len) {
					if (sz[pos + 1] == '\n') {
						pos++;
					}
				}
				if (lineHeight == 0.0f) {
					_out.advanceY += fontHeight;
				} else {
					_out.advanceY += lineHeight;
					lineHeight = 0.0f;
				}
				startLine = pos + 1;
			}
			pos++;
		}
		return sl_true;
	}

	Size Font::measureText(const StringParam& text, sl_bool flagMultiLine)
	{
		TextMetrics tm;
		if (measureText(text, flagMultiLine, tm)) {
			return Size(tm.getWidth(), tm.getHeight());
		}
		return Size::zero();
	}

	Ref<CRef> Font::getPlatformObject()
	{
		return m_platformObject;
	}

#if !defined(SLIB_GRAPHICS_IS_GDI) && !defined(SLIB_GRAPHICS_IS_QUARTZ)
	List<String> Font::getAllFamilyNames()
	{
		return sl_null;
	}
#endif

#if !defined(SLIB_GRAPHICS_IS_GDI)
	sl_bool Font::addResource(const StringParam& filePath)
	{
		return sl_false;
	}

	sl_bool Font::addResource(const void* content, sl_size size)
	{
		return sl_false;
	}
#endif

	sl_bool Font::addResource(const MemoryView& content)
	{
		return addResource(content.data, content.size);
	}

}
