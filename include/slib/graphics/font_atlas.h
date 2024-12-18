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

#ifndef CHECKHEADER_SLIB_GRAPHICS_FONT_ATLAS
#define CHECKHEADER_SLIB_GRAPHICS_FONT_ATLAS

#include "font.h"
#include "bitmap.h"

#include "../core/hash_map.h"

namespace slib
{

	class SLIB_EXPORT FontAtlasBaseParam
	{
	public:
		Color color;
		sl_real scale;
		Color strokeColor;
		sl_real strokeWidth;

		sl_uint32 planeWidth;
		sl_uint32 planeHeight;
		sl_uint32 maxPlanes;

	public:
		FontAtlasBaseParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FontAtlasBaseParam)

	};

	class SLIB_EXPORT FontAtlasParam : public FontAtlasBaseParam
	{
	public:
		Ref<Font> font;
		sl_real maximumFontSize;

	public:
		FontAtlasParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FontAtlasParam)

	};

	class SLIB_EXPORT FontAtlasChar
	{
	public:
		TextMetrics metrics;
		Ref<Bitmap> bitmap;
		RectangleI region;

	public:
		FontAtlasChar();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FontAtlasChar)

	};

	class SLIB_EXPORT FontAtlasCharImage
	{
	public:
		TextMetrics metrics;
		Ref<Image> image;

	public:
		FontAtlasCharImage();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FontAtlasCharImage)

	};

	class SLIB_EXPORT FontAtlas : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		FontAtlas();

		~FontAtlas();

	public:
		static Ref<FontAtlas> getShared(const Ref<Font>& font);

		static void removeAllShared();

		static Ref<FontAtlas> create(const FontAtlasParam& param);

		static Ref<FontAtlas> create(const Ref<Font>& font, const Color& color = Color::White);

	public:
		sl_real getFontHeight();

		sl_bool getFontMetrics(FontMetrics& _out);

		sl_bool getChar_NoLock(sl_char32 ch, FontAtlasChar& _out);

		sl_bool getChar(sl_char32 ch, FontAtlasChar& _out);

		virtual sl_bool getCharImage_NoLock(sl_char32 ch, FontAtlasCharImage& _out) = 0;

		sl_bool getCharImage(sl_char32 ch, FontAtlasCharImage& _out);

		sl_bool measureChar_NoLock(sl_char32 ch, TextMetrics& _out);

		sl_bool measureChar(sl_char32 ch, TextMetrics& _out);

		Size getCharAdvance_NoLock(sl_char32 ch);

		Size getCharAdvance(sl_char32 ch);

		sl_bool measureText(const StringParam& text, sl_bool flagMultiLine, TextMetrics& _out);

		sl_bool measureText(const StringParam& text, TextMetrics& _out);

		Size getTextAdvance(const StringParam& text, sl_bool flagMultiLine = sl_false);

		void removeAll();

	protected:
		void _initialize(const FontAtlasBaseParam& param, sl_real sourceHeight, sl_real fontHeight, sl_real strokeWidth, sl_uint32 planeWidth, sl_uint32 planeHeight);

		virtual sl_bool _getFontMetrics(FontMetrics& _out) = 0;

		sl_bool _getChar(sl_char32 ch, sl_bool flagSizeOnly, FontAtlasChar& _out);

		virtual sl_bool _measureChar(sl_char32 ch, TextMetrics& metrics) = 0;

		virtual Ref<Bitmap> _drawChar(sl_uint32 dstX, sl_uint32 dstY, sl_uint32 width, sl_uint32 height, sl_real charX, sl_real charY, sl_char32 ch) = 0;

		virtual sl_bool _createPlane() = 0;

	protected:
		sl_real m_drawHeight;
		sl_real m_drawScale;
		Color m_textColor;
		Color m_strokeColor;
		sl_real m_strokeWidth; // Plane Unit

		sl_uint32 m_planeWidth;
		sl_uint32 m_planeHeight;
		sl_uint32 m_maxPlanes;

		CHashMap<sl_char32, FontAtlasChar> m_map;
		sl_uint32 m_countPlanes;
		sl_uint32 m_currentPlaneY;
		sl_uint32 m_currentPlaneX;
		sl_uint32 m_currentPlaneRowHeight;
	};

}

#endif

