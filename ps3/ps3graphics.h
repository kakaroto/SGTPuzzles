/*
 * ps3graphics.c : PS3 Puzzles Main graphics engine
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 */

#ifndef __PS3_GRAPHICS_H__
#define __PS3_GRAPHICS_H__

#include "ps3.h"

void ps3_redraw_screen (frontend *fe);
void create_puzzles_menu (frontend * fe);
void create_main_menu (frontend * fe);
void create_types_menu (frontend * fe);
void free_sgt_menu (frontend *fe);
void free_help (frontend *fe);

#endif /* __PS3_GRAPHICS_H__ */
