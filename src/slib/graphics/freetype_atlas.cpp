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

#include "slib/graphics/freetype_atlas.h"

#include "slib/graphics/image.h"

#define PLANE_SIZE_MIN 32
#define FONT_SIZE_MIN 4
#define FONT_SIZE_MAX 48

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FreeTypeAtlasParam)

	FreeTypeAtlasParam::FreeTypeAtlasParam()
	{
	}

	SLIB_DEFINE_OBJECT(FreeTypeAtlas, Object)

	FreeTypeAtlas::FreeTypeAtlas()
	{
		m_strokeWidth = 0;
	}

	FreeTypeAtlas::~FreeTypeAtlas()
	{
	}

	Ref<FreeTypeAtlas> FreeTypeAtlas::create(const FreeTypeAtlasParam& param)
	{
		if (param.font.isNull()) {
			return sl_null;
		}
		Ref<FreeType> font = param.font;
		sl_real sourceHeight = font->getFontHeight();
		sl_real fontHeight;
		if (sourceHeight > FONT_SIZE_MAX) {
			font->setRealSize(FONT_SIZE_MAX);
			fontHeight = font->getFontHeight();
		} else if (sourceHeight < FONT_SIZE_MIN) {
			font->setRealSize(FONT_SIZE_MIN);
			fontHeight = font->getFontHeight();
		} else {
			fontHeight = sourceHeight;
		}
		sl_uint32 planeWidth = param.planeWidth;
		if (!planeWidth) {
			planeWidth = (sl_uint32)(fontHeight * 16);
			if (planeWidth > 1024) {
				planeWidth = 1024;
			}
		}
		if (planeWidth < PLANE_SIZE_MIN) {
			planeWidth = PLANE_SIZE_MIN;
		}
		sl_uint32 planeHeight = param.planeHeight;
		if (!planeHeight) {
			planeHeight = (sl_uint32)fontHeight;
		}
		if (planeHeight < PLANE_SIZE_MIN) {
			planeHeight = PLANE_SIZE_MIN;
		}
		Ref<Image> plane = Image::create(planeWidth, planeHeight);
		if (plane.isNull()) {
			return sl_null;
		}
		Ref<FreeTypeAtlas> ret = new FreeTypeAtlas;
		if (ret.isNull()) {
			return sl_null;
		}
		ret->initialize(param, sourceHeight, fontHeight, planeWidth, planeHeight);
		ret->m_font = Move(font);
		ret->m_strokeWidth = param.strokeWidth;
		ret->m_currentPlane = Move(plane);
		return ret;
	}

	sl_bool FreeTypeAtlas::getCharImage_NoLock(sl_char32 ch, FontAtlasCharImage& _out)
	{
		FontAtlasChar fac;
		if (!(getChar_NoLock(ch, sl_false, fac))) {
			return sl_false;
		}
		_out.fontWidth = fac.fontWidth;
		_out.fontHeight = fac.fontHeight;
		if (fac.bitmap.isNotNull()) {
			_out.image = Ref<Image>::cast(fac.bitmap)->sub(fac.region.left, fac.region.top, fac.region.getWidth(), fac.region.getHeight());
			if (_out.image.isNull()) {
				return sl_false;
			}
		} else {
			_out.image.setNull();
		}
		return sl_true;
	}

	Size FreeTypeAtlas::measureChar(sl_char32 ch)
	{
		Size ret = m_font->getCharExtent(ch);
		if (m_strokeWidth) {
			sl_uint32 m = m_strokeWidth << 1;
			ret.x += m;
			ret.y += m;
		}
		return ret;
	}

	Ref<Bitmap> FreeTypeAtlas::drawChar(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height, sl_char32 ch)
	{
		m_currentPlane->resetPixels(x, y, width, height, Color::Zero);
		if (m_strokeWidth) {
			m_font->strokeChar(m_currentPlane, x + m_strokeWidth, y + m_strokeWidth, ch, Color::White, m_strokeWidth);
		} else {
			m_font->drawChar(m_currentPlane, x, y, ch, Color::White);
		}
		m_currentPlane->update(x, y, width, height);
		return m_currentPlane;
	}

	sl_bool FreeTypeAtlas::createPlane()
	{
		Ref<Image> plane = Image::create(m_planeWidth, m_planeHeight);
		if (plane.isNull()) {
			return sl_false;
		}
		m_currentPlane = Move(plane);
		return sl_true;
	}

}
