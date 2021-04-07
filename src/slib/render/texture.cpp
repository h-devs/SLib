/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/render/texture.h"

#include "slib/render/engine.h"
#include "slib/graphics/image.h"

namespace slib
{

	namespace priv
	{
		namespace texture
		{

			class TextureBitmap : public Bitmap
			{
				friend class slib::Texture;
			};
			
			class TextureBitmapCache : public BitmapCache
			{
			public:
				Ref<Texture> texture;
				
			public:
				void update(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height) override
				{
					texture->update(x, y, width, height);
				}
				
			};
						
		}
	}

	using namespace priv::texture;

	SLIB_DEFINE_OBJECT(TextureInstance, RenderBaseObjectInstance)
	
	TextureInstance::TextureInstance()
	{
	}
	
	TextureInstance::~TextureInstance()
	{
	}
	
	void TextureInstance::notifyUpdated(Texture* texture, sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height)
	{
		ObjectLocker lock(this);
		if (m_flagUpdated) {
			m_updatedRegion.mergeRectangle(Rectanglei(x, y, x + width, y + height));
		} else {
			m_updatedRegion.left = x;
			m_updatedRegion.top = y;
			m_updatedRegion.right = x + width;
			m_updatedRegion.bottom = y + height;
			m_flagUpdated = sl_true;
		}
	}
	

	SLIB_DEFINE_OBJECT(Texture, RenderBaseObject)
	
	Texture::Texture(sl_uint32 width, sl_uint32 height)
	{
		m_width = width;
		m_height = height;
	}
	
	Texture::~Texture()
	{
	}
	
	Ref<Texture> Texture::create(const Ref<Bitmap>& source)
	{
		if (source.isNotNull()) {
			sl_uint32 width = source->getWidth();
			if (width > 0) {
				sl_uint32 height = source->getHeight();
				if (height > 0) {
					return new BitmapTexture(source, width, height);
				}
			}
		}
		return sl_null;
	}
	
	Ref<Texture> Texture::create(const BitmapData& bitmapData)
	{
		return create(Image::create(bitmapData));
	}
	
	Ref<Texture> Texture::create(sl_uint32 width, sl_uint32 height, const Color* pixels, sl_int32 stride)
	{
		return create(Image::create(width, height, pixels, stride));
	}
	
	Ref<Texture> Texture::loadFromMemory(const void* mem, sl_size size)
	{
		if (size == 0) {
			return sl_null;
		}
		return create(Image::loadFromMemory(mem, size));
	}
	
	Ref<Texture> Texture::loadFromMemory(const Memory& mem)
	{
		if (mem.isNull()) {
			return sl_null;
		}
		return loadFromMemory(mem.getData(), mem.getSize());
	}
	
	Ref<Texture> Texture::loadFromFile(const String& filePath)
	{
		return create(Image::loadFromFile(filePath));
	}
	
	Ref<Texture> Texture::loadFromAsset(const String& path)
	{
		return create(Image::loadFromAsset(path));
	}
	
	Ref<Texture> Texture::getBitmapRenderingCache(const Ref<Bitmap>& source)
	{
		if (source.isNotNull()) {
			sl_uint32 width = source->getWidth();
			if (width > 0) {
				sl_uint32 height = source->getHeight();
				if (height > 0) {
					TextureBitmap* tb = (TextureBitmap*)(source.get());
					Ref<BitmapCache> cache = tb->m_renderingTextureCached;
					if (cache.isNotNull()) {
						return ((TextureBitmapCache*)(cache.get()))->texture;
					}
					Ref<Texture> ret = new WeakBitmapTexture(tb, width, height);
					if (ret.isNotNull()) {
						Ref<TextureBitmapCache> tc = new TextureBitmapCache;
						if (tc.isNotNull()) {
							tc->texture = ret;
							tb->m_renderingTextureCached = tc;
							return ret;
						}
					}
				}
			}
		}
		return sl_null;
	}
	
	sl_uint32 Texture::getWidth()
	{
		return m_width;
	}

	void Texture::setWidth(sl_uint32 width)
	{
		m_width = width;
	}
	
	sl_uint32 Texture::getHeight()
	{
		return m_height;
	}

	void Texture::setHeight(sl_uint32 height)
	{
		m_height = height;
	}

	void Texture::update(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height)
	{
		if (x >= m_width) {
			return;
		}
		if (y >= m_height) {
			return;
		}
		if (width > m_width - x) {
			width = m_width - x;
		}
		if (height > m_height - y) {
			height = m_height - y;
		}
		Ref<RenderBaseObjectInstance> instance = m_instance;
		if (instance.isNotNull()) {
			((TextureInstance*)(instance.get()))->notifyUpdated(this, x, y, width, height);
		}
	}
	
	void Texture::update()
	{
		update(0, 0, m_width, m_height);
	}
	
	Ref<TextureInstance> Texture::getInstance(RenderEngine* engine)
	{
		return Ref<TextureInstance>::from(RenderBaseObject::getInstance(engine));
	}
	

	SLIB_DEFINE_OBJECT(BitmapTexture, Texture)

	BitmapTexture::BitmapTexture(const Ref<Bitmap>& bitmap)
		: Texture(bitmap->getWidth(), bitmap->getHeight()), m_source(bitmap)
	{
	}

	BitmapTexture::BitmapTexture(const Ref<Bitmap>& bitmap, sl_uint32 width, sl_uint32 height)
		: Texture(width, height), m_source(bitmap)
	{
	}

	BitmapTexture::~BitmapTexture()
	{
	}

	Ref<Bitmap> BitmapTexture::getSource()
	{
		return m_source;
	}


	SLIB_DEFINE_OBJECT(WeakBitmapTexture, Texture)

	WeakBitmapTexture::WeakBitmapTexture(const Ref<Bitmap>& bitmap)
		: Texture(bitmap->getWidth(), bitmap->getHeight()), m_source(bitmap)
	{
	}

	WeakBitmapTexture::WeakBitmapTexture(const Ref<Bitmap>& bitmap, sl_uint32 width, sl_uint32 height)
		: Texture(width, height), m_source(bitmap)
	{
	}

	WeakBitmapTexture::~WeakBitmapTexture()
	{
	}

	Ref<Bitmap> WeakBitmapTexture::getSource()
	{
		return m_source;
	}


	SLIB_DEFINE_OBJECT(EngineTexture, Texture)
	
	EngineTexture::EngineTexture() : Texture(0, 0)
	{
	}
	
	EngineTexture::~EngineTexture()
	{
	}
	
}
