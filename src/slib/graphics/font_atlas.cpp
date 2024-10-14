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

#include "slib/graphics/font_atlas.h"

#include "slib/graphics/image.h"
#include "slib/core/safe_static.h"

#define PLANE_SIZE_MIN 32
#define PLANE_WIDTH_DEFAULT 0
#define PLANE_HEIGHT_DEFAULT 0
#define MAX_PLANES_DEFAULT 16
#define FONT_SIZE_MIN 4

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FontAtlasChar)

	FontAtlasChar::FontAtlasChar()
	{
		metrics.setZero();
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FontAtlasCharImage)

	FontAtlasCharImage::FontAtlasCharImage()
	{
		metrics.setZero();
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FontAtlasBaseParam)

	FontAtlasBaseParam::FontAtlasBaseParam(): planeWidth(PLANE_WIDTH_DEFAULT), planeHeight(PLANE_HEIGHT_DEFAULT), maxPlanes(MAX_PLANES_DEFAULT), scale(1.0f), strokeWidth(0)
	{
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FontAtlasParam)

	FontAtlasParam::FontAtlasParam(): maximumFontSize(48.0f)
	{
	}


	SLIB_DEFINE_OBJECT(FontAtlas, Object)

	FontAtlas::FontAtlas()
	{
		m_drawHeight = 0.0f;
		m_drawScale = 1.0f;
		m_planeWidth = PLANE_WIDTH_DEFAULT;
		m_planeHeight = PLANE_HEIGHT_DEFAULT;
		m_maxPlanes = MAX_PLANES_DEFAULT;
		m_strokeWidth = 0;

		m_countPlanes = 0;
		m_currentPlaneY = 0;
		m_currentPlaneX = 0;
		m_currentPlaneRowHeight = 0;
	}

	FontAtlas::~FontAtlas()
	{
	}

	namespace
	{
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


	namespace
	{
		class FontAtlasImpl : public FontAtlas
		{
		public:
			Ref<Font> m_font;
			Ref<Bitmap> m_currentPlane;
			Ref<Canvas> m_currentCanvas;
			CHashMap<sl_char32, FontAtlasCharImage> m_mapImage;
			Ref<Pen> m_penStroke;

		public:
			static Ref<FontAtlasImpl> create(const FontAtlasParam& param)
			{
				if (param.font.isNull()) {
					return sl_null;
				}
				sl_real fontSize = param.font->getSize();
				if (fontSize > param.maximumFontSize) {
					fontSize = param.maximumFontSize;
				}
				if (fontSize < FONT_SIZE_MIN) {
					fontSize = FONT_SIZE_MIN;
				}
				Ref<Font> font = Font::create(param.font->getFamilyName(), fontSize, param.font->isBold());
				if (font.isNull()) {
					return sl_null;
				}
				sl_real sourceHeight = param.font->getFontHeight();
				sl_real fontHeight = font->getFontHeight();
				sl_uint32 strokeWidth = (sl_uint32)((sl_real)(param.strokeWidth) * fontHeight / sourceHeight / param.scale);
				sl_uint32 fontHeightExt = (sl_uint32)fontHeight + (strokeWidth + 1);
				sl_uint32 planeWidth = param.planeWidth;
				if (!planeWidth) {
					planeWidth = ((sl_uint32)fontHeightExt) << 4;
					if (planeWidth > 1024) {
						planeWidth = 1024;
					}
				}
				if (planeWidth < PLANE_SIZE_MIN) {
					planeWidth = PLANE_SIZE_MIN;
				}
				sl_uint32 planeHeight = param.planeHeight;
				if (!planeHeight) {
					planeHeight = (sl_uint32)fontHeightExt;
				}
				if (planeHeight < PLANE_SIZE_MIN) {
					planeHeight = PLANE_SIZE_MIN;
				}
				Ref<Bitmap> plane = Bitmap::create(planeWidth, planeHeight);
				if (plane.isNull()) {
					return sl_null;
				}
				Ref<Canvas> canvas = plane->getCanvas();
				if (canvas.isNull()) {
					return sl_null;
				}
				Ref<FontAtlasImpl> ret = new FontAtlasImpl;
				if (ret.isNull()) {
					return sl_null;
				}
				ret->_initialize(param, sourceHeight, fontHeight, strokeWidth, planeWidth, planeHeight);
				ret->m_font = Move(font);
				ret->m_currentPlane = Move(plane);
				ret->m_currentCanvas = Move(canvas);
				if (strokeWidth) {
					PenDesc desc;
					desc.style = PenStyle::Solid;
					desc.width = (sl_real)strokeWidth;
					desc.color = Color::White;
					desc.cap = LineCap::Round;
					desc.join = LineJoin::Round;
					Ref<Pen> pen = Pen::create(desc);
					if (pen.isNull()) {
						return sl_null;
					}
					ret->m_penStroke = Move(pen);
				}
				return ret;
			}

			sl_bool _getFontMetrics(FontMetrics& _out) override
			{
				return m_font->getFontMetrics(_out);
			}

			sl_bool getCharImage_NoLock(sl_char32 ch, FontAtlasCharImage& _out) override
			{
				if (m_mapImage.get_NoLock(ch, &_out)) {
					return sl_true;
				}
				FontAtlasChar fac;
				if (!(_getChar(ch, sl_false, fac))) {
					return sl_false;
				}
				_out.metrics = fac.metrics;
				if (fac.bitmap.isNotNull()) {
					_out.image = Image::createCopyBitmap(fac.bitmap, fac.region.left, fac.region.top, fac.region.getWidth(), fac.region.getHeight());
					if (_out.image.isNull()) {
						return sl_false;
					}
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
				} else {
					_out.image.setNull();
				}
				return sl_true;
			}

			Ref<FontAtlas> createStroker(sl_uint32 strokeWidth) override
			{
				FontAtlasParam param;
				param.font = m_font;
				param.scale = m_drawScale;
				param.strokeWidth = strokeWidth;
				param.planeWidth = m_planeWidth + (strokeWidth << 5);
				param.planeHeight = m_planeHeight + (strokeWidth << 1);
				param.maxPlanes = m_maxPlanes;
				return create(param);
			}

			sl_bool _measureChar(sl_char32 ch, TextMetrics& _out) override
			{
				return m_font->measureChar(ch, _out);
			}

			Ref<Bitmap> _drawChar(sl_uint32 dstX, sl_uint32 dstY, sl_uint32 width, sl_uint32 height, sl_real charX, sl_real charY, sl_char32 ch) override
			{
				m_currentPlane->resetPixels(dstX, dstY, width, height, Color::zero());
				if (m_penStroke.isNotNull()) {
					Ref<GraphicsPath> path = m_font->getCharOutline(ch, charX, charY);
					if (path.isNotNull()) {
						m_currentCanvas->drawPath(path, m_penStroke);
					}
				} else {
					m_currentCanvas->drawText(String::create(&ch, 1), charX, charY, m_font, Color::White);
				}
				m_currentPlane->update(dstX, dstY, width, height);
				return m_currentPlane;
			}

			sl_bool _createPlane() override
			{
				Ref<Bitmap> plane = Bitmap::create(m_planeWidth, m_planeHeight);
				if (plane.isNull()) {
					return sl_false;
				}
				Ref<Canvas> canvas = plane->getCanvas();
				if (canvas.isNull()) {
					return sl_false;
				}
				m_currentPlane = Move(plane);
				m_currentCanvas = Move(canvas);
				return sl_true;
			}
		};
	}

	Ref<FontAtlas> FontAtlas::create(const FontAtlasParam& param)
	{
		return Ref<FontAtlas>::cast(FontAtlasImpl::create(param));
	}

	Ref<FontAtlas> FontAtlas::create(const Ref<Font>& font, sl_uint32 strokeWidth)
	{
		FontAtlasParam param;
		param.font = font;
		param.strokeWidth = strokeWidth;
		return create(param);
	}

	void FontAtlas::_initialize(const FontAtlasBaseParam& param, sl_real sourceHeight, sl_real fontHeight, sl_uint32 strokeWidth, sl_uint32 planeWidth, sl_uint32 planeHeight)
	{
		m_drawHeight = param.scale * sourceHeight;
		m_drawScale = m_drawHeight / fontHeight;
		m_strokeWidth = strokeWidth;
		m_planeWidth = planeWidth;
		m_planeHeight = planeHeight;
		m_countPlanes = 1;
		sl_uint32 maxPlanes = param.maxPlanes;
		if (maxPlanes < 1) {
			maxPlanes = 1;
		}
		m_maxPlanes = maxPlanes;
	}

	sl_real FontAtlas::getFontHeight()
	{
		return m_drawHeight;
	}

	sl_bool FontAtlas::_getChar(sl_char32 ch, sl_bool flagSizeOnly, FontAtlasChar& _out)
	{
		if (ch == '\r' || ch == '\n') {
			_out.metrics.setBlank(m_drawHeight * 0.3f, m_drawHeight);
			if (!flagSizeOnly) {
				_out.bitmap.setNull();
			}
			return sl_true;
		}
		sl_real charX, charY;
		sl_uint32 widthChar, heightChar;
		if (m_map.get_NoLock(ch, &_out)) {
			if (_out.metrics.advanceX == 0.0f) {
				return sl_false;
			}
			if (flagSizeOnly) {
				return sl_true;
			}
			if (_out.bitmap.isNotNull()) {
				return sl_true;
			}
			charX = _out.metrics.left / m_drawScale;
			charY = _out.metrics.top / m_drawScale;
			widthChar = (sl_int32)(Math::ceil(_out.metrics.getWidth() / m_drawScale));
			heightChar = (sl_int32)(Math::ceil(_out.metrics.getHeight() / m_drawScale));
		} else {
			TextMetrics tm;
			if (!(_measureChar(ch, tm))) {
				_out.metrics.setZero();
				m_map.put_NoLock(ch, _out);
				return sl_false;
			}
			sl_real m = (sl_real)((m_strokeWidth >> 1) + 1);
			tm.left -= m;
			tm.top -= m;
			tm.right += m;
			tm.bottom += m;
			_out.metrics.left = tm.left * m_drawScale;
			_out.metrics.top = tm.top * m_drawScale;
			_out.metrics.right = tm.right * m_drawScale;
			_out.metrics.bottom = tm.bottom * m_drawScale;
			_out.metrics.advanceX = tm.advanceX * m_drawScale;
			_out.metrics.advanceY = tm.advanceY * m_drawScale;
			_out.bitmap.setNull();
			if (flagSizeOnly) {
				return m_map.put_NoLock(ch, _out);
			}
			charX = tm.left;
			charY = tm.top;
			widthChar = (sl_int32)(Math::ceil(tm.getWidth()));
			heightChar = (sl_int32)(Math::ceil(tm.getHeight()));
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
				if (!(_createPlane())) {
					return sl_false;
				}
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
		_out.bitmap = _drawChar(x, y, widthChar, heightChar, (sl_real)x - charX, (sl_real)y - charY, ch);

		m_currentPlaneX += widthChar;
		if (heightChar > m_currentPlaneRowHeight) {
			m_currentPlaneRowHeight = heightChar;
		}
		return m_map.put_NoLock(ch, _out);
	}

	sl_bool FontAtlas::getFontMetrics(FontMetrics& _out)
	{
		if (_getFontMetrics(_out)) {
			_out.ascent *= m_drawScale;
			_out.descent *= m_drawScale;
			_out.leading *= m_drawScale;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool FontAtlas::getChar_NoLock(sl_char32 ch, FontAtlasChar& _out)
	{
		return _getChar(ch, sl_false, _out);
	}

	sl_bool FontAtlas::getChar(sl_char32 ch, FontAtlasChar& _out)
	{
		ObjectLocker lock(this);
		return _getChar(ch, sl_false, _out);
	}

	sl_bool FontAtlas::getCharImage(sl_char32 ch, FontAtlasCharImage& _out)
	{
		ObjectLocker lock(this);
		return getCharImage_NoLock(ch, _out);
	}

	sl_bool FontAtlas::measureChar_NoLock(sl_char32 ch, TextMetrics& _out)
	{
		FontAtlasChar fac;
		if (_getChar(ch, sl_true, fac)) {
			_out = fac.metrics;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool FontAtlas::measureChar(sl_char32 ch, TextMetrics& _out)
	{
		ObjectLocker lock(this);
		return measureChar_NoLock(ch, _out);
	}

	Size FontAtlas::getCharAdvance_NoLock(sl_char32 ch)
	{
		FontAtlasChar fac;
		if (_getChar(ch, sl_true, fac)) {
			return Size(fac.metrics.advanceX, fac.metrics.advanceY);
		}
		return Size::zero();
	}

	Size FontAtlas::getCharAdvance(sl_char32 ch)
	{
		ObjectLocker lock(this);
		return getCharAdvance_NoLock(ch);
	}

	sl_bool FontAtlas::measureText(const StringParam& _str, sl_bool flagMultiLine, TextMetrics& _out)
	{
		StringData32 str(_str);
		sl_size len = str.getLength();
		if (!len) {
			return sl_false;
		}
		sl_char32* data = str.getData();

		FontAtlasChar fac;
		sl_real lineWidth = 0.0f;
		sl_real lineHeight = 0.0f;

		_out.setZero();
		sl_bool flagInitOut = sl_true;

		ObjectLocker lock(this);
		for (sl_size i = 0; i <= len; i++) {
			sl_char32 ch;
			if (i < len) {
				ch = data[i];
			} else {
				ch = '\n';
			}
			if (ch == '\r' || ch == '\n') {
				if (lineWidth > _out.advanceX) {
					_out.advanceX = lineWidth;
				}
				if (lineHeight == 0.0f) {
					_out.advanceY += m_drawHeight;
				} else {
					_out.advanceY += lineHeight;
				}
				if (!flagMultiLine) {
					return sl_true;
				}
				lineWidth = 0.0f;
				lineHeight = 0.0f;
				if (ch == '\r' && i + 1 < len) {
					if (data[i + 1] == '\n') {
						i++;
					}
				}
			} else {
				if (_getChar((sl_char32)ch, sl_true, fac)) {
					fac.metrics.left += lineWidth;
					fac.metrics.right += lineWidth;
					fac.metrics.top += _out.advanceY;
					fac.metrics.bottom += _out.advanceY;
					if (flagInitOut) {
						(Rectangle&)_out = fac.metrics;
						flagInitOut = sl_false;
					} else {
						_out.mergeRectangle(fac.metrics);
					}
					lineWidth += fac.metrics.advanceX;
					if (fac.metrics.advanceY > lineHeight) {
						lineHeight = fac.metrics.advanceY;
					}
				}
			}
		}
		return sl_true;
	}

	sl_bool FontAtlas::measureText(const StringParam& text, TextMetrics& _out)
	{
		if (measureText(text, sl_false, _out)) {
			return sl_true;
		}
		return sl_false;
	}

	Size FontAtlas::getTextAdvance(const StringParam& text, sl_bool flagMultiLine)
	{
		TextMetrics tm;
		if (measureText(text, flagMultiLine, tm)) {
			return Size(tm.advanceX, tm.advanceY);
		}
		return Size::zero();
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
