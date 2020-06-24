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

/**
 *
 * Elite - The New Kind.
 *
 * The code in this file has not been derived from the original Elite code.
 * Written by C.J.Pinder 1999/2000.
 *
 **/


#ifndef GFX_H
#define GFX_H

#include "config.h"
#include "elite.h"
#include "texts.h"

#define SCANNER_HEIGHT (40)
#define FUNKEY_HEIGHT  (10)

#define DSP_WIDTH  (160)
#define DSP_HEIGHT (160-SCANNER_HEIGHT-FUNKEY_HEIGHT)

#define DSP_CENTER (DSP_WIDTH/2)

#define TITLE_Y       (1)
#define TITLE_LINE_Y  (7)

#define COL_EXPAND_16(a) ((UInt32)a | (((UInt32)a)<<8))
#define COL_EXPAND_32(a) ((UInt32)a | (((UInt32)a)<<8) | (((UInt32)a)<<16) | (((UInt32)a)<<24))

#define GALAXY_SCALE(a)     ((((int)(a))*160l)/256l)
#define GALAXY_UNSCALE(a)   ((((int)(a))*256l)/160l)
#define GALAXY_Y_SCALE(a)   (9+((int)(a))*80/256)
#define GALAXY_Y_UNSCALE(a) (((int)(a-9))*256/80)

#define GFX_BTN_DOCKED  0
#define GFX_BTN_FLIGHT  1

extern UInt8 engine_idx;

extern UInt16 ticks;

extern Int16 *sintab;

#define GFX_SCALE	(1)
#define GFX_X_OFFSET	(0)
#define GFX_Y_OFFSET	(0)
#define GFX_X_CENTRE	(80)
#define GFX_Y_CENTRE	(DSP_HEIGHT/2)

#define GFX_VIEW_TX	(0)
#define GFX_VIEW_TY	(0)
#define GFX_VIEW_BX	(DSP_WIDTH-2)
#define GFX_VIEW_BY	(DSP_HEIGHT-2)

#ifndef PALM
#error This source is only Palm compatible!
#endif

#define GFX_CELL_HEIGHT         6	/* font cell height */
#define GFX_CELLS(n,p)          ((GFX_CELL_HEIGHT+p)*n)
#define TXT_ROW(a) (3+(a)*(GFX_CELL_HEIGHT+1))

#define GFX_COL_BLACK		255
#define GFX_COL_DARK_RED	161
#define GFX_COL_WHITE		0
#define GFX_COL_GOLD		0x73
#define GFX_COL_RED	       	125
#define GFX_COL_RED_1	       	0x8f

#define GFX_COL_GREY_0		217
#define GFX_COL_GREY_1		219
#define GFX_COL_GREY_2		220
#define GFX_COL_GREY_3		221
#define GFX_COL_GREY_4		222
#define GFX_COL_GREY_5		223

#define GFX_COL_BLUE_1		0x62
#define GFX_COL_BLUE_2		0x63
#define GFX_COL_BLUE_3		0x64
#define GFX_COL_BLUE_4		0x65

//#define GFX_COL_RED_3		1
//#define GFX_COL_RED_4		71

//#define GFX_COL_WHITE_2		242

//#define GFX_COL_YELLOW_1	37
//#define GFX_COL_YELLOW_2	39
//#define GFX_COL_YELLOW_3	89
#define GFX_COL_YELLOW_4	0x72
//#define GFX_COL_YELLOW_5	251

#define GFX_ORANGE_1		0x75
#define GFX_ORANGE_2		0x74
#define GFX_ORANGE_3		0x73

// #define GFX_COL_GREEN_1		192
#define GFX_COL_GREEN_1         0xd2
#define GFX_COL_GREEN_2         0xd3
#define GFX_COL_GREEN_3         0xd4
#define GFX_COL_GREEN_4         0xd5

#define GFX_COL_PINK_1		0x05

#define GFX_COL_BROWN           0x9f

// List of bitmaps
typedef enum { Btn_Empty=0, Btn_Status,  Btn_Galaxy,  Btn_Planet,
	       Btn_Market,  Btn_Invent,  Btn_Equip,   Btn_Launch,
	       Btn_Front,   Btn_Rear,    Btn_Left,    Btn_Right,
	       Btn_Hyper,   Btn_Jump,    Btn_Pilot,   Btn_GHyp,
	       Btn_Escape,  Btn_ECM,     Btn_Missile, 
	       bitmapTypeCount
} bitmap_id;

// sprites
#define SPRITE_SCANNER  0
#define SPRITE_BUTTONS  1
#define SPRITE_DICE     2
#define SPRITE_DECOMP   (SPRITE_DICE+1)  // number of sprite to decomp 
 
#define SPRITE_BLAKE    3
#define SPRITE_FORT     4
#define SPRITE_SBAY     5

#define SPRITE_NUM      6

extern char *vidbuff;
extern char *vidbase;

extern char *sprite_data[SPRITE_DECOMP];

extern UInt8 *graytab;
extern int pixperbyte;

// text alignment
#define ALIGN_LEFT   0
#define ALIGN_RIGHT  1
#define ALIGN_CENTER 2

#define NO_BUTTONS   -1
#define NO_MARKER    -2
#define SAME_MARKER  -3

#define PalmOS30   sysMakeROMVersion(3,0,0,sysROMStageRelease,0)
#define PalmOS35   sysMakeROMVersion(3,5,0,sysROMStageRelease,0)
UInt32 GetRomVersion(void);

char *get_text(int id);

Bool gfx_graphics_startup (void);
void gfx_graphics_shutdown (void);
void gfx_update_screen (void) SEC3D;
void gfx_plot_pixel (int x, int y, int col) SEC3D;
void gfx_draw_filled_circle (int cx, int cy, int radius, int circle_colour);
void gfx_draw_circle (int cx, int cy, int radius, int circle_colour);
void gfx_draw_line (int x1, int y1, int x2, int y2) SEC3D;
void gfx_draw_colour_line (int x1, int y1, int x2, int y2, int line_colour);
void gfx_xor_colour_line (int x1, int y1, int x2, int y2, int line_colour);
void gfx_draw_rectangle (int tx, int ty, int bx, int by, int col);
void gfx_fill_rectangle (int tx, int ty, int bx, int by, int col);
void gfx_display_text(int x, int y, int align, char *txt, int col);
void gfx_clear_display (void);
void gfx_clear_text_area (void);
void gfx_clear_area (int tx, int ty, int bx, int by);
void gfx_display_pretty_text (int tx, int ty, int bx, int by, int , char *txt);
void gfx_draw_scanner (void);
void gfx_set_clip_region (int tx, int ty, int bx, int by);
int  gfx_str_width (unsigned char *str);
void gfx_update_button_map(int marked) SEC3D;
void gfx_draw_sprite(Int16 bmp, int xpos, int ypos) SEC3D;
void gfx_draw_title(char *title, Bool erase);
void gfx_invert_screen(void);
void gfx_indirect_update(void);

#ifdef DEBUG
void gfx_toggle_update(void);
#endif

#endif
