
#include <Pilot.h>
#include <string.h>
#include <irlib.h>
#include <KeyMgr.h>
#include <System/DLServer.h>
#include "callback.h"
#include "IR-PongRsc.h"

#define FREEWARE

#define COMM_VERSION  1  /* version of communication protocol */

#define CREATOR 'IRPn'   /* same as in Makefile */

#define noSINGLE_GAME   // define this to test without IR connection

/* master packet types */
#define MSTR_BALL  0
#define MSTR_END   1

#define PANIC_STOP    4   /* four lost packets: stop ball */
#define PANIC_TIMEOUT 50  /* connection lost after 2 sec */

char *ir_status_str[]={
  "Successful and complete","Operation failed","Successfully started but pending",
  "Link disconnected","No IrLAP Connection exists","IR Media is busy",
  "IR Media is not busy","IrLAP not making progress","No progress condition cleared"
};

char *ir_ias_type_str[]={
  "IAS_ATTRIB_MISSING","IAS_ATTRIB_INTEGER","IAS_ATTRIB_OCTET_STRING",
  "IAS_ATTRIB_USER_STRING","IAS_ATTRIB_UNDEFINED" /* 0xff */
};

char *ir_ias_return_str[]={
  "Operation successful","No such class","No such attribute","Operation unsupported" /* 0xff */
};

// List of bitmaps
typedef enum { Paddle=0, Ball, Brick0, Brick1, Brick2, bitmapTypeCount} bitmap_id;

static Handle	  ObjectBitmapHandles[bitmapTypeCount];
static BitmapPtr  ObjectBitmapPtr[bitmapTypeCount];
static WinHandle  ObjectWindowHandles[bitmapTypeCount];

struct setup {
  unsigned char ball_speed:2,
                ball_inc:2,
                paddle_speed:2,
                sound:1;
  unsigned char board;
} setup;

/* different states the game can be in */
typedef enum {SETUP=0, RUNNING, WAITING, SLEEPING, NEW_GAME, WRONG_VERSION, SLAVE_RUNNING, SLAVE_SETUP, SLAVE_ACK} game_state;

/* 11 longs for board setup */
unsigned long levels[][11]={
  {0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000},  /* none */
  {0x00000, 0x00000, 0x00000, 0x00000, 0x00200, 0x30983, 0x00200, 0x00000, 0x00000, 0x00000, 0x00000},  /* 1 */
  {0x00000, 0x00000, 0x11111, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x11111, 0x00000, 0x00000},  /* 2 */
  {0x00000, 0x03030, 0x02020, 0x03030, 0x02020, 0x03030, 0x02020, 0x03030, 0x02020, 0x03030, 0x00000},  /* 3 */
  {0x00000, 0x3901b, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x3901b, 0x00000},  /* 4 */
  {0x10000, 0x0c000, 0x01000, 0x00c00, 0x00000, 0x00000, 0x00000, 0x000c0, 0x00010, 0x0000c, 0x00001},  /* 5 */
  {0x00000, 0x00000, 0x00000, 0x00fc0, 0x03ab0, 0x039b0, 0x03ab0, 0x00fc0, 0x00000, 0x00000, 0x00000},  /* 6 */
  {0x00000, 0x00000, 0x00000, 0x10001, 0x2800a, 0x3f03f, 0x2800a, 0x10001, 0x00000, 0x00000, 0x00000},  /* 7 */
  {0x00000, 0x00000, 0x039b0, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x039b0, 0x00000, 0x00000}   /* 8 */
};

unsigned long *playfield;

/* Packet sent from Master to Client */
typedef struct { 
  unsigned char type;
  unsigned char ball_x, ball_y;
  unsigned char paddle_pos;
} master_state;

typedef struct {
  unsigned char paddle_pos;
  unsigned char buttons;
} slave_state;

master_state master;
slave_state slave;

int sleep_counter;
int wrong_version;

#define PREFS_VERSION 3

typedef struct {
  char version;
  unsigned long score[2];   /* won/lost games */
  int reg_id;               /* registration id */
  ULong firstStart;         /* date of first Start */
  struct setup setup;       /* player/ball settings */
} PongPrefs;

Boolean packet_handled=false;
WinHandle draw_win;

int winner, level, slave_level;

/////////////////////////
// Function Prototypes
/////////////////////////

static Boolean StartApplication(void);
static Boolean MainFormHandleEvent(EventPtr event);
static Boolean ApplicationHandleEvent(EventPtr event);
static void    EventLoop(void);

UInt ir_ref=0;

game_state state = SETUP;
int i_am_master;

static MenuBarPtr CurrentMenu = NULL;
FormPtr mainform;

/* Ir stuff: */
char dev_name[]={IR_HINT_PDA, 0, 'P','a','l','m','E'};

IrConnect con;
IrIasObject ias_object;
char ias_name[]="Pong";                      /* Class name */
IrIasAttribute attribs;                      /* A pointer to an array of attributes */
char attr_name[]="IrDA:IrLMP:LsapSel";
char attr_value[]={ IAS_ATTRIB_INTEGER, 0,0,0,42};

#define RESULT_SIZE 32
IrIasQuery query;
char rlsap_query[]="\004Pong\022IrDA:IrLMP:LsapSel";
char result[RESULT_SIZE];

IrPacket packet;
char ir_data[128];

Word  panic=0;

volatile int max_tx=0, pkts=0;
unsigned char master_buttons, slave_buttons;

const int paddle_step_table[]={3,4,6};
int paddle_step;

//const int ball_step_table[]={2,3,5};
const int ball_step_table[]={4,6,10};
int ball_step;

const int ball_inc_table[]={3600,1200,400};
int ball_inc;

/* do some sound */
typedef enum { SndPaddle=0, SndBrick, SndWall, SndLost } sound;

struct SndCommandType snd_data[] = {
  {sndCmdFreqDurationAmp,400,1,sndMaxAmp},
  {sndCmdFreqDurationAmp,200,1,sndMaxAmp},
  {sndCmdFreqDurationAmp,100,1,sndMaxAmp},
  {sndCmdFreqDurationAmp,100,10,sndMaxAmp}};

unsigned char last_sound=0xff;

void do_sound(sound snd) {
  if(setup.sound)
    SndDoCmd(NULL, &snd_data[snd], 0); 

  last_sound = snd;
}

#ifdef USE_IR_INDICATION
/* ir indication */
void ir_indication(int on) {
  Handle    resH;
  BitmapPtr resP;
  RectangleType rect;
  FormPtr frm = FrmGetActiveForm ();
  CtlSetEnabled(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, PongIndicator)), on);

  if(on) {
    resH = (Handle)DmGetResource( bitmapRsc, IrIndIcon );
    ErrFatalDisplayIf( !resH, "Missing bitmap" );
    resP = MemHandleLock((VoidHand)resH);
    WinDrawBitmap (resP, 144, 1);
    MemPtrUnlock(resP);
    DmReleaseResource((VoidHand) resH );
  } else {
    rect.topLeft.x=144;  rect.topLeft.y=1; 
    rect.extent.x=16;    rect.extent.y=12;
    WinEraseRectangle(&rect,0);
  }
}
#endif

