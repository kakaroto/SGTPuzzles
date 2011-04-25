/*
 * ps3graphics.c : PS3 Puzzles Main graphics engine
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 */

#include "ps3graphics.h"
#include "ps3save.h"

static void draw_background (frontend *fe, cairo_t *cr);
static void draw_puzzle (frontend *fe, cairo_t *cr);
static void draw_pointer (frontend *fe, cairo_t *cr);
static void draw_status_bar (frontend *fe, cairo_t *cr);
static void draw_main_menu (frontend *fe, cairo_t *cr);
static void draw_puzzles_menu (frontend *fe, cairo_t *cr);
static void draw_types_menu (frontend *fe, cairo_t *cr);

void
ps3_redraw_screen (frontend *fe)
{
  rsxBuffer *buffer = &fe->buffers[fe->currentBuffer];
  cairo_surface_t *surface;
  cairo_t *cr;

  setRenderTarget(fe->context, buffer);
  /* Wait for the last flip to finish, so we can draw to the old buffer */
  waitFlip ();

  /* Draw our window */
  surface = cairo_image_surface_create_for_data ((u8 *) buffer->ptr,
      CAIRO_FORMAT_ARGB32, buffer->width, buffer->height, buffer->width * 4);

  cr = cairo_create (surface);

  draw_background (fe, cr);
  if (fe->image != NULL) {
    draw_puzzle (fe, cr);
    draw_status_bar (fe, cr);
  }
  if (fe->menu.menu)
    fe->menu.draw (fe, cr);
  else if (fe->cursor_last_move == FALSE)
    draw_pointer (fe, cr);

  cairo_destroy (cr);

  cairo_surface_finish (surface);
  cairo_surface_destroy (surface);

  /* Flip buffer onto screen */
  flipBuffer (fe->context, fe->currentBuffer);
  fe->currentBuffer++;
  if (fe->currentBuffer >= MAX_BUFFERS)
    fe->currentBuffer = 0;
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

  ps3_menu_redraw (fe->menu.menu);
  surface = ps3_menu_get_surface (fe->menu.menu);
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

  ps3_menu_redraw (fe->menu.menu);
  surface = ps3_menu_get_surface (fe->menu.menu);
  w = cairo_image_surface_get_width (surface);
  h = cairo_image_surface_get_height (surface);

  cairo_set_source_surface (cr, surface, (fe->width - w) / 2,
      (fe->height - h) / 2);
  cairo_paint (cr);
  cairo_surface_destroy (surface);
}

static void
draw_puzzles_menu (frontend *fe, cairo_t *cr)
{
  cairo_surface_t *surface;
  cairo_font_extents_t fex;
  cairo_text_extents_t tex;
  const char *description1;
  const char *description2;
  int x, y, w, h;

  ps3_menu_redraw (fe->menu.menu);
  surface = ps3_menu_get_surface (fe->menu.menu);
  w = cairo_image_surface_get_width (surface);
  h = cairo_image_surface_get_height (surface);

  cairo_set_source_surface (cr, surface, (fe->width - w) / 2,
      (fe->height - PUZZLE_MENU_DESCRIPTION_HEIGHT - h) / 2);
  cairo_paint (cr);
  cairo_surface_destroy (surface);


  /* Draw game description */
  description1 = puzzle_descriptions[fe->menu.menu->selection].description1;
  description2 = puzzle_descriptions[fe->menu.menu->selection].description2;
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

void
free_sgt_menu (frontend *fe)
{
  if (fe->menu.menu)
    ps3_menu_free (fe->menu.menu);
  fe->menu.menu = NULL;
  fe->menu.callback = NULL;
  fe->menu.draw = NULL;
  fe->menu.title = NULL;
  if (fe->menu.frame)
    cairo_surface_destroy (fe->menu.frame);
  fe->menu.frame = NULL;
}

static void
_new_game (frontend *fe)
{
  midend_new_game (fe->me);
  midend_force_redraw(fe->me);
}

static void
_restart_game (frontend *fe)
{
  midend_restart_game (fe->me);
  midend_force_redraw(fe->me);
}

static void
_save_game (frontend *fe)
{
  if (ps3_save_game (fe) == FALSE)
    fe->redraw = TRUE;
}

static void
_load_game (frontend *fe)
{
  if (ps3_load_game (fe) == FALSE)
    fe->redraw = TRUE;
}

static void
_solve_game (frontend *fe)
{
  midend_solve (fe->me);
  midend_force_redraw(fe->me);
}

static void
_change_game (frontend *fe)
{
  destroy_midend (fe);
  create_puzzles_menu (fe);
  fe->redraw = TRUE;
}


const struct {
  const char *title;
  void (*callback) (frontend *fe);
} main_menu_items[] = {
  {"New Game", _new_game},
  {"Restart Game", _restart_game},
  {"Save Game", _save_game},
  {"Load Game", _load_game},
  {"Solve", _solve_game},
  {"Change Puzzle", _change_game},
  {NULL, NULL},
};

static void
main_menu_callback (frontend *fe, int accepted)
{
  int selected_item = fe->menu.menu->selection;

  /* Destroy the menu first since the callback could create a new menu */
  free_sgt_menu (fe);

  if (accepted)
    main_menu_items[selected_item].callback (fe);
}

void
create_main_menu (frontend * fe) {
  cairo_surface_t *surface;
  int i;

  fe->menu.callback = main_menu_callback;
  fe->menu.draw = draw_main_menu;
  surface = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
      306, fe->height * 0.7);

  /* Infinite vertical scrollable menu */
  fe->menu.menu = ps3_menu_new (surface, -1, 1, 300, 40);
  for (i = 0; main_menu_items[i].title; i++) {
    ps3_menu_add_item (fe->menu.menu, main_menu_items[i].title, 25);

    if (main_menu_items[i].callback == _solve_game)
      fe->menu.menu->items[i].enabled = fe->thegame->can_solve;
  }
}

