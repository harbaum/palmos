/*
 * palm_main.c - main routines for elite
 */

#include <PalmOS.h>
typedef UInt8 UChar;
#include <GPDLib.h>

#ifdef SONY
#include <SonyCLIE.h>
#endif

#include "sections.h"

#include "Rsc.h"
#include "debug.h"
#include "config.h"
#include "gfx.h"
#include "main.h"
#include "vector.h"
#include "elite.h"
#include "docked.h"
#include "intro.h"
#include "shipdata.h"
#include "space.h"
#include "sound.h"
#include "threed.h"
#include "swat.h"
#include "random.h"
#include "options.h"
#include "stars.h"
#include "missions.h"
#include "pilot.h"
#include "file.h"
#include "keyboard.h"
#include "splash.h"
#include "sequences.h"
#include "rumble.h"
#include "dialog.h"

#ifdef SBAY
#include "sbay.h"
#endif

/* local function prototypes */
static void initialise_game (void);
static void move_cross (int dx, int dy);

static void arrow_right (void);
static void arrow_left (void);
static void arrow_up (void);
static void arrow_down (void);
static void y_pressed (void);
static void n_pressed (void);
static void d_pressed (void);
static void f_pressed (void);
static void o_pressed (void);

static Boolean my_isalpha (char chr);
static void handle_flight_keys (void);
static void set_commander_name (char *path);

static Boolean StartApplication (void);
static void StopApplication (void);

static void EventLoop (void);

Boolean color_gfx;

int old_cross_x, old_cross_y;

Bool draw_lasers, missile_fire_block;
UInt8 mcount;

/* temporary message */
UInt8 message_count=0;
char message_string[32];

Bool rolling;
Bool climbing;

#ifdef SONY
UInt16 hrLibRef = 0;
#endif

/*
 * Initialise the game parameters.
 */

static void
initialise_game (void)
{
  set_rand_seed (TimGetTicks ());
  current_screen = SCR_INTRO_ONE;

  restore_jameson();

  missile_fire_block = 0;
  flight_speed = FLIGHT_SCALE(0);
  flight_roll  = FLIGHT_SCALE(0);
  flight_climb = FLIGHT_SCALE(0);
  is_docked = 1;
  front_shield = 255;
  aft_shield = 255;
  energy = 255;
  draw_lasers = 0;
  mcount = 0;
  hyper_ready = 0;
  detonate_bomb = 0;
  witchspace = 0;
  auto_pilot = 0;

  create_new_stars ();
  clear_universe ();

  cross_x = -100;
}

/*
 * Move the planet chart cross hairs to specified position.
 */

void
move_cross (int dx, int dy)
{
  snd_click();

  if (current_screen == SCR_SHORT_RANGE) {
    cross_x = dx;
    cross_y = dy;
  } else {
    cross_x = dx;
    cross_y = dy;

    if (cross_x < 1)            cross_x = 1;
    if (cross_x > DSP_WIDTH-2)  cross_x = DSP_WIDTH-2;

    if (cross_y < 9)            cross_y = 9;
    if (cross_y > 88)           cross_y = 88;
  }

  show_distance_to_planet();
}

void
draw_laser_sights (void)
{
  int laser = 0;
  char *p = NULL;
  const char *strs[] = { "Front View", "Rear View", "Left View", "Right View"};

  switch (current_screen) {
    case SCR_FRONT_VIEW:
      p = (char*)strs[0];
      laser = cmdr.front_laser;
      break;
	
    case SCR_REAR_VIEW:
      p = (char*)strs[1];
      laser = cmdr.rear_laser;
      break;
	
    case SCR_LEFT_VIEW:
      p = (char*)strs[2];
      laser = cmdr.left_laser;
      break;
    
    case SCR_RIGHT_VIEW:
      p = (char*)strs[3];
      laser = cmdr.right_laser;
      break;
  }

  if((!message_count)&&(p))
    gfx_display_text(DSP_CENTER, MESSAGE_Y, ALIGN_CENTER, p, GFX_COL_WHITE);

  /* draw laser cross */
  if (laser) {
    /* hor left */
    gfx_draw_colour_line (DSP_WIDTH/2-16, DSP_HEIGHT/2, 
			  DSP_WIDTH/2-8,  DSP_HEIGHT/2,    GFX_COL_WHITE);
    /* hor right */
    gfx_draw_colour_line (DSP_WIDTH/2+8,  DSP_HEIGHT/2, 
			  DSP_WIDTH/2+16, DSP_HEIGHT/2,    GFX_COL_WHITE);
    /* ver top */
    gfx_draw_colour_line (DSP_WIDTH/2,    DSP_HEIGHT/2-16, 
			  DSP_WIDTH/2,    DSP_HEIGHT/2-8,  GFX_COL_WHITE);
    /* ver bot */
    gfx_draw_colour_line (DSP_WIDTH/2,    DSP_HEIGHT/2+8, 
			  DSP_WIDTH/2,    DSP_HEIGHT/2+16, GFX_COL_WHITE);
  }

  /* if missile targetted draw missile thingy */
  if(missile_target >= 0)
    gfx_draw_circle(DSP_WIDTH/2, DSP_HEIGHT/2, 10, GFX_COL_RED); 
  else if(missile_target == MISSILE_ARMED)
    gfx_draw_circle(DSP_WIDTH/2, DSP_HEIGHT/2, 10, GFX_COL_DARK_RED); 
}


