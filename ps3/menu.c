/*
 * ps3.c : PS3 Puzzles homebrew main application code
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 */

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <cairo/cairo.h>

#include "menu.h"

Ps3Menu *
ps3_menu_new (int width, int height)
{
  Ps3Menu *menu = malloc (sizeof(Ps3Menu));

  memset (menu, 0, sizeof(Ps3Menu));
  menu->width = width;
  menu->height = height;
  menu->selection = -1;

  return menu;
}

void
ps3_menu_add_item (Ps3Menu *menu, cairo_surface_t *image,
    const char *text, int id)
{
  menu->nitems++;
  menu->items = realloc (menu->items, menu->nitems * sizeof(Ps3MenuItem));
  memset (&menu->items[menu->nitems - 1], 0, sizeof(Ps3MenuItem));
  menu->items[menu->nitems - 1].image = image;
  menu->items[menu->nitems - 1].text = strdup (text);
  menu->items[menu->nitems - 1].id = id;
}

int
ps3_menu_change_selection (Ps3Menu *menu, int index)
{
  int old_sel = menu->selection;

  menu->selection = index;

  return old_sel;
}

int
ps3_menu_set_selection (Ps3Menu *menu, int id)
{
  int i = -1;

  for (i = 0; i < menu->nitems; i++) {
    if (menu->items[i].id == id) {
      menu->selection = i;
      break;
    }
  }

  return (i < menu->nitems);
}

int
ps3_menu_get_selection (Ps3Menu *menu, int *id)
{
  if (id != NULL && menu->selection != -1 && menu->selection < menu->nitems)
    *id = menu->items[menu->selection].id;

  return menu->selection;
}


static void
draw_text (cairo_t *cr, const char *text, int x, int y)
{
  cairo_font_extents_t fex;

  cairo_save (cr);
  cairo_select_font_face(cr,
      "sans-serif",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_BOLD);

  cairo_set_font_size(cr, 20);

  cairo_font_extents (cr, &fex);

  y += fex.ascent + fex.descent;

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_move_to(cr, x, y);
  cairo_show_text (cr, text);
  cairo_restore (cr);
}

static void
ps3_menu_draw_item (Ps3Menu *menu, Ps3MenuItem *item, int index,
    cairo_t *cr, int x, int y)
{
  cairo_surface_t *bg;

  if (menu->bg_surface == NULL) {
    cairo_pattern_t *linpat = NULL;
    cairo_t *grad_cr = NULL;

    menu->bg_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
        menu->width, menu->height);

    linpat = cairo_pattern_create_linear (menu->width, 0,
        menu->width, menu->height);
    cairo_pattern_add_color_stop_rgb (linpat, 0.0, 0.0, 0.0, 0.0);
    cairo_pattern_add_color_stop_rgb (linpat, 0.3, 0.4, 0.4, 0.4);
    cairo_pattern_add_color_stop_rgb (linpat, 0.7, 0.3, 0.3, 0.3);
    cairo_pattern_add_color_stop_rgb (linpat, 1.0, 0.0, 0.0, 0.0);

    grad_cr = cairo_create (menu->bg_surface);
    cairo_set_source (grad_cr, linpat);
    cairo_paint (grad_cr);
    cairo_destroy (grad_cr);
    //cairo_pattern_destroy (linpat);
  }

  if (menu->selected_surface == NULL) {
    cairo_pattern_t *linpat = NULL;
    cairo_t *grad_cr = NULL;

    menu->selected_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
        menu->width, menu->height);

    linpat = cairo_pattern_create_linear (menu->width, 0,
        menu->width, menu->height);
    cairo_pattern_add_color_stop_rgb (linpat, 0.0, 0.0, 0.8, 0.3);
    cairo_pattern_add_color_stop_rgb (linpat, 0.3, 0.0, 0.6, 0.3);
    cairo_pattern_add_color_stop_rgb (linpat, 0.7, 0.0, 0.7, 0.3);
    cairo_pattern_add_color_stop_rgb (linpat, 1.0, 0.0, 0.8, 0.3);

    grad_cr = cairo_create (menu->selected_surface);
    cairo_set_source (grad_cr, linpat);
    cairo_paint (grad_cr);
    cairo_destroy (grad_cr);
    cairo_pattern_destroy (linpat);
  }

  cairo_save (cr);
  cairo_rectangle (cr, x, y, menu->width - 3, menu->height - 3);
  cairo_clip (cr);
  if (menu->selection == index)
    bg = menu->selected_surface;
  else
    bg = menu->bg_surface;
  cairo_set_source_surface (cr, bg, x, y);
  cairo_paint (cr);
  draw_text (cr, item->text, x, y);
  cairo_restore (cr);
}

int
ps3_menu_draw (Ps3Menu *menu, cairo_t *cr, int x, int y)
{
  int i;

  for (i = 0; i < menu->nitems; i++) {
    ps3_menu_draw_item (menu, &menu->items[i], i, cr, x, y);
    y += menu->height;
  }

  return i;
}

void
ps3_menu_free (Ps3Menu *menu)
{
  int i;

  if (menu->items) {
    for (i = 0; i < menu->nitems; i++) {
      free (menu->items[i].text);
    }
    free (menu->items);
  }
  free (menu);
}
