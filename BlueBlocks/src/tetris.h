/*
  tetris.h
*/

#ifndef TETRIS_H
#define TETRIS_H

#define TETRIS_SCORE_STEP   1   // score for every step a block makes
#define TETRIS_SCORE_ROW   10   // score for a removed row

#define TETRIS_BLOCK_INCR  50   // increase level after these blocks

/* info about the current block */
typedef struct {
  Int16  x, y;
  UInt16 rot, type, next;
} tetris_block_state_t;

/* info about running game */
typedef struct {
  UInt32 score;
  UInt16 lines, level, blocks;
} tetris_game_t;

/* the palmos-5 sdk does not define these?? */
#define keyBitLeft   0x01000000
#define keyBitRight  0x02000000
#define keyBitCenter 0x04000000

/* 10x20 is the layout of the original */
/* gameboy version */
#define TETRIS_WIDTH   10
#define TETRIS_HEIGHT  20

/* state of a map field */
#define TETRIS_EMPTY    0
#define TETRIS_RED      1
#define TETRIS_GREEN    2
#define TETRIS_BLUE     3
#define TETRIS_YELLOW   4
#define TETRIS_CYAN     5
#define TETRIS_MAGENTA  6
#define TETRIS_GREY     7

extern void tetris_init(void);
extern void tetris_free(void);
extern void tetris_advance(void);

#endif // TETRIS_H
