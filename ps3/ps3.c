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
#include <stdarg.h>
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
#include "ps3drawingapi.h"

#define SHOW_FPS FALSE
#define STATUS_BAR_SHOW_FPS FALSE
// #define TEST_GRID

static char cwd[1024];

static void calculate_puzzle_size (frontend *fe);

void
fatal (char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "fatal error: ");

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);

  fprintf (stderr, "\n");
  exit (1);
}

void
get_random_seed (void **randseed, int *randseedsize)
{
  struct timeval *tvp = snew (struct timeval);

  gettimeofday (tvp, NULL);
  *randseed = (void *) tvp;
  *randseedsize = sizeof (struct timeval);
}

void
create_types_menu (frontend * fe){
  cairo_surface_t *surface;
  int n;
  int i;

  fe->mode = MODE_TYPES_MENU;
  surface = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
      306, fe->height * 0.7);

  fe->menu = ps3_menu_new (surface, -1, 1, 300, 40);
  n = midend_num_presets(fe->me);

  if(n <= 0){ /* No types */
    fe->mode = MODE_PUZZLE;
    ps3_menu_free (fe->menu);
    fe->menu = NULL;
    return;
  } else{
    for(i = 0; i < n; i++){
      char* name;
      game_params *params;
      midend_fetch_preset(fe->me, i, &name, &params);
      ps3_menu_add_item (fe->menu, name, 25);
    }
  }
}

void create_puzzle_menu (frontend * fe);
void destroy_midend (frontend * fe);

void _change_game (frontend *fe)
{
  destroy_midend (fe);
  create_puzzle_menu (fe);
  ps3_refresh_draw(fe);
}

void _new_game (frontend *fe)
{
  midend_new_game (fe->me);
  midend_force_redraw(fe->me);
}

void _restart_game (frontend *fe)
{
  midend_restart_game (fe->me);
  midend_force_redraw(fe->me);
}

void _solve_game (frontend *fe)
{
  midend_solve (fe->me);
  midend_force_redraw(fe->me);
}

struct {
  const char *title;
  void (*callback) (frontend *fe);
} main_menu_items[] = {
  {"Change game", _change_game},
  {"New game", _new_game},
  {"Restart game", _restart_game},
  {"Solve", _solve_game},
  {NULL, NULL},
};

void
create_main_menu (frontend * fe) {
  cairo_surface_t *surface;
  int i;

  fe->mode = MODE_MAIN_MENU;
  surface = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
      306, fe->height * 0.7);

  /* Infinite vertical scrollable menu */
  fe->menu = ps3_menu_new (surface, -1, 1, 300, 40);
  for (i = 0; main_menu_items[i].title; i++) {
    ps3_menu_add_item (fe->menu, main_menu_items[i].title, 25);

    if (main_menu_items[i].callback == _solve_game)
      fe->menu->items[i].enabled = fe->thegame->can_solve;
  }
}

void
create_puzzle_menu (frontend * fe) {
  cairo_surface_t *surface;
  int i ;

  fe->mode = MODE_PUZZLE_MENU;
  surface = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
      680, fe->height * 0.9);

  /* Infinite vertical scrollable menu */
  fe->menu = ps3_menu_new (surface, -1, 4, 130, 150);
  fe->menu->pad_x = 20;
  cairo_surface_destroy (surface);

  for (i = 0; i < gamecount; i++) {
    char filename[256];

    ps3_menu_add_item (fe->menu, gamelist[i]->name, 20);
    snprintf (filename, 255, "%s/data/puzzles/%s.png", cwd, gamelist_names[i]);
    surface = cairo_image_surface_create_from_png (filename);
    if (surface) {
      fe->menu->items[i].alignment = PS3_MENU_ALIGN_BOTTOM_CENTER;
      ps3_menu_set_item_image (fe->menu, i, surface, PS3_MENU_IMAGE_POSITION_TOP);
      cairo_surface_destroy (surface);
    }
  }
}

void
frontend_default_colour (frontend * fe, float *output)
{
  /* let's use grey as the background */
  output[0] = output[1] = output[2] = 0.7;
}

const struct drawing_api ps3_drawing = {
  ps3_draw_text,
  ps3_draw_rect,
  ps3_draw_line,
  ps3_draw_poly,
  ps3_draw_circle,
  ps3_draw_update,
  ps3_clip,
  ps3_unclip,
  ps3_start_draw,
  ps3_end_draw,
  ps3_status_bar,
  ps3_blitter_new,
  ps3_blitter_free,
  ps3_blitter_save,
  ps3_blitter_load,
  NULL, NULL, NULL, NULL, NULL, NULL,   /* {begin,end}_{doc,page,puzzle} */
  NULL, NULL,                   /* line_width, line_dotted */
  NULL,
  ps3_draw_thick_line,
};

void
deactivate_timer (frontend * fe)
{
  DEBUG ("Stopping timer\n");
  fe->timer_enabled = FALSE;
}

