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

#include "slib/graphics/definition.h"

#if defined(SLIB_GRAPHICS_IS_GDI)

#include "slib/graphics/bitmap.h"

#include "slib/core/scoped_buffer.h"
#include "slib/graphics/platform.h"
#include "slib/dl/win32/shlwapi.h"

namespace slib
{

	namespace {

		class BitmapImpl : public Bitmap
		{
			SLIB_DECLARE_OBJECT
		public:
			Gdiplus::Bitmap* m_bitmap;
			sl_bool m_flagFreeOnRelease;
			Ref<Referable> m_ref;

		public:
			BitmapImpl()
			{
			}

			~BitmapImpl()
			{
				if (m_flagFreeOnRelease) {
					delete m_bitmap;
				}
			}

		public:
			static Ref<BitmapImpl> create(Gdiplus::Bitmap* handle, sl_bool flagFreeOnRelease, Referable* ref)
			{
				if (handle) {
					if (handle->GetWidth() > 0 && handle->GetHeight() > 0) {
						Ref<BitmapImpl> ret = new BitmapImpl();
						if (ret.isNotNull()) {
							ret->m_bitmap = handle;
							ret->m_flagFreeOnRelease = flagFreeOnRelease;
							ret->m_ref = ref;
							return ret;
						}
					}
					if (flagFreeOnRelease) {
						delete handle;
					}
				}
				return sl_null;
			}

			static Ref<BitmapImpl> create(sl_uint32 width, sl_uint32 height)
			{
				if (width > 0 && height > 0) {
					GraphicsPlatform::startGdiplus();
					Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppARGB);
					if (bitmap) {
						return create(bitmap, sl_true, sl_null);
					}
				}
				return sl_null;
			}

			static Ref<BitmapImpl> loadFromMemory(const void* mem, sl_size size)
			{
				IStream* stream = SHCreateMemStream((BYTE*)mem, (sl_uint32)size);
				if (stream) {
					GraphicsPlatform::startGdiplus();
					Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(stream);
					stream->Release();
					if (bitmap) {
						return create(bitmap, sl_true, sl_null);
					}
				}
				return sl_null;
			}

			sl_uint32 getBitmapWidth() override
			{
				return m_bitmap->GetWidth();
			}