static void
arrow_right (void)
{
  switch (current_screen) {

  case SCR_FRONT_VIEW:
  case SCR_REAR_VIEW:
  case SCR_RIGHT_VIEW:
  case SCR_LEFT_VIEW:
    if (flight_roll > 0)
      flight_roll = 0;
    else {
      decrease_flight_roll(2);
      rolling = 1;
    }
    break;
  }
}


static void
arrow_left (void)
{
  switch (current_screen) {
  case SCR_FRONT_VIEW:
  case SCR_REAR_VIEW:
  case SCR_RIGHT_VIEW:
  case SCR_LEFT_VIEW:
    if (flight_roll < 0)
      flight_roll = 0;
    else {
      increase_flight_roll(2);
      rolling = 1;
    }
    break;
  }
}


static void
arrow_up (void)
{
  switch (current_screen) {
  case SCR_FRONT_VIEW:
  case SCR_REAR_VIEW:
  case SCR_RIGHT_VIEW:
  case SCR_LEFT_VIEW:
    if (flight_climb > 0)
      flight_climb = 0;
    else
      decrease_flight_climb(1);

    climbing = 1;
    break;
  }
}



static void
arrow_down (void)
{
  switch (current_screen) {

  case SCR_FRONT_VIEW:
  case SCR_REAR_VIEW:
  case SCR_RIGHT_VIEW:
  case SCR_LEFT_VIEW:
    if (flight_climb < 0)
      flight_climb = 0;
    else
      increase_flight_climb(1);

    climbing = 1;
    break;
  }
}

