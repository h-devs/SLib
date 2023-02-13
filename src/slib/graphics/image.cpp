/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/graphics/image.h"

#include "slib/graphics/cmyk.h"
#include "slib/io/file.h"
#include "slib/core/asset.h"
#include "slib/core/scoped_buffer.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ImageDesc)

	ImageDesc::ImageDesc(): width(0), height(0), stride(0), colors(sl_null)
	{
	}


	SLIB_DEFINE_OBJECT(Image, Bitmap)

	Image::Image()
	{
	}

	Image::~Image()
	{
	}

	Ref<Image> Image::allocate(sl_uint32 width, sl_uint32 height)
	{
		if (!width || !height) {
			return sl_null;
		}
		Memory mem = Memory::create((width * height) << 2);
		if (mem.isNull()) {
			return sl_null;
		}
		Ref<Image> ret = new Image;
		if (ret.isNull()) {
			return sl_null;
		}
		ImageDesc& desc = ret->m_desc;
		desc.width = width;
		desc.height = height;
		desc.stride = width;
		desc.ref = mem.ref;
		desc.colors = (Color*)(mem.getData());
		return ret;
	}

	Ref<Image> Image::createStatic(const ImageDesc& desc)
	{
		return createStatic(desc.width, desc.height, desc.colors, desc.stride, desc.ref.get());
	}

	Ref<Image> Image::createStatic(sl_uint32 width, sl_uint32 height, const Color* pixels, sl_reg stride, Referable* ref)
	{
		if (!width || !height || !pixels) {
			return sl_null;
		}
		if (!stride) {
			stride = width;
		}
		Ref<Image> ret = new Image;
		if (ret.isNull()) {
			return sl_null;
		}
		ImageDesc& desc = ret->m_desc;
		desc.colors = (Color*)pixels;
		desc.width = width;
		desc.height = height;
		desc.stride = stride;
		desc.ref = ref;
		return ret;
	}

	Ref<Image> Image::create(sl_uint32 width, sl_uint32 height, const Color* pixels, sl_reg stride)
	{
		Ref<Image> ret = allocate(width, height);
		if (ret.isNull()) {
			return sl_null;
		}
		if (pixels) {
			const Color* sr = pixels;
			Color* dr = ret->getColors();
			for (sl_uint32 i = 0; i < height; i++) {
				const Color* ss = sr;
				Color* ds = dr;
				for (sl_uint32 j = 0; j < width; j++) {
					*ds = *ss;
					ss++;
					ds++;
				}
				sr += stride;
				dr += width;
			}
		} else {
			Base::zeroMemory(ret->getColors(), (width * height) << 2);
		}
		return ret;
	}

	Ref<Image> Image::create(const ImageDesc& desc)
	{
		return create(desc.width, desc.height, desc.colors, desc.stride);
	}

	Ref<Image> Image::create(const ImageDesc& desc, RotationMode rotate, FlipMode flip)
	{
		NormalizeRotateAndFlip(rotate, flip);
		if (rotate == RotationMode::Rotate0 && flip == FlipMode::None) {
			return create(desc.width, desc.height, desc.colors, desc.stride);
		}
		sl_uint32 width = desc.width;
		sl_uint32 height = desc.height;
		sl_int32 widthDst, heightDst;
		if (rotate == RotationMode::Rotate90 || rotate == RotationMode::Rotate270) {
			widthDst = height;
			heightDst = width;
		} else {
			widthDst = width;
			heightDst = height;
		}
		Ref<Image> ret = allocate(widthDst, heightDst);
		if (ret.isNull()) {
			return sl_null;
		}
		const Color* colors = desc.colors;
		if (!colors) {
			Base::zeroMemory(ret->getColors(), (widthDst * heightDst) << 2);
			return ret;
		}
		sl_reg stride = desc.stride;
		const Color* sr = colors;
		Color* dr = ret->getColors();
		sl_int32 incCol, incRow;
		if (rotate == RotationMode::Rotate0) {
			if (flip == FlipMode::Horizontal) {
				incCol = -1;
				incRow = widthDst;
				dr += (widthDst - 1);
			} else { // always FlipMode::Vertical
				incCol = 1;
				incRow = -widthDst;
				dr += widthDst * (heightDst - 1);
			}
		} else if (rotate == RotationMode::Rotate180) {
			// always FlipMode::None
			incCol = -1;
			incRow = -widthDst;
			dr += widthDst * heightDst - 1;
		} else if (rotate == RotationMode::Rotate90) {
			if (flip == FlipMode::Horizontal) {
				incCol = widthDst;
				incRow = 1;
			} else { // always FlipMode::None
				incCol = widthDst;
				incRow = -1;
				dr += (widthDst - 1);
			}
		} else { // always RotationMode::Rotate270
			if (flip == FlipMode::Horizontal) {
				incCol = -widthDst;
				incRow = -1;
				dr += widthDst * heightDst - 1;
			} else { // always FlipMode::None
				incCol = -widthDst;
				incRow = 1;
				dr += widthDst * (heightDst - 1);
			}
		}
		for (sl_uint32 i = 0; i < height; i++) {
			const Color* ss = sr;
			Color* ds = dr;
			for (sl_uint32 j = 0; j < width; j++) {
				*ds = *ss;
				ss++;
				ds += incCol;
			}
			sr += stride;
			dr += incRow;
		}
		return ret;
	}

	Ref<Image> Image::create(const BitmapData& bitmapData)
	{
		sl_uint32 width = bitmapData.width;
		sl_uint32 height = bitmapData.height;
		if (!width || !height) {
			return sl_null;
		}
		Ref<Image> ret = allocate(width, height);
		if (ret.isNull()) {
			return sl_null;
		}
		BitmapData dst(width, height, ret->getColors());
		if (BitmapFormats::isPrecomputedAlpha(bitmapData.format)) {
			dst.format = BitmapFormats::getPrecomputedAlphaFormat(dst.format);
		}
		dst.copyPixelsFrom(bitmapData);
		return ret;
	}

	namespace {

		static sl_bool IsValidBitsPerComponent(sl_uint32 bitsPerComponent) noexcept
		{
			switch (bitsPerComponent) {
				case 1:
				case 2:
				case 4:
				case 8:
				case 16:
					return sl_true;
				default:
					break;
			}
			return sl_false;
		}

		template <int N>
		class BitReader;

		template <>
		class BitReader<1>
		{
		public:
			SLIB_INLINE static sl_uint8 read(sl_uint8* row, sl_uint32 index) noexcept
			{
				sl_uint32 k = index >> 3;
				sl_uint32 m = 7 - (index & 7);
				return (row[k] >> m) & 1;
			}

			SLIB_INLINE static sl_uint8 readScaled(sl_uint8* row, sl_uint32 index) noexcept
			{
				return (sl_uint8)(-(read(row, index)));
			}
		};

		template <>
		class BitReader<2>
		{
		public:
			SLIB_INLINE static sl_uint8 read(sl_uint8* row, sl_uint32 index) noexcept
			{
				sl_uint32 k = index >> 2;
				sl_uint32 m = (3 - (index & 3)) << 1;
				return (row[k] >> m) & 3;
			}

			SLIB_INLINE static sl_uint8 readScaled(sl_uint8* row, sl_uint32 index) noexcept
			{
				return (sl_uint8)(read(row, index) * 85);
			}
		};

		template <>
		class BitReader<4>
		{
		public:
			SLIB_INLINE static sl_uint8 read(sl_uint8* row, sl_uint32 index) noexcept
			{
				sl_uint32 k = index >> 1;
				sl_uint8 c = (index & 1) ? (row[k] & 15) : (row[k] >> 4);
				return c;
			}

			SLIB_INLINE static sl_uint8 readScaled(sl_uint8* row, sl_uint32 index) noexcept
			{
				return (sl_uint8)(read(row, index) * 17);
			}
		};

		template <int BITS_PER_COMP>
		static void CopyRGB_Bits(sl_uint32 width, sl_uint32 height, Color* dst, sl_reg dst_stride, sl_uint8* src, sl_reg src_pitch) noexcept
		{
			sl_uint32 nComp = width * 3;
			for (sl_uint32 iRow = 0; iRow < height; iRow++) {
				Color* c = dst;
				sl_uint32 iComp = 0;
				while (iComp < nComp) {
					c->r = BitReader<BITS_PER_COMP>::readScaled(src, iComp++);
					c->g = BitReader<BITS_PER_COMP>::readScaled(src, iComp++);
					c->b = BitReader<BITS_PER_COMP>::readScaled(src, iComp++);
					c->a = 255;
					c++;
				}
				src += src_pitch;
				dst += dst_stride;
			}
		}

		template <sl_uint32 BYTES_PER_COMP>
		static void CopyRGB_Bytes(sl_uint32 width, sl_uint32 height, Color* dst, sl_reg dst_stride, sl_uint8* src, sl_reg src_pitch) noexcept
		{
			for (sl_uint32 iRow = 0; iRow < height; iRow++) {
				sl_uint8* s = src;
				Color* c = dst;
				for (sl_uint32 iCol = 0; iCol < width; iCol++) {
					c->r = *s;
					s += BYTES_PER_COMP;
					c->g = *s;
					s += BYTES_PER_COMP;
					c->b = *s;
					s += BYTES_PER_COMP;
					c->a = 255;
					c++;
				}
				src += src_pitch;
				dst += dst_stride;
			}
		}

		template <int BITS_PER_COMP>
		static void CopyGray_Bits(sl_uint32 width, sl_uint32 height, Color* dst, sl_reg dst_stride, sl_uint8* src, sl_reg src_pitch) noexcept
		{
			for (sl_uint32 iRow = 0; iRow < height; iRow++) {
				Color* c = dst;
				for (sl_uint32 iCol = 0; iCol < width; iCol++) {
					sl_uint8 v = BitReader<BITS_PER_COMP>::readScaled(src, iCol);
					c->r = v;
					c->g = v;
					c->b = v;
					c->a = 255;
					c++;
				}
				src += src_pitch;
				dst += dst_stride;
			}
		}

		template <sl_uint32 BYTES_PER_COMP>
		static void CopyGray_Bytes(sl_uint32 width, sl_uint32 height, Color* dst, sl_reg dst_stride, sl_uint8* src, sl_reg src_pitch) noexcept
		{
			for (sl_uint32 iRow = 0; iRow < height; iRow++) {
				sl_uint8* s = src;
				Color* c = dst;
				for (sl_uint32 iCol = 0; iCol < width; iCol++) {
					sl_uint8 v = *s;
					c->r = v;
					c->g = v;
					c->b = v;
					c->a = 255;
					s += BYTES_PER_COMP;
					c++;
				}
				src += src_pitch;
				dst += dst_stride;
			}
		}

		template <int BITS_PER_COMP>
		static void CopyAlpha_Bits(sl_uint32 width, sl_uint32 height, Color* dst, sl_reg dst_stride, sl_uint8* src, sl_reg src_pitch) noexcept
		{
			for (sl_uint32 iRow = 0; iRow < height; iRow++) {
				Color* c = dst;
				for (sl_uint32 iCol = 0; iCol < width; iCol++) {
					c->a = BitReader<BITS_PER_COMP>::readScaled(src, iCol);
					c->r = 255;
					c->g = 255;
					c->b = 255;
					c++;
				}
				src += src_pitch;
				dst += dst_stride;
			}
		}

		template <sl_uint32 BYTES_PER_COMP>
		static void CopyAlpha_Bytes(sl_uint32 width, sl_uint32 height, Color* dst, sl_reg dst_stride, sl_uint8* src, sl_reg src_pitch) noexcept
		{
			for (sl_uint32 iRow = 0; iRow < height; iRow++) {
				sl_uint8* s = src;
				Color* c = dst;
				for (sl_uint32 iCol = 0; iCol < width; iCol++) {
					c->a = *s;
					c->r = 255;
					c->g = 255;
					c->b = 255;
					s += BYTES_PER_COMP;
					c++;
				}
				src += src_pitch;
				dst += dst_stride;
			}
		}

		template <int BITS_PER_COMP>
		static void CopyIndexed_Bits(sl_uint32 width, sl_uint32 height, Color* dst, sl_reg dst_stride, sl_uint8* src, sl_reg src_pitch, const Color* indices, sl_uint32 nIndices) noexcept
		{
			for (sl_uint32 iRow = 0; iRow < height; iRow++) {
				Color* c = dst;
				for (sl_uint32 iCol = 0; iCol < width; iCol++) {
					sl_uint8 v = BitReader<BITS_PER_COMP>::read(src, iCol);
					if (v < nIndices) {
						*c = indices[v];
					} else {
						c->setZero();
					}
					c++;
				}
				src += src_pitch;
				dst += dst_stride;
			}
		}

		template <sl_uint32 BYTES_PER_COMP>
		class IndexReader;

		template <>
		class IndexReader<1>
		{
		public:
			SLIB_INLINE static const Color& read(sl_uint8* s, const Color* indices, sl_uint32 nIndices) noexcept
			{
				sl_uint8 v = *s;
				if (v < nIndices) {
					return indices[v];
				} else {
					return Color::zero();
				}
			}
		};

		template <>
		class IndexReader<2>
		{
		public:
			SLIB_INLINE static const Color& read(sl_uint8* s, const Color* indices, sl_uint32 nIndices) noexcept
			{
				sl_uint16 v = SLIB_MAKE_WORD(*s, s[1]);
				if (v < nIndices) {
					return indices[v];
				} else {
					return Color::zero();
				}
			}
		};

		template <sl_uint32 BYTES_PER_COMP>
		static void CopyIndexed_Bytes(sl_uint32 width, sl_uint32 height, Color* dst, sl_reg dst_stride, sl_uint8* src, sl_reg src_pitch, const Color* indices, sl_uint32 nIndices) noexcept
		{
			for (sl_uint32 iRow = 0; iRow < height; iRow++) {
				sl_uint8* s = src;
				Color* c = dst;
				for (sl_uint32 iCol = 0; iCol < width; iCol++) {
					*c = IndexReader<BYTES_PER_COMP>::read(s, indices, nIndices);
					s += BYTES_PER_COMP;
					c++;
				}
				src += src_pitch;
				dst += dst_stride;
			}
		}

	}

