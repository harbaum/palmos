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
/*
 * sequences.c   -   in game sequences for Elite
 */


#include <PalmOS.h>
#include "sections.h"

#include "debug.h"
#include "config.h"
#include "gfx.h"
#include "vector.h"
#include "elite.h"
#include "space.h"
#include "sound.h"
#include "random.h"
#include "intro.h"
#include "stars.h"
#include "threed.h"
#include "main.h"
#include "keyboard.h"
#include "sequences.h"
#include "colors.h"
#include "docked.h"
#include "missions.h"
#include "swat.h"
#include "rumble.h"
#include "pilot.h"
#include "options.h"

static UInt8 get_random_ship(void) SEC3D;

#ifdef INTRO1
void
run_first_intro_screen (void)
{
  current_screen = SCR_INTRO_ONE;

  initialise_intro1 ();

  do {
    update_intro1 ();

    gfx_update_screen ();

    kbd_poll_keyboard ();
  } while(!(kbd_button_map & KBD_TAP) && (!finish));
}
#endif

#ifdef INTRO2
void
run_second_intro_screen (void)
{
  current_screen = SCR_INTRO_TWO;

  initialise_intro2 ();

  flight_speed = FLIGHT_SCALE(3);
  flight_roll  = FLIGHT_SCALE(0);
  flight_climb = FLIGHT_SCALE(0);

  do {
    update_intro2 ();

    gfx_update_screen ();

    kbd_poll_keyboard ();
  } while(!(kbd_button_map & KBD_TAP) && (!finish));
}
#endif

/*
 * Draw the game over sequence. 
 */

void
run_game_over_screen (void)
{
  int i;
  int newship;
  RotMatrix rotmat;
  int type;

  /* stop rumble */
  if(rumble_cnt > 0) {
    rumble_cnt = 0;
    rumble(0);
  }

  current_screen = SCR_GAME_OVER;

  flight_speed = FLIGHT_SCALE(6);
  flight_roll  = FLIGHT_SCALE(0);
  flight_climb = FLIGHT_SCALE(0);

  clear_universe();

  set_init_matrix (rotmat);


  newship = add_new_ship (SHIP_COBRA3, 0, 0, -400, rotmat, 0, 0);
  universe[newship].flags |= FLG_DEAD;

  for (i = 0; i < 5; i++) {
    type = (rand255 () & 1) ? SHIP_CARGO : SHIP_ALLOY;
    newship = add_new_ship (type, (rand255 () & 63) - 32,
			    (rand255() & 63) - 32, -400, rotmat, 0, 0);
    universe[newship].rotz = ((rand255() * 2) & 255) - 128;
    universe[newship].rotx = ((rand255() * 2) & 255) - 128;
    universe[newship].velocity = rand255() & 3;
  }

  gfx_update_button_map(-1);

  for (i = 0; (!finish) && (i < 100); i++) {
    gfx_clear_display();
    update_starfield ();
    update_universe ();
    gfx_display_text(DSP_CENTER, DSP_HEIGHT/2-3, ALIGN_CENTER,
		     "GAME OVER", GFX_COL_GOLD);
    gfx_update_screen ();
  }

  gfx_draw_scanner();
}

/*
 * Draw a break pattern (for hyperspacing).
 * Just draw a very simple one for the moment.
 */

void
display_break_pattern(int mode)
{
  int i,j;

  const UInt32 color32[2][4] = {
    { 
      /* TD_COL_GREEN_1  */ (MK_COLOR (0xd2)),
      /* TD_COL_GREEN_2  */ (MK_COLOR (0xd3)),
      /* TD_COL_GREEN_3  */ (MK_COLOR (0xd4)),
      /* TD_COL_GREEN_4  */ (MK_COLOR (0xd5))
    },{
      /* TD_COL_RED      */ (MK_COLOR (0x77)),
      /* TD_COL_DARK_RED */ (MK_COLOR (0x8f)),
      /* TD_COL_RED_3    */ (MK_COLOR (0xa1)),
      /* TD_COL_RED_4    */ (MK_COLOR (0xb3))
    }
  };

  /* stop rumble */
  if(rumble_cnt > 0) {
    rumble_cnt = 0;
    rumble(0);
  }

  if(mode <= 1) {

    for(j=0;(j<30)&&(!finish);j++) {
      gfx_clear_display();

      /* draw sixteen circles */
      for(i=0;i<16;i++) {
	draw_filled_circle (DSP_WIDTH/2, DSP_HEIGHT/2, 
			    SQUARE((j&1)+2l*(15-i))>>3, 
			    color32[mode][((j/2)+i)&3]);
      }
      
      /* process messages (especially target lost) */
      if (message_count > 0) {
	message_count--;
	gfx_display_text(DSP_CENTER, MESSAGE_Y, ALIGN_CENTER,
			 message_string, GFX_COL_WHITE);
      }

      gfx_update_screen ();
      
      /* the keyboard is polled to give the appl stop event a chance */
      kbd_poll_keyboard ();
    }
  }

  /* stop hyperspace sound loop */
  snd_sound_shutdown();

  current_screen = SCR_FRONT_VIEW;
  gfx_update_button_map(0);
}

