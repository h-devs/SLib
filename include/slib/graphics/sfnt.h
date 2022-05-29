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

#ifndef CHECKHEADER_SLIB_GRAPHICS_SFNT
#define CHECKHEADER_SLIB_GRAPHICS_SFNT

#include "definition.h"

#include "../core/string.h"
#include "../core/list.h"
#include "../core/io.h"
#include "../core/ptrx.h"

/*
	SFNT: Font file container for TrueType and OpenType font formats
*/

namespace slib
{

	enum class TrueTypeNameId
	{
		Copyright = 0,
		FontFamily = 1,
		FontSubfamily = 2,
		UniqueId = 3,
		FullName = 4,
		VersionString = 5,
		PostScriptName = 6,
		Trademark = 7,
		Manufacture = 8,
		Designer = 9,
		Description = 10,
		VendorUrl = 11,
		DesignerUrl = 12,
		License = 13,
		LicenseUrl = 14,
		TypographicFamily = 16,
		TypographicSubfamily = 17,
		MacFullName = 18,
		SampleText = 19,
		CidFindFontName = 20,
		WWSFamily = 21,
		WWSSubfamily = 22,
		LightBackground = 23,
		DarkBackground = 24,
		VariationsPrefix = 25
	};

	enum class TrueTypePlatformId
	{
		AppleUnicode = 0,
		Macintosh = 1,
		ISO = 2,
		Microsoft = 3,
		Custom = 4,
		Adobe = 7
	};

	enum class TrueTypeEncodingId
	{
		// Apple
		Apple_Default = 0, // Unicode 1.0
		Apple_Unicode_1_1 = 1,
		Apple_ISO_10646 = 2, // deprecated
		Apple_Unicode_2_0 = 3,
		Apple_Unicode_32 = 4,
		Apple_VariantSelector = 5, // variation selector data
		Apple_FullUnicode = 6, // used with type 13 cmaps
		// Macintosh
		Mac_Roman = 0,
		Mac_Japanese = 1,
		Mac_TraditionalChinese = 2,
		Mac_Korean = 3,
		Mac_Arabic = 4,
		Mac_Hebrew = 5,
		Mac_Greek = 6,
		Mac_Russian = 7,
		Mac_RSymbol = 8,
		Mac_Devanagari = 9,
		Mac_Gurmukhi = 10,
		Mac_Gujarati = 11,
		Mac_Oriya = 12,
		Mac_Bengali = 13,
		Mac_Tamil = 14,
		Mac_Telugu = 15,
		Mac_Kannada = 16,
		Mac_Malayalam = 17,
		Mac_Sinhalese = 18,
		Mac_Rurmese = 19,
		Mac_Khmer = 20,
		Mac_Thai = 21,
		Mac_Laotian = 22,
		Mac_Georgian = 23,
		Mac_Armenian = 24,
		Mac_Maldivian = 25,
		Mac_SimplifiedChinese = 25,
		Mac_Tibetan = 26,
		Mac_Mongolian = 27,
		Mac_Geez = 28,
		Mac_Slavic = 29,
		Mac_Vietnamese = 30,
		Mac_Sindhi = 31,
		Mac_Uninterp = 32,
		// ISO
		ISO_7BitAscii = 0,
		ISO_10646 = 1,
		ISO_8859_1 = 2,
		// Microsoft
		Microsoft_Symbol = 0,
		Microsoft_Unicode = 1,
		Microsoft_SJIS = 2,
		Microsoft_PRC = 3,
		Microsoft_BIG5 = 4,
		Microsoft_Wansung = 5,
		Microsoft_Johap = 6,
		Microsoft_UCS4 = 10,
		// Adobe
		Adobe_Standard = 0,
		Adobe_Expert = 1,
		Adobe_Custom = 2,
		Adobe_Latin1 = 3
	};

	class SLIB_EXPORT SFNTFontDescriptor
	{
	public:
		List<String> familyNames;
		sl_bool flagBold;
		sl_bool flagItalic;

	public:
		SFNTFontDescriptor();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SFNTFontDescriptor)

	};

	class SLIB_EXPORT SFNT
	{
	public:
		static List<SFNTFontDescriptor> getFontDescriptors(const Ptr<IReader, ISeekable>& reader);

	};

}

#endif
