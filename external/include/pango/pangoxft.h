/* Pango
 * pangoxft.h:
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000 SuSE Linux Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __PANGOXFT_H__
#define __PANGOXFT_H__

#include <pango/pango-context.h>
#include <pango/pango-ot.h>
#include <pango/pangofc-font.h>
#include <pango/pango-layout.h>
#include <pango/pangoxft-render.h>

G_BEGIN_DECLS

/**
 * PANGO_RENDER_TYPE_XFT:
 *
 * A string constant that was used to identify shape engines that work
 * with the Xft backend. See %PANGO_RENDER_TYPE_FC for the replacement.
 */
#ifndef PANGO_DISABLE_DEPRECATED
#define PANGO_RENDER_TYPE_XFT "PangoRenderXft"
#endif

/**
 * PangoXftFontMap:
 *
 * #PangoXftFontMap is an implementation of #PangoFcFontMap suitable for
 * the Xft library as the renderer.  It is used in to create fonts of
 * type #PangoXftFont.
 */
#define PANGO_TYPE_XFT_FONT_MAP              (pango_xft_font_map_get_type ())
#define PANGO_XFT_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_XFT_FONT_MAP, PangoXftFontMap))
#define PANGO_XFT_IS_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_XFT_FONT_MAP))

typedef struct _PangoXftFontMap      PangoXftFontMap;

/**
 * PangoXftFont:
 *
 * #PangoXftFont is an implementation of #PangoFcFont using the Xft
 * library for rendering.  It is used in conjunction with #PangoXftFontMap.
 */
typedef struct _PangoXftFont    PangoXftFont;

/**
 * PangoXftSubstituteFunc:
 * @pattern: the FcPattern to tweak.
 * @data: user data.
 *
 * Function type for doing final config tweaking on prepared FcPatterns.
 */
typedef void (*PangoXftSubstituteFunc) (FcPattern *pattern,
					gpointer   data);

/* Calls for applications
 */

PangoFontMap *pango_xft_get_font_map     (Display *display,
					  int      screen);
#ifndef PANGO_DISABLE_DEPRECATED

PangoContext *pango_xft_get_context      (Display *display,
					  int      screen);
#endif

void          pango_xft_shutdown_display (Display *display,
					  int      screen);

void pango_xft_set_default_substitute (Display                *display,
				       int                     screen,
				       PangoXftSubstituteFunc  func,
				       gpointer                data,
				       GDestroyNotify          notify);

void pango_xft_substitute_changed     (Display                *display,
				       int                     screen);

GType pango_xft_font_map_get_type (void) G_GNUC_CONST;

#define PANGO_XFT_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_XFT_FONT, PangoXftFont))
#define PANGO_TYPE_XFT_FONT              (pango_xft_font_get_type ())
#define PANGO_XFT_IS_FONT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_XFT_FONT))

GType      pango_xft_font_get_type (void) G_GNUC_CONST;

/* For shape engines
 */

#ifdef PANGO_ENABLE_ENGINE
PANGO_AVAILABLE_IN_ALL
XftFont *     pango_xft_font_get_font          (PangoFont *font);
PANGO_AVAILABLE_IN_ALL
Display *     pango_xft_font_get_display       (PangoFont *font);
#ifndef PANGO_DISABLE_DEPRECATED
PANGO_DEPRECATED_FOR(pango_fc_font_lock_face)
FT_Face       pango_xft_font_lock_face         (PangoFont *font);
PANGO_DEPRECATED_FOR(pango_fc_font_unlock_face)
void	      pango_xft_font_unlock_face       (PangoFont *font);
PANGO_DEPRECATED_FOR(pango_fc_font_get_glyph)
guint	      pango_xft_font_get_glyph	       (PangoFont *font,
						gunichar   wc);
PANGO_DEPRECATED_FOR(pango_fc_font_has_char)
gboolean      pango_xft_font_has_char          (PangoFont *font,
						gunichar   wc);
PANGO_DEPRECATED_FOR(PANGO_GET_UNKNOWN_GLYPH)
PangoGlyph    pango_xft_font_get_unknown_glyph (PangoFont *font,
						gunichar   wc);
#endif /* PANGO_DISABLE_DEPRECATED */
#endif /* PANGO_ENABLE_ENGINE */

G_END_DECLS

#endif /* __PANGOXFT_H__ */
