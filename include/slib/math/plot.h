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

#ifndef CHECKHEADER_SLIB_MATH_PLOT
#define CHECKHEADER_SLIB_MATH_PLOT

#include "rectangle.h"

#include "../core/object.h"
#include "../core/array.h"
#include "../graphics/color.h"

namespace slib
{

	class Image;
	class Canvas;
	class Font;
	class Window;

	enum class PlotGraphType
	{
		Point = 0,
		Line = 1
	};

	class SLIB_EXPORT PlotGraphParam
	{
	public:
		PlotGraphType type;
		Color color;
		sl_real size;

	public:
		PlotGraphParam();

		PlotGraphParam(PlotGraphType type);

		PlotGraphParam(PlotGraphType type, const Color& color, sl_real size = 1);

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PlotGraphParam)

	};

	class SLIB_EXPORT PlotGraph : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		PlotGraph();

		~PlotGraph();

	public:
		void draw(Canvas* canvas, const Rectangle& rectDst, const Rectangle& rectSrc);

		Rectangle getBounds();

	public:
		Array<Point> points;
		PlotGraphParam param;

	};

	class SLIB_EXPORT Plot : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		Plot();

		~Plot();

	public:
		const Color& getBackgroundColor();

		void setBackgroundColor(const Color& color);

		const Color& getGridLineColor();

		void setGridLineColor(const Color& color);

		const Color& getLabelColor();

		void setLabelColor(const Color& color);

		Ref<Font> getLabelFont();

		void setLabelFont(const Ref<Font>& font);

		sl_int32 getLabelMargin();

		void setLabelMargin(sl_int32 margin);

		sl_int32 getMarginLeft();

		void setMarginLeft(sl_int32 margin);

		sl_int32 getMarginTop();

		void setMarginTop(sl_int32 margin);

		sl_int32 getMarginRight();

		void setMarginRight(sl_int32 margin);

		sl_int32 getMarginBottom();

		void setMarginBottom(sl_int32 margin);

		sl_real getMinimumX();

		void setMinimumX(sl_real x);

		sl_real getMaximumX();

		void setMaximumX(sl_real x);

		sl_real getMinimumY();

		void setMinimumY(sl_real y);

		sl_real getMaximumY();

		void setMaximumY(sl_real y);

	public:
		Ref<PlotGraph> add(const Array<Point>& points, const PlotGraphParam& param);

		Ref<PlotGraph> add(const Array<Point>& points, const Color& color);

		template <class PARAM>
		Ref<PlotGraph> add(const Point* pts, sl_size count, const PARAM& param)
		{
			if (!count) {
				return sl_null;
			}
			return add(Array<Point>::create(pts, count), param);
		}

		template <class T1, class T2, class PARAM>
		Ref<PlotGraph> add(const T1* x, const T2* y, sl_size count, const PARAM& param)
		{
			if (!count) {
				return sl_null;
			}
			Array<Point> pts = Array<Point>::create(count);
			if (pts.isNotNull()) {
				Point* p = pts.getData();
				for (sl_size i = 0; i < count; i++) {
					p->x = (sl_real)(x[i]);
					p->y = (sl_real)(y[i]);
					p++;
				}
				return add(pts, param);
			}
			return sl_null;
		}

		template <class GET_POINT, class PARAM>
		Ref<PlotGraph> add(sl_size count, GET_POINT&& f, const PARAM& param)
		{
			if (!count) {
				return sl_null;
			}
			Array<Point> pts = Array<Point>::create(count);
			if (pts.isNotNull()) {
				Point* p = pts.getData();
				for (sl_size i = 0; i < count; i++) {
					_setGraphValue(*p, (sl_uint32)i, f((sl_uint32)i));
					p++;
				}
				return add(pts, param);
			}
			return sl_null;
		}

		template <class FUNC, class PARAM>
		Ref<PlotGraph> add(sl_size count, sl_real x1, sl_real x2, FUNC&& f, const PARAM& param)
		{
			if (!count) {
				return sl_null;
			}
			Array<Point> pts = Array<Point>::create(count);
			if (pts.isNotNull()) {
				Point* p = pts.getData();
				sl_real s;
				if (count > 1) {
					s = (sl_real)((x2 - x1) / (count - 1));
				} else {
					s = 1;
				}
				for (sl_size i = 0; i < count; i++) {
					sl_real x = x1 + (sl_real)(i * s);
					p->x = x;
					p->y = f(x);
					p++;
				}
				return add(pts, param);
			}
			return sl_null;
		}

	public:
		void draw(Canvas* canvas, sl_uint32 width, sl_uint32 height);

		Ref<Image> toImage(sl_uint32 width, sl_uint32 height);

		Ref<Window> show(sl_uint32 width, sl_uint32 height);

	protected:
		List< Ref<PlotGraph> > m_graphs;

	private:
		static void _setGraphValue(Point& pt, sl_uint32 i, float y)
		{
			pt.x = (sl_real)i;
			pt.y = (sl_real)y;
		}

		static void _setGraphValue(Point& pt, sl_uint32 i, double y)
		{
			pt.x = (sl_real)i;
			pt.y = (sl_real)y;
		}

		static void _setGraphValue(Point& pt, sl_uint32 i, sl_int32 y)
		{
			pt.x = (sl_real)i;
			pt.y = (sl_real)y;
		}

		static void _setGraphValue(Point& pt, sl_uint32 i, sl_int64 y)
		{
			pt.x = (sl_real)i;
			pt.y = (sl_real)y;
		}

		static void _setGraphValue(Point& pt, sl_uint32 i, const Point& _pt)
		{
			pt = _pt;
		}

	protected:
		Color m_backgroundColor;
		Color m_gridColor;

		Color m_labelColor;
		AtomicRef<Font> m_labelFont;
		sl_int32 m_labelMargin;

		sl_int32 m_marginLeft;
		sl_int32 m_marginTop;
		sl_int32 m_marginRight;
		sl_int32 m_marginBottom;

		sl_real m_minX;
		sl_bool m_flagDefinedMinX;
		sl_real m_maxX;
		sl_bool m_flagDefinedMaxX;
		sl_real m_minY;
		sl_bool m_flagDefinedMinY;
		sl_real m_maxY;
		sl_bool m_flagDefinedMaxY;

	};

}

#endif
