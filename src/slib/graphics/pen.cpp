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

#include "slib/graphics/pen.h"

#include "slib/core/safe_static.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PenDesc)

	PenDesc::PenDesc()
	{
		width = 1;
		color = Color::Black;

		style = PenStyle::Solid;
		cap = LineCap::Default;
		join = LineJoin::Default;
		miterLimit = -1.0f;
	}

	PenDesc::PenDesc(PenStyle _style, sl_real _width, const Color& _color)
	{
		style = _style;
		width = _width;
		color = _color;

		cap = LineCap::Default;
		join = LineJoin::Default;
		miterLimit = -1.0f;
	}


	SLIB_DEFINE_ROOT_OBJECT(Pen)

	Pen::Pen()
	{
		m_desc.cap = LineCap::Flat;
		m_desc.join = LineJoin::Miter;
		m_desc.miterLimit = 10;
	}

	Pen::~Pen()
	{
	}

	Ref<Pen> Pen::getDefault()
	{
		SLIB_SAFE_LOCAL_STATIC(Ref<Pen>, defaultPen, create(PenDesc()))
		if (SLIB_SAFE_STATIC_CHECK_FREED(defaultPen)) {
			return sl_null;
		}
		return defaultPen;
	}

	namespace {
		static void CopyPenDesc(PenDesc& dst, const PenDesc& src)
		{
			if (src.style != PenStyle::Default) {
				dst.style = src.style;
			}
			if (src.width >= 0.0f) {
				dst.width = src.width;
			}
			if (src.color.isNotZero()) {
				dst.color = src.color;
			}
			if (src.cap != LineCap::Default) {
				dst.cap = src.cap;
			}
			if (src.join != LineJoin::Default) {
				dst.join = src.join;
			}
			if (src.miterLimit >= 0.0f) {
				dst.miterLimit = src.miterLimit;
			}
		}
	}

	Ref<Pen> Pen::create(const PenDesc& src)
	{
		Ref<Pen> ret = new Pen;
		if (ret.isNotNull()) {
			CopyPenDesc(ret->m_desc, src);
		}
		return ret;
	}

	Ref<Pen> Pen::create(const PenDesc& src, const PenDesc& def)
	{
		Ref<Pen> ret = new Pen;
		if (ret.isNotNull()) {
			PenDesc& dst = ret->m_desc;
			if (src.style != PenStyle::Default) {
				dst.style = src.style;
			} else if (def.style != PenStyle::Default) {
				dst.style = def.style;
			}
			if (src.width >= 0.0f) {
				dst.width = src.width;
			} else if (def.width >= 0.0f) {
				dst.width = def.width;
			}
			if (src.color.isNotZero()) {
				dst.color = src.color;
			} else if (def.color.isNotZero()) {
				dst.color = def.color;
			}
			if (src.cap != LineCap::Default) {
				dst.cap = src.cap;
			} else if (def.cap != LineCap::Default) {
				dst.cap = def.cap;
			}
			if (src.join != LineJoin::Default) {
				dst.join = src.join;
			} else if (def.join != LineJoin::Default) {
				dst.join = def.join;
			}
			if (src.miterLimit >= 0.0f) {
				dst.miterLimit = src.miterLimit;
			} else if (def.miterLimit >= 0.0f) {
				dst.miterLimit = def.miterLimit;
			}
		}
		return ret;
	}

	Ref<Pen> Pen::create(const PenDesc& desc, const Ref<Pen>& original)
	{
		Ref<Pen> ret = new Pen;
		if (ret.isNotNull()) {
			if (original.isNotNull()) {
				original->getDesc(ret->m_desc);
			}
			CopyPenDesc(ret->m_desc, desc);
		}
		return ret;
	}

	Ref<Pen> Pen::create(PenStyle style, sl_real width, const Color& color)
	{
		return create(PenDesc(style, width, color));
	}

	Ref<Pen> Pen::createSolidPen(sl_real width, const Color& color)
	{
		return create(PenDesc(PenStyle::Solid, width, color));
	}

	void Pen::getDesc(PenDesc& desc)
	{
		desc = m_desc;
	}

	PenStyle Pen::getStyle()
	{
		return m_desc.style;
	}

	sl_real Pen::getWidth()
	{
		return m_desc.width;
	}

	Color Pen::getColor()
	{
		return m_desc.color;
	}

	LineCap Pen::getCap()
	{
		return m_desc.cap;
	}

	LineJoin Pen::getJoin()
	{
		return m_desc.join;
	}

	sl_real Pen::getMiterLimit()
	{
		return m_desc.miterLimit;
	}

}
