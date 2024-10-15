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

#ifndef CHECKHEADER_SLIB_GRAPHICS_CANVAS_EXT
#define CHECKHEADER_SLIB_GRAPHICS_CANVAS_EXT

#include "canvas.h"

namespace slib
{

	class SLIB_EXPORT CanvasExt : public Canvas
	{
		SLIB_DECLARE_OBJECT

	public:
		CanvasExt();

		~CanvasExt();

	public:
		using Canvas::clipToRoundRect;
		void clipToRoundRect(const Rectangle& rect, const Size& radius) override;

		using Canvas::clipToEllipse;
		void clipToEllipse(const Rectangle& rect) override;

		using Canvas::measureChar;
		sl_bool measureChar(const Ref<Font>& font, sl_char32 ch, TextMetrics& _out) override;

		using Canvas::measureText;
		sl_bool measureText(const Ref<Font>& font, const StringParam& text, sl_bool flagMultiLine, TextMetrics& _out) override;
		sl_bool measureText(const DrawTextParam& param, TextMetrics& _out) override;

		using Canvas::drawText;
		void drawText(const DrawTextParam& param) override;

		using Canvas::drawRectangle;
		void drawRectangle(const Rectangle& rect, const Ref<Pen>& pen, const Color& fillColor) override;

		using Canvas::drawRoundRect;
		void drawRoundRect(const Rectangle& rect, const Size& radius, const Ref<Pen>& pen, const Color& fillColor) override;

		using Canvas::drawEllipse;
		void drawEllipse(const Rectangle& rect, const Ref<Pen>& pen, const Color& fillColor) override;

		using Canvas::drawPolygon;
		void drawPolygon(const Point* points, sl_size pointCount, const Ref<Pen>& pen, const Color& fillColor, FillMode fillMode = FillMode::Alternate) override;

		using Canvas::drawPie;
		void drawPie(const Rectangle& rect, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen, const Color& fillColor) override;

		using Canvas::drawPath;
		void drawPath(const Ref<GraphicsPath>& path, const Ref<Pen>& pen, const Color& fillColor) override;

		using Canvas::draw;
		void draw(const Rectangle& rectDst, const Ref<Drawable>& src, const Rectangle& rectSrc, const DrawParam& param) override;

		void draw(const Rectangle& rectDst, const Ref<Drawable>& src, const DrawParam& param) override;

		void draw(sl_real xDst, sl_real yDst, const Ref<Drawable>& drawable, const DrawParam& param) override;

		void draw(const Rectangle& rectDst, const Ref<Drawable>& src, ScaleMode scaleMode, const Alignment& alignment, const DrawParam& param) override;

		sl_bool isSupportedDrawable(const Ref<Drawable>& drawable) override;

		Ref<Drawable> createDrawableCacheForImage(const Ref<Image>& image) override;

		sl_bool updateDrawableCacheForImage(Drawable* drawable, Image* image) override;

	protected:
		void onDraw(const Rectangle& rectDst, const Ref<Drawable>& src, const Rectangle& rectSrc, const DrawParam& param) override;

		void onDrawAll(const Rectangle& rectDst, const Ref<Drawable>& src, const DrawParam& param) override;

		using Canvas::onDrawText;
		void onDrawText(const StringParam& text, sl_real x, sl_real y, const DrawTextParam& param);

		virtual void onDrawTextByAtlas(const StringParam& text, sl_real x, sl_real y, const Ref<FontAtlas>& atlas, const DrawTextParam& param);

		sl_bool measureText(const DrawTextParam& param, const StringParam& text, TextMetrics& _out);

		void strokeTextByPath(const StringParam& text, sl_real x, sl_real y, const Ref<Font>& font, sl_real strokeWidth, const Color& color);

	};

}

#endif
