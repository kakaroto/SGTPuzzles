/*
 * cairo-utils.h : Cairo utility functions
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 */

#ifndef __CAIRO_UTILS_H__
#define __CAIRO_UTILS_H__

#include <cairo/cairo.h>

/**
 * cairo_utils_get_surface_size:
 * @surface: The surface to get the size of
 * @width: Where to store the width, or #NULL
 * @height: Where to store the height, or #NULL
 *
 * This utility function will get the width and height of any surface
 * and return those values in @width and @height if set.
 * This is a better alternative to cairo_image_surface_get_width() and
 * cairo_image_surface_get_height() because it will work for any surface,
 * not just Image surfaces. This means that you can use it on surfaces of type
 * subsurface too.
 */
void cairo_utils_get_surface_size (cairo_surface_t *surface,
    int *width, int *height);

/**
 * cairo_utils_get_surface_width:
 * @surface: The surface to get the width of
 *
 * This utility function will get the width of any surface
 * and return its value.
 * This is a better alternative to cairo_image_surface_get_width()
 * because it will work for any surface, not just Image surfaces.
 * This means that you can use it on surfaces of type subsurface too.
 *
 * Returns: The width of the surface
 * See also: cairo_utils_get_surface_size()
 */
int cairo_utils_get_surface_width (cairo_surface_t *surface);

/**
 * cairo_utils_get_surface_height:
 * @surface: The surface to get the height of
 *
 * This utility function will get the height of any surface
 * and return its value.
 * This is a better alternative to cairo_image_surface_get_height()
 * because it will work for any surface, not just Image surfaces.
 * This means that you can use it on surfaces of type subsurface too.
 *
 * Returns: The height of the surface
 * See also: cairo_utils_get_surface_size()
 */
int cairo_utils_get_surface_height (cairo_surface_t *surface);

/**
 * cairo_utils_clip_round_edge:
 * @cr: The cairo context
 * @width: The width of the box
 * @height: The height of the box
 * @x: The horizontal distance for the arc
 * @y: The vertical distance for the arc
 * @rad: The radius of the arc
 *
 * This utility function will create a clip to the current cairo context in the form
 * of a box with rounded edges. This is useful for creating buttons or frames or
 * whatever looks better with rounded edges.
 *
 * See also: cairo_utils_path_round_edge()
 */
void cairo_utils_clip_round_edge (cairo_t *cr,
    int width, int height, int x, int y, int rad);

/**
 * cairo_utils_path_round_edge:
 * @cr: The cairo context
 * @width: The width of the box
 * @height: The height of the box
 * @x: The horizontal distance for the arc
 * @y: The vertical distance for the arc
 * @rad: The radius of the arc
 *
 * This utility function will create a path to the current cairo context in the form
 * of a box with rounded edges. This is useful for creating buttons or frames or
 * whatever looks better with rounded edges.
 */
void cairo_utils_path_round_edge (cairo_t *cr,
    int width, int height, int x, int y, int rad);

/**
 * cairo_utils_image_surface_blur:
 * @surface: The Image Surface to blur
 * @radius: The blur radius
 *
 * This function will blur a surface using a Gaussian blur method.
 * It only works on image surfaces, and it will also skip the first @radius pixels
 * on the four sides of the surface.
 *
 * <para>
 * This function was generously released to the public domain by Steve Hanov.
 * </para>
 */
void cairo_utils_image_surface_blur (cairo_surface_t* surface, double radius);

/**
 * cairo_utils_surface_add_dropshadow:
 * @surace: The surface to which to add the dropshadow
 * @radius: The radius of the blur and dropshadow
 *
 * This will create a shadow image of the surface, blur it, then add it as a
 * dropshadow to the existing image. It will then overlay the existing image on top
 * of the shadow and return it.
 *
 * Returns: A new surface containing the data from @surface with a dropshadow of
 * @radius pixels added to it.
 */
cairo_surface_t *cairo_utils_surface_add_dropshadow (cairo_surface_t *surface,
    int radius);

#endif /* __CAIRO_UTILS_H__ */
