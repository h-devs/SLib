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

#if defined(SLIB_GRAPHICS_IS_ANDROID)

#include "slib/graphics/path.h"

#include "slib/graphics/platform.h"
#include "slib/core/scoped_buffer.h"

namespace slib
{

	namespace priv
	{
		namespace android
		{
			
			SLIB_JNI_BEGIN_CLASS(JRectF, "android/graphics/RectF")
				SLIB_JNI_FLOAT_FIELD(left);
				SLIB_JNI_FLOAT_FIELD(top);
				SLIB_JNI_FLOAT_FIELD(right);
				SLIB_JNI_FLOAT_FIELD(bottom);
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JPath, "slib/platform/android/ui/UiPath")
				SLIB_JNI_NEW(init, "()V");
				SLIB_JNI_METHOD(setFillMode, "setFillMode", "(I)V");
				SLIB_JNI_METHOD(moveTo, "moveTo", "(FF)V");
				SLIB_JNI_METHOD(lineTo, "lineTo", "(FF)V");
				SLIB_JNI_METHOD(cubicTo, "cubicTo", "(FFFFFF)V");
				SLIB_JNI_METHOD(closeSubpath, "closeSubpath", "()V");
				SLIB_JNI_METHOD(getBounds, "getBounds", "()Landroid/graphics/RectF;");
				SLIB_JNI_METHOD(containsPoint, "containsPoint", "(FF)Z");
			SLIB_JNI_END_CLASS

			class GraphicsPathPlatformObject : public Referable
			{
			public:
				JniGlobal<jobject> m_path;

			public:
				GraphicsPathPlatformObject()
				{
					JniGlobal<jobject> path = JPath::init.newObject(sl_null);
					if (path.isNotNull()) {
						m_path = Move(path);
					}
				}
			};

			class GraphicsPathHelper : public GraphicsPath
			{
			public:
				jobject getPlatformPath()
				{
					GraphicsPathPlatformObject* po = (GraphicsPathPlatformObject*)(m_platformObject.get());
					if (po) {
						return po->m_path;
					}
					return 0;
				}
			};

		}
	}

	using namespace priv::android;

	sl_bool GraphicsPath::_initialize_PO()
	{
		Ref<GraphicsPathPlatformObject> po = new GraphicsPathPlatformObject;
		if (po.isNotNull() && po->m_path.isNotNull()) {
			m_platformObject = po;
			return sl_true;
		}
		return sl_false;
	}

	void GraphicsPath::_moveTo_PO(sl_real x, sl_real y)
	{
		GraphicsPathPlatformObject* po = (GraphicsPathPlatformObject*)(m_platformObject.get());
		if (po) {
			JPath::moveTo.call(po->m_path, (float)(x), (float)(y));
		}
	}

	void GraphicsPath::_lineTo_PO(sl_real x, sl_real y)
	{
		GraphicsPathPlatformObject* po = (GraphicsPathPlatformObject*)(m_platformObject.get());
		if (po) {
			JPath::lineTo.call(po->m_path, (float)(x), (float)(y));
		}
	}

	void GraphicsPath::_cubicTo_PO(sl_real xc1, sl_real yc1, sl_real xc2, sl_real yc2, sl_real xe, sl_real ye)
	{
		GraphicsPathPlatformObject* po = (GraphicsPathPlatformObject*)(m_platformObject.get());
		if (po) {
			JPath::cubicTo.call(po->m_path, (float)(xc1), (float)(yc1) , (float)(xc2), (float)(yc2) , (float)(xe), (float)(ye));
		}
	}

	void GraphicsPath::_closeSubpath_PO()
	{
		GraphicsPathPlatformObject* po = (GraphicsPathPlatformObject*)(m_platformObject.get());
		if (po) {
			JPath::closeSubpath.call(po->m_path);
		}
	}

	void GraphicsPath::_setFillMode_PO(FillMode mode)
	{
		GraphicsPathPlatformObject* po = (GraphicsPathPlatformObject*)(m_platformObject.get());
		if (po) {
			JPath::setFillMode.call(po->m_path, (int)mode);
		}
	}

	Rectangle GraphicsPath::_getBounds_PO()
	{
		GraphicsPathPlatformObject* po = (GraphicsPathPlatformObject*)(m_platformObject.get());
		if (po) {
			JniLocal<jobject> jrect = JPath::getBounds.callObject(po->m_path);
			if (jrect.isNotNull()) {
				Rectangle ret;
				ret.left = JRectF::left.get(jrect);
				ret.top = JRectF::top.get(jrect);
				ret.right = JRectF::right.get(jrect);
				ret.bottom = JRectF::bottom.get(jrect);
				return ret;
			}
		}
		return Rectangle::zero();
	}

	sl_bool GraphicsPath::_containsPoint_PO(sl_real x, sl_real y)
	{
		GraphicsPathPlatformObject* po = (GraphicsPathPlatformObject*)(m_platformObject.get());
		if (po) {
			return (JPath::containsPoint.callBoolean(po->m_path, x, y)) != 0;
		}
		return sl_false;
	}

	jobject GraphicsPlatform::getGraphicsPath(GraphicsPath* path)
	{
		if (path) {
			return ((GraphicsPathHelper*)path)->getPlatformPath();
		}
		return 0;
	}

}

#endif
