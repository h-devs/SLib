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

#ifndef CHECKHEADER_SLIB_GRAPHICS_QUARTZ_PLATFORM
#define CHECKHEADER_SLIB_GRAPHICS_QUARTZ_PLATFORM

#include "../definition.h"

#if defined(SLIB_GRAPHICS_IS_QUARTZ)

#include "../bitmap.h"

#include "../../math/matrix4.h"
#include "../../core/platform.h"

#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

#if defined(SLIB_PLATFORM_IS_MACOS)
#	if defined(__OBJC__)
		typedef NSFont UIFont;
#	endif
#endif

namespace slib
{

	class SLIB_EXPORT GraphicsPlatform
	{
	public:
		static CGGradientRef getGradientBrushHandle(Brush* brush);
		static CGImageRef getTextureBrushRetainedHandle(Brush* brush);

		static CGPathRef getGraphicsPath(GraphicsPath* path);

		static Ref<Canvas> createCanvas(CanvasType type, CGContextRef graphics, sl_uint32 width, sl_uint32 height);
		static CGContextRef getCanvasHandle(Canvas* canvas);
		static void drawCGImage(Canvas* canvas, const Rectangle& rectDst, CGImageRef image, sl_bool flagFlipY, const DrawParam& param);

		static Ref<Drawable> createImageDrawable(CGImageRef image, sl_bool flagFlipped = sl_false);
		static CGImageRef getImageDrawableHandle(Drawable* drawable);
		
		static CGContextRef getBitmapHandle(Bitmap* bitmap);

		static Ref<Bitmap> createBitmapFromCGImage(CGImageRef image);

		static CGImageRef loadCGImageFromMemory(const void* buf, sl_size size);
		static CGImageRef loadCGImageFromApp(const StringParam& name);
		static CGColorRef getCGColorFromColor(const Color& color);

#	if defined(__OBJC__)

		static UIFont* getNativeFont(Font* font, CGFloat scaleFactor = 1);

#		if defined(SLIB_PLATFORM_IS_MACOS)

		static NSImage* loadNSImageFromMemory(const void* buf, sl_size size);
		static NSColor* getNSColorFromColor(const Color& color);
		static Color getColorFromNSColor(NSColor* color);
		static NSImage* createNSImageFromBitmap(const Ref<Bitmap>& bitmap);
		static NSImage* getNSImage(const Ref<Drawable>& drawable);

#		elif defined(SLIB_PLATFORM_IS_IOS)

		static UIImage* loadUIImageFromMemory(const void* buf, sl_size size);
		static UIColor* getUIColorFromColor(const Color& color);
		static Color getColorFromUIColor(UIColor* color);

#		endif

		static void getCATransform(CATransform3D& _out, const Matrix3& mat);
		static void getCATransform(CATransform3D& _out, const Matrix4& mat);
		static void getCGAffineTransform(CGAffineTransform& _out, const Matrix3& mat);
		static void getCGAffineTransform(CGAffineTransform& _out, const Matrix3& mat, CGFloat scaleFactor, CGFloat anchorX, CGFloat anchorY);
		static void getMatrix3FromCGAffineTransform(Matrix3& _out, const CGAffineTransform& transform);

#	endif

	};

}

#endif

#endif