#define GAME_HZ         50
#define PADDLE_SHIFT    2

#define PADDLE_HEIGHT  8
#define PADDLE_WIDTH   32
#define SCREEN_WIDTH   160
#define SCREEN_HEIGHT  160
#define BALL_DIAMETER  8

#define BALL_SHIFT   7
#define GAME_AREA_Y  16
#define GAME_AREA_H (SCREEN_HEIGHT-GAME_AREA_Y)
#define BORDER_W  3

#define PADDLE_TOP_Y  GAME_AREA_Y
#define PADDLE_BOT_Y  (GAME_AREA_Y+GAME_AREA_H-PADDLE_HEIGHT)
#define PADDLE_CENTER ((SCREEN_WIDTH-PADDLE_WIDTH)/2) 
#define PADDLE_MIN BORDER_W
#define PADDLE_MAX (SCREEN_WIDTH-BORDER_W-PADDLE_WIDTH)
#define PADDLE_MIRROR(a) (SCREEN_WIDTH-PADDLE_WIDTH-(a))

#define BALL_CENTER_X ((SCREEN_WIDTH-BALL_DIAMETER)/2)
#define BALL_CENTER_Y (GAME_AREA_Y+(GAME_AREA_H/2)-(BALL_DIAMETER/2))
#define BALL_MIN_X ((BORDER_W)<<BALL_SHIFT)
#define BALL_MAX_X ((SCREEN_WIDTH-BALL_DIAMETER-BORDER_W)<<BALL_SHIFT)
#define BALL_MIN_Y ((GAME_AREA_Y)<<BALL_SHIFT)
#define BALL_MAX_Y ((GAME_AREA_Y+GAME_AREA_H-BALL_DIAMETER)<<BALL_SHIFT)
#define BALL_MIRROR_X(a) (SCREEN_WIDTH-BALL_DIAMETER-(a))
#define BALL_MIRROR_Y(a) (2*GAME_AREA_Y+GAME_AREA_H-BALL_DIAMETER-(a))

#define BRICK_HEIGHT  8
#define BRICK_WIDTH   16
#define BRICK_ROWS    11
#define BRICK_COLUMNS ((SCREEN_WIDTH - 2*BORDER_W - 1) / (BRICK_WIDTH+1))
#define BRICK_START   (GAME_AREA_Y+(GAME_AREA_H/2) - (((BRICK_HEIGHT+1) * BRICK_ROWS)/2)) 

#define MOVE_LEFT  1
#define MOVE_RIGHT 2
#define MOVE_START 4

#define ME  0
#define YOU 1

int  paddle_pos[2];
unsigned int  ball_x, ball_y;
int  ball_dx, ball_dy, ball_dir;

unsigned long  score[2], loc_score[2];

#ifndef FREEWARE
/******** registration stuff ************/
#define REG_DAYS   14
#define REG_SECDAY (60l * 60l * 24l) 
#define REG_SECS   (REG_DAYS * REG_SECDAY)

char noname[]="NoName";
Boolean  regd;
int      reg_id;
ULong    firstStart;
Boolean  slave_only;

void check_registration(int my_id) {
  int id=0,i=0,j=0,chr;
  char name[dlkUserNameBufSize];

  DlkGetSyncInfo(0, 0, 0, name, 0, 0);
  if(!StrLen(name)) StrCopy(name, noname);

  while(name[i]!=0) {
    chr = name[i];

    if((chr>32)&&(chr<127)) {

      if((chr+j) & 1) id ^= chr;
      else            id ^= (chr<<8);

      id ^= (0x1111<<(chr&3));

      id &= 0x7fff;
      j++;
    }
    i++;
  }

  regd = (id == my_id);
  reg_id = my_id;  /* save reg_id */
}

void register_me(void) {
  FormPtr frm;
  char name[dlkUserNameBufSize];
  int ret;

  do {
    frm=FrmInitForm(frm_Register);
//    FldGrabFocus(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, RegisterCode)));
    DlkGetSyncInfo(0, 0, 0, name, 0, 0);
    if(!StrLen(name)) StrCopy(name, noname);
    CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, RegisterName)), name);
    ret = FrmDoDialog(frm);
  
    /* tapped ok */
    if(ret == RegisterOK) {
      check_registration(StrAToI(FldGetTextPtr(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, RegisterCode)))));
      if(!regd) FrmAlert(alt_regwrong);
      else      FrmAlert(alt_regok);
    }
    
    FrmDeleteForm(frm);

  } while((ret == RegisterOK)&&(!regd));
}
#endif

void draw_game_area(void) {
  Word x,y,b;
  RectangleType rect;
//  CustomPatternType mypattern={0x99dd,0xffbb,0x99bb,0xffdd};
  CustomPatternType mypattern={0xbbdd,0xee77, 0xbbdd, 0xee77};

  /* clear */
  rect.topLeft.x=0;  rect.topLeft.y=GAME_AREA_Y; 
  rect.extent.x=SCREEN_WIDTH; rect.extent.y=GAME_AREA_H;
  WinEraseRectangle(&rect,0);

  /* draw border */
  WinSetPattern(mypattern);

  rect.extent.x=BORDER_W;
  WinFillRectangle(&rect,0);

  rect.topLeft.x=SCREEN_WIDTH-BORDER_W;
  WinFillRectangle(&rect,0);

  WinDrawBitmap(ObjectBitmapPtr[Paddle], paddle_pos[YOU], PADDLE_TOP_Y);
  WinDrawBitmap(ObjectBitmapPtr[Paddle], paddle_pos[ME], PADDLE_BOT_Y);
  WinDrawBitmap(ObjectBitmapPtr[Ball],  ball_x>>BALL_SHIFT, ball_y>>BALL_SHIFT);

  /* draw bricks */
  for(y=0;y<BRICK_ROWS;y++) {
    for(x=0;x<BRICK_COLUMNS;x++) {
      if((playfield[y]>>(x<<1))&3) {
	WinDrawBitmap(ObjectBitmapPtr[Brick0 + ((playfield[y]>>(x<<1))&3) -1], 
		      BORDER_W + 1 + x * (BRICK_WIDTH+1), 
		      BRICK_START + y*(BRICK_HEIGHT+1));
      }
    }
  }
}

void move_paddle(int no, int dir) {
  int new_pos;
  RectangleType rect;

  if(draw_win != WinGetDrawWindow())
    return;

  if(no==ME) {
    /* first remove the part of the old paddle not covered by the new one */
    new_pos = paddle_pos[no]+dir;
    if(new_pos<PADDLE_MIN) new_pos = PADDLE_MIN;
    if(new_pos>PADDLE_MAX) new_pos = PADDLE_MAX;
  } else
    new_pos = dir;

  if(new_pos!=paddle_pos[no]) {
    rect.topLeft.y = (no==ME)?PADDLE_BOT_Y:PADDLE_TOP_Y; 
    rect.extent.y = PADDLE_HEIGHT;

    if(new_pos<paddle_pos[no]) {
      rect.extent.x = paddle_pos[no]-new_pos;
      rect.topLeft.x = new_pos+PADDLE_WIDTH;
    } else {
      rect.extent.x = new_pos-paddle_pos[no];
      rect.topLeft.x = new_pos-rect.extent.x;
    }
    
    WinEraseRectangle(&rect,0);

    /* now draw the paddle */
    paddle_pos[no]=new_pos;
    WinDrawBitmap(ObjectBitmapPtr[Paddle], paddle_pos[no], 
		  (no==ME)?PADDLE_BOT_Y:PADDLE_TOP_Y);
  }
}

