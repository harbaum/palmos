#include "level.h"

/*

*/

#define MAX_SPRITES 1  /* only players marble */
#define MARBLE 0

#define CREATOR     'Mulg' // 'Mulg'
#define DB_TYPE     'Levl' // 'Levl'
#define DB_TYPE_F   'LevF' // 'LevF' level databases for version IIf and up
#define DB_TYPE_N   'LevN' // 'LevN' level databases for version IIn and up
#define DB_TYPE_P   'LevP' // 'LevP' level databases for version IIp and up
#define DB_SCORE    'Scor' // 'Scor'

#define MAX_WIDTH  37
#define MAX_HEIGHT 33

typedef struct {
  unsigned char gray2[64];
  unsigned char gray4[128];
  unsigned char color[256];
} CUSTOM_TILE;

#ifndef IN_MAKELEVEL
extern CUSTOM_TILE *custom_tiles;
extern Boolean has_newscr;

#define ERROR(a) 

#define ABS(a)  ((a>0)?a:-a)
#define SGN(a)  ((a>0)?1:-1)

typedef struct {
  int   width;
  int   height;
  short *data;
} LEVEL;

#define DEBUG

#ifdef DEBUG
#define DEBUG_PORT 0x30000000
#define DEBUG_MSG(format, args...) { char xyz_tmp[128]; int xyz; StrPrintF(xyz_tmp, format, ## args); for(xyz=0;xyz<StrLen(xyz_tmp);xyz++) *((char*)DEBUG_PORT)=xyz_tmp[xyz]; }
#else
#define DEBUG_MSG(format, args...)
#endif

#endif
