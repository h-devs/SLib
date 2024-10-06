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

#ifndef CHECKHEADER_SLIB_PLATFORM_WIN32_DIRECT2D
#define CHECKHEADER_SLIB_PLATFORM_WIN32_DIRECT2D

#include "definition.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "../graphics/path.h"
#include "../math/triangle.h"
#include "../core/list.h"

namespace slib
{

	class SLIB_EXPORT Direct2D
	{
	public:
		static List< TriangleT<float> > tessellatePathFill(const Ref<GraphicsPath>& path, float flatteningTolerance = 1.0f);

		static List< TriangleT<float> > tessellatePathStroke(const Ref<GraphicsPath>& path, sl_real strokeWidth, float flatteningTolerance = 1.0f);

		static List< TriangleT<float> > tessellatePolygon(const Point* points, sl_size nPoints, FillMode fillMode = FillMode::Alternate);

		static List< TriangleT<float> > tessellatePolygonBorder(const Point* points, sl_size nPoints, float borderWidth);

		static List< TriangleT<float> > tessellatePolyline(const Point* points, sl_size nPoints, float lineWidth);

	};

}

#endif

#endif