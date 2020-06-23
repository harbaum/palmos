/*
  mulg.c - a small action game for the palm pilot

  (c) 1998/99/00 Till Harbaum
  (c) 1999       Pat Kane

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Please read readme.wn2 for the win2 lib!

  Version 1.0  - initial release
          1.1  - fixed fatal exception when entering border area 
                 (happened in level 5)
	  1.1b - fixed error with document in level 15
	  1.1c - works with game sound preferences
          II   - usage of external databases for levels + level compiler
	         new tiles/functions:
		 - groove/rampart
                 - radio buttons
		 - flip stones
		 - conways game of life
		 - bomb dispenser (+ drench bomb with box)
		 - bomb only destroys box, not stone
		 - parachute
		 - coins/slot
		 - oil
		 - support for adxl202 tilt sensor
	       - bugfixes in version II:
                 - correct display when alarm/low battery warning goes 
                   off during game
                 - prevent marble from 'tunneling' into stone
		 - wrap when leaving game area
           IIa - prevent auto off when playing with tilt sensor
           IIb - switched off tilt sensor support for OS > 3.02 
	         (to prevent Palm IIIx and Palm V crash)
		 and fixed little display bug
           IIc - fixed some problem with low batt warnings, changed the sensor
                 support to be disabled with EZ CPUs (patch from Goeff Richmond)
                 and a little fix in greyscale memory handling.
	   IId - minor bug fixes
	   IIe - changed tilt sensor interfacing to use library
	         added support for rom based level databases
                 added gif image based tiles
		 fix screen flicker with OS 3.3 or the visor
                 4 bit per pixel graphics engine (still needs graphics)
	   IIf - PEK added "light weight" blocks and "switch spaces"
	   IIf + PEK added "scarab" bot
	   IIg - finished 4bpp graphics engine
                 added color support
                 and switched to prc-tools-2.0
                 removed limitation to 16 level databases
	   IIh - memo tiles
	   IIi - scarab bug fix
                 tilt interface doesn't automatically comply about missing dongle
	   IIj - beam level databases
	   IIk - fixed bug with breaking light boxes
	   IIl - skipped, because version numbering looked ugly
	   IIm - fixed bug introduced with IIk bug fix
	   IIn - new tiles: hanoi pyramid, walker and swapper
	   IIo - fixed walker bug, new 4bpp greyscale code (works
	         on Visor Platinum, Prism and Palm IIIc
	   IIp - user defined tiles per database
	       - compressed tiles (smaller mulg binary)
*/

#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#define MemHandle VoidHand
#define MemPtr    VoidPtr
#include <SysEvtMgr.h>
#include <KeyMgr.h>
#include <ExgMgr.h>
#endif

#include "callback.h"
#include "winx.h"
#include "rsc.h"
#include "mulg.h"
#include "tiles.h"

#include "tilt.h"

#define dontPLAY_ALL_LEVELS

#define PREFS_VERSION 3     /* version of preferences structure */

#define SENSE 5             /* pen sensivity */
#define SLOW  95            /* every step reduces marble energy by 5% (due to friction) */
#define SWAMP_SLOW 50
#define DUSCH 70            /* with every hit to a wall, the marble looses 30% energie */
#define HOLED 3             /* acceleration for hole/hump */
#define WIPS  0             /* movement of the seesaw (0=toggles exactly in the middle) */
#define WIPD 0x20           /* acceleration due to seesaw */
#define VENT_ACC 20         /* acceleration due to running ventilator */
#define PUSH  1000          /* force needed to push a box */
#define LPUSH  180          /* force needed to push a light weight box */
#define HPUSH  500          /* force needed to push the pyramids of hanoi */
#define LBREAK 2000         /* force needed to break a light weight box */
#define BUMPACC 1000        /* acceleration due to bumper */
#define BUMPACCD 700        /* diagonal acceleration due to bumper (1/SQR(2)*BUMPACC) */
#define BOMB_TIMEOUT  20
#define BOMB_TIMEOUT2 5
#define PARA_FLICKER  10*25 /* flicker the last 10 sec */

#define OIL_RELOAD    (500)

/* constants for acceleration table for holes and humps (hole.h) */
/* and collision table */
#define DIR_OL 0x80
#define DIR_UL 0x40
#define DIR_UR 0x20 
#define DIR_OR 0x10

/* game states */
#define SETUP   0
#define RUNNING 1
#define MESSAGE 2

/* game end states */
#define WIN   0
#define LOOSE 1

/* movement extras */
#define GND_ICE     0x01  /* whoa, this is slippery */
#define GND_REVERSE 0x02  /* and this is stupid */
#define CARRY_MATCH 0x04  /* carrying ignited match */
#define DROP_BOMB   0x08  /* drop a bomb */
#define GND_SPACE   0x10  /* being over space (flying with parachute) */
#define GND_SWAMP   0x20

#define MAX_COLLECT 5

/* tile flags */
#define ID_MASK   0x1f  /* allowing up to 32 switch/object pairs */

#define PSTART    0xff  /* PATH */
#define REVERSE   0x80  /* PATH/BOXFIX/ICE */
#define UNREVERSE 0x40  /* PATH/BOXFIX/ICE */
#define OPENING   0x80  /* DOOR */
#define ROTATING  0x80  /* VENTILATOR */

#define ATTRIB_ORIG  -1


/* acceleration table for PEK's "light weight" block tiles */
char lbox_acc[] = { 25, 20, 10, 0, 0,  0,  0,  0};

/* acceleration table for PEK's scarab  tiles */
char scarab_acc[] = { 5, 10, 10, -10, -20,  -40,  -80,  -90};

/* acceleration tables for groove/rampart style tiles */
char groove_acc[] = { 50, 40, 30, 20, 10,  5,  2,  1};
char rampart_acc[]= {-40,-30,-21,-15,-11, -7, -4, -2};

/* direction of acceleration in groove/rampart */ 
unsigned char groove_dir[16]={
  0x99, /* (no exit) */
  0x00, /* LR (left and right exit) */
  0xff, /* TB (top and bottom exit) */
  0x87, /* RB (right and bottom) */
  0x1e, /* LB (...) */
  0xe1, /* RT */
  0x78, /* LT */
  0x9f, /* B */
  0xf9, /* T */
  0x81, /* R */
  0x18, /* L */
  0x7e, /* TLB */
  0xe7, /* TRB */
  0x60, /* LTR */
  0x06, /* LBR */
  0x66  /* LTRB */
}; 

/* load levels from internal data */
extern unsigned char hole[][16];

/* game functions */
extern void draw_tile(int tile, int x, int y);
extern void draw_level(int xp, int yp);
extern void create_sprite(int no, int tile, int hs_x, int hs_y);
extern void init_sprites(void);
extern void draw_sprite(int no, int x, int y);
extern int  disp_grey(void);
extern void init_scarabs(void);

unsigned char  level_buffer[MAX_HEIGHT][MAX_WIDTH];      /* max 37 x 33 level -> 1221 bytes */
unsigned char  attribute_buffer[MAX_HEIGHT][MAX_WIDTH];  /* max 37 x 33 level -> 1221 bytes */
unsigned short level_width, level_height;

char author[32];  /* author string */

unsigned long level_time;
char game_state=SETUP;
int collected[MAX_COLLECT];
char play_life;
unsigned int  marble_parachute;

const int x_add[]={ 0,10,19,28};
const int y_add[]={ 0, 9,17,25};
const unsigned char hole_acc[]={0x0,0x1,0x3,0x7,0xf};

/* preferences v 3 for mulg 2 */
struct {
  int  version;
  int  sound;
  int  tilt;
  char db_name[32];
} prefs;

unsigned short max_enabled, level_no, db_no;

/* state of the marble */
unsigned int marble_x,  marble_y;  /* current position on screen */
unsigned int marble_xp, marble_yp; /* current page */
int marble_sx, marble_sy;          /* speed */
int pen_last_x, pen_last_y;        /* last tap on screen */
int marble_ay, marble_ax, marble_extra;
int marble_oil;

/* database stuff */
typedef struct {
  char name[32];
  LocalID dbid;
  UInt16 card;
} level_db;

level_db *level_database = NULL;
int num_db, levels;

/* accelerometer settings */
#define ACC_ON       0x01
#define ACC_FLIP_X   0x02
#define ACC_FLIP_Y   0x04
#define ACC_FLIP_XY  0x08
#define ACC_PRESENT  0x10

/* tilt sensor library reference */
UInt16 TiltRef=0;
Err tilt_err;

short tilt_on, sound_on, db_loaded=false;
DmOpenRef dbRef, xtraRef;
Boolean xtra_open, has_newscr;

/* pointer to optional custom tiles */
char *custom_ptr=NULL;
CUSTOM_TILE *custom_tiles;
unsigned short custom_solid=0;

#ifdef OS35HDR
SndCommandType snd_switch = {sndCmdFreqDurationAmp,0,100,1,sndMaxAmp};
SndCommandType snd_kill   = {sndCmdFreqDurationAmp,0,100,1,sndMaxAmp};
#else
SndCommandType snd_switch = {sndCmdFreqDurationAmp,  100,1,sndMaxAmp};
SndCommandType snd_kill   = {sndCmdFreqDurationAmp,  100,1,sndMaxAmp};
#endif

// set custom font for multi-byte character environment by T.Shimizu
// if mode is non-zero, set the custom font if necessary. 
// otherwise, set the standard font.
static void set_custom_font(Boolean mode)
{
#ifdef OS35HDR
  // defining custom font is a new feature of Palm OS 3.5
  
  // static variable to remember whether the font was initialized
  // 0: not initialized
  // 1: initialized, custom font is not necessary (std font is single-byte)
  // 2: initialized, custom font is to be used (std font is not single-byte)
  static UInt16 s_fontState = 0; // not initialized
  
  if (s_fontState == 1) {
    // the standard font is single-byte font. nothing to do.
    return;
  }
  
  if (mode) {
    // set or initialize custom font
    if (s_fontState == 2) {
      // custom font has been initialized. set it as the current font.
      FntSetFont(AsciiFontId);
    } else {
      // initialize custom font
      FontType* pfnt = FntGetFontPtr();
      if (pfnt->fontType == 0x9000) {
        // std font is single-byte. custom font is not necessary.
        s_fontState = 1;
      } else {
        // read the custom font from the resource
        MemHandle fntHandle;
        fntHandle = DmGetResource('NFNT', AsciiFont);
        if (fntHandle) {
          // install the custom font
          FntDefineFont(AsciiFontId, (FontPtr)MemHandleLock(fntHandle));
          MemHandleUnlock(fntHandle);
          // set it as the current font
          FntSetFont(AsciiFontId);
        } else {
          // cannot obtain custom font...
          s_fontState = 1; // custom font cannot be used
        }
      }
    }
  } else {
    // unset custom font
    FntSetFont(0);
  }

#else
  // nothing to do for older Palm OS
#endif
}

