/*

  IR Pong - version 2.0 aka LazerBall

  (c) 2000 by Till Harbaum - <t.harbaum@web.de>

 */

#include <PalmOS.h>

#include "Rsc.h"
#include "IR-Pong2.h"

// List of bitmaps
typedef enum { Paddle0=0, Paddle1, 
	       Ball, Border, Border_R, PBar0, PBar1, PBar2, 
	       ShotUp, ShotDown,
	       Brick0, Brick1, Brick2, Brick3, Brick4, Brick5, Brick6, 
	       bitmapTypeCount} bitmap_id;

static MemHandle  ObjectBitmapHandles[bitmapTypeCount];
static BitmapPtr  ObjectBitmapPtr[bitmapTypeCount];

Boolean bt_present = false;

/* buffer to store state of playing area */
unsigned char playfield[13][9];         /* normal area + 2 rows opponent area */

master_state master;
slave_state slave;

int sleep_counter;
int wrong_version;

WinHandle draw_win;

int winner, level, single_level, slave_level;
int single_balls;

/////////////////////////
// Function Prototypes
/////////////////////////

static Boolean StartApplication(void);
static Boolean MainFormHandleEvent(EventPtr event);
static Boolean ApplicationHandleEvent(EventPtr event);
static void    EventLoop(void);

game_state state = SETUP;
Boolean i_am_master;

Boolean single_game = false;

static MenuBarPtr CurrentMenu = NULL;
FormPtr mainform;

UInt16  panic=0;

Boolean quit=false;

unsigned char master_buttons, slave_buttons;

const int paddle_step_table[]={3,4,6};
int paddle_step;

const int ball_step_table[]={4,6,10};
int ball_step;

const int ball_inc_table[]={3600,1200,400};
int ball_inc;

/* do some sound */
typedef enum { SndPaddle=0, SndBrick, SndWall, SndLost } sound;

struct SndCommandType snd_data[] = {
  {sndCmdFreqDurationAmp,0,400,1,sndMaxAmp},
  {sndCmdFreqDurationAmp,0,200,1,sndMaxAmp},
  {sndCmdFreqDurationAmp,0,100,1,sndMaxAmp},
  {sndCmdFreqDurationAmp,0,100,10,sndMaxAmp}
};

unsigned char last_sound=0xff;

void do_sound(sound snd) {
  if(setup.sound)
    SndDoCmd(NULL, &snd_data[snd], 0); 

  last_sound = snd;
}

/* store info on object positions and directions */
int paddle_pos[2], shot[2][2];
unsigned int ball_p[MAX_BALLS][2];
int ball_d[MAX_BALLS][2], ball_dir[MAX_BALLS];
int ball_state[MAX_BALLS];  /* BALL_NONE, BALL_MOVE, BALL_STICK */
unsigned char ball_s[MAX_BALLS][2];

/* score counters */
unsigned long score[2], loc_score[2];
unsigned long single_score, single_current_score;

/* shot charger */
int charge;

/* show connect buttons */
void btn_set(Boolean enable) {
  FormPtr frm;
  
  /* make connect button unusable */
  frm = FrmGetActiveForm();
  CtlSetEnabled(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, PongIRConnect)), enable);
  CtlSetEnabled(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, PongBTConnect)), enable);
  CtlSetEnabled(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, PongPractice)),  enable);
}

/* draw score in single player mode */
void single_draw_score(int score) {
  char str[10];

  single_current_score += score;

  FntSetFont(129);
  StrPrintF(str, "%lu", single_current_score);
  WinDrawChars(str, StrLen(str), 80-FntCharsWidth(str, StrLen(str))/2, 1);
}

void draw_game_area(void) {
  Int16 x,y,b;
  RectangleType rect;
  int i;
  char str[4];

  charge = 0;

  /* clear */
  rect.topLeft.x=0;  rect.topLeft.y=GAME_AREA_Y; 
  rect.extent.x=SCREEN_WIDTH; rect.extent.y=GAME_AREA_H;
  WinEraseRectangle(&rect,0);

  /* draw border */
  for(y=0;y<9;y++) {
    WinDrawBitmap(ObjectBitmapPtr[Border], 0, GAME_AREA_Y+y*16);
    WinDrawBitmap(ObjectBitmapPtr[Border], SCREEN_WIDTH-BORDER_W, GAME_AREA_Y+y*16);
  }

  if(single_game) {
    rect.topLeft.x = 0;            rect.topLeft.y=GAME_AREA_Y; 
    rect.extent.x  = SCREEN_WIDTH; rect.extent.y=BORDER_W;

    WinSetClip(&rect);

    for(x=0;x<10;x++)
      WinDrawBitmap(ObjectBitmapPtr[Border_R], 16*x, GAME_AREA_Y);

    WinResetClip();

    /* clear score area */
    rect.topLeft.x = 60;   rect.topLeft.y = 1; 
    rect.extent.x  = 40;   rect.extent.y  = 11;
    WinEraseRectangle(&rect,0);

    /* change single player bricks to normal bricks */
    for(y=0;y<BRICK_ROWS;y++)
      for(x=0;x<BRICK_COLUMNS;x++)
	if(playfield[y][x]&0x40)
	  playfield[y][x] &= ~0x40;

    /* draw number of balls left */
    FntSetFont(1);
    WinDrawBitmap(ObjectBitmapPtr[Ball], 106, 2); 
    str[0]='0'+single_balls; str[1]=0;
    WinDrawChars(str, 1, 116,1);

    /* one ball is in use */
    single_balls--;

    single_draw_score(0);
  } else {
    /* and erase all single player bricks */
    for(y=0;y<BRICK_ROWS;y++)
      for(x=0;x<BRICK_COLUMNS;x++)
	if(playfield[y][x]&0x40)
	  playfield[y][x]=0;

    draw_score();
  }

  if(i_am_master) {
    if(!single_game)
      WinDrawBitmap(ObjectBitmapPtr[Paddle1], paddle_pos[YOU], PADDLE_TOP_Y);

    WinDrawBitmap(ObjectBitmapPtr[Paddle0], paddle_pos[ME], PADDLE_BOT_Y);
  } else {
    if(!single_game)
      WinDrawBitmap(ObjectBitmapPtr[Paddle0], paddle_pos[YOU], PADDLE_TOP_Y);

    WinDrawBitmap(ObjectBitmapPtr[Paddle1], paddle_pos[ME], PADDLE_BOT_Y);
  }

  if(i_am_master)
    WinDrawBitmap(ObjectBitmapPtr[Ball], 
		  ball_p[0][AX]>>BALL_SHIFT, 
		  ball_p[0][AY]>>BALL_SHIFT);

  /* draw bricks */
  for(y=0;y<BRICK_ROWS;y++) {
    for(x=0;x<BRICK_COLUMNS;x++) {
      if(playfield[y][x]) {
	WinDrawBitmap(ObjectBitmapPtr[Brick0 + (playfield[y][x]&(~0x80)) - 1],
		      BORDER_W + 1 + x * (BRICK_WIDTH+1), 
		      BRICK_START + y*(BRICK_HEIGHT+1));
      }
    }
  }
}