void main_draw_score(void) {
  char str[32];

  /* draw scores */
  FntSetFont(2);
  WinDrawChars("Games won:", 10, 20, 60);
  StrPrintF(str, "%lu", score[ME]);
  FntSetFont(6);
  WinDrawChars(str, StrLen(str), 140-FntLineWidth(str, StrLen(str)), 55);
  FntSetFont(2);
  WinDrawChars("Games lost:", 11, 20, 90);
  StrPrintF(str, "%lu", score[YOU]);
  FntSetFont(6);
  WinDrawChars(str, StrLen(str), 140-FntLineWidth(str, StrLen(str)), 85);
  FntSetFont(0);
}

void draw_score(void) {
  char score_str[32];

  if(draw_win != WinGetDrawWindow())
    return;

  FntSetFont(1);
  StrPrintF(score_str, "%lu:%lu", loc_score[ME], loc_score[YOU]);
  WinDrawChars(score_str, StrLen(score_str), 80-(FntLineWidth(score_str, StrLen(score_str))/2), 1);
}

void restart_game(void) {
  /* init paddles and ball */
  ball_dir  = 1+SysRandom(0)%5;  /* niemals waagerecht/senkrecht */
  ball_dx   = (SysRandom(0)&1)?-1:1;

#ifdef SINGLE_GAME
  ball_dy   = -1;
#else
  ball_dy   = (SysRandom(0)&1)?-1:1;
#endif

  ball_step = ball_step_table[setup.ball_speed];
  ball_inc  = ball_inc_table[setup.ball_inc];
  paddle_step = paddle_step_table[setup.paddle_speed];

  if(i_am_master) {
    if(setup.board < 9) level = setup.board;
    else                level = SysRandom(0)%9;
  } else
    level = 0;  /* will be transmitted by master */

  playfield = levels[level];

  draw_score();
}

// const int sincos[]={0x00,0x0c,0x18,0x24,0x30,0x3b,0x46,0x50,0x59,0x62,0x69,0x70,0x75,0x79,0x7c,0x7e};
const int sincos[]={0x00,0x06,0x0c,0x12,0x18,0x1d,0x23,0x27,0x2c,0x30,0x34,0x37,0x3a,0x3c,0x3d,0x3e};

void move_ball_slave(unsigned int ax, unsigned int ay) {
  RectangleType rect;

  if(draw_win!=WinGetDrawWindow())
    return;

  if((ball_x>>BALL_SHIFT) != 0xff) {
    rect.topLeft.x = ball_x>>BALL_SHIFT; rect.extent.x=BALL_DIAMETER;
    rect.topLeft.y = ball_y>>BALL_SHIFT; rect.extent.y=BALL_DIAMETER;
    WinEraseRectangle(&rect,0);
  }

  ball_x = ax; ball_y = ay;

  if((ball_x>>BALL_SHIFT) != 0xff)
    WinDrawBitmap(ObjectBitmapPtr[Ball], ax>>BALL_SHIFT, ay>>BALL_SHIFT);    
}

#define HTL 0x08
#define HTR 0x04
#define HBL 0x02
#define HBR 0x01

int check_4_brick(int new_x, int new_y) {
  int brick_x1, brick_y1, brick_x2, brick_y2, brick_x3, brick_y3, hit;

  brick_x1 = (new_x - BORDER_W - 1) / (BRICK_WIDTH+1);
  brick_x2 = ((new_x+BALL_DIAMETER-1) - BORDER_W - 1) / (BRICK_WIDTH+1);
  brick_y1 = (new_y - BRICK_START) / (BRICK_HEIGHT+1);
  brick_y2 = ((new_y+BALL_DIAMETER-1) - BRICK_START) / (BRICK_HEIGHT+1);

  /* out of brick area? */
  if((brick_y1<0)||(brick_y2<0)||(brick_y1>(BRICK_ROWS-1))||(brick_y2>(BRICK_ROWS-1))) return 0;
  if((brick_x1<0)||(brick_x2<0)||(brick_x1>(BRICK_COLUMNS-1))||(brick_x2>(BRICK_COLUMNS-1))) return 0;

  hit = (((playfield[brick_y1]>>(brick_x1<<1))&3)?HTL:0) |
        (((playfield[brick_y1]>>(brick_x2<<1))&3)?HTR:0) |
        (((playfield[brick_y2]>>(brick_x1<<1))&3)?HBL:0) |
        (((playfield[brick_y2]>>(brick_x2<<1))&3)?HBR:0);

  /* running top-left (0/1/2) */
  if((ball_dx==-1) && (ball_dy==-1)) {
    brick_x3 = (new_x + 2 - BORDER_W - 1) / (BRICK_WIDTH+1);
    brick_y3 = (new_y + 2 - BRICK_START) / (BRICK_HEIGHT+1);

    hit |= (((playfield[brick_y1]>>(brick_x3<<1))&3)?HTR:0);
    hit |= (((playfield[brick_y3]>>(brick_x1<<1))&3)?HBL:0);

    if((hit == HTL)||(hit == (HTL|HTR|HBL) )||(hit == (HTR|HBL))) { ball_dx = ball_dy = 1; return 1; }
    if((hit == HTR) || (hit == (HTL|HTR)))                        { ball_dy = 1; return 1; }
    if((hit == HBL) || (hit == (HTL|HBL)))                        { ball_dx = 1; return 1; }
  }

  /* running bottom-right (1/2/3) */
  if((ball_dx==1) && (ball_dy==1)) {
    brick_x3 = ((new_x - 2 + BALL_DIAMETER-1) - BORDER_W - 1) / (BRICK_WIDTH+1);
    brick_y3 = ((new_y - 2 + BALL_DIAMETER-1) - BRICK_START) / (BRICK_HEIGHT+1);

    hit |= (((playfield[brick_y2]>>(brick_x3<<1))&3)?HBL:0);
    hit |= (((playfield[brick_y3]>>(brick_x2<<1))&3)?HTR:0);

    if((hit == HBR)||(hit == (HBR|HTR|HBL) )||(hit == (HTR|HBL))) { ball_dx = ball_dy = -1; return 1; }
    if((hit == HTR) || (hit == (HBR|HTR)))                        { ball_dx = -1; return 1; }
    if((hit == HBL) || (hit == (HBR|HBL)))                        { ball_dy = -1; return 1; }
  }

  /* running top-right (0/1/3) */
  if((ball_dx==1) && (ball_dy==-1)) {
    brick_x3 = ((new_x - 2 + BALL_DIAMETER-1) - BORDER_W - 1) / (BRICK_WIDTH+1);
    brick_y3 = (new_y + 2 - BRICK_START) / (BRICK_HEIGHT+1);

    hit |= (((playfield[brick_y1]>>(brick_x3<<1))&3)?HTL:0);
    hit |= (((playfield[brick_y3]>>(brick_x2<<1))&3)?HBR:0);

    if((hit == HTR)||(hit == (HTR|HTL|HBR) )||(hit == (HTL|HBR))) { ball_dx = -1; ball_dy = 1; return 1; }
    if((hit == HTL) || (hit == (HTR|HTL)))                        { ball_dy = 1; return 1; }
    if((hit == HBR) || (hit == (HTR|HBR)))                        { ball_dx = -1; return 1; }
  }

  /* running bottom-left (0/2/3) */
  if((ball_dx==-1) && (ball_dy==1)) {
    brick_x3 = (new_x + 2 - BORDER_W - 1) / (BRICK_WIDTH+1);
    brick_y3 = ((new_y - 2 + BALL_DIAMETER-1) - BRICK_START) / (BRICK_HEIGHT+1);

    hit |= (((playfield[brick_y2]>>(brick_x3<<1))&3)?HBR:0);
    hit |= (((playfield[brick_y3]>>(brick_x1<<1))&3)?HTL:0);

    if((hit == HBL)||(hit == (HBL|HTL|HBR) )||(hit == (HTL|HBR))) { ball_dx = 1; ball_dy = -1; return 1; }
    if((hit == HTL) || (hit == (HBL|HTL)))                        { ball_dx = 1; return 1; }
    if((hit == HBR) || (hit == (HBL|HBR)))                        { ball_dy = -1; return 1; }
  }
  
  return 0;
}

