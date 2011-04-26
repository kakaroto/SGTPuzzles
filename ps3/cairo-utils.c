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
#include <stdint.h>
#include <string.h>

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


void
cairo_utils_image_surface_blur (cairo_surface_t* surface, double radius)
{
  // Steve Hanov, 2009
  // Released into the public domain.

  // The number of times to perform the averaging. According to wikipedia,
  // three iterations is good enough to pass for a gaussian.
  const int MAX_ITERATIONS = 3;

  // get width, height
  int width = cairo_image_surface_get_width (surface);
  int height = cairo_image_surface_get_height (surface);
  uint8_t *dst = malloc(width * height * 4);
  uint32_t *precalc = malloc(width * height * sizeof(uint32_t));
  uint8_t *src = cairo_image_surface_get_data (surface);
  double mul = 1.0f / ((radius * 2) * (radius * 2));
  int channel;
  int iteration;

  memcpy (dst, src, width * height * 4);

  for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
    for(channel = 0; channel < 4; channel++) {
      int x,y;

      // precomputation step.
      uint8_t *pix = src;
      uint32_t *pre = precalc;

      pix += channel;
      for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
          int tot = pix[0];

          if (x > 0)
            tot += pre[-1];
          if (y > 0)
            tot += pre[-width];
          if (x > 0 && y > 0)
            tot -= pre[-width - 1];
          pre[0] = tot;
          pre++;
          pix += 4;
        }
      }

      // blur step.
      pix = dst + (int)radius * width * 4 + (int)radius * 4 + channel;
      for (y = radius; y < height - radius; y++) {
        for (x = radius; x < width - radius; x++) {
          int l = x < radius ? 0 : x - radius;
          int t = y < radius ? 0 : y - radius;
          int r = x + radius >= width ? width - 1 : x + radius;
          int b = y + radius >= height ? height - 1 : y + radius;
          int tot = precalc[r+b*width] + precalc[l+t*width] -
              precalc[l+b*width] - precalc[r+t*width];
          *pix=(uint32_t)(tot*mul);
          pix += 4;
        }
        pix += (int)radius * 2 * 4;
      }
    }
    memcpy (src, dst, width * height * 4);
  }

  free (dst);
  free (precalc);
}


cairo_surface_t *
cairo_utils_surface_add_dropshadow (cairo_surface_t *surface, int radius)
{
  cairo_surface_t *shadow;
  cairo_surface_t *result;
  cairo_t *cr;
  int width, height;

  /* The image_surface_blue will ignore radius pixels on each sides, so
   * if we want to blur the whole thing we need to move it by 2*radius inside
   * the surface.. but since the blur will 'overflow' outside the border, with a
   * maximum of 2*radius pixels on each side, so to hold the surface once blured
   * we need 6*radius added to width and height, then place the surface at
   * an offset of 3*radius on x and y (to skip the ignored radius border, and
   * to give enough space for the 2*radius blurring overflow), then we blur the
   * surface using the supplied radius.
   */
  cairo_utils_get_surface_size (surface, &width, &height);

  shadow = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
      width + (radius * 6), height + (radius * 6));
  cr = cairo_create (shadow);
  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_mask_surface (cr, surface, radius * 3, radius * 3);
  cairo_destroy (cr);

  cairo_utils_image_surface_blur (shadow, radius);

  /* We blur by radius pixels and put the dropshadow there, but the blur will
   * overflow so we make the new surface have radius*2 more pixels on each side
   * Then we place the shadow at position (2*radius,2*radius) after moving
   * the previous surface back to (0,0).
   */
  result = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
      width + (radius * 4), height + (radius * 4));
  cr = cairo_create (result);
  cairo_set_source_surface (cr, shadow, -radius, -radius);
  cairo_paint (cr);
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);
  cairo_surface_destroy (shadow);

  return result;
}
