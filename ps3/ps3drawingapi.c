/*
 * ps3drawingapi.c : PS3 Puzzles Drawing API
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 * Copyright (C) Clément Bouvet (TeToNN)
 * Copyright (C) Simon Tatham
 *
 * This software is distributed under the terms of the MIT License
 */

#include <assert.h>

#include "ps3drawingapi.h"
#include "ps3.h"
#include <sys/time.h>
#include <stdarg.h>

static void ps3_draw_text (void *handle, int x, int y, int fonttype,
    int fontsize, int align, int colour, char *text);
static void ps3_draw_rect (void *handle, int x, int y, int w, int h,
    int colour);
static void ps3_draw_line (void *handle, int x1, int y1, int x2, int y2,
    int colour);
static void ps3_draw_poly (void *handle, int *coords, int npoints,
    int fillcolour, int outlinecolour);
static void ps3_draw_circle (void *handle, int cx, int cy, int radius, int fillcolour,
    int outlinecolour);
static void ps3_draw_thick_line (void *handle, float thickness,
    float x1, float y1, float x2, float y2, int colour);
static void ps3_draw_update (void *handle, int x, int y, int w, int h);
static void ps3_clip (void *handle, int x, int y, int w, int h);
static void ps3_unclip (void *handle);
static void ps3_status_bar (void *handle, char *text);
static void ps3_start_draw (void *handle);
static void ps3_end_draw (void *handle);

static blitter *ps3_blitter_new (void *handle, int w, int h);
static void ps3_blitter_free (void *handle, blitter * bl);
static void ps3_blitter_save (void *handle, blitter * bl, int x, int y);
static void ps3_blitter_load (void *handle, blitter * bl, int x, int y);

void
fatal (char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "fatal error: ");

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);

  fprintf (stderr, "\n");
  exit (1);
}

void
get_random_seed (void **randseed, int *randseedsize)
{
  struct timeval *tvp = snew (struct timeval);

  gettimeofday (tvp, NULL);
  *randseed = (void *) tvp;
  *randseedsize = sizeof (struct timeval);
}

void
frontend_default_colour (frontend * fe, float *output)
{
  /* let's use grey as the background */
  output[0] = output[1] = output[2] = 0.7;
}

void
deactivate_timer (frontend * fe)
{
  fe->timer_enabled = FALSE;
}

void
activate_timer (frontend * fe)
{
  gettimeofday(&fe->timer_last_ts, NULL);
  fe->timer_enabled = TRUE;
}

const struct drawing_api ps3_drawing_api = {
  ps3_draw_text,
  ps3_draw_rect,
  ps3_draw_line,
  ps3_draw_poly,
  ps3_draw_circle,
  ps3_draw_update,
  ps3_clip,
  ps3_unclip,
  ps3_start_draw,
  ps3_end_draw,
  ps3_status_bar,
  ps3_blitter_new,
  ps3_blitter_free,
  ps3_blitter_save,
  ps3_blitter_load,
  NULL, NULL, NULL, NULL, NULL, NULL,   /* {begin,end}_{doc,page,puzzle} */
  NULL, NULL,                   /* line_width, line_dotted */
  NULL,
  ps3_draw_thick_line,
};


/* front end drawing api */

static void
set_colour (frontend * fe, int colour)
{
  cairo_set_source_rgb (fe->cr, fe->colours[3 * colour + 0],
      fe->colours[3 * colour + 1], fe->colours[3 * colour + 2]);
}

static void
ps3_draw_text (void *handle, int x, int y, int fonttype, int fontsize,
    int align, int colour, char *text)
{
  frontend *fe = (frontend *) handle;
  cairo_font_extents_t fex;
  cairo_text_extents_t tex;
  cairo_font_options_t *opt;

  cairo_save(fe->cr);
  set_colour(fe, colour);

  /* Set antialiasing */
  opt = cairo_font_options_create ();
  cairo_get_font_options (fe->cr, opt);
  cairo_font_options_set_antialias (opt, CAIRO_ANTIALIAS_SUBPIXEL);
  cairo_set_font_options (fe->cr, opt);
  cairo_font_options_destroy (opt);

  cairo_select_font_face(fe->cr,
      fonttype == FONT_FIXED ? "monospace" : "sans-serif",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_BOLD);

  cairo_set_font_size(fe->cr, fontsize);

  cairo_font_extents (fe->cr, &fex);
  cairo_text_extents (fe->cr, text, &tex);

  if (align & ALIGN_VCENTRE)
    y += (fex.ascent + fex.descent) / 2;

  if (align & ALIGN_HCENTRE)
    x -= tex.width / 2;
  else if (align & ALIGN_HRIGHT)
    x -= tex.width;

  x -= tex.x_bearing;

  cairo_move_to(fe->cr, x, y);
  cairo_show_text (fe->cr, text);
  cairo_restore(fe->cr);
}

static void
ps3_draw_rect (void *handle, int x, int y, int w, int h, int colour)
{
  frontend *fe = (frontend *) handle;

  cairo_save (fe->cr);
  cairo_new_path (fe->cr);
  set_colour (fe, colour);
  cairo_set_antialias (fe->cr, CAIRO_ANTIALIAS_NONE);
  cairo_rectangle (fe->cr, x, y, w, h);
  cairo_fill (fe->cr);
  cairo_restore (fe->cr);
}

