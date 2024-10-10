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
#define FONT_SIZE_MAX 48

namespace slib
{

	namespace
	{
		static void ZeroMetrics(FontAtlasMetrics& metrics)
		{
			metrics.left = 0.0f;
			metrics.top = 0.0f;
			metrics.width = 0.0f;
			metrics.height = 0.0f;
			metrics.advanceX = 0.0f;
		}

		static void SetBlankMetrics(FontAtlasMetrics& metrics, sl_real advanceX)
		{
			metrics.left = 0.0f;
			metrics.top = 0.0f;
			metrics.width = 0.0f;
			metrics.height = 0.0f;
			metrics.advanceX = advanceX;
		}
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FontAtlasChar)

	FontAtlasChar::FontAtlasChar()
	{
		ZeroMetrics(metrics);
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FontAtlasCharImage)

	FontAtlasCharImage::FontAtlasCharImage()
	{
		ZeroMetrics(metrics);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FontAtlasBaseParam)

	FontAtlasBaseParam::FontAtlasBaseParam(): planeWidth(PLANE_WIDTH_DEFAULT), planeHeight(PLANE_HEIGHT_DEFAULT), maxPlanes(MAX_PLANES_DEFAULT)
	{
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FontAtlasParam)

	FontAtlasParam::FontAtlasParam()
	{
	}


	SLIB_DEFINE_OBJECT(FontAtlas, Object)

