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

#include "slib/graphics/bitmap.h"

#include "slib/graphics/image.h"
#include "slib/io/file.h"
#include "slib/system/asset.h"
#include "slib/graphics/platform.h"

namespace slib
{

	Ref<Bitmap> Bitmap::create(const ImageDesc& desc)
	{
		Ref<Bitmap> ret = Bitmap::create(desc.width, desc.height);
		if (ret.isNotNull()) {
			ret->writePixels(0, 0, desc.width, desc.height, desc.colors, desc.stride);
			return ret;
		}
		return sl_null;
	}

	Ref<Bitmap> Bitmap::create(const Ref<Image>& image)
	{
		if (image.isNotNull()) {
			ImageDesc desc;
			image->getDesc(desc);
			return Bitmap::create(desc);
		}
		return sl_null;
	}

	Ref<Bitmap> Bitmap::loadFromMemory(const MemoryView& mem)
	{
		if (mem.size) {
			return Bitmap::loadFromMemory(mem.data, mem.size);
		}
		return sl_null;
	}

	Ref<Bitmap> Bitmap::loadFromFile(const StringParam& filePath)
	{
		Memory mem = File::readAllBytes(filePath);
		if (mem.isNotNull()) {
			return Bitmap::loadFromMemory(mem);
		}
		return sl_null;
	}

	Ref<Bitmap> Bitmap::loadFromAsset(const StringParam& path)
	{
		Memory mem = Assets::readAllBytes(path);
		if (mem.isNotNull()) {
			return Bitmap::loadFromMemory(mem);
		}
#if defined(SLIB_PLATFORM_IS_APPLE)
		CGImageRef image = GraphicsPlatform::loadCGImageFromApp(path);
		if (image) {
			Ref<Bitmap> ret = GraphicsPlatform::createBitmapFromCGImage(image);
			CGImageRelease(image);
			return ret;
		}
#endif
		return sl_null;
	}

	Ref<Image> Image::loadFromAsset(const StringParam& path)
	{
		Memory mem = Assets::readAllBytes(path);
		if (mem.isNotNull()) {
			return loadFromMemory(mem);
		}
#if defined(SLIB_PLATFORM_IS_APPLE)
		return Image::createCopyBitmap(Bitmap::loadFromAsset(path));
#else
		return sl_null;
#endif
	}

	Ref<Drawable> Drawable::loadFromAsset(const StringParam& path)
	{
		Memory mem = Assets::readAllBytes(path);
		if (mem.isNotNull()) {
			return loadFromMemory(mem);
		}
#if defined(SLIB_PLATFORM_IS_APPLE)
		return Bitmap::loadFromAsset(path);
#else
		return sl_null;
#endif
	}

}
