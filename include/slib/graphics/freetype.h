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

#ifndef CHECKHEADER_SLIB_GRAPHICS_FREETYPE
#define CHECKHEADER_SLIB_GRAPHICS_FREETYPE

#include "color.h"

#include "../core/object.h"
#include "../core/memory.h"
#include "../core/string.h"
#include "../core/ptr.h"
#include "../math/size.h"

struct FT_FaceRec_;

namespace slib
{

	enum class FreeTypeKind
	{
		Unknown = 0,
		Type1 = 1,
		TrueType = 2
	};
	
	class IBlockReader;
	class Image;
	class GraphicsPath;

	class SLIB_EXPORT FreeTypeGlyph : public Referable
	{
	public:
		Ref<GraphicsPath> outline;

		Ref<Image> bitmap;
		sl_int32 bitmapLeft;
		sl_int32 bitmapTop;
		sl_bool flagGrayBitmap;
		
		sl_real advance;
		sl_real height;

	public:
		FreeTypeGlyph();

		~FreeTypeGlyph();

	};

	class SLIB_EXPORT FreeTypeLoadParam
	{
	public:
		// Negative value for loading only faces count and style flags
		sl_int32 faceIndex;

		// Negative value for loading only named instance count
		sl_int32 namedInstanceIndex;

	public:
		FreeTypeLoadParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FreeTypeLoadParam)

	};

	class SLIB_EXPORT FreeType : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		FreeType();

		~FreeType();

	public:
		static Ref<FreeType> load(const Ptr<IBlockReader>& reader, sl_uint64 size, const FreeTypeLoadParam& param);

		static Ref<FreeType> load(const Ptr<IBlockReader>& reader, sl_uint64 size, sl_int32 indexFace = 0);

		static Ref<FreeType> loadFromFile(const StringParam& path, const FreeTypeLoadParam& param);

		static Ref<FreeType> loadFromFile(const StringParam& path, sl_int32 indexFace = 0);

		static Ref<FreeType> loadFromMemory(const Memory& content, const FreeTypeLoadParam& param);

		static Ref<FreeType> loadFromMemory(const Memory& content, sl_int32 indexFace = 0);

		static Ref<FreeType> loadSystemFont(const String& family);

	public:
		FT_FaceRec_* getFaceHandle() noexcept
		{
			return m_face;
		}

	public:
		sl_uint32 getFacesCount();

		sl_uint32 getNamedInstancesCount();

		FreeTypeKind getKind();

		const char* getFamilyName();

		sl_uint32 getGlyphsCount();

		sl_bool getGlyphIndex(sl_uint32 code, sl_uint32& outGlyphIndex);

		void selectCharmap(sl_bool flagSymbolic);

		sl_bool isUnicodeEncoding();

		// set size in pixels
		sl_bool setSize(sl_uint32 width, sl_uint32 height);

		sl_bool setSize(sl_uint32 size);

		// set size in pixels
		sl_bool setRealSize(sl_real width, sl_real height);

		sl_bool setRealSize(sl_real size);

		sl_real getFontHeight();

		Size getCharExtent(sl_uint32 charcode);

		Size getGlyphExtent(sl_uint32 glyphId);

		Size getStringExtent(const StringParam& text);

		// draw starting at left-bottom corner
		void drawString(const Ref<Image>& imageOutput, sl_int32 x, sl_int32 y, const StringParam& text, const Color& color);
	
		void strokeString(const Ref<Image>& imageOutput, sl_int32 x, sl_int32 y, const StringParam& text, const Color& color, sl_uint32 lineWidth);
	
		void strokeStringInside(const Ref<Image>& imageOutput, sl_int32 x, sl_int32 y, const StringParam& text, const Color& color, sl_uint32 lineWidth);
	
		void strokeStringOutside(const Ref<Image>& imageOutput, sl_int32 x, sl_int32 y, const StringParam& text, const Color& color, sl_uint32 lineWidth);

		Ref<FreeTypeGlyph> getCharGlyph(sl_uint32 charcode);

		Ref<FreeTypeGlyph> getGlyph(sl_uint32 glyphId);

	protected:
		static Ref<FreeType> _create(Referable* lib, FT_FaceRec_* face, Referable* source);

		void _strokeString(const Ref<Image>& imageOutput, sl_int32 x, sl_int32 y, const sl_char32* str, sl_uint32 len, sl_bool flagBorder, sl_bool flagOutside, sl_uint32 radius, const Color& color);

		Ref<FreeTypeGlyph> _getGlyph(sl_uint32 glyphId);

	protected:
		Ref<Referable> m_lib;
		FT_FaceRec_* m_face;
		Ref<Referable> m_source;

	};

}

#endif