			sl_uint32 getBitmapHeight() override
			{
				return m_bitmap->GetHeight();
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

				Gdiplus::Rect rc(x, y, width, height);
				Gdiplus::Status result;

				if (BitmapFormats::getBitsPerSample(dst.format) == 32 && BitmapFormats::getPlaneCount(dst.format) == 1) {

					Gdiplus::BitmapData data;
					data.Width = width;
					data.Height = height;
					data.Scan0 = dst.data;
					data.Stride = (INT)(dst.pitch);
					data.PixelFormat = PixelFormat32bppARGB;

					BitmapData src(dst);
					src.format = BitmapFormat::BGRA;

					result = m_bitmap->LockBits(&rc, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeUserInputBuf, PixelFormat32bppARGB, &data);
					if (result == Gdiplus::Ok) {
						m_bitmap->UnlockBits(&data);
						dst.copyPixelsFrom(src);
						return sl_true;
					}

					return sl_false;

				} else {

					SLIB_SCOPED_BUFFER(Color, 65536, buf, width*height);
					if (!buf) {
						return sl_false;
					}

					Gdiplus::BitmapData data;
					data.Width = width;
					data.Height = height;
					data.Scan0 = buf;
					data.Stride = width << 2;
					data.PixelFormat = PixelFormat32bppARGB;

					result = m_bitmap->LockBits(&rc, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeUserInputBuf, PixelFormat32bppARGB, &data);
					if (result == Gdiplus::Ok) {

						BitmapData src(width, height, buf);
						src.format = BitmapFormat::BGRA;
						dst.copyPixelsFrom(src);

						m_bitmap->UnlockBits(&data);

						return sl_true;
					}

					return sl_false;
				}
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

				Gdiplus::Rect rc(x, y, width, height);
				Gdiplus::Status result;

				if (src.format == BitmapFormat::BGRA) {

					Gdiplus::BitmapData data;
					data.Width = width;
					data.Height = height;
					data.PixelFormat = PixelFormat32bppARGB;
					data.Stride = (INT)(src.pitch);
					data.Scan0 = src.data;

					result = m_bitmap->LockBits(&rc, Gdiplus::ImageLockModeWrite | Gdiplus::ImageLockModeUserInputBuf, PixelFormat32bppARGB, &data);
					if (result == Gdiplus::Ok) {
						result = m_bitmap->UnlockBits(&data);
						return result == Gdiplus::Ok;
					}

				} else {

					SLIB_SCOPED_BUFFER(Color, 65536, buf, width*height);
					if (!buf) {
						return sl_false;
					}

					Gdiplus::BitmapData data;
					data.Width = width;
					data.Height = height;
					data.Scan0 = buf;
					data.Stride = width << 2;
					data.PixelFormat = PixelFormat32bppARGB;

					result = m_bitmap->LockBits(&rc, Gdiplus::ImageLockModeWrite | Gdiplus::ImageLockModeUserInputBuf, PixelFormat32bppARGB, &data);

					if (result == Gdiplus::Ok) {

						BitmapData dst(width, height, buf);
						dst.format = BitmapFormat::BGRA;
						dst.copyPixelsFrom(src);

						m_bitmap->UnlockBits(&data);
					}
					return sl_true;
				}
				return sl_false;
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

				Gdiplus::Rect rc(x, y, width, height);

				Gdiplus::BitmapData data;
				data.Width = width;
				data.Height = height;
				data.PixelFormat = PixelFormat32bppARGB;
				data.Stride = width << 2;
				data.Scan0 = sl_null;
				Gdiplus::Status result = m_bitmap->LockBits(&rc, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &data);
				if (result == Gdiplus::Ok) {
					DWORD _color = color.getARGB();
					sl_uint8* row = (sl_uint8*)(data.Scan0);
					for (sl_uint32 i = 0; i < height; i++) {
						DWORD* pixels = (DWORD*)row;
						for (sl_uint32 j = 0; j < width; j++) {
							pixels[j] = _color;
						}
						row += data.Stride;
					}
					result = m_bitmap->UnlockBits(&data);
					return result == Gdiplus::Ok;
				}
				return sl_false;
			}

			Ref<Canvas> getCanvas() override
			{
				Gdiplus::Graphics* g = new Gdiplus::Graphics(m_bitmap);
				if (g) {
					sl_uint32 w = getBitmapWidth();
					sl_uint32 h = getBitmapHeight();
					Ref<BitmapImpl> thiz = this;
					return GraphicsPlatform::createCanvas(CanvasType::Bitmap, g, w, h, [g, thiz]() {
						delete g;
					});
				}
				return sl_null;
			}

			void onDraw(Canvas* canvas, const Rectangle& rectDst, const Rectangle& rectSrc, const DrawParam& param) override
			{
				GraphicsPlatform::drawImage(canvas, rectDst, m_bitmap, rectSrc, param);
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
		return BitmapImpl::loadFromMemory(mem, size);
	}

	Ref<Bitmap> GraphicsPlatform::createBitmap(Gdiplus::Bitmap* bitmap, sl_bool flagFreeOnRelease, Referable* ref)
	{
		if (!bitmap) {
			return sl_null;
		}
		return Ref<Bitmap>::from(BitmapImpl::create(bitmap, flagFreeOnRelease, ref));
	}

	Ref<Bitmap> GraphicsPlatform::createBitmap(HBITMAP hbm)
	{
		if (!hbm) {
			return sl_null;
		}
		startGdiplus();
		Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(hbm, NULL);
		if (bitmap) {
			return Ref<Bitmap>::from(BitmapImpl::create(bitmap, sl_true, sl_null));
		}
		return sl_null;
	}

	Gdiplus::Bitmap* GraphicsPlatform::getBitmapHandle(Bitmap* _bitmap)
	{
		if (BitmapImpl* bitmap = CastInstance<BitmapImpl>(_bitmap)) {
			return bitmap->m_bitmap;
		}
		return NULL;
	}

}

#endif
