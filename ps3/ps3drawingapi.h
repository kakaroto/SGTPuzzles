
#include "ps3.h"
#include <cairo/cairo.h>
#include <assert.h>

void ps3_draw_text (void *handle, int x, int y, int fonttype, int fontsize,
    int align, int colour, char *text);
void ps3_draw_rect (void *handle, int x, int y, int w, int h, int colour);
void ps3_draw_line (void *handle, int x1, int y1, int x2, int y2, int colour);
void ps3_draw_poly (void *handle, int *coords, int npoints, int fillcolour,
    int outlinecolour);
void ps3_draw_circle (void *handle, int cx, int cy, int radius, int fillcolour,
    int outlinecolour);
void ps3_draw_thick_line (void *handle, float thickness, float x1, float y1,
    float x2, float y2, int colour);
void ps3_draw_update (void *handle, int x, int y, int w, int h);	// ??
									// TODO
void ps3_clip (void *handle, int x, int y, int w, int h);
void ps3_unclip (void *handle);
void ps3_status_bar (void *handle, char *text);
void ps3_start_draw (void *handle);
void ps3_end_draw (void *handle);

blitter *ps3_blitter_new (void *handle, int w, int h);
void ps3_blitter_free (void *handle, blitter * bl);
void ps3_blitter_save (void *handle, blitter * bl, int x, int y);
void ps3_blitter_load (void *handle, blitter * bl, int x, int y);
