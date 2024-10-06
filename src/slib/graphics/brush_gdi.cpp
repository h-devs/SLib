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

#include "slib/graphics/definition.h"

#if defined(SLIB_GRAPHICS_IS_GDI)

#include "slib/graphics/brush.h"

#include "slib/graphics/image.h"
#include "slib/core/scoped_buffer.h"
#include "slib/graphics/platform.h"

namespace slib
{

	namespace
	{
		SLIB_INLINE static Gdiplus::Color GetColor(const Color& c)
		{
			return Gdiplus::Color(c.a, c.r, c.g, c.b);
		}

		static Gdiplus::SolidBrush* CreateSolidBrush(const BrushDesc& desc)
		{
			const Color& c = desc.color;
			Gdiplus::Color color(c.a, c.r, c.g, c.b);
			return new Gdiplus::SolidBrush(color);
		}

		static Gdiplus::Brush* CreateGradientBrush(const BrushDesc& desc)
		{
			GradientBrushDetail* detail = (GradientBrushDetail*)(desc.detail.get());
			if (!detail) {
				return sl_null;
			}
			ListElements<Color> colors(detail->colors);
			ListElements<sl_real> locations(detail->locations);
			if (colors.count != locations.count) {
				return sl_null;
			}
			sl_size n = colors.count;
			if (!n) {
				return sl_null;
			}
			if (desc.style == BrushStyle::LinearGradient) {
				Gdiplus::PointF pt1((Gdiplus::REAL)(detail->point1.x), (Gdiplus::REAL)(detail->point1.y));
				Gdiplus::PointF pt2((Gdiplus::REAL)(detail->point2.x), (Gdiplus::REAL)(detail->point2.y));
				Gdiplus::LinearGradientBrush* brush = new Gdiplus::LinearGradientBrush(pt1, pt2, GetColor(colors[0]), GetColor(colors[n - 1]));
				if (brush) {
					brush->SetWrapMode(Gdiplus::WrapModeTileFlipXY);
					if (n > 2) {
						SLIB_SCOPED_BUFFER(Gdiplus::Color, 128, c, n)
						if (!c) {
							return sl_null;
						}
						SLIB_SCOPED_BUFFER(Gdiplus::REAL, 128, l, n)
						if (!l) {
							return sl_null;
						}
						for (sl_size i = 0; i < n; i++) {
							c[i] = GetColor(colors[i]);
							l[i] = (Gdiplus::REAL)(locations[i]);
						}
						brush->SetInterpolationColors(c, l, (INT)n);
					}
					return brush;
				}
			} else {
				Gdiplus::GraphicsPath path;
				Gdiplus::REAL d = (Gdiplus::REAL)(detail->radius * 2);
				path.AddEllipse((Gdiplus::REAL)(detail->point1.x - detail->radius), (Gdiplus::REAL)(detail->point1.y - detail->radius), d, d);
				Gdiplus::PathGradientBrush* brush = new Gdiplus::PathGradientBrush(&path);
				if (brush) {
					if (n > 2) {
						SLIB_SCOPED_BUFFER(Gdiplus::Color, 128, c, n)
						if (!c) {
							return sl_null;
						}
						SLIB_SCOPED_BUFFER(Gdiplus::REAL, 128, l, n)
						if (!l) {
							return sl_null;
						}
						for (sl_size i = 0; i < n; i++) {
							c[i] = GetColor(colors[n - 1 - i]);
							l[i] = (Gdiplus::REAL)(1.0f - locations[n - 1 - i]);
						}
						brush->SetInterpolationColors(c, l, (INT)n);
					} else {
						brush->SetCenterColor(GetColor(colors[0]));
						Gdiplus::Color c = GetColor(colors[n - 1]);
						INT k = 1;
						brush->SetSurroundColors(&c, &k);
					}
					brush->SetCenterPoint(Gdiplus::PointF((Gdiplus::REAL)(detail->point1.x), (Gdiplus::REAL)(detail->point1.y)));
					return brush;
				}
			}
			return sl_null;
		}

