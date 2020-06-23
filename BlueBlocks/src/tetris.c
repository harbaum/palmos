/*
  tetris.c

  basic tetris game routines
*/

#include <PalmOS.h>
#include "graphics.h"
#include "tetris.h"
#include "debug.h"

/* the map is bigger than the actual game area, since */
/* this allows us the use the game internal collision */
/* detection for the borders as well */
static UInt8 map[TETRIS_HEIGHT+4][TETRIS_WIDTH+4];

/* mapping of stones under all four angles */
static const Int8 stones[][4][4][2]= {  {  
  { {-1, 0},{ 0, 0},{ 1, 0},{ 2, 0} },   /* red     */
  { { 0,-1},{ 0, 0},{ 0, 1},{ 0, 2} },   /*  #*##   */
  { {-1, 0},{ 0, 0},{ 1, 0},{ 2, 0} },   /*         */
  { { 0,-1},{ 0, 0},{ 0, 1},{ 0, 2} }    /*         */
}, {  
  { {-1, 1},{-1, 0},{ 0, 0},{ 1, 0} },   /* green   */
  { {-1,-1},{ 0,-1},{ 0, 0},{ 0, 1} },   /*  #*#    */
  { {-1, 0},{ 0, 0},{ 1, 0},{ 1,-1} },   /*  #      */
  { { 0,-1},{ 0, 0},{ 0, 1},{ 1, 1} }    /*         */
}, {  
  { { 0, 0},{ 1, 0},{ 0, 1},{ 1, 1} },   /* blue    */
  { { 0, 0},{ 1, 0},{ 0, 1},{ 1, 1} },   /*   *#    */
  { { 0, 0},{ 1, 0},{ 0, 1},{ 1, 1} },   /*   ##    */
  { { 0, 0},{ 1, 0},{ 0, 1},{ 1, 1} }    /*         */
}, {  
  { {-1, 0},{ 0, 0},{ 1, 0},{ 1, 1} },   /* yellow  */
  { { 0,-1},{ 0, 0},{ 0, 1},{-1, 1} },   /*  #*#    */
  { {-1,-1},{-1, 0},{ 0, 0},{ 1, 0} },   /*    #    */
  { { 1,-1},{ 0,-1},{ 0, 0},{ 0, 1} }    /*         */
}, {  
  { {-1, 0},{ 0, 0},{ 0, 1},{ 1, 1} },   /* cyan    */
  { { 1,-1},{ 1, 0},{ 0, 0},{ 0, 1} },   /*  #*     */
  { {-1, 0},{ 0, 0},{ 0, 1},{ 1, 1} },   /*   ##    */
  { { 1,-1},{ 1, 0},{ 0, 0},{ 0, 1} }    /*         */
}, {  
  { { 1, 0},{ 0, 0},{ 0, 1},{-1, 1} },   /* magenta */
  { { 0,-1},{ 0, 0},{ 1, 0},{ 1, 1} },   /*   *#    */
  { { 1, 0},{ 0, 0},{ 0, 1},{-1, 1} },   /*  ##     */
  { { 0,-1},{ 0, 0},{ 1, 0},{ 1, 1} }    /*         */
}, {  
  { {-1, 0},{ 0, 0},{ 1, 0},{ 0, 1} },   /* grey    */
  { { 0,-1},{ 0, 0},{-1, 0},{ 0, 1} },   /*  #*#    */
  { {-1, 0},{ 0, 0},{ 0,-1},{ 1, 0} },   /*   #     */
  { { 0,-1},{ 0, 0},{ 1, 0},{ 0, 1} }    /*         */
} };  

/* offset to center preview */
static const UInt8 prev_offset[][2] = {
  { 2, 1}, { 3, 0}, { 2, 0}, { 3, 0}, { 3, 0}, { 3, 0}, { 3, 0}};

static tetris_block_state_t block;
static tetris_game_t game;

/* update main screen */
void tetris_refresh_main(void) {
  UInt16 x,y;

  for(y=0;y<TETRIS_HEIGHT;y++)
    for(x=0;x<TETRIS_WIDTH;x++)
      graphics_draw_main_block(x,y,map[y+2][x+2]);
}

void tetris_refresh_slave(UInt16 slave) {
  UInt16 x,y;

  for(y=0;y<TETRIS_HEIGHT;y++)
    for(x=0;x<TETRIS_WIDTH;x++)
      graphics_draw_slave_block(slave,x,y,map[y+2][x+2]);
}

#define X 0
#define Y 1

/* draw a preview block */
void tetris_preview_block(UInt16 block) {
  Int8 *stone = (Int8*)stones[block][0];
  UInt16 i;

  /* erase preview area */
  graphics_draw_preview_block(0, 0, TETRIS_EMPTY);

  /* draw preview */
  for(i=0;i<4;i++) {
    graphics_draw_preview_block(
      prev_offset[block][X] + 2*stone[2*i+X], 
      prev_offset[block][Y] + 2*stone[2*i+Y],
      block+1);
  }
}

