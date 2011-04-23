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
#include <time.h>
#include <sys/time.h>
#include <cairo/cairo.h>


#define COMBINED
#include "puzzles.h"

#include "rsxutil.h"
#include "ps3menu.h"

#define MAX_BUFFERS 2

#define STATUS_BAR_ALPHA 0.5
#define STATUS_BAR_HEIGHT 30
#define STATUS_BAR_PAD 10
#define STATUS_BAR_IPAD 4
#define STATUS_BAR_AREA_HEIGHT (STATUS_BAR_HEIGHT + (2 * STATUS_BAR_PAD))
#define STATUS_BAR_TEXT_SIZE (STATUS_BAR_HEIGHT - (2 * STATUS_BAR_IPAD))

#define PUZZLE_MENU_DESCRIPTION_HEIGHT 50

#define PAD_STICK_DEADZONE 0x20

#ifndef DISABLE_DEBUG
#define DEBUG(...) { \
  struct timeval debug_time; \
  gettimeofday (&debug_time, NULL); \
  printf ("[%02lu:%02lu:%03lu.%03lu] ", (debug_time.tv_sec % 3600) / 60, \
      debug_time.tv_sec % 60, debug_time.tv_usec / 1000, \
      debug_time.tv_usec % 1000);                        \
  printf (__VA_ARGS__);                                           \
  }
#else
#define DEBUG(...) {}
#endif

typedef enum {
  MODE_PUZZLE,
  MODE_PUZZLE_MENU,
  MODE_MAIN_MENU,
  MODE_TYPES_MENU,
} GameMode;

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
  const game* thegame;
  midend *me;
  const float *colours;
  int ncolours;
  cairo_t *cr;
  cairo_surface_t *image;
  cairo_surface_t *status_bar;
  cairo_surface_t *background;
  int x;
  int y;
  int pointer_x;
  int pointer_y;
  int cursor_last_move;
  int last_cursor_pressed;
  int current_button_pressed;
  struct timeval cursor_last_ts;
  int width;
  int height;
  int status_x;
  int status_y;
  struct timeval timer_last_ts;
  int timer_enabled;
  GameMode mode;
  Ps3Menu *menu;
};

typedef struct {
  const char *name;
  const char *description1;
  const char *description2;
} PuzzleDescription;

extern const PuzzleDescription puzzle_descriptions[];

void create_midend(frontend* fe,const game* game);
