/*
 * ps3.c : PS3 Puzzles homebrew main application code
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 */

#include <cairo/cairo.h>


typedef struct {
  cairo_surface_t *image;
  char *text;
  int id;
} Ps3MenuItem;

typedef struct {
  int width;
  int height;
  int nitems;
  Ps3MenuItem *items;
  int selection;
  cairo_surface_t *bg_surface;
  cairo_surface_t *selected_surface;
} Ps3Menu;

Ps3Menu *ps3_menu_new (int width, int height);
void ps3_menu_add_item (Ps3Menu *menu, cairo_surface_t *image,
    const char *text, int id);
int ps3_menu_change_selection (Ps3Menu *menu, int index);
int ps3_menu_set_selection (Ps3Menu *menu, int id);
int ps3_menu_get_selection (Ps3Menu *menu, int *id);
int ps3_menu_draw (Ps3Menu *menu, cairo_t *cr, int x, int y);
void ps3_menu_free (Ps3Menu *menu);
