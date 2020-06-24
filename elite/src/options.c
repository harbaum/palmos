/*
 * Options.c
 *
 * Palm game options:
 * - save/load state
 * - button mapping left/right
 *
 */


#include <PalmOS.h>

#include "elite.h"
#include "config.h"
#include "gfx.h"
#include "options.h"
#include "main.h"
#include "docked.h"
#include "file.h"
#include "debug.h"
#include "space.h"
#include "sound.h"

static void draw_option(int i) SECDK;
static void load_cmdr(int i) SECDK;
static void save_cmdr(int i) SECDK;


EliteOptions options;
Boolean sound_disabled_by_system;

void 
load_options(void) {
  SystemPreferencesType sysPrefs;
  Err err;
  UInt16 size;

  DEBUG_PRINTF("load options\n");

  /* load preferences */
  size = sizeof(EliteOptions);
  err = PrefGetAppPreferences(CREATOR, GENERIC_PREFS, &options, &size, true);

  /* get options from prefs */
  if((err==noPreferenceFound)||(options.version!=OPTIONS_VERSION)) {
    /* no valid prefs */
    options.version = OPTIONS_VERSION;
    options.bool[OPTION_LEFTY]   = 0;
    options.bool[OPTION_SOUND]   = 1;
    options.bool[OPTION_REVERSE] = 0;
    options.bool[OPTION_INVERT]  = 0;
  }

  /* get system preferences for sound */
  PrefGetPreferences(&sysPrefs);
  if((sysPrefs.gameSoundLevelV20 == slOff)&&(options.bool[OPTION_SOUND])) {
    DEBUG_PRINTF("enabled sound overwritten by system setting\n");
    options.bool[OPTION_SOUND]  = 0;
    sound_disabled_by_system = 1;    
  }

}

void 
save_options(void) {

  /* re-enable sound, if it was disabled by the system */
  if(sound_disabled_by_system)
    options.bool[OPTION_SOUND] = 1;

  PrefSetAppPreferences(CREATOR, GENERIC_PREFS, 1, 
			&options, sizeof(EliteOptions), true);
}

void
draw_option(int i) 
{
  const char *option_str[]={
    "Left handed button layout",
    "Enable sound and music",
    "Reverse dive/climb",
    "Inverse grayscale video",
    "Special M550 button mapping",
  };

      
  char str[32];
  int y = OPTION_Y+i*(2+GFX_CELL_HEIGHT);
  
  str[0] = options.bool[i]?'\006':'\005';
  StrCopy(&str[1], "  ");

  /* button string 3/4 are alternate options */
  if((color_gfx)&&(i==3)) i=4;
  StrCat(str, option_str[i]);

  gfx_clear_area(0,y,159,y+GFX_CELL_HEIGHT-1);
  gfx_display_text(LOAD_TXT_X, y, ALIGN_LEFT, str, GFX_COL_WHITE);
}

void draw_option_button(int x1, int y, int x2, char *txt, int c, int bg) {
  gfx_fill_rectangle(x1, y, x2, y+GFX_CELL_HEIGHT, bg);
  gfx_draw_rectangle(x1, y, x2, y+GFX_CELL_HEIGHT, c);
  gfx_display_text(x1+(x2-x1)/2, y+1, ALIGN_CENTER, txt, GFX_COL_WHITE);
}

void
draw_save_slot(int i) 
{
  char name[DLG_MAX_LEN+1+12];
  UInt32 gtime;
  int y = SAVE_Y+(i+1)*(2+GFX_CELL_HEIGHT);
  Err err;
  UInt16 size = DLG_MAX_LEN+1+12;

  gfx_clear_area(0,y,159,y+GFX_CELL_HEIGHT-1);

  err=PrefGetAppPreferences(CREATOR, SAVE_PREFS_0+i, name, &size, true);
  if((size<DLG_MAX_LEN+1+12)||(err==noPreferenceFound)) {
    StrCopy(name, "<unused>");
  } else {  
    /* get game timer from string */
    MemMove(&gtime, &name[(UInt32)&cmdr.timer-(UInt32)&cmdr], sizeof(UInt32));
    StrCat(name, ": ");
    set_time_str(&name[StrLen(name)], gtime);

    /* draw load button */
    draw_option_button(LOAD_TXT_X, y, LOAD_TXT_X+TXT_W, 
		       "LOAD", GFX_COL_GREEN_2, GFX_COL_GREEN_4);
  }

  /* draw slot contents */
  draw_option_button(SAVE_SLOT_X, y, SAVE_SLOT_X+SAVE_SLOT_W,
		     name, GFX_COL_GREY_4, GFX_COL_GREY_0);

  if(is_docked) {
    /* draw save button */
    draw_option_button(SAVE_TXT_X, y, SAVE_TXT_X+TXT_W,
		       "SAVE", GFX_COL_RED, GFX_COL_DARK_RED);
  }
}

