/*
 * ps3graphics.c : PS3 Puzzles Main graphics engine
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 */

#include "ps3graphics.h"

static void draw_background (frontend *fe, cairo_t *cr);
static void draw_puzzle (frontend *fe, cairo_t *cr);
static void draw_pointer (frontend *fe, cairo_t *cr);
static void draw_status_bar (frontend *fe, cairo_t *cr);
static void draw_main_menu (frontend *fe, cairo_t *cr);
static void draw_puzzle_menu (frontend *fe, cairo_t *cr);
static void draw_types_menu (frontend *fe, cairo_t *cr);

void
ps3_prepare_buffer (frontend *fe)
{
  rsxBuffer *buffer = &fe->buffers[fe->currentBuffer];

  setRenderTarget(fe->context, buffer);
  /* Wait for the last flip to finish, so we can draw to the old buffer */
  waitFlip ();
}

void
ps3_render_buffer (frontend *fe)
{
  rsxBuffer *buffer = &fe->buffers[fe->currentBuffer];
  cairo_surface_t *surface;
  cairo_t *cr;

  /* Draw our window */
  surface = cairo_image_surface_create_for_data ((u8 *) buffer->ptr,
      CAIRO_FORMAT_ARGB32, buffer->width, buffer->height, buffer->width * 4);

  cr = cairo_create (surface);
  draw_background (fe, cr);
  if (fe->mode != MODE_PUZZLE_MENU) {
    draw_puzzle (fe, cr);
    draw_status_bar (fe, cr);
    if (fe->mode == MODE_TYPES_MENU)
      draw_types_menu (fe, cr);
    else if (fe->mode == MODE_MAIN_MENU)
      draw_main_menu (fe, cr);
    else if (fe->cursor_last_move == FALSE)
      draw_pointer (fe, cr);
  } else {
    draw_puzzle_menu (fe, cr);
  }
  cairo_destroy (cr);

  cairo_surface_finish (surface);
  cairo_surface_destroy (surface);

  /* Flip buffer onto screen */
  flipBuffer (fe->context, fe->currentBuffer);
  fe->currentBuffer++;
  if (fe->currentBuffer >= MAX_BUFFERS)
    fe->currentBuffer = 0;
}

void
ps3_refresh_draw (frontend *fe)
{
  ps3_prepare_buffer (fe);
  ps3_render_buffer (fe);
}

static void
draw_background (frontend *fe, cairo_t *cr)
{
  /* Pre-cache the gradient background pattern into a cairo image surface.
   * The cairo_pattern is a vector basically, so when painting the gradient
   * into our surface, it needs to rasterize it, which makes the FPS drop
   * to 6 or 7 FPS and makes the game unusable (misses controller input, and
   * animations don't work anymore). So by pre-rasterizing it into an
   * image surface, we can do the background paint very quickly and FPS should
   * stay at 60fps or if we miss the VSYNC, drop to 30fps.
   */
  if (fe->background == NULL) {
    cairo_pattern_t *linpat = NULL;
    cairo_t *grad_cr = NULL;

    fe->background = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
        fe->buffers[0].width, fe->buffers[0].height);

    linpat = cairo_pattern_create_linear (0, 0,
        fe->buffers[0].width, fe->buffers[0].height);
    cairo_pattern_add_color_stop_rgb (linpat, 0, 0, 0.3, 0.8);
    cairo_pattern_add_color_stop_rgb (linpat, 1, 0, 0.8, 0.3);

    grad_cr = cairo_create (fe->background);
    cairo_set_source (grad_cr, linpat);
    cairo_paint (grad_cr);
    cairo_destroy (grad_cr);
    cairo_pattern_destroy (linpat);
    cairo_surface_flush (fe->background);
  }
  cairo_set_source_surface (cr, fe->background, 0, 0);
  cairo_paint (cr);
}

static void
draw_puzzle (frontend *fe, cairo_t *cr)
{
  cairo_set_source_surface (cr, fe->image, fe->x, fe->y);
  cairo_paint (cr);
}

static void
draw_pointer (frontend *fe, cairo_t *cr)
{
  cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
  cairo_move_to (cr, fe->pointer_x + fe->x - 5, fe->pointer_y + fe->y);
  cairo_line_to (cr, fe->pointer_x + fe->x + 5, fe->pointer_y + fe->y);
  cairo_move_to (cr, fe->pointer_x + fe->x, fe->pointer_y + fe->y - 5);
  cairo_line_to (cr, fe->pointer_x + fe->x, fe->pointer_y + fe->y + 5);
  cairo_stroke (cr);
}

static void
draw_status_bar (frontend *fe, cairo_t *cr)
{
  if (fe->status_bar) {
    cairo_set_source_surface (cr, fe->status_bar,
        fe->status_x, fe->status_y);
    cairo_paint_with_alpha (cr, STATUS_BAR_ALPHA);
  }
}

static void
draw_main_menu (frontend *fe, cairo_t *cr)
{
  cairo_surface_t *surface;
  int w, h;

  ps3_menu_redraw (fe->menu);
  surface = ps3_menu_get_surface (fe->menu);
  w = cairo_image_surface_get_width (surface);
  h = cairo_image_surface_get_height (surface);

  cairo_set_source_surface (cr, surface, (fe->width - w) / 2,
      (fe->height - h) / 2);
  cairo_paint (cr);
  cairo_surface_destroy (surface);
}

static void
draw_types_menu (frontend *fe, cairo_t *cr)
{
  cairo_surface_t *surface;
  int w, h;

  ps3_menu_redraw (fe->menu);
  surface = ps3_menu_get_surface (fe->menu);
  w = cairo_image_surface_get_width (surface);
  h = cairo_image_surface_get_height (surface);

  cairo_set_source_surface (cr, surface, (fe->width - w) / 2,
      (fe->height - h) / 2);
  cairo_paint (cr);
  cairo_surface_destroy (surface);
}

static void
draw_puzzle_menu (frontend *fe, cairo_t *cr)
{
  cairo_surface_t *surface;
  cairo_font_extents_t fex;
  cairo_text_extents_t tex;
  const char *description1;
  const char *description2;
  int x, y, w, h;

  ps3_menu_redraw (fe->menu);
  surface = ps3_menu_get_surface (fe->menu);
  w = cairo_image_surface_get_width (surface);
  h = cairo_image_surface_get_height (surface);

  cairo_set_source_surface (cr, surface, (fe->width - w) / 2,
      (fe->height - PUZZLE_MENU_DESCRIPTION_HEIGHT - h) / 2);
  cairo_paint (cr);
  cairo_surface_destroy (surface);


  /* Draw game description */
  description1 = puzzle_descriptions[fe->menu->selection].description1;
  description2 = puzzle_descriptions[fe->menu->selection].description2;
  cairo_save(cr);
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);

  cairo_select_font_face(cr, "Arial",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_BOLD);

  cairo_set_font_size(cr, 20);

  cairo_font_extents (cr, &fex);

  y = fe->height - (PUZZLE_MENU_DESCRIPTION_HEIGHT / 2);
  /* Center the description */
  if (description2 == NULL)
    y += fex.ascent / 2;
  else
    y -= fex.descent;

  cairo_text_extents (cr, description1, &tex);
  x = ((fe->width - tex.width) / 2) - tex.x_bearing;
  cairo_move_to(cr, x, y);
  cairo_show_text (cr, description1);

  cairo_text_extents (cr, description2, &tex);
  x = ((fe->width - tex.width) / 2) - tex.x_bearing;
  cairo_move_to(cr, x, y + tex.height);
  cairo_show_text (cr, description2);

  cairo_restore(cr);
}
