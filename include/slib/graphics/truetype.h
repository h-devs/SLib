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

#ifndef CHECKHEADER_SLIB_GRAPHICS_TRUETYPE
#define CHECKHEADER_SLIB_GRAPHICS_TRUETYPE

#include "definition.h"

#include "../core/string.h"

namespace slib
{

	enum class TruetypeNameId
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

	class Truetype
	{
	public:
		static List<String> getNames(const void* _content, sl_size size, TruetypeNameId _id);

	};

}

#endif