Boolean tetris_new_block(void) {
  UInt16  i;
  Int8 *stone;

  DEBUG_PRINTF("tetris_new_block()\n");

  /* init new block */
  block.x    = (TETRIS_WIDTH/2)-1;
  block.y    = 0;
  block.rot  = 0;
  block.type = block.next;
  block.next = SysRandom(0)%7;
  stone      = (Int8*)stones[block.type][block.rot];

  if((game.blocks++ == TETRIS_BLOCK_INCR) && (game.level<10)) {
    game.level++;
    game.blocks = 0;
    graphics_draw_level(game.level);
  }

  DEBUG_PRINTF("new block type %d\n", block.type);

  tetris_preview_block(block.next);

  /* check if block can be placed */
  for(i=0;i<4;i++) {
    if(map[2 + block.y + stone[2*i+Y]][2 + block.x + stone[2*i+X]] 
       != TETRIS_EMPTY)
      return false;
  }

  /* draw block */
  for(i=0;i<4;i++) {
    map[2 + block.y + stone[2*i+Y]][2 + block.x + stone[2*i+X]] = block.type+1;

    graphics_draw_main_block(block.x + stone[2*i+X], block.y + stone[2*i+Y],
			     block.type+1);
    
    { int j; for(j=0;j<3;j++)
      graphics_draw_slave_block(j, block.x + stone[2*i+X],
				block.y + stone[2*i+Y], block.type+1);
    }
  }
  return true;
}

Boolean tetris_step(Int16 x_diff, Int16 y_diff, Int16 rot_diff) {
  Int16 i, remove[4][2], add[4][2];
  Int8 *stone = (Int8*)stones[block.type][block.rot];
  
  DEBUG_PRINTF("tetris_step(%d,%d,%d)\n", x_diff, y_diff, rot_diff);

  /* score for singe step */
  if(y_diff == 1) game.score += TETRIS_SCORE_STEP;

  /* check which blocks will be removed */
  for(i=0;i<4;i++) {
    map[2 + block.y + stone[2*i+Y]]
       [2 + block.x + stone[2*i+X]] = TETRIS_EMPTY;

    /* remember which block has been removed */
    remove[i][Y] = block.y + stone[2*i+Y];
    remove[i][X] = block.x + stone[2*i+X];
  }

  /* advance block */
  block.x  += x_diff;  
  block.y  += y_diff;
  block.rot = (block.rot + rot_diff)&3;
  stone = (Int8*)stones[block.type][block.rot];

  /* check if new position causes collision */
  for(i=0;i<4;i++) {
    if(map[2 + block.y + stone[2*i+Y]][2 + block.x + stone[2*i+X]]
       != TETRIS_EMPTY) {

      DEBUG_PRINTF("collision at new pos\n");

      /* re-add removed blocks and quit */
      for(i=0;i<4;i++)
	map[2 + remove[i][Y]][2 + remove[i][X]] = block.type+1;

      /* undo movement */
      block.x  -= x_diff;  
      block.y  -= y_diff;
      block.rot = (block.rot - rot_diff)&3;

      return false;
    }
  }
  
  /* place new block */
  for(i=0;i<4;i++) {
    map[2 + block.y + stone[2*i+Y]][2 + block.x + stone[2*i+X]] = block.type+1;
      
    /* save position of new blocks */
    add[i][Y] = block.y + stone[2*i+Y];
    add[i][X] = block.x + stone[2*i+X];
  }

  /* only draw blocks that actually changes */
  for(i=0;i<4;i++) {
    /* has a block been removed and not restored? */
    if(!((remove[i][X] == add[0][X]) && (remove[i][Y] == add[0][Y])) &&
       !((remove[i][X] == add[1][X]) && (remove[i][Y] == add[1][Y])) &&
       !((remove[i][X] == add[2][X]) && (remove[i][Y] == add[2][Y])) &&
       !((remove[i][X] == add[3][X]) && (remove[i][Y] == add[3][Y]))) {

      graphics_draw_main_block(remove[i][X], remove[i][Y], TETRIS_EMPTY);

      { int j; for(j=0;j<3;j++) {
	graphics_draw_slave_block(j, remove[i][X], remove[i][Y], TETRIS_EMPTY);
      } }
    }
      
    /* has a block been added where none has been? */
    if(!((add[i][X] == remove[0][X]) && (add[i][Y] == remove[0][Y])) &&
       !((add[i][X] == remove[1][X]) && (add[i][Y] == remove[1][Y])) &&
       !((add[i][X] == remove[2][X]) && (add[i][Y] == remove[2][Y])) &&
       !((add[i][X] == remove[3][X]) && (add[i][Y] == remove[3][Y]))) {
      
      graphics_draw_main_block(add[i][X], add[i][Y], block.type+1);
      
      { int j; for(j=0;j<3;j++) {
	graphics_draw_slave_block(j, add[i][X], add[i][Y], block.type+1);
      } }
    }
  }
  return true;
}
  
