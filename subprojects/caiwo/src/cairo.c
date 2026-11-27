// Copyright 2026 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <cairo.h>

#include <emscripten.h>

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct cairo_t_impl {
  double              lastMoveX;
  double              lastMoveY;
  char*               canvasId;
  char*               fontFamily;
  double              fontSize;
  cairo_font_slant_t  fontSlant;
  cairo_font_weight_t fontWeight;
  cairo_fill_rule_t   fillRule;
};

cairo_t*
cairo_create_for_canvas(const char* const id)
{
  cairo_t* const cr  = (cairo_t*)malloc(sizeof(cairo_t));
  const size_t   len = strlen(id);

  cr->canvasId = (char*)malloc(len + 1U);
  memcpy(cr->canvasId, id, len + 1U);

  cr->fontFamily = (char*)malloc(12U);
  memcpy(cr->fontFamily, "sans-serif", 11U);

  cr->fontSlant  = CAIRO_FONT_SLANT_NORMAL;
  cr->fontWeight = CAIRO_FONT_WEIGHT_NORMAL;
  cr->fontSize   = 12;
  return cr;
}

void
cairo_destroy(cairo_t* const cr)
{
  if (cr) {
    free(cr->fontFamily);
    free(cr->canvasId);
    free(cr);
  }
}

/* State Management */

void
cairo_save(cairo_t* const cr)
{
  EM_ASM(
    { document.getElementById(UTF8ToString($0)).getContext('2d').save(); },
    cr->canvasId);
}

void
cairo_restore(cairo_t* const cr)
{
  EM_ASM(
    { document.getElementById(UTF8ToString($0)).getContext('2d').restore(); },
    cr->canvasId);
}

/* State Modification */

static const char hex_chars[] = "0123456789ABCDEF";

static uint8_t
level_byte(const double d)
{
  const double clamped = d < 0.0 ? 0.0 : d > 1.0 ? 1.0 : d;
  const double rounded = round(clamped * 255.0);
  return (uint8_t)rounded;
}

void
cairo_set_source_rgb(cairo_t* const cr,
                     const double   red,
                     const double   green,
                     const double   blue)
{
  const uint8_t r = level_byte(red);
  const uint8_t g = level_byte(green);
  const uint8_t b = level_byte(blue);

  const char style[8] = {'#',
                         hex_chars[r >> 4U],
                         hex_chars[r & 0x0FU],
                         hex_chars[g >> 4U],
                         hex_chars[g & 0x0FU],
                         hex_chars[b >> 4U],
                         hex_chars[b & 0x0FU],
                         '\0'};

  EM_ASM(
    {
      const ctx   = document.getElementById(UTF8ToString($0)).getContext('2d');
      const style = UTF8ToString($1);
      ctx.fillStyle   = style;
      ctx.strokeStyle = style;
    },
    cr->canvasId,
    style);
}

void
cairo_set_source_rgba(cairo_t* const cr,
                      const double   red,
                      const double   green,
                      const double   blue,
                      const double   alpha)
{
  const uint8_t r = level_byte(red);
  const uint8_t g = level_byte(green);
  const uint8_t b = level_byte(blue);
  const uint8_t a = level_byte(alpha);

  const char style[10] = {'#',
                          hex_chars[r >> 4U],
                          hex_chars[r & 0x0FU],
                          hex_chars[g >> 4U],
                          hex_chars[g & 0x0FU],
                          hex_chars[b >> 4U],
                          hex_chars[b & 0x0FU],
                          hex_chars[a >> 4U],
                          hex_chars[a & 0x0FU],
                          '\0'};

  EM_ASM(
    {
      const ctx   = document.getElementById(UTF8ToString($0)).getContext('2d');
      const style = UTF8ToString($1);
      ctx.fillStyle   = style;
      ctx.strokeStyle = style;
    },
    cr->canvasId,
    style);
}

void
cairo_set_fill_rule(cairo_t* const cr, const cairo_fill_rule_t rule)
{
  cr->fillRule = rule;
}

void
cairo_set_line_width(cairo_t* const cr, const double width)
{
  EM_ASM(
    {
      document.getElementById(UTF8ToString($0)).getContext('2d').lineWidth =
        ($1);
    },
    cr->canvasId,
    width);
}

void
cairo_translate(cairo_t* const cr, const double tx, const double ty)
{
  EM_ASM(
    {
      document.getElementById(UTF8ToString($0))
        .getContext('2d')
        .translate($1, $2);
    },
    cr->canvasId,
    tx,
    ty);
}

void
cairo_scale(cairo_t* const cr, const double sx, const double sy)
{
  EM_ASM(
    {
      document.getElementById(UTF8ToString($0)).getContext('2d').scale($1, $2);
    },
    cr->canvasId,
    sx,
    sy);
}

void
cairo_rotate(cairo_t* const cr, const double angle)
{
  EM_ASM(
    { document.getElementById(UTF8ToString($0)).getContext('2d').rotate($1); },
    cr->canvasId,
    angle);
}

/* Path Creation */

void
cairo_new_path(cairo_t* const cr)
{
  EM_ASM(
    { document.getElementById(UTF8ToString($0)).getContext('2d').beginPath(); },
    cr->canvasId);
}