/* draw progress bar (recharge) 0..32 */
void draw_pbar(unsigned int step) {
  static unsigned int old_val = 40;
  RectangleType rect;

  if(step != old_val) {
    if(step>32) step = 32;

    /* redraw gray bar background */
    if(old_val > step)
      WinDrawBitmap(ObjectBitmapPtr[PBar0], 126, 2);

    /* draw color bar */
    rect.topLeft.x = 126;  rect.topLeft.y = 2; 
    rect.extent.x  = step; rect.extent.y  = 8;

    WinSetClip(&rect);
    WinDrawBitmap(ObjectBitmapPtr[i_am_master?PBar1:PBar2], 126, 2);
    WinResetClip();

    old_val = step;
  }
}

void move_paddle(int no, int dir) {
  int new_pos;
  RectangleType rect;

  /* don't move opponents paddle in single game */
  if(single_game && (no==YOU)) return;

  if(draw_win != WinGetDrawWindow())
    return;

  if(no==ME) {
    /* first remove the part of the old paddle not covered by the new one */
    new_pos = paddle_pos[no]+dir;
    if(new_pos<PADDLE_MIN) new_pos = PADDLE_MIN;

    if(new_pos>PADDLE_MAX)  new_pos = PADDLE_MAX;
  } else
    new_pos = dir;

  /* dir == 0 is 'force redraw' */
  if((new_pos!=paddle_pos[no])||(dir == 0)) {
    rect.topLeft.y = (no==ME)?PADDLE_BOT_Y:PADDLE_TOP_Y; 
    rect.extent.y = PADDLE_HEIGHT;

    if(new_pos<paddle_pos[no]) {
      rect.extent.x = paddle_pos[no] - new_pos;
      rect.topLeft.x = new_pos + PADDLE_WIDTH;
    } else {
      rect.extent.x = new_pos - paddle_pos[no];
      rect.topLeft.x = new_pos - rect.extent.x;
    }

    WinEraseRectangle(&rect,0);

    /* now draw the paddle */
    paddle_pos[no]=new_pos;

    if(i_am_master)
      WinDrawBitmap(ObjectBitmapPtr[(no==ME)?Paddle0:Paddle1], 
		    paddle_pos[no], (no==ME)?PADDLE_BOT_Y:PADDLE_TOP_Y);
    else
      WinDrawBitmap(ObjectBitmapPtr[(no==ME)?Paddle1:Paddle0], 
		    paddle_pos[no], (no==ME)?PADDLE_BOT_Y:PADDLE_TOP_Y);
  }
}

/* set label and rightbound at 150 */
void set_label_right(FormPtr frm, UInt16 object, char *str) {
  UInt16 index;
  Int16 x,y;

  index = FrmGetObjectIndex(frm, object);
  CtlSetLabel(FrmGetObjectPtr(frm, index), str);
  FrmGetObjectPosition(frm, index, &x, &y);
  FrmSetObjectPosition(frm, index, 150-FntLineWidth(str, StrLen(str)), y);
}

void main_draw_score(FormPtr frm) {
  static char score_me[32], score_you[32], score_s[32];

  FntSetFont(2);
  StrPrintF(score_me,  "%lu", score[ME]);    
  StrPrintF(score_you, "%lu", score[YOU]);   
  StrPrintF(score_s,   "%lu", single_score);

  set_label_right(frm, PongWon,   score_me);
  set_label_right(frm, PongLost,  score_you);
  set_label_right(frm, PongScore, score_s);
}

void draw_score(void) {
  char score_str[32];

  if((draw_win != WinGetDrawWindow())||(single_game))
    return;

  FntSetFont(129);
  StrPrintF(score_str, "%02lu:%02lu", loc_score[ME], loc_score[YOU]);
  WinDrawChars(score_str, StrLen(score_str), 80-(FntLineWidth(score_str, StrLen(score_str))/2), 1);
}

void new_ball(int x, int y, int org) {
  int i;

  /* try to find a unused ball */
  for(i=0;i<MAX_BALLS;i++) {
    if((ball_state[i] == BALL_NONE)&&(i != org)) {

      /* new ball in center of brick */
      ball_p[i][AX] = (BORDER_W + 1 + x * (BRICK_WIDTH  + 1))<<BALL_SHIFT;
      ball_p[i][AY] = (BRICK_START  + y * (BRICK_HEIGHT + 1))<<BALL_SHIFT;
      
      /* new ball has direction of original ball */
      ball_dir[i]     = ball_dir[org];
      ball_d[i][AX]   = ball_d[org][AX];
      ball_d[i][AY]   = ball_d[org][AY];

      /* and activate it */
      ball_state[i]   = BALL_NEW; 

      return;
    }
  }
}

void restart_game(void) {
  int i;

  /* init paddles and ball */
  for(i=0;i<2;i++) {
    ball_dir[i]     = 1+SysRandom(0)%5;  /* niemals waagerecht/senkrecht */
    ball_d[i][AX]   = (SysRandom(0)&1)?-1:1;
  }

  if(single_game)
    ball_d[0][AY]   = -1;
  else
    ball_d[0][AY]   = (SysRandom(0)&1)?-1:1;

  ball_d[1][AY]   = -ball_d[0][AY];

  /* disable all balls */
  for(i=0;i<MAX_BALLS;i++)
    ball_state[i] = BALL_NONE;

  ball_step = ball_step_table[setup.ball_speed];
  ball_inc  = ball_inc_table[setup.ball_inc];
  paddle_step = paddle_step_table[setup.paddle_speed];

  if(i_am_master) {
    if(setup.board < 9) level = setup.board;
    else                level = SysRandom(0)%9;
  } else
    level = 0;  /* will be transmitted by master */

  if(!single_game) {
    /* initialize playfield */
    MemMove(playfield, levels[level].data, sizeof(playfield));
  }

  draw_score();

  /* adjust ball */
  for(i=0;i<2;i++) {
    if(ball_d[i][AY]>0) {
      ball_p[i][AX] = 
	(paddle_pos[YOU] + (BALL_CENTER_X - PADDLE_CENTER))<<BALL_SHIFT;
      ball_p[i][AY] = (PADDLE_TOP_Y + PADDLE_HEIGHT)<<BALL_SHIFT;
    } else {
      ball_p[i][AX] = 
	(paddle_pos[ME]  + (BALL_CENTER_X - PADDLE_CENTER))<<BALL_SHIFT;
      ball_p[i][AY] = (PADDLE_BOT_Y - BALL_DIAMETER)<<BALL_SHIFT;
    }
  }
}

