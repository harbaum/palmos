/*
  graphics.c
*/

#include <PalmOS.h>
#include "Rsc.h"
#include "debug.h"
#include "graphics.h"
#include "tetris.h"

// List of bitmaps
typedef enum { 
  block_id_red=0,   block_id_green,  block_id_blue,
  block_id_yellow,  block_id_cyan,   block_id_magenta,
  block_id_grey,

  block_sid_red,    block_sid_green, block_sid_blue,
  block_sid_yellow, block_sid_cyan,  block_sid_magenta,
  block_sid_grey,

  bitmapTypeCount
} bitmap_id;

const UInt16 bitmaps[] = { 
  block_red,       block_green,    block_blue,
  block_yellow,    block_cyan,     block_magenta,
  block_grey,
  block_sm_red,    block_sm_green, block_sm_blue,
  block_sm_yellow, block_sm_cyan,  block_sm_magenta,
  block_sm_grey
};

static MemHandle  ObjectBitmapHandles[bitmapTypeCount];
static BitmapPtr  ObjectBitmapPtr[bitmapTypeCount];

void graphics_init(void) {
  UInt16 i;

  DEBUG_PRINTF("graphics init\n");

  // Keep the Object graphics locked because they are frequently used
  for (i = 0; i < bitmapTypeCount; i++) {
    ObjectBitmapHandles[i] = (MemHandle)DmGetResource(bitmapRsc, bitmaps[i]);
    ObjectBitmapPtr[i] = MemHandleLock(ObjectBitmapHandles[i]);
  }
}

void graphics_free(void) {
  UInt16 i;

  DEBUG_PRINTF("graphics free\n");

  // Unlock and release the locked bitmaps
  for (i = 0; i < bitmapTypeCount; i++) {
    MemPtrUnlock(ObjectBitmapPtr[i]);
    DmReleaseResource(ObjectBitmapHandles[i]);
  }
}

/* draw a block */
void graphics_draw_main_block(Int16 x, Int16 y, UInt16 color) {
  if(y<0) return;

  if(color == TETRIS_EMPTY) {
    /* erase block */
    RectangleType rect;
    rect.topLeft.x = GRAPHICS_MAIN_X + 7*x;  
    rect.topLeft.y = GRAPHICS_MAIN_Y + 7*y;
    rect.extent.x = 7; rect.extent.y = 7;
    WinEraseRectangle(&rect,0);
  } else {
    /* draw bitmap */
    WinDrawBitmap(ObjectBitmapPtr[block_id_red+color-1], 
		  GRAPHICS_MAIN_X + 7*x, 
		  GRAPHICS_MAIN_Y + 7*y);
  }
}

/* draw a slave block */
void graphics_draw_slave_block(UInt16 slave, Int16 x, Int16 y, UInt16 col) {
  UInt16 sx = GRAPHICS_SLAVE_X + 24*(slave%3);

  if(y<0) return;

  if(col == TETRIS_EMPTY) {
    /* erase block */
    RectangleType rect;
    rect.topLeft.x = sx + 2*x;  
    rect.topLeft.y = GRAPHICS_SLAVE_Y + 2*y;
    rect.extent.x = 2; rect.extent.y = 2;
    WinEraseRectangle(&rect,0);
  } else {
    /* draw bitmap */
    WinDrawBitmap(ObjectBitmapPtr[block_sid_red+col-1], 
		  sx + 2*x, GRAPHICS_SLAVE_Y + 2*y);
  }
}

void graphics_shift_main(UInt16 row) {
  RectangleType rect;
  WinHandle win = WinGetDrawWindow();

  /* move display */
  rect.topLeft.x = GRAPHICS_MAIN_X;
  rect.topLeft.y = GRAPHICS_MAIN_Y;
  rect.extent.x  = 10*7;
  rect.extent.y  = 7*row;
  
  WinCopyRectangle(win, win, &rect, 
		   GRAPHICS_MAIN_X, GRAPHICS_MAIN_Y+7, winPaint);
}

