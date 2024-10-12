/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "font.h"
#include "color.h"

#include "../core/memory.h"
#include "../core/ptr.h"

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

	class SLIB_EXPORT FreeTypeGlyph : public CRef
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

	class SLIB_EXPORT FreeType : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		FreeType();

		~FreeType();

	public:
		class SLIB_EXPORT LoadParam
		{
		public:
			sl_int32 faceIndex; // Negative value for loading only faces count and style flags
			sl_int32 namedInstanceIndex; // Negative value for loading only named instance count

		public:
			LoadParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(LoadParam)
		};

		static Ref<FreeType> load(const Ptr<IBlockReader>& reader, sl_uint64 size, const LoadParam& param);

		static Ref<FreeType> load(const Ptr<IBlockReader>& reader, sl_uint64 size, sl_int32 indexFace = 0);

		static Ref<FreeType> loadFromFile(const StringParam& path, const LoadParam& param);

		static Ref<FreeType> loadFromFile(const StringParam& path, sl_int32 indexFace = 0);

		static Ref<FreeType> loadFromMemory(const Memory& content, const LoadParam& param);

		static Ref<FreeType> loadFromMemory(const Memory& content, sl_int32 indexFace = 0);

		static Ref<FreeType> loadSystemFont(const String& family, sl_bool flagBold = sl_false, sl_bool flagItalic = sl_false);

	public:
		FT_FaceRec_* getFaceHandle() noexcept
		{
			return m_face;
		}

	public:
		sl_uint32 getFaceCount();

		sl_uint32 getNamedInstanceCount();

		FreeTypeKind getKind();

		const char* getFamilyName();

		sl_bool isBoldStyle();

		sl_bool isItalicStyle();

		sl_uint32 getGlyphCount();

		// returns zero on error
		sl_uint32 getGlyphIndex(const char* name);

		// returns zero on error
		sl_uint32 getGlyphIndex(sl_uint32 code);

		void selectCharmap(sl_bool flagSymbolic);

		sl_bool isUnicodeEncoding();

		// Pixels
		sl_bool setSize(sl_uint32 width, sl_uint32 height);

		sl_bool setSize(sl_uint32 size);

		// Pixels
		sl_bool setRealSize(sl_real width, sl_real height);

		sl_bool setRealSize(sl_real size);

		// Pixels
		sl_real getFontHeight();

		sl_bool measureChar_NoLock(sl_uint32 charcode, TextMetrics& _out);

		sl_bool measureChar(sl_uint32 charcode, TextMetrics& _out);

		sl_bool measureText(const StringParam& text, TextMetrics& _out);

		sl_bool measureGlyph_NoLock(sl_uint32 glyphId, TextMetrics& _out);

		sl_bool measureGlyph(sl_uint32 glyphId, TextMetrics& _out);

		// draw starting at left-bottom corner
		void drawChar_NoLock(const Ref<Image>& _out, sl_int32 x, sl_int32 y, sl_char32 charcode, const Color& color);

		void drawChar(const Ref<Image>& _out, sl_int32 x, sl_int32 y, sl_char32 charcode, const Color& color);

		void drawText(const Ref<Image>& _out, sl_int32 x, sl_int32 y, const StringParam& text, const Color& color);

		enum {
			StrokeDefault = 0,
			StrokeOutside = 1,
			StrokeInside = 2
		};

		void strokeChar_NoLock(const Ref<Image>& _out, sl_int32 x, sl_int32 y, sl_char32 charcode, const Color& color, sl_uint32 lineWidth, sl_uint32 mode = StrokeDefault);

		void strokeChar(const Ref<Image>& _out, sl_int32 x, sl_int32 y, sl_char32 charcode, const Color& color, sl_uint32 lineWidth, sl_uint32 mode = StrokeDefault);

		void strokeText(const Ref<Image>& _out, sl_int32 x, sl_int32 y, const StringParam& text, const Color& color, sl_uint32 lineWidth, sl_uint32 mode = StrokeDefault);

		Ref<GraphicsPath> getCharOutline_NoLock(sl_char32 charcode);

		Ref<GraphicsPath> getCharOutline(sl_char32 charcode);

		Ref<GraphicsPath> getTextOutline(const StringParam& text);

		Ref<FreeTypeGlyph> getCharGlyph(sl_uint32 charcode);

		Ref<FreeTypeGlyph> getGlyph(sl_uint32 glyphId);

	protected:
		static Ref<FreeType> _create(CRef* lib, FT_FaceRec_* face, CRef* source);

		void _strokeText(const Ref<Image>& imageOutput, sl_int32 x, sl_int32 y, const sl_char32* str, sl_uint32 len, sl_bool flagBorder, sl_bool flagOutside, sl_uint32 radius, const Color& color);

		Ref<FreeTypeGlyph> _getGlyph(sl_uint32 glyphId);

	protected:
		Ref<CRef> m_lib;
		FT_FaceRec_* m_face;
		Ref<CRef> m_source;

	};

}

#endif
