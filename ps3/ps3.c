/*
 * ps3.c : PS3 Puzzles homebrew main application code
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 * Copyright (C) Clément Bouvet (TeToNN)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 */


#include <ppu-lv2.h>
#include <stdio.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sysutil/video.h>
#include <sysutil/sysutil.h>
#include <rsx/gcm_sys.h>
#include <rsx/rsx.h>
#include <io/pad.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>

#include "ps3.h"
#include "cairo-utils.h"
#include "ps3save.h"
#include "ps3drawingapi.h"
#include "ps3graphics.h"

#define SHOW_FPS FALSE
#define STATUS_BAR_SHOW_FPS FALSE
// #define TEST_GRID

char cwd[1024];

const PuzzleDescription puzzle_descriptions[] = {
  {"blackbox",
   "Locate the balls inside the box by firing lasers and observing",
   "how they are deflected or absorbed."},
  {"bridges",
   "Connect the islands with the given bridge counts so "
   "no bridges are crossing."},
  {"cube", "Roll the cube to collect all the paint."},
  {"dominosa", "Pair the numbers to form a complete and distinct set "
   "of dominoes."},
  {"fifteen", "Slide tiles around to form a grid in numerical order."},
  {"filling", "Number the squares to form regions with the same number,",
   "which is also the size of the region."},
  {"flip", "Turn over squares until all are light side up,",
   "but flipping one flips its neighbours."},
  {"galaxies", "Divide the grid into 180-degree rotationally symmetric",
   "regions each centred on a dot."},
  {"guess", "Guess the hidden colour sequence: ",
   "black is correct, white is the correct colour in the wrong place."},
  {"inertia", "Move the ball around to collect all the gems without "
   "hitting a mine."},
  {"keen", "Enter digits so every row and column contains exactly one",
   "of each digit and the arithmetic clues are satisfied."},
  {"lightup", "Place lamps so all squares are lit, no lamp lights another",
   "and numbered squares have the given number of adjacent lamps."},
  {"loopy", "Draw a single unbroken and uncrossing line such that",
   "numbered squares have the given number of edges filled."},
  {"magnets", "Place magnets and neutrals to satisfy the +/- counts",
   "and avoid magnets repelling."},
  {"map", "Copy the 4 colours to colour the map with no regions",
   "of the same colour touching."},
  {"mines", "Uncover all squares except the mines using",
   "the given counts of adjacent mines."},
  {"net", "Rotate tiles to connect all tiles to the centre tile."},
  {"netslide", "Slide rows and columns to connect all tiles to the centre "
   "tile."},
  {"pattern", "Fill the grid so that the numbers are the length",
   "of each stretch of black tiles in order."},
  {"pegs", "Remove pegs by jumping others over them, until only one "
   "is left."},
  {"range", "Place black squares to limit the visible distance from "
   "each numbered cell."},
  {"rect", "Divide the grid into rectangles containing only one number,",
   "which is also the area of the rectangle."},
  {"samegame", "Remove groups (2 or more) of the same colour to clear the grid,",
   "scoring more for larger groups."},
  {"signpost", "Connect the squares into a path following the arrows."},
  {"singles", "Remove numbers so no number appears twice in any row/column,",
   "no two black squares are adjacent and the white squares are "
   "all connected."},
  {"sixteen", "Slide rows and columns around to form a grid in numerical "
   "order."},
  {"slant", "Draw diagonal lines in every square such that circles have",
   "the given numbers of lines meeting at them and there are no loops."},
  {"solo", "Fill the grid so each block, row and column contains exactly "
   "one of each digit."},
  {"tents",
   "Place tents so each tree has a separate adjacent tent (not diagonally),",
   "no tents are next to each other and the row and column counts are correct."},
  {"towers", "Place towers so each row/column contains one of each height "
   "and each clue ",
   "matches the number of towers visible looking into the grid "
   "from that point."},
  {"twiddle", "Rotate groups of 4 to form a grid in numerical order."},
  {"unequal", "Enter digits so every row and column contains exactly",
   "one of each digit and the greater-than signs are satisfied."},
  {"untangle", "Move points around until no lines cross."}
};