		static Gdiplus::TextureBrush* CreateTextureBrush(const BrushDesc& desc, Ref<Drawable>& cache)
		{
			TextureBrushDetail* detail = (TextureBrushDetail*)(desc.detail.get());
			if (!detail) {
				return sl_null;
			}
			Bitmap* pattern = detail->pattern.get();
			if (pattern->isImage()) {
				Ref<Drawable> drawable = PlatformDrawable::create((Image*)pattern);
				if (drawable.isNotNull()) {
					Gdiplus::Image* image = GraphicsPlatform::getImageDrawableHandle(drawable.get());
					if (image) {
						cache = Move(drawable);
						return new Gdiplus::TextureBrush(image);
					}
				}
			} else {
				Gdiplus::Bitmap* bitmap = GraphicsPlatform::getBitmapHandle(pattern);
				if (bitmap) {
					return new Gdiplus::TextureBrush(bitmap);
				}
			}
			return sl_null;
		}

		static Gdiplus::HatchStyle ToHatchStyle(HatchStyle style)
		{
			switch (style)
			{
				case HatchStyle::Horizontal:
					return Gdiplus::HatchStyle::HatchStyleHorizontal;
				case HatchStyle::Vertical:
					return Gdiplus::HatchStyle::HatchStyleVertical;
				case HatchStyle::ForwardDiagonal:
					return Gdiplus::HatchStyle::HatchStyleForwardDiagonal;
				case HatchStyle::BackwardDiagonal:
					return Gdiplus::HatchStyle::HatchStyleBackwardDiagonal;
				case HatchStyle::Cross:
					return Gdiplus::HatchStyle::HatchStyleCross;
				case HatchStyle::DiagonalCross:
					return Gdiplus::HatchStyle::HatchStyleDiagonalCross;
				case HatchStyle::Dots:
					return Gdiplus::HatchStyle::HatchStyle05Percent;
				default:
					break;
			}
			return Gdiplus::HatchStyle::HatchStyleHorizontal;
		}

		static Gdiplus::HatchBrush* CreateHatchBrush(const BrushDesc& desc)
		{
			HatchBrushDetail* detail = (HatchBrushDetail*)(desc.detail.get());
			if (!detail) {
				return sl_null;
			}
			return new Gdiplus::HatchBrush(ToHatchStyle(detail->style), GraphicsPlatform::getGdiplusColor(desc.color), GraphicsPlatform::getGdiplusColor(detail->backgroundColor));
		}

		class BrushPlatformObject : public CRef
		{
		public:
			Gdiplus::Brush* m_brush;
			Ref<Drawable> m_drawableCache;

		public:
			BrushPlatformObject(const BrushDesc& desc)
			{
				GraphicsPlatform::startGdiplus();
				m_brush = NULL;
				switch (desc.style) {
					case BrushStyle::Solid:
						m_brush = CreateSolidBrush(desc);
						break;
					case BrushStyle::LinearGradient:
					case BrushStyle::RadialGradient:
						m_brush = CreateGradientBrush(desc);
						break;
					case BrushStyle::Texture:
						m_brush = CreateTextureBrush(desc, m_drawableCache);
						break;
					case BrushStyle::Hatch:
						m_brush = CreateHatchBrush(desc);
						break;
					default:
						break;
				}
			}

			~BrushPlatformObject()
			{
				delete m_brush;
			}
		};

		class BrushHelper : public Brush
		{
		public:
			BrushPlatformObject* getPlatformObject()
			{
				if (m_platformObject.isNull()) {
					SpinLocker lock(&m_lock);
					if (m_platformObject.isNull()) {
						m_platformObject = new BrushPlatformObject(m_desc);
					}
				}
				return (BrushPlatformObject*)(m_platformObject.get());
			}

			Gdiplus::Brush* getPlatformHandle()
			{
				BrushPlatformObject* po = getPlatformObject();
				if (po) {
					return po->m_brush;
				}
				return NULL;
			}
		};
	}

	Gdiplus::Brush* GraphicsPlatform::getBrushHandle(Brush* brush)
	{
		if (brush) {
			return ((BrushHelper*)brush)->getPlatformHandle();
		}
		return NULL;
	}

}

#endif
