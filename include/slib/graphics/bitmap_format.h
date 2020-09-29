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

#ifndef CHECKHEADER_SLIB_GRAPHICS_BITMAP_FORMAT
#define CHECKHEADER_SLIB_GRAPHICS_BITMAP_FORMAT

#include "definition.h"

#include "constants.h"


/*
	Format Bits Description
 
 0~7: unique id
 8~15: bits per pixel
 16~23: bits per sample
 24~25: number of planes
 26: precomputed alpha
 27: has alpha
 28~31: color space
 
*/

#define SLIB_DEFINE_BITMAP_FORMAT(colorSpace, hasAlpha, isPA, planes, bitsPerSample, bitsPerPixel, ID) \
	(((sl_uint32)(colorSpace) << 28) | (hasAlpha << 27) | (isPA << 26) | (planes << 24) | (bitsPerSample << 16) | (bitsPerPixel << 8) | (ID))


namespace slib
{
	
	enum class BitmapFormat
	{

		None = 0,

		// RGB
		// 32 bit formats
		RGBA =				SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 1, 0, 1, 32, 32, 0x01),
		BGRA =				SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 1, 0, 1, 32, 32, 0x02),
		ARGB =				SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 1, 0, 1, 32, 32, 0x03),
		ABGR =				SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 1, 0, 1, 32, 32, 0x04),

		// 32 bit formats, precomputed alpha
		RGBA_PA =			SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 1, 1, 1, 32, 32, 0x05),
		BGRA_PA =			SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 1, 1, 1, 32, 32, 0x06),
		ARGB_PA =			SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 1, 1, 1, 32, 32, 0x07),
		ABGR_PA =			SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 1, 1, 1, 32, 32, 0x08),

		// 24 bit formats
		RGB =				SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 0, 0, 1, 24, 24, 0x10),
		BGR =				SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 0, 0, 1, 24, 24, 0x11),

		// 16 bit formats
		RGB565BE =			SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 0, 0, 1, 16, 16, 0x16),
		RGB565LE =			SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 0, 0, 1, 16, 16, 0x17),
		BGR565BE =			SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 0, 0, 1, 16, 16, 0x18),
		BGR565LE =			SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 0, 0, 1, 16, 16, 0x19),

		// 8 bit formats
		GRAY8 =			SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 0, 0, 1, 8, 8, 0x20),

		// planar formats
		RGBA_PLANAR =		SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 1, 0, 4, 8, 32, 0x30),
		RGBA_PLANAR_PA =	SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 1, 1, 4, 8, 32, 0x31), // precomputed alpha
		RGB_PLANAR =		SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::RGB, 0, 0, 3, 8, 24, 0x32),

		// YUV
		// 32 bit formats
		YUVA =				SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::YUV, 1, 0, 1, 32, 32, 0x80),

		// 24 bit formats
		YUV444 =			SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::YUV, 0, 0, 1, 24, 24, 0x88),

		// planar formats
		YUVA_PLANAR =		SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::YUV, 1, 0, 4, 8, 32, 0x82),
		YUV444_PLANAR =	SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::YUV, 0, 0, 3, 8, 24, 0x89),

		// YUV420 planar formats
		YUV_I420 =			SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::YUV, 0, 0, 3, 0, 12, 0xB0), // 8 bit Y plane followed by 8 bit 2x2 subsampled U and V planes
		YUV_YV12 =			SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::YUV, 0, 0, 3, 0, 12, 0xB1), // 8 bit Y plane followed by 8 bit 2x2 subsampled V and U planes
		YUV_NV21 =			SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::YUV, 0, 0, 2, 0, 12, 0xB2), // 8-bit Y plane followed by an interleaved V/U plane with 2x2 subsampling (Android camera standard)
		YUV_NV12 =			SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::YUV, 0, 0, 2, 0, 12, 0xB3), // 8-bit Y plane followed by an interleaved U/V plane with 2x2 subsampling

		// YUV422 formats
		YUYV = SLIB_DEFINE_BITMAP_FORMAT(ColorSpace::YUV, 0, 0, 1, 16, 16, 0xC0)  // Y8 U8 Y8 V8

	};
	
	
	class SLIB_EXPORT BitmapFormats
	{
	public:
		static ColorSpace getColorSpace(BitmapFormat format);

		static sl_bool hasAlpha(BitmapFormat format);

		static sl_bool isPrecomputedAlpha(BitmapFormat format);

		static sl_uint32 getPlanesCount(BitmapFormat format);

		static sl_uint32 getBitsPerSample(BitmapFormat format);

		static sl_uint32 getBytesPerSample(BitmapFormat format);

		static sl_uint32 getBitsPerPixel(BitmapFormat format);

		static sl_uint32 getBytesPerPixel(BitmapFormat format);

		static BitmapFormat getNonPrecomputedAlphaFormat(BitmapFormat format);

		static BitmapFormat getPrecomputedAlphaFormat(BitmapFormat format);

		static BitmapFormat getCompatibleRGBFormat(BitmapFormat format);

		static sl_bool isYUV_420(BitmapFormat format);

	};

}

#endif
