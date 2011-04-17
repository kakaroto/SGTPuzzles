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
  NULL, NULL, NULL, NULL, NULL, NULL,	/* {begin,end}_{doc,page,puzzle} */
  NULL, NULL,			/* line_width, line_dotted */
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
handle_pad (frontend *fe, padData *paddata)
{
  static int prev_keyval = -1;
  int keyval = -1;
  Ps3MenuRectangle bbox;

  if (paddata->BTN_UP)
    keyval = CURSOR_UP;
  else if (paddata->BTN_DOWN)
    keyval = CURSOR_DOWN;
  else if (paddata->BTN_LEFT)
    keyval = CURSOR_LEFT;
  else if (paddata->BTN_RIGHT)
    keyval = CURSOR_RIGHT;
  else if (paddata->BTN_CROSS)
    keyval = CURSOR_SELECT;
  else if (paddata->BTN_CIRCLE)
    keyval = CURSOR_SELECT2;
  else if (paddata->BTN_L1)
    keyval = 'u';
  else if (paddata->BTN_R1)
    keyval = 'r';
  else if (paddata->BTN_R3)
    keyval = 'n';
  else if (paddata->BTN_START)
    keyval = 'q';
  else if (paddata->BTN_SELECT)
    keyval = 's';
  else
    keyval = -1;

  if (prev_keyval == keyval)
    return TRUE;

  /* Store previous key to avoid flooding the same keypress */
  prev_keyval = keyval;

  if (fe->mode == MODE_PUZZLE) {
    if (paddata->BTN_SELECT) {
      fe->mode = MODE_TYPES_MENU;
      ps3_refresh_draw (fe);
      return TRUE;
    }

    /* TODO: allow long key presses */
    if (keyval >= 0 && !midend_process_key (fe->me, 0, 0, keyval))
      return FALSE;

  } else {
    if (paddata->BTN_START || paddata->BTN_CIRCLE) {
      if(fe->mode != MODE_PUZZLE_MENU){
	fe->mode = MODE_PUZZLE;
	DEBUG ("Leaving menu mode\n");
	if (fe->menu != NULL)
	  ps3_menu_free (fe->menu);
	fe->menu = NULL;
	ps3_refresh_draw (fe);
	return TRUE;
      }
    } else if (paddata->BTN_CROSS) {
      int selectedItem = fe->menu->selection;
      if(fe->mode == MODE_PUZZLE_MENU){/* Game selected */
	create_midend(fe,gamelist[selectedItem]);
      }
      else if (fe->mode == MODE_TYPES_MENU){ /* Type selected */
	/* TODO */
      }
      fe->mode = MODE_PUZZLE;
      ps3_menu_free (fe->menu);
      fe->menu = NULL;
      ps3_refresh_draw (fe);
    } else if (paddata->BTN_UP) {
      ps3_menu_handle_input(fe->menu,PS3_MENU_INPUT_UP,&bbox);
      return TRUE;
    } else if (paddata->BTN_DOWN) {
      ps3_menu_handle_input(fe->menu,PS3_MENU_INPUT_DOWN,&bbox);
      return TRUE;
    }
    else if (paddata->BTN_LEFT) {
      ps3_menu_handle_input(fe->menu,PS3_MENU_INPUT_LEFT,&bbox);
      return TRUE;
    }
    else if (paddata->BTN_RIGHT) {
      ps3_menu_handle_input(fe->menu,PS3_MENU_INPUT_RIGHT,&bbox);
      return TRUE;
    }
  }

  return TRUE;
}

void
create_midend(frontend* fe,const game* game)
{
  int have_status = STATUS_BAR_SHOW_FPS;
  u16  width,height;
  int w, h;

  getResolution(&width, &height);

  if(fe->me) {
    midend_free(fe->me);
  }

  fe->me = midend_new (fe, game, &ps3_drawing, fe);
  fe->colours = midend_colours(fe->me, &fe->ncolours);
  midend_new_game (fe->me);

  if (midend_wants_statusbar(fe->me))
    have_status = TRUE;

  if (have_status)
    height -= STATUS_BAR_AREA_HEIGHT + 20;

  w = width * 0.8;
  h = height * 0.9;

  midend_size(fe->me, &w, &h, FALSE);

  fe->width = w;
  fe->height = h;

  fe->x = (width - w) / 2;
  fe->y = (height - h) / 2;

  DEBUG ("Puzzle is %dx%d at %d-%d\n", w, h, fe->x, fe->y);

  if (have_status) {
    cairo_t *cr;
    float rgb[3];

    fe->status_x = fe->x;
    fe->status_y = height + STATUS_BAR_PAD;
    fe->status_bar = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
						  fe->width, STATUS_BAR_HEIGHT);
    assert (fe->status_bar != NULL);

    cr = cairo_create (fe->status_bar);
    frontend_default_colour (fe, rgb);
    cairo_set_source_rgb (cr, rgb[0], rgb[1], rgb[2]);
    cairo_paint (cr);
    cairo_destroy (cr);

    DEBUG ("Having status bar at %d - %d", fe->status_x, fe->status_y);
  }

  if(fe->image){
    cairo_surface_finish (fe->image);
    cairo_surface_destroy (fe->image);
    fe->image = NULL;
  }
  fe->image = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
					   fe->width, fe->height);
  assert (fe->image != NULL);

  fe->mode = MODE_PUZZLE;

  midend_force_redraw(fe->me);
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
  fe->mode = MODE_PUZZLE_MENU; 
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

  fe->image = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
					   fe->width, fe->height);
  assert (fe->image != NULL);

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

int
main (int argc, char *argv[])
{
  padInfo padinfo;
  padData paddata;
  int frame = 0;
  struct timeval previous_time;
  int i;
  frontend *fe;
  double elapsed;

  fe = new_window ();
  ioPadInit (7);

  gettimeofday (&previous_time, NULL);

  /* Main loop */
  while (1)
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

#if SHOW_FPS || STATUS_BAR_SHOW_FPS
    /* Show FPS */
    {
      struct timeval now;
      char fps[100];

      gettimeofday(&now, NULL);
      elapsed = ((now.tv_usec - previous_time.tv_usec) * 0.000001 +
          (now.tv_sec - previous_time.tv_sec));
      previous_time = now;
      ps3_refresh_draw (fe);
#if SHOW_FPS
      DEBUG ("FPS : %f\n", 1 / elapsed);
#endif
#if STATUS_BAR_SHOW_FPS
      snprintf (fps, 100, "FPS : %f", 1 / elapsed);
      ps3_status_bar (fe, fps);
#endif
    }
#endif

    frame++;
  }

 end:

  destroy_window (fe);
  ioPadEnd();

  return 0;
}
