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

#include "slib/math/plot.h"

#include "slib/graphics/image.h"
#include "slib/graphics/font.h"

namespace slib
{

	namespace priv
	{
		namespace plot
		{

			static sl_real GetStep(sl_real width)
			{
				if (Math::isAlmostZero(width)) {
					return 1;
				}
				sl_real step = Math::pow(10, Math::floor(Math::log10(width / 10)));
				sl_real n = width / step;
				if (n >= 80) {
					step *= 10;
				} else if (n > 40) {
					step *= 5;
				} else if (n > 20) {
					step *= 2;
				}
				return step;
			}

		}
	}

	using namespace priv::plot;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PlotGraphParam)

	PlotGraphParam::PlotGraphParam(): type(PlotGraphType::Line), color(Color::Red), size(1)
	{
	}

	PlotGraphParam::PlotGraphParam(PlotGraphType _type): type(_type), color(Color::Red), size(1)
	{
	}

	PlotGraphParam::PlotGraphParam(PlotGraphType _type, const Color& _color, sl_real _size): type(_type), color(_color), size(_size)
	{
	}


	SLIB_DEFINE_OBJECT(PlotGraph, Object)

	PlotGraph::PlotGraph()
	{
	}

	PlotGraph::~PlotGraph()
	{
	}

#define CONVERT_X(x) (((x) - rectSrc.left) * sx + rectDst.left)
#define CONVERT_Y(y) (rectDst.bottom - ((y) - rectSrc.top) * sy)

	void PlotGraph::draw(Canvas* canvas, const Rectangle& rectDst, const Rectangle& rectSrc)
	{
		sl_real sx = rectDst.getWidth() / rectSrc.getWidth();
		sl_real sy = rectDst.getHeight() / rectSrc.getHeight();
		Point* pts = points.getData();
		sl_size n = points.getCount();
		if (param.type == PlotGraphType::Line) {
			if (n < 2) {
				return;
			}
			Ref<Pen> pen = Pen::createSolidPen(param.size, param.color);
			Point old;
			for (sl_size i = 0; i < n; i++) {
				sl_real x = CONVERT_X(pts->x);
				sl_real y = CONVERT_Y(pts->y);
				if (i) {
					canvas->drawLine(old.x, old.y, x, y, pen);
				}
				old.x = x;
				old.y = y;
				pts++;
			};
		} else {
			sl_real size = param.size;
			for (sl_size i = 0; i < n; i++) {
				sl_real x = CONVERT_X(pts->x);
				sl_real y = CONVERT_Y(pts->y);
				canvas->fillRectangle(x - size / 2, y - size / 2, size, size, param.color);
				pts++;
			};
		}
	}

	Rectangle PlotGraph::getBounds()
	{
		Rectangle ret;
		ret.setFromPoints(points);
		return ret;
	}


	SLIB_DEFINE_OBJECT(Plot, Object)

	Plot::Plot()
	{
		m_backgroundColor = Color::White;
		m_gridColor = Color::Gray;
		
		m_labelColor = Color::Black;
		m_labelFont = Font::create(sl_null, 15);
		m_labelMargin = 5;
		
		m_marginLeft = 40;
		m_marginRight = 20;
		m_marginTop = 20;
		m_marginBottom = 30;

		m_minX = 0;
		m_flagDefinedMinX = sl_false;
		m_maxX = 0;
		m_flagDefinedMaxX = sl_false;
		m_minY = 0;
		m_flagDefinedMinY = sl_false;
		m_maxY = 0;
		m_flagDefinedMaxY = sl_false;
	}

	Plot::~Plot()
	{
	}

	const Color& Plot::getBackgroundColor()
	{
		return m_backgroundColor;
	}

	void Plot::setBackgroundColor(const Color& color)
	{
		m_backgroundColor = color;
	}

	const Color& Plot::getGridLineColor()
	{
		return m_gridColor;
	}

	void Plot::setGridLineColor(const Color& color)
	{
		m_gridColor = color;
	}

	const Color& Plot::getLabelColor()
	{
		return m_labelColor;
	}

	void Plot::setLabelColor(const Color& color)
	{
		m_labelColor = color;
	}

	Ref<Font> Plot::getLabelFont()
	{
		return m_labelFont;
	}

	void Plot::setLabelFont(const Ref<Font>& font)
	{
		m_labelFont = font;
	}

	sl_int32 Plot::getLabelMargin()
	{
		return m_labelMargin;
	}

	void Plot::setLabelMargin(sl_int32 margin)
	{
		m_labelMargin = margin;
	}

	sl_int32 Plot::getMarginLeft()
	{
		return m_marginLeft;
	}

	void Plot::setMarginLeft(sl_int32 margin)
	{
		m_marginLeft = margin;
	}

	sl_int32 Plot::getMarginTop()
	{
		return m_marginTop;
	}

	void Plot::setMarginTop(sl_int32 margin)
	{
		m_marginTop = margin;
	}

	sl_int32 Plot::getMarginRight()
	{
		return m_marginRight;
	}

	void Plot::setMarginRight(sl_int32 margin)
	{
		m_marginRight = margin;
	}

	sl_int32 Plot::getMarginBottom()
	{
		return m_marginBottom;
	}

	void Plot::setMarginBottom(sl_int32 margin)
	{
		m_marginBottom = margin;
	}

	sl_real Plot::getMinimumX()
	{
		return m_minX;
	}

	void Plot::setMinimumX(sl_real x)
	{
		m_minX = x;
		m_flagDefinedMinX = sl_true;
	}

	sl_real Plot::getMaximumX()
	{
		return m_maxX;
	}

	void Plot::setMaximumX(sl_real x)
	{
		m_maxX = x;
		m_flagDefinedMaxX = sl_true;
	}

	sl_real Plot::getMinimumY()
	{
		return m_minY;
	}

	void Plot::setMinimumY(sl_real y)
	{
		m_minY = y;
		m_flagDefinedMinY = sl_true;
	}

	sl_real Plot::getMaximumY()
	{
		return m_maxY;
	}

	void Plot::setMaximumY(sl_real y)
	{
		m_maxY = y;
		m_flagDefinedMaxY = sl_true;
	}

	Ref<PlotGraph> Plot::add(const Array<Point>& points, const PlotGraphParam& param)
	{
		if (points.isNull()) {
			return sl_null;
		}
		Ref<PlotGraph> graph = new PlotGraph;
		if (graph.isNotNull()) {
			graph->points = points;
			graph->param = param;
			m_graphs.add(graph);
			return graph;
		}
		return sl_null;
	}

	Ref<PlotGraph> Plot::add(const Array<Point>& points, const Color& color)
	{
		PlotGraphParam param;
		param.color = color;
		return add(points, param);
	}

	void Plot::draw(Canvas* canvas, sl_uint32 width, sl_uint32 height)
	{
		Rectangle rectDst;
		rectDst.left = (sl_real)m_marginLeft;
		rectDst.top = (sl_real)m_marginTop;
		rectDst.right = (sl_real)((sl_int32)width - m_marginRight);
		rectDst.bottom = (sl_real)((sl_int32)height - m_marginBottom);

		if (rectDst.getWidth() < SLIB_EPSILON) {
			return;
		}
		if (rectDst.getHeight() < SLIB_EPSILON) {
			return;
		}

		ListLocker< Ref<PlotGraph> > graphs(m_graphs);

		Rectangle rectSrc;
		{
			for (sl_size i = 0; i < graphs.count; i++) {
				PlotGraph* graph = graphs[i].get();
				Rectangle rect = graph->getBounds();
				if (i) {
					rectSrc.mergeRectangle(rect);
				} else {
					rectSrc = rect;
				}
			}
			if (m_flagDefinedMinX) {
				rectSrc.left = m_minX;
			}
			if (m_flagDefinedMinY) {
				rectSrc.top = m_minY;
			}
			if (m_flagDefinedMaxX) {
				rectSrc.right = m_maxX;
			}
			if (m_flagDefinedMaxY) {
				rectSrc.bottom = m_maxY;
			}
			if (rectSrc.right <= rectSrc.left) {
				rectSrc.right = rectSrc.left + 1;
			}
			if (rectSrc.bottom <= rectSrc.top) {
				rectSrc.bottom = rectSrc.top + 1;
			}
		}

		sl_real stepX = GetStep(rectSrc.getWidth());
		sl_real stepY = GetStep(rectSrc.getHeight());
		
		Rectanglei rectSrci;
		rectSrci.left = (sl_int32)(Math::floor(rectSrc.left / stepX));
		rectSrci.right = (sl_int32)(Math::ceil(rectSrc.right / stepX));
		rectSrci.top = (sl_int32)(Math::floor(rectSrc.top / stepY));
		rectSrci.bottom = (sl_int32)(Math::ceil(rectSrc.bottom / stepY));
		rectSrc.left = (sl_real)(rectSrci.left * stepX);
		rectSrc.right = (sl_real)(rectSrci.right * stepX);
		rectSrc.top = (sl_real)(rectSrci.top * stepY);
		rectSrc.bottom = (sl_real)(rectSrci.bottom * stepY);

		sl_real sx = rectDst.getWidth() / rectSrc.getWidth();
		sl_real sy = rectDst.getHeight() / rectSrc.getHeight();

		canvas->fillRectangle(0, 0, (sl_real)width, (sl_real)height, m_backgroundColor);
		{
			sl_real margin = (sl_real)m_labelMargin;
			const Color& color = m_labelColor;
			Ref<Font> font = m_labelFont;
			Ref<Pen> pen = Pen::createSolidPen(1, m_gridColor);
			for (sl_int32 ix = rectSrci.left; ix <= rectSrci.right; ix ++) {
				sl_real x = (sl_real)(ix * stepX);
				sl_real _x = CONVERT_X(x);
				canvas->drawLine(_x, rectDst.top, _x, rectDst.bottom, pen);
				canvas->drawText(String::fromFloat(x), _x, rectDst.bottom + margin, font, color, Alignment::TopCenter);
			}
			for (sl_int32 iy = rectSrci.top; iy <= rectSrci.bottom; iy ++) {
				sl_real y = (sl_real)(iy * stepY);
				sl_real _y = CONVERT_Y(y);
				canvas->drawLine(rectDst.left, _y, rectDst.right, _y, pen);
				canvas->drawText(String::fromFloat(y), rectDst.left - margin, _y, font, color, Alignment::MiddleRight);
			}
		}
		{
			for (sl_size i = 0; i < graphs.count; i++) {
				PlotGraph* graph = graphs[i].get();
				graph->draw(canvas, rectDst, rectSrc);
			}
		}
	}

	Ref<Image> Plot::toImage(sl_uint32 width, sl_uint32 height)
	{
		Ref<Image> image = Image::create(width, height);
		if (image.isNotNull()) {
			Ref<Canvas> canvas = image->getCanvas();
			if (canvas.isNotNull()) {
				draw(canvas.get(), width, height);
				return image;
			}
		}
		return sl_null;
	}

}
