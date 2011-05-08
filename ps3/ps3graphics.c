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
#include "cairo-utils.h"
#include <string.h>

static void draw_background (frontend *fe, cairo_t *cr);
static void draw_puzzle (frontend *fe, cairo_t *cr);
static void draw_pointer (frontend *fe, cairo_t *cr);
static void draw_status_bar (frontend *fe, cairo_t *cr);
static void draw_help (frontend *fe, cairo_t *cr);

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

  if (fe->image != NULL) {
    draw_background (fe, cr);
    draw_puzzle (fe, cr);
    draw_status_bar (fe, cr);
  } else {
    fe->puzzles_menu.draw (fe, cr);
  }

  if (fe->menu.menu != NULL)
    fe->menu.draw (fe, cr);
  else if (fe->cursor_last_move == FALSE)
    draw_pointer (fe, cr);

  if (fe->help != NULL)
    draw_help(fe, cr);

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
draw_puzzles_menu (frontend *fe, cairo_t *cr)
{
  cairo_surface_t *surface;
  cairo_font_extents_t fex;
  cairo_text_extents_t tex;
  const char *description1;
  const char *description2;
  int selection = fe->puzzles_menu.menu->selection;
  int x, y, w, h;

  ps3_menu_redraw (fe->puzzles_menu.menu);
  surface = ps3_menu_get_surface (fe->puzzles_menu.menu);
  cairo_utils_get_surface_size (surface, &w, &h);

  cairo_set_source_surface (cr, fe->puzzles_menu.frame, 0, 0);
  cairo_paint (cr);

  cairo_set_source_surface (cr, surface, (fe->width - w) / 2,
      ((fe->height - PUZZLE_MENU_DESCRIPTION_HEIGHT - h) / 2) +
      PUZZLE_MENU_FRAME_TOP);
  cairo_paint (cr);

  cairo_surface_destroy (surface);


  /* Draw game description */
  description1 = puzzle_descriptions[selection].description1;
  description2 = puzzle_descriptions[selection].description2;
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
create_standard_menu_frame (frontend *fe)
{
  cairo_font_extents_t fex;
  cairo_text_extents_t tex;
  cairo_pattern_t *linpat = NULL;
  cairo_surface_t *frame = NULL;
  int width, height;
  cairo_t *cr;
  int x, y;

  printf ("Creating %s frame\n", fe->menu.title);

  /* Adapt frame height depending on items in the menu */
  width = STANDARD_MENU_FRAME_WIDTH;
  height = fe->menu.menu->nitems * STANDARD_MENU_ITEM_TOTAL_HEIGHT;
  if (height > STANDARD_MENU_HEIGHT)
    height = STANDARD_MENU_HEIGHT;
  height += STANDARD_MENU_FRAME_HEIGHT;

  frame = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
      width, height);
  cr = cairo_create (frame);
  cairo_utils_clip_round_edge (cr, width, height,
      STANDARD_MENU_FRAME_CORNER_RADIUS, STANDARD_MENU_FRAME_CORNER_RADIUS,
      STANDARD_MENU_FRAME_CORNER_RADIUS);
  linpat = cairo_pattern_create_linear (width, 0, width, height);

  cairo_pattern_add_color_stop_rgb (linpat, 0.0, 0.3, 0.3, 0.3);
  cairo_pattern_add_color_stop_rgb (linpat, 1.0, 0.8, 0.8, 0.8);

  cairo_set_source (cr, linpat);
  cairo_paint (cr);
  cairo_pattern_destroy (linpat);

  linpat = cairo_pattern_create_linear (width, 0, width, height);

  cairo_pattern_add_color_stop_rgb (linpat, 0.0, 0.03, 0.07, 0.10);
  cairo_pattern_add_color_stop_rgb (linpat, 0.1, 0.04, 0.09, 0.16);
  cairo_pattern_add_color_stop_rgb (linpat, 0.5, 0.05, 0.20, 0.35);
  cairo_pattern_add_color_stop_rgb (linpat, 1.0, 0.06, 0.55, 0.75);

  cairo_utils_clip_round_edge (cr, width, height,
      STANDARD_MENU_FRAME_CORNER_RADIUS + STANDARD_MENU_FRAME_BORDER_WIDTH,
      STANDARD_MENU_FRAME_CORNER_RADIUS + STANDARD_MENU_FRAME_BORDER_WIDTH,
      STANDARD_MENU_FRAME_CORNER_RADIUS);

  cairo_set_source (cr, linpat);
  cairo_paint (cr);
  cairo_pattern_destroy (linpat);

  cairo_select_font_face(cr, "Arial",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_BOLD);

  cairo_set_font_size(cr, STANDARD_MENU_TITLE_FONT_SIZE);

  cairo_font_extents (cr, &fex);
  cairo_text_extents (cr, fe->menu.title, &tex);

  y = (STANDARD_MENU_FRAME_TOP / 2) + (fex.ascent / 2);
  x = ((width - tex.width) / 2) - tex.x_bearing;
  cairo_move_to(cr, x, y);
  cairo_set_source_rgb (cr, 0.0, 0.3, 0.5);
  cairo_show_text (cr, fe->menu.title);
  cairo_destroy (cr);

  /* Create the frame with a dropshadow */
  fe->menu.frame = cairo_utils_surface_add_dropshadow (frame, 3);
  cairo_surface_destroy (frame);
}