void tetris_init(void) {
  UInt16 x,y;

  DEBUG_PRINTF("tetris_init()\n");

  /* zero score */
  game.score = 0;
  graphics_draw_score(game.score);

  game.lines = 0;
  graphics_draw_lines(game.lines);

  game.level = 1;
  game.blocks = 0;
  graphics_draw_level(game.level);

  /* the "next" to be used for the first block */
  block.next = SysRandom(0)%7;
  tetris_preview_block(block.next);

  /* empty map */
  for(y=0;y<TETRIS_HEIGHT+4;y++)
    for(x=0;x<TETRIS_WIDTH+4;x++) 
      map[y][x] = TETRIS_EMPTY;

  /* add borders */  
  for(y=0;y<TETRIS_HEIGHT+4;y++)
    map[y][0] = map[y][1] = 
      map[y][TETRIS_WIDTH+2] = 
      map[y][TETRIS_WIDTH+3] = TETRIS_RED;

  for(x=0;x<TETRIS_WIDTH+4;x++)
    map[TETRIS_HEIGHT+2][x] = 
      map[TETRIS_HEIGHT+3][x] = TETRIS_RED;

  tetris_refresh_main();

  tetris_refresh_slave(0);
  tetris_refresh_slave(1);
  tetris_refresh_slave(2);

  tetris_new_block();
}

void tetris_free(void) {
  DEBUG_PRINTF("tetris_free()\n");

}

void tetris_blocked(void) {
  DEBUG_PRINTF("is blocked\n");

  /* restart game */
  tetris_init();
}

void tetris_remove_completed(void) {
  Int16 row, column, num, row_2, mult;

  for(mult=1,row=0;row<TETRIS_HEIGHT;row++) {
    /* count number of empty entries per row */
    for(num=0,column=0;column<TETRIS_WIDTH;column++) {
      if(map[row+2][column+2] == TETRIS_EMPTY)
        num++;
    }

    if(num==0) { /* a full row */

      /* extra score */
      game.lines++;
      game.score += mult * TETRIS_SCORE_ROW;
      mult *= 2;

      for(column=0;column<TETRIS_WIDTH;column++) {
        map[row+2][column+2] = TETRIS_EMPTY;

	/* move line */
        for(row_2=row;row_2>1;row_2--)
	  map[row_2+2][column+2]=map[row_2-1+2][column+2];
      }

      graphics_shift_main(row);

      graphics_shift_slave(0, row);
      graphics_shift_slave(1, row);
      graphics_shift_slave(2, row);
    }
  }

  graphics_draw_lines(game.lines);
}

void tetris_advance(void) {
  static UInt16 cnt=0;
  static UInt32 lastKeyState=0;
  UInt32 hardKeyState = KeyCurrentState();

  /* left */
  if(((hardKeyState & keyBitHard1)&& !(lastKeyState & keyBitHard1))||
     ((hardKeyState & keyBitHard2)&& !(lastKeyState & keyBitHard2))||
     ((hardKeyState & keyBitLeft) && !(lastKeyState & keyBitLeft)))
    tetris_step(-1, 0, 0);

  /* right */
  if(((hardKeyState & keyBitHard3)&& !(lastKeyState & keyBitHard3))||
     ((hardKeyState & keyBitHard4)&& !(lastKeyState & keyBitHard4))||
     ((hardKeyState & keyBitRight)&& !(lastKeyState & keyBitRight)))
    tetris_step( 1, 0, 0);

  /* up */
  if((hardKeyState & keyBitPageUp)&& !(lastKeyState & keyBitPageUp))
    tetris_step( 0, 0,-1);

  /* tungsten center */
  if((hardKeyState & keyBitCenter)&& !(lastKeyState & keyBitCenter))
    tetris_step( 0, 0, 1);

  /* down */
  if((hardKeyState & keyBitPageDown)&& !(lastKeyState & keyBitPageDown)) {
    while(tetris_step( 0, 1, 0)) /* extra score */ ;

    tetris_remove_completed(); // remove completed rows
    graphics_draw_score(game.score);

    if(!tetris_new_block()) tetris_blocked();
  }

  /* save key state */
  lastKeyState = hardKeyState;

  if(!cnt) {
    /* one step down */
    if(!tetris_step(0,1,0)) {
      tetris_remove_completed(); // remove completed rows
      graphics_draw_score(game.score);

      if(!tetris_new_block()) tetris_blocked();
    }

    // speed depends on game level, level 10 is nearly unplayable
    cnt = 11-game.level;
  } else
    cnt--;

//  DEBUG_PRINTF("done\n");
}

