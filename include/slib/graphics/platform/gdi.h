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

#ifndef CHECKHEADER_SLIB_GRAPHICS_GDI_PLATFORM
#define CHECKHEADER_SLIB_GRAPHICS_GDI_PLATFORM

#include "../definition.h"

#if defined(SLIB_GRAPHICS_IS_GDI)

#include "../bitmap.h"
#include "../../core/function.h"
#include "../../platform.h"
#include "../../dl/win32/gdiplus.h"

namespace slib
{

	class SLIB_EXPORT GraphicsPlatform
	{
	public:
		static void startGdiplus();
		static void shutdownGdiplus();

		static Gdiplus::Brush* getBrushHandle(Brush* brush);
		static Gdiplus::Pen* getPenHandle(Pen* pen);
		static Gdiplus::GraphicsPath* getGraphicsPath(GraphicsPath* path);

		static Gdiplus::Font* getGdiplusFont(Font* font);
		static HFONT getGdiFont(Font* font);

		static Ref<Canvas> createCanvas(CanvasType type, Gdiplus::Graphics* graphics, sl_uint32 width, sl_uint32 height, const Function<void()>& onFreeCanvas);
		static Gdiplus::Graphics* getCanvasHandle(Canvas* canvas);
		static void drawImage(Canvas* canvas, const Rectangle& rectDst, Gdiplus::Image* image, const Rectangle& rectSrc, const Drawable::DrawParam& param);

		static Ref<Drawable> createImageDrawable(Gdiplus::Image* image, sl_bool flagFreeOnRelease = sl_true, CRef* ref = sl_null);
		static Gdiplus::Image* getImageDrawableHandle(Drawable* drawable);

		static Ref<Bitmap> createBitmap(Gdiplus::Bitmap* bitmap, sl_bool flagFreeOnRelease = sl_true, CRef* ref = sl_null);
		static Ref<Bitmap> createBitmap(HBITMAP hbm);
		static Gdiplus::Bitmap* getBitmapHandle(Bitmap* bitmap);

		static COLORREF getColorRef(const Color& color);
		static Color getColorFromColorRef(COLORREF cr);

		static HBITMAP createDIB(const Ref<Drawable>& drawable);
		static HICON createHICON(const Ref<Drawable>& drawable, sl_bool flagCursor = sl_false, sl_uint32 xHotspot = 0, sl_uint32 yHotspot = 0);

	};

}

#endif

#endif