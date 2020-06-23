/*
  graphics.h
*/

#ifndef GRAPHICS_H
#define GRAPHICS_H

// top left corner of main window
#define GRAPHICS_MAIN_X  2
#define GRAPHICS_MAIN_Y  18

// top left corner of left slave window
#define GRAPHICS_SLAVE_X 82
#define GRAPHICS_SLAVE_Y 112

// position of score counter
#define GRAPHICS_SCORE_X 80
#define GRAPHICS_SCORE_Y 26
#define GRAPHICS_SCORE_W 32
#define GRAPHICS_SCORE_H 15

// position of lines counter
#define GRAPHICS_LINES_X 120
#define GRAPHICS_LINES_Y 26
#define GRAPHICS_LINES_W 32
#define GRAPHICS_LINES_H 15

// top left corner of preview area
#define GRAPHICS_PREV_X 82
#define GRAPHICS_PREV_Y 50

// position of level counter
#define GRAPHICS_LEVEL_X 120
#define GRAPHICS_LEVEL_Y 50
#define GRAPHICS_LEVEL_W 32
#define GRAPHICS_LEVEL_H 15

extern void graphics_init(void);
extern void graphics_free(void);
extern void graphics_draw_main_block(Int16, Int16, UInt16);
extern void graphics_draw_slave_block(UInt16, Int16, Int16, UInt16);
extern void graphics_shift_main(UInt16);
extern void graphics_shift_slave(UInt16, UInt16);
extern void graphics_draw_score(UInt32);
extern void graphics_draw_lines(UInt16);
extern void graphics_draw_level(UInt16);
extern void graphics_draw_preview_block(Int16, Int16, UInt16);

#endif // GRAPHICS_H
