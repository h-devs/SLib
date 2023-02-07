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

#ifndef CHECKHEADER_SLIB_GRAPHICS_SVG
#define CHECKHEADER_SLIB_GRAPHICS_SVG

#include "color.h"
#include "drawable.h"

/*
	SVG - Scalable Vector Graphics
*/

typedef float sl_svg_scalar;

namespace slib
{

	class XmlElement;

	class SLIB_EXPORT Svg : public Drawable
	{
		SLIB_DECLARE_OBJECT

	protected:
		Svg();

		~Svg();

	public:
		static Ref<Svg> loadFromMemory(const void* mem, sl_size size);

		static Ref<Svg> loadFromMemory(const MemoryView& mem);

		static Ref<Svg> loadFromFile(const StringParam& filePath);

		static Ref<Svg> loadFromAsset(const StringParam& path);

	public:
		Size getSize(sl_svg_scalar containerWidth = 100, sl_svg_scalar containerHeight = 100);

		void render(Canvas* canvas, const Rectangle& rectDraw);

	public:
		static sl_svg_scalar getGlobalFontSize();

		static void setGlobalFontSize(sl_svg_scalar size);

	public:
		sl_real getDrawableWidth() override;

		sl_real getDrawableHeight() override;

		void onDrawAll(Canvas* canvas, const Rectangle& rectDst, const DrawParam& param) override;

	protected:
		Ref<Referable> m_document;

	private:
		void _querySize();

	private:
		Size m_size;
		sl_bool m_flagQuerySize;

	};

}

#endif
