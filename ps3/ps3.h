/*
 * ps3.c : PS3 Puzzles homebrew main application header
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 * Copyright (C) Clément Bouvet (TeToNN)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 */


#pragma once
#include <sysutil/video.h>
#include <rsx/gcm_sys.h>
#include <rsx/rsx.h>
#include <ppu-types.h>
#include <sys/time.h>
#include "rsxutil.h"
#include "puzzles.h"

#define MAX_BUFFERS 2

#ifndef DISABLE_DEBUG
#define DEBUG printf
#else
#define DEBUG(...) {}
#endif

struct blitter {
  cairo_surface_t *image;
  int w, h, x, y;
};

struct frontend {
  /* RSX device context */
  gcmContextData *context;
  void *host_addr;

  /* The buffers we will be drawing into. */
  rsxBuffer buffers[MAX_BUFFERS];
  int currentBuffer;
  midend *me;
  const float *colours;
  int ncolours;
  cairo_t *cr;
  cairo_surface_t *image;
  cairo_surface_t *surface;
  char *status_text;
  int x;
  int y;
  int width;
  int height;
  time_t timer_last_ts;
};