void ball_lost(int *x, int *y) {
  do_sound(SndLost);
  
  state = SLEEPING;
  sleep_counter = 30;

  *x = 0xffff;
}

void game_run(void) {
  state = NEW_GAME;

  paddle_pos[YOU] = paddle_pos[ME] = PADDLE_CENTER;

  if(ball_dy>0) winner = ME;
  else          winner = YOU;
  score[winner]++;
  loc_score[winner]++;
  restart_game();
	  
  if(ball_dy>0) {
    ball_x = (paddle_pos[YOU] + (BALL_CENTER_X - PADDLE_CENTER))<<BALL_SHIFT;
    ball_y = (PADDLE_TOP_Y + PADDLE_HEIGHT)<<BALL_SHIFT;
  } else {          
    ball_x = (paddle_pos[ME]  + (BALL_CENTER_X - PADDLE_CENTER))<<BALL_SHIFT;
    ball_y = (PADDLE_BOT_Y - BALL_DIAMETER)<<BALL_SHIFT;
  }
  
  draw_game_area();
}

#define NOISE_COUNTER 200

void move_ball(void) {
  IrStatus ret;
  int nx,ny,lx,ly;
  RectangleType rect;
  int loop;
  static int noise=0;

  noise++;
  
#ifndef SINGLE_GAME
  if(panic>PANIC_STOP) return;
#endif

  if(draw_win!=WinGetDrawWindow())
    return;
  
  /* FAKE */
#ifdef SINGLE_GAME
  nx = (ball_x>>BALL_SHIFT)-12;
  if(nx<PADDLE_MIN) nx=PADDLE_MIN;
  if(nx>PADDLE_MAX) nx=PADDLE_MAX;
  move_paddle(YOU, nx); 
#endif

  if(state == RUNNING) {
    nx = ball_x;
    ny = ball_y;
    
    for(loop=0;loop<ball_step;loop++) {
      lx = nx; ly = ny;

      nx += ball_dx * (int)sincos[ball_dir];
      ny += ball_dy * sincos[0x0f-ball_dir];

      /* ball touches side? */
      if((nx<BALL_MIN_X)||(nx>BALL_MAX_X)) {
	do_sound(SndWall);

	nx = lx;
	ball_dx = -ball_dx;
      }
      
      /* ball touches brick? */
      if(check_4_brick(nx >> BALL_SHIFT, ny >> BALL_SHIFT)) {

	/* add some random noise */
	if(noise > NOISE_COUNTER) {
	  if(ball_dir>8) ball_dir--;
	  else           ball_dir++;

	  noise = 0;
	}

	do_sound(SndBrick);

	ny = ly; nx = lx;
      }

      /* ball touches lower (MinE) paddle? */
      if(((nx>>BALL_SHIFT)>(paddle_pos[ME]-BALL_DIAMETER))&&  /* in x-dir */
	 ((nx>>BALL_SHIFT)<(paddle_pos[ME]+PADDLE_WIDTH))) {

	noise = 0;

	/* hits paddle from the top */
	if((ny>>BALL_SHIFT)==(PADDLE_BOT_Y-BALL_DIAMETER+1)) {

	  do_sound(SndPaddle);
	  ny = (PADDLE_BOT_Y-BALL_DIAMETER)<<BALL_SHIFT;
	  
	  if(master_buttons != 0) {
	    if(((master_buttons == MOVE_RIGHT)&&(ball_dx<0))||
	       ((master_buttons == MOVE_LEFT) &&(ball_dx>0))) {
	      ball_dir -= PADDLE_SHIFT;
	      if(ball_dir<=0) {
		ball_dir = -ball_dir;
		ball_dx  = -ball_dx;
	      }
	    } else {
	      ball_dir +=PADDLE_SHIFT;
	      if(ball_dir>0x0e) ball_dir=0x0e;
	    }
	  }  
	  ball_dy = -ball_dy;
	} else if ((ny>>BALL_SHIFT)>(PADDLE_BOT_Y-BALL_DIAMETER+1)){

	  /* hits paddle from side */

	  /* left side? */
	  if((nx>>BALL_SHIFT)<(paddle_pos[ME]+PADDLE_WIDTH/2)) {
	    nx = (paddle_pos[ME]-BALL_DIAMETER)<<BALL_SHIFT;
	    ball_dx = -1;
	  }

	  /* right side? */
	  if((nx>>BALL_SHIFT)>(paddle_pos[ME]+PADDLE_WIDTH/2)) {
	    nx = (paddle_pos[ME]+PADDLE_WIDTH)<<BALL_SHIFT;
	    ball_dx = 1;
	  }

	  /* ball between paddle and border? */
	  if((nx<BALL_MIN_X)||(nx>BALL_MAX_X)) {
	    loop = ball_step;
	    ball_lost(&nx, &ny);
	  }
	}
      }

      /* ball touches upper (YOUr) paddle? */
      if(((nx>>BALL_SHIFT)>(paddle_pos[YOU]-BALL_DIAMETER))&&
	 ((nx>>BALL_SHIFT)<(paddle_pos[YOU]+PADDLE_WIDTH))) {

	noise = 0;
	  
	if((ny>>BALL_SHIFT)==(PADDLE_TOP_Y+PADDLE_HEIGHT-1)) {
	  do_sound(SndPaddle);
	  ny = (PADDLE_TOP_Y+PADDLE_HEIGHT)<<BALL_SHIFT;
	  
	  if(slave_buttons != 0) {
	    if(((slave_buttons == MOVE_LEFT) &&(ball_dx<0))||
	       ((slave_buttons == MOVE_RIGHT)&&(ball_dx>0))) {
	      ball_dir -= PADDLE_SHIFT;
	      if(ball_dir<=0) {
		ball_dir = -ball_dir;
		ball_dx  = -ball_dx;
	      }
	    } else {
	      ball_dir += PADDLE_SHIFT;
	      if(ball_dir>0x0e) ball_dir=0x0e;
	    }
	  }
	  
	  ball_dy = -ball_dy;
	} else if ((ny>>BALL_SHIFT)<(PADDLE_TOP_Y+PADDLE_HEIGHT-1)){

	  /* hits paddle from side */

	  /* left side? */
	  if((nx>>BALL_SHIFT)<(paddle_pos[YOU]+PADDLE_WIDTH/2)) {
	    nx = (paddle_pos[YOU]-BALL_DIAMETER)<<BALL_SHIFT;
	    ball_dx = -1;
	  }

	  /* right side? */
	  if((nx>>BALL_SHIFT)>(paddle_pos[YOU]+PADDLE_WIDTH/2)) {
	    nx = (paddle_pos[YOU]+PADDLE_WIDTH)<<BALL_SHIFT;
	    ball_dx = 1;
	  }

	  /* ball between paddle and border? */
	  if((nx<BALL_MIN_X)||(nx>BALL_MAX_X)) {
	    loop = ball_step;
	    ball_lost(&nx, &ny);
	  }
	}
      }

      /* ball left game area */
      if(((ny>>BALL_SHIFT) > (GAME_AREA_Y + GAME_AREA_H - BALL_DIAMETER)) || ((ny>>BALL_SHIFT) < (GAME_AREA_Y))) {
	loop = ball_step;
	ball_lost(&nx, &ny);
      }
    
    }
  } else if(state == WAITING) {
    if(ball_dy>0) nx = (paddle_pos[YOU] + (BALL_CENTER_X - PADDLE_CENTER))<<BALL_SHIFT;
    else          nx = (paddle_pos[ME]  + (BALL_CENTER_X - PADDLE_CENTER))<<BALL_SHIFT;
    ny = ball_y;
  } else {
    nx = ball_x;
    ny = ball_y;
  }

  rect.topLeft.x = ball_x>>BALL_SHIFT; rect.extent.x = BALL_DIAMETER;
  rect.topLeft.y = ball_y>>BALL_SHIFT; rect.extent.y = BALL_DIAMETER;
  WinEraseRectangle(&rect,0);

  ball_x = nx; 
  ball_y = ny;
  WinDrawBitmap(ObjectBitmapPtr[Ball], ball_x>>BALL_SHIFT, ball_y>>BALL_SHIFT);
}

