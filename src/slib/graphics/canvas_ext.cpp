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

#include "slib/graphics/canvas.h"

#include "slib/graphics/font_atlas.h"
#include "slib/graphics/util.h"
#include "slib/graphics/image.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(CanvasExt, Canvas)

	CanvasExt::CanvasExt()
	{
	}

	CanvasExt::~CanvasExt()
	{
	}

	void CanvasExt::clipToRoundRect(const Rectangle& rect, const Size& radius)
	{
		Ref<GraphicsPath> path = GraphicsPath::create();
		if (path.isNotNull()) {
			path->addRoundRect(rect, radius);
			clipToPath(path);
		}
	}

	void CanvasExt::clipToEllipse(const Rectangle& rect)
	{
		Ref<GraphicsPath> path = GraphicsPath::create();
		if (path.isNotNull()) {
			path->addEllipse(rect);
			clipToPath(path);
		}
	}

	sl_bool CanvasExt::measureChar(const Ref<Font>& font, sl_char32 ch, TextMetrics& _out)
	{
		if (font.isNotNull()) {
			return font->measureChar(ch, _out);
		}
		return sl_false;
	}

	sl_bool CanvasExt::measureText(const Ref<Font>& font, const StringParam& text, sl_bool flagMultiLine, TextMetrics& _out)
	{
		if (font.isNotNull()) {
			return font->measureText(text, flagMultiLine, _out);
		}
		return sl_false;
	}

	void CanvasExt::drawText(const Canvas::DrawTextParam& param)
	{
		const Ref<Font>& font = param.font;
		if (font.isNull()) {
			drawTextByAtlas(param);
			return;
		}
		if (!(param.flagMultiLine)) {
			if (param.text.isEmpty()) {
				return;
			}
			if (param.alignment == Alignment::TopLeft) {
				onDrawText(param.text, param.x, param.y, font, param);
			} else {
				Size size = getTextAdvance(font, param.text, sl_false);
				Alignment hAlign = param.alignment & Alignment::HorizontalMask;
				Alignment vAlign = param.alignment & Alignment::VerticalMask;
				sl_real x = param.x;
				sl_real y = param.y;
				if (hAlign == Alignment::Right) {
					x += param.width - size.x;
				} else if (hAlign != Alignment::Left) {
					x += (param.width - size.x) / 2;
				}
				if (vAlign == Alignment::Bottom) {
					y += param.height - size.y;
				} else if (vAlign != Alignment::Top){
					y += (param.height - size.y) / 2;
				}
				onDrawText(param.text, x, y, font, param);
			}
			return;
		}

		StringData32 text(param.text);
		sl_size len = text.getLength();
		if (!len) {
			return;
		}
		sl_char32* data = text.getData();

		Size size;
		Point pt;
		if (param.alignment == Alignment::TopLeft) {
			size.x = 0.0f;
			size.y = 0.0f;
			pt.x = param.x;
			pt.y = param.y;
		} else {
			size = getTextAdvance(font, text, sl_true);
			pt = GraphicsUtil::calculateAlignPosition(Rectangle(param.x, param.y, param.x + param.width, param.y + param.height), size.x, size.y, param.alignment);
		}
		Alignment hAlign = param.alignment & Alignment::HorizontalMask;

		sl_size startLine = 0;
		sl_real y = pt.y;
		for (sl_size i = 0; i <= len; i++) {
			sl_char32 ch;
			if (i < len) {
				ch = data[i];
			} else {
				ch = '\n';
			}
			if (ch == '\r' || ch == '\n') {
				if (i > startLine) {
					StringView32 line(data + startLine, i - startLine);
					if (hAlign == Alignment::Left && i + 1 >= len) {
						onDrawText(line, pt.x, y, font, param);
						break;
					}
					TextMetrics tm;
					if (measureText(font, line, tm)) {
						sl_real x;
						if (hAlign == Alignment::Left) {
							x = pt.x;
						} else if (hAlign == Alignment::Right) {
							x = pt.x + size.x - tm.advanceX;
						} else {
							x = pt.x + (size.x - tm.advanceX) / 2;
						}
						onDrawText(line, x, y, font, param);
						y += tm.advanceY;
					}
				}
				if (ch == '\r' && i + 1 < len) {
					if (data[i + 1] == '\n') {
						i++;
					}
				}
				startLine = i + 1;
			}
		}
	}

	void CanvasExt::drawTextByAtlas(const Canvas::DrawTextParam& param)
	{
		const Ref<FontAtlas>& atlas = param.atlas;
		if (atlas.isNull()) {
			return;
		}
		if (!(param.flagMultiLine)) {
			if (param.text.isEmpty()) {
				return;
			}
			if (param.alignment == Alignment::TopLeft) {
				onDrawTextByAtlas(param.text, param.x, param.y, atlas, param);
			} else {
				Size size = atlas->getTextAdvance(param.text, sl_false);
				Alignment hAlign = param.alignment & Alignment::HorizontalMask;
				Alignment vAlign = param.alignment & Alignment::VerticalMask;
				sl_real x = param.x;
				sl_real y = param.y;
				if (hAlign == Alignment::Right) {
					x += param.width - size.x;
				} else if (hAlign != Alignment::Left) {
					x += (param.width - size.x) / 2;
				}
				if (vAlign == Alignment::Bottom) {
					y += param.height - size.y;
				} else if (vAlign != Alignment::Top) {
					y += (param.height - size.y) / 2;
				}
				onDrawTextByAtlas(param.text, x, y, atlas, param);
			}
			return;
		}

		StringData32 text(param.text);
		sl_size len = text.getLength();
		if (!len) {
			return;
		}
		sl_char32* data = text.getData();

		Size size;
		Point pt;
		if (param.alignment == Alignment::TopLeft) {
			size.x = 0.0f;
			size.y = 0.0f;
			pt.x = param.x;
			pt.y = param.y;
		} else {
			size = atlas->getTextAdvance(text, sl_true);
			pt = GraphicsUtil::calculateAlignPosition(Rectangle(param.x, param.y, param.x + param.width, param.y + param.height), size.x, size.y, param.alignment);
		}
		Alignment hAlign = param.alignment & Alignment::HorizontalMask;

		sl_size startLine = 0;
		sl_real y = pt.y;
		for (sl_size i = 0; i <= len; i++) {
			sl_char32 ch;
			if (i < len) {
				ch = data[i];
			} else {
				ch = '\n';
			}
			if (ch == '\r' || ch == '\n') {
				if (i > startLine) {
					StringView32 line(data + startLine, i - startLine);
					if (hAlign == Alignment::Left && i + 1 >= len) {
						onDrawTextByAtlas(line, pt.x, y, atlas, param);
						break;
					}
					TextMetrics tm;
					if (atlas->measureText(line, tm)) {
						sl_real x;
						if (hAlign == Alignment::Left) {
							x = pt.x;
						} else if (hAlign == Alignment::Right) {
							x = pt.x + size.x - tm.advanceX;
						} else {
							x = pt.x + (size.x - tm.advanceX) / 2;
						}
						onDrawTextByAtlas(line, x, y, atlas, param);
						y += tm.advanceY;
					}
				}
				if (ch == '\r' && i + 1 < len) {
					if (data[i + 1] == '\n') {
						i++;
					}
				}
				startLine = i + 1;
			}
		}
	}

	void CanvasExt::onDrawTextByAtlas(const StringParam& _text, sl_real x, sl_real y, const Ref<FontAtlas>& atlas, const DrawTextParam& param)
	{
		StringData32 text(_text);
		sl_size len = text.getLength();
		if (!len) {
			return;
		}
		sl_char32* data = text.getData();

		DrawParam dp;
		dp.colorMatrix.setOverlay(param.color);
		dp.useColorMatrix = sl_true;

		FontAtlasChar fac;
		sl_real fx = x;
		{
			ObjectLocker lock(atlas.get());
			for (sl_size i = 0; i < len; i++) {
				sl_char32 ch = data[i];
				if (atlas->getChar_NoLock(ch, fac)) {
					if (fac.bitmap.isNotNull()) {
						fac.metrics.left += fx;
						fac.metrics.top += y;
						fac.metrics.right += fx;
						fac.metrics.bottom += y;
						draw(fac.metrics, fac.bitmap, fac.region, dp);
					}
					fx += fac.metrics.advanceX;
				}
			}
		}
	}

	void CanvasExt::drawRectangle(const Rectangle& rect, const Ref<Pen>& pen, const Color& fillColor)
	{
		((Canvas*)this)->drawRectangle(rect, pen, Brush::createSolidBrush(fillColor));
	}

	void CanvasExt::drawRoundRect(const Rectangle& rect, const Size& radius, const Ref<Pen>& pen, const Color& fillColor)
	{
		((Canvas*)this)->drawRoundRect(rect, radius, pen, Brush::createSolidBrush(fillColor));
	}

	void CanvasExt::drawEllipse(const Rectangle& rect, const Ref<Pen>& pen, const Color& fillColor)
	{
		((Canvas*)this)->drawEllipse(rect, pen, Brush::createSolidBrush(fillColor));
	}

	void CanvasExt::drawPolygon(const Point* points, sl_size nPoints, const Ref<Pen>& pen, const Color& fillColor, FillMode fillMode)
	{
		((Canvas*)this)->drawPolygon(points, nPoints, pen, Brush::createSolidBrush(fillColor), fillMode);
	}

	void CanvasExt::drawPie(const Rectangle& rect, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen, const Color& fillColor)
	{
		((Canvas*)this)->drawPie(rect, startDegrees, sweepDegrees, pen, Brush::createSolidBrush(fillColor));
	}

	void CanvasExt::drawPath(const Ref<GraphicsPath>& path, const Ref<Pen>& pen, const Color& fillColor)
	{
		((Canvas*)this)->drawPath(path, pen, Brush::createSolidBrush(fillColor));
	}

	void CanvasExt::draw(const Rectangle& rectDst, const Ref<Drawable>& src, const Rectangle& rectSrc, const DrawParam& param)
	{
		if (src.isNull()) {
			return;
		}
		if (param.isTransparent()) {
			return;
		}
		if (rectDst.getWidth() < SLIB_EPSILON) {
			return;
		}
		if (rectDst.getHeight() < SLIB_EPSILON) {
			return;
		}
		if (rectSrc.getWidth() < SLIB_EPSILON) {
			return;
		}
		if (rectSrc.getHeight() < SLIB_EPSILON) {
			return;
		}
		onDraw(rectDst, src, rectSrc, param);
	}

	void CanvasExt::draw(const Rectangle& rectDst, const Ref<Drawable>& src, const DrawParam& param)
	{
		if (src.isNull()) {
			return;
		}
		if (param.isTransparent()) {
			return;
		}
		if (rectDst.getWidth() < SLIB_EPSILON) {
			return;
		}
		if (rectDst.getHeight() < SLIB_EPSILON) {
			return;
		}
		sl_real sw = src->getDrawableWidth();
		if (sw < SLIB_EPSILON) {
			return;
		}
		sl_real sh = src->getDrawableHeight();
		if (sh < SLIB_EPSILON) {
			return;
		}
		onDrawAll(rectDst, src, param);
	}

	void CanvasExt::draw(sl_real xDst, sl_real yDst, const Ref<Drawable>& src, const DrawParam& param)
	{
		if (src.isNull()) {
			return;
		}
		if (param.isTransparent()) {
			return;
		}
		sl_real sw = src->getDrawableWidth();
		if (sw < SLIB_EPSILON) {
			return;
		}
		sl_real sh = src->getDrawableHeight();
		if (sh < SLIB_EPSILON) {
			return;
		}
		Rectangle rectDst(xDst, yDst, xDst + sw, yDst + sh);
		onDrawAll(rectDst, src, param);
	}

	void CanvasExt::draw(const Rectangle& rectDst, const Ref<Drawable>& source, ScaleMode scaleMode, const Alignment& alignment, const DrawParam& param)
	{
		if (source.isNull()) {
			return;
		}
		if (param.isTransparent()) {
			return;
		}
		if (scaleMode == ScaleMode::Cover) {
			sl_real dw = rectDst.getWidth();
			if (dw < SLIB_EPSILON) {
				return;
			}
			sl_real dh = rectDst.getHeight();
			if (dh < SLIB_EPSILON) {
				return;
			}
			sl_real sw = source->getDrawableWidth();
			if (sw < SLIB_EPSILON) {
				return;
			}
			sl_real sh = source->getDrawableHeight();
			if (sh < SLIB_EPSILON) {
				return;
			}
			sl_real fw = sw / dw;
			sl_real fh = sh / dh;
			sl_real tw, th;
			if (fw > fh) {
				th = sh;
				tw = dw * fh;
			} else {
				tw = sw;
				th = dh * fw;
			}
			Rectangle rectSrc;
			rectSrc.left = 0;
			rectSrc.top = 0;
			rectSrc.right = sw;
			rectSrc.bottom = sh;
			Point pt = GraphicsUtil::calculateAlignPosition(rectSrc, tw, th, alignment);
			rectSrc.left = pt.x;
			rectSrc.top = pt.y;
			rectSrc.right = rectSrc.left + tw;
			rectSrc.bottom = rectSrc.top + th;
			onDraw(rectDst, source, rectSrc, param);
		} else {
			Rectangle rectDraw;
			if (GraphicsUtil::calculateAlignRectangle(rectDraw, rectDst, source->getDrawableWidth(), source->getDrawableHeight(), scaleMode, alignment)) {
				onDrawAll(rectDraw, source, param);
			}
		}
	}

	void CanvasExt::onDraw(const Rectangle& rectDst, const Ref<Drawable>& src, const Rectangle& rectSrc, const DrawParam& param)
	{
		src->onDraw(this, rectDst, rectSrc, param);
	}

	void CanvasExt::onDrawAll(const Rectangle& rectDst, const Ref<Drawable>& src, const DrawParam& param)
	{
		src->onDrawAll(this, rectDst, param);
	}

	sl_bool CanvasExt::isSupportedDrawable(const Ref<Drawable>& drawable)
	{
		return sl_true;
	}

	Ref<Drawable> CanvasExt::createDrawableCacheForImage(const Ref<Image>& image)
	{
		return PlatformDrawable::create(image);
	}

	sl_bool CanvasExt::updateDrawableCacheForImage(Drawable* drawable, Image* image)
	{
		if (drawable->isBitmap()) {
			Bitmap* bitmap = (Bitmap*)drawable;
			bitmap->writePixels(0, 0, image->getWidth(), image->getHeight(), image->getColors(), image->getStride());
			return sl_true;
		}
		return sl_false;
	}

}