static void
ps3_draw_line (void *handle, int x1, int y1, int x2, int y2, int colour)
{
  frontend *fe = (frontend *) handle;

  set_colour (fe, colour);
  cairo_new_path (fe->cr);
  cairo_move_to (fe->cr, x1 + 0.5, y1 + 0.5);
  cairo_line_to (fe->cr, x2 + 0.5, y2 + 0.5);
  cairo_stroke (fe->cr);
}

static void
ps3_draw_poly (void *handle, int *coords, int npoints, int fillcolour,
    int outlinecolour)
{
  frontend *fe = (frontend *) handle;

  int i;

  cairo_new_path (fe->cr);
  for (i = 0; i < npoints; i++)
    cairo_line_to (fe->cr, coords[i * 2] + 0.5, coords[i * 2 + 1] + 0.5);
  cairo_close_path (fe->cr);
  if (fillcolour >= 0) {
    set_colour (fe, fillcolour);
    cairo_fill_preserve (fe->cr);
  }
  assert (outlinecolour >= 0);
  set_colour (fe, outlinecolour);
  cairo_stroke (fe->cr);
}

static void
ps3_draw_circle (void *handle, int cx, int cy, int radius, int fillcolour,
    int outlinecolour)
{
  frontend *fe = (frontend *) handle;

  cairo_new_path (fe->cr);
  cairo_arc (fe->cr, cx + 0.5, cy + 0.5, radius, 0, 2 * PI);
  cairo_close_path (fe->cr);	/* Just in case... */
  if (fillcolour >= 0) {
    set_colour (fe, fillcolour);
    cairo_fill_preserve (fe->cr);
  }
  assert (outlinecolour >= 0);
  set_colour (fe, outlinecolour);
  cairo_stroke (fe->cr);
}

static void
ps3_draw_thick_line (void *handle, float thickness, float x1, float y1,
    float x2, float y2, int colour)
{
  frontend *fe = (frontend *) handle;

  set_colour (fe, colour);
  cairo_save (fe->cr);
  cairo_set_line_width (fe->cr, thickness);
  cairo_new_path (fe->cr);
  cairo_move_to (fe->cr, x1, y1);
  cairo_line_to (fe->cr, x2, y2);
  cairo_stroke (fe->cr);
  cairo_restore (fe->cr);
}

static void
ps3_draw_update (void *handle, int x, int y, int w, int h)
{
  /* The PS3 is fast enough to redraw the whole screen everytime,
     no need for a bbox here
  */
}

static void
ps3_clip (void *handle, int x, int y, int w, int h)
{
  frontend *fe = (frontend *) handle;

  cairo_new_path (fe->cr);
  cairo_rectangle (fe->cr, x, y, w, h);
  cairo_clip (fe->cr);
}

static void
ps3_unclip (void *handle)
{
  frontend *fe = (frontend *) handle;

  cairo_reset_clip (fe->cr);
}

static void
ps3_status_bar (void *handle, char *text)
{
  frontend *fe = (frontend *) handle;
  cairo_t *cr;
  float rgb[3];
  cairo_font_extents_t fex;
  int x, y;

  assert (fe->status_bar != NULL);

  cr = cairo_create (fe->status_bar);
  frontend_default_colour (fe, rgb);
  cairo_set_source_rgb (cr, rgb[0], rgb[1], rgb[2]);
  cairo_paint (cr);

  cairo_select_font_face(cr,
      "sans-serif",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_BOLD);

  cairo_set_font_size(cr, STATUS_BAR_TEXT_SIZE);

  cairo_font_extents (cr, &fex);

  x = STATUS_BAR_IPAD;
  y = STATUS_BAR_IPAD;
  y += fex.ascent + fex.descent;

  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_move_to(cr, x, y);
  cairo_show_text (cr, text);

  cairo_destroy (cr);
}

static void
ps3_start_draw (void *handle)
{
  frontend *fe = (frontend *) handle;

  assert (fe->image != NULL);

  fe->cr = cairo_create (fe->image);

  cairo_set_antialias (fe->cr, CAIRO_ANTIALIAS_GRAY);
  cairo_set_line_width (fe->cr, 1.0);
  cairo_set_line_cap (fe->cr, CAIRO_LINE_CAP_SQUARE);
  cairo_set_line_join (fe->cr, CAIRO_LINE_JOIN_ROUND);
}

static void
ps3_end_draw (void *handle)
{
  frontend *fe = (frontend *) handle;

  /* Release Surface */
  cairo_destroy (fe->cr);
  fe->cr = NULL;

  fe->redraw = TRUE;
}



// blitted copy pasted :)

static blitter *
ps3_blitter_new (void *handle, int w, int h)
{
  blitter *bl = snew (blitter);

  bl->image = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
  bl->w = w;
  bl->h = h;
  return bl;
}

static void
ps3_blitter_free (void *handle, blitter * bl)
{
  cairo_surface_destroy (bl->image);
  sfree (bl);
}

static void
ps3_blitter_save (void *handle, blitter * bl, int x, int y)
{
  frontend *fe = (frontend *) handle;
  cairo_t *cr = cairo_create (bl->image);

  cairo_set_source_surface (cr, fe->image, -x, -y);
  cairo_paint (cr);
  cairo_destroy (cr);
  bl->x = x;
  bl->y = y;
}

static void
ps3_blitter_load (void *handle, blitter * bl, int x, int y)
{
  frontend *fe = (frontend *) handle;

  if (x == BLITTER_FROMSAVED && y == BLITTER_FROMSAVED) {
    x = bl->x;
    y = bl->y;
  }
  cairo_set_source_surface (fe->cr, bl->image, x, y);
  cairo_paint (fe->cr);
}

// blitter end copy pasted