static int
handle_pointer (frontend *fe, padData *paddata)
{
  int analog_h, analog_v;

  /* Handle pointer movement (left analog stick) */
  analog_h = paddata->ANA_L_H - 0x80;
  analog_v = paddata->ANA_L_V - 0x80;

  if (analog_h > PAD_STICK_DEADZONE)
    analog_h -= PAD_STICK_DEADZONE;
  else if (analog_h < -PAD_STICK_DEADZONE)
    analog_h += PAD_STICK_DEADZONE;
  else
    analog_h = 0;

  if (analog_v > PAD_STICK_DEADZONE)
    analog_v -= PAD_STICK_DEADZONE;
  else if (analog_v < -PAD_STICK_DEADZONE)
    analog_v += PAD_STICK_DEADZONE;
  else
    analog_v = 0;

  if(analog_h != 0 || analog_v != 0) {
    fe->pointer_x += analog_h / 10;
    fe->pointer_y += analog_v / 10;

    if (fe->image) {
      int w, h;

      cairo_utils_get_surface_size (fe->image, &w, &h);

      if (fe->pointer_x > w)
        fe->pointer_x = w;
      if (fe->pointer_x < 0)
        fe->pointer_x = 0;
      if (fe->pointer_y > h)
        fe->pointer_y = h;
      if (fe->pointer_y < 0)
        fe->pointer_y = 0;
    }
    fe->redraw = TRUE;

    return TRUE;
  }

  return FALSE;
}

