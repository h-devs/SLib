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

#if defined(SLIB_GRAPHICS_IS_CAIRO)

#include "slib/graphics/bitmap.h"

#include "slib/graphics/image.h"
#include "slib/graphics/platform.h"

namespace slib
{

	namespace {

		class BitmapImpl : public Bitmap
		{
			SLIB_DECLARE_OBJECT

		public:
			cairo_surface_t* m_bitmap;
			sl_bool m_flagFreeOnRelease;
			Ref<Referable> m_ref;

		public:
			BitmapImpl()
			{
			}

			~BitmapImpl()
			{
				if (m_flagFreeOnRelease) {
					cairo_surface_destroy(m_bitmap);
				}
			}

		public:
			static Ref<BitmapImpl> create(cairo_surface_t* bitmap, sl_bool flagFreeOnRelease, Referable* ref)
			{
				if (bitmap) {
					Ref<BitmapImpl> ret = new BitmapImpl();
					if (ret.isNotNull()) {
						ret->m_bitmap = bitmap;
						ret->m_flagFreeOnRelease = flagFreeOnRelease;
						ret->m_ref = ref;
						return ret;
					}
					if (flagFreeOnRelease) {
						cairo_surface_destroy(bitmap);
					}
				}
				return sl_null;
			}

			static Ref<BitmapImpl> create(sl_uint32 width, sl_uint32 height)
			{
				if (width > 0 && height > 0) {
					cairo_surface_t* bitmap = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
					if (bitmap) {
						return create(bitmap, sl_true, sl_null);
					}
				}
				return sl_null;
			}

			sl_uint32 getBitmapWidth() override
			{
				return cairo_image_surface_get_width(m_bitmap);
			}

			sl_uint32 getBitmapHeight() override
			{
				return cairo_image_surface_get_height(m_bitmap);
			}

			sl_bool readPixels(sl_uint32 x, sl_uint32 y, BitmapData& _dst) override
			{

				sl_uint32 w = getBitmapWidth();
				sl_uint32 h = getBitmapHeight();
				if (x >= w || y >= h) {
					return sl_false;
				}

				BitmapData dst(_dst);
				sl_uint32 width = dst.width;
				sl_uint32 height = dst.height;

				dst.fillDefaultValues();

				if (width > w - x) {
					width = w - x;
				}
				if (height > h - y) {
					height = h - y;
				}
				if (width == 0 || height == 0) {
					return sl_true;
				}

				cairo_surface_flush(m_bitmap);

				int pitch = cairo_image_surface_get_stride(m_bitmap);
				char* buf = (char*)(cairo_image_surface_get_data(m_bitmap)) + pitch * y + (x << 2);

				BitmapData src;
				src.width = width;
				src.height = height;
				src.pitch = pitch;
				src.format = BitmapFormat::BGRA_PA;
				src.data = buf;

				dst.copyPixelsFrom(src);

				return sl_true;

			}

			sl_bool writePixels(sl_uint32 x, sl_uint32 y, const BitmapData& _src) override
			{
				sl_uint32 w = getBitmapWidth();
				sl_uint32 h = getBitmapHeight();
				if (x >= w || y >= h) {
					return sl_false;
				}

				BitmapData src(_src);
				sl_uint32 width = src.width;
				sl_uint32 height = src.height;

				src.fillDefaultValues();

				if (width > w - x) {
					width = w - x;
				}
				if (height > h - y) {
					height = h - y;
				}
				if (width == 0 || height == 0) {
					return sl_true;
				}

				int pitch = cairo_image_surface_get_stride(m_bitmap);
				char* buf = (char*)(cairo_image_surface_get_data(m_bitmap)) + pitch * y + (x << 2);

				BitmapData dst;
				dst.width = width;
				dst.height = height;
				dst.pitch = pitch;
				dst.format = BitmapFormat::BGRA_PA;
				dst.data = buf;

				dst.copyPixelsFrom(src);

				cairo_surface_mark_dirty(m_bitmap);

				return sl_true;

			}

			sl_bool resetPixels(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height, const Color& color) override
			{
				sl_uint32 w = getBitmapWidth();
				sl_uint32 h = getBitmapHeight();
				if (x >= w || y >= h) {
					return sl_false;
				}
				if (width > w - x) {
					width = w - x;
				}
				if (height > h - y) {
					height = h - y;
				}
				if (width == 0 || height == 0) {
					return sl_true;
				}

				int pitch = cairo_image_surface_get_stride(m_bitmap);
				char* buf = (char*)(cairo_image_surface_get_data(m_bitmap)) + pitch * y + (x << 2);

				Color _color = color;
				_color.convertNPAtoPA();
				sl_uint32 c = _color.getARGB();

				char* row = buf;
				for (sl_uint32 i = 0; i < height; i++) {
					sl_uint32* pixels = (sl_uint32*)row;
					for (sl_uint32 j = 0; j < width; j++) {
						pixels[j] = c;
					}
					row += pitch;
				}

				cairo_surface_mark_dirty(m_bitmap);

				return sl_true;

			}

			Ref<Canvas> getCanvas() override
			{
				cairo_t* g = cairo_create(m_bitmap);
				if (g) {
					sl_uint32 w = getBitmapWidth();
					sl_uint32 h = getBitmapHeight();
					return GraphicsPlatform::createCanvas(CanvasType::Bitmap, g, w, h);
				}
				return sl_null;
			}

			void onDraw(Canvas* canvas, const Rectangle& rectDst, const Rectangle& rectSrc, const DrawParam& param) override
			{
				cairo_surface_flush(m_bitmap);
				GraphicsPlatform::drawImage(canvas, rectDst, m_bitmap, rectSrc, param);
			}

			void onDrawAll(Canvas* canvas, const Rectangle& rectDst, const DrawParam& param) override
			{
				cairo_surface_flush(m_bitmap);
				GraphicsPlatform::drawImage(canvas, rectDst, m_bitmap, param);
			}

		};

		SLIB_DEFINE_OBJECT(BitmapImpl, Bitmap)

	}

	Ref<Bitmap> Bitmap::create(sl_uint32 width, sl_uint32 height)
	{
		return BitmapImpl::create(width, height);
	}

	Ref<Bitmap> Bitmap::loadFromMemory(const void* mem, sl_size size)
	{
		return Bitmap::create(Image::loadFromMemory(mem, size));
	}

	Ref<Bitmap> GraphicsPlatform::createBitmap(cairo_surface_t* bitmap, sl_bool flagFreeOnRelease, Referable* ref)
	{
		if (!bitmap) {
			return sl_null;
		}
		return Ref<Bitmap>::from(BitmapImpl::create(bitmap, flagFreeOnRelease, ref));
	}

	cairo_surface_t* GraphicsPlatform::getBitmapHandle(Bitmap* _bitmap)
	{
		if (BitmapImpl* bitmap = CastInstance<BitmapImpl>(_bitmap)) {
			return bitmap->m_bitmap;
		}
		return sl_null;
	}

}

#endif
