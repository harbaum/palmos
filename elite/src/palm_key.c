/*
 * Elite - The New Kind.
 *
 * Allegro version of the keyboard routines.
 *
 * The code in this file has not been derived from the original Elite code.
 * Written by C.J.Pinder 1999-2001.
 * email: <christian@newkind.co.uk>
 *
 */

/*
 * palm_key.c
 *
 * Code to handle keyboard input.
 */

#include <PalmOS.h>

/* include stuff for palm game pad */
typedef UInt8 UChar;
#include <GPDLib.h>

#ifdef SONY
/* include stuff for Sony Clie */
#include <SonyCLIE.h>
#endif

#include "config.h"
#include "keyboard.h"
#include "elite.h"
#include "debug.h"
#include "gfx.h"
#include "options.h"
#include "Rsc.h"
#include "docked.h"
#include "dialog.h"
#include "sound.h"

#include "silkjoy.h"

/* the palmos-5 sdk does not define these?? */
#define keyBitRecord 0x20000000
#define keyBitLeft   0x01000000
#define keyBitRight  0x02000000
#define keyBitCenter 0x04000000

/* key maps */
UInt16 kbd_button_map = 0;
UInt16 kbd_funkey_map = 0;

/* screen tap */
UInt8  kbd_tap[2];

/* gamepad library interface */
UInt16 gpdRef = 0;
UInt8  kbd_gamepad = 0;

#ifdef SONY
UInt8  kbd_sony = 0;
#endif

static void
release_all (void) {
  kbd_funkey_map = 0;  // no function key pressed
}

/* paramters for silkscreen joysticks */
static Boolean silkjoy_setup_from_panel;
static PointType silkjoy;
static UInt16 silkjoy_radius;
static UInt16 silkjoy_map;
static UInt16 silkjoy_vcenter;
static UInt16 silkjoy_alpha_hcenter;
static UInt16 silkjoy_num_hborder;

struct {
  PointType topLeft;
  PointType bottomRight;
} silk_area;

/* get outer bounds of silkscreen area */
static void get_silk_area(void) {
  UInt16 numAreas, i;
  const SilkscreenAreaType *areas;

  if(GetRomVersion() >= PalmOS35) {
    /* get info on silkscreen area and draw it */
    areas = EvtGetSilkscreenAreaList(&numAreas);
    DEBUG_PRINTF("returned %d areas\n", numAreas);
    
    /* walk through all areas */
    for(i=0;i<numAreas;i++) {
      
      /* fetch graffiti area info only */
      if(areas[i].areaType == 'graf') {
	
	/* add rectangle to graffiti box */
	if(areas[i].index == alphaGraffitiSilkscreenArea) {
	  /* first (alpha) rectangle */
	  silk_area.topLeft.x = areas[i].bounds.topLeft.x;
	  silk_area.topLeft.y = areas[i].bounds.topLeft.y;
	  silk_area.bottomRight.x = 
	    areas[i].bounds.topLeft.x + areas[i].bounds.extent.x;
	  silk_area.bottomRight.y = 
	    areas[i].bounds.topLeft.y + areas[i].bounds.extent.y;	  
	  
	  /* offset to hcenter */
	  silkjoy_alpha_hcenter = areas[i].bounds.extent.x/2;
	  silkjoy_radius        = areas[i].bounds.extent.x/8;
	  silkjoy_vcenter       = areas[i].bounds.topLeft.y + 
	                          areas[i].bounds.extent.y/2;
	} 
	
	if(areas[i].index == numericGraffitiSilkscreenArea) {
	  /* second (num) rectangle */
	  if(silk_area.topLeft.x > areas[i].bounds.topLeft.x)
	    silk_area.topLeft.x = areas[i].bounds.topLeft.x;
	  
	  if(silk_area.topLeft.y > areas[i].bounds.topLeft.y)
	    silk_area.topLeft.y = areas[i].bounds.topLeft.y;
	  
	  if(silk_area.bottomRight.x <
	     areas[i].bounds.topLeft.x + areas[i].bounds.extent.x) {
	    silk_area.bottomRight.x = 
	    areas[i].bounds.topLeft.x + areas[i].bounds.extent.x;
	  }
	  
	  if(silk_area.bottomRight.y <
	     areas[i].bounds.topLeft.y + areas[i].bounds.extent.y) {
	    silk_area.bottomRight.y = 
	      areas[i].bounds.topLeft.y + areas[i].bounds.extent.y;
	  }
	  
	  silkjoy_num_hborder = areas[i].bounds.extent.x;
	}
      }
    }
  } else {
    DEBUG_PRINTF("EvtGetSilkscreenAreaList unsupported\n");
    silk_area.topLeft.x = 27;        silk_area.topLeft.y = 164;
    silk_area.bottomRight.x = 133;   silk_area.bottomRight.y = 220;

    silkjoy_vcenter = 192;    
    silkjoy_alpha_hcenter = 31;
    silkjoy_num_hborder = 44; 
    silkjoy_radius = 7;
  }
}  

