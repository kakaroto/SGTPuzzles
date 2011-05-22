/*
 * ps3menu.c : PS3 Menu drawing API
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <cairo/cairo.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

enum {
  PS3_MENU_ALIGN_TOP = 0x01,
  PS3_MENU_ALIGN_MIDDLE = 0x02,
  PS3_MENU_ALIGN_BOTTOM = 0x04,
  PS3_MENU_ALIGN_LEFT = 0x10,
  PS3_MENU_ALIGN_CENTER = 0x20,
  PS3_MENU_ALIGN_RIGHT = 0x40,
};

/**
 * Ps3MenuTextAlignment:
 * @PS3_MENU_ALIGN_TOP_LEFT: Align the text to the top left corner of the menu
 * item
 * @PS3_MENU_ALIGN_TOP_CENTER: Align the text to the top and the center of the
 * menu item
 * @PS3_MENU_ALIGN_TOP_RIGHT: Align the text to the top right corner of the
 * menu item
 * @PS3_MENU_ALIGN_MIDDLE_LEFT: Align the text to the center vertically, and
 * aligned to the left horizontally in the menu item
 * @PS3_MENU_ALIGN_MIDDLE_CENTER:Align the text to the center vertically, and
 * aligned to the center horizontally in the menu item
 * @PS3_MENU_ALIGN_MIDDLE_RIGHT:Align the text to the center vertically, and
 * aligned to the right horizontally menu item
 * @PS3_MENU_ALIGN_BOTTOM_LEFT: Align the text to the bottom left corner of
 * the menu item
 * @PS3_MENU_ALIGN_BOTTOM_CENTER: Align the text to the bottom and the center
 * of the menu item
 * @PS3_MENU_ALIGN_BOTTOM_RIGHT: Align the text to the bottom right corner of
 * the menu item
 *
 * Define how the text should be aligned and where to position it in a menu
 * item's button
 */
typedef enum {
  PS3_MENU_ALIGN_TOP_LEFT = PS3_MENU_ALIGN_TOP | PS3_MENU_ALIGN_LEFT,
  PS3_MENU_ALIGN_TOP_CENTER = PS3_MENU_ALIGN_TOP | PS3_MENU_ALIGN_CENTER,
  PS3_MENU_ALIGN_TOP_RIGHT = PS3_MENU_ALIGN_TOP | PS3_MENU_ALIGN_RIGHT,
  PS3_MENU_ALIGN_MIDDLE_LEFT = PS3_MENU_ALIGN_MIDDLE | PS3_MENU_ALIGN_LEFT,
  PS3_MENU_ALIGN_MIDDLE_CENTER = PS3_MENU_ALIGN_MIDDLE | PS3_MENU_ALIGN_CENTER,
  PS3_MENU_ALIGN_MIDDLE_RIGHT = PS3_MENU_ALIGN_MIDDLE | PS3_MENU_ALIGN_RIGHT,
  PS3_MENU_ALIGN_BOTTOM_LEFT = PS3_MENU_ALIGN_BOTTOM | PS3_MENU_ALIGN_LEFT,
  PS3_MENU_ALIGN_BOTTOM_CENTER = PS3_MENU_ALIGN_BOTTOM | PS3_MENU_ALIGN_CENTER,
  PS3_MENU_ALIGN_BOTTOM_RIGHT = PS3_MENU_ALIGN_BOTTOM | PS3_MENU_ALIGN_RIGHT,
} Ps3MenuTextAlignment;

/**
 * Ps3MenuInput:
 * @PS3_MENU_INPUT_UP: Move the selection one row up
 * @PS3_MENU_INPUT_DOWN: Move the selection one row down
 * @PS3_MENU_INPUT_LEFT: Move the selection one column to the left
 * @PS3_MENU_INPUT_RIGHT: Move the selection one column to the right
 *
 * Defines the four possible input that the menu can handle for changing the
 * selection
 */
