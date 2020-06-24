/*
 * Elite - The New Kind, PalmOS port
 *
 * Reverse engineered from the BBC disk version of Elite.
 * Additional material by C.J.Pinder and T.Harbaum
 *
 * The original Elite code is (C) I.Bell & D.Braben 1984.
 * This version re-engineered in C by C.J.Pinder 1999-2001.
 *
 * email: <christian@newkind.co.uk>
 *
 * PalmOS port (c) 2002 by Till Harbaum <till@harbaum.org>
 *
 */
/*
 * options.h
 */

#ifndef OPTIONS_H
#define OPTIONS_H

#define OPTIONS_VERSION (3)

#define OPTION_Y  (TITLE_LINE_Y+6*GFX_CELL_HEIGHT/2)

#define SAVE_Y    (OPTION_Y + OPTIONS*(2+GFX_CELL_HEIGHT) + 3)

/* index of bool options */
#define OPTION_LEFTY   (0)
#define OPTION_SOUND   (1)
#define OPTION_REVERSE (2)
#define OPTION_INVERT  (3)    // grayscale invert and
#define OPTION_M550BUT (3)    // m550 share option
#define OPTIONS        (4)

#define SAVE_SLOTS    5   // save 5 commanders 

#define TXT_W       (22)
#define LOAD_TXT_X  (5)

#define SAVE_SLOT_X (LOAD_TXT_X + TXT_W + 4)
#define SAVE_SLOT_W (96)

#define SAVE_TXT_X  (SAVE_SLOT_X + SAVE_SLOT_W + 4)

#define GENERIC_PREFS 0
#define SAVE_PREFS_0  2

#define AUTO_SAVE_SLOT  -1

typedef struct {
  UInt8   version;
  Boolean bool[OPTIONS];
} EliteOptions;

extern EliteOptions options;

extern UInt8 rename_slot;

void draw_option_button(int x1, int y, int x2, char *txt, int c, int bg) SECDK;
void display_options(void) SECDK;
void load_options(void) SECDK;
void save_options(void) SECDK;
void do_option(int y) SECDK;
void do_save_slot(int y, int x) SECDK;
void draw_save_slot(int i) SECDK;
void do_new_cmdr(void) SECDK;
#endif