/* open level database no for usage */
int open_database(int no) {
  char *abase;
  MemHandle record;
  char xtra_name[32];
  LocalID xtraID;
  UInt16 at;
  MemHandle rec_src, rec_dst;
  char *src, *dst;
  unsigned long type;

  /* clear ref */
  dbRef = (DmOpenRef)0;

  xtra_open = false;
  dbRef = DmOpenDatabase(level_database[no].card, 
			 level_database[no].dbid, 
			 dmModeReadWrite);

  /* if open failed reopen read only */
  if(dbRef == 0) {
    dbRef = DmOpenDatabase(level_database[no].card, 
			   level_database[no].dbid, 
			   dmModeReadOnly);

    /* try to open xtra score db */
    StrCopy(xtra_name, level_database[no].name);
    StrCat(xtra_name, ".score");

    if((xtraID = DmFindDatabase(level_database[no].card, xtra_name))==NULL) {
      
      /* we really need this DB */
      DmCreateDatabase(level_database[no].card, xtra_name, CREATOR, DB_SCORE, false);
 
      if((xtraID = DmFindDatabase(level_database[no].card, xtra_name))==NULL) {
	DmCloseDatabase(dbRef);
	dbRef = (DmOpenRef)0;
	FrmCustomAlert( alt_err, "Unable to create score database.",0,0); 
	return 1;
      }

      /* create score DB */
      xtraRef = DmOpenDatabase(level_database[no].card, xtraID, dmModeReadWrite);

      /* get number of levels */
      levels = DmNumRecords(dbRef)-2;

      /* create writeable backup record in new db */
      at = 0; rec_dst = DmNewRecord(xtraRef, &at, 4 + levels*4);
      dst = MemHandleLock(rec_dst);

      /* copy data from rom based database to ram based */
      rec_src = DmGetRecord(dbRef, 0);
      src = MemHandleLock(rec_src);

      /* do the copy */
      DmWrite(dst, 0, src, 4 + levels*4);

      MemPtrUnlock(src);
      DmReleaseRecord(dbRef, 0, false);

      MemPtrUnlock(dst);
      DmReleaseRecord(xtraRef, 0, true);

      xtra_open = true;
    } else {
      /* create score DB */
      xtraRef = DmOpenDatabase(level_database[no].card, xtraID, dmModeReadWrite);

      xtra_open = true;
    }
  }

  /* if open for read write fails open read only and seperate high score database */
  if(dbRef!=0) {

    /* get type */
    DmDatabaseInfo(level_database[no].card, level_database[no].dbid, 
		   0, 0, 0, 0, 0, 0, 0, 0, 0, &type, 0);

    /* this new type contains an additional record */
    if((type >= 'LevP')&&(type != 'Levl')) {

      /* get address of custom tile record */
      record = DmGetRecord(dbRef, 2);
      custom_ptr = MemHandleLock(record);
      custom_solid = *((unsigned short *)custom_ptr);
      custom_tiles = (CUSTOM_TILE*)(custom_ptr+2);

      /* database contains 1 rec hiscores, 1 rec docs, 1 rec tiles and levels */
      levels = DmNumRecords(dbRef)-3;

      db_loaded=true;
    } else {
      /* database contains 1 rec hiscores, 1 rec docs and levels */
      levels = DmNumRecords(dbRef)-2;
      db_loaded=true;
    }

    /* get address of author name */
    record = DmGetRecord(dbRef, 1);
    abase=MemHandleLock(record);

    /* doc 0 + 2 bytes version */
    StrCopy(author, abase + *((unsigned short*)abase) + 2);

    /* release everything */
    MemPtrUnlock(abase);
    DmReleaseRecord(dbRef, 1, false);    

  } else {
    FrmCustomAlert( alt_err, "Unable to open level database.",0,0); 
    return 1;
  }

  return 0;
}

void close_database(void) {

  /* free custom tiles */
  if(custom_ptr) {
    MemPtrUnlock(custom_ptr);
    DmReleaseRecord(dbRef, 2, false);    

    custom_ptr = NULL;
  }

  DmCloseDatabase(dbRef);
  if(xtra_open) 
    DmCloseDatabase(xtraRef);
}

