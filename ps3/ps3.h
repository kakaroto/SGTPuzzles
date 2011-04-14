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
#include "rsxutil.h"
#include "puzzles.h"

struct blitter {
  cairo_surface_t *image;
  int w, h, x, y;
};

 // ps3 frontend
struct frontend {
  gcmContextData *context;	// rsx buffer context
  videoResolution res;		// Screen Resolution

  int currentBuffer;
  rsxBuffer *buffers[2];	// The buffer we will be drawing into.
  midend *me;
  const float *colours;
  int ncolours;
  cairo_t *cr;
  cairo_surface_t *image;
};