void
activate_timer (frontend * fe)
{
  gettimeofday(&fe->timer_last_ts, NULL);
  fe->timer_enabled = TRUE;
  DEBUG ("Starting timer at : %lu.%lu\n", fe->timer_last_ts.tv_sec,
      fe->timer_last_ts.tv_usec);
}

int
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
      int w = cairo_image_surface_get_width (fe->image);
      int h = cairo_image_surface_get_height (fe->image);

      if (fe->pointer_x > w)
        fe->pointer_x = w;
      if (fe->pointer_x < 0)
        fe->pointer_x = 0;
      if (fe->pointer_y > h)
        fe->pointer_y = h;
      if (fe->pointer_y < 0)
        fe->pointer_y = 0;
    }
    ps3_refresh_draw (fe);

    return TRUE;
  }

  return FALSE;
}

int
handle_pad (frontend *fe, padData *paddata)
{
  static int prev_keyval = -1;
  int keyval = -1;
  Ps3MenuRectangle bbox;
  struct timeval now;

  if (handle_pointer (fe, paddata))
    fe->cursor_last_move = FALSE;

  if (paddata->BTN_UP)
    keyval = CURSOR_UP;
  else if (paddata->BTN_DOWN)
    keyval = CURSOR_DOWN;
  else if (paddata->BTN_LEFT)
    keyval = CURSOR_LEFT;
  else if (paddata->BTN_RIGHT)
    keyval = CURSOR_RIGHT;

  if (IS_CURSOR_MOVE (keyval)) {
    fe->cursor_last_move = TRUE;

    if (fe->last_cursor_pressed == -1)
      gettimeofday (&fe->cursor_last_ts, NULL);

    if (fe->last_cursor_pressed == keyval) {
      double elapsed;

      gettimeofday (&now, NULL);
      elapsed = ((now.tv_usec - fe->cursor_last_ts.tv_usec) * 0.000001 +
          (now.tv_sec - fe->cursor_last_ts.tv_sec));
      if (elapsed > 0.25)
        fe->cursor_last_ts = now;
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
  }

  if (paddata->BTN_L1)
    keyval = 'u';
  else if (paddata->BTN_R1)
    keyval = 'r';
  else if (paddata->BTN_START)
    keyval = 'q';
  else if (paddata->BTN_SELECT)
    keyval = 's';


  if (!IS_CURSOR_MOVE (keyval) && prev_keyval == keyval)
    return TRUE;

  /* Store previous key to avoid flooding the same keypress */
  prev_keyval = keyval;

  if (fe->mode == MODE_PUZZLE) {
    if (paddata->BTN_SELECT) {
      create_types_menu (fe);
      ps3_refresh_draw (fe);
      return TRUE;
    }
    if (paddata->BTN_START) {
      /* TODO: need to create the menu */
      create_main_menu (fe);
      fe->mode = MODE_MAIN_MENU;
      ps3_refresh_draw (fe);
      return TRUE;
    }

    /* TODO: allow long key presses */
    if (keyval >= 0 &&
        !midend_process_key (fe->me, fe->pointer_x, fe->pointer_y, keyval))
      return FALSE;

  } else {
    if (paddata->BTN_START || paddata->BTN_CIRCLE) {
      if(fe->mode == MODE_MAIN_MENU || fe->mode == MODE_TYPES_MENU) {
        fe->mode = MODE_PUZZLE;
        DEBUG ("Leaving menu mode\n");
        if (fe->menu != NULL)
          ps3_menu_free (fe->menu);
        fe->menu = NULL;
        ps3_refresh_draw (fe);
        return TRUE;
      }
    } else if (paddata->BTN_CROSS) {
      int selected_item = fe->menu->selection;
      if(fe->mode == MODE_PUZZLE_MENU) {
        /* Game selected */
        fe->mode = MODE_PUZZLE;
        ps3_menu_free (fe->menu);
        fe->menu = NULL;

        create_midend(fe, gamelist[selected_item]);

      } else if (fe->mode == MODE_TYPES_MENU) {
        /* Type selected */
        game_params *params;
        char* name;

        fe->mode = MODE_PUZZLE;
        ps3_menu_free (fe->menu);
        fe->menu = NULL;

        midend_fetch_preset(fe->me, selected_item, &name, &params);
        midend_set_params (fe->me,params);
        midend_new_game(fe->me);

        calculate_puzzle_size (fe);

        midend_force_redraw(fe->me);
      } else if (fe->mode == MODE_MAIN_MENU) {
        fe->mode = MODE_PUZZLE;
        ps3_menu_free (fe->menu);
        fe->menu = NULL;
        main_menu_items[selected_item].callback (fe);
      }
    } else if (keyval == CURSOR_UP) {
      ps3_menu_handle_input (fe->menu, PS3_MENU_INPUT_UP, &bbox);
      ps3_refresh_draw (fe);
      return TRUE;
    } else if (keyval == CURSOR_DOWN) {
      ps3_menu_handle_input (fe->menu, PS3_MENU_INPUT_DOWN, &bbox);
      ps3_refresh_draw (fe);
      return TRUE;
    } else if (keyval == CURSOR_LEFT) {
      ps3_menu_handle_input (fe->menu, PS3_MENU_INPUT_LEFT, &bbox);
      ps3_refresh_draw (fe);
      return TRUE;
    } else if (keyval == CURSOR_RIGHT) {
      ps3_menu_handle_input (fe->menu, PS3_MENU_INPUT_RIGHT, &bbox);
      ps3_refresh_draw (fe);
      return TRUE;
    }
  }

  return TRUE;
}

static void
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

  if (have_status) {
    cairo_t *cr;
    float rgb[3];

    fe->status_x = width * 0.05;
    fe->status_y = height + STATUS_BAR_PAD;

    if (fe->status_bar) {
      cairo_surface_finish (fe->status_bar);
      cairo_surface_destroy (fe->status_bar);
      fe->status_bar = NULL;
    }

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
create_midend(frontend* fe, const game* game)
{
  if(fe->me) {
    midend_free(fe->me);
  }

  fe->thegame = game;
  fe->me = midend_new (fe, fe->thegame, &ps3_drawing, fe);
  fe->colours = midend_colours(fe->me, &fe->ncolours);
  midend_new_game (fe->me);

  calculate_puzzle_size (fe);

  fe->mode = MODE_PUZZLE;

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

frontend *
new_window ()
{
  frontend *fe;
  u16 width;
  u16 height;
  int i;

  DEBUG ("Creating new window\n");

  fe = snew (frontend);
  fe->currentBuffer = 0;
  fe->cr = NULL;
  fe->image = NULL;
  fe->status_bar = NULL;
  fe->timer_enabled = FALSE;
  fe->background = NULL;
  fe->menu = NULL;
  fe->me = NULL;

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

  create_puzzle_menu (fe);

  ps3_refresh_draw (fe);

  return fe;
}

void
destroy_window (frontend *fe)
{
  int i;

  cairo_surface_finish (fe->image);
  cairo_surface_destroy (fe->image);

  if (fe->status_bar) {
    cairo_surface_finish (fe->status_bar);
    cairo_surface_destroy (fe->status_bar);
  }

  if (fe->background) {
    cairo_surface_finish (fe->background);
    cairo_surface_destroy (fe->background);
  }

  gcmSetWaitFlip(fe->context);
  for (i = 0; i < MAX_BUFFERS; i++)
    rsxFree (fe->buffers[i].ptr);

  rsxFinish (fe->context, 1);
  free (fe->host_addr);

  sfree (fe);
}

typedef struct {
  int opened;
  int closed;
  int exit;
} XMBEvent;

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
    xmb->closed = 1;
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
main (int argc, char *argv[])
{
  padInfo padinfo;
  padData paddata;
  int frame = 0;
  XMBEvent xmb = {0, 0};
  struct timeval previous_time;
  int i;
  frontend *fe;
  double elapsed;

  get_cwd(argv[0]);

  fe = new_window ();
  ioPadInit (7);
  sysUtilRegisterCallback (SYSUTIL_EVENT_SLOT0, event_handler, &xmb);

  gettimeofday (&previous_time, NULL);

  /* Main loop */
  while (xmb.exit == 0)
  {
    ioPadGetInfo (&padinfo);
    for (i = 0; i < MAX_PADS; i++) {
      if (padinfo.status[i]) {
        ioPadGetData (i, &paddata);
        if (handle_pad (fe, &paddata) == FALSE)
          goto end;
      }
    }
    /* Check for timer */
    if (fe->timer_enabled && fe->me) {
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
    if (xmb.opened) {
      rsxBuffer *buffer = &fe->buffers[fe->currentBuffer];

      setRenderTarget(fe->context, buffer);
      /* Wait for the last flip to finish, so we can draw to the old buffer */
      waitFlip ();
      /* Flip buffer onto screen */
      flipBuffer (fe->context, fe->currentBuffer);
      fe->currentBuffer++;
      if (fe->currentBuffer >= MAX_BUFFERS)
        fe->currentBuffer = 0;
    }
    if (xmb.closed) {
      ps3_refresh_draw (fe);
    }

#if SHOW_FPS || STATUS_BAR_SHOW_FPS
    /* Show FPS */
    {
      struct timeval now;

      gettimeofday(&now, NULL);
      elapsed = ((now.tv_usec - previous_time.tv_usec) * 0.000001 +
          (now.tv_sec - previous_time.tv_sec));
      previous_time = now;
      ps3_refresh_draw (fe);
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

    frame++;
    /* We need to poll for events */
    sysUtilCheckCallback ();
  }

 end:

  destroy_window (fe);
  ioPadEnd();
  sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);

  return 0;
}
