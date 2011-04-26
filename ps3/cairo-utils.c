/*
 * cairo-utils.c : Cairo utility functions
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 */

#include "cairo-utils.h"
#include <math.h>
#include <stdlib.h>

void
cairo_utils_get_surface_size (cairo_surface_t *surface, int *width, int *height)
{
  cairo_t *cr;
  double x1, x2, y1, y2;

  cr = cairo_create (surface);
  cairo_clip_extents (cr, &x1, &y1, &x2, &y2);
  if (width)
    *width = (int) x2;
  if (height)
    *height = (int) y2;
  cairo_destroy (cr);
}

int
cairo_utils_get_surface_width (cairo_surface_t *surface)
{
  int width;

  cairo_utils_get_surface_size (surface, &width, NULL);

  return width;
}

int
cairo_utils_get_surface_height (cairo_surface_t *surface)
{
  int height;

  cairo_utils_get_surface_size (surface, NULL, &height);

  return height;
}

void
cairo_utils_clip_round_edge (cairo_t *cr,
    int width, int height, int x, int y, int rad)
{
  cairo_new_path (cr);
  cairo_arc (cr, x, y, rad, M_PI, -M_PI / 2);
  cairo_arc (cr, width - x, y, rad, -M_PI / 2, 0);
  cairo_arc (cr, width - x,  height - y, rad, 0, M_PI / 2);
  cairo_arc (cr, x, height - y, rad, M_PI / 2, M_PI);
  cairo_close_path (cr);
  cairo_clip (cr);
}
