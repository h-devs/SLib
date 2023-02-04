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

#include "slib/graphics/definition.h"

#if defined(SLIB_GRAPHICS_IS_GDI)

#include "slib/graphics/drawable.h"

#include "slib/graphics/image.h"
#include "slib/core/scoped_buffer.h"
#include "slib/graphics/platform.h"
#include "slib/dl/win32/shlwapi.h"

namespace slib
{

	Ref<Drawable> PlatformDrawable::create(const ImageDesc& desc)
	{
		return Bitmap::create(desc);
	}

	Ref<Drawable> PlatformDrawable::loadFromMemory(const void* buf, sl_size size)
	{
		IStream* stream = SHCreateMemStream((BYTE*)buf, (sl_uint32)size);
		if (stream) {
			GraphicsPlatform::startGdiplus();
			Gdiplus::Image* image = new Gdiplus::Image(stream);
			stream->Release();
			if (image && image->GetWidth() > 0 && image->GetHeight() > 0) {
				return GraphicsPlatform::createImageDrawable(image, sl_true);
			}
		}
		return sl_null;
	}

	namespace {

		class ImageDrawableImpl : public Drawable
		{
			SLIB_DECLARE_OBJECT
		public:
			Gdiplus::Image* m_image;
			sl_bool m_flagFreeOnRelease;
			Ref<Referable> m_ref;

		public:
			ImageDrawableImpl()
			{
			}

			~ImageDrawableImpl()
			{
				if (m_flagFreeOnRelease) {
					delete m_image;
				}
			}

		public:
			static Ref<ImageDrawableImpl> create(Gdiplus::Image* image, sl_bool flagFreeOnRelease, Referable* ref)
			{
				if (image) {
					Ref<ImageDrawableImpl> ret = new ImageDrawableImpl();
					if (ret.isNotNull()) {
						ret->m_image = image;
						ret->m_flagFreeOnRelease = flagFreeOnRelease;
						ret->m_ref = ref;
						return ret;
					}
					if (flagFreeOnRelease) {
						delete image;
					}
				}
				return sl_null;
			}

			void onDraw(Canvas* canvas, const Rectangle& rectDst, const Rectangle& rectSrc, const DrawParam& param) override
			{
				GraphicsPlatform::drawImage(canvas, rectDst, m_image, rectSrc, param);
			}

			sl_real getDrawableWidth() override
			{
				return (sl_real)(m_image->GetWidth());
			}

			sl_real getDrawableHeight() override
			{
				return (sl_real)(m_image->GetHeight());
			}

		};

		SLIB_DEFINE_OBJECT(ImageDrawableImpl, Drawable)

	}

	Ref<Drawable> GraphicsPlatform::createImageDrawable(Gdiplus::Image* image, sl_bool flagFreeOnRelease, Referable* ref)
	{
		return ImageDrawableImpl::create(image, flagFreeOnRelease, ref);
	}

	Gdiplus::Image* GraphicsPlatform::getImageDrawableHandle(Drawable* _drawable)
	{
		if (ImageDrawableImpl* drawable = CastInstance<ImageDrawableImpl>(_drawable)) {
			return drawable->m_image;
		}
		if (Bitmap* bitmap = CastInstance<Bitmap>(_drawable)) {
			return GraphicsPlatform::getBitmapHandle(bitmap);
		}
		return NULL;
	}

}

#endif