static void
types_menu_callback (frontend *fe, int accepted)
{
  int selected_item = fe->menu.menu->selection;

  free_sgt_menu (fe);

  if (accepted) {
    /* Type selected */
    game_params *params;
    char* name;

    midend_fetch_preset(fe->me, selected_item, &name, &params);
    midend_set_params (fe->me,params);
    midend_new_game(fe->me);

    calculate_puzzle_size (fe);

    midend_force_redraw(fe->me);
  }
}


void
create_types_menu (frontend * fe){
  cairo_surface_t *surface;
  int n;
  int i;

  fe->menu.callback = types_menu_callback;
  fe->menu.draw = draw_types_menu;
  surface = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
      306, fe->height * 0.7);

  fe->menu.menu = ps3_menu_new (surface, -1, 1, 300, 40);
  n = midend_num_presets(fe->me);

  if(n <= 0){ /* No types */
    free_sgt_menu (fe);
    return;
  } else{
    for(i = 0; i < n; i++){
      char* name;
      game_params *params;
      midend_fetch_preset(fe->me, i, &name, &params);
      ps3_menu_add_item (fe->menu.menu, name, 25);
    }
  }
}

static void
puzzles_menu_callback (frontend *fe, int accepted)
{
  int selected_item = fe->menu.menu->selection;

  /* Don't allow cancelling the puzzles menu */
  if (accepted == FALSE)
    return;

  free_sgt_menu (fe);

  /* Game selected */
  create_midend (fe, selected_item);
}

void
create_puzzles_menu (frontend * fe) {
  cairo_surface_t *surface;
  int width, height;
  int i ;

  fe->menu.callback = puzzles_menu_callback;
  fe->menu.draw = draw_puzzles_menu;
  width = fe->width * 0.9;
  height = (fe->height * 0.9) - PUZZLE_MENU_DESCRIPTION_HEIGHT;
  surface = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32, width, height);

  /* Infinite vertical scrollable menu */
  fe->menu.menu = ps3_menu_new_full (surface, -1, 4,
      (width / 4) - (2 * 20), 150, 20, 5, NULL, NULL, NULL);
  cairo_surface_destroy (surface);

  for (i = 0; i < gamecount; i++) {
    char filename[256];

    ps3_menu_add_item (fe->menu.menu, gamelist[i]->name, 20);
    snprintf (filename, 255, "%s/data/puzzles/%s.png", cwd, gamelist_names[i]);
    surface = cairo_image_surface_create_from_png (filename);
    if (surface) {
      fe->menu.menu->items[i].alignment = PS3_MENU_ALIGN_BOTTOM_CENTER;
      ps3_menu_set_item_image (fe->menu.menu, i, surface,
          PS3_MENU_IMAGE_POSITION_TOP);
      cairo_surface_destroy (surface);
    }
  }
}
