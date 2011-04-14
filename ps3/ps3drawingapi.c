/*
 * ps3drawingapi.c : PS3 Puzzles Drawing API
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 * Copyright (C) Clément Bouvet (TeToNN)
 * Copyright (C) Simon Tatham
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 */

#include "ps3drawingapi.h"
#include "ps3.h"

/* front end drawing api */

static void
set_colour (frontend * fe, int colour)
{
  cairo_set_source_rgb (fe->cr, fe->colours[3 * colour + 0],
      fe->colours[3 * colour + 1], fe->colours[3 * colour + 2]);
}

void
ps3_draw_text (void *handle, int x, int y, int fonttype, int fontsize,
    int align, int colour, char *text)
{
  frontend *fe = (frontend *) handle;
  cairo_font_extents_t fex;
  cairo_text_extents_t tex;

  cairo_save(fe->cr);
  set_colour(fe, colour);
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

  cairo_move_to(fe->cr, x, y);
  cairo_show_text (fe->cr, text);
  cairo_restore(fe->cr);
}

void
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

void
ps3_draw_line (void *handle, int x1, int y1, int x2, int y2, int colour)
{
  frontend *fe = (frontend *) handle;

  set_colour (fe, colour);
  cairo_new_path (fe->cr);
  cairo_move_to (fe->cr, x1 + 0.5, y1 + 0.5);
  cairo_line_to (fe->cr, x2 + 0.5, y2 + 0.5);
  cairo_stroke (fe->cr);
}

void
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

void
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

void
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

void
ps3_draw_update (void *handle, int x, int y, int w, int h)
{
  /* The PS3 is fast enough to redraw the whole screen everytime,
     no need for a bbox here

     TODO: Make sure we don't actually need to set the bbox with
     setRenderTarget
  */
}

void
ps3_clip (void *handle, int x, int y, int w, int h)
{
  frontend *fe = (frontend *) handle;

  cairo_new_path (fe->cr);
  cairo_rectangle (fe->cr, x, y, w, h);
  cairo_clip (fe->cr);
}

void
ps3_unclip (void *handle)
{
  frontend *fe = (frontend *) handle;

  cairo_reset_clip (fe->cr);
}

void
ps3_status_bar (void *handle, char *text)
{
  frontend *fe = (frontend *) handle;

  if (fe->status_text)
    sfree (fe->status_text);
  fe->status_text = dupstr (text);
}

void
ps3_start_draw (void *handle)
{
  frontend *fe = (frontend *) handle;
  rsxBuffer *buffer = &fe->buffers[fe->currentBuffer];

  DEBUG ("Starting to draw\n");
  setRenderTarget(fe->context, buffer);
  /* Wait for the last flip to finish, so we can draw to the old buffer */
  waitFlip ();

  fe->surface = cairo_image_surface_create_for_data ((u8 *) buffer->ptr,
      CAIRO_FORMAT_ARGB32, buffer->width, buffer->height, buffer->width * 4);
  assert (fe->surface != NULL);

  fe->image = cairo_surface_create_for_rectangle  (fe->surface,
      fe->x, fe->y, fe->width, fe->height);
  assert (fe->image != NULL);

  fe->cr = cairo_create (fe->image);

  cairo_set_antialias (fe->cr, CAIRO_ANTIALIAS_GRAY);
  cairo_set_line_width (fe->cr, 1.0);
  cairo_set_line_cap (fe->cr, CAIRO_LINE_CAP_SQUARE);
  cairo_set_line_join (fe->cr, CAIRO_LINE_JOIN_ROUND);
}

void
ps3_end_draw (void *handle)
{
  frontend *fe = (frontend *) handle;

  DEBUG ("Finished drawing\n");

  /* Release Surface */
  cairo_destroy (fe->cr);
  cairo_surface_finish (fe->image);
  cairo_surface_destroy (fe->image);
  cairo_surface_finish (fe->surface);
  cairo_surface_destroy (fe->surface);
  fe->cr = NULL;
  fe->image = NULL;
  fe->surface = NULL;

  /* Flip buffer onto screen */
  flip (fe->context, fe->currentBuffer);
  fe->currentBuffer++;
  if (fe->currentBuffer >= MAX_BUFFERS)
    fe->currentBuffer = 0;
}

// blitted copy pasted :)

blitter *
ps3_blitter_new (void *handle, int w, int h)
{
  blitter *bl = snew (blitter);

  bl->image = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
  bl->w = w;
  bl->h = h;
  return bl;
}

void
ps3_blitter_free (void *handle, blitter * bl)
{
  cairo_surface_destroy (bl->image);
  sfree (bl);
}

void
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

void
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