/* scan for level databases and save their id's */
Boolean scan_databases(void) {
  DmSearchStateType stateInfo;
  Err err;
  int i;
  LocalID dbid;
  UInt16 card;
  Boolean first;

  /* scan for 'Levl', 'LevF', 'LevF' and 'LevP' type databases */
  unsigned long db_types[]={DB_TYPE, DB_TYPE_F, DB_TYPE_N, DB_TYPE_P, 0};

  /* free all level databases buffers */
  if(level_database != NULL) MemPtrFree(level_database);

  /* scan for number of databases */
  for(num_db=0, i=0;db_types[i]!=0;i++) {
    first = true;
    do {
      err = DmGetNextDatabaseByTypeCreator(first, 
			     &stateInfo, db_types[i], 
			     CREATOR, false, &card, &dbid);

      if(!err) num_db++;
      first = false;
    } while(!err);
  }

  if(num_db==0) {
    FrmCustomAlert(alt_err, "No Mulg level databases installed.",0,0);
    return false;
  }

  /* allocate buffer for all databases */
  level_database = MemPtrNew(sizeof(level_db) * (1+num_db));
  
  if(level_database == NULL) {
    FrmCustomAlert(alt_err, "Out of memory allocating database buffer.",0,0);
    return false;
  }

  /* once again, but this time save the database infos */
  for(num_db=0, i=0;db_types[i]!=0;i++) {
    first = true;
    do {
      err = DmGetNextDatabaseByTypeCreator(first, 
	    &stateInfo, db_types[i], CREATOR, false, 
	    &(level_database[num_db].card), &(level_database[num_db].dbid));

      if(!err) num_db++;
      first = false;
    } while(!err);
  }

  /* get detailed level info */
  for(i=0;i<num_db;i++)
    DmDatabaseInfo(level_database[i].card, 
		   level_database[i].dbid, 
		   level_database[i].name, 
		   0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

  return true;
}

/* game time to ascii string */
void time2str(long time, char *str) {
  str[0]='0'+(time/600000)%10;
  str[1]='0'+(time/60000)%10; 
  str[3]='0'+(time/10000)%6;
  str[4]='0'+(time/1000)%10;
  str[6]='0'+(time/100)%10;
}

/* read/save settings */
void settings(int save, unsigned short *enabled, unsigned short *no) {
  unsigned char *base;
  MemHandle record;
  unsigned long hs;
  DmOpenRef ref = xtra_open?xtraRef:dbRef;

  if(db_loaded) {
    record = DmGetRecord(ref, 0);
    base=MemHandleLock(record);

    if(save) {
      DmWrite(base, 0,      no, sizeof(unsigned short));
      DmWrite(base, 2, enabled, sizeof(unsigned short));
    } else {
      *no      = *(unsigned short*)base;
#ifdef PLAY_ALL_LEVELS
      *enabled = 0xffff;
#else
      *enabled = *(unsigned short*)(base+2);
#endif
    }

    MemPtrUnlock(base);
    DmReleaseRecord(ref, 0, save?true:false);    
  } else {
    *no      = 0; 
    *enabled = 0;
  }
}

/* get name and hiscore for level no from database */
unsigned long get_level_info(int no, char *name) {
  unsigned char *base;
  MemHandle record;
  unsigned long hs;
  DmOpenRef ref = xtra_open?xtraRef:dbRef;

  if(db_loaded) {
    /* get address of level data (level no 0 is in record 2) */
    if(name!=NULL) {
      record = DmGetRecord(dbRef, (custom_ptr?3:2)+no);
      base=MemHandleLock(record);

      /* copy name */
      MemMove(name, base, 32);
      MemPtrUnlock(base);
      DmReleaseRecord(dbRef, (custom_ptr?3:2)+no, false);    
    }
    
    /* copy hiscore */
    record = DmGetRecord(ref, 0);
    base=MemHandleLock(record);
    MemMove(&hs, &base[4+4*no], sizeof(unsigned long));
    MemPtrUnlock(base);
    DmReleaseRecord(ref, 0, false);  
  } else {
    if(name!=NULL) StrCopy(name, "no levels loaded");
    hs=0;
  }

  return hs;
}

/* write hiscore to current db */
void save_hiscore(int no, unsigned long score) {
  MemPtr *base;
  MemHandle record;
  DmOpenRef ref = xtra_open?xtraRef:dbRef;

  record = DmGetRecord(ref, 0);
  base = MemHandleLock(record);
  DmWrite(base, 4+4*no, &score, sizeof(unsigned long));
  MemPtrUnlock(base);
  DmReleaseRecord(ref, 0, true);  /* record was modified */
}

/* start a new game */
void init_level(int no) {
  int y,x;
  unsigned char *ldata,*lbase;
  MemHandle record;

  /* zero tilt sensor */
  if(tilt_on & ACC_ON) TiltLibZero(TiltRef);

  /* get address of level data (level no 0 is in record 2) */
  record = DmGetRecord(dbRef, (custom_ptr?3:2)+no);
  lbase=MemHandleLock(record);

  ldata=lbase+32;

  level_width  = *ldata++;
  level_height = *ldata++;

  /* clear level buffer */
  for(y=0;y<MAX_HEIGHT;y++)
    for(x=0;x<MAX_WIDTH;x++)
      level_buffer[y][x]=attribute_buffer[y][x]=ZERO;


  /* copy all level information */
  for(y=0;y<level_height;y++) {
    for(x=0;x<level_width;x++) {
      attribute_buffer[y][x] = *ldata++;
      level_buffer[y][x]     = *ldata++;
    }
  }

  /* release record */
  MemPtrUnlock(lbase);
  DmReleaseRecord(dbRef, (custom_ptr?3:2)+no, false);    
  DmReleaseRecord(dbRef, 2+no, false);    
  
  /* init collect buffer */
  for(x=0;x<MAX_COLLECT;x++)
    collected[x]=ZERO;

  /* start level time counter */
  level_time=0;

  /* don't play 'game of life' */
  play_life=0;

  /* don't won a parachute */
  marble_parachute=0;

  marble_oil=0;
}

/* add an object to the collection buffer */
int add_to_collection(int type) {
  int i,ret=false;

  for(i=0;(i<MAX_COLLECT)&&(!ret);i++) {
    if(collected[i]==ZERO) {
      collected[i]=type;
      ret=true;
      if(type==MATCHX)
	marble_extra|=CARRY_MATCH;
    }
  }
  return ret;
}

/* returns the x-th object from the collection buffer */
/* and removes it if it is a doc */
int get_from_collection(int x) {
  int i,ret=ZERO;

  if(((collected[x]&0xff)==DOCX)||
     ((collected[x]&0xff)==BOMBX)) {
    ret=collected[x];
    for(i=x;i<(MAX_COLLECT-1);i++)
      collected[i]=collected[i+1];

    collected[MAX_COLLECT-1]=ZERO;
  }
  return(ret);
}

/* find the object obj in the collection buffer and remove it */
int get_object_from_collection(int obj) {
  int i,j;

  for(i=0;i<MAX_COLLECT-1;i++) {

    /* found??? */
    if((collected[i]&0xff)==obj) {

      /* remove object */
      for(j=i;j<(MAX_COLLECT-1);j++)
	collected[j]=collected[j+1];

      collected[MAX_COLLECT-1]=ZERO;      
      return true;
    }
  }
  return(false);
}

void draw_collection(void) {
  int i;
  for(i=0;i<MAX_COLLECT;i++)
    draw_tile(collected[i]&0xff, i, 9);
}

void init_marble(void) {
  int x,y;
  
  for(y=0;y<level_height;y++)
    for(x=0;x<level_width;x++)
      /* start tile? */
      if((level_buffer[y][x] == PATH)&&
	 (attribute_buffer[y][x] == PSTART)) {
	marble_xp = ((x-1)/9)*9;                /* horizontal page offset */
	marble_x  = ((16*(x-marble_xp)+7)<<8);  /* horizontal position */
	marble_yp = ((y-1)/8)*8;                /* vertical page offset */
	marble_y  = ((16*(y-marble_yp)+7)<<8);  /* vertical position */
	attribute_buffer[y][x]=0;               /* clear start flag */
      }
  
  /* start acceleration */
  marble_ax=marble_sx=0;
  marble_ay=marble_sy=0;
  marble_extra = 0;
}

/* restart everything */
void init_game(void) {
  /* start with level 0 */
  set_custom_font(true); // by T.Shimizu
  init_level(level_no);
  init_marble();
  init_sprites();
  init_scarabs();  /* do this after level has been init'd */
  create_sprite(MARBLE,BALL,7,7);
}

char time_msg[]="Time: 00:00.0";
void draw_state(void) {
  int i;

  if((game_state==RUNNING)||(game_state==MESSAGE)) {
    draw_collection();
    time2str(level_time ,&time_msg[6]);
    WinXDrawChars(time_msg, StrLen(time_msg), 102, 148);
  }
}

char debug_msg[32] = "debug";
void draw_debug(void) {
  debug_msg[31] = 0;     
  if((game_state==RUNNING)||(game_state==MESSAGE)) {
    WinXDrawChars(debug_msg, StrLen(debug_msg), 20, 148);
  }
}

void redraw_screen(void) {
  draw_level(marble_xp, marble_yp);
  draw_sprite(MARBLE, marble_x>>8, marble_y>>8);
  draw_state();
  WinFlip();
}

char no_score[]="not yet played";

void fill_form(void) {
  char no_str[4], level_name[32];
  RectangleType rect;
  unsigned long hs;

  /* level no to string */
  FntSetFont(2);
  no_str[0] = '0' + (level_no+1)/10;
  no_str[1] = '0' + (level_no+1)%10;
  no_str[2] = ':';
  no_str[3] = 0;

  rect.topLeft.x=10; rect.topLeft.y=DBSEL+11; 
  rect.extent.x=127; rect.extent.y=16;

  /* output database name */
  WinEraseRectangle(&rect,0);
  WinDrawChars(level_database[db_no].name, 
	       StrLen(level_database[db_no].name), 10, DBSEL+11);

  /* output level name and no */
  rect.topLeft.y=LVSEL+10;
  WinEraseRectangle(&rect,0);
  WinDrawChars(no_str, 3, 10, LVSEL+11);
  hs = get_level_info(level_no, level_name);
  WinDrawChars(level_name, StrLen(level_name), 30, LVSEL+11);

  /* output time */
  rect.topLeft.y=TMSEL+10;
  WinEraseRectangle(&rect,0);

  if(hs!=0xffffffff) {
    time2str(hs,&time_msg[6]);
    WinDrawChars(&time_msg[6], StrLen(&time_msg[6]), 10, TMSEL+11);
  } else
    WinDrawChars(no_score, StrLen(no_score), 10, TMSEL+11);

  FntSetFont(0);
}

void game_end(int state) {
  int i;

  if(game_state==RUNNING) {

    if(state==LOOSE) {
      /* undraw marble */
      draw_level(marble_xp, marble_yp);
      draw_state();
      WinFlip();

      if(sound_on) {
	/* do sound effect */
	for(i=400;i>100;i-=6) {
	  snd_kill.param1=i;
	  SndDoCmd(NULL, &snd_kill, false); 
	}
      }
    }
    /* wait one second */
    SysTaskDelay(SysTicksPerSecond());

    game_state=SETUP;
    WinXSetMono();

    set_custom_font(false); // by T.Shimizu

    if(state==WIN) {
      /* new high score?? */
      if(level_time < get_level_info(level_no, NULL)) {
	FrmAlert(HiScore);
	save_hiscore(level_no, level_time);
      }

      /* never played the next level?? */
      if(max_enabled==level_no) {
	level_no++;
	/* enable next level */
	max_enabled=level_no;

	/* limit level no */
	if(level_no>(levels-1))
	  level_no=(levels-1);
      }
      /* refill form */
      fill_form();
    }
  }
}

void snd_clic(void) {
  if(sound_on)
    SndDoCmd(NULL,&snd_switch,false);
}

#define MSG_WIDTH  90
#define MSG_HEIGHT 100
#define MSG_BORDER 5

void draw_message(int no) {
  int lines,wr,y;
  RectangleType rc;
  char *message,*mbase;
  MemHandle record;

  /* get address of message */
  record = DmGetRecord(dbRef, 1);
  mbase = MemHandleLock(record);

  /* get address of message n */
  message = mbase + ((unsigned short*)mbase)[no];

  game_state=MESSAGE;
  snd_clic();

  /* draw white rectangle */
  WinXSetColor(clrLtGrey);
  rc.topLeft.x=(160-MSG_WIDTH)/2;  
  rc.extent.x=MSG_WIDTH; 
  rc.topLeft.y=(160-MSG_HEIGHT)/2; 
  rc.extent.y=MSG_HEIGHT;
  WinXFillRect(&rc,0);

  /* draw border */
  WinXSetColor(clrWhite);
  rc.extent.x=MSG_WIDTH; 
  rc.extent.y=1;
  WinXFillRect(&rc,3);

  rc.extent.x=1;         
  rc.extent.y=MSG_HEIGHT;
  WinXFillRect(&rc,3);

  WinXSetColor(clrBlack);
  rc.extent.x=MSG_WIDTH; 
  rc.topLeft.y=(160+MSG_HEIGHT)/2; 
  rc.extent.y=1;
  WinXFillRect(&rc,3);

  rc.topLeft.x=(160+MSG_WIDTH)/2;  
  rc.extent.x=1; 
  rc.topLeft.y=(160-MSG_HEIGHT)/2; 
  rc.extent.y=MSG_HEIGHT+1;
  WinXFillRect(&rc,3); 

  WinXSetColor(clrBlack);
  /* determine no of lines */
  lines=0; wr=0;
  while(wr<StrLen(message)) {
    wr+=FntWordWrap(&message[wr], MSG_WIDTH-2*MSG_BORDER);
    lines++;
  }
  
  /* draw lines */
  y=80-(FntLineHeight()*(lines/2)); wr=0;
  while(wr<StrLen(message)) {
    lines=FntWordWrap(&message[wr], MSG_WIDTH-2*MSG_BORDER);
    WinXDrawChars(&message[wr],lines, 
		  81-(FntLineWidth(&message[wr],lines)/2), y);
    wr+=lines;
    y+=FntLineHeight();
  }

  /* release everything */
  MemPtrUnlock(mbase);
  DmReleaseRecord(dbRef, 1, false);    

  WinFlip();
}

/* change tile at position x,y to tile and redraw */
void change_tile(unsigned short tile, int attribute, int x, int y) {
  if(attribute>=0) 
    attribute_buffer[y][x]=attribute;

  level_buffer[y][x] = tile;
}

/* change tile at x,y to tile1 if it equals tile2, to tile2 else */
void swap_tile(unsigned short tile1, unsigned short tile2, int x, int y) {
  level_buffer[y][x] = (level_buffer[y][x]==tile2)?tile1:tile2;
}

/* check if there is a running ventilator */
int check_ventilator(unsigned int x, unsigned int y) {
  unsigned char st;

  /* since this is unsigned, values <0 are >MAX, too */
  if((x>=MAX_WIDTH)||(y>=MAX_HEIGHT))
    return 0;

  st=level_buffer[y][x];

  /* this is a nice place for explosion check, too */
  if(st==EXPLODE)
    game_end(LOOSE);
  
  /* is there a ventilator? */
  if((st>=HVENT_1)&&(st<=HVENT_4)) {

    /* is it running? */
    if(attribute_buffer[y][x]&ROTATING) {
      
      /* remove all matches from collection */
      if(marble_extra&CARRY_MATCH) {
	marble_extra&=~CARRY_MATCH;
	while(get_object_from_collection(MATCHX));
      }

      return 1;
    }
  }
  return 0;
}

void switch_it(int id) {
  int   ix,iy;
  
  snd_clic();

  /* 'game of life' switch? */
  if(id==0xff) {
    play_life = !play_life;
    id = 2;
  } else
    id &= ID_MASK;

  /* search for associated object */
  for(iy=0;iy<level_height;iy++) {
    for(ix=0;ix<level_width;ix++) {
      /* object found */
      if((attribute_buffer[iy][ix]&ID_MASK)==id) {
	switch(level_buffer[iy][ix]) {

	  /* change button state */
	case BUT0:
	case BUT1:
	case ZERO:  /* special marker state */
	  change_tile((level_buffer[iy][ix]==ZERO)?BUT1:BUT0, 
		      ATTRIB_ORIG, ix, iy);
	  break;
		
	  /* switch on/off ventilator */
	case HVENT_1:
	case HVENT_2:
	case HVENT_3:
	case HVENT_4:
	  attribute_buffer[iy][ix] = attribute_buffer[iy][ix]^ROTATING;
	  break;
	  
	  /* open/close door */
	case HDOOR_1:
	case VDOOR_1:
	case HDOOR_2:
	case VDOOR_2:
	case HDOOR_3:
	case VDOOR_3:
	case HDOOR_4:
	case VDOOR_4:
	  attribute_buffer[iy][ix] = attribute_buffer[iy][ix]^OPENING;
	  break;

	  /* close hole */
	  /* open inversed hole */
	case PATH:
	case SPACE:
	  change_tile((level_buffer[iy][ix]==PATH)?SPACE:PATH, ATTRIB_ORIG, ix, iy);
	  break;
	}
      }
    }
  }
}

int check_tile(unsigned int x, unsigned int y, int dir) {
  short id;
  unsigned char st;
  int lx,ly, t,l,i;

  /* wrap coordinate */
  x = (x + level_width) % level_width;
  y = (y + level_height)% level_height;

  /* custom tile? */
  if((st=level_buffer[y][x])>=255-16) {
    return(custom_solid & (1<<(255-st)));
  }

  switch(st) {
    /* stone -> no way */
  case STONE: 

    /* slot in use */
  case SLOT1:
  case SLOT2:
  case SLOT3:
  case SLOT4:
  case SLOT5:
  case SLOT6:
  case SLOT7:
  case SLOT8:

    /* memorize tiles */
  case MEM_0O:
  case MEM_0C:
  case MEM_1C:
  case MEM_1O:
  case MEM_2C:
  case MEM_2O:
  case MEM_3O:
  case MEM_3C:

  case FRATZE:
    return 1;

    /* okay to move onto a bug */
  case SCARAB0:
  case SCARAB90:
  case SCARAB180:
  case SCARAB270:
    return 0;     

    /* horizontal door (not open) */
  case HDOOR_1:
  case HDOOR_2:
  case HDOOR_3:

    /* vertical door */
  case VDOOR_1:
  case VDOOR_2:
  case VDOOR_3:
    if(dir==0) game_end(LOOSE);

    return 1;

  case HVENT_1:
  case HVENT_2:
  case HVENT_3:
  case HVENT_4:
    /* killed by running fan */
    if(attribute_buffer[y][x]&ROTATING) game_end(LOOSE);
    return 0;

    /* one ways */
  case OWR:  /* one-way right */
    return(!((dir==1)||(dir==0)));    

  case OWL:  /* one-way left */
    return(!((dir==5)||(dir==0)));    

  case OWU:  /* one-way up */
    return(!((dir==3)||(dir==0)));    

  case OWD:  /* one-way down */
    return(!((dir==7)||(dir==0)));    

  case MEM_Q:
    /* open this box, question mark becomes hidden tile */
    level_buffer[y][x] = MEM_0C + ((attribute_buffer[y][x]&0xc0)>>5);

    /* close all not matching tiles and see if all equal tiles */
    /* are open */
    // i,l
    for(i=1,ly=0;ly<level_height;ly++) {
      for(lx=0;lx<level_width;lx++) {
	t = level_buffer[ly][lx];

	/* memorizer tile */
	if((t>=MEM_Q)&&(t<=MEM_3O)) {
	  /* closed tile */
	  if(t == MEM_Q) {
	    if((attribute_buffer[y][x]&0xc0) == 
	       (attribute_buffer[ly][lx]&0xc0))
	      i=0;  /* not all matching tiles open */
	  } else {
	    if(((attribute_buffer[ y][ x]&0xc0) != 
	        (attribute_buffer[ly][lx]&0xc0)) && (t&1))
	      level_buffer[ly][lx] = MEM_Q;
	  }
	}
      }
    }

    /* all matching memorizers are now open */
    if(i) {
      switch_it(attribute_buffer[y][x] & 0x3f);

      /* lock open all matching tiles */
      for(ly=0;ly<level_height;ly++) {
	for(lx=0;lx<level_width;lx++) {
	  t = level_buffer[ly][lx];
	  if((t > MEM_Q)&&(t<=MEM_3O)) {
	    if((attribute_buffer[ y][ x]&0xc0) == 
	       (attribute_buffer[ly][lx]&0xc0))	      
	      level_buffer[ly][lx] = t+1;
	  }
	}
      }
    }
    return 1;

  case EXCHANGE:
    return 1;
    break;
    
  case MOVE_R:
  case MOVE_L:
  case MOVE_U:
  case MOVE_D:
    if(marble_parachute) i = st - 1;
    else                 i = st + 1;

    if(i<MOVE_R) i = MOVE_U;
    if(i>MOVE_U) i = MOVE_R;

    change_tile(i, -1, x, y);

    snd_clic();
    return 1;
    break;

  case FLIP0: /* color flip tiles */
  case FLIP1:
    /* look if all flips already have same color */
    for(i=1, l=1, ly=0;ly<level_height;ly++) {
      for(lx=0;lx<level_width;lx++) {
	t = level_buffer[ly][lx];
	if((t==FLIP0)||(t==FLIP1)) {
	  /* if there is a least one flip that differed from the current flip */
	  /* then not all flips were equal */
	  if(t!=st) i=0;

	  /* ok, flip adjacent flips */
	  if(((attribute_buffer[ly][lx]&0xf0) == (attribute_buffer[y][x]&0xf0))||
	     ((attribute_buffer[ly][lx]&0x0f) == (attribute_buffer[y][x]&0x0f))) {

	    t = level_buffer[ly][lx] = (t==FLIP0)?FLIP1:FLIP0;
	  }

	  /* if there is still a flip with the same color the active flip */
	  /* had originally, then not all flips are equal now */
	  if(t==st) l=0;
	}
      }
    }

    /* if all flips _were_ equal and aren't anymore or now _are_ equal and weren't -> action */
    if(l!=i) switch_it(1);  /* flips work always switch id 1 */
    return 1;
    
  case SLOT:
    /* any 5 money there? */
    if(get_object_from_collection(DM5X)) {
      snd_clic();

      switch_it(attribute_buffer[y][x]);
      change_tile(SLOT1, (attribute_buffer[y][x]&0x1f)|0xa0, x, y);
    }

    /* any 1 money there? */
    else if(get_object_from_collection(DM1X)) {
      snd_clic();

      switch_it(attribute_buffer[y][x]);
      change_tile(SLOT1, (attribute_buffer[y][x]&0x1f)|0xe0, x, y);
    }
    return 1;

  case KEYHOLE:
    /* change keyhole to switch */
    if(get_object_from_collection(KEYX)) {
      snd_clic();
      change_tile(SWITCH_ON, ATTRIB_ORIG, x,y);
    }
    return 1;  
    
  case SWITCH_ON:
  case SWITCH_OFF:
    switch_it(attribute_buffer[y][x]);

    /* change state of switch */
    swap_tile(SWITCH_ON, SWITCH_OFF, x, y);
    return 1;  
    
  case DICE1:
  case DICE2:
  case DICE3:
  case DICE4:
  case DICE5:
  case DICE6:
    /* look if all dices are equal */
    l=1;
    for(ly=0;ly<level_height;ly++) {
      for(lx=0;lx<level_width;lx++) {
	t = level_buffer[ly][lx];
	/* is this a dice? */
	if((t>=DICE1)&&(t<=DICE6)) {
	  /* same object identifier */
	  if(attribute_buffer[ly][lx] == attribute_buffer[y][x]) {
	    /* same value?? */
	    if(t!=level_buffer[y][x]) l=0;  /* nope! */
	  }
	}
      }
    }
    
    /* randomize the dice */
    change_tile(DICE1 + SysRandom(0)%6, ATTRIB_ORIG, x, y);
    
    /* look if all dices are still/now equal */
    i=1;
    for(ly=0;ly<level_height;ly++) {
      for(lx=0;lx<level_width;lx++) {
	t = level_buffer[ly][lx];
	/* is this a dice? */
	if((t>=DICE1)&&(t<=DICE6)) {
	  /* same object identifier */
	  if(attribute_buffer[ly][lx] == attribute_buffer[y][x]) {
	    /* same value?? */
	    if(t!=level_buffer[y][x]) i=0;  /* nope! */
	  }
	}
      }
    }
    /* yupp, all dices became equal/unequal */
    if(i!=l) switch_it(attribute_buffer[y][x]);
    return 1;
    
  case SKULL:
    game_end(LOOSE);
    return 1;

  case BUMP:
  case BUMPL:
    snd_clic();

    if(dir==1)   marble_ax=-BUMPACC; 
    if(dir==2) { marble_ax=-BUMPACCD; marble_ay= BUMPACCD; };
    if(dir==3)                        marble_ay= BUMPACC; 
    if(dir==4) { marble_ax= BUMPACCD; marble_ay= BUMPACCD; };
    if(dir==5)   marble_ax= BUMPACC; 
    if(dir==6) { marble_ax= BUMPACCD; marble_ay=-BUMPACCD; };
    if(dir==7)                        marble_ay=-BUMPACC; 
    if(dir==8) { marble_ax=-BUMPACCD; marble_ay=-BUMPACCD; };

    /* activate bumper */
    if((dir>=1)&&(dir<=8))
     change_tile(BUMPL, 0, x, y);
    return 1;

    /* bomb dispenser */
  case BOMBD:
    if(add_to_collection(BOMBX)) {
      snd_clic();
      draw_collection();
    }
    return 1;

    /* broken box */
  case LBROKEN_1:
  case LBROKEN_2:
  case LBROKEN_3:
  case LBROKEN_4:
    return 1;
    break;

    /* magnets */
  case MAGNET_P:
  case MAGNET_N:
    return 1;
    break;

    /* pushable boxes */
  case HANOI_B:
  case HANOI_M:
  case HANOI_T:
  case HANOI_BM:
  case HANOI_BT:
  case HANOI_MT:
  case HANOI_BMT:
  case BOX:
  case LBOX:           /* PEK's light weight box */
    {
      int push = PUSH;
      snd_clic();

      /* the towers of hanoi */
      if( (st>=HANOI_B)&&(st<=HANOI_BMT) )
	push = HPUSH;

      /* PEKs light boxes */
      if (st == LBOX){
	push = LPUSH;    /* less mass */
	if ((ABS(marble_sx)>LBREAK)||(ABS(marble_sy) > LBREAK))
	{
	  /* light box breaks */
	  change_tile(LBROKEN_1, attribute_buffer[y][x], x, y);
	  return(1);
	}
      }

      /* push box to the next field if possible (speed>push) */ 
      lx=x; ly=y;
      if((dir==1)&&(marble_sx> push)) lx++;    /* from the left */
      if((dir==3)&&(marble_sy<-push)) ly--;    /* from the bottom */
      if((dir==5)&&(marble_sx<-push)) lx--;    /* from the right */
      if((dir==7)&&(marble_sy> push)) ly++;    /* from the top */
      
      if((x!=lx)||(y!=ly)) {
	/* is this a tile, a box can be pushed onto? */
	t=level_buffer[ly][lx];

	/* box and lbox can be pushed onto several things */
	if( (((st == BOX)||(st == LBOX)) && 
	    ((t==BOMBI)||(t==BOMB)||(t==KEY)||(t==PATH)||(t==HUMP)
	   ||(t==HOLE)||(t==SPACE)||(t==BOXFIX)||(t==ICE)||((t>=OWL)&&(t<=OWD))
	   ||(t==SWAMP)||(t==DM1)||(t==DM5)||(t==LBOXFIX)||(t==SWSPACE))) || 

	    /* hanoi can only be pushed onto path and other hanoi's: */
	    /*      P   B   M   T  BM  BT  BMT */
	    /* B    X                          */
	    /* M    X   X                      */
	    /* T    X   X   X      X           */
	    /* BM   X   X                      */
	    /* BT   X   X   X      X           */
	    /* MT   X   X   X      X           */
	    /* BMT  X   X   X      X           */
	    (t == PATH) || 
	    ((st == HANOI_M)||(st == HANOI_BM) && (t == HANOI_B)) ||
	    ((st == HANOI_T)||(st == HANOI_BT)||
	         (st == HANOI_MT)||(st == HANOI_BMT)) && 
	    ((t == HANOI_B)||(t == HANOI_M)||(t == HANOI_BM))) {

	  /* allow to drench BOMB by pushing a BOX onto it */
	  if(t==BOMBI) level_buffer[y][x]=BOMB;

	  if(t==SPACE) {
	    if (st == LBOX)
	      level_buffer[ly][lx]=LBOXFIX;  /* fill space with light box */
	    else
	      level_buffer[ly][lx]=BOXFIX;  /* fill space with box */
	  }

	  /* check of PEK's "switch spaces" are all filled */
	  else if (t==SWSPACE) {
	    int id;
	    int ix, iy;
	    int found;
	    id = attribute_buffer[ly][lx] & ID_MASK;
	    if (st == LBOX)
	      level_buffer[ly][lx]=LBOXFIX;  /* fill space with box */
	    else
	      level_buffer[ly][lx]=BOXFIX;  /* fill space with box */

	    /* search for other switch spaces like this one */
	    found = 0;
	    for(iy=0;iy<level_height;iy++) {
	      for(ix=0;ix<level_width;ix++) {
		if( level_buffer[iy][ix] == SWSPACE
		    && (attribute_buffer[iy][ix]&ID_MASK)==id)  /* needed? */
		  found++; 
	      }
	    }
	    if ( !found )   /* all spaces filled, flip switch */
	      switch_it(id);
	  } else {
	    /* save whats under the box in attribute */
	    /* (therefore no DOC possible here :-( ) */
	    
	    if((st == LBOX)||(st == BOX)) {
	      /* standard boxes */
	      
	      attribute_buffer[ly][lx] = level_buffer[ly][lx];
	      if (st == LBOX)
		level_buffer[ly][lx] = LBOX;
	      else
		level_buffer[ly][lx] = BOX;
	  
	    } else {
	      /* hanoi stuff ... */
	      
	      switch(t) {
	      case PATH:
		/* move attribute moved with bottom tile */
		if(st==HANOI_B)
		  attribute_buffer[ly][lx] = attribute_buffer[y][x];
		else
		  attribute_buffer[ly][lx] = 0;

		switch(st) {
		case HANOI_B:
		case HANOI_M:
		case HANOI_T:
		  level_buffer[ly][lx] = st;
		  break;
		case HANOI_BM:
		  level_buffer[ly][lx] = HANOI_M;
		  break;
		case HANOI_BT:
		case HANOI_MT:
		case HANOI_BMT:
		  level_buffer[ly][lx] = HANOI_T;
		  break;
		}
		break;

	      case HANOI_B:
		/* don't touch attribute */		
		if((st == HANOI_M)||(st == HANOI_BM))
		  level_buffer[ly][lx] = HANOI_BM;
		else
		  level_buffer[ly][lx] = HANOI_BT;
		break;

	      case HANOI_M:
		attribute_buffer[ly][lx] = 0;
		level_buffer[ly][lx] = HANOI_MT;
		break;

	      case HANOI_BM:
		/* don't touch attribute */
		level_buffer[ly][lx] = HANOI_BMT;

		/* magic on ... */
		switch_it(attribute_buffer[ly][lx]);

		break;
	      }
	    }
	  }
	  
	  /* remove old box */
	  if((st==BOX)||(st==LBOX)) {
	    level_buffer[y][x] = attribute_buffer[y][x];
	    attribute_buffer[y][x] = 0;
	  } else {
	    switch(st) {
	    case HANOI_B:
	    case HANOI_M:
	    case HANOI_T:
	      attribute_buffer[y][x] = 0;
	      level_buffer[y][x] = PATH;
	      break;

	    case HANOI_BM:
	    case HANOI_BT:
	      /* attribute stays */
	      level_buffer[y][x] = HANOI_B;
	      break;

	    case HANOI_MT:
	      attribute_buffer[y][x] = 0;
	      level_buffer[y][x] = HANOI_M;
	      break;
	      
	    case HANOI_BMT:
	      /* attribute stays */
	      level_buffer[y][x] = HANOI_BM;

	      /* magic off ... */
	      switch_it(attribute_buffer[y][x]);
	      break;
	    }
	  }
	}
    }
    return 1;
    }

  case WIPPR:
  case WIPPL:
  case WIPPU:
  case WIPPO:
    /* seesaw -> determine direction of entering */
    if(dir==0) return 0;
    if((st==WIPPL)&&(dir==1))  return 0;  /* from the left onto WIPPR is ok */
    if((st==WIPPU)&&(dir==3))  return 0;  /* from the bottom onto WIPPO is ok */
    if((st==WIPPR)&&(dir==5))  return 0;  /* from the right onto WIPPL is ok */
    if((st==WIPPO)&&(dir==7))  return 0;  /* from the top onto WIPPU is ok */
    
    return 1;
  }
  
  return 0;
}

int check_marble(void) {
/* coordinates of current tile */
  int ix, iy, x, y, t, s, p, entering_new;
  short st;
  static int last_x=0, last_y=0;
  char *acc;

  marble_extra&=~GND_SPACE;

  /* reduce oil */
  if(marble_oil>0)
    marble_oil--;

  /* leaving the game area? need to switch to new game area? */
  /* note: marble_xp/marble_yp is unsigned */
  if((marble_x>>8)==152) {      /* at right */
    marble_xp+=9;
    if(marble_xp>=(level_width-1)) marble_xp-=(level_width-1);

    marble_x-=(144<<8);         /* beam marble to the very left */
  }

  if((marble_x>>8)==6) {        /* at left */
    marble_xp-=9;
    if(marble_xp>=(level_width-1)) marble_xp+=(level_width-1);

    marble_x+=(144<<8);         /* beam marble to the very right */
  }

  if((marble_y>>8)==136) {      /* at bottom */
    marble_yp+=8;
    if(marble_yp>=(level_height-1)) marble_yp-=(level_height-1);

    marble_y-=(128<<8);         /* beam marble to the very top */
  }

  if((marble_y>>8)==6) {        /* at top */
    marble_yp-=8;
    if(marble_yp>=(level_height-1)) marble_yp+=(level_height-1);

    marble_y+=(128<<8);         /* beam marble to the very bottom */
  }

  ix = (marble_x>>8)&15; x = (marble_x>>8)/16 + marble_xp;
  iy = (marble_y>>8)&15; y = (marble_y>>8)/16 + marble_yp;

  entering_new = ((x!=last_x)||(y!=last_y));
  st=level_buffer[y][x];

  /* walking on path and drop bomb? */
  if(marble_extra & DROP_BOMB) {
    if(st==PATH)
      change_tile(BOMB, 0, x, y);
    else {
      /* no path -> don't drop bomb */
      add_to_collection(BOMBX);
      draw_collection();
    }
    marble_extra &=~DROP_BOMB;
  }

  switch(st) {

  case OIL:
    marble_oil=OIL_RELOAD;
    break;

  case DOC:
  case KEY:
  case MATCH:
  case DM1:
  case DM5:
    /* collectables (document, key, match) */
    if(add_to_collection(( (int)(attribute_buffer[y][x])<<8)|(st+1))) {
      snd_clic();
      
      draw_collection();
      
      /* remove tile */
      change_tile(PATH, 0, x, y);
    }
    break;

  case PARA:
    /* found a parachute */
    snd_clic();
    marble_parachute=25 * (unsigned int)attribute_buffer[y][x];
    change_tile(PATH, 0, x, y);
    create_sprite(MARBLE,BALLSH,7,7);
    break;

  case BOMB:
    /* igniting unignited bomb */
    if((attribute_buffer[y][x]==0)&&(marble_extra&CARRY_MATCH))
      attribute_buffer[y][x]=BOMB_TIMEOUT;
    break;

  case SWAMP:
    /* .. on swamp */
    marble_extra|=GND_SWAMP;
    break;

  case BUT0:
    /* entering button */
    level_buffer[y][x]=ZERO;  /* mark this tile */
    
    /* work the switch (and alter all accociated buttons) */
    switch_it(attribute_buffer[y][x]);
    break;

  case SPACE:
  case SWSPACE:
  case HDOOR_1:
  case HDOOR_2:
  case HDOOR_3:
  case VDOOR_1:
  case VDOOR_2:
  case VDOOR_3:
    /* lost in space/standing in closing door -> end of game */
    if((st==SPACE || st==SWSPACE)&&(marble_parachute)) {
      marble_extra|=GND_SPACE;
    } else
      game_end(LOOSE);
    break;

  case GOAL:
    /* reached the center of the goal */
    if((ix>=5)&&(ix<=9)&&(iy>=5)&&(iy<=9))
      game_end(WIN);
    break;

  case WIPPR:
  case WIPPL:
  case WIPPU:
  case WIPPO:
    /* the seesaw -> may toggle */
    t=0;
    if(st==WIPPR) {  /* right seesaw */
      marble_sx += WIPD;
      if(ix<(7-WIPS)) t=WIPPL;
    }
    if(st==WIPPL) {  /* left seesaw */
      marble_sx -= WIPD;
      if(ix>(7+WIPS)) t=WIPPR;
    }
    if(st==WIPPU) {  /* top seesaw */
      marble_sy += WIPD;
      if(iy<(7-WIPS)) t=WIPPO;
    }
    if(st==WIPPO) {  /* bottom seesaw */
      marble_sy -= WIPD;
      if(iy>(7+WIPS)) t=WIPPU;
    }

    /* seesaw toggled? redraw! */
    if(t) change_tile(t, ATTRIB_ORIG, x, y);

    break;

  case HOLE:
  case HUMP:
    /* is the marble on a hole or hump?? */
    t=((st==HUMP)?-1:1);
    marble_sx += t*((ix>7)?-1:1)*((hole_acc[hole[ix][iy]&0x07])<<HOLED);
    marble_sy += t*((iy>7)?-1:1)*((hole_acc[hole[iy][ix]&0x07])<<HOLED);
    break;

    /* i ever wanted to use goto in a c-prog :-) */
  case ICE:
    /* walking on ice */
    marble_extra|=GND_ICE;
    goto case_REVERSE;

  case VAN0:
  case VAN1:
  case VAN2:
  case PATH:
    /* entering vanishing tile? */
    if((((attribute_buffer[y][x]&0x20)&&(st==PATH))||(st==VAN0)||(st==VAN1)||(st==VAN2))&&(entering_new)) {
      if(level_buffer[y][x]++==VAN2) level_buffer[y][x]=SPACE;
      if(level_buffer[y][x]==PATH+1) level_buffer[y][x]=VAN0;
      snd_clic();
    }

  /* how do you like my label? */
  case BOXFIX:
  case LBOXFIX:    /* cool label! */
  case_REVERSE:
    /* reversed PATH/BOXFIX/ICE */
    if(attribute_buffer[y][x]&REVERSE)
      marble_extra|=GND_REVERSE;
    
    if(attribute_buffer[y][x]&UNREVERSE)
      marble_extra&=~GND_REVERSE;

    /* is the marble on one of PEK's "light weight" boxes ? */
    if ( st == LBOXFIX )
    {
      /* add acceleration */
      t = 0x99;
      acc = lbox_acc;

      if(iy<=7) {
	if(ix<=7) {
	  if((ix<=iy)&&(t&0x80)) marble_sx += acc[ix];
	  if((ix>=iy)&&(t&0x40)) marble_sx += acc[ix];
	  if((ix<=iy)&&(!(t&0x80))) marble_sy += acc[iy];
	  if((ix>=iy)&&(!(t&0x40))) marble_sy += acc[iy];
	} else {
	  if(((15-ix)<=iy)&&(t&0x10)) marble_sx -= acc[15-ix];
	  if(((15-ix)>=iy)&&(t&0x20)) marble_sx -= acc[15-ix];
	  if(((15-ix)<=iy)&&(!(t&0x10))) marble_sy += acc[iy];
	  if(((15-ix)>=iy)&&(!(t&0x20))) marble_sy += acc[iy];
	}
      } else {
	if(ix<=7) {
	  if((ix<=(15-iy))&&(t&0x01)) marble_sx += acc[ix];
	  if((ix>=(15-iy))&&(t&0x02)) marble_sx += acc[ix];
	  if((ix<=(15-iy))&&(!(t&0x01))) marble_sy -= acc[15-iy];
	  if((ix>=(15-iy))&&(!(t&0x02))) marble_sy -= acc[15-iy];
	} else {
	  if(((15-ix)<=(15-iy))&&(t&0x08)) marble_sx -= acc[15-ix];
	  if(((15-ix)>=(15-iy))&&(t&0x04)) marble_sx -= acc[15-ix];
	  if(((15-ix)<=(15-iy))&&(!(t&0x08))) marble_sy -= acc[15-iy];
	  if(((15-ix)>=(15-iy))&&(!(t&0x04))) marble_sy -= acc[15-iy];
	}
      }
    }
    break;

  case SCARAB0:
  case SCARAB90:
  case SCARAB180:
  case SCARAB270:
    /* is the marble on one of PEK's scarabs ? */
    /* add acceleration */
    t = 0x99;
    acc = scarab_acc;

    if(iy<=7) {
      if(ix<=7) {
	if((ix<=iy)&&(t&0x80)) marble_sx += acc[ix];
	if((ix>=iy)&&(t&0x40)) marble_sx += acc[ix];
	if((ix<=iy)&&(!(t&0x80))) marble_sy += acc[iy];
	if((ix>=iy)&&(!(t&0x40))) marble_sy += acc[iy];
      } else {
	if(((15-ix)<=iy)&&(t&0x10)) marble_sx -= acc[15-ix];
	if(((15-ix)>=iy)&&(t&0x20)) marble_sx -= acc[15-ix];
	if(((15-ix)<=iy)&&(!(t&0x10))) marble_sy += acc[iy];
	if(((15-ix)>=iy)&&(!(t&0x20))) marble_sy += acc[iy];
      }
    } else {
      if(ix<=7) {
	if((ix<=(15-iy))&&(t&0x01)) marble_sx += acc[ix];
	if((ix>=(15-iy))&&(t&0x02)) marble_sx += acc[ix];
	if((ix<=(15-iy))&&(!(t&0x01))) marble_sy -= acc[15-iy];
	if((ix>=(15-iy))&&(!(t&0x02))) marble_sy -= acc[15-iy];
      } else {
	if(((15-ix)<=(15-iy))&&(t&0x08)) marble_sx -= acc[15-ix];
	if(((15-ix)>=(15-iy))&&(t&0x04)) marble_sx -= acc[15-ix];
	if(((15-ix)<=(15-iy))&&(!(t&0x08))) marble_sy -= acc[15-iy];
	if(((15-ix)>=(15-iy))&&(!(t&0x04))) marble_sy -= acc[15-iy];
      }
    }
    break;


  default:
    /* one of the groove/rampart tiles */
    if((st>=GR)&&(st<=WALTRB)) {

      if(st<=GRLTRB) {
	/* rolling in a groove */
	t = groove_dir[st-GR];
	acc = groove_acc;
      } else {
	/* rolling on some rampart */
	t = groove_dir[st-WA];
	acc = rampart_acc;
      }
      
      /* add appropriate acceleration */
      if(iy<=7) {
	if(ix<=7) {
	  if((ix<=iy)&&(t&0x80)) marble_sx += acc[ix];
	  if((ix>=iy)&&(t&0x40)) marble_sx += acc[ix];
	  if((ix<=iy)&&(!(t&0x80))) marble_sy += acc[iy];
	  if((ix>=iy)&&(!(t&0x40))) marble_sy += acc[iy];
	} else {
	  if(((15-ix)<=iy)&&(t&0x10)) marble_sx -= acc[15-ix];
	  if(((15-ix)>=iy)&&(t&0x20)) marble_sx -= acc[15-ix];
	  if(((15-ix)<=iy)&&(!(t&0x10))) marble_sy += acc[iy];
	  if(((15-ix)>=iy)&&(!(t&0x20))) marble_sy += acc[iy];
	}
      } else {
	if(ix<=7) {
	  if((ix<=(15-iy))&&(t&0x01)) marble_sx += acc[ix];
	  if((ix>=(15-iy))&&(t&0x02)) marble_sx += acc[ix];
	  if((ix<=(15-iy))&&(!(t&0x01))) marble_sy -= acc[15-iy];
	  if((ix>=(15-iy))&&(!(t&0x02))) marble_sy -= acc[15-iy];
	} else {
	  if(((15-ix)<=(15-iy))&&(t&0x08)) marble_sx -= acc[15-ix];
	  if(((15-ix)>=(15-iy))&&(t&0x04)) marble_sx -= acc[15-ix];
	  if(((15-ix)<=(15-iy))&&(!(t&0x08))) marble_sy -= acc[15-iy];
	  if(((15-ix)>=(15-iy))&&(!(t&0x04))) marble_sy -= acc[15-iy];
	}
      }
    }
  }  

  /* see if there is a ventilator (or explosion) near */
  if(check_ventilator(x+1, y  )) marble_sx+=VENT_ACC;
  if(check_ventilator(x  , y+1)) marble_sy+=VENT_ACC;
  if(check_ventilator(x-1, y  )) marble_sx-=VENT_ACC;
  if(check_ventilator(x  , y-1)) marble_sy-=VENT_ACC;

  /* update values of last coordinate visited */
  last_x=x; last_y=y;

  /* direction value:                                */
  /*    8 7 6   these values are set when _entering_ */
  /*     \|/    a tile, not when already standing on */
  /*    1-X-5   that tile                            */
  /*     /|\                                         */
  /*    2 3 4                                        */

  /* see, if there is something blocking our way ... */

  /* ... horizontally ... */
  if(((ix>9)&&(check_tile(x+1,y,((ix==10)&&(marble_sx>0))?1:0)))||
     ((ix<5)&&(check_tile(x-1,y,((ix== 4)&&(marble_sx<0))?5:0)))) {
    if(!(marble_extra&GND_SPACE))
      marble_sx = (DUSCH*(long)marble_sx)/100;
    return 1;
  }

  /* ... vertically ... */
  if(((iy>9)&&(check_tile(x,y+1,((iy==10)&&(marble_sy>0))?7:0)))||
     ((iy<5)&&(check_tile(x,y-1,((iy== 4)&&(marble_sy<0))?3:0)))) {
    if(!(marble_extra&GND_SPACE))
      marble_sy = (DUSCH*(long)marble_sy)/100;
    return 1;
  }

  /* ... and diagonally ... */
  if((t=hole[iy][ix])&0xf8) {
    if(((t&DIR_OL)&&(check_tile(x-1,y-1,(t&8)?4:0)))||
       ((t&DIR_UL)&&(check_tile(x-1,y+1,(t&8)?6:0)))||
       ((t&DIR_OR)&&(check_tile(x+1,y-1,(t&8)?2:0)))||
       ((t&DIR_UR)&&(check_tile(x+1,y+1,(t&8)?8:0)))) {
      if(!(marble_extra&GND_SPACE)) {
	marble_sx = (DUSCH*(long)marble_sx)/100;
	marble_sy = (DUSCH*(long)marble_sy)/100;
      }
      return 1;
    }
  }

  return 0;
}

void move_marble(UInt16 *x, UInt16 *y) {
  int r,step_x, step_y, ist_x, ist_y;
  int marble_ox, marble_oy;

  ist_x=0; ist_y=0;

  /* marble_ax = */ marble_ox = marble_sx; 
  /* marble_ay = */ marble_oy = marble_sy;

  if((ABS(marble_sx>=256)||(ABS(marble_sy)>=256))) {
    if(ABS(marble_sx)>=ABS(marble_sy)) {

      /* step in x direction */
      step_y=256*(long)marble_sy/(long)ABS(marble_sx);
      step_x=(marble_sx>0)?256:-256;
      for(r=ABS(marble_sx/256);r>0;r--) {
	ist_y+=ABS(step_y); 
	marble_y+=step_y; 
	if(check_marble()) {
	  step_y=-step_y;
	  marble_sy=-marble_sy;
	  marble_y+=2*step_y;
	}
	ist_x+=ABS(step_x); 
	marble_x+=step_x;
	if(check_marble()) {
	  step_x=-step_x;
	  marble_sx=-marble_sx;
	  marble_x+=2*step_x;
	}
      }
    } else {

      /* step in Y direction */
      step_x=256*(long)marble_sx/(long)ABS(marble_sy);
      step_y=(marble_sy>0)?256:-256;
      for(r=ABS(marble_sy/256);r>0;r--) {
	marble_y+=step_y;
	ist_y+=ABS(step_y);
	if(check_marble()) {
	  step_y=-step_y;
	  marble_sy=-marble_sy;
	  marble_y+=2*step_y;
	}
	marble_x+=step_x;
	ist_x+=ABS(step_x);
	if(check_marble()) {
	  step_x=-step_x;
	  marble_sx=-marble_sx;
	  marble_x+=2*step_x;
	}
      }
    }
  }

  /* remaining step X */
  if(ist_x<ABS(marble_ox)) {
    step_x = (ABS(marble_ox)-ist_x)*SGN(marble_sx);
    marble_x += step_x;

    /* did this change the pixel position? */
/*    if( ((marble_x-step_x)&0xff00) != (marble_x & 0xff00)) */ {
      if(check_marble()) {
	marble_sx=-marble_sx;
	marble_x -= 2*step_x;
      }
    }
  }

  /* remaining step Y */
  if(ist_y<ABS(marble_oy)) {
    step_y = (ABS(marble_oy)-ist_y)*SGN(marble_sy);
    marble_y += step_y;
    /* did this change the pixel position? */
/*    if( ((marble_y-step_y)&0xff00) != (marble_y & 0xff00)) */ {
      if(check_marble()) {
	marble_sy=-marble_sy;
	marble_y -= 2*step_y;
      }
    }
  }

  /* walking on swamp */
  if(marble_extra&GND_SWAMP) {
    marble_sx = (SWAMP_SLOW*(long)marble_sx)/100;
    marble_sy = (SWAMP_SLOW*(long)marble_sy)/100;    
  } else {
    /* walking on ice or flying? */
    if(!(marble_extra&(GND_ICE|GND_SPACE))) {
      if(!marble_oil) {
	marble_sx = (SLOW*(long)marble_sx)/100;
	marble_sy = (SLOW*(long)marble_sy)/100;
      } else {
	marble_sx -= ((OIL_RELOAD-marble_oil) * (((100-SLOW) * (long)marble_sx)/100))/OIL_RELOAD;
	marble_sy -= ((OIL_RELOAD-marble_oil) * (((100-SLOW) * (long)marble_sy)/100))/OIL_RELOAD;
      }
    }
  }

  /* new acceleration applied by some tile? */
  if(marble_ax!=0) { marble_sx=marble_ax; marble_ax=0; }
  if(marble_ay!=0) { marble_sy=marble_ay; marble_ay=0; }

  /* clear all flags except SPACE, REVERSE, CARRY_MATCH and DROP_BOMB */
  marble_extra&=(GND_SPACE|GND_REVERSE|CARRY_MATCH|DROP_BOMB);  

  *x=marble_x>>8;
  *y=marble_y>>8;
} 

/* user pushed marble */
void marble_push(int x, int y) {
  long friction;

  friction = OIL_RELOAD-marble_oil;

  /* flying in free space with parachute? */
  if(!(marble_extra&GND_SPACE)) {
    /* on force reversing tile? */
    if(marble_extra & GND_REVERSE) {
      marble_sx -= ((long)(x<<SENSE)*friction)/(long)OIL_RELOAD;
      marble_sy -= ((long)(y<<SENSE)*friction)/(long)OIL_RELOAD;
    } else {
      marble_sx += ((long)(x<<SENSE)*friction)/(long)OIL_RELOAD;
      marble_sy += ((long)(y<<SENSE)*friction)/(long)OIL_RELOAD;
    }
  }
}

void ignite(int x, int y) {
  switch(level_buffer[y][x]) {

    /* explosion ignites neighbouring bombs */
  case BOMB:
    if(attribute_buffer[y][x]==0)
      attribute_buffer[y][x]=BOMB_TIMEOUT2;
    break;

  case BOMBD:
  case SWITCH_ON:
  case SWITCH_OFF:
  case KEYHOLE:
    change_tile(STONE,0,x,y);
    break;

    /* box/doc/key/door vanishes */
  case BOX:
  case LBOX:
  case HDOOR_1:
  case HDOOR_2:
  case HDOOR_3:
  case HDOOR_4:
  case VDOOR_1:
  case VDOOR_2:
  case VDOOR_3:
  case VDOOR_4:
  case DOC:
  case KEY:
    change_tile(PATH, 0, x, y);
    break;

    /* ice becomes empty space */
  case ICE:
    change_tile(SPACE, 0, x, y);
    break;

  }
}

/* check if marble touches tile at this position */
Boolean
check_marble_pos(int nx, int ny) {
  int ix, iy, x, y, t;

  ix = (marble_x>>8)&15; x = (marble_x>>8)/16 + marble_xp;
  iy = (marble_y>>8)&15; y = (marble_y>>8)/16 + marble_yp;

  /* marble is standing on this tile */
  if((x==nx)&&(y==ny)) return true;
  
  /* marble is touching this tile horizontally ? */
  if((((ix>9)&&(x+1==nx))||((ix<5)&&(x-1==nx)))&&(y==ny)) return true;

  /* marble is touching this tile vertically? */
  if((((iy>9)&&(y+1==ny))||((iy<5)&&(y-1==ny)))&&(x==nx)) return true;

  /* ... and diagonally ... */
  if((t=hole[iy][ix])&0xf8) {
    if(((t&DIR_OL)&&((x-1==nx)&&(y-1==ny)))||
       ((t&DIR_UL)&&((x-1==nx)&&(y+1==ny)))||
       ((t&DIR_OR)&&((x+1==nx)&&(y-1==ny)))||
       ((t&DIR_UR)&&((x+1==nx)&&(y+1==ny)))) {
      return true;
    }
  }
  return false;
}

/* do the animations */
void do_animations(void) {
  int p,x,y,nx,ny;
  static unsigned int ani_cnt=0;

  /* timeout for parachute */
  if(marble_parachute>0) {
    if(marble_parachute--<PARA_FLICKER)
      create_sprite(MARBLE,(marble_parachute&2)?BALLSH:BALL,7,7);
    else
      create_sprite(MARBLE,(marble_parachute&4)?BALLSH:BALL,7,7);
  }

  /* conways game of life (slow animation)   */
  /* birth = empty+3, stay = 2/3, death = x<2 or x>3 */
  if(play_life && ((ani_cnt&15)==2)) {
    for(y=1;y<(level_height-1); y++) {
      for(x=1;(x<level_width-1); x++) {
	/* BOX=cells/PATH=empty space */
	if((level_buffer[y][x]==BOX)||
	   (level_buffer[y][x]==LBOX)||
	   (level_buffer[y][x]==PATH)||
	   (level_buffer[y][x]==ICE)||
	   (level_buffer[y][x]==KEY)) {

	  /* determine number of neighbouring boxes */
	  p = ((level_buffer[y-1][x-1]==BOX)?1:0) +
	      ((level_buffer[y-1][x]  ==BOX)?1:0) +
	      ((level_buffer[y-1][x+1]==BOX)?1:0) +
	      ((level_buffer[y]  [x-1]==BOX)?1:0) +
	      ((level_buffer[y]  [x+1]==BOX)?1:0) +
	      ((level_buffer[y+1][x-1]==BOX)?1:0) +
	      ((level_buffer[y+1][x]  ==BOX)?1:0) +
	      ((level_buffer[y+1][x+1]==BOX)?1:0);

	  /* rule of death */
	  if((level_buffer[y][x]==BOX)&&((p>3)||(p<2)))
	    attribute_buffer[y][x]|=0x80;

	  /* rule of birth */
	  if((level_buffer[y][x]==PATH)&&(p==3))
	    attribute_buffer[y][x]|=0x80;
	}
      }
    }

    /* ok, really do the modifications */
    for(y=1;y<(level_height-1); y++) {
      for(x=1;(x<level_width-1); x++) {
	if((level_buffer[y][x]==BOX)||
	   (level_buffer[y][x]==LBOX)||
	   (level_buffer[y][x]==PATH)||
	   (level_buffer[y][x]==ICE)||
	   (level_buffer[y][x]==KEY)) {

	  if(attribute_buffer[y][x]&0x80) {
	    if(level_buffer[y][x]==BOX ||
	       level_buffer[y][x]==LBOX ) {
	      /* BOX -> PATH */
	      attribute_buffer[y][x]=0;
	      level_buffer[y][x]=PATH;
	    } else {
	      /* PATH/ICE/KEY -> BOX standing on PATH/ICE/KEY */
	      attribute_buffer[y][x]=level_buffer[y][x];
	      level_buffer[y][x]=BOX;
	    }
	  }
	}
      }
    }
  }

  if((ani_cnt&0x0f)==2) {
    for(y=0;y<level_height; y++) {
      for(x=0;x<level_width; x++) {

	p = level_buffer[y][x];

	/* walker tile */
	if((p>=MOVE_R)&&(p<=MOVE_U)) {

	  if(p == MOVE_R) { nx = x + 1; ny = y; }
	  if(p == MOVE_L) { nx = x - 1; ny = y; }
	  if(p == MOVE_U) { nx = x; ny = y - 1; }
	  if(p == MOVE_D) { nx = x; ny = y + 1; }

	  /* block must not leave playing area */
	  if((nx>=0)&&(nx<=(level_width-1))&&
	     (ny>=0)&&(ny<=(level_height-1))) {

	    /* walker can walk onto PATH */
	    if(level_buffer[ny][nx] == PATH) {
	      /* change destination buffer */
	      change_tile(level_buffer[y][x] - MOVE_R + NUM_OF_TILES, level_buffer[ny][nx], nx, ny);
	      if(check_marble_pos(nx, ny)) {
		change_tile(level_buffer[y][x], level_buffer[ny][nx], nx, ny);
		change_tile(attribute_buffer[y][x], 0, x, y);
		game_end(LOOSE);
	      }

	      /* double border tile */
	      if((nx==0)||(nx==(level_width-1))||
		 (ny==0)||(ny==(level_height-1))) {
		if(nx==0) nx=level_width-1;  else if(nx==level_width-1) nx=0;
		if(ny==0) ny=level_height-1; else if(ny==level_height-1) ny=0;

		/* change destination buffer */
		change_tile(level_buffer[y][x] - MOVE_R + NUM_OF_TILES, level_buffer[ny][nx], nx, ny);
		if(check_marble_pos(nx, ny)) {
		  change_tile(level_buffer[y][x], level_buffer[ny][nx], nx, ny);
		  change_tile(attribute_buffer[y][x], 0, x, y);
		  game_end(LOOSE);
		}
	      }
	      change_tile(attribute_buffer[y][x], 0, x, y);
	    }
	  } else
	    change_tile(attribute_buffer[y][x], 0, x, y);
	}
      }
    }
  }


  for(y=0;y<level_height; y++) {
    for(x=0;x<level_width; x++) {
      switch(level_buffer[y][x]) {

	/* a magnet */
      case MAGNET_N:
      case MAGNET_P:
	nx = x - (((marble_x>>8))/16 + marble_xp);
	ny = y - (((marble_y>>8))/16 + marble_yp);

	/* square of distance to magnet */
	p = nx*nx + ny*ny;
	if(level_buffer[y][x] == MAGNET_N) p*=-1;

	if(p!=0) {
	  marble_sx += 200*nx/p;
	  marble_sy += 200*ny/p;
	}
	break;
	
	/* slot counter */
      case SLOT1:
      case SLOT2:
      case SLOT3:
      case SLOT4:
      case SLOT5:
      case SLOT6:
      case SLOT7:
	if((ani_cnt&0x0f)==0) {
	  attribute_buffer[y][x]-=0x20;

	  /* next slot tile */
	  if((attribute_buffer[y][x]&0xe0)==0xc0)
	    change_tile(level_buffer[y][x]+1, (attribute_buffer[y][x]&0x1f)|0xe0, x,y);
	  else if((attribute_buffer[y][x]&0xe0)==0x00)
	    change_tile(level_buffer[y][x]+1, (attribute_buffer[y][x]&0x1f)|0xa0, x,y);
	}
	break;

      case SLOT8:
	if((ani_cnt&0x0f)==0) {
	  attribute_buffer[y][x]-=0x20;

	  /* unused slot tile and action */
	  if(((attribute_buffer[y][x]&0xe0)==0xc0)||((attribute_buffer[y][x]&0xe0)==0x00)) {
	    change_tile(SLOT, attribute_buffer[y][x]&0x1f, x,y);
	    switch_it(attribute_buffer[y][x]);
	  }
	}
	break;

	/* count down bomb */
      case BOMB:
      case BOMBI:
	if(!(ani_cnt&3)) {
	  if(attribute_buffer[y][x]>0) {
	    attribute_buffer[y][x]-=1;

	    if(attribute_buffer[y][x]!=0)
	      level_buffer[y][x]=(attribute_buffer[y][x]&1)?BOMB:BOMBI;
	    else
	      level_buffer[y][x]=EXPLODE;
	  }
	}
	break;

	/* explosion */
      case EXPLODE:
	if(!(ani_cnt&3)) {
	  snd_clic();
	  change_tile(HOLE, 0, x, y);

	  /* explosion ignites neighbour */
	  ignite(x+1,y);
	  ignite(x,y+1);
	  ignite(x-1,y);
	  ignite(x,y-1);
	}
	break;

	/* flash bumper */
      case BUMPL:
	change_tile(BUMP, 0, x,y);
	break;
    
	/* door */
      case HDOOR_1:
      case HDOOR_2:
      case HDOOR_3:
      case HDOOR_4:
      case VDOOR_1:
      case VDOOR_2:
      case VDOOR_3:
      case VDOOR_4:
	if(!(ani_cnt&3)) {
	  /* opening/open door */
	  if(attribute_buffer[y][x]&OPENING) {
	    if((level_buffer[y][x]!=HDOOR_4)&&
	       (level_buffer[y][x]!=VDOOR_4)) {
	      change_tile(level_buffer[y][x]+1, ATTRIB_ORIG, x,y);
	    }
	  }
	     
	  /* closing/closed door */
	  if(!(attribute_buffer[y][x]&OPENING)) {
	    if((level_buffer[y][x]!=HDOOR_1)&&
	       (level_buffer[y][x]!=VDOOR_1)) {
	      change_tile(level_buffer[y][x]-1, ATTRIB_ORIG, x,y);
	    }
	  }
	}
	break;
	    
	/* rotate ventilator */
      case HVENT_1:
      case HVENT_2:
      case HVENT_3:
      case HVENT_4:
	if((!(ani_cnt&3))&&(attribute_buffer[y][x]&ROTATING)) {
	  /* counter up */
	  if(level_buffer[y][x]<HVENT_4) 
	    level_buffer[y][x]+=1;
	  else                     
	    level_buffer[y][x]=HVENT_1;
	}
	break;

      case  LBROKEN_1:
	if(!(ani_cnt&16))
	  change_tile(LBROKEN_2, -1, x, y);
	break;
      case  LBROKEN_2:
	if(!(ani_cnt&16))
	  change_tile(LBROKEN_3, -1, x, y);
	break;
      case  LBROKEN_3:
	if(!(ani_cnt&16))
	  change_tile(LBROKEN_4, -1, x, y);
	break;
      case  LBROKEN_4:
	if(!(ani_cnt&16))
	  change_tile(attribute_buffer[y][x], 0, x, y);
	break;

	/* cloaked walker tile */
      case NUM_OF_TILES:
      case NUM_OF_TILES+1:
      case NUM_OF_TILES+2:
      case NUM_OF_TILES+3:
	/* real swapper doesn't have attribute */
	if(attribute_buffer[y][x] != 0)
	  level_buffer[y][x] = (level_buffer[y][x] - NUM_OF_TILES + MOVE_R);
	break;
	
      }
    }
  }

  if ( ani_cnt%10 == 9 ) do_scarabs();

  /* increase animation counter */
  ani_cnt++;
}

/* sound and tilt indication */
void draw_sound_tilt_indication(int son, int ton) {
  MemHandle resH;
  BitmapPtr resP;

  resH = (MemHandle)DmGetResource( bitmapRsc, son?SoundOn:SoundOff );
  ErrFatalDisplayIf( !resH, "Missing bitmap" );
  resP = MemHandleLock((MemHandle)resH);
  WinDrawBitmap (resP, 5, 3);
  MemPtrUnlock(resP);
  DmReleaseResource((MemHandle) resH );

  resH = (MemHandle)DmGetResource( bitmapRsc, ton?TiltOn:TiltOff );
  ErrFatalDisplayIf( !resH, "Missing bitmap" );
  resP = MemHandleLock((MemHandle)resH);
  WinDrawBitmap (resP, 5, 16);
  MemPtrUnlock(resP);
  DmReleaseResource((MemHandle) resH );
}

/****************************************************************************/
/* beam stuff (transfer levels to other machines)                           */
/* this hiscore reset routine is extremely dirty and may malfunctions       */
/* with newer palm os versions                                              */
int beam_reset;

static Err  BeamBytes(ExgSocketPtr s, void *buffer, 
		      unsigned long bytesToSend) {
   Err err = 0;
   const unsigned long zero = 0;
   const unsigned long effeff = 0xffffffff;

   if(beam_reset) {
     if(beam_reset == 2) {
       ExgSend(s, &zero, 4, &err); bytesToSend -= 4;
       while(bytesToSend > 0) {
	 ExgSend(s, &effeff, (bytesToSend>4)?4:bytesToSend, &err);
	 bytesToSend -= 4;
       }

       beam_reset = 1;
       return err;
     }

     /* THIS is dirty */
     if(bytesToSend == 2) beam_reset = 2;
   }
   
   while (!err && bytesToSend > 0) {
      unsigned long bytesSent = ExgSend(s, buffer, bytesToSend, &err);
      bytesToSend -= bytesSent;
      buffer = ((char *) buffer) + bytesSent;
   }
   return err;
}

Err
write_proc(const void *data, unsigned long *size, void *udata) {
  Err err;
  ExgSocketPtr s = (ExgSocketPtr)udata;

  CALLBACK_PROLOGUE;

  /* send data ... */
  err = BeamBytes(s, (void*)data, *size);
 
  CALLBACK_EPILOGUE;
  return err;
}

void 
db_beam(char *name) {
   ExgSocketType  s;
   Err      err;

   MemSet(&s, sizeof(s), 0);
   s.description = name;
   s.target = 'lnch';        /* beam to launcher */
   s.length = 1;
   s.goToCreator = CREATOR;  /* hmm, this doesn't seem to work */

   beam_reset = FrmAlert(alt_HSerase);

   if (!ExgPut(&s)) {
     /* beam db */
     err = ExgDBWrite(write_proc, &s, s.description, 0, 0);
     err = ExgDisconnect(&s, err);
   }
}

static Boolean MainFormHandleEvent(EventPtr event)
{
  FormPtr frm;
  Boolean handled = false;
  int i;
  char str[4];

  switch (event->eType) {

  case ctlRepeatEvent:

    switch(event->data.ctlEnter.controlID) {

    case LevelUp:
      level_no++;
      if(level_no>max_enabled) level_no=max_enabled;
      if(level_no>(levels-1))  level_no=levels-1;
      fill_form();
      break;
    
    case LevelDown:
      if(--level_no==0xffff)   level_no=0;
      else  fill_form();
      break;

    case DBUp:
      db_no++;
      if(db_no>(num_db-1)) db_no=num_db-1;
      else {
	/* save setting for open database */
	settings(true, &max_enabled, &level_no);
	/* close it */
	close_database();

	/* open the new one, otherwiese reopen old one */
	if(open_database(db_no)!=0) {
	  db_no--;
	  open_database(db_no);
	}

	/* read the settings */
	settings(false, &max_enabled, &level_no);
	/* and display them */
	fill_form();
      }
      break;

    case DBDown:
      db_no--;
      if(db_no==0xffff) db_no=0;
      else {
	/* save setting for open database */
	settings(true, &max_enabled, &level_no);
	/* close it */
	close_database();

	/* open the new one if possible, otherwise reopen old one */
	if(open_database(db_no)!=0) {
	  db_no++;
	  open_database(db_no);
	}

	/* read the settings */
	settings(false, &max_enabled, &level_no);
	/* and display them */
	fill_form();
      }
      break;
    }
    break;
 
  case ctlSelectEvent:
    /* Game Info */
    if(event->data.ctlEnter.controlID == GameInfo) {
      StrPrintF(str, "%d", levels);

      i = FrmCustomAlert(alt_GameInfo, level_database[db_no].name, author, str);
      if(i==0) {
	if(!xtra_open) {
	  if(num_db>1) {
	    if(FrmCustomAlert(alt_DelDB, level_database[db_no].name,0,0)==1) {
	      /* close it */
	      close_database();
	      
	      /* delete database */
	      DmDeleteDatabase(0, level_database[db_no].dbid);
	    
	      db_no=0;
	      scan_databases();
	      
	      /* open the new one */
	      open_database(db_no);
	      /* read the settings */
	      settings(false, &max_enabled, &level_no);
	      /* and display them */
	      fill_form();
	    }
	  } else
	    FrmCustomAlert(alt_err, "You can't delete the last game.",0,0);
	  /* ... erase whole game instead */
	} else {
	    FrmCustomAlert(alt_err, "You can't delete a ROM based game.",0,0);
	}
      } else if(i==1) {
	db_beam(level_database[db_no].name);
      }
    }
    
    if(event->data.ctlEnter.controlID == SoundInd) {
      sound_on=!sound_on;
      draw_sound_tilt_indication(sound_on, tilt_on&ACC_ON);
      handled = true;
    }

    if(event->data.ctlEnter.controlID == Info) {
      frm=FrmInitForm(InfoForm);
      FrmDoDialog(frm);
      FrmDeleteForm(frm);
      handled = true;
    }

    if(event->data.ctlEnter.controlID == TiltInd) {

      if(tilt_on & ACC_PRESENT) {

	TiltLibZero(TiltRef);
	frm=FrmInitForm(AccSetup);

	CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, AccOn)),
		    tilt_on&ACC_ON);
	CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, AccFlipX)),
		    tilt_on&ACC_FLIP_X);
	CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, AccFlipY)),
		    tilt_on&ACC_FLIP_Y);
	CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, AccFlipXY)),
		    tilt_on&ACC_FLIP_XY);
	
	if(FrmDoDialog(frm) == AccOK) {
	  tilt_on = (tilt_on & ACC_PRESENT)|
	    (CtlGetValue(FrmGetObjectPtr (frm, 
                      FrmGetObjectIndex (frm, AccOn)))?ACC_ON:0)|
	    (CtlGetValue(FrmGetObjectPtr (frm, 
                      FrmGetObjectIndex (frm, AccFlipX)))?ACC_FLIP_X:0)|
	    (CtlGetValue(FrmGetObjectPtr (frm, 
                      FrmGetObjectIndex (frm, AccFlipY)))?ACC_FLIP_Y:0)|
	    (CtlGetValue(FrmGetObjectPtr (frm, 
                      FrmGetObjectIndex (frm, AccFlipXY)))?ACC_FLIP_XY:0);
	}
	
	FrmDeleteForm(frm);
	
	draw_sound_tilt_indication(sound_on, tilt_on&ACC_ON);
      } else {
	if(TiltRef == 0)  
	  FrmCustomAlert(alt_err, "No tilt sensor support installed.",0,0);

	else if(tilt_err == TILT_WRONG_OS)  
	  FrmCustomAlert(alt_err, "Wrong tilt sensor support installed.",0,0);

	else if(tilt_err == TILT_NO_SENSOR) 
	  FrmCustomAlert(alt_err, "Tilt sensor hardware not found.",0,0);

	else if(tilt_err == TILT_HW_BUSY) 
	  FrmCustomAlert(alt_err, "Tilt sensor interface is busy.",0,0);

	else
	  FrmCustomAlert(alt_err, "Unknown error during tilt sensor detection.",0,0);
      }
      handled = true;	
    }

    if(event->data.ctlEnter.controlID == PlayButton) {

      if(db_loaded) {
	game_state=RUNNING;
	init_game();
	if (WinXSetGreyscale() != 0) {
	  game_state=SETUP;
	} else {
	  redraw_screen();
	  redraw_screen();

	  if(sound_on) {
	    /* start sound */
	    for(i=100;i<400;i+=6) {
	      snd_kill.param1=i;
	      SndDoCmd(NULL, &snd_kill, false); 
	    }
	  }
	}
      }
      handled=true;
    }
    break;
    
  }
  return handled;
}

