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

#include "slib/graphics/resource.h"

#include "slib/graphics/canvas.h"
#include "slib/graphics/util.h"
#include "slib/core/scoped_buffer.h"

namespace slib
{

	namespace {
		static sl_uint32 g_screenWidth = 0;
		static sl_uint32 g_screenHeight = 0;
	}

	namespace priv
	{
		namespace graphics_resource
		{

			Ref<Drawable> FileEntry::get()
			{
				if (!flagValid) {
					return sl_null;
				}
				Ref<Drawable>& ref = *((Ref<Drawable>*)((void*)&object));
				if (flagLoaded) {
					return ref;
				}
				SpinLocker locker((SpinLock*)((void*)&lock));
				if (flagLoaded) {
					return ref;
				}
				ref = Drawable::loadFromMemory(source_bytes, source_size);
				flagLoaded = sl_true;
				return ref;
			}


			FileEntriesDestructor::FileEntriesDestructor(FileEntry* entries)
			{
				m_entries = entries;
			}

			FileEntriesDestructor::~FileEntriesDestructor()
			{
				FileEntry* entry;
				entry = m_entries;
				while (entry->flagValid) {
					(*((Ref<Drawable>*)((void*)&(entry->object)))).Ref<Drawable>::~Ref();
					entry->flagValid = sl_false;
					entry++;
				}
			}


			FileEntryDestructor::FileEntryDestructor(FileEntry* entries)
			{
				m_entry = entries;
			}

			FileEntryDestructor::~FileEntryDestructor()
			{
				if (m_entry->flagValid) {
					(*((Ref<Drawable>*)((void*)&(m_entry->object)))).Ref<Drawable>::~Ref();
					m_entry->flagValid = sl_false;
				}
			}


			Ref<Drawable> GetSource(FileEntry* entries, sl_uint32 requiredWidth, sl_uint32 requiredHeight)
			{
				FileEntry* entry;
				if (!requiredWidth && !requiredHeight) {
					entry = entries;
					if (entry->flagValid) {
						return entry->get();
					}
					return sl_null;
				}

				sl_uint32 minSize = 0;
				FileEntry* minEntry = sl_null;
				entry = entries;
				while (entry->flagValid) {
					sl_uint32 width = entry->width;
					sl_uint32 height = entry->height;
					if (width >= requiredWidth && height >= requiredHeight) {
						sl_uint32 size = width * height;
						if (!minSize || size < minSize) {
							minSize = size;
							minEntry = entry;
						}
					}
					entry++;
				}
				if (minEntry) {
					return minEntry->get();
				}
				sl_uint32 maxSize = 0;
				FileEntry* maxEntry = sl_null;
				entry = entries;
				while (entry->flagValid) {
					sl_uint32 width = entry->width;
					sl_uint32 height = entry->height;
					if (width < requiredWidth || height < requiredHeight) {
						sl_uint32 size = width * height;
						if (!maxSize || size > maxSize) {
							maxSize = size;
							maxEntry = entry;
						}
					}
					entry++;
				}
				if (maxEntry) {
					return maxEntry->get();
				}
				return sl_null;
			}

			List< Ref<Drawable> > GetList(FileEntry* entries)
			{
				List< Ref<Drawable> > ret;
				FileEntry* entry;
				entry = entries;
				while (entry->flagValid) {
					Ref<Drawable> source = entry->get();
					if (source.isNotNull()) {
						ret.add_NoLock(Move(source));
					}
					entry++;
				}
				return ret;
			}

		}
	}

	namespace {

		class FileEntryDrawable : public Drawable
		{
		private:
			priv::graphics_resource::FileEntry* m_entries;
			sl_uint32 m_width;
			sl_uint32 m_height;

		public:
			FileEntryDrawable(priv::graphics_resource::FileEntry* entries, sl_uint32 width, sl_uint32 height)
			{
				m_entries = entries;
				m_width = width;
				m_height = height;
			}

		public:
			sl_real getDrawableWidth() override
			{
				return (sl_real)m_width;
			}

			sl_real getDrawableHeight() override
			{
				return (sl_real)m_height;
			}

			void onDraw(Canvas* canvas, const Rectangle& rectDst, const Rectangle& rectSrc, const DrawParam& param) override
			{
				Rectangle rectDstWhole = GraphicsUtil::transformRectangle(rectDst, rectSrc, Rectangle(0, 0, (float)m_width, (float)m_height));
				sl_int32 width = (sl_int32)(rectDstWhole.getWidth());
				sl_int32 height = (sl_int32)(rectDstWhole.getHeight());
				if (width > 0 && height > 0) {
					Ref<Drawable> source = GetSource(m_entries, width, height);
					if (source.isNotNull()) {
						float fx = source->getDrawableWidth() / (float)m_width;
						float fy = source->getDrawableHeight() / (float)m_height;
						Rectangle r;
						r.left = rectSrc.left * fx;
						r.top = rectSrc.top * fy;
						r.right = rectSrc.right * fx;
						r.bottom = rectSrc.bottom * fy;
						canvas->draw(rectDst, source, r, param);
					}
				}
			}

			void onDrawAll(Canvas* canvas, const Rectangle& rectDst, const DrawParam& param) override
			{
				sl_int32 width = (sl_int32)(rectDst.getWidth());
				sl_int32 height = (sl_int32)(rectDst.getHeight());
				if (width > 0 && height > 0) {
					Ref<Drawable> source = GetSource(m_entries, width, height);
					if (source.isNotNull()) {
						canvas->draw(rectDst, source, param);
					}
				}
			}

		};

	}

	namespace priv
	{
		namespace graphics_resource
		{
			Ref<Drawable> GetDrawable(FileEntry* entries, sl_uint32 width, sl_uint32 height)
			{
				if (entries->flagValid) {
					return new FileEntryDrawable(entries, width, height);
				}
				return sl_null;
			}
		}
	}

	sl_uint32 GraphicsResource::getScreenWidth()
	{
		return g_screenWidth;
	}

	void GraphicsResource::setScreenWidth(sl_uint32 width)
	{
		g_screenWidth = width;
	}

	sl_uint32 GraphicsResource::getScreenHeight()
	{
		return g_screenHeight;
	}

	void GraphicsResource::setScreenHeight(sl_uint32 height)
	{
		g_screenHeight = height;
	}

}
