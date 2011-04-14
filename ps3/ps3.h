#pragma once
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
  VideoResolution res;		// Screen Resolution

  int currentBuffer;
  rsxBuffer *buffers[2];	// The buffer we will be drawing into.
  midend *me;
  const float *colours;
  int ncolours;
  cairo_t *cr;
  cairo_surface_t *image;
};
