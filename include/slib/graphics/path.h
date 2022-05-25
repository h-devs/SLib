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

#ifndef CHECKHEADER_SLIB_GRAPHICS_PATH
#define CHECKHEADER_SLIB_GRAPHICS_PATH

#include "constants.h"

#include "../core/list.h"
#include "../math/rectangle.h"

namespace slib
{
	
	struct SLIB_EXPORT GraphicsPathPoint
	{
		Point pt;

		// flags from GraphicsPathPointType
		sl_uint8 type;

		enum : sl_uint8
		{
			Begin = 0,
			Line = 1,
			BezierCubic = 3,
			TypeMask = 0x0f,
			FlagClose = 0x80
		};

	};
	
	
	// GraphicsPath is not thread-safe.
	class SLIB_EXPORT GraphicsPath : public Referable
	{
		SLIB_DECLARE_OBJECT

	protected:
		GraphicsPath();

		~GraphicsPath();

	public:
		static Ref<GraphicsPath> create();
	
	public:
		sl_size getPointsCount();

		GraphicsPathPoint* getPoints();

		void moveTo(sl_real x, sl_real y);

		void moveTo(const Point& pt);

		void lineTo(sl_real x, sl_real y);

		void lineTo(const Point& pt);

		void conicTo(sl_real xc, sl_real yc, sl_real xe, sl_real ye);

		void conicTo(const Point& ptControl, const Point& ptEnd);

		void cubicTo(sl_real xc1, sl_real yc1, sl_real xc2, sl_real yc2, sl_real xe, sl_real ye);
	
		void cubicTo(const Point& ptControl1, const Point& ptControl2, const Point& ptEnd);

		void closeSubpath();

		void addArc(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees, sl_bool flagMoveTo = sl_true);

		void addArc(const Rectangle& rect, sl_real startDegrees, sl_real sweepDegrees, sl_bool flagMoveTo = sl_true);

		void addRectangle(sl_real x, sl_real y, sl_real width, sl_real height);
	
		void addRectangle(const Rectangle& rect);

		void addRoundRect(sl_real x, sl_real y, sl_real width, sl_real height, sl_real rx, sl_real ry);

		void addRoundRect(const Rectangle& rect, const Size& radius);

		void addEllipse(sl_real x, sl_real y, sl_real width, sl_real height);

		void addEllipse(const Rectangle& rect);

		void addPie(sl_real x, sl_real y, sl_real width, sl_real height, sl_real startDegrees, sl_real sweepDegrees);

		void addPie(const Rectangle& rect, sl_real startDegrees, sl_real sweepDegrees);

		FillMode getFillMode();

		void setFillMode(FillMode mode);


		Rectangle getBounds();

		sl_bool containsPoint(sl_real x, sl_real y);

		sl_bool containsPoint(const Point& pt);

	protected:
		void _checkBegin() noexcept;

		sl_bool _initialize_PO();

		void _moveTo_PO(sl_real x, sl_real y);

		void _lineTo_PO(sl_real x, sl_real y);

		void _cubicTo_PO(sl_real xc1, sl_real yc1, sl_real xc2, sl_real yc2, sl_real xe, sl_real ye);

		void _closeSubpath_PO();

		void _setFillMode_PO(FillMode mode);
	
		Rectangle _getBounds_PO();
	
		sl_bool _containsPoint_PO(sl_real x, sl_real y);
	
	protected:
		CList<GraphicsPathPoint> m_points;
		sl_bool m_flagBegan;
		Point m_pointBegin;
		FillMode m_fillMode;

		Ref<Referable> m_platformObject;

	};

}

#endif
