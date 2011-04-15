/*
 * ps3.c : PS3 Puzzles homebrew main application code
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 * Copyright (C) Clément Bouvet (TeToNN)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 */


#include <cairo/cairo.h>
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
  int keyval;

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
  else
    keyval = -1;

  /* TODO: allow long key presses */
  if (keyval >= 0 && prev_keyval != keyval &&
      !midend_process_key (fe->me, 0, 0, keyval))
    return FALSE;

  /* Store previous key to avoid flooding the same keypress */
  prev_keyval = keyval;

  return TRUE;
}

frontend *
new_window ()
{
  frontend *fe;
  u16 width;
  u16 height;
  int w, h;
  int i;

  DEBUG ("Creating new window\n");

  fe = snew (frontend);
  fe->currentBuffer = 0;
  fe->cr = NULL;
  fe->image = NULL;
  fe->status_text = NULL;
  fe->timer_enabled = FALSE;

  /* Allocate a 1Mb buffer, alligned to a 1Mb boundary
   * to be our shared IO memory with the RSX. */
  fe->host_addr = memalign (1024*1024, HOST_SIZE);
  fe->context = initScreen (fe->host_addr, HOST_SIZE);

  getResolution(&width, &height);
  for (i = 0; i < MAX_BUFFERS; i++)
    makeBuffer (&fe->buffers[i], width, height, i);

  flip(fe->context, MAX_BUFFERS - 1);

  fe->me = midend_new (fe, &thegame, &ps3_drawing, fe);
  fe->colours = midend_colours(fe->me, &fe->ncolours);

  /* Start the game */
  midend_new_game (fe->me);

  DEBUG ("Screen resolution is %dx%d\n", width, height);
  if (midend_wants_statusbar(fe->me))
    height -= 50;

  w = width * 0.8;
  h = height * 0.9;

  midend_size(fe->me, &w, &h, FALSE);
  fe->width = w;
  fe->height = h;

  fe->x = (width - w) / 2;
  fe->y = (height - h) / 2;

  DEBUG ("Puzzle is %dx%d at %d-%d\n", w, h, fe->x, fe->y);

  fe->image = cairo_image_surface_create  (CAIRO_FORMAT_ARGB32,
      fe->width, fe->height);
  assert (fe->image != NULL);

  midend_force_redraw(fe->me);

  return fe;
}

void
destroy_window (frontend *fe)
{
  int i;

  cairo_surface_finish (fe->image);
  cairo_surface_destroy (fe->image);

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
  //int frame = 0;
  int i;
  frontend *fe;
  time_t now;
  double elapsed;

  fe = new_window ();
  ioPadInit (7);

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
    if (fe->timer_enabled) {
      struct timeval now;

      gettimeofday(&now, NULL);
      elapsed = ((now.tv_usec - fe->timer_last_ts.tv_usec) * 0.000001 +
          (now.tv_sec - fe->timer_last_ts.tv_sec));
      if (elapsed >= 0.02) {
        gettimeofday(&fe->timer_last_ts, NULL);
        midend_timer (fe->me, elapsed);
      }
    }
  }

 end:

  destroy_window (fe);
  ioPadEnd();

  return 0;
}