void
cairo_move_to(cairo_t* const cr, const double x, const double y)
{
  EM_ASM(
    {
      document.getElementById(UTF8ToString($0)).getContext('2d').moveTo($1, $2);
    },
    cr->canvasId,
    x,
    y);

  cr->lastMoveX = x;
  cr->lastMoveY = y;
}

void
cairo_line_to(cairo_t* const cr, const double x, const double y)
{
  EM_ASM(
    {
      document.getElementById(UTF8ToString($0)).getContext('2d').lineTo($1, $2);
    },
    cr->canvasId,
    x,
    y);
}

void
cairo_arc(cairo_t* const cr,
          const double   xc,
          const double   yc,
          const double   radius,
          const double   angle1,
          const double   angle2)
{
  EM_ASM(
    {
      document.getElementById(UTF8ToString($0))
        .getContext('2d')
        .arc($1, $2, $3, $4, $5);
    },
    cr->canvasId,
    xc,
    yc,
    radius,
    angle1,
    angle2);
}

void
cairo_rectangle(cairo_t* const cr,
                const double   x,
                const double   y,
                const double   width,
                const double   height)
{
  EM_ASM(
    {
      document.getElementById(UTF8ToString($0))
        .getContext('2d')
        .rect($1, $2, $3, $4);
    },
    cr->canvasId,
    x,
    y,
    width,
    height);
}

void
cairo_close_path(cairo_t* const cr)
{
  EM_ASM(
    { document.getElementById(UTF8ToString($0)).getContext('2d').closePath(); },
    cr->canvasId);
}

/* Painting */

void
cairo_stroke(cairo_t* const cr)
{
  cairo_stroke_preserve(cr);
  cairo_new_path(cr);
}

void
cairo_stroke_preserve(cairo_t* const cr)
{
  EM_ASM(
    { document.getElementById(UTF8ToString($0)).getContext('2d').stroke(); },
    cr->canvasId);
}

void
cairo_fill(cairo_t* const cr)
{
  cairo_fill_preserve(cr);
  cairo_new_path(cr);
}

void
cairo_fill_preserve(cairo_t* const cr)
{
  EM_ASM(
    {
      const ctx = document.getElementById(UTF8ToString($0)).getContext('2d');
      if ($1) {
        ctx.fill('evenodd');
      } else {
        ctx.fill();
      }
    },
    cr->canvasId,
    cr->fillRule);
}

/* Clipping */

void
cairo_clip(cairo_t* const cr)
{
  EM_ASM(
    { document.getElementById(UTF8ToString($0)).getContext('2d').clip(); },
    cr->canvasId);
}

/* Text */

static void
update_font(const cairo_t* const cr)
{
  EM_ASM(
    {
      const prefixes = [];
      if ($1 == 1) {
        prefixes.push('italic');
      } else if ($1 == 2) {
        prefixes.push('oblique');
      }

      if ($2 == 1) {
        prefixes.push('bold');
      }

      prefix = prefixes.join(' ') + ' ';
      font   = prefix + String($3) + 'px ' + UTF8ToString($4);
      document.getElementById(UTF8ToString($0)).getContext('2d').font = font;
    },
    cr->canvasId,
    cr->fontSlant,
    cr->fontWeight,
    cr->fontSize,
    cr->fontFamily);
}

void
cairo_select_font_face(cairo_t* const            cr,
                       const char* const         family,
                       const cairo_font_slant_t  slant,
                       const cairo_font_weight_t weight)
{
  const size_t familyLen = strlen(family);
  char* const  newFamily = (char*)realloc(cr->fontFamily, familyLen + 1U);
  if (newFamily) {
    memcpy(newFamily, family, familyLen + 1U);
    cr->fontFamily = newFamily;
    cr->fontSlant  = slant;
    cr->fontWeight = weight;
  }
  update_font(cr);
}

void
cairo_set_font_size(cairo_t* const cr, const double size)
{
  cr->fontSize = size;
  update_font(cr);
}

void
cairo_show_text(cairo_t* const cr, const char* const utf8)
{
  EM_ASM(
    {
      document.getElementById(UTF8ToString($0))
        .getContext('2d')
        .fillText(UTF8ToString($1), $2, $3);
    },
    cr->canvasId,
    utf8,
    cr->lastMoveX,
    cr->lastMoveY);
}

void
cairo_text_extents(cairo_t* const              cr,
                   const char* const           utf8,
                   cairo_text_extents_t* const extents)
{
  EM_ASM(
    {
      const m = document.getElementById(UTF8ToString($0))
                  .getContext('2d')
                  .measureText(UTF8ToString($1));

      const extents = Module.HEAPF64.subarray($2 / 8, ($2 / 8) + 6);
      const height  = m.actualBoundingBoxAscent + m.actualBoundingBoxDescent;

      extents[0] = m.actualBoundingBoxLeft;
      extents[1] = m.actualBoundingBoxAscent;
      extents[2] = m.width;
      extents[3] = height;
      extents[4] = m.width;
      extents[5] = height;
    },
    cr->canvasId,
    utf8,
    extents);
}

#ifdef __cplusplus
} // extern "C"
#endif
