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

#ifndef CHECKHEADER_SLIB_GRAPHICS_CANVAS
#define CHECKHEADER_SLIB_GRAPHICS_CANVAS

#include "pen.h"
#include "brush.h"
#include "font.h"
#include "path.h"
#include "drawable.h"

#include "../core/time.h"
#include "../math/matrix3.h"

namespace slib
{

	enum class CanvasType
	{
		View = 0,
		Bitmap = 1,
		Image = 2,
		Render = 3
	};

	class Image;

	class SLIB_EXPORT Canvas : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		Canvas();

		~Canvas();

	public:
		CanvasType getType();

		void setType(CanvasType type);

		Time getTime();

		void setTime(const Time& time);

		Size getSize();

		void setSize(const Size& size);

		const Rectangle& getInvalidatedRect();

		void setInvalidatedRect(const Rectangle& rect);

		sl_real getAlpha();

		void setAlpha(sl_real alpha);

		sl_bool isAntiAlias();

		void setAntiAlias(sl_bool flag = sl_true);

	public:
		virtual void save() = 0;

		virtual void restore() = 0;


		virtual Rectangle getClipBounds() = 0;

		virtual void clipToRectangle(const Rectangle& rect) = 0;

		void clipToRectangle(sl_real x, sl_real y, sl_real width, sl_real height);


		virtual void clipToPath(const Ref<GraphicsPath>& path) = 0;

		virtual void clipToRoundRect(const Rectangle& rect, const Size& radius) = 0;