const int sincos[]={
  0x00,0x06,0x0c,0x12,0x18,0x1d,0x23,0x27,
  0x2c,0x30,0x34,0x37,0x3a,0x3c,0x3d,0x3e};

void move_ball_slave(unsigned char aball_p[MAX_BALLS][2], 
		     unsigned char ashot[2][2]){
  RectangleType rect;
  int i;

  if(draw_win!=WinGetDrawWindow())
    return;

  /* the slave has caused the master to create a slave shot */
  if((charge == 256)&&(shot[0][0]))
    charge = 0;

  /* erase shots */
  for(i=0;i<2;i++) {
    if(shot[i][0] != 0) {
      rect.topLeft.x = shot[i][0]; rect.extent.x = SHOT_W;
      rect.topLeft.y = shot[i][1]; rect.extent.y = SHOT_H;
      WinEraseRectangle(&rect,0);
    }
  }

  /* draw balls */
  for(i=0;i<MAX_BALLS;i++) {
    if((ball_s[i][AX] != aball_p[i][AX]) || 
       (ball_s[i][AY] != aball_p[i][AY])) { 

      if(ball_s[i][AX] != 0xff) {
	rect.topLeft.x = ball_s[i][AX]; rect.extent.x=BALL_DIAMETER;
	rect.topLeft.y = ball_s[i][AY]; rect.extent.y=BALL_DIAMETER;
	WinEraseRectangle(&rect,0);
      }

      if(aball_p[i][AX] != 0xff)
      	WinDrawBitmap(ObjectBitmapPtr[Ball], aball_p[i][AX], aball_p[i][AY]);

      ball_s[i][AX] = aball_p[i][AX]; 
      ball_s[i][AY] = aball_p[i][AY];
    }
  }

  /* draw shots */
  for(i=0;i<2;i++) {
    if(ashot[i][0] != 0)
      WinDrawBitmap(ObjectBitmapPtr[ShotUp+i], ashot[i][0], ashot[i][1]);
    
    shot[i][0] = ashot[i][0];  shot[i][1] = ashot[i][1];
  }
}

#define HTL 0x08
#define HTR 0x04
#define HBL 0x02
#define HBR 0x01

void check_n_remove_brick(int y, int x, Boolean send_msg, int ball) {
  RectangleType rect;  

  if(playfield[y][x]&0x80) {

    rect.topLeft.x = BORDER_W + 1 + x * (BRICK_WIDTH+1); 
    rect.extent.x  = 16;
    rect.topLeft.y = BRICK_START + y*(BRICK_HEIGHT+1);
    rect.extent.y  = 8;
    
    WinEraseRectangle(&rect,0);

    if(send_msg) {
      /* tell slave about removed brick */
      enqueue_msg(MSG_REMOVE, (y<<4)|x);
    }

    /* create new ball? */
    if(ball>=0) {
      if((playfield[y][x]&(~0x80))==7)
	new_ball(x, y, ball);
    }

    if(single_game) single_draw_score(playfield[y][x]&0x0f);

    playfield[y][x]=0;
  }
}

int check_4_brick(int new_x, int new_y, int ball) {
  int brick_x1, brick_y1, brick_x2, brick_y2, brick_x3, brick_y3, hit;

  brick_x1 = (new_x - BORDER_W - 1) / (BRICK_WIDTH+1);
  brick_x2 = ((new_x+BALL_DIAMETER-1) - BORDER_W - 1) / (BRICK_WIDTH+1);
  brick_y1 = (new_y - BRICK_START) / (BRICK_HEIGHT+1);
  brick_y2 = ((new_y+BALL_DIAMETER-1) - BRICK_START) / (BRICK_HEIGHT+1);

  /* out of brick area? */
  if((brick_y1<0)||(brick_y2<0)||
     (brick_y1>(BRICK_ROWS-1))||
     (brick_y2>(BRICK_ROWS-1))) return 0;
  
  if((brick_x1<0)||(brick_x2<0)||
     (brick_x1>(BRICK_COLUMNS-1))||
     (brick_x2>(BRICK_COLUMNS-1))) return 0;

  hit = (playfield[brick_y1][brick_x1]?HTL:0) |
        (playfield[brick_y1][brick_x2]?HTR:0) |
        (playfield[brick_y2][brick_x1]?HBL:0) |
        (playfield[brick_y2][brick_x2]?HBR:0);

  /* remove hit bricks if required */
  if(hit & HTL) check_n_remove_brick(brick_y1, brick_x1, 1, ball);
  if(hit & HTR) check_n_remove_brick(brick_y1, brick_x2, 1, ball);
  if(hit & HBL) check_n_remove_brick(brick_y2, brick_x1, 1, ball);
  if(hit & HBR) check_n_remove_brick(brick_y2, brick_x2, 1, ball);

  /* running top-left (0/1/2) */
  if((ball_d[ball][AX]==-1) && (ball_d[ball][AY]==-1)) {
    brick_x3 = (new_x + 2 - BORDER_W - 1) / (BRICK_WIDTH+1);
    brick_y3 = (new_y + 2 - BRICK_START) / (BRICK_HEIGHT+1);

    if(playfield[brick_y1][brick_x3]) { 
      hit |= HTR; check_n_remove_brick(brick_y1, brick_x3, 1, ball); }
    if(playfield[brick_y3][brick_x1]) { 
      hit |= HBL; check_n_remove_brick(brick_y3, brick_x1, 1, ball); }

    if((hit == HTL)||(hit == (HTL|HTR|HBL) )||(hit == (HTR|HBL))) 
      { ball_d[ball][AX] = ball_d[ball][AY] = 1; return 1; }
    if((hit == HTR) || (hit == (HTL|HTR)))  { ball_d[ball][AY] = 1; return 1; }
    if((hit == HBL) || (hit == (HTL|HBL)))  { ball_d[ball][AX] = 1; return 1; }
  }

  /* running bottom-right (1/2/3) */
  if((ball_d[ball][AX]==1) && (ball_d[ball][AY]==1)) {
    brick_x3 = ((new_x - 2 + BALL_DIAMETER-1) - BORDER_W-1) / (BRICK_WIDTH+1);
    brick_y3 = ((new_y - 2 + BALL_DIAMETER-1) - BRICK_START) /(BRICK_HEIGHT+1);

    if(playfield[brick_y2][brick_x3]) { 
      hit |= HBL; check_n_remove_brick(brick_y2, brick_x3, 1, ball); }
    if(playfield[brick_y3][brick_x2]) { 
      hit |= HTR; check_n_remove_brick(brick_y3, brick_x2, 1, ball); }

    if((hit == HBR)||(hit == (HBR|HTR|HBL) )||(hit == (HTR|HBL))) 
      { ball_d[ball][AX] = ball_d[ball][AY] = -1; return 1; }
    if((hit == HTR) || (hit == (HBR|HTR))) { ball_d[ball][AX] = -1; return 1; }
    if((hit == HBL) || (hit == (HBR|HBL))) { ball_d[ball][AY] = -1; return 1; }
  }

  /* running top-right (0/1/3) */
  if((ball_d[ball][AX]==1) && (ball_d[ball][AY]==-1)) {
    brick_x3 = ((new_x - 2 + BALL_DIAMETER-1) - BORDER_W-1) / (BRICK_WIDTH+1);
    brick_y3 = (new_y + 2 - BRICK_START) / (BRICK_HEIGHT+1);

    if(playfield[brick_y1][brick_x3]) { 
      hit |= HTR; check_n_remove_brick(brick_y1, brick_x3, 1, ball); }
    if(playfield[brick_y3][brick_x2]) { 
      hit |= HBR; check_n_remove_brick(brick_y3, brick_x2, 1, ball); }

    if((hit == HTR)||(hit == (HTR|HTL|HBR) )||(hit == (HTL|HBR))) 
      { ball_d[ball][AX] = -1; ball_d[ball][AY] = 1; return 1; }
    if((hit == HTL) || (hit == (HTR|HTL))) { ball_d[ball][AY] =  1; return 1; }
    if((hit == HBR) || (hit == (HTR|HBR))) { ball_d[ball][AX] = -1; return 1; }
  }

  /* running bottom-left (0/2/3) */
  if((ball_d[ball][AX]==-1) && (ball_d[ball][AY]==1)) {
    brick_x3 = (new_x + 2 - BORDER_W - 1) / (BRICK_WIDTH+1);
    brick_y3 = ((new_y - 2 + BALL_DIAMETER-1) - BRICK_START) /(BRICK_HEIGHT+1);

    if(playfield[brick_y2][brick_x3]) { 
      hit |= HBR; check_n_remove_brick(brick_y2, brick_x3, 1, ball); }
    if(playfield[brick_y3][brick_x1]) { 
      hit |= HBL; check_n_remove_brick(brick_y3, brick_x1, 1, ball); }

    if((hit == HBL)||(hit == (HBL|HTL|HBR) )||(hit == (HTL|HBR))) 
      { ball_d[ball][AX] = 1; ball_d[ball][AY] = -1; return 1; }
    if((hit == HTL) || (hit == (HBL|HTL))) { ball_d[ball][AX] =  1; return 1; }
    if((hit == HBR) || (hit == (HBL|HBR))) { ball_d[ball][AY] = -1; return 1; }
  }
  
  return 0;
}

