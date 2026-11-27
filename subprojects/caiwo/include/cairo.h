// Copyright 2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef CAIWO_API
#  define CAIWO_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cairo_t_impl cairo_t;

cairo_t*
cairo_create_for_canvas(const char* id);

void
cairo_destroy(cairo_t* cr);

/* State Management */

CAIWO_API void
cairo_save(cairo_t* cr);

CAIWO_API void
cairo_restore(cairo_t* cr);

/* State Modification */

CAIWO_API void
cairo_set_source_rgb(cairo_t* cr, double red, double green, double blue);

CAIWO_API void
cairo_set_source_rgba(cairo_t* cr,
                      double   red,
                      double   green,
                      double   blue,
                      double   alpha);

typedef enum {
  CAIRO_FILL_RULE_WINDING,
  CAIRO_FILL_RULE_EVEN_ODD
} cairo_fill_rule_t;

CAIWO_API void
cairo_set_fill_rule(cairo_t* cr, cairo_fill_rule_t rule);

CAIWO_API void
cairo_set_line_width(cairo_t* cr, double width);

CAIWO_API void
cairo_translate(cairo_t* cr, double tx, double ty);

CAIWO_API void
cairo_scale(cairo_t* cr, double sx, double sy);

CAIWO_API void
cairo_rotate(cairo_t* cr, double angle);

/* Path Creation */

CAIWO_API void
cairo_new_path(cairo_t* cr);

CAIWO_API void
cairo_move_to(cairo_t* cr, double x, double y);

CAIWO_API void
cairo_line_to(cairo_t* cr, double x, double y);

CAIWO_API void
cairo_arc(cairo_t* cr,
          double   xc,
          double   yc,
          double   radius,
          double   angle1,
          double   angle2);

CAIWO_API void
cairo_rectangle(cairo_t* cr, double x, double y, double width, double height);

CAIWO_API void
cairo_close_path(cairo_t* cr);

/* Painting */

CAIWO_API void
cairo_stroke(cairo_t* cr);

CAIWO_API void
cairo_stroke_preserve(cairo_t* cr);

CAIWO_API void
cairo_fill(cairo_t* cr);

CAIWO_API void
cairo_fill_preserve(cairo_t* cr);

/* Clipping */

CAIWO_API void
cairo_clip(cairo_t* cr);

/* Text */

typedef struct {
  double x_bearing;
  double y_bearing;
  double width;
  double height;
  double x_advance;
  double y_advance;
} cairo_text_extents_t;

typedef enum {
  CAIRO_FONT_SLANT_NORMAL,
  CAIRO_FONT_SLANT_ITALIC,
  CAIRO_FONT_SLANT_OBLIQUE,
} cairo_font_slant_t;

typedef enum {
  CAIRO_FONT_WEIGHT_NORMAL,
  CAIRO_FONT_WEIGHT_BOLD,
} cairo_font_weight_t;

CAIWO_API void
cairo_select_font_face(cairo_t*            cr,
                       const char*         family,
                       cairo_font_slant_t  slant,
                       cairo_font_weight_t weight);

CAIWO_API void
cairo_set_font_size(cairo_t* cr, double size);

CAIWO_API void
cairo_show_text(cairo_t* cr, const char* utf8);

CAIWO_API void
cairo_text_extents(cairo_t*              cr,
                   const char*           utf8,
                   cairo_text_extents_t* extents);

#ifdef __cplusplus
} // extern "C"
#endif