Boolean
kbd_keyboard_startup (void) {
  SilkJoyPrefsType p;
  UInt16 size, version;
  Err err;

  DEBUG_PRINTF("load prefs\n");

  /* try to use gamepad */
  err = SysLibFind(GPD_LIB_NAME, &gpdRef);
  if (err == sysErrLibNotFound)
    err = SysLibLoad('libr', GPD_LIB_CREATOR, &gpdRef); 
	  
  /* unable to open gamepad lib? */
  if(err) gpdRef = 0;
  else if(GPDOpen(gpdRef))
    gpdRef = 0;

  /* try to get access to the silkscreen joystick parameters */
  silkjoy_map = 0;
  size = sizeof(SilkJoyPrefsType);
  version = PrefGetAppPreferences(SilkJoyCreator, 0,  &p, &size, true);
  if ((version == SilkJoyPrefsVersion10) && 
      (size == sizeof(SilkJoyPrefsType))) {

    DEBUG_PRINTF("found silkjoy preferences\n");

    silkjoy_setup_from_panel = true;
    silkjoy.x = p.center.x;
    silkjoy.y = p.center.y;
    silkjoy_radius = (2*p.radius)/3;
  } else
    silkjoy_setup_from_panel = false;

  /* get extent of silkscreen area */
  get_silk_area();

  /* Mask hardware keys */
  KeySetMask(keyBitPower | keyBitContrast | keyBitRecord);

  release_all ();

  return 0;
}

Boolean
kbd_keyboard_shutdown (void)
{
  UInt32 numapps;

  /* unmask keys */
  KeySetMask (keyBitsAll);

  /* close gamepad lib if in use */
  if(gpdRef != 0) {
    GPDClose(gpdRef, &numapps);

    /* no application left? close lib! */
    if (numapps == 0)
      SysLibRemove(gpdRef);
  }

  return 0;
}