/* handle screen taps */
static void
screen_tapped(void) {
  if((kbd_tap[KBD_TAP_Y] < DSP_HEIGHT)&&
     (kbd_tap[KBD_TAP_Y] > TITLE_LINE_Y)) {

    switch (current_screen) {
      case SCR_OPTIONS:
	if((kbd_tap[KBD_TAP_Y] >= OPTION_Y) && 
	   (kbd_tap[KBD_TAP_Y] <  OPTION_Y + OPTIONS*(GFX_CELL_HEIGHT+2)) &&
	   (kbd_tap[KBD_TAP_X] >= LOAD_TXT_X) &&
	   (kbd_tap[KBD_TAP_X] <  LOAD_TXT_X+10) &&
	   (kbd_button_map & KBD_TAP))
	  do_option((kbd_tap[KBD_TAP_Y]-OPTION_Y)/(GFX_CELL_HEIGHT+2));

	if((kbd_tap[KBD_TAP_Y] >= SAVE_Y-2) && 
	   (kbd_tap[KBD_TAP_Y] <  SAVE_Y-2+GFX_CELL_HEIGHT+1) &&
	   (kbd_tap[KBD_TAP_X] >= SAVE_TXT_X) &&
	   (kbd_tap[KBD_TAP_X] <  SAVE_TXT_X+TXT_W) &&
	   (kbd_button_map & KBD_TAP))
	  do_new_cmdr();

	if((kbd_tap[KBD_TAP_Y] >= SAVE_Y + GFX_CELL_HEIGHT + 2) && 
	   (kbd_tap[KBD_TAP_Y]<  SAVE_Y+(SAVE_SLOTS+1)*(GFX_CELL_HEIGHT+2)) &&
	   (kbd_button_map & KBD_TAP))
	  do_save_slot(((kbd_tap[KBD_TAP_Y]-SAVE_Y)/(GFX_CELL_HEIGHT+2))-1,
		       kbd_tap[KBD_TAP_X]);
	break;

#ifdef SBAY
      case SCR_SBAY:
	sbay_handle_tap(kbd_tap[KBD_TAP_X], kbd_tap[KBD_TAP_Y]);
	break;
#endif

      case SCR_MARKET_PRICES:
	if      ((kbd_tap[KBD_TAP_X] >= 138-16) && 
		 (kbd_tap[KBD_TAP_X] <= 138-9))
	  sell_stock((kbd_tap[KBD_TAP_Y]-TITLE_LINE_Y-1)/GFX_CELL_HEIGHT);
	else if ((kbd_tap[KBD_TAP_X] >= 138-4) && 
		 (kbd_tap[KBD_TAP_X] <= 138+2))
	  buy_stock((kbd_tap[KBD_TAP_Y]-TITLE_LINE_Y-1)/GFX_CELL_HEIGHT);
#ifdef SBAY
	else if ((kbd_tap[KBD_TAP_X] >= 138-10) && 
		 (kbd_tap[KBD_TAP_X] <= 138-5) &&
		 sbay_enabled)
	  sbay_sell_stock((kbd_tap[KBD_TAP_Y]-TITLE_LINE_Y-1)/GFX_CELL_HEIGHT);
#endif
	break;

      case SCR_EQUIP_SHIP:
	buy_equip(kbd_tap[KBD_TAP_Y], kbd_tap[KBD_TAP_X]);
	break;

      case SCR_GALACTIC_CHART:
      case SCR_SHORT_RANGE:
	if(handle_map_tap(kbd_tap[KBD_TAP_X], kbd_tap[KBD_TAP_Y]))
	  return;

	move_cross (kbd_tap[KBD_TAP_X], kbd_tap[KBD_TAP_Y]);
	break;
    }
  }
}

static Boolean
my_isalpha (char chr) {
  return (((chr >= 'a') && (chr <= 'z')) || ((chr >= 'A') && (chr <= 'Z')));
}

