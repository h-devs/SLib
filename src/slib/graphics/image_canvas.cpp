/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/graphics/image.h"

#include "slib/graphics/font_atlas.h"

namespace slib
{
	
	namespace priv
	{
		namespace image
		{

			class CanvasImpl : public CanvasExt
			{
				SLIB_DECLARE_OBJECT

			public:
				Ref<Image> image;

			public:
				CanvasImpl(Image* _image) : image(_image)
				{
					setType(CanvasType::Image);
					setSize(Size((sl_real)(_image->getWidth()), (sl_real)(_image->getHeight())));
				}

				~CanvasImpl()
				{
				}

			public:
				void save() override
				{
				}

				void restore() override
				{
				}

				Rectangle getClipBounds() override
				{
					Size size = getSize();
					return Rectangle(0, 0, size.x, size.y);
				}

				void clipToRectangle(const Rectangle& rect) override
				{
				}

				void clipToPath(const Ref<GraphicsPath>& path) override
				{
				}

				void concatMatrix(const Matrix3& matrix) override
				{
				}

				void _drawLine(const Point& pt1, const Point& pt2, const Color& color)
				{
					if (isAntiAlias()) {
						image->drawSmoothLine((sl_int32)(pt1.x), (sl_int32)(pt1.y), (sl_int32)(pt2.x), (sl_int32)(pt2.y), color);
					} else {
						image->drawLine((sl_int32)(pt1.x), (sl_int32)(pt1.y), (sl_int32)(pt2.x), (sl_int32)(pt2.y), color);
					}
				}

				void drawLine(const Point& pt1, const Point& pt2, const Ref<Pen>& pen) override
				{
					if (pen.isNull()) {
						return;
					}
					Color color = pen->getColor();
					if (color.isZero()) {
						return;
					}
					_drawLine(pt1, pt2, color);
				}

				void drawLines(const Point* points, sl_uint32 countPoints, const Ref<Pen>& pen) override
				{
					if (countPoints < 2) {
						return;
					}
					if (pen.isNull()) {
						return;
					}
					Color color = pen->getColor();
					if (color.isZero()) {
						return;
					}
					for (sl_uint32 i = 1; i < countPoints; i++) {
						_drawLine(points[i - 1], points[i], color);
					}
				}

				void drawArc(const Rectangle& rect, sl_real startDegrees, sl_real endDegrees, const Ref<Pen>& pen) override
				{
				}

				void drawRectangle(const Rectangle& rect, const Ref<Pen>& pen, const Ref<Brush>& brush) override
				{
					if (brush.isNotNull()) {
						Color color = brush->getColor();
						if (color.isNotZero()) {
							image->fillRectangle((sl_int32)(rect.left), (sl_int32)(rect.top), (sl_int32)(rect.right), (sl_int32)(rect.bottom), color);
						}
					}
					if (pen.isNotNull()) {
						Color color = pen->getColor();
						if (color.isNotZero()) {
							image->drawRectangle((sl_int32)(rect.left), (sl_int32)(rect.top), (sl_int32)(rect.right), (sl_int32)(rect.bottom), color);
						}
					}
				}

				void drawRoundRect(const Rectangle& rect, const Size& radius, const Ref<Pen>& pen, const Ref<Brush>& brush) override
				{
				}

				void drawEllipse(const Rectangle& rect, const Ref<Pen>& pen, const Ref<Brush>& brush) override
				{
					if (pen.isNotNull()) {
						Color color = pen->getColor();
						if (color.isNotZero()) {
							if (isAntiAlias()) {
								image->drawSmoothEllipse((sl_int32)(rect.left), (sl_int32)(rect.top), (sl_int32)(rect.right), (sl_int32)(rect.bottom), color);
							} else {
								image->drawEllipse((sl_int32)(rect.left), (sl_int32)(rect.top), (sl_int32)(rect.right), (sl_int32)(rect.bottom), color);
							}
						}
					}
				}

				void drawPolygon(const Point* points, sl_uint32 countPoints, const Ref<Pen>& pen, const Ref<Brush>& brush, FillMode fillMode) override
				{
				}

				void drawPie(const Rectangle& rect, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen, const Ref<Brush>& brush) override
				{
				}

