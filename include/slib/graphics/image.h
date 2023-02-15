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

#ifndef CHECKHEADER_SLIB_GRAPHICS_IMAGE
#define CHECKHEADER_SLIB_GRAPHICS_IMAGE

#include "constants.h"
#include "color.h"
#include "bitmap.h"

#include "../core/memory.h"
#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT ImageDesc
	{
	public:
		sl_uint32 width;
		sl_uint32 height;
		sl_reg stride;
		Color* colors;

		Ref<Referable> ref;

	public:
		ImageDesc();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ImageDesc)

	};

	class SLIB_EXPORT Image : public Bitmap
	{
		SLIB_DECLARE_OBJECT

	protected:
		Image();

		~Image();

	public:
		static Ref<Image> allocate(sl_uint32 width, sl_uint32 height);

		static Ref<Image> createStatic(const ImageDesc& desc);

		static Ref<Image> createStatic(sl_uint32 width, sl_uint32 height, const Color* pixels, sl_reg stride = 0, Referable* ref = sl_null);

		static Ref<Image> create(sl_uint32 width, sl_uint32 height, const Color* pixels = sl_null, sl_reg strideSource = 0);

		static Ref<Image> create(const ImageDesc& desc);

		static Ref<Image> create(const ImageDesc& desc, RotationMode rotate, FlipMode flip = FlipMode::None);

		static Ref<Image> create(const BitmapData& bitmapData);

		static Ref<Image> createFromRGB(sl_uint32 width, sl_uint32 height, const void* data, sl_uint32 bitsPerComponent = 8, sl_reg pitch = 0);

		static Ref<Image> createFromGray(sl_uint32 width, sl_uint32 height, const void* data, sl_uint32 bitsPerComponent = 8, sl_reg pitch = 0);

		static Ref<Image> createCopyAlphaFromGray(sl_uint32 width, sl_uint32 height, const void* data, sl_uint32 bitsPerComponent = 8, sl_reg pitch = 0);

		static Ref<Image> createFromIndexed(sl_uint32 width, sl_uint32 height, const void* data, const Color* indices, sl_uint32 nIndices, sl_uint32 bitsPerComponent = 8, sl_reg pitch = 0);

		static Ref<Image> createFromCMYK(sl_uint32 width, sl_uint32 height, const void* data, sl_uint32 bitsPerComponent = 8, sl_reg pitch = 0);

		static Ref<Image> createCopy(const Ref<Image>& image);

		static Ref<Image> createCopy(const Ref<Image>& image, RotationMode rotate, FlipMode flip = FlipMode::None);

		static Ref<Image> createCopyBitmap(const Ref<Bitmap>& bitmap);

		static Ref<Image> createCopyBitmap(const Ref<Bitmap>& bitmap, sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height);

		static Ref<Image> from(const Ref<Drawable>& drawable);

	protected:
		Ref<Image> getImage() override;

	public:
		sl_uint32 getWidth() const;

		sl_uint32 getHeight() const;

		sl_bool isEmpty() const;

		sl_bool isNotEmpty() const;

		sl_reg getStride() const;

		Color* getColors() const;

		Color* getColorsAt(sl_int32 x, sl_int32 y) const;

		Color& getPixel(sl_int32 x, sl_int32 y) const;

		void getDesc(ImageDesc& desc) const;


		sl_uint32 getBitmapWidth() override;

		sl_uint32 getBitmapHeight() override;

		sl_bool readPixels(sl_uint32 x, sl_uint32 y, BitmapData& desc) override;

		sl_bool writePixels(sl_uint32 x, sl_uint32 y, const BitmapData& desc) override;

		sl_bool resetPixels(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height, const Color& color) override;

		sl_bool readPixels(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height, Color* colors, sl_reg stride = 0);

		sl_bool writePixels(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height, const Color* colors, sl_reg stride = 0);

		sl_bool resetPixels(const Color& color);

		Ref<Canvas> getCanvas() override;

		void fillColor(const Color& color);

		void tintColor(const Color& color);

		void blend(const Color& colorBack, const Ref<Image>& imageOver);

		void blend(const Ref<Image>& imageBack, const Color& colorOver);

		void makeGray();

		void writeAlphaFromGray(sl_uint32 width, sl_uint32 height, const void* data, sl_uint32 bitsPerComponent = 8, sl_reg pitch = 0);

		void multiplyAlphaFromGray(sl_uint32 width, sl_uint32 height, const void* data, sl_uint32 bitsPerComponent = 8, sl_reg pitch = 0);


		static void draw(ImageDesc& dst, const ImageDesc& src, BlendMode blend = BlendMode::Over, StretchMode stretch = StretchMode::Default);

		void drawImage(sl_int32 dx, sl_int32 dy, sl_int32 dw, sl_int32 dh,
			const Ref<Image>& src, sl_int32 sx, sl_int32 sy, sl_int32 sw, sl_int32 sh,
			BlendMode blend = BlendMode::Over, StretchMode stretch = StretchMode::Default);

		void drawImage(sl_int32 dx, sl_int32 dy, sl_int32 dw, sl_int32 dh,
			const Ref<Image>& src, BlendMode blend = BlendMode::Over, StretchMode stretch = StretchMode::Default);

		void drawImage(sl_int32 dx, sl_int32 dy,
			const Ref<Image>& src, BlendMode blend = BlendMode::Over, StretchMode stretch = StretchMode::Default);

		void drawImage(const Rectanglei& rectDst,
			const Ref<Image>& src, const Rectanglei& rectSrc,
			BlendMode blend = BlendMode::Over, StretchMode stretch = StretchMode::Default);

		void drawImage(const Rectanglei& rectDst,
			const Ref<Image>& src, BlendMode blend = BlendMode::Over, StretchMode stretch = StretchMode::Default);

		void drawImage(sl_int32 dx, sl_int32 dy, sl_int32 dw, sl_int32 dh,
			const Ref<Image>& src, const Color4f& srcMul, const Color4f& srcAdd,
			sl_int32 sx, sl_int32 sy, sl_int32 sw, sl_int32 sh,
			BlendMode blend = BlendMode::Over, StretchMode stretch = StretchMode::Default);

		void drawImage(sl_int32 dx, sl_int32 dy, sl_int32 dw, sl_int32 dh,
			const Ref<Image>& src, const ColorMatrix& cm,
			sl_int32 sx, sl_int32 sy, sl_int32 sw, sl_int32 sh,
			BlendMode blend = BlendMode::Over, StretchMode stretch = StretchMode::Default);

		void copyBitmap(const Ref<Bitmap>& bitmap, sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height);


		Ref<Drawable> subDrawable(sl_real x, sl_real y, sl_real width, sl_real height) override;

		Ref<Image> sub(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height) const;

		Ref<Image> stretch(sl_uint32 width, sl_uint32 height, StretchMode stretch = StretchMode::Default) const;

		Ref<Image> stretchToSmall(sl_uint32 requiredWidth, sl_uint32 requiredHeight, sl_bool flagKeepAspectRatio = sl_true, StretchMode stretch = StretchMode::Default) const;

		Ref<Image> stretchToSmall(sl_uint32 sampleSize) const;

		Ref<Image> rotateImage(RotationMode rotate, FlipMode flip = FlipMode::None) const;

		Ref<Image> flipImage(FlipMode flip) const;

		Ref<Image> duplicate() const;

		Ref<Image> duplicate(RotationMode rotate, FlipMode flip = FlipMode::None) const;

		Ref<Image> duplicate(FlipMode flip) const;

		Ref<Image> duplicate(sl_uint32 newWidth, sl_uint32 newHeight, StretchMode stretch = StretchMode::Default) const;


		static ImageFileType getFileType(const void* mem, sl_size size);

		static ImageFileType getFileType(const MemoryView& mem);

		static Ref<Image> loadFromMemory(ImageFileType type, const void* mem, sl_size size);

		static Ref<Image> loadFromMemory(const void* mem, sl_size size);

		static Ref<Image> loadFromMemory(const MemoryView& mem);

		static Ref<Image> loadFromFile(const StringParam& filePath);

		static Ref<Image> loadFromAsset(const StringParam& path);

		static Ref<AnimationDrawable> loadAnimationFromMemory(ImageFileType type, const void* mem, sl_size size);

		static Ref<AnimationDrawable> loadAnimationFromMemory(const void* mem, sl_size size);

		static Ref<AnimationDrawable> loadAnimationFromMemory(const MemoryView& mem);

		static Ref<AnimationDrawable> loadAnimationFromFile(const StringParam& filePath);

		static Ref<AnimationDrawable> loadAnimationFromAsset(const StringParam& path);


		static Ref<Image> loadStb(const void* content, sl_size size);

		static Ref<AnimationDrawable> loadStbGif(const void* content, sl_size size);


		static Ref<Image> loadPng(const void* content, sl_size size);

		static Memory savePng(const Ref<Image>& image);

		Memory savePng();

		static sl_bool savePng(const StringParam& filePath, const Ref<Image>& image);

		sl_bool savePng(const StringParam& filePath);


		static Ref<Image> loadJpeg(const void* content, sl_size size);

		static Memory saveJpeg(const Ref<Image>& image, float quality = 0.5f);

		static Memory saveGrayJpeg(const Ref<Image>& image, float quality = 0.5f);

		Memory saveJpeg(float quality = 0.5f);

		Memory saveGrayJpeg(float quality = 0.5f);

		static sl_bool saveJpeg(const StringParam& filePath, const Ref<Image>& image, float quality = 0.5f);

		sl_bool saveJpeg(const StringParam& filePath, float quality = 0.5f);


		void drawLine(sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend = BlendMode::Over);

		void drawSmoothLine(sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend = BlendMode::Over);

		void drawSmoothLineF(sl_real x1, sl_real y1, sl_real x2, sl_real y2, const Color& color, BlendMode blend = BlendMode::Over);

		void drawRectangle(sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend = BlendMode::Over);

		void fillRectangle(sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend = BlendMode::Over);

		void drawEllipse(sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend = BlendMode::Over);

		void drawSmoothEllipse(sl_int32 x1, sl_int32 y1, sl_int32 x2, sl_int32 y2, const Color& color, BlendMode blend = BlendMode::Over);

		void drawImage(const Ref<Image>& src, const Matrix3& transform, BlendMode blend = BlendMode::Over, StretchMode stretch = StretchMode::Default);

		void drawImage(const Ref<Image>& src, const Color4f& srcMul, const Color4f& srcAdd, const Matrix3& transform, BlendMode blend = BlendMode::Over, StretchMode stretch = StretchMode::Default);

		void drawImage(const Ref<Image>& src, const ColorMatrix& cm, const Matrix3& transform, BlendMode blend = BlendMode::Over, StretchMode stretch = StretchMode::Default);

		sl_bool getDrawnBounds(Rectanglei* _out = sl_null) const;

	protected:
		Ref<Drawable> getDrawableCache(Canvas* canvas);

		void onDraw(Canvas* canvas, const Rectangle& rectDst, const Rectangle& rectSrc, const DrawParam& param) override;

		void onDrawAll(Canvas* canvas, const Rectangle& rectDst, const DrawParam& param) override;

	protected:
		ImageDesc m_desc;

	};

}

#endif