static void
handle_flight_keys (void)
{
  int keyasc;
  static Boolean jump_warp_enabled = 0;

  kbd_poll_keyboard ();

  /* launch via gamepad */
  if((kbd_gamepad & GAMEPAD_START)&&(is_docked))
    launch_player ();

#ifdef SONY
  /* use sony back key to toggle between front and rear view */
  if(kbd_sony & KBD_SONY_BACK) {
    if(current_screen == SCR_FRONT_VIEW) {
      current_screen = SCR_REAR_VIEW;
      gfx_update_button_map(1);
      flip_stars ();
    } else if(current_screen == SCR_REAR_VIEW) {
      current_screen = SCR_FRONT_VIEW;
      gfx_update_button_map(0);
      flip_stars ();
    }
  }
#endif

  if (kbd_funkey_map & KBD_F1) {
    if (is_docked)
      launch_player ();
    else {
      if (current_screen != SCR_FRONT_VIEW) {
	current_screen = SCR_FRONT_VIEW;
	gfx_update_button_map(0);
	flip_stars ();
      }
    }
  }

  if (kbd_funkey_map & KBD_F2) {
    if (!is_docked) {
      if (current_screen != SCR_REAR_VIEW) {
	current_screen = SCR_REAR_VIEW;
	gfx_update_button_map(1);
	flip_stars ();
      }
    }
  }

  /* toggle front/rear view with m550 center button */
  if((!is_docked)&&(kbd_button_map & KBD_TOGGLE)) {
    if(current_screen == SCR_FRONT_VIEW) {
      current_screen = SCR_REAR_VIEW;
      gfx_update_button_map(1);
      flip_stars ();
    } else if(current_screen == SCR_REAR_VIEW) {
      current_screen = SCR_FRONT_VIEW;
      gfx_update_button_map(0);
      flip_stars ();
    }    
  }

  if (kbd_funkey_map & KBD_F3) {
    if (!is_docked) {
      if (current_screen != SCR_LEFT_VIEW) {
	current_screen = SCR_LEFT_VIEW;
	gfx_update_button_map(2);
	flip_stars ();
      }
    }
  }

  if (kbd_funkey_map & KBD_F4) {
    if (is_docked)
      equip_ship ();
    else {
      if (current_screen != SCR_RIGHT_VIEW) {
	current_screen = SCR_RIGHT_VIEW;
	gfx_update_button_map(3);
	flip_stars ();
      }
    }
  }

  if (kbd_funkey_map & KBD_F5) {
    old_cross_x = -100;

    /* toggle between both chart types and always return to the last one */
    if(current_screen == SCR_GALACTIC_CHART)
      display_short_range_chart ();
    else if(current_screen == SCR_SHORT_RANGE)
      display_galactic_chart ();
    else {
      if(last_chart == SCR_GALACTIC_CHART)
	display_galactic_chart ();
      else
	display_short_range_chart ();
    }
  }

  if (kbd_funkey_map & KBD_F6) {
    display_data_on_planet ();
  }

  if ((kbd_funkey_map & KBD_F7) && (!witchspace)) {
    display_market_prices ();
  }

  if (kbd_funkey_map & KBD_F8) {
    display_commander_status ();
  }

  if (kbd_funkey_map & KBD_F9) {
    display_inventory ();
  }

  if (((kbd_funkey_map & KBD_F10)
       || (kbd_button_map & KBD_MSLE)
#ifdef SONY
       || (kbd_sony & KBD_SONY_PUSH)
#endif
       ) &&(!is_docked)) {

    /* arm if not armed */
    if(missile_target == MISSILE_UNARMED)
      arm_missile();
    else
      unarm_missile();
  }

  if((kbd_funkey_map & KBD_F11) && (!is_docked) && (cmdr.ecm)) {
    activate_ecm(1);
  }

  if ((kbd_funkey_map & KBD_F13) && (!is_docked)) {
    start_galactic_hyperspace ();
  }

  if ((kbd_funkey_map & KBD_F14) && (!is_docked)) {
    start_hyperspace ();
  }

  if (((kbd_funkey_map & KBD_F15)&&(!is_docked) && (!witchspace))||
      (jump_warp_enabled)) {

    /* display a message on button press */
    if(kbd_funkey_map & KBD_F15) {
      /* toggle jump warp */
      if(jump_warp_enabled) {
	info_message("Jump warp disabled");
	jump_warp_enabled = 0;
      } else {
	info_message("Jump warp enabled");
	jump_warp_enabled = 1;
      }
    }

    if(jump_warp_enabled) {
      /* jump warp returns true if jump may continue */
      jump_warp_enabled = jump_warp ();
    }
  }

  if (kbd_button_map & KBD_MENU) {
    /* enter options if not already there */
    if(current_screen != SCR_OPTIONS)
      display_options ();
    else {
      /* go to status view if docked, front view otherwise */
      if(is_docked)
	display_commander_status ();
      else {
	current_screen = SCR_FRONT_VIEW;
	gfx_update_button_map(0);
      }
    }
  }

  if((kbd_button_map & KBD_FIRE)&&(!is_docked)) {
    if(missile_target < 0) {

      /* if a missile has been fired, button has to be released */
      /* to use laser */
      if ((draw_lasers == 0)&&(!missile_fire_block))
	draw_lasers = fire_laser ();
    } else {
      fire_missile ();
      missile_fire_block = 1;
    }
  } else 
    missile_fire_block = 0;

  if (kbd_funkey_map & KBD_F16) {
    if (!is_docked && cmdr.docking_computer) {
      if (!auto_pilot)
	/* no docking on angry stations */
	if (((ship_count[SHIP_CORIOLIS] == 0) && 
	     (ship_count[SHIP_DODEC] == 0)) ||
	    !(universe[1].flags & FLG_ANGRY)) {

	  engage_auto_pilot ();
	} else 
	  info_message ("Access to Station Denied");
      else {
	disengage_auto_pilot();
	info_message ("Docking Computers Off");
      }
    }
  }

  /* special palm speed handling */
  if(!is_docked) {
    static UInt8 speed_state = 0;

    /* handle gamepad buttons */
    if(kbd_gamepad & GAMEPAD_SELECT)
      flight_speed -= ticks*5;

    if(kbd_gamepad & GAMEPAD_START)
      flight_speed += ticks*5;

#ifdef SONY
    if(kbd_sony & KBD_SONY_UP)
      flight_speed += ticks*5*SONY_STEP;

    if(kbd_sony & KBD_SONY_DOWN)
      flight_speed -= ticks*5*SONY_STEP;
#endif

    /* change direction on button press */
    if(kbd_button_map & KBD_SPEED) {
      if(!(speed_state & 1)) speed_state ^= 2;    

      if(speed_state & 2)
	flight_speed += ticks*5;
      else
	flight_speed -= ticks*5;

      speed_state |= 1;
    } else 
      speed_state &= ~1;

    /* speed buttons from m550 mapping */
    if(kbd_button_map & KBD_SPINC)
      flight_speed += ticks*5;

    if(kbd_button_map & KBD_SPDEC)
      flight_speed -= ticks*5;

    /* limit flight speed */
    if(flight_speed > MAX_SPEED)       flight_speed = MAX_SPEED;
    if(flight_speed < FLIGHT_SCALE(1)) flight_speed = FLIGHT_SCALE(1);
  }

  if(kbd_button_map & KBD_UP)    arrow_up ();
  if(kbd_button_map & KBD_DOWN)  arrow_down ();
  if(kbd_button_map & KBD_LEFT)  arrow_left ();
  if(kbd_button_map & KBD_RIGHT) arrow_right ();

  if (kbd_button_map & KBD_TAP_REPEAT) 
    screen_tapped();
    
  /* F12 detonates energy bomb and launches escape capsule */
  if ((kbd_funkey_map & KBD_F12) && (!is_docked)) {
    
    /* first priority: energy bomb */
    if(cmdr.energy_bomb) {
      const struct dialog ebomb = {
	DLG_ALERT, DLG_EBOMB,
	"Energy bomb",
	"Detonate energy bomb, commander?",
	NULL,
	"Yes",
	"No"
      };      
      dialog_draw(&ebomb);

    } else if((cmdr.escape_pod) && (!witchspace)) {
      const struct dialog escape = {
	DLG_ALERT, DLG_ESCAPE,
	"Escape capsule",
	"Really abondon ship, commander?",
	NULL,
	"Yes",
	"No"
      };
      
      dialog_draw(&escape);
    }
  }
}

