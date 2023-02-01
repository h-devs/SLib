/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/math/transform2d.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(DrawTextParam)

	DrawTextParam::DrawTextParam():
		color(Color::Black), alignment(Alignment::TopLeft), flagMultiLine(sl_false),
		x(0), y(0), width(0), height(0),
		shadowOpacity(0), shadowRadius(3), shadowColor(Color::Black), shadowOffset(0, 0)
	{
	}

	SLIB_DEFINE_OBJECT(Canvas, Object)

	Canvas::Canvas()
	{
		m_time = Time::now();
		m_alpha = 1;
		m_flagAntiAlias = sl_true;
		m_type = CanvasType::Bitmap;
	}

	Canvas::~Canvas()
	{
	}

	CanvasType Canvas::getType()
	{
		return m_type;
	}

	void Canvas::setType(CanvasType type)
	{
		m_type = type;
	}

	Time Canvas::getTime()
	{
		return m_time;
	}

	void Canvas::setTime(const Time& time)
	{
		m_time = time;
	}

	Size Canvas::getSize()
	{
		return m_size;
	}

	void Canvas::setSize(const Size& size)
	{
		m_size = size;
		m_invalidatedRect.left = 0;
		m_invalidatedRect.top = 0;
		m_invalidatedRect.right = size.x;
		m_invalidatedRect.bottom = size.y;
	}

	const Rectangle& Canvas::getInvalidatedRect()
	{
		return m_invalidatedRect;
	}

	void Canvas::setInvalidatedRect(const Rectangle& rect)
	{
		m_invalidatedRect = rect;
	}

	sl_real Canvas::getAlpha()
	{
		return m_alpha;
	}

	void Canvas::setAlpha(sl_real alpha)
	{
		if (Math::isAlmostZero(m_alpha - alpha)) {
			m_alpha = alpha;
		} else {
			m_alpha = alpha;
			_setAlpha(alpha);
		}
	}

	sl_bool Canvas::isAntiAlias()
	{
		return m_flagAntiAlias;
	}

	void Canvas::setAntiAlias(sl_bool flag)
	{
		if (m_flagAntiAlias != flag) {
			m_flagAntiAlias = flag;
			_setAntiAlias(flag);
		}
	}

	void Canvas::clipToRectangle(sl_real x, sl_real y, sl_real width, sl_real height)
	{
		clipToRectangle(Rectangle(x, y, x + width, y + height));
	}

	void Canvas::clipToRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real rx, sl_real ry)
	{
		clipToRoundRect(Rectangle(x, y, x + width, y + height), Size(rx, ry));
	}

	void Canvas::clipToEllipse(sl_real x, sl_real y, sl_real width, sl_real height)
	{
		clipToEllipse(Rectangle(x, y, x+width, y+height));
	}

	void Canvas::translate(sl_real dx, sl_real dy)
	{
		Matrix3 mat;
		Transform2::setTranslation(mat, dx, dy);
		concatMatrix(mat);
	}

	void Canvas::rotate(sl_real radians)
	{
		Matrix3 mat;
		Transform2::setRotation(mat, radians);
		concatMatrix(mat);
	}

	void Canvas::rotate(sl_real cx, sl_real cy, sl_real radians)
	{
		Matrix3 mat;
		Transform2::setRotation(mat, cx, cy, radians);
		concatMatrix(mat);
	}

	void Canvas::scale(sl_real sx, sl_real sy)
	{
		Matrix3 mat;
		Transform2::setScaling(mat, sx, sy);
		concatMatrix(mat);
	}

	void Canvas::drawText(const StringParam& text, sl_real x, sl_real y, const Ref<Font>& font, const Color& color)
	{
		DrawTextParam param;
		param.text = text;
		param.font = font;
		param.color = color;
		param.x = x;
		param.y = y;
		drawText(param);
	}

	void Canvas::drawText(const StringParam& text, sl_real x, sl_real y, const Ref<Font>& font, const Color& color, const Alignment& alignment, sl_bool flagMultiLine)
	{
		DrawTextParam param;
		param.text = text;
		param.font = font;
		param.color = color;
		param.x = x;
		param.y = y;
		param.alignment = alignment;
		param.flagMultiLine = flagMultiLine;
		drawText(param);
	}

	void Canvas::drawText(const StringParam& text, const Rectangle& rcDst, const Ref<Font>& font, const Color& color, const Alignment& alignment, sl_bool flagMultiLine)
	{
		DrawTextParam param;
		param.text = text;
		param.font = font;
		param.color = color;
		param.x = rcDst.left;
		param.y = rcDst.top;
		param.width = rcDst.getWidth();
		param.height = rcDst.getHeight();
		param.alignment = alignment;
		param.flagMultiLine = flagMultiLine;
		drawText(param);
	}

	void Canvas::drawLine(sl_real x1, sl_real y1, sl_real x2, sl_real y2, const Ref<Pen>& pen)
	{
		drawLine(Point(x1, y1), Point(x2, y2), pen);
	}

	void Canvas::drawLines(const List<Point>& _points, const Ref<Pen>& pen)
	{
		ListLocker<Point> points(_points);
		drawLines(points.data, (sl_uint32)(points.count), pen);
	}

	void Canvas::drawArc(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen)
	{
		drawArc(Rectangle(x, y, x + width, y + height), startDegrees, sweepDegrees, pen);
	}

	void Canvas::drawRectangle(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Pen>& pen, const Ref<Brush>& brush)
	{
		drawRectangle(Rectangle(x, y, x+width, y+height), pen, brush);
	}

	void Canvas::drawRectangle(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Pen>& pen, const Color& fillColor)
	{
		drawRectangle(Rectangle(x, y, x+width, y+height), pen, fillColor);
	}

	void Canvas::drawRectangle(const Rectangle& rc, const Ref<Pen>& pen)
	{
		drawRectangle(rc, pen, Ref<Brush>::null());
	}

	void Canvas::drawRectangle(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Pen>& pen)
	{
		drawRectangle(Rectangle(x, y, x+width, y+height), pen, Ref<Brush>::null());
	}

	void Canvas::fillRectangle(const Rectangle& rc, const Ref<Brush>& brush)
	{
		drawRectangle(rc, Ref<Pen>::null(), brush);
	}

	void Canvas::fillRectangle(const Rectangle& rc, const Color& color)
	{
		drawRectangle(rc, Ref<Pen>::null(), color);
	}

	void Canvas::fillRectangle(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Brush>& brush)
	{
		drawRectangle(Rectangle(x, y, x+width, y+height), Ref<Pen>::null(), brush);
	}

	void Canvas::fillRectangle(sl_real x, sl_real y, sl_real width, sl_real height, const Color& color)
	{
		drawRectangle(Rectangle(x, y, x+width, y+height), Ref<Pen>::null(), color);
	}

	void Canvas::drawRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real rx, sl_real ry, const Ref<Pen>& pen, const Ref<Brush>& brush)
	{
		drawRoundRect(Rectangle(x, y, x + width, y + height), Size(rx, ry), pen, brush);
	}

	void Canvas::drawRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real rx, sl_real ry, const Ref<Pen>& pen, const Color& fillColor)
	{
		drawRoundRect(Rectangle(x, y, x + width, y + height), Size(rx, ry), pen, fillColor);
	}

	void Canvas::drawRoundRect(const Rectangle& rc, const Size& radius, const Ref<Pen>& pen)
	{
		drawRoundRect(rc, radius, pen, Ref<Brush>::null());
	}

	void Canvas::drawRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real rx, sl_real ry, const Ref<Pen>& pen)
	{
		drawRoundRect(Rectangle(x, y, x+width, y+height), Size(rx, ry), pen, Ref<Brush>::null());
	}

	void Canvas::fillRoundRect(const Rectangle& rc, const Size& radius, const Ref<Brush>& brush)
	{
		drawRoundRect(rc, radius, Ref<Pen>::null(), brush);
	}

	void Canvas::fillRoundRect(const Rectangle& rc, const Size& radius, const Color& color)
	{
		drawRoundRect(rc, radius, Ref<Pen>::null(), color);
	}

	void Canvas::fillRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real rx, sl_real ry, const Ref<Brush>& brush)
	{
		drawRoundRect(Rectangle(x, y, x+width, y+height), Size(rx, ry), Ref<Pen>::null(), brush);
	}

	void Canvas::fillRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real rx, sl_real ry, const Color& color)
	{
		drawRoundRect(Rectangle(x, y, x+width, y+height), Size(rx, ry), Ref<Pen>::null(), color);
	}

	void Canvas::drawEllipse(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Pen>& pen, const Ref<Brush>& brush)
	{
		drawEllipse(Rectangle(x, y, x+width, y+height), pen, brush);
	}

	void Canvas::drawEllipse(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Pen>& pen, const Color& fillColor)
	{
		drawEllipse(Rectangle(x, y, x+width, y+height), pen, fillColor);
	}

	void Canvas::drawEllipse(const Rectangle& rc, const Ref<Pen>& pen)
	{
		drawEllipse(rc, pen, Ref<Brush>::null());
	}

	void Canvas::drawEllipse(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Pen>& pen)
	{
		drawEllipse(Rectangle(x, y, x+width, y+height), pen, Ref<Brush>::null());
	}

	void Canvas::fillEllipse(const Rectangle& rc, const Ref<Brush>& brush)
	{
		drawEllipse(rc, Ref<Pen>::null(), brush);
	}

	void Canvas::fillEllipse(const Rectangle& rc, const Color& color)
	{
		drawEllipse(rc, Ref<Pen>::null(), color);
	}

	void Canvas::fillEllipse(sl_real x, sl_real y, sl_real width, sl_real height, const Ref<Brush>& brush)
	{
		drawEllipse(Rectangle(x, y, x+width, y+height), Ref<Pen>::null(), brush);
	}

	void Canvas::fillEllipse(sl_real x, sl_real y, sl_real width, sl_real height, const Color& color)
	{
		drawEllipse(Rectangle(x, y, x+width, y+height), Ref<Pen>::null(), color);
	}

	void Canvas::drawPolygon(const List<Point>& _points, const Ref<Pen>& pen, const Ref<Brush>& brush, FillMode fillMode)
	{
		ListLocker<Point> points(_points);
		drawPolygon(points.data, (sl_uint32)(points.count), pen, brush, fillMode);
	}

	void Canvas::drawPolygon(const List<Point>& _points, const Ref<Pen>& pen, const Color& fillColor, FillMode fillMode)
	{
		ListLocker<Point> points(_points);
		drawPolygon(points.data, (sl_uint32)(points.count), pen,fillColor, fillMode);
	}

	void Canvas::drawPolygon(const Point* points, sl_uint32 countPoints, const Ref<Pen>& pen)
	{
		drawPolygon(points, countPoints, pen, Ref<Brush>::null());
	}

	void Canvas::drawPolygon(const List<Point>& _points, const Ref<Pen>& pen)
	{
		ListLocker<Point> points(_points);
		drawPolygon(points.data, (sl_uint32)(points.count), pen, Ref<Brush>::null());
	}

	void Canvas::fillPolygon(const Point* points, sl_uint32 countPoints, const Ref<Brush>& brush)
	{
		drawPolygon(points, countPoints, Ref<Pen>::null(), brush);
	}

	void Canvas::fillPolygon(const Point* points, sl_uint32 countPoints, const Color& color)
	{
		drawPolygon(points, countPoints, Ref<Pen>::null(), color);
	}

	void Canvas::fillPolygon(const List<Point>& _points, const Ref<Brush>& brush)
	{
		ListLocker<Point> points(_points);
		drawPolygon(points.data, (sl_uint32)(points.count), Ref<Pen>::null(), brush);
	}

	void Canvas::fillPolygon(const List<Point>& _points, const Color& color)
	{
		ListLocker<Point> points(_points);
		drawPolygon(points.data, (sl_uint32)(points.count), Ref<Pen>::null(), color);
	}

	void Canvas::drawPie(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen, const Ref<Brush>& brush)
	{
		drawPie(Rectangle(x, y, x + width, y + height), startDegrees, sweepDegrees, pen, brush);
	}

	void Canvas::drawPie(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen, const Color& fillColor)
	{
		drawPie(Rectangle(x, y, x + width, y + height), startDegrees, sweepDegrees, pen, fillColor);
	}

	void Canvas::drawPie(const Rectangle& rc, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen)
	{
		drawPie(rc, startDegrees, sweepDegrees, pen, Ref<Brush>::null());
	}

	void Canvas::drawPie(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees, const Ref<Pen>& pen)
	{
		drawPie(Rectangle(x, y, x+width, y+height), startDegrees, sweepDegrees, pen, Ref<Brush>::null());
	}

	void Canvas::fillPie(const Rectangle& rc, sl_real startDegrees, sl_real sweepDegrees, const Ref<Brush>& brush)
	{
		drawPie(rc, startDegrees, sweepDegrees, Ref<Pen>::null(), brush);
	}

	void Canvas::fillPie(const Rectangle& rc, sl_real startDegrees, sl_real sweepDegrees, const Color& color)
	{
		drawPie(rc, startDegrees, sweepDegrees, Ref<Pen>::null(), color);
	}

	void Canvas::fillPie(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees, const Color& color)
	{
		drawPie(Rectangle(x, y, x+width, y+height), startDegrees, sweepDegrees, Ref<Pen>::null(), color);
	}

	void Canvas::drawPath(const Ref<GraphicsPath>& path, const Ref<Pen>& pen)
	{
		drawPath(path, pen, Ref<Brush>::null());
	}

	void Canvas::fillPath(const Ref<GraphicsPath>& path, const Ref<Brush>& brush)
	{
		drawPath(path, Ref<Pen>::null(), brush);
	}

	void Canvas::fillPath(const Ref<GraphicsPath>& path, const Color& color)
	{
		drawPath(path, Ref<Pen>::null(), color);
	}

	void Canvas::drawShadowRectangle(sl_real x, sl_real y, sl_real width, sl_real height, const Color& color, sl_real shadowRadius)
	{
		if (shadowRadius < SLIB_EPSILON) {
			fillRectangle(x, y, width, height, color);
			return;
		}
		sl_bool flagAntialias = isAntiAlias();
		if (flagAntialias) {
			setAntiAlias(sl_false);
		}
		fillRectangle(x, y, width, height, color);
		Color color0(color.r, color.g, color.b, 0);
		fillRectangle(x - shadowRadius, y, shadowRadius, height, Brush::createLinearGradientBrush(Point(x - shadowRadius, y), Point(x, y), color0, color));
		fillRectangle(x + width, y, shadowRadius, height, Brush::createLinearGradientBrush(Point(x + width + shadowRadius, y), Point(x + width, y), color0, color));
		fillRectangle(x, y - shadowRadius, width, shadowRadius, Brush::createLinearGradientBrush(Point(x, y - shadowRadius), Point(x, y), color0, color));
		fillRectangle(x, y + height, width, shadowRadius, Brush::createLinearGradientBrush(Point(x, y + height + shadowRadius), Point(x, y + height), color0, color));
		fillRectangle(x - shadowRadius, y - shadowRadius, shadowRadius, shadowRadius, Brush::createRadialGradientBrush(Point(x, y), shadowRadius, color, color0));
		fillRectangle(x + width, y - shadowRadius, shadowRadius, shadowRadius, Brush::createRadialGradientBrush(Point(x + width, y), shadowRadius, color, color0));
		fillRectangle(x - shadowRadius, y + height, shadowRadius, shadowRadius, Brush::createRadialGradientBrush(Point(x, y + height), shadowRadius, color, color0));
		fillRectangle(x + width, y + height, shadowRadius, shadowRadius, Brush::createRadialGradientBrush(Point(x + width, y + height), shadowRadius, color, color0));
		if (flagAntialias) {
			setAntiAlias(sl_true);
		}
	}

	void Canvas::drawShadowRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real roundRadius, const Color& color, sl_real shadowRadius)
	{
		if (shadowRadius < SLIB_EPSILON) {
			fillRoundRect(x, y, width, height, roundRadius, roundRadius, color);
			return;
		}
		if (roundRadius < SLIB_EPSILON) {
			drawShadowRectangle(x, y, width, height, color, shadowRadius);
			return;
		}
		sl_bool flagAntialias = isAntiAlias();
		if (flagAntialias) {
			setAntiAlias(sl_false);
		}
		sl_real width_half = width / 2;
		if (roundRadius > width_half) {
			roundRadius = width_half;
		}
		sl_real height_half = height / 2;
		if (roundRadius > height_half) {
			roundRadius = height_half;
		}
		sl_real roundRadius2 = roundRadius * 2;
		fillRectangle(x + roundRadius, y + roundRadius, width - roundRadius2, height - roundRadius2, color);
		Color colors[3];
		colors[0] = color;
		colors[1] = color;
		colors[2] = color;
		colors[2].a = 0;
		sl_real radius = roundRadius + shadowRadius;
		sl_real locations[3] = {0, roundRadius / radius, 1};
		fillRectangle(x - shadowRadius, y + roundRadius, radius, height - roundRadius2, Brush::createLinearGradientBrush(Point(x + roundRadius, y), Point(x - shadowRadius, y), 3, colors, locations));
		fillRectangle(x + width - roundRadius, y + roundRadius, radius, height - roundRadius2, Brush::createLinearGradientBrush(Point(x + width - roundRadius, y), Point(x + width + shadowRadius, y), 3, colors, locations));
		fillRectangle(x + roundRadius, y - shadowRadius, width - roundRadius2, radius, Brush::createLinearGradientBrush(Point(x, y + roundRadius), Point(x, y - shadowRadius), 3, colors, locations));
		fillRectangle(x + roundRadius, y + height - roundRadius, width - roundRadius2, radius, Brush::createLinearGradientBrush(Point(x, y + height - roundRadius), Point(x, y + height + shadowRadius), 3, colors, locations));
		fillRectangle(x - shadowRadius, y - shadowRadius, radius, radius, Brush::createRadialGradientBrush(Point(x + roundRadius, y + roundRadius), radius, 3, colors, locations));
		fillRectangle(x + width - roundRadius, y - shadowRadius, radius, radius, Brush::createRadialGradientBrush(Point(x + width - roundRadius, y + roundRadius), radius, 3, colors, locations));
		fillRectangle(x - shadowRadius, y + height - roundRadius, radius, radius, Brush::createRadialGradientBrush(Point(x + roundRadius, y + height - roundRadius), radius, 3, colors, locations));
		fillRectangle(x + width - roundRadius, y + height - roundRadius, radius, radius, Brush::createRadialGradientBrush(Point(x + width - roundRadius, y + height - roundRadius), radius, 3, colors, locations));
		if (flagAntialias) {
			setAntiAlias(sl_true);
		}
	}

	void Canvas::drawShadowCircle(sl_real centerX, sl_real centerY, sl_real circleRadius, const Color& color, sl_real shadowRadius)
	{
		if (circleRadius < SLIB_EPSILON) {
			return;
		}
		if (shadowRadius < SLIB_EPSILON) {
			sl_real circleRadius2 = circleRadius * 2;
			fillEllipse(centerX - circleRadius, centerY - circleRadius, circleRadius2, circleRadius2, color);
			return;
		}
		sl_bool flagAntialias = isAntiAlias();
		if (flagAntialias) {
			setAntiAlias(sl_false);
		}
		Color colors[3];
		colors[0] = color;
		colors[1] = color;
		colors[2] = color;
		colors[2].a = 0;
		sl_real radius = circleRadius + shadowRadius;
		sl_real radius2 = radius * 2;
		sl_real locations[3] = {0, circleRadius / radius, 1};
		fillEllipse(centerX - radius, centerY - radius, radius2, radius2, Brush::createRadialGradientBrush(Point(centerX, centerY), radius, 3, colors, locations));
		if (flagAntialias) {
			setAntiAlias(sl_true);
		}
	}

	namespace priv
	{
		namespace canvas
		{

			SLIB_ALIGN(8) static const char g_defaultDrawParamBuf[sizeof(DrawParam)] = {0};

			static const DrawParam& g_defaultDrawParam = *((const DrawParam*)((void*)g_defaultDrawParamBuf));

		}
	}

	void Canvas::draw(const Rectangle& rectDst, const Ref<Drawable>& src, const Rectangle& rectSrc)
	{
		draw(rectDst, src, rectSrc, priv::canvas::g_defaultDrawParam);
	}

	void Canvas::draw(const Rectangle& rectDst, const Ref<Drawable>& src)
	{
		draw(rectDst, src, priv::canvas::g_defaultDrawParam);
	}

	void Canvas::draw(sl_real xDst, sl_real yDst, sl_real widthDst, sl_real heightDst, const Ref<Drawable>& src, sl_real xSrc, sl_real ySrc, sl_real widthSrc, sl_real heightSrc, const DrawParam& param)
	{
		draw(Rectangle(xDst, yDst, xDst + widthDst, yDst + heightDst), src, Rectangle(xSrc, ySrc, xSrc + widthSrc, ySrc + heightSrc), param);
	}

	void Canvas::draw(sl_real xDst, sl_real yDst, sl_real widthDst, sl_real heightDst, const Ref<Drawable>& src, sl_real xSrc, sl_real ySrc, sl_real widthSrc, sl_real heightSrc)
	{
		draw(Rectangle(xDst, yDst, xDst + widthDst, yDst + heightDst), src, Rectangle(xSrc, ySrc, xSrc + widthSrc, ySrc + heightSrc), priv::canvas::g_defaultDrawParam);
	}

	void Canvas::draw(sl_real xDst, sl_real yDst, sl_real widthDst, sl_real heightDst, const Ref<Drawable>& src, const DrawParam& param)
	{
		draw(Rectangle(xDst, yDst, xDst + widthDst, yDst + heightDst), src, param);
	}

	void Canvas::draw(sl_real xDst, sl_real yDst, sl_real widthDst, sl_real heightDst, const Ref<Drawable>& src)
	{
		draw(Rectangle(xDst, yDst, xDst + widthDst, yDst + heightDst), src, priv::canvas::g_defaultDrawParam);
	}

	void Canvas::draw(sl_real xDst, sl_real yDst, const Ref<Drawable>& src)
	{
		draw(xDst, yDst, src, priv::canvas::g_defaultDrawParam);
	}

	void Canvas::draw(const Rectangle& rectDst, const Ref<Drawable>& source, ScaleMode scaleMode, const Alignment& alignment)
	{
		draw(rectDst, source, scaleMode, alignment, priv::canvas::g_defaultDrawParam);
	}

	void Canvas::_setAlpha(sl_real alpha)
	{
	}

	void Canvas::_setAntiAlias(sl_bool flag)
	{
	}


	CanvasStateScope::CanvasStateScope()
	{
	}

	CanvasStateScope::CanvasStateScope(const Ref<Canvas>& canvas)
	{
		save(canvas);
	}

	CanvasStateScope::~CanvasStateScope()
	{
		restore();
	}

	void CanvasStateScope::save(const Ref<Canvas>& canvas)
	{
		restore();
		if (canvas.isNotNull()) {
			canvas->save();
		}
		m_canvas = canvas;
	}

	void CanvasStateScope::restore()
	{
		Ref<Canvas> canvas = m_canvas;
		if (canvas.isNotNull()) {
			canvas->restore();
			m_canvas.setNull();
		}
	}

	Ref<Canvas> CanvasStateScope::getCanvas()
	{
		return m_canvas;
	}

}
