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

#include "slib/graphics/definition.h"

#if defined(SLIB_GRAPHICS_IS_QUARTZ)

#include "slib/graphics/path.h"

#include "slib/graphics/platform.h"

namespace slib
{

	namespace priv
	{
		namespace graphics_path
		{

			class PlatformObject : public Referable
			{
			public:
				CGMutablePathRef path;
				
			protected:
				PlatformObject(CGMutablePathRef _path): path(_path) {}
				
				~PlatformObject()
				{
					CGPathRelease(path);
				}

			public:
				static Ref<PlatformObject> create()
				{
					CGMutablePathRef path = CGPathCreateMutable();
					if (path) {
						Ref<PlatformObject> ret = new PlatformObject(path);
						if (ret.isNotNull()) {
							return ret;
						}
						CGPathRelease(path);
					}
					return sl_null;
				}
				
			};

			class GraphicsPathHelper : public GraphicsPath
			{
			public:
				CGPathRef getPlatformPath()
				{
					_initPlatformObject();
					PlatformObject* po = (PlatformObject*)(m_platformObject.get());
					if (po) {
						return po->path;
					}
					return sl_null;
				}
			};

		}
	}

	using namespace priv::graphics_path;

	Ref<Referable> GraphicsPath::_createPlatformObject()
	{
		return Ref<Referable>::from(PlatformObject::create());
	}

	void GraphicsPath::_moveTo_PO(Referable* _po, sl_real x, sl_real y)
	{
		PlatformObject* po = (PlatformObject*)_po;
		CGPathMoveToPoint(po->path, sl_null, x, y);
	}

	void GraphicsPath::_lineTo_PO(Referable* _po, sl_real x, sl_real y)
	{
		PlatformObject* po = (PlatformObject*)_po;
		CGPathAddLineToPoint(po->path, sl_null, x, y);
	}

	void GraphicsPath::_cubicTo_PO(Referable* _po, sl_real xc1, sl_real yc1, sl_real xc2, sl_real yc2, sl_real xe, sl_real ye)
	{
		PlatformObject* po = (PlatformObject*)_po;
		CGPathAddCurveToPoint(po->path, sl_null, xc1, yc1, xc2, yc2, xe, ye);
	}

	void GraphicsPath::_closeSubpath_PO(Referable* _po)
	{
		PlatformObject* po = (PlatformObject*)_po;
		CGPathCloseSubpath(po->path);
	}

	void GraphicsPath::_setFillMode_PO(Referable* po, FillMode mode)
	{
	}

	CGPathRef GraphicsPlatform::getGraphicsPath(GraphicsPath* path)
	{
		if (path) {
			return ((GraphicsPathHelper*)path)->getPlatformPath();
		}
		return NULL;
	}

}

#endif