static void
draw_standard_menu (frontend *fe, cairo_t *cr)
{
  cairo_surface_t *surface;
  int w, h;
  int menu_width;
  int menu_height;

  if (fe->menu.frame == NULL) {
    create_standard_menu_frame (fe);
  }

  cairo_utils_get_surface_size (fe->menu.frame, &w, &h);
  surface = ps3_menu_get_surface (fe->menu.menu);

  ps3_menu_redraw (fe->menu.menu);

  cairo_set_source_surface (cr, fe->menu.frame, (fe->width - w) / 2,
      (fe->height - h) / 2);
  cairo_paint (cr);

  /* Draw a frame around the menu so cut off buttons don't appear clippped */

  menu_width = STANDARD_MENU_WIDTH;
  menu_height = fe->menu.menu->nitems * STANDARD_MENU_ITEM_TOTAL_HEIGHT;

  if (menu_height > STANDARD_MENU_HEIGHT)
    menu_height = STANDARD_MENU_HEIGHT;

  cairo_save (cr);
  cairo_translate (cr, ((fe->width - w) / 2) + STANDARD_MENU_FRAME_SIDE,
      ((fe->height - h) / 2) + STANDARD_MENU_FRAME_TOP);
  cairo_utils_clip_round_edge (cr, menu_width, menu_height, 20, 20, 20);
  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_paint_with_alpha (cr, 0.5);

  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_paint (cr);
  cairo_restore (cr);

  cairo_surface_destroy (surface);
}

static void
draw_help (frontend *fe, cairo_t *cr)
{
  double x;
  double y;
  double width = fe->width * 0.8;
  double height = fe->height * 0.8;
  char **line;
  int cnt = fe->help->start_line;


  cairo_save (cr);
  cairo_translate (cr, (fe->width - width) / 2, (fe->height - height) / 2);

  cairo_set_source_surface (cr, fe->help->background, 0, 0);
  cairo_paint (cr);

  cairo_utils_clip_round_edge (cr, width, height, 30, 30, 20);
  x = 10;
  y = 10;

  line = fe->help->lines;
  while (*line != NULL && y < height) {
    if (cnt > 0) {
      cnt--;
      line++;
      continue;
    }

    /* Drawing text line */
    cairo_save (cr);

    cairo_select_font_face (cr,
        "monospace",
        CAIRO_FONT_SLANT_NORMAL,
        CAIRO_FONT_WEIGHT_BOLD);

    cairo_set_font_size (cr, 15);

    cairo_set_source_rgb (cr, 1, 1, 1);
    cairo_move_to (cr, x, y + 20);
    cairo_show_text (cr, *line);
    cairo_restore (cr);
    y += 20;
    line++;
  }

  cairo_restore (cr);

}


