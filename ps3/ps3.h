/*
 * ps3.c : PS3 Puzzles homebrew main application header
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 * Copyright (C) Clément Bouvet (TeToNN)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 */


#ifndef __PS3_H__
#define __PS3_H__

#include <sysutil/video.h>
#include <sys/thread.h>
#include <sysutil/save.h>
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

struct blitter {
  cairo_surface_t *image;
  int w, h, x, y;
};

enum SaveDataMode {
  PS3_SAVE_MODE_ICON,
  PS3_SAVE_MODE_SCREENSHOT,
  PS3_SAVE_MODE_DATA,
  PS3_SAVE_MODE_DONE,
};

typedef struct {
  sys_ppu_thread_t save_tid;
  int saving;
  int loading;
  char prefix[SYS_SAVE_MAX_DIRECTORY_NAME];
  sysSaveNewSaveGame new_save;
  sysSaveNewSaveGameIcon new_save_icon;
  enum SaveDataMode mode;
  u8 *icon_data;
  u64 icon_size;
  u8 *screenshot_data;
  u64 screenshot_size;
  u8 *save_data;
  u64 save_size;
  u32 deserialize_offset;
  s32 result;
} SaveData;


typedef struct {
  int drawing;
  int opened;
  int closed;
  int exit;
} XMBEvent;

typedef struct {
  Ps3Menu *menu;
  void (*callback) (frontend *fe, int accepted);
  void (*draw) (frontend *fe, cairo_t *cr);
} SGTPuzzlesMenu;

struct frontend {
  /* RSX device context */
  gcmContextData *context;
  void *host_addr;

  /* The buffers we will be drawing into. */
  rsxBuffer buffers[MAX_BUFFERS];
  int currentBuffer;
  int game_idx;
  const game* thegame;
  midend *me;
  const float *colours;
  int ncolours;
  cairo_t *cr;
  cairo_surface_t *image;
  cairo_surface_t *status_bar;
  cairo_surface_t *background;
  int redraw;
  int x;
  int y;
  int pointer_x;
  int pointer_y;
  int cursor_last_move;
  int last_cursor_pressed;
  int current_button_pressed;
  int width;
  int height;
  int status_x;
  int status_y;
  struct timeval timer_last_ts;
  int timer_enabled;
  SGTPuzzlesMenu menu;
  SaveData save_data;
  XMBEvent xmb;
};

typedef struct {
  const char *name;
  const char *description1;
  const char *description2;
} PuzzleDescription;

extern const PuzzleDescription puzzle_descriptions[];
extern char cwd[];

void destroy_midend (frontend * fe);
void create_midend (frontend* fe, int game_idx);
void calculate_puzzle_size (frontend *fe);
int main_loop_iterate (frontend *fe);

#endif /* __PS3_H__ */