/*
 * Draw the launch sequence from the dodo/coriolis station
 */

static UInt8 get_random_ship(void) {
  const UInt8 ship_yard[32]={
    SHIP_SHUTTLE, SHIP_TRANSPORTER, SHIP_COBRA3, SHIP_PYTHON,
    SHIP_BOA, SHIP_ANACONDA, SHIP_VIPER, SHIP_SIDEWINDER,
    SHIP_MAMBA, SHIP_KRAIT, SHIP_ADDER, SHIP_GECKO,
    SHIP_COBRA1, SHIP_WORM, SHIP_ASP2, SHIP_FER_DE_LANCE,
    SHIP_MORAY, SHIP_VIPER, SHIP_VIPER, SHIP_VIPER,
    0,0,0,0,0,0,0,0,0,0,0,0 };

  return ship_yard[rand255()%31];
}

#define TOP  (0)
#define BOT  (DSP_HEIGHT)
#define LFT  (0)
#define RGT  (DSP_WIDTH-1)

void
display_launch_sequence(void) {
  int i, x1,x2,x3,x4,y1,y2,y3,y4;
  Int32 j;
  RotMatrix rotmat;
  int newship;
  struct point2d points[16];
  const UInt8 top_list[]={0,1,7,6,5,4,3,2};
  const UInt8 bot_list[]={8,9,10,11,12,13,15,14};
  const UInt8 l1_list[]={3,4,10,9};
  const UInt8 r1_list[]={5,6,12,11};
  const UInt8 l2_list[]={2,3,9,8};
  const UInt8 r2_list[]={6,7,13,12};
  const int pos[2]={-100,100};

  create_new_stars();
  current_screen = SCR_FRONT_VIEW;
  gfx_update_button_map(NO_BUTTONS);

  clear_universe();
  set_init_matrix(rotmat);

#ifndef NO_SEQUENCES 
  /* try to add two ships */
  for(i=0;i<2;i++) {

    newship = get_random_ship();
    if(newship != 0) {
      newship = add_new_ship(newship,  pos[i], -40, 1000, rotmat, 0, 0);

      /* remove this and parked hostile ships will attack :-) */
      universe[newship].flags = 0;

      /* turn ship around 180 deg */
      universe[newship].rotmat[0].x = -universe[newship].rotmat[0].x;
      universe[newship].rotmat[2].z = -universe[newship].rotmat[2].z;
    }
  }

  for (i = 0; (!finish) && (i < 100); i++) {
    gfx_clear_display();

    flight_speed = FLIGHT_SCALE(12);
    flight_roll  = FLIGHT_SCALE(-15);
    flight_climb = FLIGHT_SCALE(0);

    /* draw stars */
    update_starfield ();

    /* planet is green */
    draw_filled_circle (DSP_WIDTH/2, DSP_HEIGHT/2, 
			GALAXY_SCALE(6291456l / 65536l), 
			0xd4d4d4d4l);

    j = 10000l/(100l-i);
    x1 = DSP_WIDTH/2 - j*DSP_WIDTH/300;    x2 = DSP_WIDTH/2 - j*DSP_WIDTH/500;
    x3 = DSP_WIDTH/2 + j*DSP_WIDTH/500;    x4 = DSP_WIDTH/2 + j*DSP_WIDTH/300;

    y1 = DSP_HEIGHT/2 - j*DSP_HEIGHT/600;  y2 = DSP_HEIGHT/2 - j*DSP_HEIGHT/1000;
    y3 = DSP_HEIGHT/2 + j*DSP_HEIGHT/1000; y4 = DSP_HEIGHT/2 + j*DSP_HEIGHT/600;

    /* set all 16 points of the launch window */
    SET_POINT(points[0],  LFT, TOP);    SET_POINT(points[1], RGT, TOP);

    SET_POINT(points[2],  LFT,  y1);    SET_POINT(points[3],   x1, y1);
    SET_POINT(points[4],   x2,  y2);    SET_POINT(points[5],   x3, y2);
    SET_POINT(points[6],   x4,  y1);    SET_POINT(points[7],  RGT, y1);

    SET_POINT(points[8],  LFT,  y4);    SET_POINT(points[9],   x1, y4);
    SET_POINT(points[10],  x2,  y3);    SET_POINT(points[11],  x3, y3);
    SET_POINT(points[12],  x4,  y4);    SET_POINT(points[13], RGT, y4);

    SET_POINT(points[14], LFT, BOT);    SET_POINT(points[15], RGT, BOT);

    /* draw the innards of the station */
    draw_polygon(points, (UInt8*)top_list, 8, TD_COL_GREY_5);
    draw_polygon(points, (UInt8*)l2_list,  4, TD_COL_GREY_2);
    draw_polygon(points, (UInt8*)l1_list,  4, TD_COL_GREY_3);
    draw_polygon(points, (UInt8*)r1_list,  4, TD_COL_GREY_3);
    draw_polygon(points, (UInt8*)r2_list,  4, TD_COL_GREY_2);
    draw_polygon(points, (UInt8*)bot_list, 8, TD_COL_GREY_5);

    gfx_display_text(DSP_CENTER, DSP_HEIGHT-GFX_CELLS(2,1), ALIGN_CENTER,    
			     "AUTOMATIC LAUNCH",   GFX_COL_GOLD);
    gfx_display_text(DSP_CENTER, DSP_HEIGHT-GFX_CELLS(1,1), ALIGN_CENTER,
			     "SEQUENCE IN PROGRESS", GFX_COL_GOLD);

    /* make sure, other ships don't roll in the dock */
    flight_roll = FLIGHT_SCALE(0);

    /* this is to make the engines of parked ships be off */
    engine_idx = 4;
    update_universe ();

    draw_laser_sights();

    gfx_update_screen ();

    /* the keyboard is polled to give the appl stop event a chance */
    kbd_poll_keyboard ();
  }
#endif

  /* go into ordinary flight mode */
  is_docked = 0;
  flight_speed = FLIGHT_SCALE(12);
  flight_roll  = FLIGHT_SCALE(-15);
  flight_climb = FLIGHT_SCALE(0);
  cmdr.legal_status |= carrying_contraband ();

  clear_universe ();
  set_init_matrix (rotmat);

  add_new_ship (SHIP_PLANET, 0, 0, 65536, rotmat, 0, 0);

  rotmat[2].x = -rotmat[2].x;
  rotmat[2].y = -rotmat[2].y;
  rotmat[2].z = -rotmat[2].z;
  add_new_station (0, 0, -256, rotmat);

  gfx_update_button_map(0);
}