#define DEFINE_CREATE_FROM_FUNC(FUNC_PREFIX, ...) \
		if (width && height) { \
			if (!(IsValidBitsPerComponent(bitsPerComponent))) { \
				return sl_null; \
			} \
			Ref<Image> ret = Image::allocate(width, height); \
			if (ret.isNull()) { \
				return sl_null; \
			} \
			sl_uint8* data = (sl_uint8*)_data; \
			Color* colors = ret->getColors(); \
			if (!pitch) { \
				pitch = (3 * bitsPerComponent * width + 7) >> 3; \
			} \
			switch (bitsPerComponent) { \
				case 1: \
					FUNC_PREFIX##_Bits<1>(width, height, colors, width, data, pitch, ##__VA_ARGS__); \
					break; \
				case 2: \
					FUNC_PREFIX##_Bits<2>(width, height, colors, width, data, pitch, ##__VA_ARGS__); \
					break; \
				case 4: \
					FUNC_PREFIX##_Bits<4>(width, height, colors, width, data, pitch, ##__VA_ARGS__); \
					break; \
				case 8: \
					FUNC_PREFIX##_Bytes<1>(width, height, colors, width, data, pitch, ##__VA_ARGS__); \
					break; \
				case 16: \
					FUNC_PREFIX##_Bytes<2>(width, height, colors, width, data, pitch, ##__VA_ARGS__); \
					break; \
				default: \
					break; \
			} \
			return ret; \
		} \
		return sl_null;


	Ref<Image> Image::createFromRGB(sl_uint32 width, sl_uint32 height, const void* _data, sl_uint32 bitsPerComponent, sl_reg pitch)
	{
		DEFINE_CREATE_FROM_FUNC(CopyRGB)
	}

	Ref<Image> Image::createFromGray(sl_uint32 width, sl_uint32 height, const void* _data, sl_uint32 bitsPerComponent, sl_reg pitch)
	{
		DEFINE_CREATE_FROM_FUNC(CopyGray)
	}

	Ref<Image> Image::createCopyAlphaFromGray(sl_uint32 width, sl_uint32 height, const void* _data, sl_uint32 bitsPerComponent, sl_reg pitch)
	{
		DEFINE_CREATE_FROM_FUNC(CopyAlpha)
	}

	Ref<Image> Image::createFromIndexed(sl_uint32 width, sl_uint32 height, const void* _data, const Color* indices, sl_uint32 nIndices, sl_uint32 bitsPerComponent, sl_reg pitch)
	{
		DEFINE_CREATE_FROM_FUNC(CopyIndexed, indices, nIndices)
	}

	namespace {
		template <sl_uint32 BYTES_PER_COMP>
		static void CopyCMYK(sl_uint32 width, sl_uint32 height, Color* dst, sl_reg dst_stride, sl_uint8* src, sl_reg src_pitch) noexcept
		{
			for (sl_uint32 iRow = 0; iRow < height; iRow++) {
				sl_uint8* s = src;
				Color* c = dst;
				for (sl_uint32 iCol = 0; iCol < width; iCol++) {
					sl_uint8 C = *s;
					s += BYTES_PER_COMP;
					sl_uint8 M = *s;
					s += BYTES_PER_COMP;
					sl_uint8 Y = *s;
					s += BYTES_PER_COMP;
					sl_uint8 K = *s;
					s += BYTES_PER_COMP;
					CMYK::convertCMYKToRGB(C, M, Y, K, c->r, c->g, c->b);
					c->a = 255;
					c++;
				}
				src += src_pitch;
				dst += dst_stride;
			}
		}
	}

	Ref<Image> Image::createFromCMYK(sl_uint32 width, sl_uint32 height, const void* _data, sl_uint32 bitsPerComponent, sl_reg pitch)
	{
		if (width && height && (bitsPerComponent == 8 || bitsPerComponent == 16)) {
			Ref<Image> ret = Image::allocate(width, height);
			if (ret.isNull()) {
				return sl_null;
			}
			sl_uint8* data = (sl_uint8*)_data;
			Color* colors = ret->getColors();
			if (!pitch) {
				pitch = ((bitsPerComponent << 2) * width + 7) >> 3;
			}
			if (bitsPerComponent == 8) {
				CopyCMYK<1>(width, height, colors, width, data, pitch);
			} else {
				CopyCMYK<2>(width, height, colors, width, data, pitch);
			}
			return ret;
		}
		return sl_null;
	}

	Ref<Image> Image::createCopy(const Ref<Image>& image)
	{
		if (image.isNotNull()) {
			ImageDesc& desc = image->m_desc;
			return create(desc.width, desc.height, desc.colors, desc.stride);
		}
		return sl_null;
	}

	Ref<Image> Image::createCopy(const Ref<Image>& image, RotationMode rotate, FlipMode flip)
	{
		if (image.isNotNull()) {
			ImageDesc& desc = image->m_desc;
			return create(desc, rotate, flip);
		}
		return sl_null;
	}

	Ref<Image> Image::createCopyBitmap(const Ref<Bitmap>& bitmap)
	{
		if (bitmap.isNull()) {
			return sl_null;
		}
		sl_uint32 width = bitmap->getWidth();
		sl_uint32 height = bitmap->getHeight();
		if (width <= 0 || height <= 0) {
			return sl_null;
		}
		Memory mem = Memory::create((width*height) << 2);
		if (mem.isNull()) {
			return sl_null;
		}
		Color* buf = (Color*)(mem.getData());
		if (bitmap->readPixels(0, 0, width, height, buf)) {
			ImageDesc desc;
			desc.width = width;
			desc.height = height;
			desc.stride = width;
			desc.colors = buf;
			desc.ref = mem.ref;
			return Image::createStatic(desc);
		}
		return sl_null;
	}

	Ref<Image> Image::createCopyBitmap(const Ref<Bitmap>& bitmap, sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height)
	{
		if (bitmap.isNull()) {
			return sl_null;
		}
		if (width <= 0 || height <= 0) {
			return sl_null;
		}
		sl_uint32 bw = bitmap->getWidth();
		sl_uint32 bh = bitmap->getHeight();
		if (x >= bw) {
			return sl_null;
		}
		if (y >= bh) {
			return sl_null;
		}
		if (width > bw - x) {
			return sl_null;
		}
		if (height > bh - y) {
			return sl_null;
		}
		Memory mem = Memory::create((width*height) << 2);
		if (mem.isNull()) {
			return sl_null;
		}
		Color* buf = (Color*)(mem.getData());
		if (bitmap->readPixels(x, y, width, height, buf)) {
			ImageDesc desc;
			desc.width = width;
			desc.height = height;
			desc.stride = width;
			desc.colors = buf;
			desc.ref = mem.ref;
			return Image::createStatic(desc);
		}
		return sl_null;
	}

	sl_uint32 Image::getWidth() const
	{
		return m_desc.width;
	}

	sl_uint32 Image::getHeight() const
	{
		return m_desc.height;
	}

	sl_bool Image::isEmpty() const
	{
		return !(m_desc.width) || !(m_desc.height);
	}

	sl_bool Image::isNotEmpty() const
	{
		return m_desc.width != 0 && m_desc.height != 0;
	}

	sl_reg Image::getStride() const
	{
		return m_desc.stride;
	}

	Color* Image::getColors() const
	{
		return (Color*)(m_desc.colors);
	}

	Color* Image::getColorsAt(sl_int32 x, sl_int32 y) const
	{
		return getColors() + x + y * m_desc.stride;
	}

	Color& Image::getPixel(sl_int32 x, sl_int32 y) const
	{
		return getColors()[x + y * m_desc.stride];
	}

	void Image::getDesc(ImageDesc& desc) const
	{
		desc = m_desc;
	}


	sl_uint32 Image::getBitmapWidth()
	{
		return m_desc.width;
	}

	sl_uint32 Image::getBitmapHeight()
	{
		return m_desc.height;
	}

	sl_bool Image::readPixels(sl_uint32 x, sl_uint32 y, BitmapData& bmd)
	{
		ImageDesc& imd = m_desc;
		if (x >= imd.width || y >= imd.height) {
			return sl_false;
		}
		if (x + bmd.width > imd.width || y + bmd.height > imd.height) {
			return sl_false;
		}
		Color* colors = getColorsAt(x, y);
		BitmapData src(bmd.width, bmd.height, colors, imd.stride);
		bmd.copyPixelsFrom(src);
		return sl_true;
	}

	sl_bool Image::writePixels(sl_uint32 x, sl_uint32 y, const BitmapData& bmd)
	{
		ImageDesc& imd = m_desc;
		if (x >= imd.width || y >= imd.height) {
			return sl_false;
		}
		if (x + bmd.width > imd.width || y + bmd.height > imd.height) {
			return sl_false;
		}
		Color* colors = getColorsAt(x, y);
		BitmapData src(bmd.width, bmd.height, colors, imd.stride);
		src.copyPixelsFrom(bmd);
		return sl_true;
	}

	sl_bool Image::resetPixels(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height, const Color& color)
	{
		ImageDesc& desc = m_desc;
		if (x >= desc.width || y >= desc.height) {
			return sl_false;
		}
		if (x + width > desc.width) {
			width = desc.width - x;
		}
		if (y + height > desc.height) {
			height = desc.height - y;
		}
		sl_reg stride = desc.stride;
		Color* colors = getColorsAt(x, y);
		for (sl_uint32 yi = 0; yi < height; yi++) {
			Color* d = colors;
			for (sl_uint32 xi = 0; xi < width; xi++) {
				*d = color;
				d++;
			}
			colors += stride;
		}
		return sl_true;
	}

	sl_bool Image::readPixels(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height, Color* colors, sl_reg stride)
	{
		BitmapData bitmapData(width, height, colors, stride);
		return readPixels(x, y, bitmapData);
	}

	sl_bool Image::writePixels(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height, const Color* colors, sl_reg stride)
	{
		BitmapData bitmapData(width, height, colors, stride);
		return writePixels(x, y, bitmapData);
	}

	sl_bool Image::resetPixels(const Color& color)
	{
		return resetPixels(0, 0, getWidth(), getHeight(), color);
	}

	void Image::fillColor(const Color& color)
	{
		sl_uint32 width = m_desc.width;
		sl_uint32 height = m_desc.height;
		sl_reg stride = m_desc.stride;
		Color* colors = m_desc.colors;
		for (sl_uint32 y = 0; y < height; y++) {
			Color* d = colors;
			for (sl_uint32 x = 0; x < width; x++) {
				*d = color;
				d++;
			}
			colors += stride;
		}
	}

	void Image::tintColor(const Color& color)
	{
		sl_uint32 width = m_desc.width;
		sl_uint32 height = m_desc.height;
		sl_reg stride = m_desc.stride;
		Color* colors = m_desc.colors;
		for (sl_uint32 y = 0; y < height; y++) {
			Color* d = colors;
			for (sl_uint32 x = 0; x < width; x++) {
				d->r = color.r;
				d->g = color.g;
				d->b = color.b;
				d++;
			}
			colors += stride;
		}
	}

	void Image::blend(const Color& colorBack, const Ref<Image>& imageOver)
	{
		if (imageOver.isNull()) {
			return;
		}
		sl_uint32 width = getWidth();
		sl_uint32 height = getHeight();
		if (imageOver->getWidth() != width) {
			return;
		}
		if (imageOver->getHeight() != height) {
			return;
		}
		Color* rowDst = getColors();
		Color* rowSrc = imageOver->getColors();
		sl_reg strideDst = getStride();
		sl_reg strideSrc = imageOver->getStride();
		for (sl_uint32 y = 0; y < height; y++) {
			Color* dst = rowDst;
			Color* src = rowSrc;
			for (sl_uint32 x = 0; x < width; x++) {
				Color c = colorBack;
				c.blend_NPA_NPA(*src);
				*dst = c;
				dst++;
				src++;
			}
			rowDst += strideDst;
			rowSrc += strideSrc;
		}
	}

	void Image::blend(const Ref<Image>& imageBack, const Color& colorOver)
	{
		if (imageBack.isNull()) {
			return;
		}
		sl_uint32 width = getWidth();
		sl_uint32 height = getHeight();
		if (imageBack->getWidth() != width) {
			return;
		}
		if (imageBack->getHeight() != height) {
			return;
		}
		Color* rowDst = getColors();
		Color* rowSrc = imageBack->getColors();
		sl_reg strideDst = getStride();
		sl_reg strideSrc = imageBack->getStride();
		for (sl_uint32 y = 0; y < height; y++) {
			Color* dst = rowDst;
			Color* src = rowSrc;
			for (sl_uint32 x = 0; x < width; x++) {
				*dst = *src;
				dst->blend_NPA_NPA(colorOver);
				dst++;
				src++;
			}
			rowDst += strideDst;
			rowSrc += strideSrc;
		}
	}

	void Image::makeGray()
	{
		sl_uint32 width = m_desc.width;
		sl_uint32 height = m_desc.height;
		sl_reg stride = m_desc.stride;
		Color* colors = m_desc.colors;
		for (sl_uint32 y = 0; y < height; y++) {
			Color* d = colors;
			for (sl_uint32 x = 0; x < width; x++) {
				sl_uint8 g = (sl_uint8)(((sl_uint32)(d->r) + (sl_uint32)(d->g) + (sl_uint32)(d->b)) / 3);
				d->r = g;
				d->g = g;
				d->b = g;
				d++;
			}
			colors += stride;
		}
	}

	namespace {

		template <int BITS_PER_COMP>
		static void CopyOnlyAlpha_Bits(sl_uint32 width, sl_uint32 height, Color* dst, sl_reg dst_stride, sl_uint8* src, sl_reg src_pitch) noexcept
		{
			for (sl_uint32 iRow = 0; iRow < height; iRow++) {
				Color* c = dst;
				for (sl_uint32 iCol = 0; iCol < width; iCol++) {
					c->a = BitReader<BITS_PER_COMP>::readScaled(src, iCol);
					c++;
				}
				src += src_pitch;
				dst += dst_stride;
			}
		}

		template <sl_uint32 BYTES_PER_COMP>
		static void CopyOnlyAlpha_Bytes(sl_uint32 width, sl_uint32 height, Color* dst, sl_reg dst_stride, sl_uint8* src, sl_reg src_pitch) noexcept
		{
			for (sl_uint32 iRow = 0; iRow < height; iRow++) {
				sl_uint8* s = src;
				Color* c = dst;
				for (sl_uint32 iCol = 0; iCol < width; iCol++) {
					c->a = *s;
					s += BYTES_PER_COMP;
					c++;
				}
				src += src_pitch;
				dst += dst_stride;
			}
		}

	}

	void Image::writeAlphaFromGray(sl_uint32 width, sl_uint32 height, const void* _data, sl_uint32 bitsPerComponent, sl_reg pitch)
	{
		sl_uint32 widthDst = m_desc.width;
		sl_uint32 heightDst = m_desc.height;
		if (width > widthDst) {
			width = widthDst;
		}
		if (height > heightDst) {
			height = heightDst;
		}
		if (width && height) {
			if (!(IsValidBitsPerComponent(bitsPerComponent))) {
				return;
			}
			sl_uint8* data = (sl_uint8*)_data;
			Color* colors = m_desc.colors;
			sl_reg stride = m_desc.stride;
			if (!pitch) {
				pitch = (3 * bitsPerComponent * width + 7) >> 3;
			}
			switch (bitsPerComponent) {
				case 1:
					CopyOnlyAlpha_Bits<1>(width, height, colors, stride, data, pitch);
					break;
				case 2:
					CopyOnlyAlpha_Bits<2>(width, height, colors, stride, data, pitch);
					break;
				case 4:
					CopyOnlyAlpha_Bits<4>(width, height, colors, stride, data, pitch);
					break;
				case 8:
					CopyOnlyAlpha_Bytes<1>(width, height, colors, stride, data, pitch);
					break;
				case 16:
					CopyOnlyAlpha_Bytes<2>(width, height, colors, stride, data, pitch);
					break;
				default:
					break;
			}
		}
	}

	namespace {

		template <int BITS_PER_COMP>
		static void MultiplyAlpha_Bits(sl_uint32 width, sl_uint32 height, Color* dst, sl_reg dst_stride, sl_uint8* src, sl_reg src_pitch) noexcept
		{
			for (sl_uint32 iRow = 0; iRow < height; iRow++) {
				Color* c = dst;
				for (sl_uint32 iCol = 0; iCol < width; iCol++) {
					c->a = (sl_uint8)((sl_uint32)(c->a) * (sl_uint32)(BitReader<BITS_PER_COMP>::read(src, iCol)) / ((1 << BITS_PER_COMP) - 1));
					c++;
				}
				src += src_pitch;
				dst += dst_stride;
			}
		}

		template <sl_uint32 BYTES_PER_COMP>
		static void MultiplyAlpha_Bytes(sl_uint32 width, sl_uint32 height, Color* dst, sl_reg dst_stride, sl_uint8* src, sl_reg src_pitch) noexcept
		{
			for (sl_uint32 iRow = 0; iRow < height; iRow++) {
				sl_uint8* s = src;
				Color* c = dst;
				for (sl_uint32 iCol = 0; iCol < width; iCol++) {
					c->a = (sl_uint8)(((sl_uint32)(c->a) * (sl_uint32)(*s)) / 255);
					s += BYTES_PER_COMP;
					c++;
				}
				src += src_pitch;
				dst += dst_stride;
			}
		}

	}

	void Image::multiplyAlphaFromGray(sl_uint32 width, sl_uint32 height, const void* _data, sl_uint32 bitsPerComponent, sl_reg pitch)
	{
		sl_uint32 widthDst = m_desc.width;
		sl_uint32 heightDst = m_desc.height;
		if (width > widthDst) {
			width = widthDst;
		}
		if (height > heightDst) {
			height = heightDst;
		}
		if (width && height) {
			if (!(IsValidBitsPerComponent(bitsPerComponent))) {
				return;
			}
			sl_uint8* data = (sl_uint8*)_data;
			Color* colors = m_desc.colors;
			sl_reg stride = m_desc.stride;
			if (!pitch) {
				pitch = (3 * bitsPerComponent * width + 7) >> 3;
			}
			switch (bitsPerComponent) {
				case 1:
					MultiplyAlpha_Bits<1>(width, height, colors, stride, data, pitch);
					break;
				case 2:
					MultiplyAlpha_Bits<2>(width, height, colors, stride, data, pitch);
					break;
				case 4:
					MultiplyAlpha_Bits<4>(width, height, colors, stride, data, pitch);
					break;
				case 8:
					MultiplyAlpha_Bytes<1>(width, height, colors, stride, data, pitch);
					break;
				case 16:
					MultiplyAlpha_Bytes<2>(width, height, colors, stride, data, pitch);
					break;
				default:
					break;
			}
		}
	}

	namespace {

		class ColorOp_None
		{
		public:
			SLIB_INLINE const Color& operator()(const Color& src) const
			{
				return src;
			}
		};

		class ColorOp_MulAddColor
		{
		public:
			Color4f colorMul;
			Color4f colorAdd;

		public:
			SLIB_INLINE ColorOp_MulAddColor(const Color4f& _colorMul, const Color4f& _colorAdd): colorMul(_colorMul), colorAdd(_colorAdd)
			{
			}

		public:
			SLIB_INLINE Color operator()(const Color& src) const
			{
				return Color(
					Math::clamp0_255((sl_int32)(src.r * colorMul.x + colorAdd.x * 255)),
					Math::clamp0_255((sl_int32)(src.g * colorMul.y + colorAdd.y * 255)),
					Math::clamp0_255((sl_int32)(src.b * colorMul.z + colorAdd.z * 255)),
					Math::clamp0_255((sl_int32)(src.a * colorMul.w + colorAdd.w * 255)));
			}
		};

		class ColorOp_ColorMatrix
		{
		public:
			const ColorMatrix* cm;

		public:
			SLIB_INLINE ColorOp_ColorMatrix(const ColorMatrix* _cm) : cm(_cm)
			{
			}

		public:
			SLIB_INLINE Color operator()(const Color& src) const
			{
				return cm->transformColor(src);
			}
		};

		template <class ColorOp>
		class Blend_Copy
		{
		public:
			ColorOp src_op;

		public:
			Blend_Copy(const ColorOp& op): src_op(op)
			{
			}

		public:
			SLIB_INLINE void operator()(Color& dst, const Color& src) const
			{
				dst = src_op(src);
			}
		};

		template <class ColorOp>
		class Blend_Over
		{
		public:
			ColorOp src_op;

		public:
			Blend_Over(const ColorOp& op): src_op(op)
			{
			}

		public:
			SLIB_INLINE void operator()(Color& dst, const Color& src) const
			{
				dst.blend_PA_NPA(src_op(src));
			}
		};

		class Stretch_FillColor
		{
		public:
			template <class BLEND>
			static void stretch(const BLEND& blend, ImageDesc& dst, const ImageDesc& src)
			{
				Color* colorsDst = dst.colors;
				Color color = *(src.colors);
				for (sl_uint32 y = 0; y < dst.height; y++) {
					for (sl_uint32 x = 0; x < dst.width; x++) {
						blend(colorsDst[x], color);
					}
					colorsDst += dst.stride;
				}
			}
		};

		class Stretch_Copy
		{
		public:
			template <class BLEND>
			static void stretch(const BLEND& blend, ImageDesc& dst, const ImageDesc& src)
			{
				Color* colorsDst = dst.colors;
				const Color* colorsSrc = src.colors;
				for (sl_uint32 y = 0; y < dst.height; y++) {
					for (sl_uint32 x = 0; x < dst.width; x++) {
						blend(colorsDst[x], colorsSrc[x]);
					}
					colorsDst += dst.stride;
					colorsSrc += src.stride;
				}
			}
		};

		class Stretch_Nearest
		{
		public:
			template <class BLEND>
			static void stretchX(const BLEND& blend, ImageDesc& dst, const ImageDesc& src)
			{
				sl_uint32 dx, dy;

				SLIB_SCOPED_BUFFER(sl_uint32, 1024, mapx, dst.width);
				if (!mapx) {
					return;
				}
				sl_uint32 tx = 0;
				for (dx = 0; dx < dst.width; dx++) {
					mapx[dx] = tx / dst.width;
					tx += src.width;
				}

				Color* colorsDst = dst.colors;
				const Color* colorsSrc = src.colors;
				for (dy = 0; dy < dst.height; dy++) {
					for (dx = 0; dx < dst.width; dx++) {
						blend(colorsDst[dx], colorsSrc[mapx[dx]]);
					}
					colorsDst += dst.stride;
					colorsSrc += src.stride;
				}
			}

			template <class BLEND>
			static void stretchY(const BLEND& blend, ImageDesc& dst, const ImageDesc& src)
			{
				sl_uint32 dx, dy;

				Color* colorsDst = dst.colors;
				for (dy = 0; dy < dst.height; dy++) {
					const Color* colorsSrc = src.colors + ((dy * src.height) / dst.height) * src.stride;
					for (dx = 0; dx < dst.width; dx++) {
						blend(colorsDst[dx], colorsSrc[dx]);
					}
					colorsDst += dst.stride;
				}
			}

			template <class BLEND>
			static void stretch(const BLEND& blend, ImageDesc& dst, const ImageDesc& src)
			{
				if (src.width == dst.width) {
					stretchX(blend, dst, src);
					return;
				}
				if (src.height == dst.height) {
					stretchY(blend, dst, src);
					return;
				}
				SLIB_SCOPED_BUFFER(sl_uint32, 1024, mapx, dst.width);
				if (!mapx) {
					return;
				}
				{
					sl_uint32 tx = 0;
					for (sl_uint32 dx = 0; dx < dst.width; dx++) {
						mapx[dx] = tx / dst.width;
						tx += src.width;
					}
				}
				Color* colorsDst = dst.colors;
				for (sl_uint32 dy = 0; dy < dst.height; dy++) {
					const Color* colorsSrc = src.colors + ((dy * src.height) / dst.height) * src.stride;
					for (sl_uint32 dx = 0; dx < dst.width; dx++) {
						blend(colorsDst[dx], colorsSrc[mapx[dx]]);
					}
					colorsDst += dst.stride;
				}
			}

		};

		struct Stretch_FilterParam
		{
			sl_bool flagBox;
			float filterSize;
		};

		class Stretch_Smooth_LinearFilter
		{
		public:
			SLIB_INLINE static void getColorAt(Color& _out, const Color* colors, float fx, float fy, sl_reg stride, const Stretch_FilterParam& px, const Stretch_FilterParam& py)
			{
				const Color& c00 = *colors;
				const Color& c01 = colors[1];
				const Color& c10 = colors[stride];
				const Color& c11 = colors[stride + 1];
				float sx = 1 - fx;
				float ex = fx;
				float sy = 1 - fy;
				float ey = fy;
				float f00 = sx * sy;
				float f01 = ex * sy;
				float f10 = sx * ey;
				float f11 = ex * ey;
				float r = (float)(c00.r) * f00 + (float)(c01.r) * f01 + (float)(c10.r) * f10 + (float)(c11.r) * f11;
				float g = (float)(c00.g) * f00 + (float)(c01.g) * f01 + (float)(c10.g) * f10 + (float)(c11.g) * f11;
				float b = (float)(c00.b) * f00 + (float)(c01.b) * f01 + (float)(c10.b) * f10 + (float)(c11.b) * f11;
				float a = (float)(c00.a) * f00 + (float)(c01.a) * f01 + (float)(c10.a) * f10 + (float)(c11.a) * f11;
				_out.r = (sl_uint8)(r);
				_out.g = (sl_uint8)(g);
				_out.b = (sl_uint8)(b);
				_out.a = (sl_uint8)(a);
			}

			SLIB_INLINE static void getColorAtX(Color& _out, const Color* colors, float fx, const Stretch_FilterParam& px)
			{
				const Color& c0 = *colors;
				const Color& c1 = colors[1];
				float f0 = 1 - fx;
				float f1 = fx;
				float r = (float)(c0.r) * f0 + (float)(c1.r) * f1;
				float g = (float)(c0.g) * f0 + (float)(c1.g) * f1;
				float b = (float)(c0.b) * f0 + (float)(c1.b) * f1;
				float a = (float)(c0.a) * f0 + (float)(c1.a) * f1;
				_out.r = (sl_uint8)(r);
				_out.g = (sl_uint8)(g);
				_out.b = (sl_uint8)(b);
				_out.a = (sl_uint8)(a);
			}

			SLIB_INLINE static void getColorAtY(Color& _out, const Color* colors, float fy, sl_reg stride, const Stretch_FilterParam& py)
			{
				const Color& c0 = *colors;
				const Color& c1 = colors[stride];
				float f0 = 1 - fy;
				float f1 = fy;
				float r = (float)(c0.r) * f0 + (float)(c1.r) * f1;
				float g = (float)(c0.g) * f0 + (float)(c1.g) * f1;
				float b = (float)(c0.b) * f0 + (float)(c1.b) * f1;
				float a = (float)(c0.a) * f0 + (float)(c1.a) * f1;
				_out.r = (sl_uint8)(r);
				_out.g = (sl_uint8)(g);
				_out.b = (sl_uint8)(b);
				_out.a = (sl_uint8)(a);
			}

		};

		class Stretch_Smooth_BoxFilter
		{
		public:
			SLIB_INLINE static void prepareFilter(float f, const Stretch_FilterParam& p, float& s, float& e, int& n, float& area)
			{
				if (p.flagBox) {
					s = 1 - f;
					e = f + p.filterSize; // p.filterSize > 1
					n = (int)(e - 0.005);
					e -= (float)n;
					n -= 1;
					area = p.filterSize;
				} else {
					s = 1 - f;
					e = f;
					n = 0;
					area = 1;
				}
			}

			SLIB_INLINE static void getColorAt(Color& _out, const Color* colors, float fx, float fy, sl_reg stride, const Stretch_FilterParam& px, const Stretch_FilterParam& py)
			{
				float sx, ex, ax; int nx;
				prepareFilter(fx, px, sx, ex, nx, ax);
				float sy, ey, ay; int ny;
				prepareFilter(fy, py, sy, ey, ny, ay);
				int ix, iy;
				float r = sx * (float)(colors[0].r) + ex * (float)(colors[nx + 1].r);
				float g = sx * (float)(colors[0].g) + ex * (float)(colors[nx + 1].g);
				float b = sx * (float)(colors[0].b) + ex * (float)(colors[nx + 1].b);
				float a = sx * (float)(colors[0].a) + ex * (float)(colors[nx + 1].a);
				for (ix = 0; ix < nx; ix++) {
					r += (float)(colors[1 + ix].r);
					g += (float)(colors[1 + ix].g);
					b += (float)(colors[1 + ix].b);
					a += (float)(colors[1 + ix].a);
				}
				r *= sy;
				g *= sy;
				b *= sy;
				a *= sy;
				colors += stride;
				for (iy = 0; iy < ny; iy++) {
					r += sx * (float)(colors[0].r) + ex * (float)(colors[nx + 1].r);
					g += sx * (float)(colors[0].g) + ex * (float)(colors[nx + 1].g);
					b += sx * (float)(colors[0].b) + ex * (float)(colors[nx + 1].b);
					a += sx * (float)(colors[0].a) + ex * (float)(colors[nx + 1].a);
					for (ix = 0; ix < nx; ix++) {
						r += (float)(colors[1 + ix].r);
						g += (float)(colors[1 + ix].g);
						b += (float)(colors[1 + ix].b);
						a += (float)(colors[1 + ix].a);
					}
					colors += stride;
				}
				r += ey * (sx * (float)(colors[0].r) + ex * (float)(colors[nx + 1].r));
				g += ey * (sx * (float)(colors[0].g) + ex * (float)(colors[nx + 1].g));
				b += ey * (sx * (float)(colors[0].b) + ex * (float)(colors[nx + 1].b));
				a += ey * (sx * (float)(colors[0].a) + ex * (float)(colors[nx + 1].a));
				for (ix = 0; ix < nx; ix++) {
					r += ey * (float)(colors[1 + ix].r);
					g += ey * (float)(colors[1 + ix].g);
					b += ey * (float)(colors[1 + ix].b);
					a += ey * (float)(colors[1 + ix].a);
				}
				float area = ax * ay;
				_out.r = (sl_uint8)(r / area);
				_out.g = (sl_uint8)(g / area);
				_out.b = (sl_uint8)(b / area);
				_out.a = (sl_uint8)(a / area);
			}

			SLIB_INLINE static void getColorAtX(Color& _out, const Color* colors, float fx, const Stretch_FilterParam& px)
			{
				float sx, ex, ax; int nx;
				prepareFilter(fx, px, sx, ex, nx, ax);
				int ix;
				float r = sx * (float)(colors[0].r) + ex * (float)(colors[nx + 1].r);
				float g = sx * (float)(colors[0].g) + ex * (float)(colors[nx + 1].g);
				float b = sx * (float)(colors[0].b) + ex * (float)(colors[nx + 1].b);
				float a = sx * (float)(colors[0].a) + ex * (float)(colors[nx + 1].a);
				for (ix = 0; ix < nx; ix++) {
					r += (float)(colors[1 + ix].r);
					g += (float)(colors[1 + ix].g);
					b += (float)(colors[1 + ix].b);
					a += (float)(colors[1 + ix].a);
				}
				_out.r = (sl_uint8)(r / ax);
				_out.g = (sl_uint8)(g / ax);
				_out.b = (sl_uint8)(b / ax);
				_out.a = (sl_uint8)(a / ax);
			}

			SLIB_INLINE static void getColorAtY(Color& _out, const Color* colors, float fy, sl_reg stride, const Stretch_FilterParam& py)
			{
				float sy, ey, ay; int ny;
				prepareFilter(fy, py, sy, ey, ny, ay);
				int iy;
				float r = sy * (float)(colors[0].r);
				float g = sy * (float)(colors[0].g);
				float b = sy * (float)(colors[0].b);
				float a = sy * (float)(colors[0].a);
				colors += stride;
				for (iy = 0; iy < ny; iy++) {
					r += (float)(colors[0].r);
					g += (float)(colors[0].g);
					b += (float)(colors[0].b);
					a += (float)(colors[0].a);
					colors += stride;
				}
				r += ey * (float)(colors[0].r);
				g += ey * (float)(colors[0].g);
				b += ey * (float)(colors[0].b);
				a += ey * (float)(colors[0].a);
				_out.r = (sl_uint8)(r / ay);
				_out.g = (sl_uint8)(g / ay);
				_out.b = (sl_uint8)(b / ay);
				_out.a = (sl_uint8)(a / ay);
			}

		};

		SLIB_INLINE static void Stretch_Smooth_Prepare(sl_int32 sw, sl_int32 dw, sl_int32& step_num, sl_int32& step_denom, sl_int32& dx_start, sl_int32& dx_end, Stretch_FilterParam& param)
		{
			if (sw >= dw) {
				step_num = sw;
				step_denom = dw;
				param.flagBox = sl_true;
				param.filterSize = (float)(sw) / (float)(dw);
				dx_start = 0;
				dx_end = dw;
			} else {
				step_num = sw - 1;
				step_denom = dw - 1;
				param.flagBox = sl_false;
				param.filterSize = 1;
				dx_start = 1;
				dx_end = dw - 1;
			}
		}

		template <class FILTER>
		class Stretch_Smooth
		{
		public:
			template <class BLEND>
			static void stretchX(const BLEND& blend, ImageDesc& dst, const ImageDesc& src)
			{
				sl_int32 sw = src.width;
				sl_int32 dw = dst.width;
				sl_int32 sx_step_num, sx_step_denom;
				sl_int32 isx, dx_start, dx_end;
				Stretch_FilterParam px;
				Stretch_Smooth_Prepare(sw, dw, sx_step_num, sx_step_denom, dx_start, dx_end, px);
				float f_sx_step_denom = (float)(sx_step_denom);

				sl_int32 dx, dy;
				sl_int32 dh = dst.height;
				float sx;

				Color* colorsDst;
				const Color* colorsSrc;
				Color color;

				if (dx_start) {
					// left
					colorsDst = dst.colors;
					colorsSrc = src.colors;
					for (dy = 0; dy < dh; dy++) {
						blend(*colorsDst, *colorsSrc);
						colorsDst += dst.stride;
						colorsSrc += src.stride;
					}
					// right
					colorsDst = dst.colors + (dw - 1);
					colorsSrc = src.colors + (sw - 1);
					for (dy = 0; dy < dh; dy++) {
						blend(*colorsDst, *colorsSrc);
						colorsDst += dst.stride;
						colorsSrc += src.stride;
					}
				}

				colorsDst = dst.colors;
				colorsSrc = src.colors;
				for (dy = 0; dy < dh; dy++) {
					for (dx = dx_start; dx < dx_end; dx++) {
						sx = (float)(dx * sx_step_num) / f_sx_step_denom;
						isx = (sl_int32)sx;
						FILTER::getColorAtX(color, colorsSrc + isx, sx - (float)isx, px);
						blend(colorsDst[dx], color);
					}
					colorsDst += dst.stride;
					colorsSrc += src.stride;
				}
			}

			template <class BLEND>
			static void stretchY(const BLEND& blend, ImageDesc& dst, const ImageDesc& src)
			{
				sl_int32 sh = src.height;
				sl_int32 dh = dst.height;
				sl_int32 sy_step_num, sy_step_denom;
				sl_int32 isy, dy_start, dy_end;
				Stretch_FilterParam py;
				Stretch_Smooth_Prepare(sh, dh, sy_step_num, sy_step_denom, dy_start, dy_end, py);
				float f_sy_step_denom = (float)(sy_step_denom);

				sl_int32 dx, dy;
				sl_int32 dw = dst.width;
				float sy;

				Color* colorsDst;
				const Color* colorsSrc;
				Color color;

				if (dy_start) {
					// top
					colorsDst = dst.colors;
					colorsSrc = src.colors;
					for (dx = 0; dx < dw; dx++) {
						blend(colorsDst[dx], colorsSrc[dx]);
					}
					// bottom
					colorsDst = dst.colors + (sl_uint32)(dh - 1) * dst.stride;
					colorsSrc = src.colors + (sl_uint32)(sh - 1) * src.stride;
					for (dx = 0; dx < dw; dx++) {
						blend(colorsDst[dx], colorsSrc[dx]);
					}
				}

				colorsDst = dst.colors + dy_start * dst.stride;
				colorsSrc = src.colors;
				for (dy = dy_start; dy < dy_end; dy++) {
					sy = (float)(dy * sy_step_num) / f_sy_step_denom;
					isy = (sl_int32)sy;
					float fsy = sy - (float)isy;
					const Color* lineSrc = colorsSrc + (sl_uint32)isy * src.stride;
					for (dx = 0; dx < dw; dx++) {
						FILTER::getColorAtY(color, lineSrc + dx, fsy, src.stride, py);
						blend(colorsDst[dx], color);
					}
					colorsDst += dst.stride;
				}
			}

			template <class BLEND>
			static void stretchOneRowSrc(const BLEND& blend, ImageDesc& dst, const ImageDesc& src)
			{
				sl_int32 sw = src.width;
				sl_int32 dw = dst.width;
				sl_int32 sx_step_num, sx_step_denom;
				sl_int32 isx, dx_start, dx_end;
				Stretch_FilterParam px;
				Stretch_Smooth_Prepare(sw, dw, sx_step_num, sx_step_denom, dx_start, dx_end, px);
				float f_sx_step_denom = (float)(sx_step_denom);

				sl_int32 dx, dy;
				sl_int32 dh = dst.height;
				float sx;

				Color* colorsDst;
				Color color;

				if (dx_start) {
					// left
					colorsDst = dst.colors;
					color = *(src.colors);
					for (dy = 0; dy < dh; dy++) {
						blend(*colorsDst, color);
						colorsDst += dst.stride;
					}
					// right
					colorsDst = dst.colors + (dw - 1);
					color = *(src.colors + (sw - 1));
					for (dy = 0; dy < dh; dy++) {
						blend(*colorsDst, color);
						colorsDst += dst.stride;
					}
				}

				for (dx = dx_start; dx < dx_end; dx++) {
					sx = (float)(dx * sx_step_num) / f_sx_step_denom;
					isx = (sl_int32)sx;
					FILTER::getColorAtX(color, src.colors + isx, sx - (float)isx, px);
					colorsDst = dst.colors + dx;
					for (dy = 0; dy < dh; dy++) {
						blend(*(colorsDst), color);
						colorsDst += dst.stride;
					}
				}
			}

			template <class BLEND>
			static void stretchOneColSrc(const BLEND& blend, ImageDesc& dst, const ImageDesc& src)
			{
				sl_int32 sh = src.height;
				sl_int32 dh = dst.height;
				sl_int32 sy_step_num, sy_step_denom;
				sl_int32 isy, dy_start, dy_end;
				Stretch_FilterParam py;
				Stretch_Smooth_Prepare(sh, dh, sy_step_num, sy_step_denom, dy_start, dy_end, py);
				float f_sy_step_denom = (float)sy_step_denom;
				sl_int32 dx, dy;
				sl_int32 dw = dst.width;
				float sy;

				Color* colorsDst;
				Color color;

				if (dy_start) {
					// top
					colorsDst = dst.colors;
					color = *(src.colors);
					for (dx = 0; dx < dw; dx++) {
						blend(colorsDst[dx], color);
					}
					// bottom
					colorsDst = dst.colors + (dh - 1) * dst.stride;
					color = *(src.colors + (sh - 1) * src.stride);
					for (dx = 0; dx < dw; dx++) {
						blend(colorsDst[dx], color);
					}
				}

				colorsDst = dst.colors + dy_start * dst.stride;
				for (dy = dy_start; dy < dy_end; dy++) {
					sy = (float)(dy * sy_step_num) / f_sy_step_denom;
					isy = (sl_int32)sy;
					float fsy = sy - (float)isy;
					FILTER::getColorAtY(color, src.colors + isy * src.stride, fsy, src.stride, py);
					for (dx = 0; dx < dw; dx++) {
						blend(colorsDst[dx], color);
					}
					colorsDst += dst.stride;
				}
			}

			template <class BLEND>
			static void stretch(const BLEND& blend, ImageDesc& dst, const ImageDesc& src)
			{
				if (src.width == dst.width) {
					stretchY(blend, dst, src);
					return;
				}
				if (src.height == dst.height) {
					stretchX(blend, dst, src);
					return;
				}
				if (src.width == 1) {
					stretchOneColSrc(blend, dst, src);
					return;
				}
				if (src.height == 1) {
					stretchOneRowSrc(blend, dst, src);
					return;
				}

				sl_int32 sw = src.width;
				sl_int32 dw = dst.width;
				sl_int32 sx_step_num, sx_step_denom;
				sl_int32 isx, dx_start, dx_end;
				Stretch_FilterParam px;
				Stretch_Smooth_Prepare(sw, dw, sx_step_num, sx_step_denom, dx_start, dx_end, px);
				float f_sx_step_denom = (float)sx_step_denom;

				sl_int32 sh = src.height;
				sl_int32 dh = dst.height;
				sl_int32 sy_step_num, sy_step_denom;
				sl_int32 isy, dy_start, dy_end;
				Stretch_FilterParam py;
				Stretch_Smooth_Prepare(sh, dh, sy_step_num, sy_step_denom, dy_start, dy_end, py);
				float f_sy_step_denom = (float)sy_step_denom;

				sl_int32 dx, dy;
				float sx, sy;

				Color color;
				Color* colorsDst;
				const Color* colorsSrc;

				// 4-corners
				if (dx_start && dy_start) {
					colorsDst = dst.colors;
					colorsSrc = src.colors;
					blend(colorsDst[0], colorsSrc[0]);
					blend(colorsDst[dw - 1], colorsSrc[sw - 1]);
					blend(colorsDst[(dh - 1) * dst.stride], colorsSrc[(sh - 1) * src.stride]);
					blend(colorsDst[(dh - 1) * dst.stride + dw - 1], colorsSrc[(sh - 1) * src.stride + sw - 1]);
				}

				if (dy_start) {
					// top
					colorsDst = dst.colors;
					colorsSrc = src.colors;
					for (dx = dx_start; dx < dx_end; dx++) {
						sx = (float)(dx * sx_step_num) / f_sx_step_denom;
						isx = (sl_int32)sx;
						FILTER::getColorAtX(color, colorsSrc + isx, sx - (float)isx, px);
						blend(colorsDst[dx], color);
					}
					// bottom
					colorsDst = dst.colors + (dh - 1) * dst.stride;
					colorsSrc = src.colors + (sh - 1) * src.stride;
					for (dx = dx_start; dx < dx_end; dx++) {
						sx = (float)(dx * sx_step_num) / f_sx_step_denom;
						isx = (sl_int32)sx;
						FILTER::getColorAtX(color, colorsSrc + isx, sx - (float)isx, px);
						blend(colorsDst[dx], color);
					}
				}
				if (dx_start) {
					// left
					colorsDst = dst.colors + dy_start * dst.stride;
					colorsSrc = src.colors;
					for (dy = dy_start; dy < dy_end; dy++) {
						sy = (float)(dy * sy_step_num) / f_sy_step_denom;
						isy = (sl_int32)sy;
						FILTER::getColorAtY(color, colorsSrc + isy * src.stride, sy - (float)isy, src.stride, py);
						blend(*colorsDst, color);
						colorsDst += dst.stride;
					}
					// right
					colorsDst = dst.colors + (dy_start * dst.stride + dw - 1);
					colorsSrc = src.colors + (sw - 1);
					for (dy = dy_start; dy < dy_end; dy++) {
						sy = (float)(dy * sy_step_num) / f_sy_step_denom;
						isy = (sl_int32)sy;
						FILTER::getColorAtY(color, colorsSrc + isy * src.stride, sy - (float)isy, src.stride, py);
						blend(*colorsDst, color);
						colorsDst += dst.stride;
					}
				}

				colorsDst = dst.colors + dy_start * dst.stride;
				colorsSrc = src.colors;
				for (dy = dy_start; dy < dy_end; dy++) {
					sy = (float)(dy * sy_step_num) / f_sy_step_denom;
					isy = (sl_int32)sy;
					float fsy = sy - (float)isy;
					const Color* lineSrc = colorsSrc + isy * src.stride;
					for (dx = dx_start; dx < dx_end; dx++) {
						sx = (float)(dx * sx_step_num) / f_sx_step_denom;
						isx = (sl_int32)sx;
						FILTER::getColorAt(color, lineSrc + isx, sx - (float)isx, fsy, src.stride, px, py);
						blend(colorsDst[dx], color);
					}
					colorsDst += dst.stride;
				}
			}
		};

		class Stretch_Smooth_IntBox
		{
		public:
			template <class BLEND>
			static void stretch(const BLEND& blend, ImageDesc& dst, const ImageDesc& src)
			{
				sl_uint32 dw = dst.width;
				sl_uint32 sw = src.width;
				sl_uint32 fx = sw / dw;
				if (!fx) {
					return;
				}
				sl_uint32 dx, sx;

				sl_uint32 dh = dst.height;
				sl_uint32 sh = src.height;
				sl_uint32 fy = sh / dh;
				if (!fy) {
					return;
				}
				sl_uint32 dy, sy;

				Color* colorsDst = dst.colors;
				const Color* colorsSrc = src.colors;
				sl_reg ly = fy * src.stride;

				sl_uint32 area = fx * fy;
				sl_uint32 n = Math::getMostSignificantBits(area) - 1;
				if (area == (1U << n)) {
					for (dy = 0; dy < dst.height; dy++) {
						const Color* cs = colorsSrc;
						for (dx = 0; dx < dst.width; dx++) {
							sl_uint32 r = 0;
							sl_uint32 g = 0;
							sl_uint32 b = 0;
							sl_uint32 a = 0;
							const Color* c = cs;
							for (sy = 0; sy < fy; sy++) {
								for (sx = 0; sx < fx; sx++) {
									r += c[sx].r;
									g += c[sx].g;
									b += c[sx].b;
									a += c[sx].a;
								}
								c += src.stride;
							}
							cs += fx;
							blend(colorsDst[dx], Color((sl_uint8)(r >> n), (sl_uint8)(g >> n), (sl_uint8)(b >> n), (sl_uint8)(a >> n)));
						}
						colorsDst += dst.stride;
						colorsSrc += ly;
					}
				} else {
					for (dy = 0; dy < dst.height; dy++) {
						const Color* cs = colorsSrc;
						for (dx = 0; dx < dst.width; dx++) {
							sl_uint32 r = 0;
							sl_uint32 g = 0;
							sl_uint32 b = 0;
							sl_uint32 a = 0;
							const Color* c = cs;
							for (sy = 0; sy < fy; sy++) {
								for (sx = 0; sx < fx; sx++) {
									r += c[sx].r;
									g += c[sx].g;
									b += c[sx].b;
									a += c[sx].a;
								}
								c += src.stride;
							}
							cs += fx;
							blend(colorsDst[dx], Color((sl_uint8)(r / area), (sl_uint8)(g / area), (sl_uint8)(b / area), (sl_uint8)(a / area)));
						}
						colorsDst += dst.stride;
						colorsSrc += ly;
					}
				}
			}

		};

		class Stretch
		{
		public:
			template <class STRETCH_OP, class COLOR_OP>
			static void stretch(ImageDesc& dst, const ImageDesc& src, const COLOR_OP& src_op, BlendMode blend)
			{
				switch (blend) {
					case BlendMode::Copy:
						STRETCH_OP::stretch(Blend_Copy<COLOR_OP>(src_op), dst, src);
						break;
					case BlendMode::Over:
						STRETCH_OP::stretch(Blend_Over<COLOR_OP>(src_op), dst, src);
						break;
				}
			}
		};

		template <class COLOR_OP>
		static void Draw(ImageDesc& dst, const ImageDesc& src, const COLOR_OP& src_op, BlendMode blend, StretchMode stretch)
		{
			if (!(src.width) || !(src.height) || !(src.stride) || !(src.colors)) {
				return;
			}
			if (!(dst.width) || !(dst.height) || !(dst.stride) || !(dst.colors)) {
				return;
			}
			if (src.width == dst.width && src.height == dst.height) {
				Stretch::template stretch<Stretch_Copy>(dst, src, src_op, blend);
				return;
			}
			if (src.width == 1 && src.height == 1) {
				Stretch::template stretch<Stretch_FillColor>(dst, src, src_op, blend);
				return;
			}
			if (stretch == StretchMode::Nearest) {
				Stretch::template stretch<Stretch_Nearest>(dst, src, src_op, blend);
			} else if (stretch == StretchMode::Linear) {
				Stretch::template stretch< Stretch_Smooth<Stretch_Smooth_LinearFilter> >(dst, src, src_op, blend);
			} else {
				if (src.width <= dst.width && src.height <= dst.height) {
					Stretch::template stretch< Stretch_Smooth<Stretch_Smooth_LinearFilter> >(dst, src, src_op, blend);
					return;
				}
				if (!(src.width % dst.width) && !(src.height % dst.height)) {
					Stretch::template stretch<Stretch_Smooth_IntBox>(dst, src, src_op, blend);
					return;
				}
				Stretch::template stretch< Stretch_Smooth<Stretch_Smooth_BoxFilter> >(dst, src, src_op, blend);
			}
		}
	}

	void Image::draw(ImageDesc& dst, const ImageDesc& src, BlendMode blend, StretchMode stretch)
	{
		Draw(dst, src, ColorOp_None(), blend, stretch);
	}

	namespace {

		template <class COLOR_OP>
		static void DrawImage(
			ImageDesc& dst, sl_int32 dx, sl_int32 dy, sl_int32 dw, sl_int32 dh,
			ImageDesc& src, const COLOR_OP& src_op, sl_int32 sx, sl_int32 sy, sl_int32 sw, sl_int32 sh,
			BlendMode blend, StretchMode stretch)
		{
			if (dw <= 0 || dh <= 0) {
				return;
			}
			if (sw <= 0 || sh <= 0) {
				return;
			}
			sl_int32 dwx = dst.width;
			sl_int32 dhx = dst.height;
			sl_int32 swx = src.width;
			sl_int32 shx = src.height;
			if (dwx <= 0 || dhx <= 0) {
				return;
			}
			if (swx <= 0 || shx <= 0) {
				return;
			}

			sl_int32 dx2 = dx + dw;
			sl_int32 dy2 = dy + dh;
			sl_int32 sx2 = sx + sw;
			sl_int32 sy2 = sy + sh;

			if (dx < 0) {
				sx -= dx * sw / dw;
				dx = 0;
			}
			if (dy < 0) {
				sy -= dy * sh / dh;
				dy = 0;
			}
			if (dx2 > dwx) {
				sx2 -= (dwx - dx2) * sw / dw;
				dx2 = dwx;
			}
			if (dy2 > dhx) {
				sy2 -= (dhx - dy2) * sh / dh;
				dy2 = dhx;
			}

			if (sx < 0) {
				dx -= sx * dw / sw;
				sx = 0;
			}
			if (sy < 0) {
				dy -= sy * dh / sh;
				sy = 0;
			}
			if (sx2 > swx) {
				dx2 -= (swx - sx2) * dw / sw;
				sx2 = swx;
			}
			if (sy2 > shx) {
				dy2 -= (shx - sy2) * dh / sh;
				sy2 = shx;
			}
			if (sx2 <= sx) {
				return;
			}
			if (sy2 <= sy) {
				return;
			}

			if (dx < 0) {
				dx = 0;
			}
			if (dy < 0) {
				dy = 0;
			}
			if (dx2 > dwx) {
				dx2 = dwx;
			}
			if (dy2 > dhx) {
				dy2 = dhx;
			}
			if (dx2 <= dx) {
				return;
			}
			if (dy2 <= dy) {
				return;
			}

			ImageDesc descDst, descSrc;
			descDst.colors = dst.colors + (dy * dst.stride + dx);
			descDst.width = dx2 - dx;
			descDst.height = dy2 - dy;
			descDst.stride = dst.stride;
			descSrc.colors = src.colors + (sy * src.stride + sx);
			descSrc.width = sx2 - sx;
			descSrc.height = sy2 - sy;
			descSrc.stride = src.stride;
			Draw(descDst, descSrc, src_op, blend, stretch);
		}

		static void DrawImage(
			ImageDesc& dst, sl_int32 dx, sl_int32 dy, sl_int32 dw, sl_int32 dh,
			ImageDesc& src, sl_int32 sx, sl_int32 sy, sl_int32 sw, sl_int32 sh,
			BlendMode blend, StretchMode stretch)
		{
			DrawImage(dst, dx, dy, dw, dh,
				src, ColorOp_None(), sx, sy, sw, sh,
				blend, stretch);
		}

		static void DrawImage(
			ImageDesc& dst, sl_int32 dx, sl_int32 dy, sl_int32 dw, sl_int32 dh,
			ImageDesc& src, const Color4f& srcMul, const Color4f& srcAdd,
			sl_int32 sx, sl_int32 sy, sl_int32 sw, sl_int32 sh,
			BlendMode blend, StretchMode stretch)
		{
			DrawImage(dst, dx, dy, dw, dh,
				src, ColorOp_MulAddColor(srcMul, srcAdd), sx, sy, sw, sh,
				blend, stretch);
		}

		static void DrawImage(
			ImageDesc& dst, sl_int32 dx, sl_int32 dy, sl_int32 dw, sl_int32 dh,
			ImageDesc& src, const ColorMatrix& cm,
			sl_int32 sx, sl_int32 sy, sl_int32 sw, sl_int32 sh,
			BlendMode blend, StretchMode stretch)
		{
			DrawImage(dst, dx, dy, dw, dh,
				src, ColorOp_ColorMatrix(&cm), sx, sy, sw, sh,
				blend, stretch);
		}

	}

	void Image::drawImage(sl_int32 dx, sl_int32 dy, sl_int32 dw, sl_int32 dh,
		const Ref<Image>& src, sl_int32 sx, sl_int32 sy, sl_int32 sw, sl_int32 sh,
		BlendMode blend, StretchMode stretch)
	{
		if (src.isNull()) {
			return;
		}
		ImageDesc srcDesc;
		src->getDesc(srcDesc);
		DrawImage(m_desc, dx, dy, dw, dh, srcDesc, sx, sy, sw, sh, blend, stretch);
	}

	void Image::drawImage(sl_int32 dx, sl_int32 dy, sl_int32 dw, sl_int32 dh,
		const Ref<Image>& src, BlendMode blend, StretchMode stretch)
	{
		if (src.isNull()) {
			return;
		}
		drawImage(dx, dy, dw, dh, src, 0, 0, src->getWidth(), src->getHeight(), blend, stretch);
	}

	void Image::drawImage(sl_int32 dx, sl_int32 dy,
		const Ref<Image>& src, BlendMode blend, StretchMode stretch)
	{
		if (src.isNull()) {
			return;
		}
		sl_uint32 w = src->getWidth();
		sl_uint32 h = src->getHeight();
		drawImage(dx, dy, w, h, src, 0, 0, w, h, blend, stretch);
	}

	void Image::drawImage(const Rectanglei& rectDst,
		const Ref<Image>& src, const Rectanglei& rectSrc,
		BlendMode blend, StretchMode stretch)
	{
		drawImage(rectDst.left, rectDst.top, rectDst.getWidth(), rectDst.getHeight(),
			src, rectSrc.left, rectSrc.top, rectSrc.getWidth(), rectSrc.getHeight(),
			blend, stretch);
	}

	void Image::drawImage(const Rectanglei& rectDst,
		const Ref<Image>& src, BlendMode blend, StretchMode stretch)
	{
		if (src.isNull()) {
			return;
		}
		drawImage(rectDst.left, rectDst.top, rectDst.getWidth(), rectDst.getHeight(),
			src, 0, 0, src->getWidth(), src->getHeight(),
			blend, stretch);
	}

	void Image::drawImage(sl_int32 dx, sl_int32 dy, sl_int32 dw, sl_int32 dh,
		const Ref<Image>& src, const Color4f& srcMul, const Color4f& srcAdd,
		sl_int32 sx, sl_int32 sy, sl_int32 sw, sl_int32 sh,
		BlendMode blend, StretchMode stretch)
	{
		if (src.isNull()) {
			return;
		}
		ImageDesc srcDesc;
		src->getDesc(srcDesc);
		DrawImage(m_desc, dx, dy, dw, dh, srcDesc, srcMul, srcAdd, sx, sy, sw, sh, blend, stretch);
	}

	void Image::drawImage(sl_int32 dx, sl_int32 dy, sl_int32 dw, sl_int32 dh,
		const Ref<Image>& src, const ColorMatrix& cm,
		sl_int32 sx, sl_int32 sy, sl_int32 sw, sl_int32 sh,
		BlendMode blend, StretchMode stretch)
	{
		if (src.isNull()) {
			return;
		}
		ImageDesc srcDesc;
		src->getDesc(srcDesc);
		DrawImage(m_desc, dx, dy, dw, dh, srcDesc, cm, sx, sy, sw, sh, blend, stretch);
	}

	void Image::copyBitmap(const Ref<Bitmap>& bitmap, sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height)
	{
		if (bitmap.isNull()) {
			return;
		}
		if (width <= 0 || height <= 0) {
			return;
		}
		sl_uint32 bw = bitmap->getWidth();
		sl_uint32 bh = bitmap->getHeight();
		if (x >= bw) {
			return;
		}
		if (y >= bh) {
			return;
		}
		if (width > bw - x) {
			width = bw - x;
		}
		if (height > bh - y) {
			height = bh - y;
		}
		sl_uint32 iw = m_desc.width;
		sl_uint32 ih = m_desc.height;
		if (x >= iw) {
			return;
		}
		if (y >= ih) {
			return;
		}
		if (width > iw - x) {
			width = iw - x;
		}
		if (height > ih - y) {
			height = ih - y;
		}
		bitmap->readPixels(x, y, width, height, m_desc.colors, m_desc.stride);
	}

	Ref<Drawable> Image::subDrawable(sl_real _x, sl_real _y, sl_real _width, sl_real _height)
	{
		sl_int32 x = (sl_int32)_x;
		sl_int32 y = (sl_int32)_y;
		sl_int32 width = (sl_int32)_width;
		sl_int32 height = (sl_int32)_height;
		if (x >= 0 && y >= 0 && width > 0 && height > 0) {
			return sub(x, y, width, height);
		}
		return sl_null;
	}

	Ref<Image> Image::sub(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height) const
	{
		if (!x && !y && width == m_desc.width && height == m_desc.height) {
			return (Image*)this;
		}
		if (width <= 0 || height <= 0) {
			return sl_null;
		}
		if (x > m_desc.width || y > m_desc.height) {
			return sl_null;
		}
		if (width > m_desc.width - x || height > m_desc.height - y) {
			return sl_null;
		}
		Ref<Image> ret = new Image;
		if (ret.isNotNull()) {
			ret->m_desc.colors = getColorsAt(x, y);
			ret->m_desc.width = width;
			ret->m_desc.height = height;
			ret->m_desc.stride = m_desc.stride;
			ret->m_desc.ref = m_desc.ref;
			return ret;
		}
		return sl_null;
	}

	Ref<Image> Image::stretch(sl_uint32 width, sl_uint32 height, StretchMode stretchMode) const
	{
		if (width == m_desc.width && height == m_desc.height) {
			return (Image*)this;
		}
		return duplicate(width, height, stretchMode);
	}

	Ref<Image> Image::stretchToSmall(sl_uint32 requiredWidth, sl_uint32 requiredHeight, sl_bool flagKeepAspectRatio, StretchMode stretchMode) const
	{
		if (requiredWidth <= 0 || requiredHeight <= 0) {
			return sl_null;
		}
		if (requiredWidth >= m_desc.width && requiredHeight >= m_desc.height) {
			return (Image*)this;
		}
		sl_uint32 width, height;
		if (flagKeepAspectRatio) {
			float fw = (float)requiredWidth / (float)(m_desc.width);
			float fh = (float)requiredHeight / (float)(m_desc.height);
			float f = SLIB_MIN(fw, fh);
			width = (sl_uint32)((float)(m_desc.width) * f);
			height = (sl_uint32)((float)(m_desc.height) * f);
		} else {
			width = SLIB_MIN(requiredWidth, m_desc.width);
			height = SLIB_MIN(requiredHeight, m_desc.height);
		}
		return stretch(width, height, stretchMode);
	}

	Ref<Image> Image::stretchToSmall(sl_uint32 sampleSize) const
	{
		if (!sampleSize) {
			return sl_null;
		}
		sl_uint32 width = m_desc.width / sampleSize;
		sl_uint32 height = m_desc.height / sampleSize;
		if (!width || !height) {
			return sl_null;
		}
		Memory temp = Memory::create(width<<4);
		if (temp.isNull()) {
			return sl_null;
		}
		Ref<Image> ret = Image::allocate(width, height);
		if (ret.isNull()) {
			return sl_null;
		}
		Color* row_d = ret->m_desc.colors;
		sl_reg pitch_d = ret->m_desc.stride;
		Color* row_s = m_desc.colors;
		sl_reg pitch_s = m_desc.stride;
		sl_uint32* t = (sl_uint32*)(temp.getData());
		sl_uint32 i, j, row, col;
		sl_uint32 n = sampleSize * sampleSize;
		sl_uint32 nShift;
		if (!(n & (n - 1))) {
			nShift = Math::getMostSignificantBits(n) - 1;
		} else {
			nShift = 0;
		}
		for (row = 0; row < height; row++) {
			Base::zeroMemory(t, width << 4);
			for (i = 0; i < sampleSize; i++) {
				sl_uint32* c_d = t;
				Color* c_s = row_s;
				for (col = 0; col < width; col++) {
					for (j = 0; j < sampleSize; j++) {
						*c_d += c_s->r;
						c_d[1] += c_s->g;
						c_d[2] += c_s->b;
						c_d[3] += c_s->a;
						c_s++;
					}
					c_d += 4;
				}
				row_s += pitch_s;
			}
			if (nShift) {
				for (col = 0; col < width; col++) {
					Color& c = row_d[col];
					i = col<<2;
					c.r = t[i] >> nShift;
					c.g = t[i+1] >> nShift;
					c.b = t[i+2] >> nShift;
					c.a = t[i+3] >> nShift;
				}
			} else {
				for (col = 0; col < width; col++) {
					Color& c = row_d[col];
					i = col<<2;
					c.r = t[i] / n;
					c.g = t[i+1] / n;
					c.b = t[i+2] / n;
					c.a = t[i+3] / n;
				}
			}
			row_d += pitch_d;
		}
		return ret;
	}

	Ref<Image> Image::rotateImage(RotationMode rotate, FlipMode flip) const
	{
		NormalizeRotateAndFlip(rotate, flip);
		if (rotate == RotationMode::Rotate0 && flip == FlipMode::None) {
			return (Image*)this;
		}
		return create(m_desc, rotate, flip);
	}

	Ref<Image> Image::flipImage(FlipMode flip) const
	{
		if (flip == FlipMode::None) {
			return (Image*)this;
		}
		return create(m_desc, RotationMode::Rotate0, flip);
	}

	Ref<Image> Image::duplicate() const
	{
		return create(m_desc);
	}

	Ref<Image> Image::duplicate(RotationMode rotate, FlipMode flip) const
	{
		return create(m_desc, rotate, flip);
	}

	Ref<Image> Image::duplicate(FlipMode flip) const
	{
		return create(m_desc, RotationMode::Rotate0, flip);
	}

	Ref<Image> Image::duplicate(sl_uint32 width, sl_uint32 height, StretchMode stretch) const
	{
		if (width > 0 && height > 0) {
			Ref<Image> ret = Image::allocate(width, height);
			if (ret.isNotNull()) {
				draw(ret->m_desc, m_desc, BlendMode::Copy, stretch);
				return ret;
			}
		}
		return sl_null;
	}

	ImageFileType Image::getFileType(const void* _mem, sl_size size)
	{
		sl_uint8* mem = (sl_uint8*)_mem;
		if (size > 4 && mem[0] == 0xFF && mem[1] == 0xD8) {
			return ImageFileType::JPEG;
		}
		if (size > 0x08 && mem[0] == 0x89 && mem[1] == 0x50 && mem[2] == 0x4E && mem[3] == 0x47 && mem[4] == 0x0D && mem[5] == 0x0A && mem[6] == 0x1A && mem[7] == 0x0A) {
			return ImageFileType::PNG;
		}
		if (size > 12 && mem[0] == 'B' && mem[1] == 'M') {
			return ImageFileType::BMP;
		}
		if (size > 4 && mem[0] == 'G' && mem[1] == 'I' && mem[2] == 'F' && mem[3] == '8') {
			return ImageFileType::GIF;
		}
		if (size > 4 && mem[0] == 'D' && mem[1] == 'D' && mem[2] == 'S' && mem[3] == ' ') {
			return ImageFileType::DDS;
		}
		return ImageFileType::Unknown;
	}

	ImageFileType Image::getFileType(const MemoryView& mem)
	{
		return getFileType(mem.data, mem.size);
	}

	Ref<Image> Image::loadFromMemory(ImageFileType type, const void* mem, sl_size size)
	{
		switch (type) {
			case ImageFileType::JPEG:
				return loadJpeg(mem, size);
			case ImageFileType::PNG:
				return loadPng(mem, size);
			default:
				return loadStb(mem, size);
		}
	}

	Ref<Image> Image::loadFromMemory(const void* mem, sl_size size)
	{
		return loadFromMemory(getFileType(mem, size), mem, size);
	}

	Ref<Image> Image::loadFromMemory(const MemoryView& mem)
	{
		return loadFromMemory(mem.data, mem.size);
	}

	Ref<Image> Image::loadFromFile(const StringParam& filePath)
	{
		Memory mem = File::readAllBytes(filePath);
		if (mem.isNotNull()) {
			return loadFromMemory(mem);
		}
		return sl_null;
	}

	Ref<AnimationDrawable> Image::loadAnimationFromMemory(ImageFileType type, const void* mem, sl_size size)
	{
		if (type == ImageFileType::GIF) {
			return loadStbGif(mem, size);
		}
		return sl_null;
	}

	Ref<AnimationDrawable> Image::loadAnimationFromMemory(const void* mem, sl_size size)
	{
		return loadAnimationFromMemory(getFileType(mem, size), mem, size);
	}

	Ref<AnimationDrawable> Image::loadAnimationFromMemory(const MemoryView& mem)
	{
		return loadAnimationFromMemory(mem.data, mem.size);
	}

	Ref<AnimationDrawable> Image::loadAnimationFromFile(const StringParam& filePath)
	{
		Memory mem = File::readAllBytes(filePath);
		if (mem.isNotNull()) {
			return loadAnimationFromMemory(mem);
		}
		return sl_null;
	}

	Ref<AnimationDrawable> Image::loadAnimationFromAsset(const StringParam& path)
	{
		Memory mem = Assets::readAllBytes(path);
		if (mem.isNotNull()) {
			return loadAnimationFromMemory(mem);
		}
		return sl_null;
	}

	Ref<Drawable> Image::getDrawableCache(Canvas* canvas)
	{
		Ref<Drawable> drawableCached = m_drawableCached;
		if (drawableCached.isNotNull()) {
			if (canvas->isSupportedDrawable(drawableCached)) {
				if (m_flagInvalidatedCache) {
					if (canvas->updateDrawableCacheForImage(drawableCached.get(), this)) {
						return drawableCached;
					}
					m_flagInvalidatedCache = sl_false;
				} else {
					return drawableCached;
				}
			}
			m_drawableCached.setNull();
		}
		drawableCached = canvas->createDrawableCacheForImage(this);
		if (drawableCached.isNotNull()) {
			m_flagInvalidatedCache = sl_false;
			m_drawableCached = drawableCached;
		}
		return drawableCached;
	}

	void Image::onDraw(Canvas* canvas, const Rectangle& rectDst, const Rectangle& rectSrc, const DrawParam& param)
	{
		Ref<Drawable> drawableCached = getDrawableCache(canvas);
		if (drawableCached.isNotNull()) {
			drawableCached->onDraw(canvas, rectDst, rectSrc, param);
		}
	}

	void Image::onDrawAll(Canvas* canvas, const Rectangle& rectDst, const DrawParam& param)
	{
		Ref<Drawable> drawableCached = getDrawableCache(canvas);
		if (drawableCached.isNotNull()) {
			drawableCached->onDrawAll(canvas, rectDst, param);
		}
	}

	namespace {

		template <class BLEND>
		SLIB_INLINE static void SetPixel(const BLEND& blend, ImageDesc& dst, sl_int32 x, sl_int32 y, const Color& color)
		{
			if (x < 0) {
				return;
			}
			if (x >= (sl_int32)(dst.width)) {
				return;
			}
			if (y < 0) {
				return;
			}
			if (y >= (sl_int32)(dst.height)) {
				return;
			}
			blend(dst.colors[dst.stride * y + x], color);
		}

		// https://github.com/libgd/libgd/blob/master/src/gd.c
		template <class T>
		static sl_bool ClipLineHelper(T& x0, T& y0, T& x1, T& y1, T mindim, T maxdim)
		{
			if (x0 < mindim) {
				if (x1 < mindim) {
					return sl_false;
				}
				double gradient = (double)(y1 - y0) / (double)(x1 - x0);
				y0 -= (T)(gradient * (x0 - mindim));
				x0 = mindim;
				if (x1 > maxdim) {
					y1 += (T)(gradient * (maxdim - x1));
					x1 = maxdim;
				}
				return sl_true;
			}
			if (x0 > maxdim) {
				if (x1 > maxdim) {
					return sl_false;
				}
				double gradient = (double)(y1 - y0) / (double)(x1 - x0);
				y0 += (T)(gradient * (maxdim - x0));
				x0 = maxdim;
				if (x1 < mindim) {
					y1 -= (T)(gradient * (x1 - mindim));
					x1 = mindim;
				}
				return sl_true;
			}
			if (x1 > maxdim) {
				double gradient = (double)(y1 - y0) / (double)(x1 - x0);
				y1 += (T)(gradient * (maxdim - x1));
				x1 = maxdim;
				return sl_true;
			}
			if (x1 < mindim) {
				double gradient = (y1 - y0) / (double) (x1 - x0);
				y1 -= (T)(gradient * (x1 - mindim));
				x1 = mindim;
				return sl_true;
			}
			return sl_true;
		}

		template <class T>
		static sl_bool ClipLine(T& x1, T& y1, T& x2, T& y2, T left, T top, T right, T bottom)
		{
			if (right < left) {
				return sl_false;
			}
			if (bottom < top) {
				return sl_false;
			}
			if (x1 == x2) {
				if (x1 < left || x1 > right) {
					return sl_false;
				}
				if (y1 < y2) {
					if (y1 < top) {
						y1 = top;
					}
					if (y2 > bottom) {
						y2 = bottom;
					}
					if (y1 > y2) {
						return sl_false;
					}
				} else {
					if (y2 < top) {
						y2 = top;
					}
					if (y1 > bottom) {
						y1 = bottom;
					}
					if (y2 > y1) {
						return sl_false;
					}
				}
				return sl_true;
			}
			if (y1 == y2) {
				if (y1 < top || y1 > bottom) {
					return sl_false;
				}
				if (x1 < x2) {
					if (x1 < left) {
						x1 = left;
					}
					if (x2 > right) {
						x2 = right;
					}
					if (x1 > x2) {
						return sl_false;
					}
				} else {
					if (x2 < left) {
						x2 = left;
					}
					if (x1 > right) {
						x1 = right;
					}
					if (x2 > x1) {
						return sl_false;
					}
				}
				return sl_true;
			}
			if (!(ClipLineHelper(x1, y1, x2, y2, left, right))) {
				return sl_false;
			}
			if (!(ClipLineHelper(y1, x1, y2, x2, top, bottom))) {
				return sl_false;
			}
			return sl_true;
		}

		template <class BLEND>
		static void DrawHorizontalLine(const BLEND& blend, ImageDesc& dst, sl_int32 x1, sl_int32 x2, sl_int32 y, const Color& color)
		{
			sl_int32 w = (sl_int32)(dst.width);
			sl_int32 h = (sl_int32)(dst.height);
			if (y < 0 || y >= h) {
				return;
			}
			if (x1 > x2) {
				sl_int32 t = x2;
				x2 = x1;
				x1 = t;
			}
			if (x1 < 0) {
				x1 = 0;
			}
			if (x2 >= w) {
				x2 = w - 1;
			}
			if (x1 > x2) {
				return;
			}
			sl_uint32 dx = x2 - x1;
			Color* c = dst.colors;
			sl_reg stride = dst.stride;
			Color* d = c + (y * stride + x1);
			for (sl_uint32 i = 0; i <= dx; i++) {
				blend(*d, color);
				d++;
			}
		}

		template <class BLEND>
		static void DrawVerticalLine(const BLEND& blend, ImageDesc& dst, sl_int32 x, sl_int32 y1, sl_int32 y2, const Color& color)
		{
			sl_int32 w = (sl_int32)(dst.width);
			sl_int32 h = (sl_int32)(dst.height);
			if (x < 0 || x >= w) {
				return;
			}
			if (y1 > y2) {
				sl_int32 t = y2;
				y2 = y1;
				y1 = t;
			}
			if (y1 < 0) {
				y1 = 0;
			}
			if (y2 >= h) {
				y2 = h - 1;
			}
			if (y1 > y2) {
				return;
			}
			sl_uint32 dy = y2 - y1;
			Color* c = dst.colors;
			sl_reg stride = dst.stride;
			Color* d = c + (y1 * stride + x);
			for (sl_uint32 i = 0; i <= dy; i++) {
				blend(*d, color);
				d += stride;
			}
		}

		// https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
		template <class BLEND>
		static void DrawLine(const BLEND& blend, ImageDesc& dst, sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, sl_bool flagAntialias)
		{
			if (y1 == y2) {
				DrawHorizontalLine(blend, dst, x1, x2, y1, color);
				return;
			}
			if (x1 == x2) {
				DrawVerticalLine(blend, dst, x1, y1, y2, color);
				return;
			}
			sl_int32 w = (sl_int32)(dst.width);
			sl_int32 h = (sl_int32)(dst.height);
			if (!w) {
				return;
			}
			if (!h) {
				return;
			}
			ClipLine(x1, y1, x2, y2, 0, 0, w - 1, h - 1);
			if (y1 == y2) {
				DrawHorizontalLine(blend, dst, x1, x2, y1, color);
				return;
			}
			if (x1 == x2) {
				DrawVerticalLine(blend, dst, x1, y1, y2, color);
				return;
			}
			sl_int32 dx = x2 - x1;
			sl_int32 dy = y2 - y1;
			Color* c = dst.colors;
			sl_reg stride = dst.stride;
			if (Math::abs(dx) > Math::abs(dy)) {
				if (dx < 0) {
					sl_int32 t = x2;
					x2 = x1;
					x1 = t;
					t = y2;
					y2 = y1;
					y1 = t;
					dx = -dx;
					dy = -dy;
				}
				c += x1 + y1 * stride;
				sl_reg inc_stride;
				if (dy < 0) {
					inc_stride = -stride;
					dy = -dy;
				} else {
					inc_stride = stride;
				}
				if (flagAntialias) {
					float dxf = (float)dx;
					Color src = color;
					for (sl_int32 ix = 0; ix <= dx; ix++) {
						sl_int32 t = ix * dy;
						sl_int32 iy = t / dx;
						sl_int32 my = t % dx;
						if (my) {
							Color* cn = c + inc_stride * iy;
							float f = (float)(my) / dxf;
							src.a = (sl_uint8)(Math::clamp0_255((sl_int32)(color.a * f)));
							blend(*(cn + inc_stride), src);
							if (color.a > src.a) {
								src.a = color.a - src.a;
								blend(*cn, src);
							}
						} else {
							blend(*(c + inc_stride * iy), color);
						}
						c++;
					}
				} else {
					sl_int32 D = 2 * dy - dx;
					for (sl_int32 ix = 0; ix <= dx; ix++) {
						blend(*c, color);
						if (D > 0) {
							c += inc_stride;
							D = D - 2 * dx;
						}
						D = D + 2 * dy;
						c++;
					}
				}
			} else {
				if (dy < 0) {
					sl_int32 t = x2;
					x2 = x1;
					x1 = t;
					t = y2;
					y2 = y1;
					y1 = t;
					dx = -dx;
					dy = -dy;
				}
				c += x1 + y1 * stride;
				sl_int32 inc_x;
				if (dx < 0) {
					inc_x = -1;
					dx = -dx;
				} else {
					inc_x = 1;
				}
				if (flagAntialias) {
					float dyf = (float)dy;
					Color src = color;
					for (sl_int32 iy = 0; iy <= dy; iy++) {
						sl_int32 t = iy * dx;
						sl_int32 ix = t / dy;
						sl_int32 mx = t % dy;
						if (mx) {
							Color* cn = c + inc_x * ix;
							float f = (float)(mx) / dyf;
							src.a = (sl_uint8)(Math::clamp0_255((sl_int32)(color.a * f)));
							blend(*(cn + inc_x), src);
							if (color.a > src.a) {
								src.a = color.a - src.a;
								blend(*cn, src);
							}
						} else {
							blend(*(c + inc_x * ix), color);
						}
						c += stride;
					}
				} else {
					sl_int32 D = 2 * dx - dy;
					for (sl_int32 iy = 0; iy <= dy; iy++) {
						blend(*c, color);
						if (D > 0) {
							c += inc_x;
							D = D - 2 * dy;
						}
						D = D + 2 * dx;
						c += stride;
					}
				}
			}
		}

		static void DrawLine(ImageDesc& dst, sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend, sl_bool flagAntialias)
		{
			switch (blend) {
				case BlendMode::Copy:
					DrawLine(Blend_Copy<ColorOp_None>(ColorOp_None()), dst, x1, y1, x2, y2, color, flagAntialias);
					break;
				case BlendMode::Over:
					DrawLine(Blend_Over<ColorOp_None>(ColorOp_None()), dst, x1, y1, x2, y2, color, flagAntialias);
					break;
			}
		}

		SLIB_INLINE static sl_real GetFloatPart(sl_real f)
		{
			return f - Math::floor(f);
		}

		SLIB_INLINE static sl_real GetReverseFloatPart(sl_real f)
		{
			return 1 - GetFloatPart(f);
		}

		// Xiaolin Wu's line algorithm
		//  http://www.ece.mcmaster.ca/~xwu/
		template <class BLEND>
		static void DrawLineF(const BLEND& blend, ImageDesc& dst, sl_real x0, sl_real y0, sl_real x1, sl_real y1, const Color& color)
		{
			sl_int32 w = (sl_int32)(dst.width);
			sl_int32 h = (sl_int32)(dst.height);
			if (!w) {
				return;
			}
			if (!h) {
				return;
			}
			ClipLine(x0, y0, x1, y1, (sl_real)0, (sl_real)0, (sl_real)(w - 1), (sl_real)(h - 1));

			sl_bool bSteep = Math::abs(y1 - y0) > Math::abs(x1 - x0);
			if (bSteep) {
				Swap(x0, y0);
				Swap(x1, y1);
			}
			if (x0 > x1) {
				Swap(x0, x1);
				Swap(y0, y1);
			}
			sl_real dx = x1 - x0;
			sl_real dy = y1 - y0;
			sl_real gradient;
			if (Math::isAlmostZero(dx)) {
				gradient = 1;
			} else {
				gradient = dy / dx;
			}
			Color src = color;
			sl_int32 ix1, iy1, ix2, iy2;
			sl_real yInter;
			// handle first endpoint
			{
				sl_real xEnd = Math::round(x0);
				sl_real yEnd = y0 + gradient * (xEnd - x0);
				ix1 = (sl_int32)xEnd;
				iy1 = (sl_int32)(Math::floor(yEnd));
				sl_real a = (sl_real)(color.a * GetReverseFloatPart(x0 + 0.5f));
				sl_real f = GetFloatPart(yEnd);
				if (bSteep) {
					src.a = (sl_uint8)(Math::clamp0_255((sl_int32)(a * (1 - f))));
					SetPixel(blend, dst, iy1, ix1, src);
					src.a = (sl_uint8)(Math::clamp0_255((sl_int32)(a * f)));
					SetPixel(blend, dst, iy1 + 1, ix1, src);
				} else {
					src.a = (sl_uint8)(Math::clamp0_255((sl_int32)(a * (1 - f))));
					SetPixel(blend, dst, ix1, iy1, src);
					src.a = (sl_uint8)(Math::clamp0_255((sl_int32)(a * f)));
					SetPixel(blend, dst, ix1, iy1 + 1, src);
				}
				yInter = yEnd + gradient; // first y-intersection for the main loop
			}
			// handle second endpoint
			{
				sl_real xEnd = Math::round(x1);
				sl_real yEnd = y1 + gradient * (xEnd - x1);
				ix2 = (sl_int32)xEnd;
				iy2 = (sl_int32)(Math::floor(yEnd));
				sl_real a = (sl_real)(color.a * GetFloatPart(x0 + 0.5f));
				sl_real f = GetFloatPart(yEnd);
				if (bSteep) {
					src.a = (sl_uint8)(Math::clamp0_255((sl_int32)(a * (1 - f))));
					SetPixel(blend, dst, iy2, ix2, src);
					src.a = (sl_uint8)(Math::clamp0_255((sl_int32)(a * f)));
					SetPixel(blend, dst, iy2 + 1, ix2, src);
				} else {
					src.a = (sl_uint8)(Math::clamp0_255((sl_int32)(a * (1 - f))));
					SetPixel(blend, dst, ix2, iy2, src);
					src.a = (sl_uint8)(Math::clamp0_255((sl_int32)(a * f)));
					SetPixel(blend, dst, ix2, iy2 + 1, src);
				}
			}
			if (bSteep) {
				for (sl_int32 ix = ix1; ix < ix2; ix++) {
					sl_int32 iy = (sl_int32)(yInter);
					sl_real f = GetFloatPart(yInter);
					src.a = (sl_uint8)(Math::clamp0_255((sl_int32)(color.a * (1 - f))));
					SetPixel(blend, dst, iy, ix, src);
					src.a = (sl_uint8)(Math::clamp0_255((sl_int32)(color.a * f)));
					SetPixel(blend, dst, iy + 1, ix, src);
					yInter += gradient;
				}
			} else {
				for (sl_int32 ix = ix1; ix < ix2; ix++) {
					sl_int32 iy = (sl_int32)(yInter);
					sl_real f = GetFloatPart(yInter);
					src.a = (sl_uint8)(Math::clamp0_255((sl_int32)(color.a * (1 - f))));
					SetPixel(blend, dst, ix, iy, src);
					src.a = (sl_uint8)(Math::clamp0_255((sl_int32)(color.a * f)));
					SetPixel(blend, dst, ix, iy + 1, src);
					yInter += gradient;
				}
			}
		}

		static void DrawLineF(ImageDesc& dst, sl_real x1, sl_real y1, sl_real x2, sl_real y2, const Color& color, BlendMode blend)
		{
			switch (blend) {
			case BlendMode::Copy:
				DrawLineF(Blend_Copy<ColorOp_None>(ColorOp_None()), dst, x1, y1, x2, y2, color);
				break;
			case BlendMode::Over:
				DrawLineF(Blend_Over<ColorOp_None>(ColorOp_None()), dst, x1, y1, x2, y2, color);
				break;
			}
		}

	}

	void Image::drawLine(sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend)
	{
		DrawLine(m_desc, x1, y1, x2, y2, color, blend, sl_false);
	}

	void Image::drawSmoothLine(sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend)
	{
		DrawLine(m_desc, x1, y1, x2, y2, color, blend, sl_true);
	}

	void Image::drawSmoothLineF(sl_real x1, sl_real y1, sl_real x2, sl_real y2, const Color& color, BlendMode blend)
	{
		DrawLineF(m_desc, x1, y1, x2, y2, color, blend);
	}

	namespace {

		template <class BLEND>
		static void FillRectangle(const BLEND& blend, ImageDesc& dst, sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color)
		{
			if (x1 == x2) {
				return;
			}
			if (y1 == y2) {
				return;
			}
			if (x1 > x2) {
				sl_int32 t = x2;
				x2 = x1;
				x1 = t;
			}
			if (y1 > y2) {
				sl_int32 t = y2;
				y2 = y1;
				y1 = t;
			}
			sl_int32 w = (sl_int32)(dst.width);
			sl_int32 h = (sl_int32)(dst.height);
			if (x2 <= 0 || x1 >= w) {
				return;
			}
			if (x1 < 0) {
				x1 = 0;
			}
			if (x2 > w) {
				x2 = w;
			}
			if (y2 <= 0 || y1 >= h) {
				return;
			}
			if (y1 < 0) {
				y1 = 0;
			}
			if (y2 > h) {
				y2 = h;
			}
			sl_reg stride = dst.stride;
			Color* colors = dst.colors + y1 * stride + x1;
			for (sl_int32 y = y1; y < y2; y++) {
				Color* d = colors;
				for (sl_int32 x = x1; x < x2; x++) {
					blend(*d, color);
					d++;
				}
				colors += stride;
			}
		}

		static void FillRectangle(ImageDesc& dst, sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend)
		{
			switch (blend) {
			case BlendMode::Copy:
				FillRectangle(Blend_Copy<ColorOp_None>(ColorOp_None()), dst, x1, y1, x2, y2, color);
				break;
			case BlendMode::Over:
				FillRectangle(Blend_Over<ColorOp_None>(ColorOp_None()), dst, x1, y1, x2, y2, color);
				break;
			}
		}

	}

	void Image::drawRectangle(sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend)
	{
		if (x1 == x2) {
			return;
		}
		if (y1 == y2) {
			return;
		}
		if (x1 > x2) {
			sl_int32 t = x2;
			x2 = x1;
			x1 = t;
		}
		if (y1 > y2) {
			sl_int32 t = y2;
			y2 = y1;
			y1 = t;
		}
		drawLine(x1, y1, x2 - 1, y1, color, blend);
		drawLine(x1, y2 - 1, x2 - 1, y2 - 1, color, blend);
		if (y2 > y1 + 2) {
			drawLine(x1, y1 + 1, x1, y2 - 2, color, blend);
			drawLine(x2 - 1, y1 + 1, x2 - 1, y2 - 2, color, blend);
		}
	}

	void Image::fillRectangle(sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend)
	{
		FillRectangle(m_desc, x1, y1, x2, y2, color, blend);
	}

	namespace {

		template <class BLEND>
		static void DrawEllipse(const BLEND& blend, ImageDesc& dst, sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, sl_bool flagAntialias)
		{
			if (x1 == x2) {
				return;
			}
			if (y1 == y2) {
				return;
			}
			if (x1 > x2) {
				sl_int32 t = x2;
				x2 = x1;
				x1 = t;
			}
			if (y1 > y2) {
				sl_int32 t = y2;
				y2 = y1;
				y1 = t;
			}
			sl_int32 cy = (y1 + y2) >> 1;
			sl_int32 w = x2 - x1;
			sl_int32 h = y2 - y1;
			sl_int32 a = w >> 1;
			sl_int32 b = h >> 1;
			if (flagAntialias) {
				sl_int32 cx = (x1 + x2) >> 1;
				sl_int32 mx1 = cx;
				sl_int32 mx2 = cx;
				sl_int32 my1 = cy;
				sl_int32 my2 = cy;
				SetPixel(blend, dst, mx2, y1, color);
				SetPixel(blend, dst, mx2, y2 - 1, color);
				if (!((x1 + x2) & 1)) {
					mx1--;
					a--;
					SetPixel(blend, dst, mx1, y1, color);
					SetPixel(blend, dst, mx1, y2 - 1, color);
				}
				SetPixel(blend, dst, x1, my2, color);
				SetPixel(blend, dst, x2 - 1, my2, color);
				if (!((y1 + y2) & 1)) {
					my1--;
					b--;
					SetPixel(blend, dst, x1, my1, color);
					SetPixel(blend, dst, x2 - 1, my1, color);
				}
				Color src = color;
				sl_int32 aq = a * a;
				sl_int32 bq = b * b;
				sl_int32 abq = aq * bq;
				sl_int32 nx = a - 1;
				sl_int32 ny = b - 1;
				float aqf = (float)aq;
				float bqf = (float)bq;
				{
					float oy = (float)b;
					sl_int32 x = 1;
					for (; x < nx; x++) {
						float y = Math::sqrt((float)(abq - x * x * bq) / aqf);
						float yf = Math::floor(y);
						sl_int32 iy = (sl_int32)yf;
						float fy = y - yf;
						if (y > oy + 1.0f) {
							break;
						}
						sl_uint8 ify = (sl_uint8)(Math::clamp0_255((sl_int32)(color.a * fy)));
						if (color.a > ify) {
							src.a = color.a - ify;
							SetPixel(blend, dst, mx2 + x, my1 - iy, src);
							SetPixel(blend, dst, mx2 + x, my2 + iy, src);
							SetPixel(blend, dst, mx1 - x, my1 - iy, src);
							SetPixel(blend, dst, mx1 - x, my2 + iy, src);
						}
						if (ify) {
							iy++;
							src.a = ify;
							SetPixel(blend, dst, mx2 + x, my1 - iy, src);
							SetPixel(blend, dst, mx2 + x, my2 + iy, src);
							SetPixel(blend, dst, mx1 - x, my1 - iy, src);
							SetPixel(blend, dst, mx1 - x, my2 + iy, src);
						}
						oy = y;
					}
				}
				{
					float ox = (float)a;
					sl_int32 y = 1;
					for (; y < ny; y++) {
						float x = Math::sqrt((float)(abq - y * y * aq) / bqf);
						float xf = Math::floor(x);
						sl_int32 ix = (sl_int32)xf;
						float fx = x - xf;
						if (x < ox - 1.0f) {
							break;
						}
						sl_uint8 ifx = (sl_uint8)(Math::clamp0_255((sl_int32)(color.a * fx)));
						if (color.a > ifx) {
							src.a = color.a - ifx;
							SetPixel(blend, dst, mx2 + ix, my1 - y, src);
							SetPixel(blend, dst, mx2 + ix, my2 + y, src);
							SetPixel(blend, dst, mx1 - ix, my1 - y, src);
							SetPixel(blend, dst, mx1 - ix, my2 + y, src);
						}
						if (ifx) {
							ix++;
							src.a = ifx;
							SetPixel(blend, dst, mx2 + ix, my1 - y, src);
							SetPixel(blend, dst, mx2 + ix, my2 + y, src);
							SetPixel(blend, dst, mx1 - ix, my1 - y, src);
							SetPixel(blend, dst, mx1 - ix, my2 + y, src);
						}
						ox = x;
					}
				}
			} else {
				sl_int32 mx1 = x1;
				sl_int32 mx2 = x2 - 1;
				sl_int32 my1 = cy;
				sl_int32 my2 = cy;
				SetPixel(blend, dst, x1, cy, color);
				SetPixel(blend, dst, x2 - 1, cy, color);
				if (!((y1 + y2) & 1)) {
					my1--;
					SetPixel(blend, dst, x1, my2, color);
					SetPixel(blend, dst, x2 - 1, my2, color);
				}
				sl_int32 aq = a * a;
				sl_int32 bq = b * b;
				sl_int32 dx = aq << 1;
				sl_int32 dy = bq << 1;
				sl_int32 r = a * bq;
				sl_int32 rx = r << 1;
				sl_int32 ry = 0;
				sl_int32 x = a;
				while (x > 0) {
					if (r > 0) {
						my1++;
						my2--;
						ry += dx;
						r -= ry;
					}
					if (r <= 0) {
						x--;
						mx1++;
						mx2--;
						rx -= dy;
						r += rx;
					}
					SetPixel(blend, dst, mx1, my1, color);
					SetPixel(blend, dst, mx1, my2, color);
					SetPixel(blend, dst, mx2, my1, color);
					SetPixel(blend, dst, mx2, my2, color);
				}
			}
		}

		static void DrawEllipse(ImageDesc& dst, sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend, sl_bool flagAntialias)
		{
			switch (blend) {
			case BlendMode::Copy:
				DrawEllipse(Blend_Copy<ColorOp_None>(ColorOp_None()), dst, x1, y1, x2, y2, color, flagAntialias);
				break;
			case BlendMode::Over:
				DrawEllipse(Blend_Over<ColorOp_None>(ColorOp_None()), dst, x1, y1, x2, y2, color, flagAntialias);
				break;
			}
		}

	}

	void Image::drawEllipse(sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend)
	{
		DrawEllipse(m_desc, x1, y1, x2, y2, color, blend, sl_false);
	}

	void Image::drawSmoothEllipse(sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend)
	{
		DrawEllipse(m_desc, x1, y1, x2, y2, color, blend, sl_true);
	}

	namespace {

		template <class BLEND>
		static void DrawImage(const BLEND& blend, ImageDesc& dst, ImageDesc& src, const Matrix3& transform, StretchMode stretch)
		{
			sl_int32 dw = (sl_int32)(dst.width);
			sl_int32 dh = (sl_int32)(dst.height);
			if (!dw) {
				return;
			}
			if (!dh) {
				return;
			}
			sl_int32 sw = (sl_int32)(src.width);
			sl_int32 sh = (sl_int32)(src.height);
			if (!sw) {
				return;
			}
			if (!sh) {
				return;
			}

			Rectangle bounds;
			bounds.setFromPoint(transform.transformPosition(0, 0));
			bounds.mergePoint(transform.transformPosition(0, (sl_real)(src.height)));
			bounds.mergePoint(transform.transformPosition((sl_real)(src.width), (sl_real)(src.height)));
			bounds.mergePoint(transform.transformPosition((sl_real)(src.width), 0));

			sl_int32 left = (sl_int32)(bounds.left);
			if (left < 0) {
				left = 0;
			}
			sl_int32 right = (sl_int32)(Math::ceil(bounds.right));
			if (right >= dw) {
				right = dw - 1;
			}
			if (right < left) {
				return;
			}
			sl_int32 top = (sl_int32)(bounds.top);
			if (top < 0) {
				top = 0;
			}
			sl_int32 bottom = (sl_int32)(Math::ceil(bounds.bottom));
			if (bottom >= dh) {
				bottom = dh - 1;
			}
			if (bottom < top) {
				return;
			}

			Matrix3 transformInverse = transform.inverse();
			sl_reg dst_stride = dst.stride;
			Color* src_colors = src.colors;
			sl_reg src_stride = src.stride;
			Color* row = dst.colors + (dst_stride * top + left);
			if (stretch == StretchMode::Nearest) {
				for (sl_int32 dy = top; dy <= bottom; dy++) {
					Color* c = row;
					for (sl_int32 dx = left; dx <= right; dx++) {
						Pointi pt = transformInverse.transformPosition((sl_real)dx, (sl_real)dy);
						if (pt.x >= 0 && pt.x < sw && pt.y >= 0 && pt.y < sh) {
							blend(*c, src_colors[src_stride * pt.y + pt.x]);
						}
						c++;
					}
					row += dst_stride;
				}
			} else {
				for (sl_int32 dy = top; dy <= bottom; dy++) {
					Color* c = row;
					for (sl_int32 dx = left; dx <= right; dx++) {
						Point pt = transformInverse.transformPosition((sl_real)dx, (sl_real)dy);
						if (pt.x > -1 && pt.y > -1) {
							sl_real sx = Math::floor(pt.x);
							sl_real sy = Math::floor(pt.y);
							sl_int32 ix = (sl_int32)sx;
							sl_int32 iy = (sl_int32)sy;
							if (ix >= -1 && ix < sw && iy >= -1 && iy < sh) {
								sl_real fx = pt.x - sx;
								sl_real fy = pt.y - sy;
								sl_int32 sa = 0, sr = 0, sg = 0, sb = 0;
								Color* sc = src_colors + (src_stride * iy + ix);
								if (ix >= 0) {
									if (iy >= 0) {
										float f = (1.0f - fx) * (1.0f - fy);
										Color& color = *sc;
										sa += (sl_int32)(color.a * f);
										sr += (sl_int32)(color.r * f);
										sg += (sl_int32)(color.g * f);
										sb += (sl_int32)(color.b * f);
									}
									if (iy < sh - 1) {
										float f = (1.0f - fx) * fy;
										Color& color = sc[src_stride];
										sa += (sl_int32)(color.a * f);
										sr += (sl_int32)(color.r * f);
										sg += (sl_int32)(color.g * f);
										sb += (sl_int32)(color.b * f);
									}
								}
								if (ix < sw - 1) {
									if (iy >= 0) {
										float f = fx * (1.0f - fy);
										Color& color = sc[1];
										sa += (sl_int32)(color.a * f);
										sr += (sl_int32)(color.r * f);
										sg += (sl_int32)(color.g * f);
										sb += (sl_int32)(color.b * f);
									}
									if (iy < sh - 1) {
										float f = fx * fy;
										Color& color = sc[src_stride + 1];
										sa += (sl_int32)(color.a * f);
										sr += (sl_int32)(color.r * f);
										sg += (sl_int32)(color.g * f);
										sb += (sl_int32)(color.b * f);
									}
								}
								blend(*c, Color(sr, sg, sb, sa));
							}
						}
						c++;
					}
					row += dst_stride;
				}
			}
		}

		template <class COLOR_OP>
		static void DrawImage(ImageDesc& dst, ImageDesc& src, const COLOR_OP& src_op, const Matrix3& transform, BlendMode blend, StretchMode stretch)
		{
			switch (blend) {
			case BlendMode::Copy:
				DrawImage(Blend_Copy<COLOR_OP>(src_op), dst, src, transform, stretch);
				break;
			case BlendMode::Over:
				DrawImage(Blend_Over<COLOR_OP>(src_op), dst, src, transform, stretch);
				break;
			}
		}

		static void DrawImage(ImageDesc& dst, ImageDesc& src, const Matrix3& transform, BlendMode blend, StretchMode stretch)
		{
			DrawImage(dst, src, ColorOp_None(), transform, blend, stretch);
		}

		static void DrawImage(ImageDesc& dst, ImageDesc& src, const Color4f& srcMul, const Color4f& srcAdd, const Matrix3& transform, BlendMode blend, StretchMode stretch)
		{
			DrawImage(dst, src, ColorOp_MulAddColor(srcMul, srcAdd), transform, blend, stretch);
		}

		static void DrawImage(ImageDesc& dst, ImageDesc& src, const ColorMatrix& cm, const Matrix3& transform, BlendMode blend, StretchMode stretch)
		{
			DrawImage(dst, src, ColorOp_ColorMatrix(&cm), transform, blend, stretch);
		}

	}

	void Image::drawImage(const Ref<Image>& src, const Matrix3& transform, BlendMode blend, StretchMode stretch)
	{
		if (src.isNull()) {
			return;
		}
		ImageDesc srcDesc;
		src->getDesc(srcDesc);
		DrawImage(m_desc, srcDesc, transform, blend, stretch);
	}

	void Image::drawImage(const Ref<Image>& src, const Color4f& srcMul, const Color4f& srcAdd, const Matrix3& transform, BlendMode blend, StretchMode stretch)
	{
		if (src.isNull()) {
			return;
		}
		ImageDesc srcDesc;
		src->getDesc(srcDesc);
		DrawImage(m_desc, srcDesc, srcMul, srcAdd, transform, blend, stretch);
	}

	void Image::drawImage(const Ref<Image>& src, const ColorMatrix& cm, const Matrix3& transform, BlendMode blend, StretchMode stretch)
	{
		if (src.isNull()) {
			return;
		}
		ImageDesc srcDesc;
		src->getDesc(srcDesc);
		DrawImage(m_desc, srcDesc, cm, transform, blend, stretch);
	}

	sl_bool Image::getDrawnBounds(Rectanglei* _out) const
	{
		sl_uint32 w = m_desc.width;
		if (!w) {
			return sl_false;
		}
		sl_uint32 h = m_desc.height;
		if (!h) {
			return sl_false;
		}

		sl_reg stride = m_desc.stride;
		Color* c = m_desc.colors;

		sl_uint32 top = 0;
		sl_uint32 bottom = 0;
		sl_uint32 left = 0;
		sl_uint32 right = 0;

		// find top
		{
			Color* row = c;
			sl_uint32 y = 0;
			for (; y < h; y++) {
				sl_uint32 x = 0;
				for (; x < w; x++) {
					if (row[x].a) {
						top = y;
						break;
					}
				}
				if (x < w) {
					break;
				}
				row += stride;
			}
			if (y >= h) {
				return sl_false;
			}
		}
		// find bottom
		{
			Color* row = c + ((h - 1) * stride);
			sl_uint32 y = 0;
			for (; y < h; y++) {
				sl_uint32 x = 0;
				for (; x < w; x++) {
					if (row[x].a) {
						bottom = h - y;
						break;
					}
				}
				if (x < w) {
					break;
				}
				row -= stride;
			}
			if (y >= h) {
				return sl_false;
			}
		}
		// find left
		{
			Color* col = c + (top * stride);
			sl_uint32 x = 0;
			for (; x < w; x++) {
				Color* p = col;
				sl_uint32 y = top;
				for (; y < bottom; y++) {
					if (p->a) {
						left = x;
						break;
					}
					p += stride;
				}
				if (y < bottom) {
					break;
				}
				col++;
			}
			if (x >= w) {
				return sl_false;
			}
		}
		// find right
		{
			Color* col = c + (top * stride + w - 1);
			sl_uint32 x = 0;
			for (; x < w; x++) {
				Color* p = col;
				sl_uint32 y = top;
				for (; y < bottom; y++) {
					if (p->a) {
						right = w - x;
						break;
					}
					p += stride;
				}
				if (y < bottom) {
					break;
				}
				col--;
			}
			if (x >= w) {
				return sl_false;
			}
		}
		if (_out) {
			_out->left = left;
			_out->top = top;
			_out->right = right;
			_out->bottom = bottom;
		}
		return sl_true;
	}

}
