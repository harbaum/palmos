#ifndef __IR_PONG_H__
#define __IR_PONG_H__

#define MAX_BALLS      3

#define COMM_VERSION   2  /* version of communication protocol */

/* master packet types */
#define MSTR_BALL      0
#define MSTR_END       1

#define GAME_HZ        50

#define PANIC_STOP     4            /* ten lost packets: stop ball */
#define PANIC_TIMEOUT  (GAME_HZ*2)  /* connection lost after 2 sec */

#define MSG_IDLE       0
#define MSG_REMOVE     1

#define PADDLE_SHIFT   2

#define PADDLE_HEIGHT  8
#define PADDLE_WIDTH   32
#define SCREEN_WIDTH   160
#define SCREEN_HEIGHT  160
#define BALL_DIAMETER  8
#define SHOT_H         12
#define SHOT_W         8

#define BALL_SHIFT     7
#define GAME_AREA_Y    16
#define GAME_AREA_H    (SCREEN_HEIGHT-GAME_AREA_Y)
#define BORDER_W       3

#define PADDLE_TOP_Y   GAME_AREA_Y
#define PADDLE_BOT_Y   (GAME_AREA_Y+GAME_AREA_H-PADDLE_HEIGHT)
#define PADDLE_CENTER  ((SCREEN_WIDTH-PADDLE_WIDTH)/2) 
#define PADDLE_MIN     BORDER_W
#define PADDLE_MAX     (SCREEN_WIDTH-BORDER_W-PADDLE_WIDTH)
#define PADDLE_MIRROR(a)  (SCREEN_WIDTH-PADDLE_WIDTH-(a))

#define PADDLE2SHOT(a)  (a+((PADDLE_WIDTH-SHOT_W)/2))
#define SHOT_START     (PADDLE_BOT_Y - SHOT_H)
#define SHOT_SPEED     2
#define SHOT_MIRROR_X(a) (SCREEN_WIDTH-SHOT_W-(a))
#define SHOT_MIRROR_Y(a) (2*GAME_AREA_Y+GAME_AREA_H-SHOT_H-(a))

#define BALL_CENTER_X  ((SCREEN_WIDTH-BALL_DIAMETER)/2)
#define BALL_CENTER_Y  (GAME_AREA_Y+(GAME_AREA_H/2)-(BALL_DIAMETER/2))
#define BALL_MIN_X     ((BORDER_W)<<BALL_SHIFT)
#define BALL_MAX_X     ((SCREEN_WIDTH-BALL_DIAMETER-BORDER_W)<<BALL_SHIFT)
#define BALL_MIN_Y     ((GAME_AREA_Y)<<BALL_SHIFT)
#define BALL_MAX_Y     ((GAME_AREA_Y+GAME_AREA_H-BALL_DIAMETER)<<BALL_SHIFT)
#define BALL_MIRROR_X(a) (SCREEN_WIDTH-BALL_DIAMETER-(a))
#define BALL_MIRROR_Y(a) (2*GAME_AREA_Y+GAME_AREA_H-BALL_DIAMETER-(a))

#define BRICK_HEIGHT   8
#define BRICK_WIDTH    16
#define BRICK_ROWS     13
#define BRICK_COLUMNS  ((SCREEN_WIDTH - 2*BORDER_W - 1) / (BRICK_WIDTH+1))
#define BRICK_START    (GAME_AREA_Y+(GAME_AREA_H/2) - (((BRICK_HEIGHT+1) * (BRICK_ROWS+2))/2)) 

#define MOVE_LEFT  1
#define MOVE_RIGHT 2
#define MOVE_START 4
#define MOVE_SHOT  8

#define ME  0
#define YOU 1

#define AX 0
#define AY 1

#define BALL_NONE  0
#define BALL_NEW   1
#define BALL_MOVE  2
#define BALL_STICK 3

/* level */
typedef struct {
  char name[16];
  Boolean dual_ball;
  UInt8 data[13][9];
} level_def;

/* Packet sent from Master to Client */
typedef struct { 
  UInt8  type;
  UInt8  ball_p[MAX_BALLS][2];
  UInt8  paddle_pos;
  UInt8  shot[2][2];
  UInt16 msg;
} master_state;

#if 0 // new protocol
/* Packet sent from Master to Client */
typedef struct { 
  UInt8  type;

  union {
    UInt8  ball_p[MAX_BALLS][2];
    UInt8  paddle_pos;
    UInt8  shot[2][2];
  }

  UInt16 msg;
} master_state;
#endif

typedef struct {
  UInt8 paddle_pos;
  UInt8 buttons;      // ff -> paddle_pos = com_version
  UInt8 msg_ack;
} slave_state;

struct setup {
  UInt8 ball_speed:2,
        ball_inc:2,
        paddle_speed:2,
        sound:1;
  UInt8 board;
} setup;

#define PREFS_VERSION 6

typedef struct {
  Int8 version;
  UInt32 score[2];              /* won/lost games */
  struct setup setup;           /* player/ball settings */
  
  UInt32 single_score;          /* top score in single player mode */
} PongPrefs;

/* different states the game can be in */
typedef enum {SETUP=0, RUNNING, SLEEPING, NEW_GAME, WRONG_VERSION, SLAVE_RUNNING, SLAVE_SETUP, SLAVE_ACK} game_state;

extern level_def levels[];

/* stuff exported from IR-Pong.c */
extern game_state state;
extern UInt16 panic;
extern Boolean i_am_master;
extern int wrong_version;
extern int winner, level, slave_level;
extern UInt8 master_buttons, slave_buttons;
extern master_state master;
extern slave_state slave;
extern unsigned long score[2], loc_score[2];
extern int paddle_pos[2];
extern UInt8 last_sound;
extern int shot[2][2];
extern unsigned int ball_p[MAX_BALLS][2];
extern UInt8 ball_s[MAX_BALLS][2];
extern int ball_state[MAX_BALLS];
extern Boolean single_game;
extern Boolean quit;

extern void move_ball_slave(UInt8 ball_p[MAX_BALLS][2], UInt8 ashot[2][2]);
extern void move_paddle(int no, int dir);
extern void stop_game(void);
extern void start_game(int mode);
extern void draw_score(void);
extern void btn_set(Boolean mode);

/* ir.c */
extern Boolean ir_open(void);
extern void ir_close(void);
extern void ir_start_discovery(void);
extern void ir_discon(Boolean complain);

/* bt.c */
extern Boolean bt_open(void);
extern void bt_close(void);
extern void bt_start_discovery(void);
extern Boolean l2cap_close(void);
extern Boolean bt_discon(Boolean complain);
extern Boolean ACLcon;
extern void bt_send_state(void);

/* msg_queue.c */
extern Boolean enqueue_msg(int type, int data);
extern UInt16 current_msg(void);
extern void update_queue(UInt8 ack);

/* textbox.c */
extern void show_text(char *title, UInt16 rsc, MemHandle hand);

/* beam.c */
extern void beam_app(void);
extern void send_app(void);

/* playlist.c */
extern char opponent_name[];

extern void list_init(void);
extern void list_add(Boolean win);
extern void list_show(void);
extern void list_save(void);
extern Boolean known_user(void);

#endif
