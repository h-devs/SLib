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

#ifndef CHECKHEADER_SLIB_GRAPHICS_FREETYPE_ATLAS
#define CHECKHEADER_SLIB_GRAPHICS_FREETYPE_ATLAS

#include "freetype.h"
#include "font_atlas.h"

namespace slib
{

	class SLIB_EXPORT FreeTypeAtlasParam : public FontAtlasBaseParam
	{
	public:
		Ref<FreeType> font;

	public:
		FreeTypeAtlasParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FreeTypeAtlasParam)

	};

	class SLIB_EXPORT FreeTypeAtlas : public FontAtlas
	{
		SLIB_DECLARE_OBJECT

	protected:
		FreeTypeAtlas();

		~FreeTypeAtlas();

	public:
		static Ref<FreeTypeAtlas> create(const FreeTypeAtlasParam& param);

		static Ref<FreeTypeAtlas> create(const Ref<FreeType>& font, const Color& color = Color::White);

	public:
		sl_bool getCharImage_NoLock(sl_char32 ch, FontAtlasCharImage& _out) override;

	protected:
		sl_bool _getFontMetrics(FontMetrics& _out) override;

		sl_bool _measureChar(sl_char32 ch, TextMetrics& _out) override;

		Ref<Bitmap> _drawChar(sl_uint32 dstX, sl_uint32 dstY, sl_uint32 width, sl_uint32 height, sl_real charX, sl_real charY, sl_char32 ch) override;

		sl_bool _createPlane() override;

	protected:
		Ref<FreeType> m_font;
		Ref<Image> m_currentPlane;
	};

}

#endif