		void clipToRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real rx, sl_real ry);

		virtual void clipToEllipse(const Rectangle& rect) = 0;

		void clipToEllipse(sl_real x, sl_real y, sl_real width, sl_real height);


		// concat matrix to the left (pre-concat)
		virtual void concatMatrix(const Matrix3& matrix) = 0;

		virtual void translate(sl_real dx, sl_real dy);

		virtual void rotate(sl_real radians);

		virtual void rotate(sl_real cx, sl_real cy, sl_real radians);

		virtual void scale(sl_real sx, sl_real sy);


		virtual sl_bool measureChar(const Ref<Font>& font, sl_char32 ch, TextMetrics& _out) = 0;

		virtual sl_bool measureText(const Ref<Font>& font, const StringParam& text, sl_bool flagMultiLine, TextMetrics& _out) = 0;

		sl_bool measureText(const Ref<Font>& font, const StringParam& text, TextMetrics& _out);

		Size getTextAdvance(const Ref<Font>& font, const StringParam& text, sl_bool flagMultiLine = sl_false);

		class DrawTextParam
		{
		public:
			StringParam text;
			Ref<Font> font;
			Ref<FontAtlas> atlas;
			Color color;
			Alignment alignment;
			sl_bool flagMultiLine;

			sl_real x;
			sl_real y;
			sl_real width;
			sl_real height;

			sl_real strokeWidth;
			Color strokeColor;

			sl_real shadowOpacity;
			sl_real shadowRadius;
			Color shadowColor;
			Point shadowOffset;

		public:
			DrawTextParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DrawTextParam)
		};

		virtual void drawText(const DrawTextParam& param) = 0;

		void drawText(const StringParam& text, sl_real x, sl_real y, const Ref<Font>& font, const Color& color);

		void drawText(const StringParam& text, sl_real x, sl_real y, const Ref<Font>& font, const Color& color, const Alignment& alignment, sl_bool flagMultiLine = sl_false);

		void drawText(const StringParam& text, const Rectangle& rcDst, const Ref<Font>& font, const Color& color, const Alignment& alignment, sl_bool flagMultiLine = sl_false);

		virtual sl_bool measureText(const DrawTextParam& param, TextMetrics& _out) = 0;

		Size getTextAdvance(const DrawTextParam& param);


		virtual void drawLine(const Point& pt1, const Point& pt2, const Ref<Pen>& pen) = 0;

		void drawLine(sl_real x1, sl_real y1, sl_real x2, sl_real y2, const Ref<Pen>& pen);


		virtual void drawLines(const Point* points, sl_size pointCount, const Ref<Pen>& pen) = 0;

		void drawLines(const List<Point>& points, const Ref<Pen>& pen);


		virtual void drawArc(const Rectangle& rect, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen) = 0;

		void drawArc(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen);


		virtual void drawRectangle(const Rectangle& rect, const Ref<Pen>& pen, const Ref<Brush>& brush) = 0;

		virtual void drawRectangle(const Rectangle& rect, const Ref<Pen>& pen, const Color& fillColor) = 0;

		void drawRectangle(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Pen>& pen, const Ref<Brush>& brush);

		void drawRectangle(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Pen>& pen, const Color& fillColor);

		void drawRectangle(const Rectangle& rc, const Ref<Pen>& pen);

		void drawRectangle(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Pen>& pen);

		void fillRectangle(const Rectangle& rc, const Ref<Brush>& brush);

		void fillRectangle(const Rectangle& rc, const Color& color);

		void fillRectangle(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Brush>& brush);

		void fillRectangle(sl_real x, sl_real y, sl_real width, sl_real height, const Color& color);


		virtual void drawRoundRect(const Rectangle& rect, const Size& radius, const Ref<Pen>& pen, const Ref<Brush>& brush) = 0;

		virtual void drawRoundRect(const Rectangle& rect, const Size& radius, const Ref<Pen>& pen, const Color& fillColor) = 0;

		void drawRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real rx, sl_real ry, const Ref<Pen>& pen, const Ref<Brush>& brush);

		void drawRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real rx, sl_real ry, const Ref<Pen>& pen, const Color& fillColor);

		void drawRoundRect(const Rectangle& rc, const Size& radius, const Ref<Pen>& pen);

		void drawRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real rx, sl_real ry, const Ref<Pen>& pen);

		void fillRoundRect(const Rectangle& rc, const Size& radius, const Ref<Brush>& brush);

		void fillRoundRect(const Rectangle& rc, const Size& radius, const Color& color);

		void fillRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real rx, sl_real ry, const Ref<Brush>& brush);

		void fillRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real rx, sl_real ry, const Color& color);


		virtual void drawEllipse(const Rectangle& rect, const Ref<Pen>& pen, const Ref<Brush>& brush) = 0;

		virtual void drawEllipse(const Rectangle& rect, const Ref<Pen>& pen, const Color& fillColor) = 0;

		void drawEllipse(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Pen>& pen, const Ref<Brush>& brush);

		void drawEllipse(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Pen>& pen, const Color& fillColor);

		void drawEllipse(const Rectangle& rc, const Ref<Pen>& pen);

		void drawEllipse(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Pen>& pen);

		void fillEllipse(const Rectangle& rc, const Ref<Brush>& brush);

		void fillEllipse(const Rectangle& rc, const Color& color);

		void fillEllipse(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Brush>& brush);

		void fillEllipse(sl_real x, sl_real y, sl_real width, sl_real height, const Color& color);


		virtual void drawPolygon(const Point* points, sl_size pointCount, const Ref<Pen>& pen, const Ref<Brush>& brush, FillMode fillMode = FillMode::Alternate) = 0;

		virtual void drawPolygon(const Point* points, sl_size pointCount, const Ref<Pen>& pen, const Color& fillColor, FillMode fillMode = FillMode::Alternate) = 0;

		void drawPolygon(const List<Point>& points, const Ref<Pen>& pen, const Ref<Brush>& brush, FillMode fillMode = FillMode::Alternate);

		void drawPolygon(const List<Point>& points, const Ref<Pen>& pen, const Color& fillColor, FillMode fillMode = FillMode::Alternate);

		void drawPolygon(const Point* points, sl_size pointCount, const Ref<Pen>& pen);

		void drawPolygon(const List<Point>& points, const Ref<Pen>& pen);

		void fillPolygon(const Point* points, sl_size pointCount, const Ref<Brush>& brush, FillMode fillMode = FillMode::Alternate);

		void fillPolygon(const Point* points, sl_size pointCount, const Color& color, FillMode fillMode = FillMode::Alternate);

		void fillPolygon(const List<Point>& points, const Ref<Brush>& brush, FillMode fillMode = FillMode::Alternate);

		void fillPolygon(const List<Point>& points, const Color& color, FillMode fillMode = FillMode::Alternate);


		virtual void drawPie(const Rectangle& rect, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen, const Ref<Brush>& brush) = 0;

		virtual void drawPie(const Rectangle& rect, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen, const Color& fillColor) = 0;

		void drawPie(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen, const Ref<Brush>& brush);

		void drawPie(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen, const Color& fillColor);

		void drawPie(const Rectangle& rc, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen);

		void drawPie(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen);

		void fillPie(const Rectangle& rc, sl_real startDegrees, sl_real sweepDegrees, const Ref<Brush>& brush);

		void fillPie(const Rectangle& rc, sl_real startDegrees, sl_real sweepDegrees, const Color& color);

		void fillPie(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees, const Ref<Brush>& brush);

		void fillPie(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees, const Color& color);


		virtual void drawPath(const Ref<GraphicsPath>& path, const Ref<Pen>& pen, const Ref<Brush>& brush) = 0;

		virtual void drawPath(const Ref<GraphicsPath>& path, const Ref<Pen>& pen, const Color& fillColor) = 0;

		void drawPath(const Ref<GraphicsPath>& path, const Ref<Pen>& pen);

		void fillPath(const Ref<GraphicsPath>& path, const Ref<Brush>& brush);

		void fillPath(const Ref<GraphicsPath>& path, const Color& color);


		void drawShadowRectangle(sl_real x, sl_real y, sl_real width, sl_real height, const Color& color, sl_real shadowRadius);

		void drawShadowRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real roundRadius, const Color& color, sl_real shadowRadius);

		void drawShadowCircle(sl_real centerX, sl_real centerY, sl_real circleRadius, const Color& color, sl_real shadowRadius);

		
		typedef Drawable::DrawParam DrawParam;

		virtual void draw(const Rectangle& rectDst, const Ref<Drawable>& src, const Rectangle& rectSrc, const DrawParam& param) = 0;

		void draw(const Rectangle& rectDst, const Ref<Drawable>& src, const Rectangle& rectSrc);

		virtual void draw(const Rectangle& rectDst, const Ref<Drawable>& src, const DrawParam& param) = 0;

		void draw(const Rectangle& rectDst, const Ref<Drawable>& src);

		void draw(sl_real xDst, sl_real yDst, sl_real widthDst, sl_real heightDst, const Ref<Drawable>& src, sl_real xSrc, sl_real ySrc, sl_real widthSrc, sl_real heightSrc, const DrawParam& param);

		void draw(sl_real xDst, sl_real yDst, sl_real widthDst, sl_real heightDst, const Ref<Drawable>& src, sl_real xSrc, sl_real ySrc, sl_real widthSrc, sl_real heightSrc);

		void draw(sl_real xDst, sl_real yDst, sl_real widthDst, sl_real heightDst, const Ref<Drawable>& drawable, const DrawParam& param);

		void draw(sl_real xDst, sl_real yDst, sl_real widthDst, sl_real heightDst, const Ref<Drawable>& drawable);

		virtual void draw(sl_real xDst, sl_real yDst, const Ref<Drawable>& drawable, const DrawParam& param) = 0;

		void draw(sl_real xDst, sl_real yDst, const Ref<Drawable>& drawable);

		virtual void draw(const Rectangle& rectDst, const Ref<Drawable>& src, ScaleMode scaleMode, const Alignment& alignment, const DrawParam& param) = 0;

		void draw(const Rectangle& rectDst, const Ref<Drawable>& src, ScaleMode scaleMode, const Alignment& alignment);


		virtual sl_bool isSupportedDrawable(const Ref<Drawable>& drawable) = 0;

		virtual Ref<Drawable> createDrawableCacheForImage(const Ref<Image>& image) = 0;

		virtual sl_bool updateDrawableCacheForImage(Drawable* drawable, Image* image) = 0;

	protected:
		// ignores `text`, `font`, `flagMultiLine`, `alignment`, `x`, `y`, `width` and `height`
		virtual void onDrawText(const StringParam& text, sl_real x, sl_real y, const Ref<Font>& font, const DrawTextParam& param) = 0;

		virtual void onDraw(const Rectangle& rectDst, const Ref<Drawable>& src, const Rectangle& rectSrc, const DrawParam& param) = 0;

		virtual void onDrawAll(const Rectangle& rectDst, const Ref<Drawable>& src, const DrawParam& param) = 0;

	protected:
		virtual void _setAlpha(sl_real alpha);

		virtual void _setAntiAlias(sl_bool flag);

	protected:
		CanvasType m_type;
		Time m_time;
		Size m_size;
		Rectangle m_invalidatedRect;
		sl_real m_alpha;
		sl_bool m_flagAntiAlias;

	};

	class SLIB_EXPORT CanvasStateScope
	{
	public:
		Canvas* canvas;

	public:
		CanvasStateScope();

		CanvasStateScope(Canvas* canvas);

		~CanvasStateScope();

	public:
		void save(Canvas* canvas);

		void restore();

	};

	class SLIB_EXPORT CanvasAntiAliasScope
	{
	public:
		Canvas* canvas;
		sl_bool flagOriginalAntiAlias;

	public:
		CanvasAntiAliasScope(Canvas* canvas, sl_bool flagAntiAlias);

		~CanvasAntiAliasScope();

	};

}

#endif