/* start a game of IR pong */
void start_game(int mode) {
  FormPtr frm;

  i_am_master=mode;

  /* hide connect button */
  CtlHideControl(FrmGetObjectPtr (mainform, FrmGetObjectIndex (mainform, PongConnect)));

#ifdef USE_IR_INDICATION
  ir_indication(1);
#endif

  restart_game();

  paddle_pos[ME] = paddle_pos[YOU] = PADDLE_CENTER;

  if(ball_dy>0) { 
    /* ball sticks to opposite paddle */
    ball_x = (paddle_pos[YOU] + (BALL_CENTER_X - PADDLE_CENTER))<<BALL_SHIFT;
    ball_y = (PADDLE_TOP_Y + PADDLE_HEIGHT)<<BALL_SHIFT;
  } else {
    /* ball sticks to own paddle */
    ball_x = (paddle_pos[ME] + (BALL_CENTER_X - PADDLE_CENTER))<<BALL_SHIFT;
    ball_y = (PADDLE_BOT_Y - BALL_DIAMETER)<<BALL_SHIFT;
  }

  draw_game_area();

  if(i_am_master) {
    /* wait for key press */
    state = NEW_GAME;
  } else {
    /* just be slave */
    state = SLAVE_RUNNING;
  }
}

int redraw_main=false;

/* stop a game of IR pong */
void redraw_main_win(void) {
  FormPtr frm;
  RectangleType rect;

  if(draw_win == WinGetDrawWindow()) {
    /* clear */
    rect.topLeft.x=0;  rect.topLeft.y=GAME_AREA_Y; 
    rect.extent.x=SCREEN_WIDTH; rect.extent.y=GAME_AREA_H;
    WinEraseRectangle(&rect,0);

    main_draw_score();
  }

#ifndef FREEWARE
  if(!slave_only)
#endif
    CtlShowControl(FrmGetObjectPtr (mainform, FrmGetObjectIndex (mainform, PongConnect)));

#ifdef USE_IR_INDICATION
  ir_indication(0);
#endif
}

void stop_game(void) {
  state = SETUP;
  winner = 2;

  if(draw_win != WinGetDrawWindow()) 
    redraw_main = true;
  else
    redraw_main_win();
}

#ifndef SINGLE_GAME
static void IrIasCallback(IrStatus status) {
  IrStatus ret;
  CALLBACK_PROLOGUE;

  if(((ret=IrIAS_GetType(&query)) == IAS_ATTRIB_INTEGER)&&(query.retCode==IAS_RET_SUCCESS)) {
    con.rLsap = IrIAS_GetIntLsap(&query);
    
    /* build my first IrPacket and buffer */
    packet.buff = ir_data;
    packet.len = 0;
    
    /* open TinyTP session */ 
    if(IrConnectReq(ir_ref, &con, &packet, 0x7f)!=IR_STATUS_PENDING)
      FrmAlert(alt_irconnectl);
  } else {
    if(query.retCode!=IAS_RET_SUCCESS) {
      FrmCustomAlert( alt_iriaserr, ir_ias_return_str[(query.retCode>2)?2:query.retCode], 0,0);

      /* disconnect if needed */
      if(IrIsIrLapConnected(ir_ref))
	if((ret=IrDisconnectIrLap(ir_ref))!=IR_STATUS_PENDING)
	  FrmCustomAlert(alt_irdiscon, ir_status_str[ret],0,0);
    } else
      FrmCustomAlert( alt_iriaserr, ir_ias_type_str[(ret>3)?3:ret],0,0);
  }

  CALLBACK_EPILOGUE;
}


