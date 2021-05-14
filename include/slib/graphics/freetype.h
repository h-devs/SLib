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

#ifndef CHECKHEADER_SLIB_GRAPHICS_FREETYPE
#define CHECKHEADER_SLIB_GRAPHICS_FREETYPE

#include "color.h"

#include "../core/object.h"
#include "../core/memory.h"
#include "../core/string.h"
#include "../math/size.h"

struct FT_LibraryRec_;
struct FT_FaceRec_;

namespace slib
{
	
	class Image;

	namespace priv
	{
		namespace freetype
		{
			class Library;
		}
	}

	class SLIB_EXPORT FreeType : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		FreeType();

		~FreeType();

	public:
		static Ref<FreeType> loadFromMemory(const Memory& mem, sl_uint32 indexFace = 0);

		static Ref<FreeType> loadFromFile(const String& fontFilePath, sl_uint32 index = 0);

	public:
		sl_uint32 getFacesCount();

		sl_uint32 getGlyphsCount();

		// set size in pixels
		sl_bool setSize(sl_uint32 width, sl_uint32 height);

		void setSize(sl_uint32 size);

		Size getStringExtent(const StringParam& text);

		// draw starting at left-bottom corner
		void drawString(const Ref<Image>& imageOutput, sl_int32 x, sl_int32 y, const StringParam& text, const Color& color);
	
		void strokeString(const Ref<Image>& imageOutput, sl_int32 x, sl_int32 y, const StringParam& text, const Color& color, sl_uint32 lineWidth);
	
		void strokeStringInside(const Ref<Image>& imageOutput, sl_int32 x, sl_int32 y, const StringParam& text, const Color& color, sl_uint32 lineWidth);
	
		void strokeStringOutside(const Ref<Image>& imageOutput, sl_int32 x, sl_int32 y, const StringParam& text, const Color& color, sl_uint32 lineWidth);
	
	protected:
		void _strokeString(const Ref<Image>& imageOutput, sl_int32 x, sl_int32 y, const sl_char16* sz, sl_uint32 len, sl_bool flagBorder, sl_bool flagOutside, sl_uint32 radius, const Color& color);

	protected:
		Ref<priv::freetype::Library> m_libraryRef;
		FT_LibraryRec_* m_library;
		FT_FaceRec_* m_face;
		Memory m_mem;

	};

}

#endif
