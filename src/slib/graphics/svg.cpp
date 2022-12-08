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

#include "slib/graphics/svg.h"

#include "slib/core/memory.h"
#include "slib/core/file.h"
#include "slib/core/asset.h"
#include "slib/core/xml.h"

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600

namespace slib
{

	namespace priv
	{
		namespace svg
		{

			SLIB_STATIC_STRING(g_attrName_viewBox, "viewBox")
			SLIB_STATIC_STRING(g_attrName_width, "width")
			SLIB_STATIC_STRING(g_attrName_height, "height")

			SLIB_INLINE static sl_svg_scalar ParseScalar(const StringView& str, sl_svg_scalar def = 0)
			{
				return str.parseFloat(def);
			}

			static sl_bool ParseViewBox(const StringView& str, sl_svg_scalar& minX, sl_svg_scalar& minY, sl_svg_scalar& width, sl_svg_scalar& height)
			{
				sl_char8* data = str.getData();
				sl_size len = str.getLength();
				sl_char8* end = data + len;
				sl_char8* start = data;
				for (sl_uint32 i = 0; i < 4; i++) {
					while (data < end) {
						if (!SLIB_CHAR_IS_SPACE_TAB(*data)) {
							break;
						}
						data++;
					}
					if (data >= end) {
						return sl_false;
					}
					if (i) {
						if (*data == ',') {
							data++;
							while (data < end) {
								if (!SLIB_CHAR_IS_SPACE_TAB(*data)) {
									break;
								}
								data++;
							}
							if (data >= end) {
								return sl_false;
							}
						}
					}
					sl_svg_scalar value;
					sl_reg result = String::parseFloat(&value, data, 0, end - data);
					if (result == SLIB_PARSE_ERROR) {
						return sl_false;
					}
					data += result;
					switch (i) {
						case 0:
							minX = value;
							break;
						case 1:
							minY = value;
							break;
						case 2:
							width = value;
							break;
						case 3:
							height = value;
							break;
					}
				}
				return sl_true;
			}

		}
	}

	using namespace priv::svg;

	SLIB_DEFINE_OBJECT(Svg, Drawable)

	Svg::Svg(): m_minX(0), m_minY(0), m_width(DEFAULT_WIDTH), m_height(DEFAULT_HEIGHT)
	{
	}

	Svg::~Svg()
	{
	}

	sl_bool Svg::_load(XmlElement* root)
	{
		String strWidth = root->getAttribute(g_attrName_width);
		m_width = ParseScalar(strWidth, m_width);
		String strHeight = root->getAttribute(g_attrName_height);
		m_height = ParseScalar(strHeight, m_height);
		String viewBox = root->getAttribute(g_attrName_viewBox);
		if (viewBox.isNotEmpty()) {
			if (!(ParseViewBox(viewBox, m_minX, m_minY, m_width, m_height))) {
				return sl_false;
			}
		} else {
			if (strWidth.isEmpty()) {
				m_width = m_height;
			} else {
				if (strHeight.isEmpty()) {
					m_height = m_width;
				}
			}
		}
		sl_size nChildren = root->getChildCount();
		for (sl_size i = 0; i < nChildren; i++) {

		}
		return sl_true;
	}

	Ref<Svg> Svg::loadFromMemory(const void* mem, sl_size size)
	{
		XmlParseParam param;
		param.flagLogError = sl_false;
		Ref<XmlDocument> xml = Xml::parse((sl_char8*)mem, size, param);
		if (xml.isNull()) {
			return sl_null;
		}
		Ref<XmlElement> root = xml->getRoot();
		if (root.isNull()) {
			return sl_null;
		}
		Ref<Svg> svg = new Svg;
		if (svg.isNotNull()) {
			if (svg->_load(root.get())) {
				return svg;
			}
		}
		return sl_null;
	}

	Ref<Svg> Svg::loadFromMemory(const MemoryView& mem)
	{
		return loadFromMemory(mem.data, mem.size);
	}

	Ref<Svg> Svg::loadFromFile(const StringParam& filePath)
	{
		Memory mem = File::readAllBytes(filePath);
		if (mem.isNotNull()) {
			return loadFromMemory(mem);
		}
		return sl_null;
	}

	Ref<Svg> Svg::loadFromAsset(const StringParam& path)
	{
		Memory mem = Assets::readAllBytes(path);
		if (mem.isNotNull()) {
			return loadFromMemory(mem);
		}
		return sl_null;
	}

	sl_svg_scalar Svg::getMinimumX()
	{
		return m_minX;
	}

	sl_svg_scalar Svg::getMinimumY()
	{
		return m_minY;
	}

	sl_svg_scalar Svg::getWidth()
	{
		return m_width;
	}

	sl_svg_scalar Svg::getHeight()
	{
		return m_height;
	}

}