void erase_ball(int ball) {
  RectangleType rect;

  /* erase ball */
  rect.topLeft.x = ball_p[ball][AX]>>BALL_SHIFT; 
  rect.extent.x  = BALL_DIAMETER;
  rect.topLeft.y = ball_p[ball][AY]>>BALL_SHIFT; 
  rect.extent.y  = BALL_DIAMETER;
  WinEraseRectangle(&rect,0);
  
  ball_state[ball] = BALL_NONE;
}

void ball_lost(int lwinner) {
  int i;

  if(!single_game) {
    score[lwinner]++;
    loc_score[lwinner]++;
    list_add(lwinner);
  }

  /* save info on laster winner */
  winner = lwinner;

  /* redraw paddle (in case it was damaged by the ball) */
  move_paddle(ME, 0);  
  
  do_sound(SndLost);

  state = SLEEPING;
  sleep_counter = 30;

  /* hide all balls */
  for(i=0;i<MAX_BALLS;i++) {

    if(ball_state[i] != BALL_NONE)
      erase_ball(i);
  }

 if(single_game) {
   if(single_balls == -1) {
     if(single_current_score > single_score) {
       FrmAlert(alt_hiscore);
       single_score = single_current_score;
     }
     /* stop game */
     stop_game();
   }
 }
}

void game_rerun(void) {
  int i;

  state = NEW_GAME;

  shot[0][0] = shot[1][0] = 0;

  paddle_pos[YOU]  = paddle_pos[ME]    = PADDLE_CENTER;

  restart_game();

  draw_game_area();
}

/* check whether shot hit something */
/* dir == 0 is up */
Boolean check_shot(int dir, int x, int y) {
  int xb, yb;
  Boolean ret = false;

  /* hot spot of opponents shot is a bottom */
  if(dir == 1) y += SHOT_H - 1;

  /* find brick */
  yb = y - BRICK_START;

  /* avoid 'gaps' between bricks */
  if((yb % (BRICK_HEIGHT+1)) != BRICK_HEIGHT) {
    yb /= (BRICK_HEIGHT+1);

    if((yb>=0)&&(yb<BRICK_ROWS)) {

      /* left hot spot */
      xb = x - (BORDER_W+1);
      if((xb % (BRICK_WIDTH+1)) != BRICK_WIDTH) {
	xb /= (BRICK_WIDTH+1);

	if((xb>=0)&&(xb<BRICK_COLUMNS)) {
	  if(playfield[yb][xb]) {
	    check_n_remove_brick(yb, xb, 1, -1);
	    ret = true;
	  }
	}
      }

      /* right hot spot */
      xb = x + SHOT_W - 1 - (BORDER_W+1);
      if((xb % (BRICK_WIDTH+1)) != BRICK_WIDTH) {
	xb /= (BRICK_WIDTH+1);

	if((xb>=0)&&(xb<BRICK_COLUMNS)) {
	  if(playfield[yb][xb]) {
	    check_n_remove_brick(yb, xb, 1, -1);
	    ret = true;
	  }
	}
      }
    }
  }
  return ret;
}

#define NOISE_COUNTER 200