static cairo_surface_t *
create_standard_background (float r, float g, float b) {
  cairo_surface_t *bg;
  cairo_surface_t *background;
  cairo_t *cr;
  int width, height;
  cairo_pattern_t *linpat = NULL;

  width = STANDARD_MENU_ITEM_BOX_WIDTH;
  height = STANDARD_MENU_ITEM_BOX_HEIGHT;
  background = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32, width, height);

  bg = ps3_menu_create_default_background (STANDARD_MENU_ITEM_WIDTH,
      STANDARD_MENU_ITEM_HEIGHT, r, g, b);

  cr = cairo_create (background);
  cairo_utils_clip_round_edge (cr, width, height,
      STANDARD_MENU_BOX_CORNER_RADIUS, STANDARD_MENU_BOX_CORNER_RADIUS,
      STANDARD_MENU_BOX_CORNER_RADIUS);

  linpat = cairo_pattern_create_linear (width, 0, width, height);

  cairo_pattern_add_color_stop_rgb (linpat, 0.0, 0.3, 0.3, 0.3);
  cairo_pattern_add_color_stop_rgb (linpat, 1.0, 0.7, 0.7, 0.7);

  cairo_set_source (cr, linpat);
  cairo_paint_with_alpha (cr, 0.5);

  cairo_utils_clip_round_edge (cr, width, height,
      STANDARD_MENU_BOX_CORNER_RADIUS + STANDARD_MENU_BOX_BORDER_WIDTH,
      STANDARD_MENU_BOX_CORNER_RADIUS + STANDARD_MENU_BOX_BORDER_WIDTH,
      STANDARD_MENU_BOX_CORNER_RADIUS);

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_paint_with_alpha (cr, 0.4);

  cairo_set_source_surface (cr, bg, STANDARD_MENU_BOX_X, STANDARD_MENU_BOX_Y);
  cairo_paint (cr);
  cairo_destroy (cr);
  cairo_pattern_destroy (linpat);
  cairo_surface_destroy (bg);

  return background;
}

static void
standard_menu_create (frontend *fe, const char *title)
{
  cairo_surface_t *surface;
  cairo_surface_t *background, *selected_background, *disabled;
  cairo_t *cr;

  fe->menu.draw = draw_standard_menu;
  fe->menu.title = title;

  background = create_standard_background (0, 0, 0);
  selected_background = create_standard_background (0.05, 0.30, 0.60);
  disabled = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
      STANDARD_MENU_ITEM_BOX_WIDTH, STANDARD_MENU_ITEM_BOX_HEIGHT);

  cr = cairo_create (disabled);

  cairo_set_source_rgba (cr, 0.7, 0.7, 0.7, 0.7);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_utils_clip_round_edge (cr,
      STANDARD_MENU_ITEM_BOX_WIDTH, STANDARD_MENU_ITEM_BOX_HEIGHT,
      STANDARD_MENU_BOX_X + 7, STANDARD_MENU_BOX_Y + 7, 7);
  cairo_paint (cr);

  cairo_destroy (cr);
  cairo_surface_flush (disabled);

  surface = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
      STANDARD_MENU_WIDTH, STANDARD_MENU_HEIGHT);
  /* Infinite vertical scrollable menu */
  fe->menu.menu = ps3_menu_new_full (surface, -1, 1,
      STANDARD_MENU_ITEM_BOX_WIDTH, STANDARD_MENU_ITEM_BOX_HEIGHT,
      STANDARD_MENU_PAD_X, STANDARD_MENU_PAD_Y, 0,
      background, selected_background, disabled);
  cairo_surface_destroy (surface);
  cairo_surface_destroy (background);
  cairo_surface_destroy (selected_background);
  cairo_surface_destroy (disabled);
}

static void
standard_menu_add_item (frontend *fe, const char *title, int fontsize)
{
  int idx;

  idx = ps3_menu_add_item (fe->menu.menu, title, fontsize);
  fe->menu.menu->items[idx].ipad_x = STANDARD_MENU_ITEM_IPAD_X;
  fe->menu.menu->items[idx].ipad_y = STANDARD_MENU_ITEM_IPAD_Y;
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
  fe->redraw = TRUE;
}