void
display_docking_sequence(void) {
  int i, x1,x2,y2,y3,y4,y5;
  Int32 j;
  RotMatrix rotmat;
  int newship;
  struct point2d points[16];
  const UInt8 bg_list[]={0,1,15,14};
  const UInt8 l_list[]={2,3,9,8};
  const UInt8 r_list[]={6,7,13,12};
  const UInt8 c_list[]={4,5,11,10};
  const int pos[2]={-100,100};

  /* stop rumble */
  if(rumble_cnt > 0) {
    rumble_cnt = 0;
    rumble(0);
  }

#ifndef NO_SEQUENCES 
  current_screen = SCR_FRONT_VIEW;
  gfx_update_button_map(NO_BUTTONS);

  clear_universe();
  set_init_matrix(rotmat);

  /* try to add two ships */
  for(i=0;i<2;i++) {

    newship = get_random_ship();
    if(newship != 0) {
      newship = add_new_ship(newship,  pos[i], -40, 1000, rotmat, 0, 0);

      /* remove this and parked hostile ships will attack :-) */
      universe[newship].flags = 0;
    }
  }

  flight_speed = FLIGHT_SCALE(12);
  flight_roll  = FLIGHT_SCALE(0);
  flight_climb = FLIGHT_SCALE(0);

  for (i = 0; (!finish) && (i < 100); i++) {
 
    gfx_clear_display();

    j = 10000l/(100l-i);
    x1 = DSP_WIDTH/2 - j*DSP_WIDTH/600;    
    x2 = DSP_WIDTH/2 + j*DSP_WIDTH/600;

    y2 = DSP_HEIGHT/2 - j*DSP_HEIGHT/1200;
    y3 = DSP_HEIGHT/2 - j*DSP_HEIGHT/2000;  
    y4 = DSP_HEIGHT/2 + j*DSP_HEIGHT/2000;
    y5 = DSP_HEIGHT/2 + j*DSP_HEIGHT/1200;  

    #define Y1  (DSP_HEIGHT/2 - DSP_HEIGHT/4)
    #define Y6  (DSP_HEIGHT/2 + DSP_HEIGHT/4)

    /* set all 16 points of the docking window */
    SET_POINT(points[0],  LFT, TOP);    SET_POINT(points[1], RGT, TOP);

    SET_POINT(points[2],  LFT,  Y1);    SET_POINT(points[3],   x1, y2);
    SET_POINT(points[4],   x1,  y3);    SET_POINT(points[5],   x2, y3);
    SET_POINT(points[6],   x2,  y2);    SET_POINT(points[7],  RGT, Y1);

    SET_POINT(points[8],  LFT,  Y6);    SET_POINT(points[9],   x1, y5);
    SET_POINT(points[10],  x1,  y4);    SET_POINT(points[11],  x2, y4);
    SET_POINT(points[12],  x2,  y5);    SET_POINT(points[13], RGT, Y6);

    SET_POINT(points[14], LFT, BOT);    SET_POINT(points[15], RGT, BOT);

    /* draw the innards of the station */
    draw_polygon(points, (UInt8*)bg_list,  4, TD_COL_GREY_5);
    draw_polygon(points, (UInt8*)c_list,   4, TD_COL_GREY_4);
    draw_polygon(points, (UInt8*)l_list,   4, TD_COL_GREY_3);
    draw_polygon(points, (UInt8*)r_list,   4, TD_COL_GREY_3);

    gfx_display_text(DSP_CENTER, DSP_HEIGHT-GFX_CELLS(2,1), ALIGN_CENTER,
			     "AUTOMATIC DOCKING",    GFX_COL_GOLD);
    gfx_display_text(DSP_CENTER, DSP_HEIGHT-GFX_CELLS(1,1), ALIGN_CENTER,  
			     "SEQUENCE IN PROGRESS", GFX_COL_GOLD);

    /* this is to make the engines of parked ships be off */
    engine_idx = 4;
    update_universe ();

    draw_laser_sights();

    gfx_update_screen ();

    /* the keyboard is polled to give the appl stop event a chance */
    kbd_poll_keyboard ();
  }
#endif

  gfx_draw_scanner();

  /* we have just docked */
  update_console (1);

  check_mission_brief ();

  /* for the missions it make more sense to go to the planet info first */
//  display_commander_status ();
  display_data_on_planet();
}

