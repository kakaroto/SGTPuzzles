/*
 * test-menu-gtk.c : PS3 Menu testing application
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 *
 *
 * Compile with :
 *
 * gcc -g -O0 -o test-menu-gtk test-menu-gtk.c ps3menu.c \
 *     `pkg-config --cflags --libs cairo`                \
 *     `pkg-config --cflags --libs gtk+-2.0`
*/


#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <sys/time.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <cairo/cairo.h>

#include "ps3menu.h"

static cairo_surface_t *surface = NULL;
static Ps3Menu *menu = NULL;


static gint key_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    Ps3MenuInput input;
    Ps3MenuRectangle bbox;

    if (event->keyval == GDK_Up)
        input = PS3_MENU_INPUT_UP;
    else if (event->keyval == GDK_Down)
        input = PS3_MENU_INPUT_DOWN;
    else if (event->keyval == GDK_Left)
        input = PS3_MENU_INPUT_LEFT;
    else if (event->keyval == GDK_Right)
        input = PS3_MENU_INPUT_RIGHT;
    else
      return TRUE;

    ps3_menu_handle_input (menu, input, &bbox);
    gtk_widget_queue_draw_area (widget, bbox.x, bbox.y, bbox.width, bbox.height);
    return TRUE;
}


static void
draw (GtkWidget *widget, GdkEventExpose *eev, gpointer data)
{
  cairo_t *cr;
  cairo_surface_t *image = ps3_menu_get_surface (menu);

  cr = gdk_cairo_create(widget->window);

  ps3_menu_redraw (menu);

  cairo_set_source_surface (cr, image, 0, 0);
  cairo_paint (cr);
  cairo_destroy(cr);
  cairo_surface_destroy (image);
}

int main(int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *area;
  int idx;
  cairo_surface_t *image = NULL;

  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  area = gtk_drawing_area_new();
  gtk_widget_set_size_request (area, 640, 420);
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(area));

  g_signal_connect (G_OBJECT (window), "delete-event",
      G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (area), "expose-event", G_CALLBACK (draw), NULL);
    gtk_signal_connect(GTK_OBJECT(window), "key_press_event",
		       GTK_SIGNAL_FUNC(key_event), NULL);

  gtk_widget_show_all(window);



  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
      620, 400);
  menu = ps3_menu_new (surface, -1, 3, (620 / 3) - 10, 50);

  idx = ps3_menu_add_item (menu, "TOP LEFT", 10);
  menu->items[idx].alignment = PS3_MENU_ALIGN_TOP_LEFT;
  idx = ps3_menu_add_item (menu, "TOP CENTER", 10);
  menu->items[idx].alignment = PS3_MENU_ALIGN_TOP_CENTER;
  idx = ps3_menu_add_item (menu, "TOP RIGHT", 10);
  menu->items[idx].alignment = PS3_MENU_ALIGN_TOP_RIGHT;
  idx = ps3_menu_add_item (menu, "MIDDLE LEFT", 10);
  menu->items[idx].alignment = PS3_MENU_ALIGN_MIDDLE_LEFT;
  idx = ps3_menu_add_item (menu, "MIDDLE CENTER", 10);
  menu->items[idx].alignment = PS3_MENU_ALIGN_MIDDLE_CENTER;
  idx = ps3_menu_add_item (menu, "MIDDLE RIGHT", 10);
  menu->items[idx].alignment = PS3_MENU_ALIGN_MIDDLE_RIGHT;
  idx = ps3_menu_add_item (menu, "BOTTOM LEFT", 10);
  menu->items[idx].alignment = PS3_MENU_ALIGN_BOTTOM_LEFT;
  idx = ps3_menu_add_item (menu, "BOTTOM CENTER", 10);
  menu->items[idx].alignment = PS3_MENU_ALIGN_BOTTOM_CENTER;
  idx = ps3_menu_add_item (menu, "BOTTOM RIGHT", 10);
  menu->items[idx].alignment = PS3_MENU_ALIGN_BOTTOM_RIGHT;
  idx = ps3_menu_add_item (menu, "Pattern", 15);
  menu->items[idx].alignment = PS3_MENU_ALIGN_MIDDLE_LEFT;
  image = cairo_image_surface_create_from_png ("data/puzzles/pattern.png");
  ps3_menu_set_item_image (menu, idx, image, PS3_MENU_IMAGE_POSITION_RIGHT);
  cairo_surface_destroy (image);
  idx = ps3_menu_add_item (menu, "Rectangles", 15);
  menu->items[idx].alignment = PS3_MENU_ALIGN_MIDDLE_RIGHT;
  image = cairo_image_surface_create_from_png ("data/puzzles/rect.png");
  ps3_menu_set_item_image (menu, idx, image, PS3_MENU_IMAGE_POSITION_LEFT);
  cairo_surface_destroy (image);
  ps3_menu_add_item (menu, "Hello world 3", 20);
  ps3_menu_add_item (menu, "Hello world 4", 20);
  ps3_menu_add_item (menu, "Hello world 5", 20);
  ps3_menu_add_item (menu, "Hello world 6", 20);
  ps3_menu_add_item (menu, "Hello world 7", 20);
  ps3_menu_add_item (menu, "Hello world 8", 20);
  ps3_menu_add_item (menu, "Hello world 9", 20);
  ps3_menu_add_item (menu, "Hello world 10", 15);
  ps3_menu_add_item (menu, "Hello world 11", 5);
  ps3_menu_add_item (menu, "Hello world 12", 5);
  ps3_menu_add_item (menu, "Hello world 13", 5);
  ps3_menu_add_item (menu, "Hello world 14", 5);
  ps3_menu_add_item (menu, "Hello world 5", 15);
  ps3_menu_add_item (menu, "Hello world 1", 10);
  ps3_menu_add_item (menu, "Hello world 2", 10);
  ps3_menu_add_item (menu, "Hello world 3", 10);
  ps3_menu_add_item (menu, "Hello world 4", 10);
  ps3_menu_add_item (menu, "Hello world 5", 10);
  ps3_menu_add_item (menu, "Hello world 6", 10);
  ps3_menu_add_item (menu, "Hello world 7", 10);
  ps3_menu_add_item (menu, "Hello world 8", 10);
  ps3_menu_add_item (menu, "Hello world 9", 10);
  ps3_menu_add_item (menu, "Hello world 10", 10);
  ps3_menu_add_item (menu, "Hello world 11", 10);
  ps3_menu_add_item (menu, "Hello world 12", 10);
  ps3_menu_add_item (menu, "Hello world 13", 10);
  ps3_menu_add_item (menu, "Hello world 14", 10);
  ps3_menu_add_item (menu, "Hello world 15", 10);
  cairo_surface_destroy (surface);

  gtk_main();

  return 0;
}
