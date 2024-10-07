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

#include "slib/platform/definition.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "slib/render/d2d.h"

#include "slib/core/safe_static.h"
#include "slib/dl/win32/d2d.h"

namespace slib
{

	namespace
	{
		static ID2D1Factory* CreateFactory()
		{
			auto api = d2d1::getApi_D2D1CreateFactory();
			if (!api) {
				return sl_null;
			}
			ID2D1Factory* factory = sl_null;
			api(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), sl_null, (void**)&factory);
			return factory;
		}

		class SharedContext
		{
		public:
			ID2D1Factory* factory = sl_null;

		public:
			SharedContext()
			{
				factory = CreateFactory();
			}
		};

		SLIB_SAFE_STATIC_GETTER(SharedContext, GetSharedContext)

		static ID2D1Factory* GetSharedFactory()
		{
			SharedContext* context = GetSharedContext();
			if (context) {
				return context->factory;
			}
			return sl_null;
		}

		static D2D1_POINT_2F ToD2DPoint(const Point& point)
		{
			D2D1_POINT_2F ret;
			ret.x = (float)(point.x);
			ret.y = (float)(point.y);
			return ret;
		}

		static sl_bool CreateGeometryAndSink(ID2D1PathGeometry*& geometry, ID2D1GeometrySink*& sink)
		{
			ID2D1Factory* factory = CreateFactory();
			if (!factory) {
				return sl_false;
			}
			sl_bool bRet = sl_false;
			factory->CreatePathGeometry(&geometry);
			if (geometry) {
				geometry->Open(&sink);
				if (sink) {
					return sl_true;
				}
				geometry->Release();
			}
			factory->Release();
			return bRet;
		}

		static ID2D1PathGeometry* CreateGeometryFromPath(const Ref<GraphicsPath>& path)
		{
			if (path.isNull()) {
				return sl_null;
			}
			ID2D1PathGeometry* geometry;
			ID2D1GeometrySink* sink;
			if (CreateGeometryAndSink(geometry, sink)) {
				if (path->getFillMode() == FillMode::Alternate) {
					sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
				} else {
					sink->SetFillMode(D2D1_FILL_MODE_WINDING);
				}
				GraphicsPathPoint* pts = path->getPoints();
				sl_size n = path->getPointCount();
				sl_bool flagEnd = sl_true;
				for (sl_size i = 0; i < n; i++) {
					GraphicsPathPoint& pt = pts[i];
					switch (pt.type) {
						case GraphicsPathPoint::MoveTo:
							if (flagEnd) {
								flagEnd = sl_false;
							} else {
								sink->EndFigure(D2D1_FIGURE_END_OPEN);
							}
							sink->BeginFigure(ToD2DPoint(pt), D2D1_FIGURE_BEGIN_FILLED);
							break;
						case GraphicsPathPoint::LineTo:
							sink->AddLine(ToD2DPoint(pt));
							break;
						case GraphicsPathPoint::CubicTo:
							{
								if (i + 2 < n) {
									if (pts[i + 1].type == GraphicsPathPoint::CubicTo && pts[i + 2].type == GraphicsPathPoint::CubicTo) {
										D2D1_BEZIER_SEGMENT bezier;
										bezier.point1 = ToD2DPoint(pt);
										bezier.point2 = ToD2DPoint(pts[i + 1]);
										bezier.point3 = ToD2DPoint(pts[i + 2]);
										i += 2;
										sink->AddBezier(bezier);
									}
								}
							}
							break;
						default:
							break;
					}
					if (pt.flagClose) {
						sink->EndFigure(D2D1_FIGURE_END_CLOSED);
						flagEnd = sl_true;
					}
				}
				if (!flagEnd) {
					sink->EndFigure(D2D1_FIGURE_END_OPEN);
				}
				sink->Close();
				sink->Release();
				return geometry;
			}
			return sl_null;
		}

		static ID2D1PathGeometry* CreateGeometryFromPoints(const PointT<float>* points, sl_size nPoints, FillMode fillMode, sl_bool flagClose)
		{
			if (nPoints < 2) {
				return sl_null;
			}
			ID2D1PathGeometry* geometry;
			ID2D1GeometrySink* sink;
			if (CreateGeometryAndSink(geometry, sink)) {
				if (fillMode == FillMode::Alternate) {
					sink->SetFillMode(D2D1_FILL_MODE_ALTERNATE);
				} else {
					sink->SetFillMode(D2D1_FILL_MODE_WINDING);
				}
				sink->BeginFigure(ToD2DPoint(*points), D2D1_FIGURE_BEGIN_FILLED);
				for (sl_size i = 1; i < nPoints; i++) {
					sink->AddLine(ToD2DPoint(points[i]));
				}
				if (flagClose) {
					sink->EndFigure(D2D1_FIGURE_END_CLOSED);
				} else {
					sink->EndFigure(D2D1_FIGURE_END_OPEN);
				}
				sink->Close();
				sink->Release();
				return geometry;
			}
			return sl_null;
		}