void
free_help (frontend *fe)
{
  if (fe->help != NULL) {
    if (fe->help->lines[0] != NULL)
      free (fe->help->lines[0]);
    free (fe->help->lines);

    cairo_surface_destroy (fe->help->background);
    free (fe->help);
    fe->help = NULL;
  }
}

static void
_help_game (frontend *fe)
{
 char filename[255];
 char *text = NULL;
 char *ptr;
 FILE* fd = NULL;
 int filesize;
 int lines;
 double width = fe->width * 0.8;
 double height = fe->height * 0.8;
 cairo_pattern_t *linpat = NULL;
 cairo_t *cr;

 free_help (fe);

 /* Read help file text */
 snprintf (filename, 255, "%s/data/help/%s.txt",
     cwd, gamelist_names[fe->game_idx]);

 fd = fopen(filename,"rb");
 if (fd != NULL) {
   fe->help = (PuzzleHelp*) malloc (sizeof(PuzzleHelp));
   fe->help->start_line = 0;

   fseek(fd, 0, SEEK_END);
   filesize = ftell(fd);
   fseek(fd, 0, SEEK_SET);

   text = (char*) malloc(filesize);
   fread(text, 1, filesize, fd);

   fclose(fd);

   lines = 0;
   ptr = text;
   while (*ptr != 0) {
     if (*ptr++ == '\n')
       lines++;
   }
   /* Last line */
   lines++;

   fe->help->lines = malloc ((lines + 1) * sizeof(char *));
   fe->help->nlines = 0;
   ptr = text;
   while (*ptr != 0) {
     fe->help->lines[fe->help->nlines++] = ptr;

     while (*ptr != 0 && *ptr != '\r' && *ptr != '\n')
       ptr++;
     if (*ptr == '\r')
       *ptr++ = 0;
     if (*ptr == '\n')
       *ptr++ = 0;
   }
   fe->help->lines[fe->help->nlines] = NULL;

   /* Setup background */
   fe->help->background = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
       width, height);
   cr = cairo_create (fe->help->background);

   linpat = cairo_pattern_create_linear (width, 0, width, height);

   cairo_pattern_add_color_stop_rgb (linpat, 0.0, 0.3, 0.3, 0.3);
   cairo_pattern_add_color_stop_rgb (linpat, 1.0, 0.8, 0.8, 0.8);

   cairo_utils_clip_round_edge (cr, width, height, 20, 20, 20);
   cairo_set_source (cr, linpat);
   cairo_paint (cr);
   cairo_pattern_destroy (linpat);

   linpat = cairo_pattern_create_linear (width, 0, width, height);

   cairo_pattern_add_color_stop_rgb (linpat, 0.0, 0.03, 0.07, 0.10);
   cairo_pattern_add_color_stop_rgb (linpat, 0.1, 0.04, 0.09, 0.16);
   cairo_pattern_add_color_stop_rgb (linpat, 0.5, 0.05, 0.20, 0.35);
   cairo_pattern_add_color_stop_rgb (linpat, 1.0, 0.06, 0.55, 0.75);

   cairo_utils_clip_round_edge (cr, width, height, 22, 22, 20);
   cairo_set_source (cr, linpat);
   cairo_paint (cr);
   cairo_pattern_destroy (linpat);
   cairo_destroy (cr);
 }
 fe->redraw = TRUE;
}