static void IrCallback(IrConnect *connect, IrCallBackParms *parms) {
  IrStatus ret;
  int i,j;
  FormPtr frm;  

  CALLBACK_PROLOGUE;

  switch(parms->event) {
  case LEVENT_STATUS_IND:
    break;

 case LEVENT_DISCOVERY_CNF:
   /* make connect button usable */
   frm = FrmGetActiveForm ();
   CtlSetEnabled(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, PongConnect)), 1);   

   if(parms->deviceList->nItems == 0) {
     if(FrmAlert(alt_irnoother)==1)  /* retry */
       start_discovery();
   } else {
     for(j=-1,i=0;i<parms->deviceList->nItems;i++)
       if(StrNCompare(parms->deviceList->dev[i].xid, dev_name, 7)==0)
	 j=i;

     if(j>=0) {
       if(IrConnectIrLap(ir_ref, parms->deviceList->dev[j].hDevice)!=IR_STATUS_PENDING)
	 FrmAlert(alt_irconnect);
     } else
       if(FrmAlert(alt_irnoother)==1)  /* retry */
	 start_discovery();
   }
   break;

  case LEVENT_LM_CON_IND:

    /* accept connection */
    packet.buff = ir_data;
    packet.len = 0;

    max_tx = IrMaxTxSize(ir_ref, &con);

    if((ret=IrConnectRsp(ir_ref, &con, &packet, 0x7f))!=IR_STATUS_PENDING)
      FrmCustomAlert(alt_irrespond, ir_status_str[ret],0,0);
    else
      start_game(0); /* start game as slave */
    break;

  case LEVENT_LAP_DISCON_IND:
  case LEVENT_LM_DISCON_IND:
    stop_game();
    break;

  case LEVENT_LAP_CON_CNF:
    // get rLsap
    query.queryBuf = rlsap_query;
    query.queryLen = StrLen(query.queryBuf);
    query.resultBufSize = RESULT_SIZE;
    query.result = result;
    query.callBack = IrIasCallback;

    if((ret=IrIAS_Query(ir_ref, &query))!=IR_STATUS_PENDING)
      FrmCustomAlert(alt_irquery, ir_status_str[ret], 0,0);      
    break;
    
  case LEVENT_DATA_IND:
    panic = 0;  /* reset panic counter */

    if(state != SETUP) {
      if(i_am_master) {
	MemMove(&slave, parms->rxBuff, sizeof(slave_state)); 

	/* normal slave packet */
	if(slave.buttons!=0xff) {
	  /* get paddle_pos etc. from slave */
	  move_paddle(YOU, PADDLE_MIRROR(slave.paddle_pos)); 
	  slave_buttons = slave.buttons;
	} else if(slave.buttons == 0xff) {

	  /* verify comm version */
	  if(slave.paddle_pos != COMM_VERSION) {
	    /* slave ack with comm version */
	    IrDisconnectIrLap(ir_ref);
	    state = WRONG_VERSION;

	    wrong_version = slave.paddle_pos;
	  } else
	    state = WAITING;
	}
      } else {
	MemMove(&master, parms->rxBuff, sizeof(master_state)); 
	if((master.type>>4) != 0xf) do_sound(master.type>>4);

	if((master.type&0x0f) == MSTR_BALL) {
	  state = SLAVE_RUNNING;

	  /* ball/paddle coordinates */
	  move_paddle(YOU, PADDLE_MIRROR(master.paddle_pos)); 	
	  move_ball_slave( ((unsigned int)master.ball_x)<<BALL_SHIFT, 
			   ((unsigned int)master.ball_y)<<BALL_SHIFT);
	} else if((master.type&0x0f) == MSTR_END) {
	  if(state != SLAVE_ACK) {
	    /* win/loose information */
	    if(master.paddle_pos != 2) {
	      score[(master.paddle_pos)?ME:YOU]++;
	      loc_score[(master.paddle_pos)?ME:YOU]++;
	      draw_score();
	    }

	    slave_level = master.ball_x;

	    paddle_pos[ME] = PADDLE_CENTER;
	  }
	  state = SLAVE_ACK;
	} 
      }
    }
    break;

  case LEVENT_LM_CON_CNF:
    start_game(1);  /* start game as master */

    max_tx = IrMaxTxSize(ir_ref, &con);
    /* continue with PACKET_HANDLED */

  case LEVENT_PACKET_HANDLED:
    packet_handled=true;

    if(!i_am_master) {
      if(max_tx>=sizeof(slave_state)) {
	if(state == SLAVE_ACK) {
	  slave.buttons = 0xff;
	  slave.paddle_pos = COMM_VERSION;
	} else {
	  slave.buttons = slave_buttons;
	  slave.paddle_pos = paddle_pos[ME];  /* send own paddle position */
	}
    
	packet.buff = (char*)&slave;
	packet.len = sizeof(slave_state);
	      
	/* send it! */ 
	if((ret=IrDataReq(ir_ref, &con, &packet))!=IR_STATUS_PENDING) ;
//	  FrmCustomAlert(alt_irsend, ir_status_str[ret],0,0);    
      }
    } else {
      if(max_tx>=sizeof(master_state)) {
	
	/* normal playing state */
	if((state == RUNNING)||(state == WAITING)||(state == SLEEPING)) {
	  master.type = MSTR_BALL;
	  master.paddle_pos = paddle_pos[ME];
	  master.ball_x = BALL_MIRROR_X(ball_x >> BALL_SHIFT);
	  master.ball_y = BALL_MIRROR_Y(ball_y >> BALL_SHIFT);

	  if(state == SLEEPING) master.ball_x = 0xff;  /* slave shall not draw the ball */
	  
	  /* end of one game */
	} else if(state == NEW_GAME) {
	  master.type = MSTR_END;
	  master.paddle_pos = winner;
	  master.ball_x = level;
	}

	/* send sound to slave */
	master.type |= (last_sound<<4);
	last_sound = 0xff;
	
	packet.buff = (char*)&master;
	packet.len = sizeof(master_state);
	
	/* send it! */ 
	if((ret=IrDataReq(ir_ref, &con, &packet))!=IR_STATUS_PENDING) ;
//	  FrmCustomAlert(alt_irsend, ir_status_str[ret],0,0);    
      }
    }

    break;

  default:
    /* panic here */
    break;    
  }
  CALLBACK_EPILOGUE;
}
#endif

static Boolean StartApplication(void) {
  int i;
  WinHandle oldDrawWinH;
  IrStatus ret;
  FormPtr frm;  
  char str[32];
  SystemPreferencesType sysPrefs;

  PongPrefs prefs;
  Word error, size;
  
  FontPtr font128;
  int wPalmOSVerMajor;
  long dwVer;

  winner = 2;  /* no winner */
  level = slave_level = -1; 

  /* load new font */
  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &dwVer);
  wPalmOSVerMajor = sysGetROMVerMajor(dwVer);
  if (wPalmOSVerMajor < 3) {
    FrmAlert(alt_wrongOs);
    return false;
  }

  font128=MemHandleLock(DmGetResource('NFNT', 1000));
  FntDefineFont(128, font128);

  /* load preferences */
  size = sizeof(PongPrefs);
  error=PrefGetAppPreferences( CREATOR, 0, &prefs, &size, true);

  /* get lib from prefs */
  if((error!=noPreferenceFound)&&(prefs.version==PREFS_VERSION)) {
    /* valid prefs */
    score[ME]  = prefs.score[ME];
    score[YOU] = prefs.score[YOU];
#ifndef FREEWARE
    firstStart = prefs.firstStart;
    reg_id = prefs.reg_id;
#endif
    setup = prefs.setup;

#ifndef FREEWARE
    check_registration(reg_id);
#endif
  } else {
    /* no prefs */
    score[ME] = score[YOU] = 0;
    loc_score[ME] = loc_score[YOU] = 0;
#ifndef FREEWARE
    regd = false;
    reg_id = 0;
#endif

    /* setup setup */
    setup.ball_speed = 1;
    setup.ball_inc = 1;
    setup.paddle_speed = 1;
    setup.sound = 1;
    setup.board = 1;
#ifndef FREEWARE
    /* this is the first game */
    firstStart = TimGetSeconds();
#endif
  }

  /* get system preferences for sound */
  PrefGetPreferences(&sysPrefs);
  if(sysPrefs.gameSoundLevelV20 == slOff) setup.sound = 0;

