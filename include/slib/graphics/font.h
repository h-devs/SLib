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

#ifndef CHECKHEADER_SLIB_GRAPHICS_FONT
#define CHECKHEADER_SLIB_GRAPHICS_FONT

#include "definition.h"

#include "../core/object.h"
#include "../core/string.h"
#include "../math/rectangle.h"

#define SLIB_FONT_SIZE_PRECISION_MULTIPLIER 10
#define SLIB_FONT_SIZE_PRECISION_APPLY(f) ((sl_real)((sl_int32)((f) * SLIB_FONT_SIZE_PRECISION_MULTIPLIER + 0.5f)) / SLIB_FONT_SIZE_PRECISION_MULTIPLIER)

namespace slib
{

	class SLIB_EXPORT FontDesc
	{
	public:
		String familyName; // empty value means default
		sl_real size; // negative value means default
		union {
			sl_int32 flags; // negative value means default
			struct {
				sl_bool flagBold : 1;
				sl_bool flagItalic : 1;
				sl_bool flagUnderline : 1;
				sl_bool flagStrikeout : 1;
			};
		};

	public:
		FontDesc();

		FontDesc(const String& familyName, sl_real size = -1, sl_bool flagBold = sl_false, sl_bool flagItalic = sl_false, sl_bool flagUnderline = sl_false, sl_bool flagStrikeout = sl_false);

		FontDesc(const String& familyName, sl_real size, sl_int32 flags);

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FontDesc)

	};

	struct SLIB_EXPORT FontMetrics
	{
		sl_real ascent;
		sl_real descent;
		sl_real leading;
	};

	class SLIB_EXPORT TextMetrics : public Rectangle
	{
	public:
		sl_real advanceX;
		sl_real advanceY;

	public:
		void setZero();

		void setBlank(sl_real advanceX, sl_real advanceY);
	};

	class Locale;
	class FontAtlas;

	class SLIB_EXPORT Font : public CRef
	{
		SLIB_DECLARE_OBJECT

	protected:
		Font();

		~Font();

	public:
		static Ref<Font> create(const FontDesc& desc);

		static Ref<Font> create(const String& familyName, sl_real size = -1, sl_bool flagBold = sl_false, sl_bool flagItalic = sl_false, sl_bool flagUnderline = sl_false, sl_bool flagStrikeout = sl_false);

		static Ref<Font> create(const String& familyName, sl_real size, const Ref<Font>& original);

		static Ref<Font> create(const String& familyName, const Ref<Font>& original);

		static Ref<Font> create(sl_real size, const Ref<Font>& original);

		static Ref<Font> create(const FontDesc& desc, const Ref<Font>& original);

		static Ref<Font> createBold(const Ref<Font>& original);

		static Ref<Font> createItalic(const Ref<Font>& original);

		static Ref<Font> createUnderline(const Ref<Font>& original);

		static Ref<Font> getDefault();

		static void setDefault(const Ref<Font>& font);

		static sl_real getDefaultFontSize();

		static void setDefaultFontSize(sl_real size);

		static String getDefaultFontFamily();

		static void setDefaultFontFamily(const String& fontFamily);

		static String getDefaultFontFamilyForLocale(const Locale& locale);

		static void setDefaultFontFamilyForLocale(const Locale& locale);

		static List<String> getAllFamilyNames();

		static sl_bool addResource(const StringParam& filePath);

		static sl_bool addResource(const void* content, sl_size size);

		static sl_bool addResource(const MemoryView& content);

	public:
		void getDesc(FontDesc& desc);

		String getFamilyName();

		sl_real getSize();

		sl_uint32 getFlags();

		sl_bool isBold();

		sl_bool isItalic();

		sl_bool isUnderline();

		sl_bool isStrikeout();

		sl_bool getFontMetrics(FontMetrics& _out);

		sl_real getFontHeight();

		sl_real getFontAscent();

		sl_real getFontDescent();

		sl_bool measureChar(sl_char32 ch, TextMetrics& _out);

		sl_bool measureText(const StringParam& text, TextMetrics& _out);

		sl_bool measureText(const StringParam& text, sl_bool flagMultiLine, TextMetrics& _out);

		Size measureText(const StringParam& text, sl_bool flagMultiLine = sl_false);

		Ref<FontAtlas> getAtlas();

		Ref<FontAtlas> getSharedAtlas();

		Ref<CRef> getPlatformObject();

	private:
		sl_bool _getFontMetrics_PO(FontMetrics& _out);

		sl_bool _measureText_PO(const StringParam& text, TextMetrics& _out);

	protected:
		FontDesc m_desc;
		FontMetrics m_metricsCache;
		sl_bool m_flagMetricsCache;

		AtomicRef<FontAtlas> m_fontAtlas;

		Ref<CRef> m_platformObject;
		SpinLock m_lock;

	};

}

#endif