void
info_message (char *message)
{
  /* message starting with * contains damage info */

  if(message[0] != '*') {
    /* check if message has changed */
    if(StrCompare(message_string, message) != 0) {
      snd_play_sample (SND_BEEP);
      StrCopy (message_string, message);
    }
  } else {
    /* check if message has changed */
    if(StrCompare(message_string+3, message+3) != 0) {
      snd_play_sample (SND_BEEP);
      MemMove(message_string, message, 3);
      StrCopy (message_string+3, message+3);
    }
  }

  /* re-init message counter */
  message_count = 37;
}


static Boolean
StartApplication (void)
{
  UInt32 depth, width, height;
  Err err;

  DEBUG_INIT;

  if(GetRomVersion() < PalmOS30) {
    FrmCustomAlert(alt_err, "Palm OS 3.3 or higher required.", 0, 0);
    return false;
  }

#ifdef SONY
  /* first try it the sony way ... */
  err = SysLibFind(sonySysLibNameHR, &hrLibRef);
  /* lib not loaded, try to load it */
  if(err == sysErrLibNotFound) {
    err = SysLibLoad('libr', sonySysFileCHRLib, &hrLibRef);
  }
  
  if(err) hrLibRef = 0;
  else {
    err = HROpen(hrLibRef);
    
    if(!err) {
      /* try to switch to real 160 x 160 mode */
      width = 160;
      height = 160;
      depth = 8;		  // 8 bit - 256 different combinations with color
      color_gfx = true;
      
      // try to change to the correct screen mode
      if(err = HRWinScreenMode(hrLibRef, winScreenModeSet, &width, &height, &depth, &color_gfx)) {
      
	width = 160;
	height = 160;
	depth = 4;		  // 4 bit - 16 different grayshades
	color_gfx = false;
	
	// then perhaps grayscale?
	err = HRWinScreenMode(hrLibRef, winScreenModeSet, &width, &height, &depth, &color_gfx);
      }
    }
  }

  /* ... if the sony way failed use the palm way ... */
  if(err)
#endif
  {
    /* unable to open sony specific lib */
    DEBUG_PRINTF("no sony extension present\n");
    
    /* try to switch to 160x160x8bpp color mode */
    width = 160;
    height = 160;
    depth = 8;		  // 8 bit - 256 different combinations with color
    color_gfx = true;
    
    // try to change to the correct screen mode
    if(WinScreenMode(winScreenModeSet, &width, &height, &depth, &color_gfx)) {
      
      width = 160;
      height = 160;
      depth = 4;		  // 4 bit - 16 different grayshades
      color_gfx = false;
      
      // then perhaps grayscale?
      if(WinScreenMode(winScreenModeSet, &width, &height, &depth, &color_gfx)) {
	FrmCustomAlert(alt_err, "The required video mode "
		       "is not supported on this device.", 0, 0);
	
	return false;
      }
    }
  } 

  /* try to init rumble hardware */
  rumble_open();

#ifdef SBAY
  /* try to open ir system */
  sbay_init();
#endif

  /* load game options */
  load_options();

  return true;
}

