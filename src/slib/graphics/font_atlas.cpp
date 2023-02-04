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

#include "slib/graphics/font_atlas.h"

#include "slib/graphics/image.h"
#include "slib/core/variant.h"
#include "slib/core/safe_static.h"

#define PLANE_SIZE_MIN 32
#define PLANE_WIDTH_DEFAULT 0
#define PLANE_HEIGHT_DEFAULT 0
#define MAX_PLANES_DEFAULT 16
#define FONT_SIZE_MIN 4
#define FONT_SIZE_MAX 48

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FontAtlasParam)

	FontAtlasParam::FontAtlasParam()
	: planeWidth(PLANE_WIDTH_DEFAULT), planeHeight(PLANE_HEIGHT_DEFAULT), maxPlanes(MAX_PLANES_DEFAULT)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FontAtlasChar)

	FontAtlasChar::FontAtlasChar()
	{
		fontWidth = 0;
		fontHeight = 0;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FontAtlasCharImage)

	FontAtlasCharImage::FontAtlasCharImage()
	{
		fontWidth = 0;
		fontHeight = 0;
	}


	SLIB_DEFINE_OBJECT(FontAtlas, Object)

	FontAtlas::FontAtlas()
	{
		m_planeWidth = PLANE_WIDTH_DEFAULT;
		m_planeHeight = PLANE_HEIGHT_DEFAULT;
		m_maxPlanes = MAX_PLANES_DEFAULT;
		m_fontSourceHeight = 0;

		m_countPlanes = 0;
		m_currentPlaneY = 0;
		m_currentPlaneX = 0;
		m_currentPlaneRowHeight = 0;
	}

	FontAtlas::~FontAtlas()
	{
	}

	Ref<FontAtlas> FontAtlas::create(const FontAtlasParam& param)
	{
		if (param.font.isNotNull()) {
			String fontFamily = param.font->getFamilyName();
			sl_real fontSize = param.font->getSize();
			sl_bool fontBold = param.font->isBold();
			Ref<Font> fontSource = Font::create(fontFamily, fontSize, fontBold);
			if (fontSource.isNotNull()) {
				Ref<Font> fontDraw;
				if (fontSize > FONT_SIZE_MAX) {
					fontSize = FONT_SIZE_MAX;
					fontDraw = Font::create(fontFamily, fontSize, fontBold);
				} else if (fontSize < FONT_SIZE_MIN) {
					fontSize = FONT_SIZE_MIN;
					fontDraw = Font::create(fontFamily, fontSize, fontBold);
				} else {
					fontDraw = fontSource;
				}
				if (fontDraw.isNotNull()) {
					sl_uint32 planeWidth = param.planeWidth;
					if (planeWidth == 0) {
						planeWidth = (sl_uint32)(fontDraw->getFontHeight() * 16);
						if (planeWidth > 1024) {
							planeWidth = 1024;
						}
					}
					if (planeWidth < PLANE_SIZE_MIN) {
						planeWidth = PLANE_SIZE_MIN;
					}
					sl_uint32 planeHeight = param.planeHeight;
					if (planeHeight == 0) {
						planeHeight = (sl_uint32)(fontDraw->getFontHeight());
					}
					if (planeHeight < PLANE_SIZE_MIN) {
						planeHeight = PLANE_SIZE_MIN;
					}
					sl_uint32 maxPlanes = param.maxPlanes;
					if (maxPlanes < 1) {
						maxPlanes = 1;
					}
					Ref<Bitmap> plane = Bitmap::create(planeWidth, planeHeight);
					if (plane.isNotNull()) {
						Ref<Canvas> canvas = plane->getCanvas();
						if (canvas.isNotNull()) {
							Ref<FontAtlas> ret = new FontAtlas;
							if (ret.isNotNull()) {
								ret->m_fontSource = fontSource;
								ret->m_fontSourceHeight = fontSource->getFontHeight();
								ret->m_fontDraw = fontDraw;
								ret->m_planeWidth = planeWidth;
								ret->m_planeHeight = planeHeight;
								ret->m_maxPlanes = maxPlanes;
								ret->m_currentPlane = plane;
								ret->m_currentCanvas = canvas;
								ret->m_countPlanes = 1;
								return ret;
							}
						}
					}
				}
			}
		}
		return sl_null;
	}

	namespace {

		typedef CHashMap< String, WeakRef<FontAtlas> > FontAtlasMap;

		SLIB_SAFE_STATIC_GETTER(FontAtlasMap, GetFontAtlasMap)

	}

	Ref<FontAtlas> FontAtlas::getShared(const Ref<Font>& font)
	{
		if (font.isNotNull()) {
			FontAtlasMap* map = GetFontAtlasMap();
			if (map) {
				String fontFamily = font->getFamilyName();
				sl_real fontSize = SLIB_FONT_SIZE_PRECISION_APPLY(font->getSize());
				sl_bool fontBold = font->isBold();
				String sig = fontFamily;
				sig += String::fromUint32((sl_uint32)(fontSize * SLIB_FONT_SIZE_PRECISION_MULTIPLIER));
				if (fontBold) {
					sig += "B";
				}
				WeakRef<FontAtlas> weakFA;
				map->get(sig, &weakFA);
				Ref<FontAtlas> ret = weakFA;
				if (ret.isNotNull()) {
					return ret;
				}
				Ref<Font> fontSource = Font::create(fontFamily, fontSize, fontBold);
				if (fontSource.isNotNull()) {
					FontAtlasParam fap;
					fap.font = fontSource;
					ret = FontAtlas::create(fap);
					if (ret.isNotNull()) {
						map->put(sig, ret);
						return ret;
					}
				}
			}
		}
		return sl_null;
	}

	void FontAtlas::removeAllShared()
	{
		FontAtlasMap* map = GetFontAtlasMap();
		if (map) {
			map->removeAll();
		}
	}

	sl_bool FontAtlas::getChar(sl_char32 ch, FontAtlasChar& _out)
	{
		ObjectLocker lock(this);
		return _getChar(ch, sl_false, _out);
	}

	sl_bool FontAtlas::getCharImage(sl_char32 ch, FontAtlasCharImage& _out)
	{
		ObjectLocker lock(this);
		if (m_mapImage.get_NoLock(ch, &_out)) {
			return sl_true;
		}
		FontAtlasChar fac;
		if (_getChar(ch, sl_false, fac)) {
			_out.fontWidth = fac.fontWidth;
			_out.fontHeight = fac.fontHeight;
			if (fac.bitmap.isNotNull()) {
				_out.image = Image::createCopyBitmap(fac.bitmap, fac.region.left, fac.region.top, fac.region.getWidth(), fac.region.getHeight());
				if (_out.image.isNotNull()) {
					Color* colors = _out.image->getColors();
					sl_uint32 width = _out.image->getWidth();
					sl_uint32 height = _out.image->getHeight();
					sl_reg stride = _out.image->getStride();
					for (sl_uint32 i = 0; i < height; i++) {
						Color* c = colors;
						for (sl_uint32 j = 0; j < width; j++) {
							c->a = c->a * (c->r + c->g + c->b) / 765;
							c->r = c->g = c->b = 255;
							c++;
						}
						colors += stride;
					}
					m_mapImage.put_NoLock(ch, _out);
					return sl_true;
				}
			} else {
				_out.image.setNull();
				return sl_true;
			}
		}
		return sl_false;
	}

	Size FontAtlas::getFontSize(sl_char32 ch)
	{
		ObjectLocker lock(this);
		FontAtlasChar fac;
		if (_getChar(ch, sl_true, fac)) {
			return Size(fac.fontWidth, fac.fontHeight);
		}
		return Size::zero();
	}

	Size FontAtlas::getFontSize_NoLock(sl_char32 ch)
	{
		FontAtlasChar fac;
		if (_getChar(ch, sl_true, fac)) {
			return Size(fac.fontWidth, fac.fontHeight);
		}
		return Size::zero();
	}

	Size FontAtlas::measureText(const StringParam& _str, sl_bool flagMultiLine)
	{
		StringData16 str(_str);
		if (str.isEmpty()) {
			return Size::zero();
		}
		ObjectLocker lock(this);
		sl_char16* data = str.getData();
		sl_size len = str.getLength();
		sl_real lineWidth = 0;
		sl_real lineHeight = 0;
		sl_real maxWidth = 0;
		sl_real totalHeight = 0;
		FontAtlasChar fac;
		for (sl_size i = 0; i < len;) {
			sl_char32 ch;
			if (Charsets::getUnicode(ch, data, len, i)) {
				if (ch == '\r' || ch == '\n') {
					if (!flagMultiLine) {
						return Size(lineWidth, lineHeight);
					}
					if (lineHeight < m_fontSourceHeight) {
						lineHeight = m_fontSourceHeight;
					}
					if (lineWidth > maxWidth) {
						maxWidth = lineWidth;
					}
					totalHeight += lineHeight;
					lineWidth = 0;
					lineHeight = 0;
					if (ch == '\r' && i < len) {
						if (data[i] == '\n') {
							i++;
						}
					}
				} else {
					if (_getChar((sl_char32)ch, sl_true, fac)) {
						lineWidth += fac.fontWidth;
						if (lineHeight < fac.fontHeight) {
							lineHeight = fac.fontHeight;
						}
					}
				}
			} else {
				i++;
			}
		}
		if (lineWidth > maxWidth) {
			maxWidth = lineWidth;
		}
		totalHeight += lineHeight;
		return Size(maxWidth, totalHeight);
	}

	sl_bool FontAtlas::_getChar(sl_char32 ch, sl_bool flagSizeOnly, FontAtlasChar& _out)
	{
		if (ch == ' ' || ch == '\r' || ch == '\n') {
			_out.fontWidth = m_fontSourceHeight * 0.3f;
			_out.fontHeight = m_fontSourceHeight;
			if (!flagSizeOnly) {
				_out.bitmap.setNull();
			}
			return sl_true;
		}
		if (ch == '\t') {
			_out.fontWidth = m_fontSourceHeight * 2.0f;
			_out.fontHeight = m_fontSourceHeight;
			if (!flagSizeOnly) {
				_out.bitmap.setNull();
			}
			return sl_true;
		}
		if (m_map.get_NoLock(ch, &_out)) {
			if (_out.fontWidth <= 0 || _out.fontHeight <= 0) {
				return sl_false;
			}
			if (flagSizeOnly) {
				return sl_true;
			}
			if (_out.bitmap.isNotNull()) {
				return sl_true;
			}
		} else {
			Size sizeFont = m_fontSource->measureText(String::create(&ch, 1));
			if (sizeFont.x <= 0 || sizeFont.y <= 0) {
				FontAtlasChar fac;
				fac.fontWidth = 0;
				fac.fontHeight = 0;
				m_map.put_NoLock(ch, fac);
				return sl_false;
			}
			_out.fontWidth = sizeFont.x;
			_out.fontHeight = sizeFont.y;
			_out.bitmap.setNull();
			if (flagSizeOnly) {
				return m_map.put_NoLock(ch, _out);
			}
		}

		String s = String::create(&ch, 1);

		Sizei sizeDraw = m_fontDraw->measureText(s);

		sl_uint32 widthChar = sizeDraw.x;
		sl_uint32 heightChar = sizeDraw.y;
		if (sizeDraw.x <= 0 || sizeDraw.y <= 0) {
			_out.fontWidth = 0;
			_out.fontHeight = 0;
			m_map.put_NoLock(ch, _out);
			return sl_false;
		}

		if (m_currentPlaneX > 0 && m_currentPlaneX + widthChar > m_planeWidth) {
			m_currentPlaneX = 0;
			m_currentPlaneY += m_currentPlaneRowHeight;
			m_currentPlaneRowHeight = 0;
		}
		if (m_currentPlaneY > 0 && m_currentPlaneY + heightChar > m_planeHeight) {
			if (m_countPlanes >= m_maxPlanes) {
				m_map.removeAll_NoLock();
				m_countPlanes = 1;
			} else {
				Ref<Bitmap> plane = Bitmap::create(m_planeWidth, m_planeHeight);
				if (plane.isNull()) {
					return sl_false;
				}
				Ref<Canvas> canvas = plane->getCanvas();
				if (canvas.isNull()) {
					return sl_false;
				}
				m_currentPlane = plane;
				m_currentCanvas = canvas;
				m_countPlanes++;
			}
			m_currentPlaneX = 0;
			m_currentPlaneY = 0;
			m_currentPlaneRowHeight = 0;
		}

		sl_uint32 x = m_currentPlaneX;
		sl_uint32 y = m_currentPlaneY;
		_out.region.left = x;
		_out.region.top = y;
		_out.region.right = x + widthChar;
		_out.region.bottom = y + heightChar;

		m_currentPlane->resetPixels(x, y, widthChar, heightChar, Color::Zero);
		m_currentCanvas->drawText(s, (sl_real)x, (sl_real)y, m_fontDraw, Color::White);

		m_currentPlane->update(x, y, widthChar, heightChar);

		m_currentPlaneX += widthChar;

		if (heightChar > m_currentPlaneRowHeight) {
			m_currentPlaneRowHeight = heightChar;
		}

		_out.bitmap = m_currentPlane;

		return m_map.put_NoLock(ch, _out);

	}

	void FontAtlas::removeAll()
	{
		ObjectLocker lock(this);
		m_map.removeAll_NoLock();
		m_countPlanes = 1;
		m_currentPlaneX = 0;
		m_currentPlaneY = 0;
		m_currentPlaneRowHeight = 0;
	}

	Ref<FontAtlas> Font::getAtlas()
	{
		Ref<FontAtlas> fa = m_fontAtlas;
		if (fa.isNull()) {
			FontAtlasParam param;
			param.font = this;
			fa = FontAtlas::create(param);
			if (fa.isNotNull()) {
				m_fontAtlas = fa;
			}
		}
		return fa;
	}

	Ref<FontAtlas> Font::getSharedAtlas()
	{
		Ref<FontAtlas> fa = m_fontAtlas;
		if (fa.isNull()) {
			fa = FontAtlas::getShared(this);
			if (fa.isNotNull()) {
				m_fontAtlas = fa;
			}
		}
		return fa;
	}

}