static int
handle_pad (frontend *fe, padData *paddata)
{
  static int prev_keyval = -1;
  static struct timeval cursor_last_ts;
  int keyval = -1;
  Ps3MenuRectangle bbox;
  struct timeval now;
  int mouse_moved = FALSE;

  if (handle_pointer (fe, paddata))
    mouse_moved = TRUE;

  if (paddata->BTN_UP)
    keyval = CURSOR_UP;
  else if (paddata->BTN_DOWN)
    keyval = CURSOR_DOWN;
  else if (paddata->BTN_LEFT)
    keyval = CURSOR_LEFT;
  else if (paddata->BTN_RIGHT)
    keyval = CURSOR_RIGHT;

  if (mouse_moved)
    fe->cursor_last_move = FALSE;

  if (IS_CURSOR_MOVE (keyval)) {
    fe->cursor_last_move = TRUE;

    if (fe->last_cursor_pressed == -1)
      gettimeofday (&cursor_last_ts, NULL);

    if (fe->last_cursor_pressed == keyval) {
      double elapsed;

      gettimeofday (&now, NULL);
      elapsed = ((now.tv_usec - cursor_last_ts.tv_usec) * 0.000001 +
          (now.tv_sec - cursor_last_ts.tv_sec));
      if (elapsed > 0.25)
        cursor_last_ts = now;
      else
        keyval = -1;
    } else {
      fe->last_cursor_pressed = keyval;
    }
  } else {
    fe->last_cursor_pressed = -1;
  }

  if (fe->cursor_last_move) {
    if (paddata->BTN_CROSS)
      keyval = CURSOR_SELECT;
    else if (paddata->BTN_CIRCLE)
      keyval = CURSOR_SELECT2;
  } else {
    if (paddata->BTN_CROSS)
      keyval = LEFT_BUTTON;
    else if (paddata->BTN_CIRCLE)
      keyval = RIGHT_BUTTON;
    else if (paddata->BTN_SQUARE)
      keyval = MIDDLE_BUTTON;
    else {
      if (fe->current_button_pressed == LEFT_BUTTON ||
          fe->current_button_pressed == LEFT_DRAG)
        keyval = LEFT_RELEASE;
      else if (fe->current_button_pressed == RIGHT_BUTTON ||
          fe->current_button_pressed == RIGHT_DRAG)
        keyval = RIGHT_RELEASE;
      else if (fe->current_button_pressed == MIDDLE_BUTTON ||
          fe->current_button_pressed == MIDDLE_DRAG)
        keyval = MIDDLE_RELEASE;
    }

    if (IS_MOUSE_DRAG (fe->current_button_pressed))
      mouse_moved = TRUE;

    if (fe->current_button_pressed != -1) {
      if (keyval == LEFT_BUTTON && mouse_moved)
        keyval = LEFT_DRAG;
      else if (keyval == RIGHT_BUTTON && mouse_moved)
        keyval = RIGHT_DRAG;
      else if (keyval == MIDDLE_BUTTON && mouse_moved)
        keyval = MIDDLE_DRAG;
    }
    fe->current_button_pressed = keyval;
  }


  if (paddata->BTN_L1)
    keyval = 'u';
  else if (paddata->BTN_R1)
    keyval = 'r';
  else if (paddata->BTN_START)
    keyval = 'q';
  else if (paddata->BTN_SELECT)
    keyval = 's';


  if (!IS_CURSOR_MOVE (keyval) && !IS_MOUSE_DRAG (keyval) &&
      prev_keyval == keyval)
    return TRUE;

  /* Store previous key to avoid flooding the same keypress */
  prev_keyval = keyval;

  if (fe->help != NULL ) {
    if (keyval == CURSOR_UP) {
      if (fe->help->start_line > 0)
        fe->help->start_line--;
    } else if(keyval == CURSOR_DOWN) {
      if (fe->help->start_line < fe->help->nlines - 1)
        fe->help->start_line++;
    } if (paddata->BTN_START || paddata->BTN_CIRCLE) {
      /* Exiting help */
      free_help (fe);
    }
    fe->redraw = TRUE;
  }
  else if (fe->menu.menu != NULL) {
    if (paddata->BTN_START || paddata->BTN_CIRCLE) {
      DEBUG ("Cancelling menu\n");
      fe->menu.callback (fe, FALSE);
      fe->redraw = TRUE;
      return TRUE;
    } else if (paddata->BTN_CROSS) {
      DEBUG ("Accepting menu\n");
      fe->menu.callback (fe, TRUE);
      fe->redraw = TRUE;
      return TRUE;
    } else if (keyval == CURSOR_UP) {
      ps3_menu_handle_input (fe->menu.menu, PS3_MENU_INPUT_UP, &bbox);
      fe->redraw = TRUE;
      return TRUE;
    } else if (keyval == CURSOR_DOWN) {
      ps3_menu_handle_input (fe->menu.menu, PS3_MENU_INPUT_DOWN, &bbox);
      fe->redraw = TRUE;
      return TRUE;
    } else if (keyval == CURSOR_LEFT) {
      ps3_menu_handle_input (fe->menu.menu, PS3_MENU_INPUT_LEFT, &bbox);
      fe->redraw = TRUE;
      return TRUE;
    } else if (keyval == CURSOR_RIGHT) {
      ps3_menu_handle_input (fe->menu.menu, PS3_MENU_INPUT_RIGHT, &bbox);
      fe->redraw = TRUE;
      return TRUE;
    }
  } else {
    if (paddata->BTN_SELECT) {
      create_types_menu (fe);
      fe->redraw = TRUE;
      return TRUE;
    }
    if (paddata->BTN_START) {
      create_main_menu (fe);
      fe->redraw = TRUE;
      return TRUE;
    }

    if (keyval >= 0 &&
        !midend_process_key (fe->me, fe->pointer_x, fe->pointer_y, keyval))
      return FALSE;

  }

  return TRUE;
}

