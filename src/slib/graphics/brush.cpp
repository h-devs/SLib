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

#include "slib/graphics/brush.h"

#include "slib/graphics/bitmap.h"
#include "slib/core/safe_static.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(BrushDesc)

	BrushDesc::BrushDesc()
	{
		style = BrushStyle::Solid;
		color = Color::White;
	}


	SLIB_DEFINE_ROOT_OBJECT(Brush)

	Brush::Brush()
	{
	}

	Brush::~Brush()
	{
	}

	Ref<Brush> Brush::getDefault()
	{
		SLIB_SAFE_LOCAL_STATIC(Ref<Brush>, defaultBrush, createSolidBrush(Color::White))
		if (SLIB_SAFE_STATIC_CHECK_FREED(defaultBrush)) {
			return sl_null;
		}
		return defaultBrush;
	}

	Ref<Brush> Brush::create(const BrushDesc& desc)
	{
		Ref<Brush> ret = new Brush;
		if (ret.isNotNull()) {
			ret->m_desc = desc;
			return ret;
		}
		return sl_null;
	}

	Ref<Brush> Brush::createSolidBrush(const Color& color)
	{
		Ref<Brush> ret = new Brush;
		if (ret.isNotNull()) {
			ret->m_desc.color = color;
			return ret;
		}
		return sl_null;
	}

	Ref<Brush> Brush::createLinearGradientBrush(const Point& pt1, const Point& pt2, const Color& color1, const Color& color2)
	{
		Ref<GradientBrushDetail> detail = new GradientBrushDetail;
		if (detail.isNull()) {
			return sl_null;
		}
		detail->point1 = pt1;
		detail->point2 = pt2;
		detail->colors = List<Color>::createFromElements(color1, color2);
		if (detail->colors.isNull()) {
			return sl_null;
		}
		detail->locations = List<sl_real>::createFromElements(0.0f, 1.0f);
		if (detail->locations.isNull()) {
			return sl_null;
		}
		Ref<Brush> ret = new Brush;
		if (ret.isNull()) {
			return sl_null;
		}
		ret->m_desc.style = BrushStyle::LinearGradient;
		ret->m_desc.detail = Move(detail);
		return ret;
	}

	namespace
	{
		static sl_bool PrepareGradientDetailColors(GradientBrushDetail* detail, sl_uint32 nColors, const Color* colors, const sl_real* locations)
		{
			if (!(Math::isAlmostZero(locations[0]))) {
				if (!(detail->colors.add_NoLock(colors[0]))) {
					return sl_false;
				}
				if (!(detail->locations.add_NoLock(0.0f))) {
					return sl_false;
				}
			}
			if (!(detail->colors.addElements_NoLock(colors, nColors))) {
				return sl_false;
			}
			if (!(detail->locations.addElements_NoLock(locations, nColors))) {
				return sl_false;
			}
			if (!(Math::isAlmostZero(locations[nColors - 1] - 1.0f))) {
				if (!(detail->colors.add_NoLock(colors[nColors - 1]))) {
					return sl_false;
				}
				if (!(detail->locations.add_NoLock(1.0f))) {
					return sl_false;
				}
			}
			return sl_true;
		}
	}

	Ref<Brush> Brush::createLinearGradientBrush(const Point& pt1, const Point& pt2, sl_uint32 nColors, const Color* colors, const sl_real* locations)
	{
		if (nColors < 2) {
			return sl_null;
		}
		Ref<GradientBrushDetail> detail = new GradientBrushDetail;
		if (detail.isNull()) {
			return sl_null;
		}
		detail->point1 = pt1;
		detail->point2 = pt2;
		if (!(PrepareGradientDetailColors(detail.get(), nColors, colors, locations))) {
			return sl_null;
		}
		Ref<Brush> ret = new Brush;
		if (ret.isNull()) {
			return sl_null;
		}
		ret->m_desc.style = BrushStyle::LinearGradient;
		ret->m_desc.detail = Move(detail);
		return ret;
	}

	Ref<Brush> Brush::createRadialGradientBrush(const Point& ptCenter, sl_real radius, const Color& colorCenter, const Color& colorEdge)
	{
		Ref<GradientBrushDetail> detail = new GradientBrushDetail;
		if (detail.isNull()) {
			return sl_null;
		}
		detail->point1 = ptCenter;
		detail->radius = radius;
		detail->colors = List<Color>::createFromElements(colorCenter, colorEdge);
		if (detail->colors.isNull()) {
			return sl_null;
		}
		detail->locations = List<sl_real>::createFromElements(0.0f, 1.0f);
		if (detail->locations.isNull()) {
			return sl_null;
		}
		Ref<Brush> ret = new Brush;
		if (ret.isNull()) {
			return sl_null;
		}
		ret->m_desc.style = BrushStyle::RadialGradient;
		ret->m_desc.detail = Move(detail);
		return ret;
	}

	Ref<Brush> Brush::createRadialGradientBrush(const Point& ptCenter, sl_real radius, sl_uint32 nColors, const Color* colors, const sl_real* locations)
	{
		if (nColors < 2) {
			return sl_null;
		}
		Ref<GradientBrushDetail> detail = new GradientBrushDetail;
		if (detail.isNull()) {
			return sl_null;
		}
		detail->point1 = ptCenter;
		detail->radius = radius;
		if (!(PrepareGradientDetailColors(detail.get(), nColors, colors, locations))) {
			return sl_null;
		}
		Ref<Brush> ret = new Brush;
		if (ret.isNull()) {
			return sl_null;
		}
		ret->m_desc.style = BrushStyle::RadialGradient;
		ret->m_desc.detail = Move(detail);
		return ret;
	}

	Ref<Brush> Brush::createTextureBrush(const Ref<Bitmap>& bitmap)
	{
		if (bitmap.isNotNull()) {
			Ref<TextureBrushDetail> detail = new TextureBrushDetail;
			if (detail.isNotNull()) {
				detail->pattern = bitmap;
				Ref<Brush> ret = new Brush;
				if (ret.isNotNull()) {
					ret->m_desc.style = BrushStyle::Texture;
					ret->m_desc.detail = Move(detail);
					return ret;
				}
			}
		}
		return sl_null;
	}

	Ref<Brush> Brush::createHatchBrush(HatchStyle style, const Color& foreColor, const Color& backColor)
	{
		Ref<HatchBrushDetail> detail = new HatchBrushDetail;
		if (detail.isNull()) {
			return sl_null;
		}
		detail->style = style;
		detail->backgroundColor = backColor;
		Ref<Brush> ret = new Brush;
		if (ret.isNull()) {
			return sl_null;
		}
		ret->m_desc.style = BrushStyle::Hatch;
		ret->m_desc.color = foreColor;
		ret->m_desc.detail = Move(detail);
		return ret;
	}

	void Brush::getDesc(BrushDesc& desc)
	{
		desc = m_desc;
	}

	BrushDesc& Brush::getDesc()
	{
		return m_desc;
	}

	BrushStyle Brush::getStyle()
	{
		return m_desc.style;
	}

	Color Brush::getColor()
	{
		return m_desc.color;
	}


	GradientBrushDetail::GradientBrushDetail()
	{
	}

	GradientBrushDetail::~GradientBrushDetail()
	{
	}


	TextureBrushDetail::TextureBrushDetail()
	{
	}

	TextureBrushDetail::~TextureBrushDetail()
	{
	}


	HatchBrushDetail::HatchBrushDetail(): style(HatchStyle::Solid)
	{
	}

	HatchBrushDetail::~HatchBrushDetail()
	{
	}

}