void
display_options (void)
{
  int i;

  current_screen = SCR_OPTIONS;
  gfx_update_button_map(NO_MARKER);

  gfx_draw_title("GAME MENU", 1);

  gfx_display_text(DSP_CENTER, TITLE_LINE_Y + 2 + GFX_CELLS(0,1), ALIGN_CENTER,
		   get_text(TXT_VERSION_DATE), GFX_COL_WHITE);
  gfx_display_text(DSP_CENTER, TITLE_LINE_Y + 2 + GFX_CELLS(1,1), ALIGN_CENTER,
		   get_text(TXT_URL), GFX_COL_WHITE);

  for(i=0;i<OPTIONS;i++) draw_option(i);

  /* display the save slots */
  gfx_display_text(LOAD_TXT_X, SAVE_Y-1, ALIGN_LEFT, 
		   "Save game slots:", GFX_COL_WHITE);
  
  /* draw new button */
  draw_option_button(SAVE_TXT_X, SAVE_Y-2, SAVE_TXT_X+TXT_W,
		     "NEW", GFX_COL_BLUE_2, GFX_COL_BLUE_4);

  for(i=0;i<SAVE_SLOTS;i++) draw_save_slot(i);
}

void
do_option(int i) {
  snd_click();

  options.bool[i] = !options.bool[i];

  /* do instant invert */
  if((!color_gfx) && (i == OPTION_INVERT)) {
    if(options.bool[i]) graytab += 256;
    else                graytab -= 256;
    
    gfx_invert_screen();
  }

  draw_option(i);
}

UInt8 rename_slot;

void
do_save_slot(int i, int x) {
  Err err;

  if((x>=LOAD_TXT_X)&&(x<=LOAD_TXT_X+TXT_W)) 
    load_cmdr(i);

  if((x>=SAVE_SLOT_X)&&(x<=SAVE_SLOT_X+SAVE_SLOT_W)) {
    char name[DLG_MAX_LEN+1];
    UInt16 size = DLG_MAX_LEN+1;
    struct dialog rename = {
      DLG_TXTINPUT, DLG_RENAME,
      "Rename Commander",
      "Enter new name:",
      NULL,
      "Ok",
      "Cancel"
    };

    /* fetch name from prefs */
    err=PrefGetAppPreferences(CREATOR, SAVE_PREFS_0+i, name, &size, true);
    if((size<DLG_MAX_LEN+1)||(err==noPreferenceFound)) return;

    rename.init = name;
    rename_slot = i;
    snd_click();
    dialog_draw(&rename);  
  }

  if((x>=SAVE_TXT_X) && (x<=SAVE_TXT_X+TXT_W) && (is_docked))
    save_cmdr(i);
}

void do_new_cmdr(void) {
  char name[DLG_MAX_LEN+1];
  UInt16 size = DLG_MAX_LEN+1;
  
  struct dialog new_name = {
    DLG_TXTINPUT, DLG_NEW,
    "Restart game",
    "Enter your name:",
    NULL,
    "Ok",
    "Cancel"
  };

  snd_click();
  DEBUG_PRINTF("new commander\n");

  StrCopy(name, cmdr.name);
  new_name.init = name;
  dialog_draw(&new_name);  
}

static void 
save_cmdr(int i) {
  snd_click();
  save_commander(i);
  draw_save_slot(i);
}

static void 
load_cmdr(int i) {
  int n;

  if((n = load_commander(i))!= 0) {
    const struct dialog escape = {
      DLG_ALERT, DLG_NOTHING,
      "Error",
      "Unable to restore saved state!",
      NULL,
      "Ok",
      NULL
    };

    /* ignore not found */
    if(n>1) {
      snd_click();
      dialog_draw(&escape);  
    }

    return;
  }

  snd_click();

  dock_player();
  gfx_draw_scanner();
  update_console (1);

  display_commander_status ();
}