typedef enum {
  PS3_MENU_INPUT_UP,
  PS3_MENU_INPUT_DOWN,
  PS3_MENU_INPUT_LEFT,
  PS3_MENU_INPUT_RIGHT,
} Ps3MenuInput;

/**
 * Ps3MenuImagePosition:
 * @PS3_MENU_IMAGE_POSITION_LEFT: Position the image on the left of the button
 * @PS3_MENU_IMAGE_POSITION_RIGHT: Position the image on the right of the
 * button
 * @PS3_MENU_IMAGE_POSITION_TOP: Position the image on the top of the button
 * @PS3_MENU_IMAGE_POSITION_BOTTOM: Position the image on the bottom of the
 * button
 *
 * Defines the position of an image on a menu item. The image will be scaled
 * so that it's height fits the menu item's height (for left/right positions)
 * or so that it's width fits the menu item's width (for top/bottom positions)
 */
typedef enum {
  PS3_MENU_IMAGE_POSITION_LEFT,
  PS3_MENU_IMAGE_POSITION_RIGHT,
  PS3_MENU_IMAGE_POSITION_TOP,
  PS3_MENU_IMAGE_POSITION_BOTTOM,
} Ps3MenuImagePosition;

/**
 * Ps3MenuColor:
 * @red: The red value
 * @green: The green value
 * @blue: The blue value
 * @alpha: The opacity
 *
 * A structure representing a single color, with values between 0.0 and 1.0
 */
typedef struct {
  float red;
  float green;
  float blue;
  float alpha;
} Ps3MenuColor;

/**
 * Ps3MenuRectangle:
 * @x: the x position of the rectangle
 * @x: the y position of the rectangle
 * @x: the width of the rectangle
 * @x: the height of the rectangle
 *
 * A structure representing a rectangle
 */
typedef struct {
  int x;
  int y;
  int width;
  int height;
} Ps3MenuRectangle;


/**
 * PS3_MENU_DEFAULT_IPAD_X:
 *
 * Default horizontal internal padding of a menu's item
 */
#define PS3_MENU_DEFAULT_IPAD_X 10

/**
 * PS3_MENU_DEFAULT_IPAD_Y:
 *
 * Default vertical internal padding of a menu's item
 */
#define PS3_MENU_DEFAULT_IPAD_Y 5

/**
 * PS3_MENU_DEFAULT_PAD_X:
 *
 * Default horizontal padding of a menu's item
 */
#define PS3_MENU_DEFAULT_PAD_X 3

/**
 * PS3_MENU_DEFAULT_PAD_Y:
 *
 * Default vertical padding of a menu's item
 */
#define PS3_MENU_DEFAULT_PAD_Y 2

/**
 * PS3_MENU_DEFAULT_TEXT_COLOR:
 *
 * The default color to use for drawing text
 */
#define PS3_MENU_DEFAULT_TEXT_COLOR (Ps3MenuColor) {1.0, 1.0, 1.0, 1.0}

typedef struct _Ps3MenuItem Ps3MenuItem;
typedef struct _Ps3Menu Ps3Menu;

/**
 * Ps3MenuDrawItemCb:
 * @menu: The #Ps3Menu being drawn
 * @item: The #Ps3MenuItem being drawn
 * @selected: #TRUE if this item is currently selected, #FALSE otherwise
 * @cr: The Cairo context to draw into
 * @x: The X position where to draw in the Cairo surface.
 * @y: The Y position where to draw in the Cairo surface.
 * @user_data: Any user provided data for the callback
 *
 * This callback is called whenever the #Ps3Menu needs to draw a menu item.
 * The button's clipping area will be already set, and you need to draw on
 * the (@x, @y) coordinate, up to(@x + @item->width - @item->ipad_x,
 * @y + @item->height - @item->ipad_y).
 *
 */
typedef int (*Ps3MenuDrawItemCb) (Ps3Menu *menu, Ps3MenuItem *item,
    int selected, cairo_t *cr, int x, int y, void *user_data);