const struct {
  const char *title;
  void (*callback) (frontend *fe);
} main_menu_items[] = {
  {"New Game", _new_game},
  {"Restart Game", _restart_game},
  {"How To Play",_help_game},
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
  int i;

  fe->menu.callback = main_menu_callback;

  /* Infinite vertical scrollable menu */
  standard_menu_create (fe, "Main Menu");

  for (i = 0; main_menu_items[i].title; i++) {
    standard_menu_add_item (fe, main_menu_items[i].title, MAIN_MENU_FONT_SIZE);

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
  int n;
  int i;

  fe->menu.callback = types_menu_callback;

  standard_menu_create (fe, "Presets Menu");
  n = midend_num_presets(fe->me);

  if(n <= 0){ /* No types */
    free_sgt_menu (fe);
    return;
  } else{
    for(i = 0; i < n; i++){
      char* name;
      game_params *params;

      midend_fetch_preset(fe->me, i, &name, &params);
      standard_menu_add_item (fe, name, TYPES_MENU_FONT_SIZE);
    }
  }
}

static void
puzzles_menu_callback (frontend *fe, int accepted)
{
  int selected_item = fe->puzzles_menu.menu->selection;

  /* Don't allow cancelling the puzzles menu */
  if (accepted == FALSE)
    return;

  /* Game selected */
  create_midend (fe, selected_item);
}

void
create_puzzles_menu (frontend * fe) {
  cairo_pattern_t *linpat = NULL;
  cairo_surface_t *surface;
  cairo_font_extents_t fex;
  cairo_text_extents_t tex;
  int width, height;
  cairo_t *cr;
  int x, y;
  int i ;

  fe->puzzles_menu.callback = puzzles_menu_callback;
  fe->puzzles_menu.draw = draw_puzzles_menu;
  fe->puzzles_menu.title = "Choose Puzzle";

  width = fe->width * 0.9;
  height = (fe->height * 0.9) -
      PUZZLE_MENU_DESCRIPTION_HEIGHT - PUZZLE_MENU_FRAME_TOP;
  surface = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32, width, height);

  /* Infinite vertical scrollable menu */
  fe->puzzles_menu.menu = ps3_menu_new_full (surface, -1, 4,
      (width / 4) - (2 * 20), 150, 20, 5, 3, NULL, NULL, NULL);
  cairo_surface_destroy (surface);

  for (i = 0; i < gamecount; i++) {
    char filename[256];

    ps3_menu_add_item (fe->puzzles_menu.menu, gamelist[i]->name, 20);
    snprintf (filename, 255, "%s/data/puzzles/%s.png", cwd, gamelist_names[i]);
    surface = cairo_image_surface_create_from_png (filename);
    if (surface) {
      fe->puzzles_menu.menu->items[i].alignment = PS3_MENU_ALIGN_BOTTOM_CENTER;
      ps3_menu_set_item_image (fe->puzzles_menu.menu, i, surface,
          PS3_MENU_IMAGE_POSITION_TOP);
      cairo_surface_destroy (surface);
    }
  }


  /* Adapt frame height depending on items in the menu */
  fe->puzzles_menu.frame = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
      fe->width, fe->height);
  cr = cairo_create (fe->puzzles_menu.frame);
  linpat = cairo_pattern_create_linear (width, 0, width, height);

  cairo_pattern_add_color_stop_rgb (linpat, 0.0, 0.03, 0.07, 0.10);
  cairo_pattern_add_color_stop_rgb (linpat, 0.1, 0.04, 0.09, 0.16);
  cairo_pattern_add_color_stop_rgb (linpat, 0.5, 0.05, 0.20, 0.35);
  cairo_pattern_add_color_stop_rgb (linpat, 1.0, 0.06, 0.55, 0.75);

  cairo_set_source (cr, linpat);
  cairo_paint (cr);
  cairo_pattern_destroy (linpat);

  /* Draw a frame around the menu so cut off buttons don't appear clippped */
  cairo_save (cr);
  cairo_translate (cr, (fe->width - width) / 2,
      ((fe->height - PUZZLE_MENU_DESCRIPTION_HEIGHT - height) / 2) +
      PUZZLE_MENU_FRAME_TOP);
  cairo_utils_clip_round_edge (cr, width, height, 20, 20, 20);
  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_paint_with_alpha (cr, 0.5);
  cairo_restore (cr);

  cairo_select_font_face(cr, "Arial",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_BOLD);

  cairo_set_font_size(cr, 25);

  cairo_font_extents (cr, &fex);
  cairo_text_extents (cr, fe->puzzles_menu.title, &tex);

  y = (STANDARD_MENU_FRAME_TOP / 2) + (fex.ascent / 2);
  x = ((fe->width - tex.width) / 2) - tex.x_bearing;
  cairo_move_to(cr, x, y);
  cairo_set_source_rgb (cr, 0.0, 0.5, 0.8);
  cairo_show_text (cr, fe->puzzles_menu.title);
  cairo_destroy (cr);

  cairo_surface_flush (fe->puzzles_menu.frame);
}