void move_ball(void) {
  IrStatus ret;
  int nx[MAX_BALLS],ny[MAX_BALLS], lx,ly, i, ball, j, k;
  RectangleType rect;
  int loop;
  static int noise[MAX_BALLS]={0,0,0};
  int nshot[2][2], squit = 0;
  
#if 0 //XXX
  {
    static int cnt=0;
    char str[32];

    cnt++;
    StrPrintF(str, "%d", cnt%10);
    WinDrawChars(str, StrLen(str), 0, 0);

    for(i=0;i<MAX_BALLS;i++) {
      StrPrintF(str, "%d", ball_state[i]);
      WinDrawChars(str, StrLen(str), 20 + i*10, 0);
    }
  }
#endif

  for(i=0;i<MAX_BALLS;i++)
    noise[i]++;

  if(!single_game)
    if(panic>PANIC_STOP) return;

  if(draw_win!=WinGetDrawWindow())
    return;

  /* process shots */
  for(i=0;i<2;i++) {
    nshot[i][0] = shot[i][0];  
    nshot[i][1] = shot[i][1];
      
    if(shot[i][0] != 0) {

      if(i==0) {
	/* up shot */
	nshot[i][1] -= SHOT_SPEED;
	if(nshot[i][1] < PADDLE_TOP_Y) nshot[i][0] = 0;

	/* don't hit border in single player mode */
	if(single_game) {
	  if(nshot[i][1] < (GAME_AREA_Y + BORDER_W))
	    nshot[i][0]=0;
	}

	/* shot touching paddle? */
	if((nshot[i][1] < (PADDLE_TOP_Y+PADDLE_HEIGHT))&&
	   (nshot[i][0] > (paddle_pos[YOU]-SHOT_W))&&
	   (nshot[i][0] < (paddle_pos[YOU]+PADDLE_WIDTH))) {

	  squit = 1;
	  nshot[i][0] = 0;
	}
      } else {
	/* down shot */
	nshot[i][1] += SHOT_SPEED;
	if(nshot[i][1] > PADDLE_BOT_Y) nshot[i][0] = 0;

	/* shot touching paddle? */
	if((nshot[i][1] > (PADDLE_BOT_Y-SHOT_H))&&
	   (nshot[i][0] > (paddle_pos[ME]-SHOT_W))&&
	   (nshot[i][0] < (paddle_pos[ME]+PADDLE_WIDTH))) {

	  squit = 2;
	  nshot[i][0] = 0;
	}
      }

      /* see whether shot hit something */
      if(check_shot(i, nshot[i][0], nshot[i][1]))
	nshot[i][0] = 0;  /* hitting something kills shot */
    }
  }

  for(ball=0;ball<MAX_BALLS;ball++) {
    nx[ball] = ball_p[ball][AX];
    ny[ball] = ball_p[ball][AY];

    if((state == RUNNING)&&(ball_state[ball]==BALL_MOVE)) {

      if(!squit) {
	for(loop=0;loop<ball_step;loop++) {
	  lx = nx[ball]; ly = ny[ball];
	
	  nx[ball] += ball_d[ball][AX] * (int)sincos[ball_dir[ball]];
	  ny[ball] += ball_d[ball][AY] * sincos[0x0f-ball_dir[ball]];

	  /* ball touches side? */
	  if((nx[ball]<BALL_MIN_X)||(nx[ball]>BALL_MAX_X)) {
	    do_sound(SndWall);

	    nx[ball] = lx;
	    ball_d[ball][AX] = -ball_d[ball][AX];
	  }
	
	  /* ball touches brick? */
	  if(check_4_brick(nx[ball] >> BALL_SHIFT, ny[ball] >> BALL_SHIFT, ball)) {
	  
	    /* add some random noise */
	    if(noise[ball] > NOISE_COUNTER) {

	      /* try to get closer to diagonal movement */
	      if(ball_dir[ball]>8) ball_dir[ball]--;
	      else                 ball_dir[ball]++;
	      
	      noise[ball] = 0;
	    }
	  
	    do_sound(SndBrick);
	  
	    ny[ball] = ly; nx[ball] = lx;
	  }
	
	  /* ball touches lower (MinE) paddle? */
	  if(((nx[ball]>>BALL_SHIFT)>(paddle_pos[ME]-BALL_DIAMETER))&&  /* in x-dir */
	     ((nx[ball]>>BALL_SHIFT)<(paddle_pos[ME]+PADDLE_WIDTH))) {
	  
	    /* hits paddle from the top */
	    if((ny[ball]>>BALL_SHIFT)==(PADDLE_BOT_Y-BALL_DIAMETER+1)) {
	    
	      noise[ball] = 0;	  
	      do_sound(SndPaddle);

	      ny[ball] = (PADDLE_BOT_Y-BALL_DIAMETER)<<BALL_SHIFT;
	    
	      if(master_buttons != 0) {
		if(((master_buttons == MOVE_RIGHT)&&(ball_d[ball][AX]<0))||
		   ((master_buttons == MOVE_LEFT) &&(ball_d[ball][AX]>0))) {
		  ball_dir[ball] -= PADDLE_SHIFT;
		  if(ball_dir[ball]<=0) {
		    ball_dir[ball]    = -ball_dir[ball];
		    ball_d[ball][AX]  = -ball_d[ball][AX];
		  }
		} else {
		  ball_dir[ball] +=PADDLE_SHIFT;
		  if(ball_dir[ball]>0x0e) ball_dir[ball]=0x0e;
		}
	      }  
	      ball_d[ball][AY] = -ball_d[ball][AY];
	    } else if ((ny[ball]>>BALL_SHIFT)>(PADDLE_BOT_Y-BALL_DIAMETER+1)){
	    
	      /* hits paddle from side */
	    
	      /* left side? */
	      if((nx[ball]>>BALL_SHIFT)<(paddle_pos[ME]+PADDLE_WIDTH/2)) {
		nx[ball] = (paddle_pos[ME]-BALL_DIAMETER)<<BALL_SHIFT;
		ball_d[ball][AX] = -1;
	      }
	    
	      /* right side? */
	      if((nx[ball]>>BALL_SHIFT)>(paddle_pos[ME]+PADDLE_WIDTH/2)) {
		nx[ball] = (paddle_pos[ME]+PADDLE_WIDTH)<<BALL_SHIFT;
		ball_d[ball][AX] = 1;
	      }
	    
	      /* ball between paddle and border? */
	      if((nx[ball]<BALL_MIN_X)||(nx[ball]>BALL_MAX_X)) {
		loop = ball_step;
		ball_lost(YOU);  /* you win */
	      }
	    }
	  }
	
	  /* ball touches upper (YOUr) paddle? */
	  if(((nx[ball]>>BALL_SHIFT)>(paddle_pos[YOU]-BALL_DIAMETER))&&
	     ((nx[ball]>>BALL_SHIFT)<(paddle_pos[YOU]+PADDLE_WIDTH))&&
	     (!single_game)) {
	  
	    if((ny[ball]>>BALL_SHIFT)==(PADDLE_TOP_Y+PADDLE_HEIGHT-1)) {

	      noise[ball] = 0;
	      do_sound(SndPaddle);

	      ny[ball] = (PADDLE_TOP_Y+PADDLE_HEIGHT)<<BALL_SHIFT;
	    
	      if(slave_buttons != 0) {
		if(((slave_buttons == MOVE_LEFT) &&(ball_d[ball][AX]<0))||
		   ((slave_buttons == MOVE_RIGHT)&&(ball_d[ball][AX]>0))) {
		  ball_dir[ball] -= PADDLE_SHIFT;
		  if(ball_dir[ball]<=0) {
		    ball_dir[ball] = -ball_dir[ball];
		    ball_d[ball][AX]  = -ball_d[ball][AX];
		  }
		} else {
		  ball_dir[ball] += PADDLE_SHIFT;
		  if(ball_dir[ball]>0x0e) ball_dir[ball]=0x0e;
		}
	      }
	    
	      ball_d[ball][AY] = -ball_d[ball][AY];
	    } else if ((ny[ball]>>BALL_SHIFT)<(PADDLE_TOP_Y+PADDLE_HEIGHT-1)){
	    
	      /* hits paddle from side */
	    
	      /* left side? */
	      if((nx[ball]>>BALL_SHIFT)<(paddle_pos[YOU]+PADDLE_WIDTH/2)) {
		nx[ball] = (paddle_pos[YOU]-BALL_DIAMETER)<<BALL_SHIFT;
		ball_d[ball][AX] = -1;
	      }
	    
	      /* right side? */
	      if((nx[ball]>>BALL_SHIFT)>(paddle_pos[YOU]+PADDLE_WIDTH/2)) {
		nx[ball] = (paddle_pos[YOU]+PADDLE_WIDTH)<<BALL_SHIFT;
		ball_d[ball][AX] = 1;
	      }
	    
	      /* ball between paddle and border? */
	      if((nx[ball]<BALL_MIN_X)||(nx[ball]>BALL_MAX_X)) {
		loop = ball_step;
		ball_lost(ME); /* i win */
	      }
	    }
	  } else {
	    /* in practice mode reflect ball at upper border */
	    if((single_game)&&((ny[ball]>>BALL_SHIFT) <= (GAME_AREA_Y+BORDER_W))) {
	      if(ball_d[ball][AY] < 0) {
		ball_d[ball][AY] = -ball_d[ball][AY];
		do_sound(SndWall);
	      }
	    }
	  }
	  
	  /* ball left game area */
	  if((ny[ball]>>BALL_SHIFT) > 
	     (GAME_AREA_Y + GAME_AREA_H - BALL_DIAMETER)) {
	    loop = ball_step;

	    /* in single game all balls must be lost to loose */
	    if(single_game) {
	      erase_ball(ball);
	      
	      for(k=0,j=0;j<MAX_BALLS;j++)
		if(ball_state[j] != BALL_NONE)
		  k++;

	      if(k==0) ball_lost(YOU);
	    } else
	      ball_lost(YOU);
	  }

	  if(!single_game) {
	    if((ny[ball]>>BALL_SHIFT) < (GAME_AREA_Y)) {
	      loop = ball_step;
	      ball_lost(ME);
	    }
	  }
	}
      } else {
	/* squit */
	if(squit == 1) ball_lost(ME);
	if(squit == 2) ball_lost(YOU);
      }
    } else if((state == RUNNING)&&(ball_state[ball] == BALL_STICK)) {
      if(ball_d[ball][AY]>0) {
	nx[ball] = (paddle_pos[YOU] + (BALL_CENTER_X - PADDLE_CENTER))<<BALL_SHIFT;
	ny[ball] = (PADDLE_TOP_Y + PADDLE_HEIGHT)<<BALL_SHIFT;
      } else {
	nx[ball] = (paddle_pos[ME]  + (BALL_CENTER_X - PADDLE_CENTER))<<BALL_SHIFT;
	ny[ball] = (PADDLE_BOT_Y - BALL_DIAMETER)<<BALL_SHIFT;
      }
    } else {
      nx[ball] = ball_p[ball][AX];
      ny[ball] = ball_p[ball][AY];
    }
  }

  /* erase shots */
  for(i=0;i<2;i++) {
    if(shot[i][0] != 0) {
      rect.topLeft.x = shot[i][0]; rect.extent.x = SHOT_W;
      rect.topLeft.y = shot[i][1]; rect.extent.y = SHOT_H;
      WinEraseRectangle(&rect,0);
    }
  }

  rect.extent.x = BALL_DIAMETER;
  rect.extent.y = BALL_DIAMETER;

  for(ball=0;ball<MAX_BALLS;ball++) {
    if(ball_state[ball] >= BALL_MOVE) {
      if((ball_p[ball][AX] != nx[ball]) || (ball_p[ball][AY] != ny[ball])) {
	/* erase ball */
	rect.topLeft.x = ball_p[ball][AX]>>BALL_SHIFT;
	rect.topLeft.y = ball_p[ball][AY]>>BALL_SHIFT;
	WinEraseRectangle(&rect,0);
      }
    }
  }

  for(ball=0;ball<MAX_BALLS;ball++) {
    if(ball_state[ball] >= BALL_MOVE) {
      if((ball_p[ball][AX] != nx[ball]) || (ball_p[ball][AY] != ny[ball])) {
	/* draw ball */
	ball_p[ball][AX] = nx[ball]; ball_p[ball][AY] = ny[ball];
	WinDrawBitmap(ObjectBitmapPtr[Ball], 
		      ball_p[ball][AX]>>BALL_SHIFT, 
		      ball_p[ball][AY]>>BALL_SHIFT);
      }
    }

    /* activate new balls */
    if(ball_state[ball] == BALL_NEW)
      ball_state[ball] = BALL_MOVE;
  }

  /* draw shots */
  for(i=0;i<2;i++) {
    if(nshot[i][0] != 0)
      WinDrawBitmap(ObjectBitmapPtr[ShotUp+i], nshot[i][0], nshot[i][1]);
    
    shot[i][0] = nshot[i][0];  shot[i][1] = nshot[i][1];
  }

  /* check if single player game is won */
  if(single_game) {
    for(i=0,ly=0;ly<BRICK_ROWS;ly++)
      for(lx=0;lx<BRICK_COLUMNS;lx++)
	if(playfield[ly][lx] & 0x80) i++;

    /* all bricks removed */
    if(i==0) {

      /* next level */
      single_level++;
      if(single_level == 9) single_level=0;
      MemMove(playfield, levels[single_level].data, sizeof(playfield));

      single_balls++;

      start_game(1);
    }
  }
}