#ifdef OD35HDR
UInt32	PilotMain(UInt16 cmd, void *cmdPBP, UInt16 launchFlags)
#else
DWord   PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
#endif
{
  short err;
  EventType e;
  FormType *pfrm;
  Boolean fMainWindowActive, handled;
  UInt16  ticks_wait,x,y;
  long  last_ticks=0;
  UInt16  size;
  SystemPreferencesType sysPrefs;
  UInt16 tx,ty;
  UInt16 numapps;
  DWord newDepth, depth;
  int wPalmOSVerMajor;
  long dwVer;

  if (cmd == sysAppLaunchCmdNormalLaunch) {
    /* enter best video mode available */

    /* check for OS3.0 */
    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &dwVer);
    wPalmOSVerMajor = sysGetROMVerMajor(dwVer);
    has_newscr = (wPalmOSVerMajor >= 3);    

    if(has_newscr) {

      /* if BW, try to enter greyscale */
      ScrDisplayMode(scrDisplayModeGet, NULL, NULL, &depth, NULL);
      if(depth == 1) {
	ScrDisplayMode(scrDisplayModeGetSupportedDepths, NULL, NULL, &depth, NULL);
	
	if(depth & ((1<<1)|(1<<3))) {
	  
	  /* is 4bpp or 2bpp supported? */
	  if (depth & (1<<3))     newDepth = 4;
	  else if(depth & (1<<1)) newDepth = 2;
	
	  ScrDisplayMode(scrDisplayModeSet, NULL, NULL, &newDepth, NULL);
	}
      }
    }

    /* try to open tilt sensor lib */
    err = SysLibFind("Tilt Sensor Lib", &TiltRef);
    if (err) err = SysLibLoad('libr', 'Tilt', &TiltRef);
    
    if(!err) {
      /* try to initialize sensor */
      tilt_err = TiltLibOpen(TiltRef);

      if(tilt_err != 0) {
	tilt_on = 0; 
      } else {
	/* tilt sensor is ok */
	tilt_on = ACC_PRESENT;
      }
    } else {
      tilt_on = 0;
      TiltRef = 0; /* library is not open */
    }

    if(!scan_databases())     /* search for mulg level databases */
      return 0;               /* no databases found? exit! */

    size = sizeof(prefs);
    x=PrefGetAppPreferences( CREATOR, 0, &prefs, &size, true);

    db_no=0;

    sound_on=1;

    /* get lib from prefs */
    if((x!=noPreferenceFound)&&(prefs.version==PREFS_VERSION)) {
      for(x=0;x<num_db;x++) {
	if(StrCompare(level_database[x].name,prefs.db_name)==0)
	  db_no=x;
      }
      sound_on=prefs.sound;

      /* read tilt sensor preferences */
      if(tilt_on & ACC_PRESENT)
	tilt_on = (prefs.tilt & 0x0f) | ACC_PRESENT;
    }
  
    if(open_database(db_no)!=0)
      return;

    /* read settings from database */
    settings(false, &max_enabled, &level_no);

    /* get system preferences for sound */
    PrefGetPreferences(&sysPrefs);

    /* My system does not have gameSoundLevelV20 -- PEK'99 */
    /* then upgrade it :-) - Till */
    if(sysPrefs.gameSoundLevelV20 == slOff) sound_on=0;

    /* initialize Timer */
    ticks_wait = SysTicksPerSecond()/50;  /* 20ms/frame */

    FrmGotoForm(RscForm1);

    while(1) {
      handled = false;

      EvtGetEvent(&e, ticks_wait);

      if(game_state!=SETUP)
	if (WinXPreFilterEvent(&e))
	  continue;

      if (SysHandleEvent(&e)) 
	continue;

      if (MenuHandleEvent((void *)0, &e, &err)) 
	continue;
	
      /* formerly in winEnterEvent: */
      /* left greyscale mode? (may happen due to alarm etc) */
      if((game_state!=SETUP)&&(!disp_grey())) {
	if (WinXSetGreyscale() != 0) {
	  FrmCustomAlert(alt_err, "Out of memory!",0,0);
	  game_state=SETUP;
	}
      }

      switch (e.eType) {
      case frmLoadEvent:	
	pfrm = FrmInitForm(e.data.frmLoad.formID);
	FrmSetActiveForm(pfrm);
	FrmSetEventHandler(pfrm, MainFormHandleEvent);
	handled = true;
 	break;

      case frmOpenEvent:
	pfrm = FrmGetActiveForm();
	FrmDrawForm(pfrm);
	fill_form();
	draw_sound_tilt_indication(sound_on, tilt_on&ACC_ON);
	handled = true;
	break;

      case penDownEvent:
	switch(game_state) {
	case MESSAGE:
	  snd_clic();
	  game_state = RUNNING;
	  handled = true;
	  break;

	case RUNNING:
	  /* collection bar */
	  if((e.screenY>143)&&(e.screenY<160)) {
	    if(x=get_from_collection(e.screenX>>4)) {
	      if((x&0xff)==DOCX)  draw_message(x>>8);
	      if((x&0xff)==BOMBX) marble_extra |= DROP_BOMB;
	    }
	  }
	  handled = true;
	  break;
	}
	
	pen_last_x = e.screenX;
	pen_last_y = e.screenY;
	break;

      case penMoveEvent:
	if(game_state==RUNNING)
	  marble_push(e.screenX - pen_last_x, e.screenY - pen_last_y);

	pen_last_x = e.screenX;
	pen_last_y = e.screenY;
	break;

	/* tap menu soft key to stop game */
      case keyDownEvent:
	if(e.data.keyDown.chr==menuChr) {
	  game_end(LOOSE);
	  handled = true;
	}
	break;

      case appStopEvent:
	/* save name of current database */
	MemMove(prefs.db_name, level_database[db_no].name, 32);
	prefs.version = PREFS_VERSION;
	prefs.sound   = sound_on;
	prefs.tilt    = tilt_on;
	PrefSetAppPreferences( CREATOR, 0, 1, &prefs, sizeof(prefs), true);
	settings(true, &max_enabled, &level_no);

	if(game_state != SETUP) WinXSetMono();

	/* close open db */
	close_database();

	/* release database buffer */
	if(level_database != NULL) MemPtrFree(level_database);

	/* close sensor library */
	if(TiltRef!=0) {
	  TiltLibClose(TiltRef, &numapps);

	  /* Check for errors in the Close() routine */
	  if (numapps == 0) {
	    SysLibRemove(TiltRef);
	  }
	}

	/* return to default video mode */
	if(has_newscr)
	  ScrDisplayMode(scrDisplayModeSetToDefaults, NULL, NULL, NULL, NULL); 

	return 0;
      }

      if(!handled)
	FrmDispatchEvent(&e);

      /* do some kind of 50Hz timer event */
      if(TimGetTicks()>last_ticks) {
	if(game_state==RUNNING) {
	  do_animations();
	  move_marble(&x, &y);
	}

	/* game state may change in move_marble() */
	if(game_state==RUNNING) {

	  if(tilt_on & ACC_ON) {
	    /* toggle auto off timer */
	    EvtResetAutoOffTimer();

	    if(tilt_on & ACC_FLIP_XY)  TiltLibGet(TiltRef, &ty, &tx);
	    else                       TiltLibGet(TiltRef, &tx, &ty);
	      
	    if((tx>-2)&&(tx<2)&&(ty>-2)&&(ty<2)) tx=ty=0;
	      
	    marble_push((tilt_on&ACC_FLIP_X)?(-tx/2):(tx/2), 
			(tilt_on&ACC_FLIP_Y)?(ty/2):(-ty/2));
	  }

	  level_time+=40;
	  redraw_screen();
	}

	last_ticks=TimGetTicks()+ticks_wait;
      }
    }
  }

  return 0;
}