void
calculate_puzzle_size (frontend *fe)
{
  int have_status = STATUS_BAR_SHOW_FPS;
  u16  width, height;
  int w, h;

  getResolution(&width, &height);

  if (midend_wants_statusbar(fe->me))
    have_status = TRUE;

  if (have_status)
    height -= STATUS_BAR_AREA_HEIGHT + 20;

  w = width * 0.8;
  h = height * 0.9;

  midend_size(fe->me, &w, &h, FALSE);

  fe->x = (width - w) / 2;
  fe->y = (height - h) / 2;

  DEBUG ("Puzzle is %dx%d at %d-%d\n", w, h, fe->x, fe->y);

  if (fe->status_bar) {
    cairo_surface_finish (fe->status_bar);
    cairo_surface_destroy (fe->status_bar);
    fe->status_bar = NULL;
  }

  if (have_status) {
    cairo_t *cr;
    float rgb[3];

    fe->status_x = width * 0.05;
    fe->status_y = height + STATUS_BAR_PAD;

    fe->status_bar = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
        fe->width * 0.9, STATUS_BAR_HEIGHT);
    assert (fe->status_bar != NULL);

    cr = cairo_create (fe->status_bar);
    frontend_default_colour (fe, rgb);
    cairo_set_source_rgb (cr, rgb[0], rgb[1], rgb[2]);
    cairo_paint (cr);
    cairo_destroy (cr);

    DEBUG ("Having status bar at %d - %d\n", fe->status_x, fe->status_y);
  }

  if(fe->image) {
    cairo_surface_finish (fe->image);
    cairo_surface_destroy (fe->image);
    fe->image = NULL;
  }
  fe->image = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32, w, h);
  assert (fe->image != NULL);

  fe->pointer_x = w / 2;
  fe->pointer_y = h / 2;
  fe->cursor_last_move = TRUE;
  fe->last_cursor_pressed = -1;
}

void
create_midend (frontend* fe, int game_idx)
{
  if(fe->me) {
    midend_free(fe->me);
  }

  fe->game_idx = game_idx;
  fe->thegame = gamelist[fe->game_idx];
  fe->me = midend_new (fe, fe->thegame, &ps3_drawing_api, fe);
  fe->colours = midend_colours(fe->me, &fe->ncolours);
  midend_new_game (fe->me);

  calculate_puzzle_size (fe);

  memset (&fe->save_data, 0, sizeof(SaveData));

  midend_force_redraw(fe->me);
}

void
destroy_midend (frontend *fe)
{
  if(fe->me) {
    midend_free(fe->me);
    fe->me = NULL;
  }

  if (fe->status_bar) {
    cairo_surface_destroy (fe->status_bar);
    fe->status_bar = NULL;
  }

  if(fe->image){
    cairo_surface_finish (fe->image);
    cairo_surface_destroy (fe->image);
    fe->image = NULL;
  }
}

static frontend *
new_window ()
{
  frontend *fe;
  u16 width;
  u16 height;
  int i;

  DEBUG ("Creating new window\n");

  fe = snew (frontend);
  memset (fe, 0, sizeof(frontend));

  /* Allocate a 1Mb buffer, alligned to a 1Mb boundary
   * to be our shared IO memory with the RSX. */
  fe->host_addr = memalign (1024*1024, HOST_SIZE);
  fe->context = initScreen (fe->host_addr, HOST_SIZE);

  getResolution(&width, &height);
  for (i = 0; i < MAX_BUFFERS; i++)
    makeBuffer (&fe->buffers[i], width, height, i);

  flipBuffer(fe->context, MAX_BUFFERS - 1);

  fe->width = width;
  fe->height = height;

  create_puzzles_menu (fe);

  fe->redraw = TRUE ;

  return fe;
}

static void
destroy_window (frontend *fe)
{
  int i;

  destroy_midend (fe);

  if (fe->background) {
    cairo_surface_finish (fe->background);
    cairo_surface_destroy (fe->background);
  }

  gcmSetWaitFlip(fe->context);
  for (i = 0; i < MAX_BUFFERS; i++)
    rsxFree (fe->buffers[i].ptr);

  rsxFinish (fe->context, 1);
  free (fe->host_addr);

  free_sgt_menu (fe);

  free_help (fe);

  sfree (fe);
}