#ifndef FREEWARE
  slave_only = false; 

  if(!regd) {

    /* expired */
    if(TimGetSeconds()>(firstStart+REG_SECS)) {
      do {
	if(FrmCustomAlert(alt_exp, StrIToA(str, REG_DAYS), 0,0) == 0) register_me();	
	else slave_only = true;  /* continue slave only */
      } while((!regd)&&(!slave_only));
    } else {
      /* trial */
      if(FrmCustomAlert(alt_trial, StrIToA(str, REG_DAYS-(TimGetSeconds()-firstStart)/REG_SECDAY), 0,0) == 0) 
	register_me();	
    }
  }
#endif
  
#ifndef SINGLE_GAME
  /* open IR library */
  if(SysLibFind(irLibName, &ir_ref)!=0) {
    FrmAlert(alt_libfind);
    return false;
  } else {
    /* ok, so far, so good, now open the library */
    if(IrOpen(ir_ref, 0)!=0) {
      FrmAlert(alt_iropen);
      return false;
    } else {
      /* hey things are working ... */
      if(IrBind(ir_ref, &con, IrCallback)!=IR_STATUS_SUCCESS) {
	FrmAlert(alt_irbind);
	return false;
      } else {
	if(IrSetDeviceInfo(ir_ref, dev_name, 7)!=IR_STATUS_SUCCESS) {
	  FrmAlert(alt_irsetdev);
	  return false;
	} else {
	  IrSetConTypeLMP(&con);  /* set connection type to IrLMP */

	  ias_object.name = ias_name;
	  ias_object.len  = StrLen(ias_name);
	  ias_object.nAttribs = 1;
	  ias_object.attribs = &attribs;
	  
	  attribs.name=attr_name;            /* Pointer to name of attribute */
	  attribs.len=StrLen(attr_name);     /* Length of attribute name */
	  attribs.value=attr_value;          /* Hardcode value */
	  attribs.valLen=5;                  /* Length of the value. */
	  attr_value[4]=con.lLsap;
	  
	  // set up our service in IAS db
	  if(IrIAS_Add(ir_ref, &ias_object)!=IR_STATUS_SUCCESS) {
	    FrmAlert(alt_iriasadd);
	    return false;
	  }
	}
      }
    }
  }
#endif

  // Keep the Object graphics locked because they are frequently used
  oldDrawWinH = WinGetDrawWindow();
  for (i = 0; i < bitmapTypeCount; i++) {
    ObjectBitmapHandles[i] = (Handle)DmGetResource( bitmapRsc, firstObjectBmp + i);
    ObjectBitmapPtr[i] = MemHandleLock((VoidHand)ObjectBitmapHandles[i]);
		
    ObjectWindowHandles[i] = 
      WinCreateOffscreenWindow(ObjectBitmapPtr[i]->width, 
			       ObjectBitmapPtr[i]->height,
				screenFormat, &error);
    ErrFatalDisplayIf(error, "Error loading images");
    WinSetDrawWindow(ObjectWindowHandles[i]);
    WinDrawBitmap(ObjectBitmapPtr[i], 0, 0);  
  }
  WinSetDrawWindow(oldDrawWinH);

  /* Mask hardware keys */
  KeySetMask(~(keyBitHard1 | keyBitHard2 | keyBitHard3 | keyBitHard4));

  SysRandom(TimGetTicks());

  FrmGotoForm(frm_Main);

  return true;
}

static void StopApplication(void) {
  int i;
  PongPrefs prefs;

  /* unmask keys */
  KeySetMask(	keyBitsAll );

  // Unlock and release the locked bitmaps
  for (i = 0; i < bitmapTypeCount; i++) {
    MemPtrUnlock(ObjectBitmapPtr[i]);
    DmReleaseResource((VoidHand)ObjectBitmapHandles[i]);

    if (ObjectWindowHandles[i]) 
      WinDeleteWindow(ObjectWindowHandles[i], false);
  }

#ifndef SINGLE_GAME
  if (ir_ref!=0) {
    /* close connection (don't warn if that fails, we finish anyway) */
    if(IrIsIrLapConnected(ir_ref))
      IrDisconnectIrLap(ir_ref);

    /* close ir library */
    IrClose(ir_ref);
  }
#endif

  /* save preferences */
  prefs.version=PREFS_VERSION;
  prefs.score[ME]  = score[ME];
  prefs.score[YOU] = score[YOU]; 
  prefs.setup = setup;
#ifdef FREEWARE
  prefs.reg_id = 0;
  prefs.firstStart = 0;
#else
  prefs.reg_id = reg_id;
  prefs.firstStart = firstStart;
#endif
  PrefSetAppPreferences( CREATOR, 0, 1, &prefs, sizeof(PongPrefs), true);  
}

#ifndef SINGLE_GAME
int start_discovery(void) {
  FormPtr frm;

  if(IrIsMediaBusy(ir_ref))
    FrmAlert(alt_irbusy);
  else {
    /* search for someone */
    if(IrDiscoverReq(ir_ref, &con)!=IR_STATUS_PENDING)
      FrmAlert(alt_irdiscover);
    else {
      /* make connect button unusable */
      frm = FrmGetActiveForm ();
      CtlSetEnabled(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, PongConnect)),0);
    }
  }
}
#endif