static void
StopApplication (void)
{
  if(GetRomVersion() >= PalmOS30) {
    /* save game options */
    save_options();

#ifdef SBAY
    /* close the sbay system */
    sbay_free();
#endif

    /* close rumble hardware */
    if(rumble_cnt != 0) rumble(0);
    rumble_close();

#ifdef SONY
    /* close sonys hires lib */
    if(hrLibRef != 0) {
      DEBUG_PRINTF("switching back to system settings with hrlib\n");
      HRWinScreenMode(hrLibRef, winScreenModeSetToDefaults, NULL, NULL, NULL, NULL);  

      HRClose(hrLibRef);
    } else 
#endif
    {
      DEBUG_PRINTF("switching back to system settings without hrlib\n");
      WinScreenMode(winScreenModeSetToDefaults, NULL, NULL, NULL, NULL);  
    }
  }

  DEBUG_FREE;
}

UInt32
PilotMain (UInt16 cmd, void *cmdPBP, UInt16 launchFlags)
{
  if (cmd == sysAppLaunchCmdNormalLaunch) {
    if (StartApplication ()) {

      /* Do any setup necessary for the keyboard... */
      kbd_keyboard_startup ();
      finish = 0;

      if (gfx_graphics_startup ()) {

	/* Start the sound system... */
	snd_sound_startup ();

	auto_pilot = 0;

	while (!finish) {
	  game_over = 0;
	  initialise_game ();
	  dock_player ();

	  update_console (1);

	  current_screen = SCR_FRONT_VIEW;

#ifdef INTRO1
	  run_first_intro_screen ();
#endif
#ifdef INTRO2
	  if (!finish)
	    run_second_intro_screen ();
#endif

	  old_cross_x = -100;
	  
	  dock_player ();
	  display_commander_status ();

#ifdef LIMIT
	  /* jump to limit code */
	  if(! (((int(*)(void))(get_text(LIMIT_OFFSET))) ()) ) {
	    mission_limit();
	    finish = 1;
	  }
#endif
	  
	  while ((!game_over)&&(!finish)) {
	    gfx_update_screen ();

	    rolling = 0;
	    climbing = 0;

	    handle_flight_keys ();

	    if(auto_pilot)
	      auto_dock ();

	    if (message_count > 0)
	      message_count--;

	    /* damping on roll */
	    if (!rolling) {
	      if (flight_roll > FLIGHT_SCALE(0))
		if(flight_roll > FLIGHT_SCALE(3))
		  flight_roll -= FLIGHT_SCALE(3);
		else
		  flight_roll = 0;
	      
	      if (flight_roll < FLIGHT_SCALE( 0)) 
		if(flight_roll < FLIGHT_SCALE(-3))
		  flight_roll += FLIGHT_SCALE(3);
		else
		  flight_roll = 0;
	    }
	    
	    /* damping on climb */
	    if (!climbing) {
	      if (flight_climb > FLIGHT_SCALE(0))
		if(flight_climb > FLIGHT_SCALE(2))
		  flight_climb -= FLIGHT_SCALE(2);
		else
		  flight_climb = 0;
	      
	      if (flight_climb < FLIGHT_SCALE(0))
		if(flight_climb < FLIGHT_SCALE(-2))
		  flight_climb += FLIGHT_SCALE(2);
		else
		  flight_climb = 0;
	    }
	      
	    if (!is_docked) {

	      if ((current_screen == SCR_FRONT_VIEW)
		  || (current_screen == SCR_REAR_VIEW)
		  || (current_screen == SCR_LEFT_VIEW)
		  || (current_screen == SCR_RIGHT_VIEW)
		  || (current_screen == SCR_INTRO_ONE)
		  || (current_screen == SCR_INTRO_TWO)
		  || (current_screen == SCR_GAME_OVER)) {

		gfx_clear_display();

		update_starfield ();
	      }

	      if (auto_pilot) {
		/* refresh message if previous message is about */
		/* to vanish */
		if(message_count < 5)
		  info_message("Docking Computers On");
	      }

	      update_universe ();

	      if (is_docked) {
		update_console (1);
		continue;
	      }

	      if ((current_screen == SCR_FRONT_VIEW) ||
		  (current_screen == SCR_REAR_VIEW) ||
		  (current_screen == SCR_LEFT_VIEW) ||
		  (current_screen == SCR_RIGHT_VIEW)) {

		if(draw_lasers) {
		  draw_laser_lines();
		  draw_lasers--;
		}
		
		draw_laser_sights();
		
		if(message_count > 0) {
	          if(message_string[0] != '*') 
		    gfx_display_text(DSP_CENTER, MESSAGE_Y, ALIGN_CENTER,
				     message_string, GFX_COL_WHITE);
		  else {
	            /* special enemy energy display */
		    gfx_display_text(DSP_CENTER, MESSAGE_Y, ALIGN_CENTER,
				     message_string+3, GFX_COL_WHITE);
		    
		    /* draw border of energy bar */
		    gfx_draw_rectangle((DSP_WIDTH-message_string[2])/2 - 2, 
		      MESSAGE_Y - GFX_CELL_HEIGHT - 3,
		      (DSP_WIDTH-message_string[2])/2 + message_string[2] + 2,
		      MESSAGE_Y - 3, GFX_COL_WHITE);

		    /* draw energy bar */
		    if(message_string[1] > 0) {
		      gfx_fill_rectangle((DSP_WIDTH-message_string[2])/2, 
			 MESSAGE_Y - GFX_CELL_HEIGHT - 1,
			 (DSP_WIDTH-message_string[2])/2 + message_string[1],
			 MESSAGE_Y - 5,
			 (message_string[1] > message_string[2]/2) ? 
			 GFX_COL_GREEN_1 : ((message_string[1] > 
			 message_string[2]/8)? GFX_ORANGE_3 : GFX_COL_RED));
		    }
		  }
		}
	      }

	      if(hyper_ready) {
		display_hyper_status ();
		if ((mcount & 3) == 0) {
		  countdown_hyperspace ();
		}
	      }

	      if(rumble_cnt > 0) {
		rumble_cnt--;
		if(rumble_cnt == 0)
		  rumble(0);
	      }

	      mcount--;

// not required with UInt8:
//	      if (mcount < 0)
//		mcount = 255;

	      if ((mcount & 7) == 0)
		regenerate_shields ();

	      if ((mcount & 31) == 10) {
		if (energy < 50) {
		  info_message ("ENERGY LOW");
		  snd_play_sample (SND_BEEP);
		}

		update_altitude();
	      }

	      if ((mcount & 31) == 20)
		update_cabin_temp();

	      if ((mcount == 0) && (!witchspace))
		random_encounter();

	      cool_laser();                             
	      time_ecm();

	      update_console (0);
	    }

	    switch(current_screen) {
	      case SCR_CMDR_STATUS:
		display_game_timer();
		break;

	      case SCR_LAUNCH_SEQUENCE:
		display_launch_sequence();
		break;

	      case SCR_DOCKING_SEQUENCE:
		display_docking_sequence();
		break;

	      case SCR_HYPERSPACE:
		display_break_pattern(0);
		break;

	      case SCR_GALACTIC_HYPERSPACE:
		display_break_pattern(1);
		break;
	    }
	  }

	  if (!finish)
	    run_game_over_screen ();
	}

	/* auto save commander if in dock */
	if(is_docked) {
	  save_commander(AUTO_SAVE_SLOT);
	}

	snd_sound_shutdown ();
      }

      gfx_graphics_shutdown ();

      kbd_keyboard_shutdown();
    }

    StopApplication ();
  }

  return (0);
}