void controls(Boolean hide) {
  int i, objects[]={PongIRConnect, PongBTConnect, PongPractice, -1};

  for(i=0;objects[i]!=-1;i++) {
    if((hide) || ((objects[i] == PongBTConnect) && (!bt_present)))
      CtlHideControl(FrmGetObjectPtr(mainform, FrmGetObjectIndex (mainform, objects[i])));
    else
      CtlShowControl(FrmGetObjectPtr(mainform, FrmGetObjectIndex (mainform, objects[i])));
  }
}

/* start a game of IR pong */
void start_game(int mode) {
  FormPtr frm;

  winner = 2;  /* no winner */
  loc_score[ME] = loc_score[YOU] = 0;
  i_am_master=mode;

  /* hide connect button */
  controls(true);

  restart_game();

  paddle_pos[ME]   = paddle_pos[YOU]   = PADDLE_CENTER;

  if(ball_d[0][AY]>0) { 
    /* ball sticks to opposite paddle */
    ball_p[0][AX] = 
      (paddle_pos[YOU] + (BALL_CENTER_X - PADDLE_CENTER))<<BALL_SHIFT;
    ball_p[0][AY] = (PADDLE_TOP_Y + PADDLE_HEIGHT)<<BALL_SHIFT;
  } else {
    /* ball sticks to own paddle */
    ball_p[0][AX] = 
      (paddle_pos[ME] + (BALL_CENTER_X - PADDLE_CENTER))<<BALL_SHIFT;
    ball_p[0][AY] = (PADDLE_BOT_Y - BALL_DIAMETER)<<BALL_SHIFT;
  }

  draw_game_area();

  if(i_am_master) {
    /* wait for key press */
    state = NEW_GAME;
  } else {
    /* just be slave */
    state = SLAVE_RUNNING;

    /* Mask hardware keys */
    KeySetMask(~(keyBitHard1 | keyBitHard2 | keyBitHard3 | keyBitHard4));
  }
}