static Boolean MainFormHandleEvent(EventPtr event)
{
  FormPtr frm;
  IrStatus ret;
  char str[32], *id_str;
  char name[dlkUserNameBufSize];
  int id,i;
  ListPtr lst;
  
  Boolean handled = false;
  
  switch (event->eType) {
  case ctlSelectEvent:
    if(event->data.ctlEnter.controlID == PongConnect) {
#ifndef SINGLE_GAME
      start_discovery();
#else
      start_game(1);
#endif
      handled=true;
    }

#ifdef USE_IR_INDICATION
    /* TODO: stop game */
    if(event->data.ctlEnter.controlID == PongIndicator) {
      IrDisconnectIrLap(ir_ref);
      state = SETUP;
      handled=true;
    }
    break;
#endif
    
  case frmOpenEvent:
    FrmDrawForm(FrmGetActiveForm());
    draw_win = WinGetDrawWindow();

    main_draw_score();
    draw_score();

#ifdef USE_IR_INDICATION
    ir_indication(0);
#endif

    handled = true;
    break;

  case menuEvent:
    switch (event->data.menu.itemID) {

    case menuHelp:
      FrmHelp(MainHelp);
      handled = true;
      break;

    case menuDiscon:
      if((ret=IrDisconnectIrLap(ir_ref))!=IR_STATUS_PENDING)
	FrmCustomAlert(alt_irdiscon, ir_status_str[ret],0,0);

      state = SETUP;
      handled = true;
      break;

    case menuAbout:
      frm=FrmInitForm(frm_About);
      FrmDoDialog(frm);
      FrmDeleteForm(frm);
      handled = true;
      break;

    case menuSetup:
      frm=FrmInitForm(frm_Setup);

      /* set sound selector */
      CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SetupSound)), setup.sound);      

      /* set board indicator */
      lst = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SetupBrdLst));
      LstSetSelection(lst, setup.board);
      CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SetupBrdLstSel)), 
		  LstGetSelectionText(lst, setup.board));

      for(i=0;i<3;i++) {
	/* set paddle speed / ball speed / ball increment indicator */
	CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SetupPSpdS+i)), setup.paddle_speed==i);
	CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SetupBSpdS+i)), setup.ball_speed==i);
	CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SetupBIncS+i)), setup.ball_inc==i);
      }

      if(FrmDoDialog(frm)==SetupOK) {
	/* read setups */
	setup.sound = CtlGetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SetupSound)));      
	setup.board = LstGetSelection(lst);
	
	for(i=0;i<3;i++) {
	  if(CtlGetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SetupPSpdS+i)))) setup.paddle_speed=i;
	  if(CtlGetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SetupBSpdS+i)))) setup.ball_speed=i;
	  if(CtlGetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SetupBIncS+i)))) setup.ball_inc=i;
	}
      }

      FrmDeleteForm(frm);
      handled = true;
      break;

#ifndef FREEWARE
    case menuRegister:
      if(!regd) {
	register_me();
	if((slave_only)&&(regd)) {
	  if(state == SETUP) {
	    CtlShowControl(FrmGetObjectPtr (mainform, FrmGetObjectIndex (mainform, PongConnect)));
	  }
	  slave_only = false;
	}
      } else 
	FrmAlert(alt_already);
      handled = true;
      break;
#endif

    }
    break;
  }
  
  return(handled);
}

/////////////////////////
// ApplicationHandleEvent
/////////////////////////

static Boolean ApplicationHandleEvent(EventPtr event)
{
  Word    formId;
  Boolean handled = false;
  
  if (event->eType == frmLoadEvent) {
    formId = event->data.frmLoad.formID;
    mainform = FrmInitForm(formId);
    FrmSetActiveForm(mainform);

#ifndef FREEWARE
    /* unregistered user can only play with registered one */
    if(slave_only) CtlHideControl(FrmGetObjectPtr (mainform, FrmGetObjectIndex (mainform, PongConnect)));
#endif
    
    switch (formId) {
    case frm_Main:
      FrmSetEventHandler(mainform, MainFormHandleEvent);
      break;
    }
    handled = true;
  }

  return handled;
}

/////////////////////////
// EventLoop
/////////////////////////

static void EventLoop(void)
{
  EventType event;
  Word      error, buttons;
  DWord hardKeyState;
  IrStatus ret;
  char str1[8], str2[8];
  
  Word  x,y, speed_cnt=0;
  Long ticks_wait, i,l;
  
  /* initialize Timer */
  ticks_wait = SysTicksPerSecond()/GAME_HZ;  /* 40ms/frame=25Hz */    

  l = TimGetTicks()+ticks_wait;
  
  do {
    i=l-TimGetTicks();
    EvtGetEvent(&event, (i<0)?0:i);
    if (!SysHandleEvent(&event))
      if (!MenuHandleEvent(CurrentMenu, &event, &error))
	if (!ApplicationHandleEvent(&event))
	  FrmDispatchEvent(&event);
    
    /* do some kind of 25Hz timer event */
    if(TimGetTicks()>l) {

      l = TimGetTicks()+ticks_wait;

      hardKeyState = KeyCurrentState();

      if(state!=SETUP) {
	buttons = 0;

	/* only move while playing */
	if((state == RUNNING)||(state == WAITING)||(state == SLAVE_RUNNING)) {

	  /* move left button? */
	  if( ((hardKeyState & (keyBitHard1 | keyBitHard2)) != 0)&&
	      ((hardKeyState & (keyBitHard3 | keyBitHard4)) == 0)) { 
	    move_paddle(ME, -paddle_step); 
	    buttons = MOVE_LEFT;  
	  }

	  /* move right button? */
	  if( ((hardKeyState & (keyBitHard3 | keyBitHard4)) != 0)&&
	      ((hardKeyState & (keyBitHard1 | keyBitHard2)) == 0)) { 
	    move_paddle(ME, +paddle_step); 
	    buttons = MOVE_RIGHT; 
	  }

	  /* the start button? */
	  if(hardKeyState & (keyBitPageUp | keyBitPageDown))
	    buttons |= MOVE_START;
	}

#ifndef SINGLE_GAME
	/* verify that connection is alive */
	if((state != SETUP)&&(state != WRONG_VERSION)) {
	  panic++;

	  if(panic==PANIC_TIMEOUT) {
	    FrmAlert(alt_irlost);

	    IrDisconnectIrLap(ir_ref);

	    state = SETUP;
	  }
	}
#endif

	/* redraw main if needed */
	if(redraw_main) {
	  redraw_main_win();
	  redraw_main = false;
	}

	if(i_am_master) {
	  master_buttons = buttons;

	  if(state == WRONG_VERSION) {
	    FrmCustomAlert(alt_version, StrIToA(str1, COMM_VERSION), StrIToA(str2, wrong_version), 0);
	    stop_game();
	    state = SETUP;
	  } else if(state == SLEEPING) {
	    sleep_counter--;
	    if(sleep_counter == 0) {
	      game_run();
	    }
	  } else if(state == RUNNING) {
	    /* 10 sek */
	    if(speed_cnt++==ball_inc) {
	      ball_step++;  /* increase ball speed */
	      speed_cnt = 0;
	    }
	    
	    /* master moves ball */
	    move_ball();
	  } else if(state == WAITING) {
	    move_ball();

	    /* it's masters turn ... */
	    if((ball_dy<0)&&(buttons & MOVE_START))
	      state = RUNNING;  /* ok, start game */

	    /* this is for slaves turn ... */
	    if((ball_dy>0)&&(slave_buttons & MOVE_START))
	      state = RUNNING;
	  }
	} else {

	  slave_buttons = buttons;

	  if(slave_level != -1) {
	    if(draw_win == WinGetDrawWindow()) {
	      level = slave_level;
	      slave_level = -1;
	      
	      /* initialize playfield */
	      playfield = levels[level];
	      draw_game_area();

	    }
	  }
	}

#ifdef SINGLE_GAME
	if(state == NEW_GAME) {
	  state = WAITING;
	}
#endif
      }
    }
  }
  while (event.eType != appStopEvent);
}

/////////////////////////
// PilotMain
/////////////////////////

DWord PilotMain(Word cmd, Ptr cmdBPB, Word launchFlags) {
  if (cmd == sysAppLaunchCmdNormalLaunch) {

    if(!StartApplication()) return(0);
    EventLoop();
    StopApplication();
  }
  return(0);
}