	FontAtlas::FontAtlas()
	{
		m_sourceHeight = 0.0f;
		m_fontScale = 1.0f;
		m_planeWidth = PLANE_WIDTH_DEFAULT;
		m_planeHeight = PLANE_HEIGHT_DEFAULT;
		m_maxPlanes = MAX_PLANES_DEFAULT;

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

		public:
			static Ref<FontAtlasImpl> create(const FontAtlasParam& param)
			{
				if (param.font.isNull()) {
					return sl_null;
				}
				sl_real fontSize = param.font->getSize();
				if (fontSize > FONT_SIZE_MAX) {
					fontSize = FONT_SIZE_MAX;
				} else if (fontSize < FONT_SIZE_MIN) {
					fontSize = FONT_SIZE_MIN;
				}
				Ref<Font> font = Font::create(param.font->getFamilyName(), fontSize, param.font->isBold());
				if (font.isNull()) {
					return sl_null;
				}
				sl_real sourceHeight = param.font->getFontHeight();
				sl_real fontHeight = font->getFontHeight();
				sl_uint32 planeWidth = param.planeWidth;
				if (!planeWidth) {
					planeWidth = ((sl_uint32)fontHeight) << 4;
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
				ret->initialize(param, sourceHeight, fontHeight, planeWidth, planeHeight);
				ret->m_font = Move(font);
				ret->m_currentPlane = Move(plane);
				ret->m_currentCanvas = Move(canvas);
				return ret;
			}

			sl_bool getCharImage_NoLock(sl_char32 ch, FontAtlasCharImage& _out) override
			{
				if (m_mapImage.get_NoLock(ch, &_out)) {
					return sl_true;
				}
				FontAtlasChar fac;
				if (!(getChar_NoLock(ch, sl_false, fac))) {
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

			sl_bool measureChar(sl_char32 ch, FontAtlasMetrics& _out) override
			{
				Size size = m_font->measureText(String::create(&ch, 1));
				_out.left = 0;
				_out.top = 0;
				_out.width = size.x;
				_out.height = size.y;
				_out.advanceX = size.x;
				return size.x > 0.0f;
			}

			Ref<Bitmap> drawChar(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height, sl_char32 ch) override
			{
				m_currentPlane->resetPixels(x, y, width, height, Color::Zero);
				m_currentCanvas->drawText(String::create(&ch, 1), (sl_real)x, (sl_real)y, m_font, Color::White);
				m_currentPlane->update(x, y, width, height);
				return m_currentPlane;
			}

			sl_bool createPlane() override
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

	void FontAtlas::initialize(const FontAtlasBaseParam& param, sl_real sourceHeight, sl_real fontHeight, sl_uint32 planeWidth, sl_uint32 planeHeight)
	{
		m_sourceHeight = sourceHeight;
		m_fontScale = sourceHeight / fontHeight;
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
		return m_sourceHeight;
	}

	sl_bool FontAtlas::getChar_NoLock(sl_char32 ch, FontAtlasChar& _out)
	{
		return getChar_NoLock(ch, sl_false, _out);
	}

	sl_bool FontAtlas::getChar(sl_char32 ch, FontAtlasChar& _out)
	{
		ObjectLocker lock(this);
		return getChar_NoLock(ch, sl_false, _out);
	}

	sl_bool FontAtlas::getCharImage(sl_char32 ch, FontAtlasCharImage& _out)
	{
		ObjectLocker lock(this);
		return getCharImage_NoLock(ch, _out);
	}

	sl_bool FontAtlas::getCharMetrics_NoLock(sl_char32 ch, FontAtlasMetrics& _out)
	{
		FontAtlasChar fac;
		if (getChar_NoLock(ch, sl_true, fac)) {
			_out = fac.metrics;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool FontAtlas::getCharMetrics(sl_char32 ch, FontAtlasMetrics& _out)
	{
		ObjectLocker lock(this);
		return getCharMetrics_NoLock(ch, _out);
	}

	Size FontAtlas::measureText(const StringParam& _str, sl_bool flagMultiLine)
	{
		StringData16 str(_str);
		if (str.isEmpty()) {
			return Size::zero();
		}
		ObjectLocker lock(this);
		sl_real lineWidth = 0;
		sl_real maxWidth = 0;
		sl_real totalHeight = 0;
		FontAtlasChar fac;
		sl_char16* data = str.getData();
		sl_size len = str.getLength();
		for (sl_size i = 0; i < len;) {
			sl_char32 ch;
			if (Charsets::getUnicode(ch, data, len, i)) {
				if (ch == '\r' || ch == '\n') {
					if (!flagMultiLine) {
						return Size(lineWidth, m_sourceHeight);
					}
					if (lineWidth > maxWidth) {
						maxWidth = lineWidth;
					}
					totalHeight += m_sourceHeight;
					lineWidth = 0;
					if (ch == '\r' && i < len) {
						if (data[i] == '\n') {
							i++;
						}
					}
				} else {
					if (getChar_NoLock((sl_char32)ch, sl_true, fac)) {
						lineWidth += fac.metrics.advanceX;
					}
				}
			} else {
				i++;
			}
		}
		if (lineWidth > maxWidth) {
			maxWidth = lineWidth;
		}
		totalHeight += m_sourceHeight;
		return Size(maxWidth, totalHeight);
	}

	sl_bool FontAtlas::getChar_NoLock(sl_char32 ch, sl_bool flagSizeOnly, FontAtlasChar& _out)
	{
		if (ch == ' ' || ch == '\r' || ch == '\n') {
			SetBlankMetrics(_out.metrics, m_sourceHeight * 0.3f);
			if (!flagSizeOnly) {
				_out.bitmap.setNull();
			}
			return sl_true;
		}
		if (ch == '\t') {
			SetBlankMetrics(_out.metrics, m_sourceHeight * 2.0f);
			if (!flagSizeOnly) {
				_out.bitmap.setNull();
			}
			return sl_true;
		}
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
			widthChar = (sl_uint32)(_out.metrics.width / m_fontScale);
			heightChar = (sl_uint32)(_out.metrics.height / m_fontScale);
		} else {
			FontAtlasMetrics metrics;
			if (!(measureChar(ch, metrics))) {
				ZeroMetrics(_out.metrics);
				m_map.put_NoLock(ch, _out);
				return sl_false;
			}
			_out.metrics.left = metrics.left * m_fontScale;
			_out.metrics.top = metrics.top * m_fontScale;
			_out.metrics.width = metrics.width * m_fontScale;
			_out.metrics.height = metrics.height * m_fontScale;
			_out.metrics.advanceX = metrics.advanceX * m_fontScale;
			_out.bitmap.setNull();
			if (flagSizeOnly) {
				return m_map.put_NoLock(ch, _out);
			}
			widthChar = (sl_uint32)(metrics.width);
			heightChar = (sl_uint32)(metrics.height);
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
				if (!(createPlane())) {
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
		_out.bitmap = drawChar(x, y, widthChar, heightChar, ch);

		m_currentPlaneX += widthChar;
		if (heightChar > m_currentPlaneRowHeight) {
			m_currentPlaneRowHeight = heightChar;
		}
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

	Ref<FontAtlas> FontAtlas::createStroker(sl_uint32 strokeWidth)
	{
		return sl_null;
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