int redraw_main=false;

/* stop a game of IR pong */
void redraw_main_win(void) {
  FormPtr frm = FrmGetActiveForm();
  RectangleType rect;

  if(draw_win == WinGetDrawWindow()) {
    /* clear */
    rect.topLeft.x=0;  rect.topLeft.y=GAME_AREA_Y; 
    rect.extent.x=SCREEN_WIDTH; rect.extent.y=GAME_AREA_H;
    WinEraseRectangle(&rect,0);

    /* clear progress bar */
    rect.topLeft.x = 100;   rect.topLeft.y = 2; 
    rect.extent.x  =  58;   rect.extent.y  = 8;
    WinEraseRectangle(&rect,0);

    /* redraw main window */
    main_draw_score(frm);
    FrmDrawForm(frm);
  }

  /* show controls */
  controls(false);
}

void stop_game(void) {
  state = SETUP;

  /* unmask keys */
  KeySetMask( keyBitsAll );

  if(draw_win != WinGetDrawWindow()) 
    redraw_main = true;
  else
    redraw_main_win();
}

static Boolean StartApplication(void) {
  int i;
  FormPtr frm;  
  char str[32];
  SystemPreferencesType sysPrefs;
  UInt32 newDepth, depth;

  PongPrefs prefs;
  UInt16 error, size;
  
  FontPtr font129;
  int wPalmOSVerMajor;
  long dwVer;

  level = slave_level = -1; 

  /* load new font */
  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &dwVer);
  wPalmOSVerMajor = sysGetROMVerMajor(dwVer);
  if (wPalmOSVerMajor < 3) {
    FrmAlert(alt_wrongOs);
    return false;
  }

  /* switch to appropriate video mode */

  /* if BW, try to enter greyscale */
  WinScreenMode(winScreenModeGet, NULL, NULL, &depth, NULL);
  if(depth == 1) {
    WinScreenMode(winScreenModeGetSupportedDepths, NULL, NULL, &depth, NULL);

    if(depth & ((1<<1)|(1<<3))) {

      /* is 4bpp or 2bpp supported? */
      if (depth & (1<<3))     newDepth = 4;
      else if(depth & (1<<1)) newDepth = 2;

      WinScreenMode(winScreenModeSet, NULL, NULL, &newDepth, NULL);
    }
  }

  font129 = MemHandleLock(DmGetResource('NFNT', 1001));
  FntDefineFont(129, font129);

  /* load preferences */
  size = sizeof(PongPrefs);
  error=PrefGetAppPreferences( CREATOR, 0, &prefs, &size, true);

  /* get lib from prefs */
  if((error!=noPreferenceFound)&&(prefs.version==PREFS_VERSION)) {
    /* valid prefs */
    score[ME]  = prefs.score[ME];
    score[YOU] = prefs.score[YOU];
    setup = prefs.setup;

    /* get single player score */
    single_score = prefs.single_score;
  } else {
    /* no prefs */
    score[ME] = score[YOU] = 0;
    loc_score[ME] = loc_score[YOU] = 0;

    /* setup setup */
    setup.ball_speed = 1;
    setup.ball_inc = 1;
    setup.paddle_speed = 1;
    setup.sound = 1;
    setup.board = 1;

    single_score = 0;
  }

  /* get system preferences for sound */
  PrefGetPreferences(&sysPrefs);
  if(sysPrefs.gameSoundLevelV20 == slOff) setup.sound = 0;

  /* load list of games */
  list_init();

  /* open ir interface, if this fails bail out */
  if(!ir_open()) return false;

  /* open bluetooth, if this fails just remember */
  bt_present = bt_open();

  // Keep the Object graphics locked because they are frequently used
  for (i = 0; i < bitmapTypeCount; i++) {
    ObjectBitmapHandles[i] = (MemHandle)DmGetResource(bitmapRsc, firstObjectBmp + i);
    ObjectBitmapPtr[i] = MemHandleLock(ObjectBitmapHandles[i]);
  }

  SysRandom(TimGetTicks());

  FrmGotoForm(frm_Main);

  return true;
}

static void StopApplication(void) {
  int i;
  PongPrefs prefs;

  /* save game info */
  list_save();

  // Unlock and release the locked bitmaps
  for (i = 0; i < bitmapTypeCount; i++) {
    MemPtrUnlock(ObjectBitmapPtr[i]);
    DmReleaseResource(ObjectBitmapHandles[i]);
  }

  /* close bt interface */
  bt_close();

  /* close ir interface */
  ir_close();

  /* return to default video mode */
  WinScreenMode(winScreenModeSetToDefaults, NULL, NULL, NULL, NULL);

  /* save preferences */
  prefs.version      = PREFS_VERSION;
  prefs.score[ME]    = score[ME];
  prefs.score[YOU]   = score[YOU]; 
  prefs.setup        = setup;
  prefs.single_score = single_score;

  PrefSetAppPreferences( CREATOR, 0, 1, &prefs, sizeof(PongPrefs), true);  
}