/**
 * Ps3MenuItem:
 * @image: An image to draw in the menu
 * @image_position: Where to place the image on the menu
 * @text: The text to show
 * @text_size: The font size of the text to show
 * @text_color: The color to use for the menu item's text
 * @alignment: The alignment of the text
 * @draw_cb: The drawing callback when the item needs to be drawn
 * @draw_data: User data for the drawing callback
 * @width: Item width
 * @height: Item height
 * @ipad_x: Internal horizontal padding
 * @ipad_y: Internal vertical padding
 * @enabled: Wether or not the item is enabled or not (greyed)
 * @bg_image: Background image for non-selected item
 * @bg_sel_image: Background image for selected item
 * @index: The index of this item in the #Ps3Menu. DO NOT modify this value.
 *
 * A structure representing a menu item, each attribute can be configured by
 * modifying the structure.
 */
struct _Ps3MenuItem {
  cairo_surface_t *image;
  Ps3MenuImagePosition image_position;
  char *text;
  int text_size;
  Ps3MenuColor text_color;
  Ps3MenuTextAlignment alignment;
  Ps3MenuDrawItemCb draw_cb;
  void *draw_data;
  int width;
  int height;
  int ipad_x;
  int ipad_y;
  int enabled;
  cairo_surface_t *bg_image;
  cairo_surface_t *bg_sel_image;
  /* Private - you can read, but don't modify */
  int index;
};

/**
 * Ps3Menu:
 * @surface: The surface where the menu gets drawn
 * @rows: Total rows to show in the menu
 * @columns: Total columns in the menu
 * @default_item_width: The default width of each menu item
 * @default_item_height: The default height of each menu item
 * @pad_x: The horizontal padding between items
 * @pad_y: The vertical padding between items
 * @dropshadow_radius: The radius for a dropshadow to apply to all items. 0 for none.
 * @bg_image: The default background image for non selected items
 * @bg_sel_image: The default background image for selected items
 * @disabled_image: An image to overlay on top of disabled items
 * @nitems: Number of items in the menu
 * @items: The items in the menu
 * @selection: Currently selected item index
 * @start_item: The first item to be drawn (!= 0 if scrolled)
 * @dropshadow: A surface with the dropshadow to apply to all items.
 */
struct _Ps3Menu {
  cairo_surface_t *surface;
  int rows;
  int columns;
  int default_item_width;
  int default_item_height;
  int pad_x;
  int pad_y;
  int dropshadow_radius;
  cairo_surface_t *bg_image;
  cairo_surface_t *bg_sel_image;
  cairo_surface_t *disabled_image;
  /* Private - can read but don't modify */
  int nitems;
  Ps3MenuItem *items;
  int selection;
  int start_item;
  cairo_surface_t *dropshadow;
};

/**
 * ps3_menu_new:
 * @surface: The surface associated with the menu, where it will get drawn. This
 * surface will be referenced so if you do not want to hold a reference to it,
 * then call cairo_surface_destroy() on it
 * @rows: The number of rows to show in the menu, or -1 for infinite
 * @columns: The number of columns to show in the menu, or -1 for infinite
 * @default_item_width: The default width of each menu item
 * @default_item_height: The default height of each menu item
 *
 * Create a new #Ps3Menu with default settings and configured to show a
 * specific number of rows or columns.
 * If a row or column is set to -1 (infinite), then the menu will be scrollable
 * in that direction.
 * Both @rows and @columns cannot be set to -1 at the same time or an error is
 * returned.
 *
 <note>
   <para>
   An item's width and height can always be modified after adding the item to
   the menu, however, if the width and height of each item is different and the
   menu has more than one row or column, then the behavior is undetermined.
   It is best to use the same width/height for all the items in the menu.
   </para>
 </note>
 *
 * Returns: the newly created #Ps3Menu or #NULL in case of an error.
 */
Ps3Menu *ps3_menu_new (cairo_surface_t *surface, int rows, int columns,
    int default_item_width, int default_item_height);