				void drawPath(const Ref<GraphicsPath>& path, const Ref<Pen>& pen, const Ref<Brush>& brush) override
				{
				}

				void onDraw(const Rectangle& rectDst, const Ref<Drawable>& src, const Rectangle& rectSrc, const DrawParam& param) override
				{
					if (src->isBitmap()) {
						_drawBitmap(rectDst, (Bitmap*)(src.get()), rectSrc, param);
					} else {
						CanvasExt::onDraw(rectDst, src, rectSrc, param);
					}
				}

				void onDrawAll(const Rectangle& rectDst, const Ref<Drawable>& src, const DrawParam& param) override
				{
					if (src->isBitmap()) {
						_drawBitmap(rectDst, (Bitmap*)(src.get()), Rectangle(0, 0, src->getDrawableWidth(), src->getDrawableHeight()), param);
					} else {
						CanvasExt::onDrawAll(rectDst, src, param);
					}
				}

				void _drawBitmap(const Rectangle& rectDst, Bitmap* _src, const Rectangle& rectSrc, const DrawParam& param)
				{
					Ref<Image> src = _src->toImage();
					if (src.isNotNull()) {
						image->drawImage(rectDst, src, rectSrc);
					}
				}

				void onDrawText(const StringParam& _text, sl_real x, sl_real y, const Ref<Font>& font, const DrawTextParam& param) override
				{
					StringData16 text(_text);
					if (text.isEmpty()) {
						return;
					}

					sl_char16* arrChar = text.getData();
					sl_size len = text.getLength();
					sl_real fontHeight = font->getFontHeight();

					Ref<FontAtlas> fa = font->getSharedAtlas();
					if (fa.isNull()) {
						return;
					}

					FontAtlasCharImage fac;
					Color color = param.color;
					sl_real fx = x;

					for (sl_size i = 0; i < len; i++) {

						sl_char32 ch = arrChar[i];
						if (ch >= 0xD800 && ch < 0xE000) {
							if (i + 1 < len) {
								sl_uint32 ch1 = (sl_uint32)((sl_uint16)arrChar[++i]);
								if (ch < 0xDC00 && ch1 >= 0xDC00 && ch1 < 0xE000) {
									ch = (sl_char32)(((ch - 0xD800) << 10) | (ch1 - 0xDC00)) + 0x10000;
								} else {
									ch = 0;
								}
							} else {
								ch = 0;
							}
						}

						if (ch) {

							if (fa->getCharImage(ch, fac)) {

								sl_real fw = fac.fontWidth;
								sl_real fh = fac.fontHeight;
								sl_real fxn = fx + fw;

								if (fac.image.isNotNull()) {

									Rectangle rcDst;
									rcDst.left = fx;
									rcDst.right = fxn;
									rcDst.top = y + (fontHeight - fh);
									rcDst.bottom = rcDst.top + fh;

									image->drawImage(rcDst, fac.image);

								}

								fx = fxn;
							}
						}
					}

					if (font->isStrikeout() || font->isUnderline()) {
						FontMetrics fm;
						font->getFontMetrics(fm);
						if (font->isUnderline()) {
							sl_real yLine = y + fm.leading + fm.ascent;
							_drawLine(Point(x, yLine), Point(fx, yLine), param.color);
						}
						if (font->isStrikeout()) {
							sl_real yLine = y + fm.leading + fm.ascent / 2;
							_drawLine(Point(x, yLine), Point(fx, yLine), param.color);
						}
					}

				}

				Size measureText(const Ref<Font>& _font, const StringParam& text, sl_bool flagMultiLine) override
				{
					if (text.isEmpty()) {
						return Size::zero();
					}
					Ref<Font> font = _font;
					if (font.isNull()) {
						return Size::zero();
					}
					Ref<FontAtlas> fa = font->getSharedAtlas();
					if (fa.isNull()) {
						return Size::zero();
					}
					return fa->measureText(text, flagMultiLine);
				}

			};

			SLIB_DEFINE_OBJECT(CanvasImpl, CanvasExt)

		}
	}

	using namespace priv::image;

	Ref<Canvas> Image::getCanvas()
	{
		return new CanvasImpl(this);
	}

}
