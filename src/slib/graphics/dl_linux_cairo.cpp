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

#define SLIB_IMPLEMENT_DYNAMIC_LIBRARY

#include "slib/graphics/dl/linux/cairo.h"

namespace slib
{
	namespace cairo
	{

		cairo_public cairo_surface_t* wrap_cairo_surface_create_for_rectangle(
			cairo_surface_t* target,
			double x,
			double y,
			double width,
			double height
		) {
			auto func = getApi_cairo_surface_create_for_rectangle();
			if (func) {
				return func(target, x, y, width, height);
			}
			return sl_null;
		}

	}

	namespace pango
	{

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_context_new,
			PangoContext *, ,
			void
		)
		#define pango_context_new slib::pango::getApi_pango_context_new()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_context_set_font_map,
			void, ,
			PangoContext *context,
			PangoFontMap *font_map
		)
		#define pango_context_set_font_map slib::pango::getApi_pango_context_set_font_map()

	}

	namespace pangocairo
	{

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_cairo_font_map_get_default,
			PangoFontMap *, ,
			void
		)
		#define pango_cairo_font_map_get_default slib::pangocairo::getApi_pango_cairo_font_map_get_default()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_cairo_update_context,
			void, ,
			cairo_t *cr,
			PangoContext *context
		)
		#define pango_cairo_update_context slib::pangocairo::getApi_pango_cairo_update_context()

		PangoContext * wrap_pango_cairo_create_context(cairo_t * cr)
		{
			auto func = getApi_pango_cairo_create_context(); // defined above pango >= 1.21.0
			if (func) {
				return func(cr);
			}
			if (!cr) {
				return sl_null;
			}
			PangoFontMap* fontmap = pango_cairo_font_map_get_default();
			if (!fontmap) {
				return sl_null;
			}
			PangoContext* context = pango_context_new();
			pango_context_set_font_map(context, fontmap);
			pango_cairo_update_context(cr, context);
			return context;
		}

	}
}