/**
 * ps3_menu_new_full:
 * @surface: The surface associated with the menu, where it will get drawn. This
 * surface will be referenced so if you do not want to hold a reference to it,
 * then call cairo_surface_destroy() on it
 * @rows: The number of rows to show in the menu, or -1 for infinite
 * @columns: The number of columns to show in the menu, or -1 for infinite
 * @default_item_width: The default width of each menu item
 * @default_item_height: The default height of each menu item
 * @pad_x: The horizontal padding between items
 * @pad_y: The vertical padding between items
 * @dropshadow_radius: The radius for a dropshadow to add to the menus.
 * @bg_image: The default background image for non selected items
 * @bg_sel_image: The default background image for selected items
 * @disabled_overlay: An image to overlay on top of disabled items
 *
 * Create a new #Ps3Menu with all settings specified. This is a more complete
 * version of ps3_menu_new() that allows you to specify each parameter for
 * an increased sense of customization.
 * Specifying 0 for the dropshadow radius means that it will be disabled. Note that
 * the dropshadow will be unique and applied to all items, so the form and the size
 * of the @bg_image and @bg_sel_image as well as each item's sizes must be the same
 * otherwise the result is to be unexpected behavior.
 * If @bg_image, @bg_sel_image, or @disabled_overlay are #NULL, then the default
 * will be used.
 *
 * Returns: the newly created #Ps3Menu or #NULL in case of an error.
 */
Ps3Menu *ps3_menu_new_full (cairo_surface_t *surface, int rows, int columns,
    int default_item_width, int default_item_height, int pad_x, int pad_y,
    int dropshadow_radius, cairo_surface_t *bg_image,
    cairo_surface_t *bg_sel_image, cairo_surface_t *disabled_overlay);

/**
 * ps3_menu_add_item:
 * @menu: The #Ps3Menu to add the item to
 * @text: The middle-left aligned text to show in the item
 * @text_size: the size of the font for showing the text, or -1 for automatic
 *
 * Append a new item to the @menu.
 * If the number of rows is infinite or if both the number of rows and columns
 * are finite, the items are populated from left to right on each row,
 * then subsequent rows are filled.
 * If the number of columns is infinite, the items are populated from top to
 * bottom on each column, then subsequent columns are filled.
 *
 * Returns: A unique identifier (the item's index) representing this item or
 * -1 in case of an error (no room left).
 * The created item can then be accessed using menu->items[index].
 */
int ps3_menu_add_item (Ps3Menu *menu, const char *text, int text_size);

/**
 * ps3_menu_add_item_full:
 * @menu: The #Ps3Menu to add the item to
 * @image: An image to draw in the menu, or #NULL
 * @image_position: Where to place the image on the menu
 * @text: The text to show in the item
 * @text_size: The font size of the text to show, or -1 for automatic
 * @text_color: The color to use for the menu item's text
 * @alignment: The alignment of the text
 * @width: Item width
 * @height: Item height
 * @ipad_x: Internal horizontal padding
 * @ipad_y: Internal vertical padding
 * @enabled: Wether or not the item is enabled or not (greyed)
 * @bg_image: Background image for non-selected item, or #NULL
 * @bg_sel_image: Background image for selected item, or #NULL
 * @draw_cb: The drawing callback when the item needs to be drawn, or #NULL
 * @draw_data: User data for the drawing callback
 *
 * Append a new item to the @menu.
 * This is a more complete version of ps3_menu_add_item() where you can
 * specify all the options that you would normally modify directly in the
 * #Ps3MenuItem. The same rules apply as with ps3_menu_add_item().
 * The @draw_cb is the callback used for drawing the item, it is usually set
 * to an internal function that does the drawing, but it can be overridden if
 * you want to draw the item yourself using some magical algorithm. You can set
 * the @bg_image, @bg_sel_image and @draw_cb to #NULL if you want the defaults to
 * be used, as if calling ps3_menu_add_item().
 *
 * Returns: A unique identifier (the item's index) representing this item or
 * -1 in case of an error (no room left).
 * The created item can then be accessed using menu->items[index].
 */
