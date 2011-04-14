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
  /* TODO: */
}

void
activate_timer (frontend * fe)
{
  /* TODO: */
}

frontend *
new_window ()
{
  frontend *fe;
  u16 width;
  u16 height;
  int x, y;
  int i;

  fe = snew (frontend);
  fe->currentBuffer = 0;
  fe->cr = NULL;
  fe->image = NULL;
  fe->status_text = NULL;

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

  /* TODO: probably need to store where the puzzle gets positioned */
  x = width;
  y = width;
  midend_size(fe->me, &x, &y, FALSE);
  midend_force_redraw(fe->me);

  return fe;
}

void
destroy_window (frontend *fe)
{
  int i;

  gcmSetWaitFlip(fe->context);
  for (i = 0; i < MAX_BUFFERS; i++)
    rsxFree (fe->buffers[i].ptr);

  rsxFinish (fe->context, 1);
  free (fe->host_addr);
}

int
main (int argc, char *argv[])	// TODO :D
{
  padInfo padinfo;
  padData paddata;
  //int frame = 0;
  int i;
  frontend *fe;

  fe = new_window ();
  ioPadInit (7);

  /* Main loop */
  while (1)
  {
    ioPadGetInfo (&padinfo);
    for (i = 0; i < MAX_PADS; i++) {
      if (padinfo.status[i]) {
	ioPadGetData (i, &paddata);
        if(paddata.BTN_START) {
          goto end;
        }
      }
    }
  }

 end:

  destroy_window (fe);
  ioPadEnd();

  return 0;
}