void
kbd_poll_keyboard (void)
{
  EventType event;
  UInt16 i, j, map;
  UInt32 hardKeyState;
  static UInt32 last_hardKeyState;
  static Boolean pen_is_down = 0;
  char *p;
  static UInt32 off_time = 0;
  static UInt16 tick_wait = 0;
#ifdef LIMIT_EVENT
  UInt16 max_event = 10;
#endif

  /* right and lefty layout */
  const UInt32 buttons[2][4]={
   { keyBitHard1, keyBitHard2, keyBitHard3, keyBitHard4 }, 
   { keyBitHard3, keyBitHard4, keyBitHard2, keyBitHard1 }
  };

  kbd_button_map = 0;
  kbd_funkey_map = 0;
  kbd_sony       = 0;

  /* limit refresh rate to 25 Hz */
  if(ticks<40) {
    tick_wait++;
//	DEBUG_PRINTF("25 Hz refresh limit reached\n");
  } else {
    if(tick_wait > 0)
      tick_wait--;
  }

  /* process until empty event returned (EvtEventAvail does not work) */
  event.eType = 0xffff;
  while((event.eType != nilEvent)&&
	(event.eType != penDownEvent)
#ifdef LIMIT_EVENT
	&&(max_event--)
#endif
    ) {

    if(current_screen & SCR_FLAG_3D) {
      /* get a palmos event */
      EvtGetEvent (&event, tick_wait);  // as fast as possible
    } else
      EvtGetEvent (&event, SysTicksPerSecond()/12);  // slower

    /* catch find chr before system processes them */
    if((!(current_screen & SCR_FLAG_NI))&&
       (event.eType == keyDownEvent)&&
       (event.data.keyDown.chr == findChr)) {
      snd_click();
      find_named_planet();
    } else {

#ifdef DEBUG
      /* the debug video can be stopped using the */
      /* space bar */
      if((event.eType == keyDownEvent)&&
	 (event.data.keyDown.chr == ' '))
	gfx_toggle_update();
#endif

      /* check for silkscreen joystick and process them */
      /* if no text input currently required */
      if((txt_pos < 0)&&
	 ((event.eType == penMoveEvent)||(event.eType == penDownEvent))&&
	 (event.screenX > silk_area.topLeft.x) &&
	 (event.screenY > silk_area.topLeft.y) &&
	 (event.screenX < silk_area.bottomRight.x) &&
	 (event.screenY < silk_area.bottomRight.y)) {
	 
	/* mapping of fire and speed button */
	if((options.bool[OPTION_LEFTY] && 
	    (event.screenX < silk_area.topLeft.x+silkjoy_num_hborder))||
	   (!options.bool[OPTION_LEFTY] && 
	    (event.screenX > silk_area.bottomRight.x-silkjoy_num_hborder))) {

	  /* no direction button */
	  silkjoy_map &= ~(KBD_LEFT | KBD_RIGHT | KBD_UP | KBD_DOWN);

	  /* border between both buttons */
	  if(event.screenY > silkjoy_vcenter) {
	    silkjoy_map |=  KBD_FIRE;
	    silkjoy_map &= ~KBD_SPEED;
	  } else {
	    silkjoy_map |=  KBD_SPEED;
	    silkjoy_map &= ~KBD_FIRE;
	  }
	} else {
	  /* no fire/speed button */
	  silkjoy_map &= ~(KBD_SPEED | KBD_FIRE);
	  
	  /* silkjoy panel specific settings */
	  if(silkjoy_setup_from_panel) {
	    i = silkjoy.x;
	    j = silkjoy.y;
	  } else {
	    /* mapping of direction keys */
	    if(options.bool[OPTION_LEFTY]) 
	      i = silk_area.bottomRight.x - silkjoy_alpha_hcenter;
	    else
	      i = silk_area.topLeft.x + silkjoy_alpha_hcenter;
	    
	    j = silkjoy_vcenter;
	  }
	  
	  /* horizontal mapping */
	  if(event.screenX < i - silkjoy_radius) {
	    /* move left */
	    silkjoy_map |=  KBD_LEFT;
	    silkjoy_map &= ~KBD_RIGHT;
	  } else if(event.screenX > i + silkjoy_radius) {
	    /* move right */
	    silkjoy_map |=  KBD_RIGHT;
	    silkjoy_map &= ~KBD_LEFT;
	  } else {
	    /* dead zone */
	    silkjoy_map &= ~(KBD_LEFT | KBD_RIGHT);
	  }
	  
	  /* vertical mapping */
	  if(event.screenY < j - silkjoy_radius) {
	    /* move up */
	    silkjoy_map |=  KBD_UP;
	    silkjoy_map &= ~KBD_DOWN;
	  } else if(event.screenY > j + silkjoy_radius) {
	    /* move down */
	    silkjoy_map |=  KBD_DOWN;
	    silkjoy_map &= ~KBD_UP;
	  } else {
	    /* dead zone */
	    silkjoy_map &= ~(KBD_UP | KBD_DOWN);
	  }
	}
      } else {
	if (!SysHandleEvent (&event))
	  FrmDispatchEvent (&event);
      }


      switch (event.eType) {
	case keyDownEvent:

	  /* corrent game_timer if palm has been switched */
	  /* off for some time */
	  if(off_time != 0) {
	    game_timer += TimGetSeconds()-off_time;
	    off_time = 0;
	  }

#ifdef SONY
	  /* special sony keys */
	  if(event.data.keyDown.chr == vchrJogUp)   kbd_sony |= KBD_SONY_UP;
	  if(event.data.keyDown.chr == vchrJogDown) kbd_sony |= KBD_SONY_DOWN;
	  if(event.data.keyDown.chr == vchrJogPush) kbd_sony |= KBD_SONY_PUSH;
	  if(event.data.keyDown.chr == vchrJogBack) kbd_sony |= KBD_SONY_BACK;
#endif

	  /* palm is about to be switched off -> save time */
	  if((event.data.keyDown.chr == autoOffChr) ||
	     (event.data.keyDown.chr == powerOffChr))
	    off_time = TimGetSeconds();

	  if(!(event.data.keyDown.chr & 0xff00))
	    dialog_handle_char(event.data.keyDown.chr);

	  if(event.data.keyDown.chr == menuChr) {
	    snd_click();
	    kbd_button_map |= KBD_MENU;
	  }
	  break;
	  
	case penDownEvent:
	  if(event.screenY < 160) {
	    if(dialog_btn_active) {
	      /* tap in dialog box? */
	      if((event.screenX >= DLG_X1)&&
		 (event.screenX <= DLG_X2)&&
		 (event.screenY >= DSP_HEIGHT + DLG_Y1)&&
		 (event.screenY <= DSP_HEIGHT + DLG_Y2))
		dialog_handle_tap(event.screenX, event.screenY);
	      else
		snd_beep();
	    } else {
	      kbd_button_map |= KBD_TAP;
	      pen_is_down = 1;
	    
	      /* save position of screen tap */
	      kbd_tap[KBD_TAP_X] = event.screenX;
	      kbd_tap[KBD_TAP_Y] = event.screenY;
	  
	      if((event.screenY > 160 - FUNKEY_HEIGHT) && 
		 (event.screenY < 160)) {
		i = event.screenX / 10;
		if((i>=0)&&(i<=15)) {
		  snd_click();
		  kbd_funkey_map |= (1<<i);
		}
	      }
	    }
	  }
	  break;

	case penUpEvent:
	  silkjoy_map = 0;
	  pen_is_down = 0;
	  release_all();
	  break;
	  
	case appStopEvent:
	  game_over = 1;
	  finish = 1;
	  break;
      }
    }
  }

  if(pen_is_down) {
//    kbd_button_map |= KBD_TAP_HOLD;
    
    if((pen_is_down == 1)||(pen_is_down == 5))
      kbd_button_map |= KBD_TAP_REPEAT;
    
    if(pen_is_down < 5) pen_is_down++;
    else                pen_is_down=2;
  }

  /* poll gamepad buttons */
  if(gpdRef != 0) {
    static UInt8 last_gp=0;

    /* read gamepad buttons */
    if(!GPDReadInstant(gpdRef, &kbd_gamepad)) {

      /* avoid auto off when playing with gamepad */
      if(kbd_gamepad != last_gp) EvtResetAutoOffTimer();

      /* direction buttons are mapped to standard buttons */
      if(kbd_gamepad & GAMEPAD_UP)        kbd_button_map |= KBD_UP;
      if(kbd_gamepad & GAMEPAD_DOWN)      kbd_button_map |= KBD_DOWN;
      if(kbd_gamepad & GAMEPAD_LEFT)      kbd_button_map |= KBD_LEFT;
      if(kbd_gamepad & GAMEPAD_RIGHT)     kbd_button_map |= KBD_RIGHT;

      /* right fire is mapped as well */
      if(kbd_gamepad & GAMEPAD_RIGHTFIRE) kbd_button_map |= KBD_FIRE;

      /* left fire is mapped to F10 (one time event) */
      if((kbd_gamepad & GAMEPAD_LEFTFIRE)&&!(last_gp & GAMEPAD_LEFTFIRE)) 
	kbd_funkey_map |= KBD_F10;

      /* other keys are handled in main */

      last_gp = kbd_gamepad;
    }
  }
 
  hardKeyState = KeyCurrentState();

  /* get active button map */
  map = options.bool[OPTION_LEFTY];

  /* map hard buttons */

  /* special m550 mapping */
  if((color_gfx) && (options.bool[OPTION_M550BUT])) {
    static UInt8 last_mt = 0;

    kbd_button_map |= silkjoy_map |
      ((hardKeyState & keyBitLeft)    ? KBD_LEFT  : 0) |
      ((hardKeyState & keyBitRight)   ? KBD_RIGHT : 0) |
      (((hardKeyState & keyBitCenter)&&(!(last_mt&1))) ? KBD_TOGGLE: 0) |
      (((hardKeyState & keyBitHard3)&&(!(last_mt&2)))  ? KBD_MSLE  : 0) |
      ((hardKeyState & keyBitHard4)   ? KBD_FIRE  : 0) |
      ((hardKeyState & keyBitHard1)   ? KBD_SPDEC : 0) |
      ((hardKeyState & keyBitHard2)   ? KBD_SPINC : 0) |
      ((hardKeyState & keyBitPageUp)  ? KBD_UP    : 0) |
      ((hardKeyState & keyBitPageDown)? KBD_DOWN  : 0);

    /* these two buttons only generate one shot events */
    if(hardKeyState & keyBitCenter) last_mt |=  1;
    else                            last_mt &= ~1;

    if(hardKeyState & keyBitHard3)  last_mt |=  2;
    else                            last_mt &= ~2;

  } else {
    kbd_button_map |= silkjoy_map |
      ((hardKeyState & buttons[map][0]) ? KBD_LEFT  : 0) |
      ((hardKeyState & buttons[map][1]) ? KBD_RIGHT : 0) |
      ((hardKeyState & buttons[map][2]) ? KBD_FIRE  : 0) |
      ((hardKeyState & buttons[map][3]) ? KBD_SPEED : 0) |
      ((hardKeyState & keyBitPageUp)    ? KBD_UP    : 0) |
      ((hardKeyState & keyBitPageDown)  ? KBD_DOWN  : 0);
  }

  /* revert up/down if user wants to */
  if(options.bool[OPTION_REVERSE]) {
    i = kbd_button_map & KBD_UP;    /* save up */

    kbd_button_map &= ~KBD_UP;      /* replace up with down */
    if(kbd_button_map & KBD_DOWN) 
      kbd_button_map |= KBD_UP;

    kbd_button_map &= ~KBD_DOWN;     /* replace down with up */
    if(i) 
      kbd_button_map |= KBD_DOWN;
  }

  last_hardKeyState = hardKeyState;
}