static Boolean MainFormHandleEvent(EventPtr event)
{
  FormPtr frm;
  char str[32], *id_str;
  int id,i;
  ListPtr lst;
  MemHandle h;
  
  Boolean handled = false;
  
  switch (event->eType) {
  case ctlSelectEvent:
    if(event->data.ctlEnter.controlID == PongIRConnect) {
      single_game = false;
      ir_start_discovery();
      handled=true;
    }

    if(event->data.ctlEnter.controlID == PongBTConnect) {
      single_game = false;
      bt_start_discovery();
      handled=true;
    }

    if(event->data.ctlEnter.controlID == PongPractice) {

      /* initialize playfield */
      single_level = 0;
      single_balls = 3;
      single_current_score=0;

      MemMove(playfield, levels[single_level].data, sizeof(playfield));

      single_game = true;
      StrCopy(opponent_name, "Practice");
      start_game(1);
    }

    break;

  case frmOpenEvent:
    main_draw_score(FrmGetActiveForm());
    controls(false);
    FrmDrawForm(FrmGetActiveForm());

    draw_win = WinGetDrawWindow();

    handled = true;
    break;

  case menuEvent:
    switch (event->data.menu.itemID) {

    case menuSend:
      send_app();
      break;

    case menuBeam:
      beam_app();
      break;

    case menuList:
      list_show();
      break;

    case menuHelp:
      show_text("Instructions", MainHelp, NULL);
      break;

    case menuDiscon:
      if(state != SETUP) {
	/* stop running game */
	if(!single_game) {
	  if(ACLcon) // ACLcon -> BT up
	    bt_discon(true);
	  else
	    ir_discon(true);
	} else
	  stop_game();
      } else {
	/* quit the whole game */
	if(ACLcon)
	  bt_discon(true);

	quit = true;
      }

      state = SETUP;

      /* unmask keys */
      KeySetMask( keyBitsAll );

      break;

    case menuAbout:
      frm=FrmInitForm(frm_About);

      // setup the version string
      h = DmGetResource(verRsc, 1);
      FrmCopyLabel(frm, AboutVersion, MemHandleLock(h));
      MemHandleUnlock(h);
      DmReleaseResource(h);

      FrmDoDialog(frm);
      FrmDeleteForm(frm);
      break;

    case menuSetup:
      frm=FrmInitForm(frm_Setup);

      /* set sound selector */
      CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SetupSound)), setup.sound);      

      /* set board indicator */
      lst = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SetupBrdLst));
      LstSetSelection(lst, setup.board);
      CtlSetLabel(FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,SetupBrdLstSel)), 
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
      break;
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
  UInt16    formId;
  Boolean handled = false;
  
  if (event->eType == frmLoadEvent) {
    formId = event->data.frmLoad.formID;
    mainform = FrmInitForm(formId);
    FrmSetActiveForm(mainform);

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
  UInt16 error, buttons;
  UInt32 hardKeyState;
  IrStatus ret;
  char str1[8], str2[8];
  
  UInt16 x,y, speed_cnt=0;
  Int32 ticks_wait, i,l;
  
  /* initialize Timer */
  ticks_wait = SysTicksPerSecond()/GAME_HZ;  /* 40ms/frame=25Hz */    
  if(ticks_wait < 1) ticks_wait = 1;

  l = TimGetTicks()+ticks_wait;
  
  do {
    i=l-TimGetTicks();
    EvtGetEvent(&event, (i<0)?0:i);

    if (!SysHandleEvent(&event))
	if (!MenuHandleEvent(CurrentMenu, &event, &error))
	    if (!ApplicationHandleEvent(&event))
		FrmDispatchEvent(&event);

    /* bt is handled here */
    bt_send_state();

    if(event.eType == appStopEvent) {
      if(ACLcon)
	bt_discon(true);

      quit = 1;
    }
   
    /* do some kind of 25Hz timer event */
    if(TimGetTicks()>l) {

      l = TimGetTicks()+ticks_wait;
      hardKeyState = KeyCurrentState();

      if(state!=SETUP) {
	buttons = 0;

	/* process charge bar */
	if(charge<256) {
	  charge++;
	  draw_pbar(charge/8);
	}

	/* only move while playing */
	if((state == RUNNING)||(state == SLAVE_RUNNING)) {

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

	if(!single_game) {
	  /* verify that connection is alive */
	  if((state != SETUP)&&(state != WRONG_VERSION)) {
	    panic++;
	    
	    /* use panic counter only for irda */
	    if((panic==PANIC_TIMEOUT)&&(!ACLcon)) {
	      FrmAlert(alt_irlost);
	      
	      if(ACLcon) 
		bt_discon(false);
	      else
		ir_discon(false);
	      
	      state = SETUP;

	      /* unmask keys */
	      KeySetMask( keyBitsAll );
	    }
	  }
	}

	/* redraw main if needed */
	if(redraw_main) {
	  redraw_main_win();
	  redraw_main = false;
	}

	if(i_am_master) {
	  master_buttons = buttons;

	  if(state == WRONG_VERSION) {
	    FrmCustomAlert(alt_version, StrIToA(str1, COMM_VERSION), 
			   StrIToA(str2, wrong_version), 0);
	    stop_game();
	    state = SETUP;

	    /* unmask keys */
	    KeySetMask( keyBitsAll );

	  } else if(state == SLEEPING) {
	    sleep_counter--;
	    if(sleep_counter == 0) {
	      game_rerun();
	    }
	  } else if(state == RUNNING) {
	    /* 10 sek */
	    if(speed_cnt++==ball_inc) {
	      ball_step++;  /* increase ball speed */
	      speed_cnt = 0;
	    }

	    /* check whether master fired a shot */
	    if((charge == 256)&&(buttons & MOVE_START)) {
	      shot[0][0] = PADDLE2SHOT(paddle_pos[ME]);
	      shot[0][1] = SHOT_START;
	      charge = 0;
	    }

	    /* check whether slave fired a shot */
	    if((slave_buttons & MOVE_SHOT)&&(shot[1][0]==0)) {
	      shot[1][0] = PADDLE2SHOT(paddle_pos[YOU]);
	      shot[1][1] = PADDLE_TOP_Y + PADDLE_HEIGHT;
	    }
	    
	    for(i=0;i<MAX_BALLS;i++) {
	      /* it's masters turn ... */
	      if((ball_d[i][AY]<0)&&(buttons & MOVE_START)&&
		 (ball_state[i]==BALL_STICK))
		ball_state[i] = BALL_MOVE;  /* ok, start ball */

	      /* this is for slaves turn ... */
	      if((ball_d[i][AY]>0)&&(slave_buttons & MOVE_START)&&
		 (ball_state[i]==BALL_STICK))
		ball_state[i] = BALL_MOVE;  /* ok, start ball */
	    }

	    /* master moves ball */
	    move_ball();
	  }
	} else {

	  slave_buttons = buttons;
	  if((!shot[0][0])&&(charge==256)&&(buttons & MOVE_START))
	    slave_buttons |= MOVE_SHOT;

	  if(slave_level != -1) {
	    if(draw_win == WinGetDrawWindow()) {
	      level = slave_level;
	      slave_level = -1;
	      
	      /* initialize playfield */
	      MemMove(playfield, levels[level].data, sizeof(playfield));

	      draw_game_area();
	    }
	  }
	}

	if(single_game) {
	  if(state == NEW_GAME) {
	    state = RUNNING;

	    /* Mask hardware keys */
	    KeySetMask(~(keyBitHard1 | keyBitHard2 | keyBitHard3 | keyBitHard4));

	    for(i=0;i<MAX_BALLS;i++) 
	      ball_state[i] = BALL_NONE;
	    
	    ball_state[0] = BALL_STICK;
	    
	    if(levels[level].dual_ball)
	      ball_state[1] = BALL_STICK;
	  }
	}
      }
    }
  } while((!quit)||(ACLcon));  // only quit with no existing ACL con 

  FrmCloseAllForms();
}

/////////////////////////
// PilotMain
/////////////////////////

UInt32 PilotMain(UInt16 cmd, void *cmdBPB, UInt16 launchFlags) {
  if (cmd == sysAppLaunchCmdNormalLaunch) {

    if(!StartApplication()) return(0);
    EventLoop();
    StopApplication();
  }
  return(0);
}