		class TessellationSink : public ID2D1TessellationSink
		{
		public:
			List< TriangleT<float> > triangles;

		public:
			STDMETHODIMP_(ULONG) AddRef() { return 2; }
			STDMETHODIMP_(ULONG) Release() { return 1; }
			STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
			{
				if (riid == __uuidof(ID2D1TessellationSink)) {
					*ppv = (ID2D1TessellationSink*)this;
					return NOERROR;
				} else if (riid == __uuidof(IUnknown)) {
					*ppv = (IUnknown*)((ID2D1TessellationSink*)this);
					return NOERROR;
				}
				return E_NOINTERFACE;
			}

		public:
			void CALLBACK AddTriangles(CONST D2D1_TRIANGLE* t, UINT n) override
			{
				triangles.addElements_NoLock((TriangleT<float>*)t, (sl_size)n);
			}

			HRESULT CALLBACK Close() override
			{
				return S_OK;
			}
		};

		static List< TriangleT<float> > TessellateGeometry(ID2D1PathGeometry* geometry, float flatteningTolerance = 1.0f)
		{
			TessellationSink sink;
			HRESULT hr = geometry->Tessellate(sl_null, flatteningTolerance, &sink);
			if (SUCCEEDED(hr)) {
				return sink.triangles;
			}
			return sl_null;
		}

		static List< TriangleT<float> > TessellateGeometryStroke(ID2D1PathGeometry* geometry, float strokeWidth, float flatteningTolerance = 1.0f)
		{
			List< TriangleT<float> > ret;
			ID2D1PathGeometry* stroke;
			ID2D1GeometrySink* strokeSink;
			if (CreateGeometryAndSink(stroke, strokeSink)) {
				HRESULT hr = geometry->Widen(strokeWidth, sl_null, sl_null, strokeSink);
				if (SUCCEEDED(hr)) {
					strokeSink->Close();
					ret = TessellateGeometry(stroke, flatteningTolerance);
				}
				strokeSink->Release();
				stroke->Release();
			}
			return ret;
		}
	}

	List< TriangleT<float> > Direct2D::tessellatePathFill(const Ref<GraphicsPath>& path, float flatteningTolerance)
	{
		List< TriangleT<float> > ret;
		ID2D1PathGeometry* geometry = CreateGeometryFromPath(path);
		if (geometry) {
			ret = TessellateGeometry(geometry, flatteningTolerance);
			geometry->Release();
		}
		return ret;
	}

	List< TriangleT<float> > Direct2D::tessellatePathStroke(const Ref<GraphicsPath>& path, sl_real strokeWidth, float flatteningTolerance)
	{
		List< TriangleT<float> > ret;
		ID2D1PathGeometry* geometry = CreateGeometryFromPath(path);
		if (geometry) {
			ret = TessellateGeometryStroke(geometry, strokeWidth, flatteningTolerance);
			geometry->Release();
		}
		return ret;
	}

	List< TriangleT<float> > Direct2D::tessellatePolygon(const Point* points, sl_size nPoints, FillMode fillMode)
	{
		List< TriangleT<float> > ret;
		ID2D1PathGeometry* geometry = CreateGeometryFromPoints(points, nPoints, fillMode, sl_true);
		if (geometry) {
			ret = TessellateGeometry(geometry);
			geometry->Release();
		}
		return ret;
	}

	List< TriangleT<float> > Direct2D::tessellatePolygonBorder(const Point* points, sl_size nPoints, float borderWidth)
	{
		List< TriangleT<float> > ret;
		ID2D1PathGeometry* geometry = CreateGeometryFromPoints(points, nPoints, FillMode::Alternate, sl_true);
		if (geometry) {
			ret = TessellateGeometryStroke(geometry, borderWidth);
			geometry->Release();
		}
		return ret;
	}

	List< TriangleT<float> > Direct2D::tessellatePolyline(const Point* points, sl_size nPoints, float lineWidth)
	{
		List< TriangleT<float> > ret;
		ID2D1PathGeometry* geometry = CreateGeometryFromPoints(points, nPoints, FillMode::Alternate, sl_false);
		if (geometry) {
			ret = TessellateGeometryStroke(geometry, lineWidth);
			geometry->Release();
		}
		return ret;
	}

}

#endif