void graphics_shift_slave(UInt16 slave, UInt16 row) {
  UInt16 sx = GRAPHICS_SLAVE_X + 24*(slave%3);
  RectangleType rect;
  WinHandle win = WinGetDrawWindow();

  /* move display */
  rect.topLeft.x = sx;
  rect.topLeft.y = GRAPHICS_SLAVE_Y;
  rect.extent.x  = 10*2;
  rect.extent.y  = 2*row;
  
  WinCopyRectangle(win, win, &rect, sx, GRAPHICS_SLAVE_Y+2, winPaint);
}

void graphics_draw_score(UInt32 score) {
  RectangleType rect;
  char str[10];

  DEBUG_PRINTF("graphics_draw_score(%ld)\n", score);

  if(!score) {
    rect.topLeft.x = GRAPHICS_SCORE_X;
    rect.topLeft.y = GRAPHICS_SCORE_Y;
    rect.extent.x  = GRAPHICS_SCORE_W;
    rect.extent.y  = GRAPHICS_SCORE_H;
    WinEraseRectangle(&rect, 0);
  }

  FntSetFont(largeBoldFont);
  StrIToA(str, score);
  WinDrawChars(str, StrLen(str), GRAPHICS_SCORE_X + GRAPHICS_SCORE_W/2 - 
	       FntLineWidth(str, StrLen(str))/2, GRAPHICS_SCORE_Y);
}

void graphics_draw_lines(UInt16 lines) {
  RectangleType rect;
  char str[10];

  DEBUG_PRINTF("graphics_draw_lines(%d)\n", lines);

  if(!lines) {
    rect.topLeft.x = GRAPHICS_LINES_X;
    rect.topLeft.y = GRAPHICS_LINES_Y;
    rect.extent.x  = GRAPHICS_LINES_W;
    rect.extent.y  = GRAPHICS_LINES_H;
    WinEraseRectangle(&rect, 0);
  }

  FntSetFont(largeBoldFont);
  StrIToA(str, lines);
  WinDrawChars(str, StrLen(str), GRAPHICS_LINES_X + GRAPHICS_LINES_W/2 - 
	       FntLineWidth(str, StrLen(str))/2, GRAPHICS_LINES_Y);
}

void graphics_draw_level(UInt16 level) {
  RectangleType rect;
  char str[10];

  DEBUG_PRINTF("graphics_draw_level(%d)\n", level);

  if(level == 1) {
    rect.topLeft.x = GRAPHICS_LEVEL_X;
    rect.topLeft.y = GRAPHICS_LEVEL_Y;
    rect.extent.x  = GRAPHICS_LEVEL_W;
    rect.extent.y  = GRAPHICS_LEVEL_H;
    WinEraseRectangle(&rect, 0);
  }

  FntSetFont(largeBoldFont);
  StrIToA(str, level);

  WinDrawChars(str, StrLen(str), GRAPHICS_LEVEL_X + GRAPHICS_LEVEL_W/2 -
	       FntLineWidth(str, StrLen(str))/2, GRAPHICS_LEVEL_Y);
}

void graphics_draw_preview_block(Int16 x, Int16 y, UInt16 color) {
  /* x/y position is given in half block lengths, to allow nice */
  /* centering of preview block */
  x = (7*x)/2;
  y = (7*y)/2;

  /* tetris_empty erases whole 4x2 preview area */
  if(color == TETRIS_EMPTY) {
    /* erase block */
    RectangleType rect;
    rect.topLeft.x = GRAPHICS_PREV_X;  
    rect.topLeft.y = GRAPHICS_PREV_Y;
    rect.extent.x = 4*7; rect.extent.y = 2*7;
    WinEraseRectangle(&rect,0);
  } else {
    /* draw bitmap */
    WinDrawBitmap(ObjectBitmapPtr[block_id_red+color-1], 
		  GRAPHICS_PREV_X + x, 
		  GRAPHICS_PREV_Y + y);
  }
}