int ps3_menu_add_item_full (Ps3Menu *menu, cairo_surface_t *image,
    Ps3MenuImagePosition image_position, const char *text, int text_size,
    Ps3MenuColor text_color, Ps3MenuTextAlignment alignment,
    int width, int height, int ipad_x, int ipad_y, int enabled,
    cairo_surface_t *bg_image, cairo_surface_t *bg_sel_image,
    Ps3MenuDrawItemCb draw_cb, void *draw_data);

/**
 * ps3_menu_set_item_image:
 * @menu: The menu containing the item
 * @item_index: The item index in the menu to which to set the image
 * @image: An image to overlay to the item or #NULL.
 * @image_position: The position of the @image in the item.
 *
 * Sets @image to be overlayed on the item, positioned at @image_position.
 * This function will use @image and will scale it to fit the current item's
 * width or height (depending on the position), taking into account the
 * internal padding of the item.
 * It is preferable to use this function since it will scale the image,
 * If you set the image surface directly into @item, then it will simply
 * be overlayed on top of the item without any kind of scaling.
 * After this function returns, @image will not be used, and can be destroyed.
 */
void ps3_menu_set_item_image (Ps3Menu *menu, int item_index, cairo_surface_t *image,
    Ps3MenuImagePosition image_position);

/**
 * ps3_menu_handle_input:
 * @menu: The menu to change its selection
 * @input: The input received by the Pad
 * @bbox: The dirty rectangle of the surface
 *
 * Handle a controller input, causing a possible redraw of the
 * surface. If a drawing operation is needed, the @bbox rectangle will be
 * updated with the area that is marked dirty.
 *
 * Returns: The id of the currently selected menu item
 */
int ps3_menu_handle_input (Ps3Menu *menu, Ps3MenuInput input,
    Ps3MenuRectangle *bbox);

/**
 * ps3_menu_set_selection:
 * @menu: The menu to change its selection
 * @id: The id (as returned by ps3_menu_add_item()) to set as selected
 * @bbox: The dirty rectangle of the surface
 *
 * Changes the current selection of the menu, causing a possible redraw of the
 * surface. If a drawing operation is needed, the @bbox rectangle will be
 * updated with the area that is marked dirty
 */
void ps3_menu_set_selection (Ps3Menu *menu, int id, Ps3MenuRectangle *bbox);

/**
 * ps3_menu_redraw:
 * @menu: The menu to draw
 *
 * Force a redraw of the entire menu on the entire surface given.
 */
void ps3_menu_redraw (Ps3Menu *menu);

/**
 * ps3_menu_get_surface:
 * @menu: The menu
 *
 * Get the Cairo surface where the menu is drawn. This increases the reference
 * count of the surface, so you must call cairo_surface_destroy() once you are
 * done using it.
 *
 * Returns: The Cairo surface where the menu is drawn with its reference count
 * incremented.
 */
cairo_surface_t *ps3_menu_get_surface (Ps3Menu *menu);

/**
 * ps3_menu_free:
 * @menu: The menu to free
 *
 * Frees and destroys any memory used by the menu.
 * If any variable was modified in the #Ps3Menu or one of its #Ps3MenuItem then
 * it will be freed, so make sure that its reference count has been increased or
 * that it is not used afterwards.
 */
void ps3_menu_free (Ps3Menu *menu);

/**
 * ps3_menu_create_default_background:
 * @width: The width of the surface to create
 * @height: The height of the surface to create
 * @red: The red component of the top color in the gradient
 * @green: The green component of the top color in the gradient
 * @bllue: The blue component of the top color in the gradient
 *
 * This utility function is used to create the default background image to be
 * used for the menu items. It is called internally by ps3_menu_new() but it
 * can be used to generate a new background with a different color.
 * It will create a gradient from the color specified as argument, going to
 * lighter colors towards the bottom, and make the button round edged, as well as
 * add a glossy effect to it.
 *
 * Returns: A cairo surface with the generated background
 */
cairo_surface_t *ps3_menu_create_default_background (int width, int height,
    float red, float green, float blue);