void
run_escape_sequence (void)
{
  int i;
  int newship;
  RotMatrix rotmat;

  current_screen = SCR_ESCAPE_POD;

  flight_speed = FLIGHT_SCALE(1);
  flight_roll  = FLIGHT_SCALE(0);
  flight_climb = FLIGHT_SCALE(0);

  set_init_matrix(rotmat);
  rotmat[2].z = ANGLE(1.0);

  newship = add_new_ship (SHIP_COBRA3, 0, 0, 200, rotmat, -127, -127);
  universe[newship].velocity = 7;
  snd_play_sample (SND_LAUNCH);

  for (i = 0; (i < 90)&&(!finish); i++) {
    if (i == 40) {
      universe[newship].flags |= FLG_DEAD;
      snd_play_sample(SND_EXPLODE);
    }

    gfx_clear_display();
    update_starfield();
    update_universe();

    universe[newship].location.x = 0;
    universe[newship].location.y = 0;
    universe[newship].location.z += 2;

    gfx_display_text(DSP_CENTER, 20, ALIGN_CENTER,
		     "Escape pod launched", GFX_COL_WHITE);

    gfx_display_text(DSP_CENTER, 20+GFX_CELL_HEIGHT, ALIGN_CENTER,
		     "Ship auto-destruct initiated", GFX_COL_WHITE);

    update_console (1);
    gfx_update_screen ();

    kbd_poll_keyboard ();
  }

  while((ship_count[SHIP_CORIOLIS] == 0) && 
	(ship_count[SHIP_DODEC] == 0) &&
	(!finish)) {
    auto_dock();

    if ((abs (flight_roll) < FLIGHT_SCALE(3)) && 
	(abs (flight_climb) < FLIGHT_SCALE(3))) {
      for (i = 0; i < MAX_UNIV_OBJECTS; i++) {
	if (universe[i].type != 0)
	  universe[i].location.z -= 1500;
      }
    }

    warp_stars = 1;
    gfx_clear_display();
    update_starfield ();
    update_universe ();
    update_console (1);
    gfx_update_screen ();

    kbd_poll_keyboard ();
  }

  abandon_ship();

  dock_player ();

  gfx_draw_scanner();
  update_console (1);

  display_commander_status ();
}
