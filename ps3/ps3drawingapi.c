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

// front end drawing api

void
ps3_draw_text (void *handle, int x, int y, int fonttype, int fontsize,
    int align, int colour, char *text)
{
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
ps3_draw_update (void *handle, int x, int y, int w, int h)	// TODO ( ca
								// permet de
								// rafraichir
								// que le
								// rectangle
								// (x,y,w,h) ?)
{

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
  // TODO
}

void
ps3_start_draw (void *handle)
{
  waitFlip ();			// Wait for the last flip to finish, so we can
				// draw to the old buffer

  frontend *fe = (frontend *) handle;

  rsxBuffer *buffer = fe->buffers[fe->currentBuffer];

  fe->image =
      cairo_image_surface_create_for_data ((u8 *) buffer->ptr,
      CAIRO_FORMAT_RGB24, buffer->width, buffer->height, buffer->width * 4);
  assert (fe->image != NULL);

  fe->cr = cairo_create (fe->image);

  cairo_set_antialias (fe->cr, CAIRO_ANTIALIAS_GRAY);
  cairo_set_line_width (fe->cr, 1.0);
  cairo_set_line_cap (fe->cr, CAIRO_LINE_CAP_SQUARE);
}

void
ps3_end_draw (void *handle)
{
  frontend *fe = (frontend *) handle;

  // TODO : stuff

  cairo_destroy (fe->cr);	// Realease Surface
  cairo_surface_finish (fe->image);
  cairo_surface_destroy (fe->image);	// Flush and destroy the cairo surface

  flip (fe->context, fe->currentBuffer);	// Flip buffer onto screen
  fe->currentBuffer = !fe->currentBuffer;
}

// blitted copy pasted :)

blitter *
ps3_blitter_new (void *handle, int w, int h)
{
  blitter *bl = snew (blitter);

  bl->image = cairo_image_surface_create (CAIRO_FORMAT_RGB24, w, h);
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
