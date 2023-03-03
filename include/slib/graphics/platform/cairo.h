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

#ifndef CHECKHEADER_SLIB_GRAPHICS_CAIRO_PLATFORM
#define CHECKHEADER_SLIB_GRAPHICS_CAIRO_PLATFORM

#include "../definition.h"

#if defined(SLIB_GRAPHICS_IS_CAIRO)

#include "../bitmap.h"

#if	defined(SLIB_PLATFORM_IS_TIZEN)
#	include <cairo.h>
#	include <pangocairo.h>
#elif defined(SLIB_PLATFORM_IS_LINUX)
#	include "../../dl/linux/glib.h"
#	include "../../dl/linux/cairo.h"
#else
#	include "cairo/cairo.h"
#	include "pango/pangocairo.h"
#endif

namespace slib
{

	class SLIB_EXPORT GraphicsPlatform
	{
	public:
		static PangoFontDescription* getPangoFont(Font* font);

		static Ref<Canvas> createCanvas(CanvasType type, cairo_t* graphics, sl_uint32 width, sl_uint32 height, sl_bool flagFreeOnRelease = sl_true);
		static cairo_t* getCanvasHandle(Canvas* canvas);

		static Ref<Bitmap> createBitmap(cairo_surface_t* bitmap, sl_bool flagFreeOnRelease = sl_true, Referable* ref = sl_null);
		static cairo_surface_t* getBitmapHandle(Bitmap* bitmap);

		static void drawImage(Canvas* canvas, const Rectangle& rectDst, cairo_surface_t* image, const Drawable::DrawParam& param);
		static void drawImage(Canvas* canvas, const Rectangle& rectDst, cairo_surface_t* image, const Rectangle& rectSrc, const Drawable::DrawParam& param);

	};

}

#endif

#endif