static void
event_handler (u64 status, u64 param, void * user_data)
{
  XMBEvent *xmb = user_data;

  printf ("Received event %lX\n", status);
  if (status == SYSUTIL_EXIT_GAME) {
    xmb->exit = 1;
  } else if (status == SYSUTIL_MENU_OPEN) {
    xmb->opened = 1;
    xmb->closed = 0;
  } else if (status == SYSUTIL_MENU_CLOSE) {
    xmb->opened = 0;
    /* After we get closed, we must redraw a few times for the xmb's animation
     * to finish and get back our normal image */
    xmb->closed = 10;
  } else if (status == SYSUTIL_DRAW_BEGIN) {
    xmb->drawing = 1;
  } else if (status == SYSUTIL_DRAW_END) {
    xmb->drawing = 0;
  }
}

static void
get_cwd(const char *path)
{
  const char *ptr = path;

  while((ptr = strstr (ptr, "/")) != NULL) {
    strncpy (cwd, path, ptr - path);
    ptr++;
  }
  printf ("cwd is : %s\n", cwd);
}


int
main_loop_iterate (frontend *fe)
{
  padInfo padinfo;
  padData paddata;
  int i;

  if (fe->xmb.exit != 0)
    goto done;

  ioPadGetInfo (&padinfo);
  for (i = 0; i < MAX_PADS; i++) {
    if (padinfo.status[i]) {
      ioPadGetData (i, &paddata);
      if (handle_pad (fe, &paddata) == FALSE)
        goto done;
    }
  }

  /* Check for timer */
  if (fe->timer_enabled && fe->me) {
    double elapsed;
    struct timeval now;

    gettimeofday(&now, NULL);
    elapsed = ((now.tv_usec - fe->timer_last_ts.tv_usec) * 0.000001 +
        (now.tv_sec - fe->timer_last_ts.tv_sec));
    if (elapsed >= 0.02) {
      gettimeofday(&fe->timer_last_ts, NULL);
      midend_timer (fe->me, elapsed);
    }
  }

  /* When the XMB is opened, we need to flip buffers otherwise it will freeze.
   * It seems the XMB needs to get our buffers so it can overlay itself on top.
   * If we stop flipping buffers, the xmb will freeze.
   */
  if (fe->xmb.opened || fe->xmb.drawing)
    fe->redraw = TRUE;

  if (fe->xmb.closed > 0) {
    fe->xmb.closed--;
    fe->redraw = TRUE;
  }
  if ((fe->save_data.saving || fe->save_data.loading) &&
      fe->save_data.save_tid == 0) {
    /* We successfully loaded a new game, need to recaulculate size and force
       a redraw because the game params might have changed */
    if (fe->save_data.loading && fe->save_data.result == 0) {
      calculate_puzzle_size (fe);

      midend_force_redraw(fe->me);
    }
    fe->save_data.saving = fe->save_data.loading = FALSE;
    fe->redraw = TRUE;
  }

#if SHOW_FPS || STATUS_BAR_SHOW_FPS
  /* Show FPS */
  {
    static struct timeval previous_time;
    struct timeval now;
    double elapsed;

    gettimeofday(&now, NULL);
    elapsed = ((now.tv_usec - previous_time.tv_usec) * 0.000001 +
        (now.tv_sec - previous_time.tv_sec));
    previous_time = now;
    fe->redraw = TRUE;

#if SHOW_FPS
    DEBUG ("FPS : %f\n", 1 / elapsed);
#endif
#if STATUS_BAR_SHOW_FPS
    {
      char fps[100];
      snprintf (fps, 100, "FPS : %f", 1 / elapsed);
      ps3_status_bar (fe, fps);
    }
#endif
  }
#endif

  if (fe->redraw)
    ps3_redraw_screen (fe);
  fe->redraw = FALSE;

  /* We need to poll for events */
  sysUtilCheckCallback ();

  return TRUE;

 done:
  return FALSE;
}

int
main (int argc, char *argv[])
{
  frontend *fe;

  get_cwd(argv[0]);

  fe = new_window ();
  ioPadInit (7);
  sysUtilRegisterCallback (SYSUTIL_EVENT_SLOT0, event_handler, &fe->xmb);

  /* Main loop */
  while (main_loop_iterate (fe));

  printf ("Exiting application\n");
  destroy_window (fe);
  ioPadEnd();
  sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);

  return 0;
}
