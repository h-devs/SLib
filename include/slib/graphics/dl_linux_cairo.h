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

#ifndef CHECKHEADER_SLIB_GRAPHICS_DL_LINUX_CAIRO
#define CHECKHEADER_SLIB_GRAPHICS_DL_LINUX_CAIRO

#include "../core/definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX) && defined(SLIB_PLATFORM_IS_DESKTOP)

#include "../core/dl.h"

#include "cairo/cairo.h"
#include "pango/pangocairo.h"

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(cairo, "libcairo.so.2")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_create,
			cairo_t *, cairo_public,
			cairo_surface_t * target
		)
		#define cairo_create slib::cairo::getApi_cairo_create()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_destroy,
			void, cairo_public,
			cairo_t * cr
		)
		#define cairo_destroy slib::cairo::getApi_cairo_destroy()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_save,
			void, cairo_public,
			cairo_t * cr
		)
		#define cairo_save slib::cairo::getApi_cairo_save()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_restore,
			void, cairo_public,
			cairo_t * cr
		)
		#define cairo_restore slib::cairo::getApi_cairo_restore()
		SLIB_IMPORT_LIBRARY_WRAP_FUNCTION(
			cairo_surface_create_for_rectangle,
			cairo_surface_t *, cairo_public,
			cairo_surface_t * target,
			double x,
			double y,
			double width,
			double height
		)
		#define cairo_surface_create_for_rectangle slib::cairo::wrap_cairo_surface_create_for_rectangle
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_surface_mark_dirty,
			void, cairo_public,
			cairo_surface_t * surface
		)
		#define cairo_surface_mark_dirty slib::cairo::getApi_cairo_surface_mark_dirty()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_surface_flush,
			void, cairo_public,
			cairo_surface_t * surface
		)
		#define cairo_surface_flush slib::cairo::getApi_cairo_surface_flush()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_surface_destroy,
			void, cairo_public,
			cairo_surface_t * surface
		)
		#define cairo_surface_destroy slib::cairo::getApi_cairo_surface_destroy()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_image_surface_create,
			cairo_surface_t *, cairo_public,
			cairo_format_t format,
			int width,
			int height
		)
		#define cairo_image_surface_create slib::cairo::getApi_cairo_image_surface_create()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_image_surface_get_data,
			unsigned char *, cairo_public,
			cairo_surface_t * surface
		)
		#define cairo_image_surface_get_data slib::cairo::getApi_cairo_image_surface_get_data()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_image_surface_get_format,
			cairo_format_t, cairo_public,
			cairo_surface_t * surface
		)
		#define cairo_image_surface_get_format slib::cairo::getApi_cairo_image_surface_get_format()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_image_surface_get_width,
			int, cairo_public,
			cairo_surface_t * surface
		)
		#define cairo_image_surface_get_width slib::cairo::getApi_cairo_image_surface_get_width()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_image_surface_get_height,
			int, cairo_public,
			cairo_surface_t * surface
		)
		#define cairo_image_surface_get_height slib::cairo::getApi_cairo_image_surface_get_height()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_image_surface_get_stride,
			int, cairo_public,
			cairo_surface_t * surface
		)
		#define cairo_image_surface_get_stride slib::cairo::getApi_cairo_image_surface_get_stride()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_clip,
			void, cairo_public,
			cairo_t * cr
		)
		#define cairo_clip slib::cairo::getApi_cairo_clip()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_clip_preserve,
			void, cairo_public,
			cairo_t * cr
		)
		#define cairo_clip_preserve slib::cairo::getApi_cairo_clip_preserve()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_clip_extents,
			void, cairo_public,
			cairo_t * cr,
			double * x1, double * y1,
			double * x2, double * y2
		)
		#define cairo_clip_extents slib::cairo::getApi_cairo_clip_extents()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_paint,
			void, cairo_public,
			cairo_t * cr
		)
		#define cairo_paint slib::cairo::getApi_cairo_paint()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_paint_with_alpha,
			void, cairo_public,
			cairo_t * cr,
			double alpha
		)
		#define cairo_paint_with_alpha slib::cairo::getApi_cairo_paint_with_alpha()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_fill,
			void, cairo_public,
			cairo_t * cr
		)
		#define cairo_fill slib::cairo::getApi_cairo_fill()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_fill_preserve,
			void, cairo_public,
			cairo_t * cr
		)
		#define cairo_fill_preserve slib::cairo::getApi_cairo_fill_preserve()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_stroke,
			void, cairo_public,
			cairo_t * cr
		)
		#define cairo_stroke slib::cairo::getApi_cairo_stroke()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_stroke_preserve,
			void, cairo_public,
			cairo_t * cr
		)
		#define cairo_stroke_preserve slib::cairo::getApi_cairo_stroke_preserve()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_move_to,
			void, cairo_public,
			cairo_t * cr,
			double x,
			double y
		)
		#define cairo_move_to slib::cairo::getApi_cairo_move_to()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_line_to,
			void, cairo_public,
			cairo_t * cr,
			double x,
			double y
		)
		#define cairo_line_to slib::cairo::getApi_cairo_line_to()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_curve_to,
			void, cairo_public,
			cairo_t * cr,
			double x1, double y1,
			double x2, double y2,
			double x3, double y3
		)
		#define cairo_curve_to slib::cairo::getApi_cairo_curve_to()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_rectangle,
			void, cairo_public,
			cairo_t * cr,
			double x, double y,
			double width, double height
		)
		#define cairo_rectangle slib::cairo::getApi_cairo_rectangle()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_translate,
			void, cairo_public,
			cairo_t * cr,
			double tx, double ty
		)
		#define cairo_translate slib::cairo::getApi_cairo_translate()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_scale,
			void, cairo_public,
			cairo_t * cr,
			double sx, double sy
		)
		#define cairo_scale slib::cairo::getApi_cairo_scale()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_transform,
			void, cairo_public,
			cairo_t * cr,
			const cairo_matrix_t * matrix
		)
		#define cairo_transform slib::cairo::getApi_cairo_transform()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_set_antialias,
			void, cairo_public,
			cairo_t * cr,
			cairo_antialias_t antialias
		)
		#define cairo_set_antialias slib::cairo::getApi_cairo_set_antialias()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_set_fill_rule,
			void, cairo_public,
			cairo_t * cr,
			cairo_fill_rule_t fill_rule
		)
		#define cairo_set_fill_rule slib::cairo::getApi_cairo_set_fill_rule()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_set_line_width,
			void, cairo_public,
			cairo_t * cr,
			double width
		)
		#define cairo_set_line_width slib::cairo::getApi_cairo_set_line_width()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_set_line_cap,
			void, cairo_public,
			cairo_t * cr,
			cairo_line_cap_t line_cap
		)
		#define cairo_set_line_cap slib::cairo::getApi_cairo_set_line_cap()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_set_line_join,
			void, cairo_public,
			cairo_t * cr,
			cairo_line_join_t line_join
		)
		#define cairo_set_line_join slib::cairo::getApi_cairo_set_line_join()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_set_dash,
			void, cairo_public,
			cairo_t * cr,
			const double * dashes,
			int num_dashes,
			double offset
		)
		#define cairo_set_dash slib::cairo::getApi_cairo_set_dash()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_set_miter_limit,
			void, cairo_public,
			cairo_t * cr,
			double limit
		)
		#define cairo_set_miter_limit slib::cairo::getApi_cairo_set_miter_limit()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_set_source,
			void, cairo_public,
			cairo_t * cr,
			cairo_pattern_t * source
		)
		#define cairo_set_source slib::cairo::getApi_cairo_set_source()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_set_source_rgba,
			void, cairo_public,
			cairo_t * cr,
			double red, double green, double blue,
			double alpha
		)
		#define cairo_set_source_rgba slib::cairo::getApi_cairo_set_source_rgba()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_set_source_surface,
			void, cairo_public,
			cairo_t * cr,
			cairo_surface_t * surface,
			double x, double y
		)
		#define cairo_set_source_surface slib::cairo::getApi_cairo_set_source_surface()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_close_path,
			void, cairo_public,
			cairo_t * cr
		)
		#define cairo_close_path slib::cairo::getApi_cairo_close_path()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_pattern_create_for_surface,
			cairo_pattern_t *, cairo_public,
			cairo_surface_t * surface
		)
		#define cairo_pattern_create_for_surface slib::cairo::getApi_cairo_pattern_create_for_surface()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_pattern_create_linear,
			cairo_pattern_t *, cairo_public,
			double x0, double y0,
			double x1, double y1
		)
		#define cairo_pattern_create_linear slib::cairo::getApi_cairo_pattern_create_linear()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_pattern_create_radial,
			cairo_pattern_t *, cairo_public,
			double cx0, double cy0, double radius0,
			double cx1, double cy1, double radius1
		)
		#define cairo_pattern_create_radial slib::cairo::getApi_cairo_pattern_create_radial()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_pattern_destroy,
			void, cairo_public,
			cairo_pattern_t * pattern
		)
		#define cairo_pattern_destroy slib::cairo::getApi_cairo_pattern_destroy()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_pattern_add_color_stop_rgba,
			void, cairo_public,
			cairo_pattern_t * pattern,
			double offset,
			double red, double green, double blue,
			double alpha
		)
		#define cairo_pattern_add_color_stop_rgba slib::cairo::getApi_cairo_pattern_add_color_stop_rgba()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			cairo_pattern_set_extend,
			void, cairo_public,
			cairo_pattern_t * pattern,
			cairo_extend_t extend
		)
		#define cairo_pattern_set_extend slib::cairo::getApi_cairo_pattern_set_extend()
	SLIB_IMPORT_LIBRARY_END

	SLIB_IMPORT_LIBRARY_BEGIN(pangocairo, "libpangocairo-1.0.so.0")
		SLIB_IMPORT_LIBRARY_WRAP_FUNCTION(
			pango_cairo_create_context,
			PangoContext *, ,
			cairo_t * cr
		)
		#define pango_cairo_create_context slib::pangocairo::wrap_pango_cairo_create_context
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_cairo_create_layout,
			PangoLayout *, ,
			cairo_t * cr
		)
		#define pango_cairo_create_layout slib::pangocairo::getApi_pango_cairo_create_layout()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_cairo_layout_path,
			void, ,
			cairo_t * cr,
			PangoLayout * layout
		)
		#define pango_cairo_layout_path slib::pangocairo::getApi_pango_cairo_layout_path()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_cairo_show_layout,
			void, ,
			cairo_t * cr,
			PangoLayout * layout
		)
		#define pango_cairo_show_layout slib::pangocairo::getApi_pango_cairo_show_layout()
	SLIB_IMPORT_LIBRARY_END
	
	SLIB_IMPORT_LIBRARY_BEGIN(pango, "libpango-1.0.so.0")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_context_get_metrics,
			PangoFontMetrics *, ,
			PangoContext * context,
			const PangoFontDescription * desc,
			PangoLanguage * language
		)
		#define pango_context_get_metrics slib::pango::getApi_pango_context_get_metrics()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_font_description_new,
			PangoFontDescription *, ,
			void
		)
		#define pango_font_description_new slib::pango::getApi_pango_font_description_new()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_font_description_free,
			void, ,
			PangoFontDescription * desc
		)
		#define pango_font_description_free slib::pango::getApi_pango_font_description_free()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_font_description_set_family,
			void, ,
			PangoFontDescription * desc,
			const char * family
		)
		#define pango_font_description_set_family slib::pango::getApi_pango_font_description_set_family()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_font_description_set_style,
			void, ,
			PangoFontDescription * desc,
			PangoStyle style
		)
		#define pango_font_description_set_style slib::pango::getApi_pango_font_description_set_style()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_font_description_set_weight,
			void, ,
			PangoFontDescription * desc,
			PangoWeight weight
		)
		#define pango_font_description_set_weight slib::pango::getApi_pango_font_description_set_weight()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_font_description_set_absolute_size,
			void, ,
			PangoFontDescription * desc,
			double size
		)
		#define pango_font_description_set_absolute_size slib::pango::getApi_pango_font_description_set_absolute_size()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_font_metrics_get_ascent,
			int, ,
			PangoFontMetrics * metrics
		)
		#define pango_font_metrics_get_ascent slib::pango::getApi_pango_font_metrics_get_ascent()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_font_metrics_get_descent,
			int, ,
			PangoFontMetrics * metrics
		)
		#define pango_font_metrics_get_descent slib::pango::getApi_pango_font_metrics_get_descent()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_font_metrics_unref,
			void, ,
			PangoFontMetrics * metrics
		)
		#define pango_font_metrics_unref slib::pango::getApi_pango_font_metrics_unref()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_layout_new,
			PangoLayout *, ,
			PangoContext * context
		)
		#define pango_layout_new slib::pango::getApi_pango_layout_new()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_layout_get_pixel_size,
			void, ,
			PangoLayout * layout,
			int * width,
			int * height
		)
		#define pango_layout_get_pixel_size slib::pango::getApi_pango_layout_get_pixel_size()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_layout_set_font_description,
			void, ,
			PangoLayout * layout,
			const PangoFontDescription * desc
		)
		#define pango_layout_set_font_description slib::pango::getApi_pango_layout_set_font_description()
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pango_layout_set_text,
			void, ,
			PangoLayout * layout,
			const char * text,
			int length
		)
		#define pango_layout_set_text slib::pango::getApi_pango_layout_set_text()
	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
