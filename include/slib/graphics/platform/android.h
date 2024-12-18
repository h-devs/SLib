/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_GRAPHICS_ANDROID_PLATFORM
#define CHECKHEADER_SLIB_GRAPHICS_ANDROID_PLATFORM

#include "../definition.h"

#if defined(SLIB_GRAPHICS_IS_ANDROID)

#include "../bitmap.h"
#include "../../platform.h"

namespace slib
{

	class SLIB_EXPORT GraphicsPlatform
	{
	public:
		static jobject getBrushHandle(Brush* brush);
		static jobject getPenHandle(Pen* pen);
		static jobject getGraphicsPath(GraphicsPath* path);
		static jobject getNativeFont(Font* font);

		static Ref<Canvas> createCanvas(CanvasType type, jobject canvas);
		static jobject getCanvasHandle(Canvas* canvas);

		static Ref<Bitmap> createBitmap(jobject bitmap, sl_bool flagRecycleOnRelease = sl_true, CRef* ref = sl_null);
		static jobject getBitmapHandle(Bitmap* bitmap);

	};

}

#endif

#endif
